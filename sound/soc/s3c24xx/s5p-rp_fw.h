/* sound/soc/s3c24xx/s5p-rp_fw.h
 *
 * SRP Audio Firmware for Samsung s5pc210
 *
 * Copyright (c) 2010 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _RP_FW_H_
#define _RP_FW_H_

#ifdef CONFIG_TARGET_LOCALE_NAATT
extern unsigned long rp_fw_vliw_att[];
extern unsigned long rp_fw_cga_att[];
extern unsigned long rp_fw_cga_sa_att[];
extern unsigned long rp_fw_data_att[];
extern unsigned long rp_fw_vliw_len_att;
extern unsigned long rp_fw_cga_len_att;
extern unsigned long rp_fw_cga_sa_len_att;
extern unsigned long rp_fw_data_len_att;
#else
extern unsigned long rp_fw_vliw[];
extern unsigned long rp_fw_cga[];
extern unsigned long rp_fw_cga_sa[];
extern unsigned long rp_fw_data[];

extern unsigned long rp_fw_vliw_len;
extern unsigned long rp_fw_cga_len;
extern unsigned long rp_fw_cga_sa_len;
extern unsigned long rp_fw_data_len;
#endif
#endif
