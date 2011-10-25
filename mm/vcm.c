/*
 * Virtual Contiguous Memory core
 * Copyright (c) 2010 by Samsung Electronics.
 * Written by Michal Nazarewicz (m.nazarewicz@samsung.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License or (at your optional) any later version of the license.
 */

/*
 * See Documentation/virtual-contiguous-memory.txt for details.
 */

#include <linux/vcm-drv.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/genalloc.h>

#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <asm/outercache.h>

/******************************** Devices API *******************************/

void vcm_destroy(struct vcm *vcm)
{
	if (WARN_ON(atomic_read(&vcm->activations)))
		vcm->driver->deactivate(vcm);

	if (vcm->driver->cleanup)
		vcm->driver->cleanup(vcm);
	else
		kfree(vcm);
}
EXPORT_SYMBOL_GPL(vcm_destroy);

static void
__vcm_alloc_and_reserve(struct vcm *vcm, resource_size_t size,
			struct vcm_phys **phys, unsigned alloc_flags,
			struct vcm_res **res, unsigned res_flags)
{
	int ret, alloc = 0;

	if (WARN_ON(!vcm) || !size) {
		ret = -EINVAL;
		goto error;
	}

	size = PAGE_ALIGN(size);

	if (vcm->driver->alloc) {
		ret = vcm->driver->alloc(vcm, size,
					 phys, alloc_flags, res, res_flags);
		if (ret)
			goto error;
		alloc = 1;
	} else if ((res && !vcm->driver->res) || (phys && !vcm->driver->phys)) {
		ret = -EOPNOTSUPP;
		goto error;
	}

	if (res) {
		if (!alloc) {
			*res = vcm->driver->res(vcm, size, res_flags);
			if (IS_ERR(*res)) {
				ret = PTR_ERR(*res);
				goto error;
			}
		}
		(*res)->bound_size = 0;
		(*res)->vcm = vcm;
		(*res)->phys = NULL;
#ifdef CONFIG_VCM_RES_REFCNT
		atomic_set(&(*res)->refcnt, 1);
#endif
	}

	if (phys) {
		if (!alloc) {
			*phys = vcm->driver->phys(vcm, size, alloc_flags);
			if (WARN_ON(!(*phys)->free))
				phys = ERR_PTR(-EINVAL);
			if (IS_ERR(*phys)) {
				ret = PTR_ERR(*phys);
				goto error;
			}
		}
		atomic_set(&(*phys)->bindings, 0);
	}

	return;

error:
	if (phys)
		*phys = ERR_PTR(ret);
	if (res) {
		if (*res)
			vcm_unreserve(*res);
		*res = ERR_PTR(ret);
	}
}

struct vcm_res *__must_check
vcm_make_binding(struct vcm *vcm, resource_size_t size,
		 unsigned alloc_flags, unsigned res_flags)
{
	struct vcm_phys *phys;
	struct vcm_res *res;

	if (WARN_ON(!vcm || !size || (size & (PAGE_SIZE - 1))))
		return ERR_PTR(-EINVAL);
	else if (vcm->driver->alloc || !vcm->driver->map) {
		int ret;

		__vcm_alloc_and_reserve(vcm, size, &phys, alloc_flags,
					&res, res_flags);

		if (IS_ERR(res))
			return res;

		ret = vcm_bind(res, phys);
		if (!ret)
			return res;

		if (vcm->driver->unreserve)
			vcm->driver->unreserve(res);
		phys->free(phys);
		return ERR_PTR(ret);
	} else {
		__vcm_alloc_and_reserve(vcm, size, &phys, alloc_flags,
					NULL, 0);

		if (IS_ERR(phys))
			return ERR_CAST(res);

		res = vcm_map(vcm, phys, res_flags);
		if (IS_ERR(res))
			phys->free(phys);

		return res;
	}
}
EXPORT_SYMBOL_GPL(vcm_make_binding);

struct vcm_phys *__must_check
vcm_alloc(struct vcm *vcm, resource_size_t size, unsigned flags)
{
	struct vcm_phys *phys;

	__vcm_alloc_and_reserve(vcm, size, &phys, flags, NULL, 0);

	return phys;
}
EXPORT_SYMBOL_GPL(vcm_alloc);

