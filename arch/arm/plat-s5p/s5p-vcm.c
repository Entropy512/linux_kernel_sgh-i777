#include <asm/pgtable.h>
#include <asm/atomic.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/cma.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <plat/s5p-vcm.h>
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/genalloc.h>

#define PG_FLAG_MASK 0x3
#define PG_LV1_SECTION_FLAG 0x2
#define PG_LV1_PAGE_FLAG 0x1
#define PG_LV2_SPAGE_FLAG 0x2
#define PG_LV2_LPAGE_FLAG 0x1
#define PG_FAULT_FLAG 0

#define PG_LV1_LV2BASE_MASK 0xFFFFFC00
#define PG_LV1_SECTION_ADDR 0xFFF00000

#define LPAGESHIFT 16
#define SPAGESHIFT PAGE_SHIFT
#define SECTIONSHIFT 20
#define KBSHIFT 10

#define SECTIONSIZE (1 << SECTIONSHIFT)
#define LPAGESIZE (1 << LPAGESHIFT)
#define SPAGESIZE (1 << SPAGESHIFT)

#define SECTIONMASK (SECTIONSIZE - 1)
#define LPAGEMASK (LPAGESIZE - 1)
#define SPAGEMASK (SPAGESIZE - 1)

#define MAXSECTIONS (1 << (32 - SECTIONSHIFT))
#define LV1ENTRIES MAXSECTIONS
#define LV1TABLESIZE (LV1ENTRIES * sizeof(int))
#define LV2ENTRIES (1 << (SECTIONSHIFT - SPAGESHIFT))
#define LV2TABLESIZE (LV2ENTRIES * sizeof(int))

#define LV1OFF_FROM_VADDR(addr) (addr >> SECTIONSHIFT)
#define LV2OFF_FROM_VADDR(addr) ((addr & 0xFF000) >> SPAGESHIFT)
#define LV2OFF_FROM_VADDR_LPAGE(addr) ((addr & 0xF0000) >> SPAGESHIFT)

/* slab cache for 2nd level page tables */
static struct kmem_cache *l2pgtbl_cachep;

/* function pointer to vcm_mmu_activate() defined in mm/vcm.c */
static int (*vcm_mmu_activate)(struct vcm *vcm);

struct s5p_vcm_unified {
	struct s5p_vcm_mmu {
		struct vcm_mmu mmu;
		const struct s5p_vcm_driver *driver;
	} *vcms[VCM_DEV_NUM];

	struct gen_pool *pool;
	unsigned long *pgd;
} shared_vcm;

static inline enum vcm_dev_id find_s5p_vcm_mmu_id(struct vcm *vcm)
{
	int i;

	if (WARN_ON(!vcm))
		return VCM_DEV_NONE;

	for (i = 0; i < VCM_DEV_NUM; i++)
		if (shared_vcm.vcms[i] &&
				(&(shared_vcm.vcms[i]->mmu.vcm) == vcm))
			return (enum vcm_dev_id)i;

	return VCM_DEV_NONE;
}

static inline struct s5p_vcm_mmu *find_s5p_vcm_mmu(struct vcm *vcm)
{
	enum vcm_dev_id id;

	id = find_s5p_vcm_mmu_id(vcm);

	return (id == VCM_DEV_NONE) ? NULL : shared_vcm.vcms[id];
}

static inline void s5p_mmu_cacheflush_contig(void *vastart, void *vaend)
{
	dmac_flush_range(vastart, vaend);
	outer_flush_range(virt_to_phys(vastart),
				virt_to_phys(vaend));
}

static void s5p_mmu_cleanup(struct vcm *vcm)
{
	enum vcm_dev_id id;

	if (WARN_ON(vcm == NULL))
		return;

	id = find_s5p_vcm_mmu_id(vcm);

	if (WARN_ON(id == VCM_DEV_NUM))
		return;

	kfree(shared_vcm.vcms[id]);
	shared_vcm.vcms[id] = NULL;
}

