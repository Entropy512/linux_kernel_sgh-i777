/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8050_isr.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/09/11 	jason		initial
*******************************************************************************/

#ifndef __FC8050_ISR__
#define __FC8050_ISR__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

extern u32 gFicUserData;
extern u32 gMscUserData;

extern int (*pFicCallback)(u32 userdata, u8 *data, int length);
extern int (*pMscCallback)(u32 userdata, u8 subchid, u8 *data, int length);

extern void fc8050_isr(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif // __FC8050_ISR__