struct vcm_res *__must_check
vcm_reserve(struct vcm *vcm, resource_size_t size, unsigned flags)
{
	struct vcm_res *res;

	__vcm_alloc_and_reserve(vcm, size, NULL, 0, &res, flags);

	return res;
}
EXPORT_SYMBOL_GPL(vcm_reserve);

#ifdef CONFIG_VCM_RES_REFCNT
int __must_check vcm_ref_reserve(struct vcm_res *res)
{
	if (WARN_ON(!res) || (atomic_inc_return(&res->refcnt) < 2))
		return -EINVAL;
	return 0;
}
EXPORT_SYMBOL_GPL(vcm_ref_reserve);
#endif

struct vcm_res *__must_check
vcm_map(struct vcm *vcm, struct vcm_phys *phys, unsigned flags)
{
	struct vcm_res *res;
	int ret;

	if (WARN_ON(!vcm))
		return ERR_PTR(-EINVAL);

	if (vcm->driver->map) {
		res = vcm->driver->map(vcm, phys, flags);
		if (!IS_ERR(res)) {
			atomic_inc(&phys->bindings);
			res->phys       = phys;
			res->bound_size = phys->size;
			res->vcm        = vcm;
		}
		return res;
	}

	res = vcm_reserve(vcm, phys->size, flags);
	if (IS_ERR(res))
		return res;

	ret = vcm_bind(res, phys);
	if (!ret)
		return res;

	vcm_unreserve(res);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(vcm_map);

void vcm_unreserve(struct vcm_res *res)
{
	if (!WARN_ON(!res)) {
#ifdef CONFIG_VCM_RES_REFCNT
		if (!atomic_dec_and_test(&res->refcnt))
			return;
#endif
		if (WARN_ON(res->phys))
			vcm_unbind(res);
		if (!WARN_ON_ONCE(!res->vcm->driver->unreserve))
			res->vcm->driver->unreserve(res);
	}
}
EXPORT_SYMBOL_GPL(vcm_unreserve);

void vcm_free(struct vcm_phys *phys)
{
	if (!WARN_ON(!phys || atomic_read(&phys->bindings)))
		phys->free(phys);
}
EXPORT_SYMBOL_GPL(vcm_free);

int  __must_check vcm_bind(struct vcm_res *res, struct vcm_phys *phys)
{
	int ret;

	if (WARN_ON(!res || !phys))
		return -EINVAL;

	if (res->phys == phys)
		return -EALREADY;

	if (res->phys)
		return -EADDRINUSE;

	if (phys->size > res->res_size)
		return -ENOSPC;

	if (!res->vcm->driver->bind)
		return -EOPNOTSUPP;

	ret = res->vcm->driver->bind(res, phys);
	if (ret >= 0) {
		atomic_inc(&phys->bindings);
		res->phys = phys;
		res->bound_size = phys->size;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(vcm_bind);

struct vcm_phys *vcm_unbind(struct vcm_res *res)
{
	struct vcm_phys *phys = NULL;
	if (!WARN_ON(!res || !res->phys)) {
		phys = res->phys;
		if (res->vcm->driver->unbind)
			res->vcm->driver->unbind(res);
		WARN_ON(!atomic_add_unless(&phys->bindings, -1, 0));
		res->phys = NULL;
		res->bound_size = 0;
	}
	return phys;
}
EXPORT_SYMBOL_GPL(vcm_unbind);

void vcm_unmap(struct vcm_res *res)
{
#ifdef CONFIG_VCM_RES_REFCNT
	if (atomic_read(&res->refcnt) > 1) {
		atomic_dec(&res->refcnt);
		return;
	}
#endif
	vcm_unbind(res);
	vcm_unreserve(res);
}
EXPORT_SYMBOL_GPL(vcm_unmap);

void vcm_destroy_binding(struct vcm_res *res)
{
	if (!WARN_ON(!res)) {
		struct vcm_phys *phys;
#ifdef CONFIG_VCM_RES_REFCNT
		if (atomic_read(&res->refcnt) > 1) {
			atomic_dec(&res->refcnt);
			return;
		}
#endif
		phys = vcm_unbind(res);
		if (phys)
			vcm_free(phys);
		vcm_unreserve(res);
	}
}
EXPORT_SYMBOL_GPL(vcm_destroy_binding);

int  __must_check vcm_activate(struct vcm *vcm)
{
	if (WARN_ON(!vcm))
		return -EINVAL;
	else if (atomic_inc_return(&vcm->activations) != 1
	      || !vcm->driver->activate)
		return 0;
	else
		return vcm->driver->activate(vcm);
}
EXPORT_SYMBOL_GPL(vcm_activate);

void vcm_deactivate(struct vcm *vcm)
{
	if (!WARN_ON(!vcm || !atomic_read(&vcm->activations))
	 && atomic_dec_and_test(&vcm->activations)
	 && vcm->driver->deactivate)
		vcm->driver->deactivate(vcm);
}
EXPORT_SYMBOL_GPL(vcm_deactivate);


/****************************** VCM VMM driver ******************************/

static void vcm_vmm_cleanup(struct vcm *vcm)
{
	/* This should never be called.  vcm_vmm is a static object. */
	BUG_ON(1);
}

static struct vcm_phys *
vcm_vmm_phys(struct vcm *vcm, resource_size_t size, unsigned flags)
{
	static const unsigned char orders[] = { 0 };
	return vcm_phys_alloc(size, flags, orders);
}

static void vcm_vmm_unreserve(struct vcm_res *res)
{
	kfree(res);
}

struct vcm_res *vcm_vmm_map(struct vcm *vcm, struct vcm_phys *phys,
			    unsigned flags)
{
	/*
	 * Original implementation written by Cho KyongHo
	 * (pullip.cho@samsung.com).  Later rewritten by mina86.
	 */
	struct vcm_phys_part *part;
	struct page **pages, **p;
	struct vcm_res *res;
	int ret = -ENOMEM;
	unsigned i;

	pages = kmalloc((phys->size >> PAGE_SHIFT) * sizeof *pages, GFP_KERNEL);
	if (!pages)
		return ERR_PTR(-ENOMEM);
	p = pages;

	res = kmalloc(sizeof *res, GFP_KERNEL);
	if (!res)
		goto error_pages;

	i    = phys->count;
	part = phys->parts;
	do {
		unsigned j = part->size >> PAGE_SHIFT;
		struct page *page = part->page;
		if (!page)
			goto error_notsupp;
		do {
			*p++ = page++;
		} while (--j);
	} while (++part, --i);

	res->start = (dma_addr_t)vmap(pages, p - pages, VM_ALLOC, PAGE_KERNEL);
	if (!res->start)
		goto error_res;

	kfree(pages);
	res->res_size = phys->size;
	return res;

error_notsupp:
	ret = -EOPNOTSUPP;
error_res:
	kfree(res);
error_pages:
	kfree(pages);
	return ERR_PTR(ret);
}

static void vcm_vmm_unbind(struct vcm_res *res)
{
	vunmap((void *)res->start);
}

static int vcm_vmm_activate(struct vcm *vcm)
{
	/* no operation, all bindings are immediately active */
	return 0;
}

static void vcm_vmm_deactivate(struct vcm *vcm)
{
	/*
	 * no operation, all bindings are immediately active and
	 * cannot be deactivated unless unbound.
	 */
}

struct vcm vcm_vmm[1] = { {
	.start       = 0,
	.size        = ~(resource_size_t)0,
	/* prevent activate/deactivate from being called */
	.activations = ATOMIC_INIT(1),
	.driver      = &(const struct vcm_driver) {
		.cleanup	= vcm_vmm_cleanup,
		.map		= vcm_vmm_map,
		.phys		= vcm_vmm_phys,
		.unbind		= vcm_vmm_unbind,
		.unreserve	= vcm_vmm_unreserve,
		.activate	= vcm_vmm_activate,
		.deactivate	= vcm_vmm_deactivate,
	}
} };
EXPORT_SYMBOL_GPL(vcm_vmm);


/****************************** VCM Drivers API *****************************/

struct vcm *__must_check vcm_init(struct vcm *vcm)
{
	if (WARN_ON(!vcm || !vcm->size
		 || ((vcm->start | vcm->size) & ~PAGE_MASK)
		 || !vcm->driver || !vcm->driver->unreserve))
		return ERR_PTR(-EINVAL);

	atomic_set(&vcm->activations, 0);

	return vcm;
}
EXPORT_SYMBOL_GPL(vcm_init);


/*************************** Hardware MMU wrapper ***************************/

#ifdef CONFIG_VCM_MMU

struct vcm_mmu_res {
	struct vcm_res			res;
	struct list_head		bound;
};

static void vcm_mmu_cleanup(struct vcm *vcm)
{
	struct vcm_mmu *mmu = container_of(vcm, struct vcm_mmu, vcm);
	WARN_ON(spin_is_locked(&mmu->lock) || !list_empty(&mmu->bound_res));
	gen_pool_destroy(mmu->pool);
	if (mmu->driver->cleanup)
		mmu->driver->cleanup(vcm);
	else
		kfree(mmu);
}

static struct vcm_res *
vcm_mmu_res(struct vcm *vcm, resource_size_t size, unsigned flags)
{
	struct vcm_mmu *mmu = container_of(vcm, struct vcm_mmu, vcm);
	const unsigned char *orders;
	struct vcm_mmu_res *res;
	dma_addr_t addr;
	unsigned order;

	res = kzalloc(sizeof *res, GFP_KERNEL);
	if (!res)
		return ERR_PTR(-ENOMEM);

	order = ffs(size) - PAGE_SHIFT - 1;
	for (orders = mmu->driver->orders; *orders > order; ++orders)
		/* nop */;
	order = *orders + PAGE_SHIFT;

	addr = gen_pool_alloc_aligned(mmu->pool, size, order);
	if (!addr) {
		kfree(res);
		return ERR_PTR(-ENOSPC);
	}

	INIT_LIST_HEAD(&res->bound);
	res->res.start = addr;
	res->res.res_size = size;

	return &res->res;
}

static struct vcm_phys *
vcm_mmu_phys(struct vcm *vcm, resource_size_t size, unsigned flags)
{
	return vcm_phys_alloc(size, flags,
			      container_of(vcm, struct vcm_mmu,
					   vcm)->driver->orders);
}

static int __must_check
__vcm_mmu_activate(struct vcm_res *res, struct vcm_phys *phys)
{
	struct vcm_mmu *mmu = container_of(res->vcm, struct vcm_mmu, vcm);
	if (mmu->driver->activate)
		return mmu->driver->activate(res, phys);

	return vcm_phys_walk(res->start, phys, mmu->driver->orders,
			     mmu->driver->activate_page,
			     mmu->driver->deactivate_page, res->vcm);
}

static void __vcm_mmu_deactivate(struct vcm_res *res, struct vcm_phys *phys)
{
	struct vcm_mmu *mmu = container_of(res->vcm, struct vcm_mmu, vcm);
	if (mmu->driver->deactivate)
		return mmu->driver->deactivate(res, phys);

	vcm_phys_walk(res->start, phys, mmu->driver->orders,
		      mmu->driver->deactivate_page, NULL, res->vcm);
}

static int vcm_mmu_bind(struct vcm_res *_res, struct vcm_phys *phys)
{
	struct vcm_mmu_res *res = container_of(_res, struct vcm_mmu_res, res);
	struct vcm_mmu *mmu = container_of(_res->vcm, struct vcm_mmu, vcm);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&mmu->lock, flags);
	if (mmu->activated) {
		ret = __vcm_mmu_activate(_res, phys);
		if (ret < 0)
			goto done;
	}
	list_add_tail(&res->bound, &mmu->bound_res);
	ret = 0;
done:
	spin_unlock_irqrestore(&mmu->lock, flags);

	return ret;
}

static void vcm_mmu_unbind(struct vcm_res *_res)
{
	struct vcm_mmu_res *res = container_of(_res, struct vcm_mmu_res, res);
	struct vcm_mmu *mmu = container_of(_res->vcm, struct vcm_mmu, vcm);
	unsigned long flags;

	spin_lock_irqsave(&mmu->lock, flags);
	if (mmu->activated)
		__vcm_mmu_deactivate(_res, _res->phys);
	list_del_init(&res->bound);
	spin_unlock_irqrestore(&mmu->lock, flags);
}

static void vcm_mmu_unreserve(struct vcm_res *res)
{
	struct vcm_mmu *mmu = container_of(res->vcm, struct vcm_mmu, vcm);
	gen_pool_free(mmu->pool, res->start, res->res_size);
}

static int vcm_mmu_activate(struct vcm *vcm)
{
	struct vcm_mmu *mmu = container_of(vcm, struct vcm_mmu, vcm);
	struct vcm_mmu_res *r, *rr;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&mmu->lock, flags);

	list_for_each_entry(r, &mmu->bound_res, bound) {
		ret = __vcm_mmu_activate(&r->res, r->res.phys);
		if (ret >= 0)
			continue;

		list_for_each_entry(rr, &mmu->bound_res, bound) {
			if (r == rr)
				goto done;
			__vcm_mmu_deactivate(&rr->res, rr->res.phys);
		}
	}

	mmu->activated = 1;
	ret = 0;

done:
	spin_unlock_irqrestore(&mmu->lock, flags);

	return ret;
}

static void vcm_mmu_deactivate(struct vcm *vcm)
{
	struct vcm_mmu *mmu = container_of(vcm, struct vcm_mmu, vcm);
	struct vcm_mmu_res *r;
	unsigned long flags;

	spin_lock_irqsave(&mmu->lock, flags);

	mmu->activated = 0;

	list_for_each_entry(r, &mmu->bound_res, bound)
		mmu->driver->deactivate(&r->res, r->res.phys);

	spin_unlock_irqrestore(&mmu->lock, flags);
}

struct vcm *__must_check vcm_mmu_init(struct vcm_mmu *mmu)
{
	static const struct vcm_driver driver = {
		.cleanup	= vcm_mmu_cleanup,
		.res		= vcm_mmu_res,
		.phys		= vcm_mmu_phys,
		.bind		= vcm_mmu_bind,
		.unbind		= vcm_mmu_unbind,
		.unreserve	= vcm_mmu_unreserve,
		.activate	= vcm_mmu_activate,
		.deactivate	= vcm_mmu_deactivate,
	};

	struct vcm *vcm;
	int ret;

	if (WARN_ON(!mmu || !mmu->driver ||
		    !(mmu->driver->activate ||
		      (mmu->driver->activate_page &&
		       mmu->driver->deactivate_page)) ||
		    !(mmu->driver->deactivate ||
		      mmu->driver->deactivate_page)))
		return ERR_PTR(-EINVAL);

	mmu->vcm.driver = &driver;
	vcm = vcm_init(&mmu->vcm);
	if (IS_ERR(vcm))
		return vcm;

	mmu->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!mmu->pool)
		return ERR_PTR(-ENOMEM);

	ret = gen_pool_add(mmu->pool, mmu->vcm.start, mmu->vcm.size, -1);
	if (ret) {
		gen_pool_destroy(mmu->pool);
		return ERR_PTR(ret);
	}

	vcm->driver     = &driver;
	INIT_LIST_HEAD(&mmu->bound_res);
	spin_lock_init(&mmu->lock);

	return &mmu->vcm;
}
EXPORT_SYMBOL_GPL(vcm_mmu_init);