static void s5p_remove_2nd_mapping(unsigned long *base, resource_size_t *vaddr,
		resource_size_t *vsize)
{
	unsigned long *entry;
	resource_size_t wsize;
	unsigned long *flush_start;

	BUG_ON((*vaddr & SPAGEMASK) || (*vsize & SPAGEMASK));

	wsize = SECTIONSIZE - (*vaddr & SECTIONMASK);
	if (*vsize < wsize)
		wsize = *vsize;

	entry = base + LV2OFF_FROM_VADDR(*vaddr);
	flush_start = entry;

	*vsize -= wsize;
	*vaddr += wsize;

	/* This works on both of small pages and large pages */
	while (wsize) {
		*entry = 0;
		wsize -= SPAGESIZE;
		entry++;
	}

	s5p_mmu_cacheflush_contig(flush_start, entry);
}

static void s5p_remove_mapping(unsigned long *pgd, resource_size_t vaddr,
			resource_size_t vsize)
{
	unsigned long *flush_start;
	unsigned long *flush_end;

	flush_start = pgd + LV1OFF_FROM_VADDR(vaddr);
	flush_end = flush_start + ((vsize + SECTIONSIZE) >> SECTIONSHIFT);

	while (vsize) {
		unsigned long *first_entry;
		unsigned long *second_pgtable;
		unsigned long diff;

		first_entry = pgd + LV1OFF_FROM_VADDR(vaddr);

		switch (*first_entry & PG_FLAG_MASK) {
		case PG_LV1_PAGE_FLAG:
			second_pgtable =
			phys_to_virt(*first_entry & PG_LV1_LV2BASE_MASK);
			s5p_remove_2nd_mapping(second_pgtable, &vaddr, &vsize);
			break;
		case PG_LV1_SECTION_FLAG:
			BUG_ON(vsize < SECTIONSIZE);
			BUG_ON(vaddr & SECTIONMASK);

			*first_entry = 0;
			vsize -= SECTIONSIZE;
			vaddr += SECTIONSIZE;
			break;
		default:
			/* Invalid and illegal entry. Clear it anyway */
			WARN_ON(1);
			*first_entry = 0;
			diff = SECTIONSIZE - (vaddr & SECTIONMASK);
			vaddr = vaddr + diff;
			if (vsize > diff)
				vsize -= diff;
			else
				vsize = 0;
			break;
		}
	}
	s5p_mmu_cacheflush_contig(flush_start, flush_end);
}

static int s5p_write_2nd_table(unsigned long *base, resource_size_t *vaddr,
		resource_size_t vend, struct vcm_phys_part **_parts_cur,
		struct vcm_phys_part *parts_end)
{
	unsigned long *entry;
	unsigned long *flush_start;
	struct vcm_phys_part *parts_cur = *_parts_cur;
	struct vcm_phys_part chunk = {0};

	BUG_ON(*vaddr & SPAGEMASK);

	if ((vend - (*vaddr & ~SECTIONMASK)) > SECTIONSIZE)
		vend = (*vaddr & ~SECTIONMASK) + SECTIONSIZE;

	entry = base + LV2OFF_FROM_VADDR(*vaddr);

	flush_start = entry;

