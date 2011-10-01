/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fci_tun.h
 
 Description : tuner control driver header
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#ifndef __FCI_TUN_H__
#define __FCI_TUN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

typedef enum {
	FCI_I2C_TYPE = 0
} i2c_type;

typedef enum {
	BAND3_TYPE = 0,
	LBAND_TYPE
} band_type;

typedef enum {
	FC8000_TUNER = 0,
	FC8050_TUNER
} product_type;

extern int tuner_i2c_init(HANDLE hDevice, int speed, int slaveaddr);
extern int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 alen, u8* data, u8 len);
extern int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 alen, u8* data, u8 len);

extern int tuner_select(HANDLE hDevice, u32 product, u32 band);
extern int tuner_set_freq(HANDLE hDevice, u32 freq);
extern int tuner_get_rssi(HANDLE hDevice, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif		// __FCI_TUN_H__