#endif


/**************************** One-to-One wrapper ****************************/

#ifdef CONFIG_VCM_O2O

static void vcm_o2o_cleanup(struct vcm *vcm)
{
	struct vcm_o2o *o2o = container_of(vcm, struct vcm_o2o, vcm);
	if (o2o->driver->cleanup)
		o2o->driver->cleanup(vcm);
	else
		kfree(o2o);
}

static struct vcm_phys *
vcm_o2o_phys(struct vcm *vcm, resource_size_t size, unsigned flags)
{
	struct vcm_o2o *o2o = container_of(vcm, struct vcm_o2o, vcm);
	struct vcm_phys *phys;

	phys = o2o->driver->phys(vcm, size, flags);
	if (!IS_ERR(phys) &&
	    WARN_ON(!phys->free || !phys->parts->size ||
		    phys->parts->size < size ||
		    ((phys->parts->start | phys->parts->size) &
		     ~PAGE_MASK))) {
		if (phys->free)
			phys->free(phys);
		return ERR_PTR(-EINVAL);
	}

	return phys;
}

static struct vcm_res *
vcm_o2o_map(struct vcm *vcm, struct vcm_phys *phys, unsigned flags)
{
	struct vcm_res *res;

	if (phys->count != 1)
		return ERR_PTR(-EOPNOTSUPP);

	if (!phys->parts->size
	 || ((phys->parts->start | phys->parts->size) & ~PAGE_MASK))
		return ERR_PTR(-EINVAL);

	res = kmalloc(sizeof *res, GFP_KERNEL);
	if (!res)
		return ERR_PTR(-ENOMEM);

	res->start    = phys->parts->start;
	res->res_size = phys->parts->size;
	return res;
}

