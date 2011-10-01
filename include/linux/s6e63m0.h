/*inclue/linux/tl2796.h
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

struct s6e63m0_panel_data {
	const unsigned short *seq_panel_condition_set;
	const unsigned short *seq_display_set;
	const unsigned short *default_gamma;
	const unsigned short *seq_etc_set;
	const unsigned short *display_on;
	const unsigned short *display_off;
	const unsigned short **gamma19_table;
	const unsigned short **gamma22_table;
	const unsigned short **acl_table;
	const unsigned short *acl_on;
	const unsigned short **elvss_table;
	const unsigned short *elvss_on;
	int gamma_table_size;
};

