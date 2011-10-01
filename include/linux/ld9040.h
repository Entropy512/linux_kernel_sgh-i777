/*inclue/linux/ld9040.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Header file for Samsung Display Panel(AMOLED) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/types.h>

struct ld9040_panel_data {
	const unsigned short *seq_user_set;
	const unsigned short *seq_panelcondition_set;
	const unsigned short *seq_displayctl_set;
	const unsigned short *seq_gtcon_set;
	const unsigned short *seq_pwrctl_set;
	const unsigned short *seq_pwrctl_set_sm2_a2;
	const unsigned short *seq_gamma_set1;
	const unsigned short *seq_sm2_gamma_set1;
	const unsigned short *seq_sm2_gamma_set2;
	const unsigned short *display_on;
	const unsigned short *display_off;
	const unsigned short *sleep_in;
	const unsigned short *sleep_out;
	const unsigned short **gamma19_table;
	const unsigned short **gamma22_table;
	const unsigned short **gamma_sm2_a1_19_table;
	const unsigned short **gamma_sm2_a1_22_table;
	const unsigned short **gamma_sm2_a2_19_table;
	const unsigned short **gamma_sm2_a2_22_table;
	const unsigned short **acl_table;
	const unsigned short **elvss_table;
	const unsigned short **elvss_sm2_table;
	const unsigned short *acl_on;
	const unsigned short *elvss_on;
	int gamma_table_size;
};

#define	LCDTYPE_M2		(1)
#define	LCDTYPE_SM2_A1		(0)
#define	LCDTYPE_SM2_A2		(2)