static int vcm_o2o_bind(struct vcm_res *res, struct vcm_phys *phys)
{
	if (phys->count != 1)
		return -EOPNOTSUPP;

	if (!phys->parts->size
	 || ((phys->parts->start | phys->parts->size) & ~PAGE_MASK))
		return -EINVAL;

	if (res->start != phys->parts->start)
		return -EOPNOTSUPP;

	return 0;
}

struct vcm *__must_check vcm_o2o_init(struct vcm_o2o *o2o)
{
	static const struct vcm_driver driver = {
		.cleanup	= vcm_o2o_cleanup,
		.phys		= vcm_o2o_phys,
		.map		= vcm_o2o_map,
		.bind		= vcm_o2o_bind,
		.unreserve	= (void (*)(struct vcm_res *))kfree,
	};

	if (WARN_ON(!o2o || !o2o->driver || !o2o->driver->phys))
		return ERR_PTR(-EINVAL);

	o2o->vcm.driver = &driver;
	return vcm_init(&o2o->vcm);
}
EXPORT_SYMBOL_GPL(vcm_o2o_init);

#endif


/************************ Physical memory management ************************/

#ifdef CONFIG_VCM_PHYS

struct vcm_phys_list {
	struct vcm_phys_list	*next;
	unsigned		count;
	struct vcm_phys_part	parts[31];
};