	/* This routine never touch the page table entry and thus return with
	 * error code immediately if it is not 'fault mapping', no matter what
	 * the content the entry contains. An entry has 'fault mapping' if its
	 * last significant 2 bits are 0b00 which means no mapping exists.
	 *
	 * vaddr contains the address of the next page of the last page
	 * written: This is important to continue mapping or to recover
	 * from error */
	while ((parts_cur != parts_end) && (*vaddr < vend)) {
		unsigned long update_size;

		if (chunk.size == 0) {
			chunk.start = parts_cur->start;
			chunk.size = parts_cur->size;
		}

		/* Reports an error if the size of a chunk to map is
		 * smaller than the smallest page size. */
		if (chunk.size < SPAGESIZE)
			return -EBADR;

		if ((*entry & PG_FLAG_MASK) != PG_FAULT_FLAG)
			return -EADDRINUSE;

		if (((*vaddr & LPAGEMASK) == 0)
				&& ((chunk.start & LPAGEMASK) == 0)
				&& ((vend - *vaddr) >= LPAGESIZE)
				&& (chunk.size >= LPAGESIZE)) {
			int i;

			for (i = 0; i < (1 << (LPAGESHIFT - SPAGESHIFT)); i++) {
				if ((*entry & PG_FLAG_MASK) != PG_FAULT_FLAG)
					return -EADDRINUSE;
				*entry = chunk.start | PG_LV2_LPAGE_FLAG;
				entry++;
				*vaddr += SPAGESIZE;
			}
			update_size = LPAGESIZE;
		} else if (((*vaddr & SPAGEMASK) == 0)
				&& ((vend - *vaddr) >= SPAGESIZE)
				&& (chunk.size >= SPAGESIZE)
				&& ((chunk.start & SPAGEMASK) == 0)) {
			*entry = chunk.start | PG_LV2_SPAGE_FLAG;
			entry++;
			update_size = SPAGESIZE;
			*vaddr += update_size;
		} else {
			return -EBADR;
		}

		chunk.size -= update_size;
		if (chunk.size == 0)
			parts_cur++;
		else
			chunk.start += update_size;
	}

	*_parts_cur = parts_cur;

	s5p_mmu_cacheflush_contig(flush_start, entry);

	/* Return value here must be positive number including zero */
	/* Returning larger than 0 means the current chunk isn't exausted. */
	return (int)(chunk.size >> SPAGESHIFT);
}

inline int lv2_pgtable_empty(unsigned long *pte_start)
{
	unsigned long *pte_end = pte_start + LV2ENTRIES;
	while ((pte_start != pte_end) && (*pte_start == 0))
		pte_start++;

	return (pte_start == pte_end) ? -1 : 0; /* true : false */
}

