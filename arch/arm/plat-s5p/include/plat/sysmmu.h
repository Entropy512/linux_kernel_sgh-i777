/* linux/arch/arm/plat-s5p/include/plat/sysmmu.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * Samsung sysmmu driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __SYSMMU_H__
#define __SYSMMU_H__

/* debug macro */
#ifdef CONFIG_S5P_SYSMMU_DEBUG
#define sysmmu_debug(fmt, arg...)	printk(KERN_INFO "[%s] " fmt, __FUNCTION__, ## arg)
#else
#define sysmmu_debug(fmt, arg...)	do { } while (0)
#endif

#define S5P_SYSMMU_TOTAL_IPNUM          (16)

struct s5p_sysmmu_platdata {

	unsigned int version;
};

typedef enum {
	SYSMMU_MDMA     = 0,
	SYSMMU_SSS      = 1,
	SYSMMU_FIMC0    = 2,
	SYSMMU_FIMC1    = 3,
	SYSMMU_FIMC2    = 4,
	SYSMMU_FIMC3    = 5,
	SYSMMU_JPEG     = 6,
	SYSMMU_FIMD0    = 7,
	SYSMMU_FIMD1    = 8,
	SYSMMU_PCIe     = 9,
	SYSMMU_G2D      = 10,
	SYSMMU_ROTATOR  = 11,
	SYSMMU_MDMA2    = 12,
	SYSMMU_TV       = 13,
	SYSMMU_MFC_L    = 14,
	SYSMMU_MFC_R    = 15,
} sysmmu_ips;

/*
* SysMMU enum
*/
typedef enum sysmmu_pagetable_type sysmmu_table_type_t;
enum sysmmu_pagetable_type {
	SHARED,
	SEPARATED,
};

/*
 * SysMMU struct
*/
typedef struct sysmmu_tt_info sysmmu_tt_info_t;
typedef struct sysmmu_controller sysmmu_controller_t;

struct sysmmu_tt_info {
	unsigned long *pgd;
	unsigned long pgd_paddr;
	unsigned long *pte;
};

struct sysmmu_controller {
	const char		*name;
	void __iomem		*regs;		/* channels registers */
	unsigned int		irq;		/* channel irq */
	sysmmu_ips              ips;            /* SysMMU controller type */
	sysmmu_table_type_t	table_type;	/* SysMMU table type: shared or separated */
	sysmmu_tt_info_t	*tt_info;	/* Translation Table Info. */
	struct resource *mem;
	struct device *dev;
	bool			enable;		/* SysMMU controller enable - true : enable */
};

int sysmmu_on(sysmmu_ips ips);
int sysmmu_off(sysmmu_ips ips);
int sysmmu_set_tablebase_pgd(sysmmu_ips ips, unsigned long pgd);
int sysmmu_tlb_invalidate(sysmmu_ips ips);
#endif /* __SYSMMU_H__ */