static struct vcm_phys_list *__must_check
vcm_phys_alloc_list_order(struct vcm_phys_list *last, resource_size_t *pages,
			  unsigned flags, unsigned order, unsigned *total,
			  gfp_t gfp)
{
	unsigned count;

	count	= *pages >> order;

	do {
		struct page *page = alloc_pages(gfp, order);

		if (!page)
			/*
			 * If allocation failed we may still
			 * try to continua allocating smaller
			 * pages.
			 */
			break;

		if (last->count == ARRAY_SIZE(last->parts)) {
			struct vcm_phys_list *l;
			l = kmalloc(sizeof *l, GFP_KERNEL);
			if (!l)
				return NULL;

			l->next = NULL;
			l->count = 0;
			last->next = l;
			last = l;
		}

		last->parts[last->count].start = page_to_phys(page);
		last->parts[last->count].size  = (1 << order) << PAGE_SHIFT;
		last->parts[last->count].page  = page;
		++last->count;
		++*total;
		*pages -= 1 << order;
	} while (--count);

	return last;
}

static unsigned __must_check
vcm_phys_alloc_list(struct vcm_phys_list *first,
		    resource_size_t size, unsigned flags,
		    const unsigned char *orders, gfp_t gfp)
{
	struct vcm_phys_list *last = first;
	unsigned total_parts = 0;
	resource_size_t pages;

	/*
	 * We are trying to allocate as large pages as possible but
	 * not larger then pages that MMU driver that called us
	 * supports (ie. the ones provided by page_sizes).  This makes
	 * it possible to map the region using fewest possible number
	 * of entries.
	 */
	pages = size >> PAGE_SHIFT;
	do {
		while (!(pages >> *orders))
			++orders;

		last = vcm_phys_alloc_list_order(last, &pages, flags, *orders,
						 &total_parts, gfp);
		if (!last)
			return 0;

	} while (*orders++ && pages);

	if (pages)
		return 0;

	return total_parts;
}