static int s5p_write_mapping(unsigned long *pgd, resource_size_t vaddr,
			resource_size_t vsize, struct vcm_phys_part *parts,
			unsigned int num_chunks)
{
	unsigned long *first_entry;
	resource_size_t vcur = vaddr;
	resource_size_t vend = vaddr + vsize;
	struct vcm_phys_part *parts_end;
	struct vcm_phys_part chunk = {0};
	int i;
	int ret = 0;
	unsigned long *second_pgtable = NULL;

	if (WARN_ON((vaddr | vsize) & SPAGEMASK))
		return -EFAULT;

	for (i = 0; i < num_chunks; i++)
		if (WARN_ON((parts[i].start | parts[i].size) & SPAGEMASK))
			return -EFAULT;
	/* ASSUMPTION: Sum of physical chunks is not smaller than vsize */

	parts_end = parts + num_chunks;

	first_entry = pgd + LV1OFF_FROM_VADDR(vaddr);

	/* Physical memory chunks in array 'parts'
	 * must be aligned by 1M, 64K or 4K */
	/* NO local variable is allowed in the below 'while' clause
	 * because of so many 'goto's! */
	while ((parts != parts_end) && (vcur < vend)) {
		if (chunk.size == 0) {
			chunk.start = parts->start;
			chunk.size = parts->size;
		}

		switch (*first_entry & PG_FLAG_MASK) {
		case PG_LV1_SECTION_FLAG:
			/* mapping already exists */
			ret = -EADDRINUSE;
			goto fail;

		case PG_LV1_PAGE_FLAG:
			second_pgtable = phys_to_virt(
					*first_entry & PG_LV1_LV2BASE_MASK);

			if ((((vcur | chunk.start) & SECTIONMASK) != 0)
					|| (chunk.size < SECTIONSIZE)
					|| ((vend - vcur) < SECTIONSIZE)) {
				goto page_mapping;
			}
			/* else */
			if (lv2_pgtable_empty(second_pgtable)) {
				kmem_cache_free(l2pgtbl_cachep, second_pgtable);
				*first_entry = 0;
				goto section_mapping;
			}
			/* else */
			ret = -EADDRINUSE;
			goto fail;

		case PG_FAULT_FLAG:
			if ((((vcur | chunk.start) & SECTIONMASK) == 0)
					&& ((vend - vcur) >= SECTIONSIZE)
					&& (chunk.size >= SECTIONSIZE))
				goto section_mapping;
			else
				goto new_page_mapping;
		}

		continue; /* guard: never reach here! */

section_mapping:
		do {
			*first_entry = (chunk.start & ~SECTIONMASK)
					| PG_LV1_SECTION_FLAG;
			first_entry++;
			chunk.start += SECTIONSIZE;
			chunk.size -= SECTIONSIZE;
			vcur += SECTIONSIZE;
		} while ((chunk.size >= SECTIONSIZE)
				&& ((vend - vcur) >= SECTIONSIZE)
				&& (*first_entry == 0));

		if (chunk.size == 0)
			parts++;
		continue;

new_page_mapping:
		second_pgtable = kmem_cache_zalloc(l2pgtbl_cachep,
						GFP_KERNEL | GFP_ATOMIC);
		if (!second_pgtable) {
			ret = -ENOMEM;
			goto fail;
		}
		BUG_ON((unsigned long)second_pgtable & ~PG_LV1_LV2BASE_MASK);
		*first_entry = virt_to_phys(second_pgtable) | PG_LV1_PAGE_FLAG;

page_mapping:
		ret = s5p_write_2nd_table(second_pgtable, &vcur , vend,
						&parts, parts_end);
		if (ret < 0)
			goto fail;
		/* ret is the adjustment of the current chunk */
		chunk.size = (resource_size_t)(ret << SPAGESHIFT);
		chunk.start = parts->start + chunk.size;

		first_entry++;
	}

	s5p_mmu_cacheflush_contig(pgd + LV1OFF_FROM_VADDR(vaddr), first_entry);

fail:
	if (IS_ERR_VALUE(ret))
		s5p_remove_mapping(pgd, vaddr, vcur - vaddr);
	return ret;
}

static int s5p_mmu_activate(struct vcm_res *res, struct vcm_phys *phys)
{
	/* We don't need to check res and phys because vcm_bind() in mm/vcm.c
	 * already have checked them.
	 */
	return s5p_write_mapping(shared_vcm.pgd, res->start, phys->size,
					phys->parts, phys->count);
}

static void s5p_mmu_deactivate(struct vcm_res *res, struct vcm_phys *phys)
{
	enum vcm_dev_id id;
	struct s5p_vcm_mmu *s5p_mmu;

	if (WARN_ON(!res || !phys))
		return;

	id = find_s5p_vcm_mmu_id(res->vcm);
	if (id == VCM_DEV_NONE)
		return;

	s5p_mmu = shared_vcm.vcms[id];
	if (!s5p_mmu)
		return;

	s5p_remove_mapping(shared_vcm.pgd, res->start, res->bound_size);

	if (s5p_mmu->driver && s5p_mmu->driver->tlb_invalidator)
		s5p_mmu->driver->tlb_invalidator(id);
}

/* This is exactly same as vcm_mmu_activate() in mm/vcm.c. We have to include
 * TLB invalidation in the critical section of mmu->mutex. That's why we have
 * copied exactly same code.
 */
