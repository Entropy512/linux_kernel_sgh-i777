/* linux/arch/arm/plat-s5p/dev-ipc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Device definition for IPC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <asm/irq.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start = S5PV210_PA_IPC,
		.end   = S5PV210_PA_IPC + S5PV210_SZ_IPC - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		  = "s3c-ipc",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_ipc_resource),
	.resource	  = s3c_ipc_resource,
};