static void vcm_phys_free_parts(struct vcm_phys_part *parts, unsigned count)
{
	do {
		__free_pages(parts->page, ffs(parts->size) - 1 - PAGE_SHIFT);
	} while (++parts, --count);
}

static void vcm_phys_free(struct vcm_phys *phys)
{
	vcm_phys_free_parts(phys->parts, phys->count);
	kfree(phys);
}

struct vcm_phys *__must_check
__vcm_phys_alloc_coherent(resource_size_t size, unsigned flags,
		 const unsigned char *orders, gfp_t gfp)
{
	struct vcm_phys *phys;
	int i;

	/* the physical memory must reside in the lowmem */
	phys = __vcm_phys_alloc(size, flags, orders, GFP_DMA32);
	if (IS_ERR(phys))
		return phys;

	for (i = 0; i < phys->count; i++) {
		struct vcm_phys_part *part;

		part = phys->parts + i;;
		/* invalidate */
		dmac_map_area(phys_to_virt(part->start), part->size, 2);
		outer_inv_range(part->start, part->start + part->size);
	}

	return phys;
}
EXPORT_SYMBOL_GPL(__vcm_phys_alloc_coherent);

struct vcm_phys *__must_check
__vcm_phys_alloc(resource_size_t size, unsigned flags,
		 const unsigned char *orders, gfp_t gfp)
{
	struct vcm_phys_list *lst, *n;
	struct vcm_phys_part *out;
	struct vcm_phys *phys;
	unsigned count;

	if (WARN_ON((size & (PAGE_SIZE - 1)) || !size || !orders))
		return ERR_PTR(-EINVAL);

	lst = kmalloc(sizeof *lst, GFP_KERNEL);
	if (!lst)
		return ERR_PTR(-ENOMEM);

	lst->next = NULL;
	lst->count = 0;

	count = vcm_phys_alloc_list(lst, size, flags, orders, gfp);
	if (!count)
		goto error;

	phys = kmalloc(sizeof *phys + count * sizeof *phys->parts, GFP_KERNEL);
	if (!phys)
		goto error;

	phys->free  = vcm_phys_free;
	phys->count = count;
	phys->size  = size;

	out = phys->parts;
	do {
		memcpy(out, lst->parts, lst->count * sizeof *out);
		out += lst->count;

		n = lst->next;
		kfree(lst);
		lst = n;
	} while (lst);

	return phys;

error:
	do {
		vcm_phys_free_parts(lst->parts, lst->count);

		n = lst->next;
		kfree(lst);
		lst = n;
	} while (lst);

	return ERR_PTR(-ENOMEM);
}
EXPORT_SYMBOL_GPL(__vcm_phys_alloc);