static int s5p_vcm_mmu_activate(struct vcm *vcm)
{
	struct s5p_vcm_mmu *s5p_mmu;
	enum vcm_dev_id id;
	int ret;

	id = find_s5p_vcm_mmu_id(vcm);
	if (id == VCM_DEV_NONE)
		return -EINVAL;

	s5p_mmu = shared_vcm.vcms[id];
	if (!s5p_mmu)
		return -EINVAL;

	/* pointer to vcm_mmu_activate in mm/vcm.c */
	ret = vcm_mmu_activate(vcm);
	if (ret)
		return ret;

	if (s5p_mmu->driver) {
	       if (s5p_mmu->driver->pgd_base_specifier)
			s5p_mmu->driver->pgd_base_specifier(id,
					virt_to_phys(shared_vcm.pgd));

		if (s5p_mmu->driver->tlb_invalidator)
			s5p_mmu->driver->tlb_invalidator(id);
	}

	return 0;
}

static struct vcm_phys *s5p_vcm_mmu_phys(struct vcm *vcm, resource_size_t size,
					unsigned flags)
{
	struct vcm_phys *phys = NULL;
	struct vcm_mmu *mmu;
	struct s5p_vcm_mmu *list;

	if (WARN_ON(!vcm || !size))
		return ERR_PTR(-EINVAL);

	mmu = container_of(vcm, struct vcm_mmu, vcm);
	list = container_of(mmu, struct s5p_vcm_mmu, mmu);

	if (list->driver->phys_alloc) {
		dma_addr_t phys_addr;

		phys_addr = list->driver->phys_alloc(size, flags);

		if (!IS_ERR_VALUE(phys_addr)) {
			phys = kmalloc(sizeof(*phys) + sizeof(*phys->parts),
					GFP_KERNEL);
			phys->count = 1;
			phys->size = size;
			phys->free = list->driver->phys_free;
			phys->parts[0].start = phys_addr;
			phys->parts[0].size = size;
		}
	} else {
		phys = vcm_phys_alloc(size, 0,
			container_of(vcm, struct vcm_mmu, vcm)->driver->orders);
	}
	return phys;
}

static const unsigned char s5p_alloc_orders[] = {
	20 - PAGE_SHIFT,
	16 - PAGE_SHIFT,
	12 - PAGE_SHIFT
};

static struct vcm_mmu_driver s5p_vcm_mmu_driver = {
	.orders = s5p_alloc_orders,
	.cleanup = &s5p_mmu_cleanup,
	.activate = &s5p_mmu_activate,
	.deactivate = &s5p_mmu_deactivate
};

static int __init s5p_vcm_init(void)
{
	int ret;

	shared_vcm.pgd = (unsigned long *)__get_free_pages(
			GFP_KERNEL | __GFP_ZERO,
			fls(LV1TABLESIZE / PAGE_SIZE) - 1); /* 2 */
	if (shared_vcm.pgd == NULL)
		return -ENOMEM;

	shared_vcm.pool = gen_pool_create(SECTIONSHIFT, -1);
	if (shared_vcm.pool == NULL)
		return -ENOMEM;

	ret = gen_pool_add(shared_vcm.pool, 0x100000, 0xFFE00000, -1);
	if (ret)
		return ret;

	l2pgtbl_cachep = kmem_cache_create("VCM_L2_PGTable",
			1024, 1024, 0, NULL);
	if (!l2pgtbl_cachep)
		return -ENOMEM;

	return 0;
}
subsys_initcall(s5p_vcm_init);

