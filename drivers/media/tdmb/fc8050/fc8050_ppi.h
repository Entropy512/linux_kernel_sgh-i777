/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8050_ppi.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/09/14 	jason		initial
*******************************************************************************/

#ifndef __FC8050_PPI_H__
#define __FC8050_PPI_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8050_ppi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8050_ppi_byteread(HANDLE hDevice, u16 addr, u8 *data);
extern int fc8050_ppi_wordread(HANDLE hDevice, u16 addr, u16 *data);
extern int fc8050_ppi_longread(HANDLE hDevice, u16 addr, u32 *data);
extern int fc8050_ppi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8050_ppi_bytewrite(HANDLE hDevice, u16 addr, u8 data);
extern int fc8050_ppi_wordwrite(HANDLE hDevice, u16 addr, u16 data);
extern int fc8050_ppi_longwrite(HANDLE hDevice, u16 addr, u32 data);
extern int fc8050_ppi_bulkwrite(HANDLE hDevice, u16 addr, u8* data, u16 length);
extern int fc8050_ppi_dataread(HANDLE hDevice, u16 addr, u8* data, u16 length);
extern int fc8050_ppi_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif // __FC8050_PPI_H__

