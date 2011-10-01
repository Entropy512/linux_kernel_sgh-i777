/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : FIC.c
 
 Description : FIC Wrapper
 
 History : 
 ----------------------------------------------------------------------
*******************************************************************************/
#include "fic.h"

int FIC_DEC_Put(Fic *pFic, u16 length) {
	return fic_decoder(pFic, length);
}

esbInfo_t* FIC_DEC_GetEsbInfo(u32 freq) {
	return GetEsbInfo();
}

subChInfo_t* FIC_DEC_GetSubChInfo(u8 subChId) {
	return GetSubChInfo(subChId);
}

svcInfo_t* FIC_DEC_GetSvcInfo(u32 SId) {
	return GetSvcInfo(SId);
}

scInfo_t* FIC_DEC_GetScInfo(u16 SCId) {
	return GetScInfo(SCId);
}

svcInfo_t* FIC_DEC_GetSvcInfoList(u8 SvcIdx) {
	return GetSvcInfoList(SvcIdx);
}

void FIC_DEC_SubChannelOrganizationPrn(int subChId) {
	SubChannelOrganizationPrn(subChId);
}

int FIC_DEC_SubChOrgan2DidpReg(subChInfo_t *pSubChInfo, didpInfo_t *pDidp) {
	return SubChOrgan2DidpReg(pSubChInfo, pDidp);
}

int FIC_DEC_FoundAllLabels(void) {
	if(FoundAllLabels()) 
		return 1;
	return 0;
}

int FIC_DEC_SubChInfoClean(void) {
	SubChannelOrganizationClean();
	return 0;
}