struct vcm *__must_check
vcm_create_unified(resource_size_t size, enum vcm_dev_id id,
					const struct s5p_vcm_driver *driver)
{
	static struct vcm_driver vcm_driver;
	struct s5p_vcm_mmu *s5p_vcm_mmu = NULL;
	int ret;

	BUG_ON(!shared_vcm.pgd);

	if (WARN_ON(shared_vcm.vcms[id]))
		return ERR_PTR(-EEXIST);

	if (size & SPAGEMASK)
		return ERR_PTR(-EINVAL);

	WARN_ON(size & SECTIONMASK);
	if (WARN_ON(id >= VCM_DEV_NUM))
		return ERR_PTR(-EINVAL);

	s5p_vcm_mmu = kzalloc(sizeof *s5p_vcm_mmu, GFP_KERNEL);
	if (!s5p_vcm_mmu) {
		ret = -ENOMEM;
		goto error;
	}

	s5p_vcm_mmu->mmu.vcm.size = (size + SECTIONSIZE - 1) & (~SECTIONMASK);
	s5p_vcm_mmu->mmu.vcm.start = gen_pool_alloc(shared_vcm.pool,
						s5p_vcm_mmu->mmu.vcm.size);
	s5p_vcm_mmu->mmu.driver = &s5p_vcm_mmu_driver;
	if (&s5p_vcm_mmu->mmu.vcm != vcm_mmu_init(&s5p_vcm_mmu->mmu)) {
		ret = -EBADR;
		goto error;
	}

	memcpy(&vcm_driver, s5p_vcm_mmu->mmu.vcm.driver,
						sizeof(struct vcm_driver));
	vcm_driver.phys = &s5p_vcm_mmu_phys;
	vcm_mmu_activate = vcm_driver.activate;
	vcm_driver.activate = &s5p_vcm_mmu_activate;
	s5p_vcm_mmu->mmu.vcm.driver = &vcm_driver;

	s5p_vcm_mmu->driver = driver;

	shared_vcm.vcms[id] = s5p_vcm_mmu;

	return &(s5p_vcm_mmu->mmu.vcm);
error:
	/* checkpatch.pl says that "if (s5p_vcm_mmu)" is not required :) */
	kfree(s5p_vcm_mmu);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(vcm_create_unified);

/* You will use this function when you want to make your peripheral device
 * refer to the given reservation and you don't know if you can specify give the
 * address of the reservation to your peripheral deivce.
 * Thus, you can specify the given reservation to your peripheral device unless
 * this function returns S5PVCM_RES_NOT_IN_VCM.
 * If the given reservation does not belogns to the given VCM context but to the
 * same virtual address space, you can also use the reservation. But you have to
 * refelect the mapping of reservation in the other VCM context with TLB
 * invalidation.
 */
enum S5PVCM_RESCHECK vcm_reservation_in_vcm(struct vcm *vcm,
							struct vcm_res *res)
{
	enum vcm_dev_id id;

	if (WARN_ON(!vcm || !res))
		return S5PVCM_RES_NOT_IN_VCM;

	if (res->vcm == vcm)
		return S5PVCM_RES_IN_VCM;

	id = find_s5p_vcm_mmu_id(res->vcm);
	if (id == VCM_DEV_NONE)
		return S5PVCM_RES_NOT_IN_VCM;

	id = find_s5p_vcm_mmu_id(vcm);
	if (shared_vcm.vcms[id]->driver->tlb_invalidator)
		shared_vcm.vcms[id]->driver->tlb_invalidator(id);

	return S5PVCM_RES_IN_ADDRSPACE;
}
EXPORT_SYMBOL(vcm_reservation_in_vcm);

struct vcm *vcm_find_vcm(enum vcm_dev_id id)
{
	if ((id < VCM_DEV_NUM) && (id > VCM_DEV_NONE))
		return &(shared_vcm.vcms[id]->mmu.vcm);

	return NULL;
}
EXPORT_SYMBOL(vcm_find_vcm);

void vcm_set_pgtable_base(enum vcm_dev_id id)
{
	if ((id > VCM_DEV_NONE) && (id < VCM_DEV_NUM)) {
		struct s5p_vcm_mmu *s5p_mmu = shared_vcm.vcms[id];
		if (s5p_mmu && s5p_mmu->driver &&
					s5p_mmu->driver->pgd_base_specifier)
			s5p_mmu->driver->pgd_base_specifier(id,
						virt_to_phys(shared_vcm.pgd));

	}
}
EXPORT_SYMBOL(vcm_set_pgtable_base);
