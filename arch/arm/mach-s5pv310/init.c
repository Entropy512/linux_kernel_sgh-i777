/* linux/arch/arm/mach-s5pv310/init.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/serial_core.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/regs-serial.h>

#ifdef CONFIG_S5PV310_FPGA
static struct s3c24xx_uart_clksrc s5pv310_serial_clocks[] = {
	[0] = {
		.name		= "xtal",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 0,
	},
};
#else
static struct s3c24xx_uart_clksrc s5pv310_serial_clocks[] = {
	[0] = {
		.name		= "uclk1",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 0,
	},
};
#endif

/* uart registration process */
void __init s5pv310_common_init_uarts(struct s3c2410_uartcfg *cfg, int no)
{
	struct s3c2410_uartcfg *tcfg = cfg;
	u32 ucnt;

	for (ucnt = 0; ucnt < no; ucnt++, tcfg++) {
		if (!tcfg->clocks) {
			tcfg->has_fracval = 1;
			tcfg->clocks = s5pv310_serial_clocks;
			tcfg->clocks_size = ARRAY_SIZE(s5pv310_serial_clocks);
		}
	}

	s3c24xx_init_uartdevs("s5pv210-uart", s5p_uart_resources, cfg, no);
}
