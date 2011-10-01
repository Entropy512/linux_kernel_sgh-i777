/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : FIC.h
 
 Description : FIC Wrapper
 
 History : 
 ----------------------------------------------------------------------
*******************************************************************************/
#ifndef __FIC_H__
#define __FIC_H__

#include "ficdecoder.h"

int          FIC_DEC_Put(Fic *pFic, u16 length);
esbInfo_t*   FIC_DEC_GetEsbInfo(u32 freq);
subChInfo_t* FIC_DEC_GetSubChInfo(u8 subChId);
svcInfo_t*   FIC_DEC_GetSvcInfo(u32 SId);
scInfo_t*    FIC_DEC_GetScInfo(u16 SCId);
svcInfo_t*   FIC_DEC_GetSvcInfoList(u8 SvcIdx);
void         FIC_DEC_SubChannelOrganizationPrn(int subChId);
int          FIC_DEC_FoundAllLabels(void);
int          FIC_DEC_SubChInfoClean(void);
int          FIC_DEC_SubChOrgan2DidpReg(subChInfo_t *pSubChInfo, didpInfo_t *pDidp);

#endif // __FIC_H__
