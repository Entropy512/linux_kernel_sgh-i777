/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : bbm.h
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#ifndef __BBM_H__
#define __BBM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

#define DRIVER_VER 	"VER 3.4.3"

#define BBM_HPI		0		// EBI2
#define BBM_SPI		1		// SPI
#define BBM_USB		2		// USB
#define BBM_I2C		3		// I2C
#define BBM_PPI		4		// PPI

extern int BBM_RESET(HANDLE hDevice);
extern int BBM_PROBE(HANDLE hDevice);
extern int BBM_INIT(HANDLE hDevice);
extern int BBM_DEINIT(HANDLE hDevice);
extern int BBM_READ(HANDLE hDevice, u16 addr, u8 *data);
extern int BBM_BYTE_READ(HANDLE hDevice, u16 addr, u8 *data);
extern int BBM_WORD_READ(HANDLE hDevice, u16 addr, u16 *data);
extern int BBM_LONG_READ(HANDLE hDevice, u16 addr, u32 *data);
extern int BBM_BULK_READ(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int BBM_DATA(HANDLE hDevice, u16 addr,u8 *data, u16 size);
extern int BBM_WRITE(HANDLE hDevice, u16 addr, u8 data);
extern int BBM_BYTE_WRITE(HANDLE hDevice, u16 addr, u8 data);
extern int BBM_WORD_WRITE(HANDLE hDevice, u16 addr, u16 data);
extern int BBM_LONG_WRITE(HANDLE hDevice, u16 addr, u32 data);
extern int BBM_BULK_WRITE(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int BBM_TUNER_READ(HANDLE hDevice, u8 addr, u8 alen, u8 *buffer, u8 len);
extern int BBM_TUNER_WRITE(HANDLE hDevice, u8 addr, u8 alen, u8 *buffer, u8 len);
extern int BBM_TUNER_SET_FREQ(HANDLE hDevice, u32 freq);
extern int BBM_TUNER_SELECT(HANDLE hDevice, u32 product, u32 band);
extern int BBM_TUNER_GET_RSSI(HANDLE hDevice, s32 *rssi);
extern int BBM_SCAN_STATUS(HANDLE hDevice);
extern int BBM_CHANNEL_SELECT(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int BBM_VIDEO_SELECT(HANDLE hDevice, u8 subChId, u8 svcChId, u8 cdiId);
extern int BBM_AUDIO_SELECT(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int BBM_DATA_SELECT(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int BBM_CHANNEL_DESELECT(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int BBM_VIDEO_DESELECT(HANDLE hDevice, u8 subChId, u8 svcChId, u8 cdiId);
extern int BBM_AUDIO_DESELECT(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int BBM_DATA_DESELECT(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int BBM_HOSTIF_SELECT(HANDLE hDevice, u8 hostif);
extern int BBM_HOSTIF_DESELECT(HANDLE hDevice);
extern int BBM_FIC_CALLBACK_REGISTER(u32 userdata, int (*callback)(u32 userdata, u8 *data, int length));
extern int BBM_MSC_CALLBACK_REGISTER(u32 userdata, int (*callback)(u32 userdata, u8 subChId, u8 *data, int length));
extern int BBM_FIC_CALLBACK_DEREGISTER(HANDLE hDevice);
extern int BBM_MSC_CALLBACK_DEREGISTER(HANDLE hDevice);
extern void BBM_ISR(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif /* __BBM_H__ */

