/*
 *  linux/include/linux/mmc/mmc_ioctl.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef LINUX_MMC_MMC_IOCTL_H
#define LINUX_MMC_MMC_IOCTL_H

#include <linux/ioctl.h>

/* Same as MMC block device major number */
#define MMC_IOCTL_CODE 0xB3

#ifdef CONFIG_MMC_DISCARD
#define MMCTRIMINFO	_IOR(MMC_IOCTL_CODE, 1, struct mmc_blk_erase_info)
#define MMCTRIM		_IOW(MMC_IOCTL_CODE, 2, struct mmc_blk_erase_args)

struct mmc_blk_erase_args {
	unsigned int from;
	unsigned int nr;
};

struct mmc_blk_erase_info {
	unsigned int pref_trim;
};
#endif /* CONFIG_MMC_DISCARD */
#endif /* LINUX_MMC_MMC_IOCTL_H */