static inline bool is_of_order(dma_addr_t size, unsigned order)
{
	return !(size & (((dma_addr_t)PAGE_SIZE << order) - 1));
}

static int
__vcm_phys_walk_part(dma_addr_t vaddr, const struct vcm_phys_part *part,
		     const unsigned char *orders,
		     int (*callback)(dma_addr_t vaddr, dma_addr_t paddr,
				     unsigned order, void *priv), void *priv,
		     unsigned *limit)
{
	resource_size_t size = part->size;
	dma_addr_t paddr = part->start;
	resource_size_t ps;

	while (!is_of_order(vaddr, *orders))
		++orders;
	while (!is_of_order(paddr, *orders))
		++orders;

	ps = PAGE_SIZE << *orders;
	for (; *limit && size; --*limit) {
		int ret;

		while (ps > size)
			ps = PAGE_SIZE << *++orders;

		ret = callback(vaddr, paddr, *orders, priv);
		if (ret < 0)
			return ret;

		ps = PAGE_SIZE << *orders;
		vaddr += ps;
		paddr += ps;
		size  -= ps;
	}

	return 0;
}

int vcm_phys_walk(dma_addr_t _vaddr, const struct vcm_phys *phys,
		  const unsigned char *orders,
		  int (*callback)(dma_addr_t vaddr, dma_addr_t paddr,
				  unsigned order, void *arg),
		  int (*recovery)(dma_addr_t vaddr, dma_addr_t paddr,
				  unsigned order, void *arg),
		  void *priv)
{
	unsigned limit = ~0;
	int r = 0;

	if (WARN_ON(!phys || ((_vaddr | phys->size) & (PAGE_SIZE - 1)) ||
		    !phys->size || !orders || !callback))
		return -EINVAL;

	for (;;) {
		const struct vcm_phys_part *part = phys->parts;
		unsigned count = phys->count;
		dma_addr_t vaddr = _vaddr;
		int ret = 0;

		for (; count && limit; --count, ++part) {
			ret = __vcm_phys_walk_part(vaddr, part, orders,
						   callback, priv, &limit);
			if (ret)
				break;

			vaddr += part->size;
		}

		if (r)
			/* We passed error recovery */
			return r;

		/*
		 * Either operation suceeded or we were not provided
		 * with a recovery callback -- return.
		 */
		if (!ret || !recovery)
			return ret;

		/* Switch to recovery */
		limit = ~0 - limit;
		callback = recovery;
		r = ret;
	}
}
EXPORT_SYMBOL_GPL(vcm_phys_walk);

#endif
