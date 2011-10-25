#ifndef __S5P_VCM_H
#define __S5P_VCM_H

#include <linux/vcm-drv.h>
#include <plat/sysmmu.h>

#ifdef CONFIG_VCM_MMU

/* for physical allocator of frame buffer */
enum vcm_dev_id {
	VCM_DEV_NONE = -1,
	VCM_DEV_MDMA = 0,
	VCM_DEV_SSS,
	VCM_DEV_FIMC0,
	VCM_DEV_FIMC1,
	VCM_DEV_FIMC2,
	VCM_DEV_FIMC3,
	VCM_DEV_JPEG,
	VCM_DEV_FIMD0,
	VCM_DEV_FIMD1,
	VCM_DEV_PCIe,
	VCM_DEV_G2D,
	VCM_DEV_ROTATOR,
	VCM_DEV_MDMA2,
	VCM_DEV_TV,
	VCM_DEV_MFC,
	VCM_DEV_NUM
};

struct vcm_cma_alloc {
	struct device *dev;
	char *type;
	dma_addr_t alignment;
};

enum S5PVCM_RESCHECK {
	S5PVCM_RES_NOT_IN_VCM = 0,
	S5PVCM_RES_IN_VCM,
	S5PVCM_RES_IN_ADDRSPACE
};

struct s5p_vcm_driver {
	void (*tlb_invalidator)(enum vcm_dev_id id);
	void (*pgd_base_specifier)(enum vcm_dev_id id, unsigned long base);
	dma_addr_t (*phys_alloc)(resource_size_t size, unsigned flag);
	void (*phys_free)(struct vcm_phys *phys);
};

/* The caller must check the return value of vcm_create_unified with
 * IS_ERR_OR_NULL() macro defined in err.h
 */
struct vcm *__must_check
vcm_create_unified(resource_size_t size, enum vcm_dev_id id,
				const struct s5p_vcm_driver *driver);

/* vcm : your vcm context.
 * res : the reservation that you want to know if it is allocated from
 *       your vcm.
 */
enum S5PVCM_RESCHECK
vcm_reservation_in_vcm(struct vcm *vcm, struct vcm_res *res);

struct vcm *vcm_find_vcm(enum vcm_dev_id id);

/* Initializes the page table base of the system MMU of the given device
 * id: device ID defined in enum vcm_dev_id
 *
 * No operation performs if the ID is wrong or no callback method is specified
 * by the device driver.
 */
void vcm_set_pgtable_base(enum vcm_dev_id id);

#endif /* CONFIG_VCM_MMU */

#endif /* __S5P_VCM_H */
