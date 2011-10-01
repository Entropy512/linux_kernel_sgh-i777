/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : ficdecoder.c
 
 Description : fic parser
 
 History : 
 ----------------------------------------------------------------------
*******************************************************************************/
#include <linux/string.h>
#include <linux/delay.h>

#include "ficdecoder.h"
#include "fci_oal.h"
//#include <string.h>

#define DPRINTK(x...) printk("TDMB " x)

#define MSB(X) 				(((X) >>8) & 0Xff)
#define LSB(X)  			((X) & 0Xff)
#define BYTESWAP(X) 			((LSB(X)<<8) | MSB(X))

esbInfo_t gEsbInfo[MAX_ESB_NUM];
svcInfo_t gSvcInfo[MAX_SVC_NUM];
scInfo_t gScInfo[MAX_SC_NUM];
subChInfo_t subChInfo[MAX_SUBCH_NUM];
didpInfo_t didpInfo[MAX_DIDP_NUM];

static int fig0_decoder(Fig *pFig);
static int fig1_decoder(Fig *pFig);

static int fig0_ext1_decoder(u8 cn,u8 *fibBuffer, int figLength);
static int fig0_ext2_decoder(u8 *fibBuffer, int figLength, int pd);
static int fig0_ext3_decoder(u8 *fibBuffer, int figLength);
//static int fig0_ext4_decoder(u8 *fibBuffer, int figLength); // Add
static int fig0_ext10_decoder(u8 *fibBuffer, int figLength);
static int fig0_ext13_decoder(u8 *fibBuffer, int figLength, int pd);
static int fig0_ext14_decoder(u8 *fibBuffer, int figLength); // Add
static int fig0_ext15_decoder(u8 *fibBuffer, int figLength, int pd);
static int fig0_ext18_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext0_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext5_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext1_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext4_decoder(u8 *fibBuffer, int figLength);

int SubChOrgan2DidpReg(subChInfo_t *pSubChInfo, didpInfo_t *pDidp);


const u16 BitRateProfile[64][3] = {  //CU  PL  Bit Rates
	 { 16, 5,  32},  { 21, 4,  32},  { 24, 3,  32},  { 29, 2,  32},  { 35, 1,  32},  
	 { 24, 5,  48},  { 29, 4,  48},  { 35, 3,  48},  { 42, 2,  48},  { 52, 1,  48},  
	 { 29, 5,  56},  { 35, 4,  56},  { 42, 3,  56},  { 52, 2,  56},  { 32, 5,  64},  
	 { 42, 4,  64},  { 48, 3,  64},  { 58, 2,  64},  { 70, 1,  64},  { 40, 5,  80},  
	 { 52, 4,  80},  { 58, 3,  80},  { 70, 2,  80},  { 84, 1,  80},  { 48, 5,  96},  
	 { 58, 4,  96},  { 70, 3,  96},  { 84, 2,  96},  {104, 1,  96},  { 58, 5, 112}, 
	 { 70, 4, 112},  { 84, 3, 112},  {104, 2, 112},  { 64, 5, 128},  { 84, 4, 128}, 
	 { 96, 3, 128},  {116, 2, 128},  {140, 1, 128},  { 80, 5, 160},  {104, 4, 160}, 
	 {116, 3, 160},  {140, 2, 160},  {168, 1, 160},  { 96, 5, 192},  {116, 4, 192}, 
	 {140, 3, 192},  {168, 2, 192},  {208, 1, 192},  {116, 5, 224},  {140, 4, 224}, 
	 {168, 3, 224},  {208, 2, 224},  {232, 1, 224},  {128, 5, 256},  {168, 4, 256}, 
	 {192, 3, 256},  {232, 2, 256},  {280, 1, 256},  {160, 5, 320},  {208, 4, 320}, 
	 {280, 2, 320},  {192, 5, 384},  {280, 3, 384},  {416, 1, 384}
};

const u16 UEPProfile[14][5][9] = {	// L1  L2  L3  L4 PI1 PI2 PI3 PI4 pad 
	// 32kbps
	{
		{  3,  5, 13,  3, 24, 17, 12, 17, 4},
		{  3,  4, 14,  3, 22, 13,  8, 13, 0},
		{  3,  4, 14,  3, 15,  9,  6,  8, 0},	// 4 -> 3
		{  3,  3, 18,  0, 11,  6,  5,  0, 0},
		{  3,  4, 17,  0,  5,  3,  2,  0, 0}	// 4 -> 3
	},

	// 48kbps
	{
		{  3,  5, 25,  3, 24, 18, 13, 18, 0},
		{  3,  4, 26,  3, 24, 14,  8, 15, 0},
		{  3,  4, 26,  3, 15, 10,  6,  9, 4},
		{  3,  4, 26,  3,  9,  6,  4,  6, 0},
		{  4,  3, 26,  3,  5,  4,  2,  3, 0}	// 10
	},

	// 56kbps
	{
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	// not use
		{  6, 10, 23,  3, 23, 13,  8, 13, 8},
		{  6, 12, 21,  3, 16,  7,  6,  9, 0},
		{  6, 10, 23,  3,  9,  6,  4,  5, 0},
		{  6, 10, 23,  3,  5,  4,  2,  3, 0}	// 15
	},

	// 64kbps
	{
		{  6, 11, 28,  3, 24, 18, 12, 18, 4},
		{  6, 10, 29,  3, 23, 13,  8, 13, 8},	// 3 -> 13
		{  6, 12, 27,  3, 16,  8,  6,  9, 0},
		{  6,  9, 33,  0, 11,  6,  5,  0, 0},
		{  6,  9, 31,  2,  5,  3,  2,  3, 0}	// 20
	},

	// 80kbps
	{
		{  6, 10, 41,  3, 24, 17, 12, 18, 4},
		{  6, 10, 41,  3, 23, 13,  8, 13, 8},	// 4 -> 3
		{  6, 11, 40,  3, 16,  8,  6,  7, 0},
		{  6, 10, 41,  3, 11,  6,  5,  6, 0},
		{  6, 10, 41,  3,  6,  3,  2,  3, 0}	// 25
	},

	// 96kbps
	{
		{  6, 13, 50,  3, 24, 18, 13, 19, 0},
		{  6, 10, 53,  3, 22, 12,  9, 12, 0},
		{  6, 12, 51,  3, 16,  9,  6, 10, 4},
		{  7, 10, 52,  3,  9,  6,  4,  6, 0},
		{  7,  9, 53,  3,  5,  4,  2,  4, 0}	// 30
	},

	// 112kbps
	{
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	// not use
		{ 11, 21, 49,  3, 23, 12,  9, 14, 4},
		{ 11, 23, 47,  3, 16,  8,  6,  9, 0},
		{ 11, 21, 49,  3,  9,  6,  4,  8, 0},
		{ 14, 17, 50,  3,  5,  4,  2,  5, 0}	// 35
	},

	// 128kbps
	{
		{ 11, 20, 62,  3, 24, 17, 13, 19, 8},
		{ 11, 21, 61,  3, 22, 12,  9, 14, 0},
		{ 11, 22, 60,  3, 16,  9,  6, 10, 4},
		{ 11, 21, 61,  3, 11,  6,  5,  7, 0},
		{ 12, 19, 62,  3,  5,  3,  2,  4, 0}	// 40
	},

	// 160kbps
	{
		{ 11, 22, 84,  3, 24, 18, 12, 19, 0},
		{ 11, 21, 85,  3, 22, 11,  9, 13, 0},
		{ 11, 24, 82,  3, 16,  8,  6, 11, 0},
		{ 11, 23, 83,  3, 11,  6,  5,  9, 0},
		{ 11, 19, 87,  3,  5,  4,  2,  4, 0}	// 45
	},

	// 192kbps
	{
		{ 11, 21,109,  3, 24, 20, 13, 24, 0},
		{ 11, 20,110,  3, 22, 13,  9, 13, 8},
		{ 11, 24,106,  3, 16, 10,  6, 11, 0},
		{ 11, 22,108,  3, 10,  6,  4,  9, 0},
		{ 11, 20,110,  3,  6,  4,  2,  5, 0}	// 50
	},

	// 224kbps
	{
		{ 11, 24,130,  3, 24, 20, 12, 20, 4},
		{ 11, 22,132,  3, 24, 16, 10, 15, 0},
		{ 11, 20,134,  3, 16, 10,  7,  9, 0},
		{ 12, 26,127,  3, 12,  8,  4, 11, 0},
		{ 12, 22,131,  3,  8,  6,  2,  6, 4}	// 55
	},

	// 256kbps
	{
		{ 11, 26,152,  3, 24, 19, 14, 18, 4},
		{ 11, 22,156,  3, 24, 14, 10, 13, 8},
		{ 11, 27,151,  3, 16, 10,  7, 10, 0},
		{ 11, 24,154,  3, 12,  9,  5, 10, 4},
		{ 11, 24,154,  3,  6,  5,  2,  5, 0}	// 60
	},

	// 320kbps
	{
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	// not use
		{ 11, 26,200,  3, 24, 17,  9, 17, 0},
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	// not use
		{ 11, 25,201,  3, 13,  9,  5, 10, 8},
		{ 11, 26,200,  3,  8,  5,  2,  6, 4}	// 65
	},

	// 384kbps
	{
		{ 12, 28,245,  3, 24, 20, 14, 23, 8},
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	// not use
		{ 11, 24,250,  3, 16,  9,  7, 10, 4},
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	// not use
		{ 11, 27,247,  3,  8,  6,  2,  7, 0}	// 70
	}
};

int crc_good_cnt = 0;
int crc_bad_cnt = 0;
int fic_nice_cnt = 0;

int announcement = 0;

void DidpPrn(didpInfo_t *pDidp);
void SubChannelOrganizationPrn(int subChId);
int dummy_decoder(u8 *fibBuffer, int figLength);
int FoundAllLabels(void);

int bbm_recfg_flag = 0;

esbInfo_t *GetEsbInfo(void)
{
	return &gEsbInfo[0];
}

subChInfo_t *GetSubChInfo(u8 subChId) 
{
	subChInfo_t *pSubChInfo;
	int i;

	for(i=0; i<MAX_SUBCH_NUM; i++) {
		pSubChInfo = &subChInfo[i];
		if((pSubChInfo->flag != 0) && (pSubChInfo->subChId == subChId))
			break;
	}

	if(i == MAX_SUBCH_NUM) {
		for(i=0; i<MAX_SUBCH_NUM; i++) {
			pSubChInfo = &subChInfo[i];
			if(pSubChInfo->flag == 0)
				break;
		}
		if(i == MAX_SUBCH_NUM)
			return NULL;
	}

	return pSubChInfo;		
}

svcInfo_t *GetSvcInfoList(u8 SvcIdx)
{
	return &gSvcInfo[SvcIdx];
}

svcInfo_t *GetSvcInfo(u32 SId)
{
	svcInfo_t *pSvcInfo;
	int i;

	for(i=0; i<MAX_SVC_NUM; i++) {
		pSvcInfo = &gSvcInfo[i];
		if ((pSvcInfo->flag != 0) && (SId == pSvcInfo->SId))
			break;
	}

	if(i == MAX_SVC_NUM) {
		for(i=0; i<MAX_SVC_NUM; i++) {
			pSvcInfo = &gSvcInfo[i];
			if(pSvcInfo->flag == 0) {
				pSvcInfo->SId = SId;
				break;
			}
		}
		if(i == MAX_SVC_NUM)
			return NULL;
	}

	return pSvcInfo;
}

scInfo_t *GetScInfo(u16	SCId)
{
	scInfo_t  *pScInfo;
	int i;

	for(i=0; i<MAX_SC_NUM; i++) { 
		pScInfo = &gScInfo[i];
		if((pScInfo->flag == 99) && (pScInfo->SCId == SCId)) {
			//pScInfo->SCId = 0xffff;
			break;
		}
	}
	if(i == MAX_SVC_NUM) {
		for(i=0; i<MAX_SVC_NUM; i++) {
			pScInfo = &gScInfo[i];
			if(pScInfo->flag == 0) {
				break;
			}
		}
		if(i == MAX_SC_NUM) 
			return NULL;
	}

	return pScInfo;
}

static unsigned short crc16(unsigned char *fibBuffer, int len)
{
	int i, j, k;
	unsigned int sta, din;
	unsigned int crc_tmp=0x0; 
	int crc_buf[16];
	int crc_coff[16] = {		// CRC16 CCITT REVERSED
		0, 0, 0, 0, 	// 0x0
		1, 0, 0, 0, 	// 0x8
		0, 0, 0, 1, 	// 0x1
		0, 0, 0, 1	// 0x1
	};

	for(j=0; j<16; j++) 
		crc_buf[j] = 0x1;

	for(i=0; i<len; i++)
	{
		sta = fibBuffer[i] & 0xff;

		for(k=7; k>=0; k--)
		{
			din = ((sta >> k) & 0x1) ^ (crc_buf[15] & 0x1);

			for(j=15; j>0; j--) 
				crc_buf[j] = (crc_buf[j-1] & 0x1) ^ ((crc_coff[j-1] * din) & 0x1);

			crc_buf[0] = din;
		}
	}

	crc_tmp = 0;
	for(j=15; j>=0; j--) 
		crc_tmp = (crc_tmp << 1) | (crc_buf[j] & 0x1);

	return ~crc_tmp & 0xffff;	
}

int fic_crc_ctrl = 1;		// fic crc check enable

int fic_decoder(Fic *pFic, u16 length)
{
	Fib 	*pFib;
	int 	result = 0;
	int	i;
	u16	bufferCnt;

	bufferCnt = length;

	if(bufferCnt % 32) {
		//PRINTF(NULL, "FIC BUFFER LENGTH ERROR %d\n", bufferCnt);
		return 1;
	}

	for(i=0; i<bufferCnt/32; i++) {
		pFib = &pFic->fib[i];
		if(fic_crc_ctrl) {
			if(crc16(pFib->data,30) == BYTESWAP(pFib->crc)) {
				crc_good_cnt++;
				result = fib_decoder(pFib);
			} else {
				crc_bad_cnt++;
				//PRINTF(NULL, "CRC ERROR: FIB %d\n", i);
			}
		} else {
			result = fib_decoder(pFib);
			crc_good_cnt++;
		}
	}

	return result;
}

int fib_decoder(Fib *pFib)
{
	Fig  *pFig;
	int  type, length;
	int  fib_ptr = 0;
	int  result = 0;

	while (fib_ptr < 30) {
		pFig = (Fig *)&pFib->data[fib_ptr];

		type = (pFig->head >> 5) & 0x7;
		length = pFig->head & 0x1f;

		if(pFig->head == 0xff || !length) {	 // end mark
			break;
		}

		fic_nice_cnt++;

		switch(type)
		{
			case 0: 
				result = fig0_decoder(pFig);		// MCI & SI
				break;			
			case 1: 
				result = fig1_decoder(pFig);		// SI
				if(result) {
					//PRINTF(NULL, "SI Error [%x]\n", result);
				}
				break;
#if 0
			case 5: 
				result = fig5_decoder(pFig);		// FIDC
				break;
			case 6: 
				result = fig6_decoder(pFig);		// CA
				break;
#endif
			default: 
				//PRINTF(NULL, "FIG 0x%X Length : 0x%X 0x%X\n", type, length, fib_ptr);
				result = 1;
				break;
		}

		fib_ptr += length + 1;
	}

	return result;
}

/*
 * MCI & SI 
 */
static int fig0_decoder(Fig *pFig)
{
	int result = 0;
	int extension,length, pd;
	u8  cn;

	length = pFig->head & 0x1f;
	cn = (pFig->data[0] & 0x80) >> 7;
	if ((bbm_recfg_flag == 1) && (cn == 0)) 
			return 0;
	//if(cn) 
	//	PRINTF(NULL, "N");

	extension = pFig->data[0] & 0x1F;
	pd = (pFig->data[0] & 0x20) >> 5;

	switch (extension) {
		case 1:
			result = fig0_ext1_decoder(cn, &pFig->data[1], length);
			break;
		case 2:
			result = fig0_ext2_decoder(&pFig->data[1], length, pd);
			break;
		case 3:		// Service component in packet mode or without CA
			result = fig0_ext3_decoder(&pFig->data[1], length);
			break;
		case 4:		// Service component with CA
			//result = fig0_ext4_decoder(&pFig->data[1], length);
			break;
		case 10:	// Date & Time
			result = fig0_ext10_decoder(&pFig->data[1], length-1);
			break;
		case 13:
			result = fig0_ext13_decoder(&pFig->data[1], length, pd);
			break;
		case 14:    // FEC
			result = fig0_ext14_decoder(&pFig->data[1], length);
			break;
		case 15:
			result = fig0_ext15_decoder(&pFig->data[1], length, pd);
			break;
		case 0:		// Ensembel Information
		case 5:		// Language
		case 8:		// Service component global definition
		case 9:		// Country LTO and International table
		case 17:	// Programme Type
			result = dummy_decoder(&pFig->data[1], length);
			break;
		case 18:	// Announcements
			if(announcement)
				result = fig0_ext18_decoder(&pFig->data[1], length);
			break;
		case 19:	// Announcements switching
			//PRINTF(NULL, "FIG 0x%X/0x%X Length : 0x%X\n", 0, extension, length);
			break;
		default:
			//PRINTF(NULL, "FIG 0x%X/0x%X Length : 0x%X\n", 0, extension, length);
			result = 1;
			break;
	}

	return result;
}

static int fig1_decoder(Fig *pFig)
{
	int result = 0;
	int length;
	int /*charset, oe,*/ extension;

	length = pFig->head & 0x1f;
	//charset = (pFig->data[0] >> 4) & 0xF;
	//oe = (pFig->data[0]) >> 3 & 0x1;
	extension = pFig->data[0] & 0x7;

	switch (extension) {
		case 0:
			result = fig1_ext0_decoder(&pFig->data[1],length);	// Ensembel Label
			break;
		case 1:
			result = fig1_ext1_decoder(&pFig->data[1],length);	// Programme service Label
			break;
		case 5:
			result = fig1_ext5_decoder(&pFig->data[1],length);	// Data service Label
			break;
		case 4:
			result = fig1_ext4_decoder(&pFig->data[1],length);	// Service component Label
			break;
		default:
			//PRINTF(NULL, "FIG 0x%X/0x%X Length : 0x%X\n", 1, extension, length);
			result = 1;
			break;
	}

	return result;
}

int dummy_decoder(u8 *fibBuffer, int figLength)
{
	return 0;
}

/*
 *  FIG 0/1 MCI, Sub Channel Organization 
 */
int fig0_ext1_decoder(u8 cn,u8 *fibBuffer, int figLength)
{
	u8	sta;
	int 	result = 0;
	int	readcnt = 0;

	u8 	subChId;
	subChInfo_t	*pSubChInfo;

	while(figLength-1 > readcnt) {
		sta = fibBuffer[readcnt++];
		if(sta == 0xFF)
			break;
		subChId = (sta >> 2) & 0x3F;
		pSubChInfo = GetSubChInfo(subChId);
		if(pSubChInfo == NULL) {
			//PRINTF(NULL, "subChInfo error ..\n");
			return 1;
		}

		pSubChInfo->flag = 99;
		pSubChInfo->mode = 0; 		// T-DMB
		pSubChInfo->subChId = subChId;

		pSubChInfo->startAddress = ( sta & 0x3) << 8;
		sta = fibBuffer[readcnt++];
		pSubChInfo->startAddress |= sta;
		sta = fibBuffer[readcnt++];
		pSubChInfo->formType = (sta & 0x80) >> 7;

		switch (pSubChInfo->formType) {
			case	0:	// short form
				pSubChInfo->tableSwitch = (sta & 0x40) >> 6; 
				pSubChInfo->tableIndex = sta & 0x3f;
				break;
			case	1:	// long form
				pSubChInfo->option = (sta & 0x70) >> 4; 
				pSubChInfo->protectLevel = (sta & 0x0c) >> 2;
				pSubChInfo->subChSize = (sta & 0x03) << 8;
				sta = fibBuffer[readcnt++];
				pSubChInfo->subChSize |= sta;
				break;
			default:
				//PRINTF(NULL, "Unknown Form Type %d\n", pSubChInfo->formType);
				result = 1;
				break;
		}
		if(cn) {
			if(pSubChInfo->reCfg == 0) {
				pSubChInfo->reCfg = 1;		// ReConfig Info Updated
			}
		}
	}
       
	return result;
}

/*
 *  FIG 0/2 MCI, Sub Channel Organization 
 */
static int fig0_ext2_decoder(u8 *fibBuffer, int figLength, int pd)
{
	svcInfo_t *pSvcInfo;
	subChInfo_t	*pSubChInfo;
	u8	sta;
	int 	result = 0;
	int	readcnt = 0;
	u32	SId = 0xffffffff;
	int  	nscps;
	u32	temp;
	int 	TMId;
	int	i;

	while(figLength-1 > readcnt) {
		temp = 0;

		temp = fibBuffer[readcnt++];
		temp = (temp << 8) | fibBuffer[readcnt++];


		switch (pd) {
			case 0:		// 16-bit SId, used for programme services
				{
					temp = temp;
					//SId = temp & 0xFFF;
					SId = temp;
				}
				break;
			case 1:		//32bit SId, used for data service
				{
					temp = (temp << 8) | fibBuffer[readcnt++];
					temp = (temp << 8) | fibBuffer[readcnt++];

					//SId = temp & 0xFFFFF;
					SId = temp;
				}
				break;
			default:
				break;
		}

		pSvcInfo = GetSvcInfo(SId);
		if(pSvcInfo == NULL) {
			//PRINTF(NULL, "GetSvcInfo Error ...\n");
			break;
		}
	
		pSvcInfo->addrType = pd;
		pSvcInfo->SId = SId;
		pSvcInfo->flag |= 0x02;

		sta = fibBuffer[readcnt++];    // flag, CAId, nscps 

		nscps = sta & 0xF;

		pSvcInfo->nscps = nscps;

		for(i=0; i<nscps; i++) {
			sta = fibBuffer[readcnt++];
			TMId = (sta >> 6) & 0x3;
			//pSvcInfo->TMId = TMId;

			switch(TMId) {
				case 0:		// MSC stream audio
					pSvcInfo->ASCTy = sta & 0x3f;
					sta = fibBuffer[readcnt++];
					if ((sta & 0x02) == 0x02) {		// Primary
						pSvcInfo->SubChId = (sta >> 2) & 0x3F;
						pSvcInfo->TMId = TMId;
					}
					pSubChInfo = GetSubChInfo(pSvcInfo->SubChId);
					if(pSubChInfo == NULL) {
						//PRINTF(NULL, "GetSubChInfo Error ...\n");
						return 1;
					}
					pSubChInfo->SId = pSvcInfo->SId;
					pSvcInfo->flag |= 0x04;
					break;
				case 1:		// MSC stream data
					pSvcInfo->DSCTy = sta & 0x3f;
					sta = fibBuffer[readcnt++];
					if ((sta & 0x02) == 0x02) {		// Primary
						pSvcInfo->SubChId = (sta >> 2) & 0x3F;
						pSvcInfo->TMId = TMId;
					}
					pSubChInfo = GetSubChInfo(pSvcInfo->SubChId);
					if(pSubChInfo == NULL) {
						//PRINTF(NULL, "GetSubChInfo Error ...\n");
						return 1;
					}
					pSubChInfo->SId = pSvcInfo->SId;
					pSvcInfo->flag |= 0x04;
					break;
				case 2:		// FIDC
					pSvcInfo->DSCTy = sta & 0x3f;
					sta = fibBuffer[readcnt++];
					if ((sta & 0x02) == 0x02) {		// Primary
						pSvcInfo->FIDCId = (sta & 0xFC) >> 2;
						pSvcInfo->TMId = TMId;
					}
					pSvcInfo->flag |= 0x04;
					break;
				case 3:		// MSC packet data
					pSvcInfo->SCId = (sta & 0x3F) << 6;
					sta = fibBuffer[readcnt++];
					if ((sta & 0x02) == 0x02) { 		// Primary
						pSvcInfo->SCId |= (sta & 0xFC) >> 2;
						pSvcInfo->TMId = TMId;
					}
					// by iproda
					pSvcInfo->flag |= 0x04;
					break;
				default:
					//PRINTF(NULL, "Unkown TMId [%X]\n", TMId);
					result = 1;
					break;
			}
		}
	}

	return result;
}

int fig0_ext3_decoder(u8 *fibBuffer, int figLength)
{
	u8	sta;
	int 	result = 0;
	int	readcnt = 0;
	u16	SCId;
	int i;

	scInfo_t	*pScInfo;
	svcInfo_t 	*pSvcInfo;
	subChInfo_t 	*pSubChInfo;

	while(figLength-1 > readcnt) {
		SCId = 0;
		sta = fibBuffer[readcnt++];
		SCId = sta;
		SCId = SCId << 4;
		sta = fibBuffer[readcnt++];
		SCId |= (sta & 0xf0) >> 4;

		pScInfo = GetScInfo(SCId);
		if(pScInfo == NULL) {
			//PRINTF(NULL, "GetScInfo Error ...\n");
			return 1;
		}

		pScInfo->flag = 99;
		pScInfo->SCId = SCId;
		pScInfo->SCCAFlag = sta & 0x1;
		sta = fibBuffer[readcnt++];
		pScInfo->DGFlag = (sta & 0x80) >> 7;
		pScInfo->DSCTy = (sta & 0x3f);
		sta = fibBuffer[readcnt++];
		pScInfo->SubChId = (sta & 0xfc) >> 2;
		pScInfo->PacketAddress = sta & 0x3;
		pScInfo->PacketAddress = pScInfo->PacketAddress << 8;
		sta = fibBuffer[readcnt++];
		pScInfo->PacketAddress |= sta;
		if(pScInfo->SCCAFlag) {
			sta = fibBuffer[readcnt++];
			pScInfo->SCCA = sta;
			pScInfo->SCCA = pScInfo->SCCA << 8;
			sta = fibBuffer[readcnt++];
			pScInfo->SCCA |= sta;
		}

		for(i=0; i<MAX_SVC_NUM; i++) {
			pSvcInfo = &gSvcInfo[i];
			if(pSvcInfo->SCId == pScInfo->SCId && pSvcInfo->TMId == 3) {
				pSubChInfo = GetSubChInfo(pScInfo->SubChId);
				if(pSubChInfo == NULL) {
					//PRINTF(NULL, "GetSubChInfo Error ...\n");
					return 1;
				}

				pSubChInfo->SId = pSvcInfo->SId;
				pSvcInfo->SubChId = pSubChInfo->subChId;
			}
		}
	}
       
	return result;
}

/*int fig0_ext4_decoder(u8 *fibBuffer, int figLength) {
	int result = 0;
	int readcnt = 0;
	int Mf, SubChId, CAOrg;

	while(figLength - 1 > readcnt) {
		Mf      = (fibBuffer[readcnt] & 0x40) >> 6;
		SubChId = (fibBuffer[readcnt] & 0x3f);
		CAOrg   = (fibBuffer[readcnt + 1] << 8) + fibBuffer[readcnt + 2];
		readcnt += 3;
		//PRINTF(NULL, "CA MF: %d, SubChiD: %d, CAOrg: %d\n", Mf, SubChId, CAOrg);
	}

	return result;
}*/

/*
 *  FIG 0/10 Date & Time
 */
int fig0_ext10_decoder(u8 *fibBuffer, int figLength)
{
	int result = 0;

	u8 MJD,  /*ConfInd,*/ UTCflag;
	//u16 LSI;
	u8 hour = 0; /*minutes = 0, seconds = 0*/
	u16 milliseconds = 0;

	MJD = (fibBuffer[0] & 0x7f) << 10;
	MJD |= (fibBuffer[1] << 2);
	MJD |= (fibBuffer[2] & 0xc0) >> 6;
	//LSI = (fibBuffer[2] & 0x20) >> 5;
	//ConfInd = (fibBuffer[2] & 0x10) >> 4;
	UTCflag = (fibBuffer[2] & 0x08) >> 3;

	hour = (fibBuffer[2] & 0x07) << 2;
	hour |= (fibBuffer[3] & 0xc0) >> 6;
	
	//minutes = fibBuffer[3] & 0x3f;

	if(UTCflag) {
		//seconds = (fibBuffer[4] & 0xfc) >> 2;
		milliseconds = (fibBuffer[4] & 0x03) << 8;
		milliseconds |= fibBuffer[5];
	}

	//PRINTF(NULL, " %d:%d:%d.%d\n", hour+9, minutes, seconds, milliseconds);

	return result;
}

/*
 *  FIG 0/13 Announcement
 */
int fig0_ext13_decoder(u8 *fibBuffer, int figLength, int pd)
{
	u8	sta;
	int 	result = 0;
	int	readcnt = 0;
	u32	SId = 0xffffffff;
	//u8	SCIdS;
	u8	NumOfUAs;
	u16	UAtype;
	u8	UAlen;
	int 	i,j;

	svcInfo_t 	*pSvcInfo;

#if 0
	PRINTF(NULL, "FIG0/13 = 0x%X\n", figLength);

	for(i=0; i<figLength; i++) {
		PRINTF(NULL, "0x%X ", fibBuffer[i]);
	}
	PRINTF(NULL, "\n");
#endif

	while(figLength-1 > readcnt) {
		switch (pd) {
			case 0:		// 16-bit SId, used for programme services
				{
					u32 temp;

					temp = 0;

					temp = fibBuffer[readcnt++];
					temp = (temp << 8) | fibBuffer[readcnt++];

					SId = temp;
				}
				break;
			case 1:		//32bit SId, used for data service
				{
					u32 temp;

					temp = 0;

					temp = fibBuffer[readcnt++];
					temp = (temp << 8) | fibBuffer[readcnt++];
					temp = (temp << 8) | fibBuffer[readcnt++];
					temp = (temp << 8) | fibBuffer[readcnt++];

					SId = temp;
				}
				break;
			default:
				break;
		}

		pSvcInfo = GetSvcInfo(SId);
		if(pSvcInfo == NULL) {
			//PRINTF(NULL, "GetSvcInfo Error ...\n");
			break;
		}
		pSvcInfo->SId = SId;

		pSvcInfo->flag |= 0x04;

		sta = fibBuffer[readcnt++];

		//SCIdS = (sta & 0xff00) >> 4;
		NumOfUAs = sta & 0xff;

#if 1 // Because of Visual Radio
		pSvcInfo->NumberofUserAppl = NumOfUAs;
#endif

		for(i=0; i<NumOfUAs; i++) {
			UAtype = 0;
			sta = fibBuffer[readcnt++];
			UAtype = sta;
			sta = fibBuffer[readcnt++];
			UAtype = (UAtype << 3) | ((sta >> 5) & 0x07);

#if 1 // Because of Visual Radio
			UAlen = sta & 0x1f;

			pSvcInfo->UserApplType[i] = UAtype;
			pSvcInfo->UserApplLength[i] = UAlen;
				
			for(j=0; j<UAlen; j++) {
				sta = fibBuffer[readcnt++];
				pSvcInfo->UserApplData[i][j] = sta;
			}
#else
			pSvcInfo->UAtype = UAtype;
			UAlen = sta & 0x1f;

			for(j=0; j<UAlen; j++) {
				sta = fibBuffer[readcnt++];
			}
#endif
		}
#if 0
		PRINTF(NULL, "SId = 0x%X\n", pSvcInfo->SId);
		PRINTF(NULL, "UAtype = 0x%X\n", pSvcInfo->UAtype);
		PRINTF(NULL, "NumOfUAs = 0x%X\n", NumOfUAs);
		PRINTF(NULL, "UAlen = 0x%X\n", UAlen);
#endif
	}

	return result;
}

int fig0_ext14_decoder(u8 *fibBuffer, int figLength) 
{
	int result = 0;
	int	readcnt = 0;
	unsigned char subch, fec_scheme;
	subChInfo_t* pSubChInfo;

	while(figLength-1 > readcnt) {
		subch = (fibBuffer[readcnt] & 0xfc) >> 2;
		fec_scheme = (fibBuffer[readcnt] & 0x03);
		readcnt++;
		// PRINTF(NULL, "SubChID: %d, FEC Scheme: %d\n", subch, fec_scheme);
		pSubChInfo = GetSubChInfo(subch);
		if(pSubChInfo)
			pSubChInfo->fecScheme = fec_scheme;
	}

	return result;
}

 /*
 * TMMB kjju TODO
 */
int fig0_ext15_decoder(u8 *fibBuffer, int figLength, int pd)
{
	u8	sta;
	int 	result = 0;
	int	readcnt = 0;
	u8 subChId;
	subChInfo_t 	*pSubChInfo;

	while(figLength-1 > readcnt) {
		sta = fibBuffer[readcnt++];
		if(sta == 0xFF)
			break;

		subChId = (sta & 0xfc) >> 2;
		pSubChInfo = GetSubChInfo(subChId);
		if(pSubChInfo == NULL) {
			//PRINTF(NULL, "subChInfo error ..\n");
			return 1;
		}

		pSubChInfo->flag = 99;
		pSubChInfo->mode = 1; 		// T-MMB
		pSubChInfo->subChId = subChId;
		pSubChInfo->startAddress = (sta & 0x3) << 8;

		sta = fibBuffer[readcnt++];
		pSubChInfo->startAddress |= sta;

		pSubChInfo = GetSubChInfo(pSubChInfo->subChId);
		if(pSubChInfo == NULL) {
			//PRINTF(NULL, "subChInfo error ..\n");
			return 1;
		}

		sta = fibBuffer[readcnt++];

		pSubChInfo->modType = (sta & 0xc0) >> 6;
		pSubChInfo->encType = (sta & 0x20) >> 5;
		pSubChInfo->intvDepth = (sta & 0x18) >> 3;
		pSubChInfo->pl = (sta & 0x04) >> 2;
		pSubChInfo->subChSize = (sta & 0x03) << 8;

		sta = fibBuffer[readcnt++];
		pSubChInfo->subChSize |= sta;
	}

#if 0
	PRINTF(NULL, "subChId = 0x%x\n", pSubChInfo->subChId);
	PRINTF(NULL, "mode = 0x%x\n", pSubChInfo->mode);
	PRINTF(NULL, "modType = 0x%x\n", pSubChInfo->modType);
	PRINTF(NULL, "encType = 0x%x\n", pSubChInfo->encType);
	PRINTF(NULL, "intvDepth = 0x%x\n", pSubChInfo->intvDepth);
	PRINTF(NULL, "pl = 0x%x\n", pSubChInfo->pl);
	PRINTF(NULL, "startAddress = 0x%x\n", pSubChInfo->startAddress);
	PRINTF(NULL, "subChSize = 0x%x\n", pSubChInfo->subChSize);
#endif

	return result;
}

/*
 *  FIG 0/18 Announcement
 */
int fig0_ext18_decoder(u8 *fibBuffer, int figLength)
{
	u8	sta;
	int 	result = 0;
	int	readcnt = 0;
	u16	SId;
	//u8	CId;
	u16  	AsuFlag;
	int  	nocs;
	int	i;

	while(figLength-1 > readcnt) {
		sta = fibBuffer[readcnt++];
		SId = sta << 8;
		sta = fibBuffer[readcnt++];
		SId |= sta;
		//PRINTF(NULL, "SId = 0x%X, ", SId);
		
		sta = fibBuffer[readcnt++];
		AsuFlag = sta << 8;
		sta = fibBuffer[readcnt++];
		AsuFlag |= sta;
		//PRINTF(NULL, "AsuFlag = 0x%X, ", AsuFlag);

		sta = fibBuffer[readcnt++];
		nocs = sta & 0x1F;
		//PRINTF(NULL, "nocs = 0x%X, ", nocs);

		for(i=0; i<nocs; i++) {
			sta = fibBuffer[readcnt++];
			//CId = sta;
			//PRINTF(NULL, "CId = %d, ", CId);
		}
		//PRINTF(NULL, "\n");
	}

	return result;
}

static int fig1_ext0_decoder(u8 *fibBuffer, int figLength)
{
	int 	result = 0;
	int	readcnt = 0;
	int  	i;

	u16 EId;
	u16 flag;
	
	EId = 0;
	EId = fibBuffer[readcnt++];
	EId = EId << 8 | fibBuffer[readcnt++];

	for(i=0; i<16; i++) 
	 	gEsbInfo[0].label[i] = fibBuffer[readcnt++];

	flag = 0;
	flag = fibBuffer[readcnt++];
	flag = flag << 8 | fibBuffer[readcnt++];

	gEsbInfo[0].label[16] = '\0';
	gEsbInfo[0].flag = 99;
	gEsbInfo[0].EId  = EId;
	//PRINTF(DMB_FIC_INFO"FIG 1/0 label [%x][%s]\n", EId, gEsbInfo[0].label);

#if 1	// test label filter
	for(i = 16-1; i >= 0; i--)
	{
		if(gEsbInfo[0].label[i] == 0x20) 
		{
		    gEsbInfo[0].label[i] = 0;
    }
		else
		{
			if(i == 16-1) 
				gEsbInfo[0].label[i] = 0;
			break;
    }			
    
  }    
#endif	

	return result;
}

static int fig1_ext1_decoder(u8 *fibBuffer, int figLength)
{
	svcInfo_t *pSvcInfo;
	u32	temp;
	int 	result = 0;
	int	readcnt = 0;
	int 	i;

	u16 SId;
	
	temp = 0;
	temp = fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];

	SId = temp;

	pSvcInfo = GetSvcInfo(SId);
	if(pSvcInfo == NULL) {
		//PRINTF(NULL, "GetSvcInfo Error ...\n");
		return 1;
	}

	pSvcInfo->SId = SId;

	pSvcInfo->flag |= 0x01;

	for(i=0; i<16; i++) {
		pSvcInfo->label[i] = fibBuffer[readcnt++];
	}

	pSvcInfo->label[16] = '\0';
	//PRINTF(NULL, "FIG 1/1 label [%x][%s]\n", SId, pSvcInfo->label);

	return result;
}

static int fig1_ext4_decoder(u8 *fibBuffer, int figLength)
{
	scInfo_t  *pScInfo;
	u8	sta;
	u8	pd;
	u32	temp;
	int 	result = 0;
	int	readcnt = 0;
	int 	i;

	u16 	SCId;
	//u32		SId;
	u16 	flag;
	
	sta = fibBuffer[readcnt++];

	pd = (sta & 0x80) >> 7;
	SCId = (sta &0x0f);

	temp = 0;
	temp = fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];

	if(pd) {
		temp = temp << 8 | fibBuffer[readcnt++];
		temp = temp << 8 | fibBuffer[readcnt++];
		//SId = temp;
	} else {
		//SId = temp;
	}

	pScInfo = GetScInfo(SCId);
	if(pScInfo == NULL) {
		//PRINTF(NULL, "GetSvcInfo Error ...\n");
		return 1;
	}

	pScInfo->flag = 99;
	pScInfo->SCId = SCId;

	for(i=0; i<16; i++)
		pScInfo->label[i] = fibBuffer[readcnt++];

	flag = 0;
	flag = fibBuffer[readcnt++];
	flag = flag << 8 | fibBuffer[readcnt++];

	pScInfo->label[16] = '\0';
	//PRINTF(NULL, "FIG 1/4 label [%x][%s]\n", SId, pScInfo->label);

	return result;
}

static int fig1_ext5_decoder(u8 *fibBuffer, int figLength)
{
	svcInfo_t *pSvcInfo;
	u32	temp;
	int 	result = 0;
	int	readcnt = 0;
	int 	i;

	u32 SId;
	u16 flag;
	
	temp = 0;
	temp = fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];

	SId = temp;

	pSvcInfo = GetSvcInfo(SId);
	if(pSvcInfo == NULL) {
		//PRINTF(NULL, "GetSvcInfo Error ...\n");
		return 1;
	}

	pSvcInfo->SId = SId;

	pSvcInfo->flag |= 0x01;

	for(i=0; i<16; i++)
		pSvcInfo->label[i] = fibBuffer[readcnt++];

	flag = 0;
	flag = fibBuffer[readcnt++];
	flag = flag << 8 | fibBuffer[readcnt++];

	pSvcInfo->label[16] = '\0';

#if 1	// test label filter
	for(i = 16-1; i >= 0; i--)
	{
		if(pSvcInfo->label[i] == 0x20) 
		{
		    pSvcInfo->label[i] = 0;
    }
		else
		{
			if(i == 16-1) 
				pSvcInfo->label[i] = 0;
			break;
    }			
    
  }    
#endif	
	//PRINTF(NULL, "FIG 1/5 label [%x][%s]\n", SId, pSvcInfo->label);

	return result;
}

void SubChannelOrganizationPrn(int subChId)
{
	didpInfo_t  didp;
	subChInfo_t *pSubChInfo;

	memset(&didp, 0, sizeof(didp));

	pSubChInfo = GetSubChInfo(subChId);
	if(pSubChInfo == NULL)
		return;

	if(pSubChInfo->flag == 99) {
		SubChOrgan2DidpReg(pSubChInfo, &didp);
		//if(pSubChInfo->svcChId & 0x40)
		//	PRINTF(NULL, "svcChId = 0x%X, ", pSubChInfo->svcChId & 0x3F);
		//else
		//	PRINTF(NULL, "svcChId = NOTUSE, ");

		switch(pSubChInfo->reCfg)  {
			case 0:
				//PRINTF(NULL, "reCfg = INIT\n");
				break;
			case 1:
				//PRINTF(NULL, "reCfg = UPDATED\n");
				break;
			case 2:
				//PRINTF(NULL, "reCfg = DONE\n");
				break;
		}

		//PRINTF(NULL, "SId = 0x%X\n", pSubChInfo->SId);
		// DidpPrn(&didp);
	}
}

void SubChannelOrganizationClean(void)
{
	int i;

#if 1
	memset(gEsbInfo, 0, sizeof(esbInfo_t) * MAX_ESB_NUM);
	memset(gSvcInfo,0, sizeof(svcInfo_t) * MAX_SVC_NUM);
	memset(gScInfo,0, sizeof(scInfo_t) * MAX_SC_NUM);
	memset(subChInfo,0, sizeof(subChInfo_t) * MAX_SUBCH_NUM);
#endif

	for(i=0; i<MAX_SUBCH_NUM; i++) {
		subChInfo[i].flag = 0;
	}

	for(i=0; i<MAX_SVC_NUM; i++) {
		gSvcInfo[i].flag = 0;
		gSvcInfo[i].SCId = 0xffff;
	}

	for(i=0; i<MAX_SC_NUM; i++) {
		gScInfo[i].SCId = 0xffff;
	}

	return;
}

int BitRate2Index(u16 bitrate)
{
	int index;

	switch (bitrate) {
		case 32: index =  0; break;
		case 48: index =  1; break;
		case 56: index =  2; break;
		case 64: index =  3; break;
		case 80: index =  4; break;
		case 96: index =  5; break;
		case 112: index =  6; break;
		case 128: index =  7; break;
		case 160: index =  8; break;
		case 192: index =  9; break;
		case 224: index =  10; break;
		case 256: index =  11; break;
		case 320: index =  12; break;
		case 384: index =  13; break;
		default: index =  -1; break;
	}

	return index;
}

int GetN(subChInfo_t *pSubChInfo,int *n)
{
	int result = 0;

	switch (pSubChInfo->option) {
		case 0: 
			switch (pSubChInfo->protectLevel) {
				case 0:
					*n = pSubChInfo->subChSize / 12;
					break;
				case 1:
					*n = pSubChInfo->subChSize / 8;
					break;
				case 2:
					*n = pSubChInfo->subChSize / 6;
					break;
				case 3:
					*n = pSubChInfo->subChSize / 4;
					break;
				default:
					//PRINTF(NULL, "Unknown Protection Level %d\n", pSubChInfo->protectLevel);
					result = 1;
					break;
			}
			break;
		case 1: 
			switch (pSubChInfo->protectLevel) {
				case 0:
					*n = pSubChInfo->subChSize / 27;
					break;
				case 1:
					*n = pSubChInfo->subChSize / 21;
					break;
				case 2:
					*n = pSubChInfo->subChSize / 18;
					break;
				case 3:
					*n = pSubChInfo->subChSize / 15;
					break;
				default:
					//PRINTF(NULL, "Unknown Protection Level %d\n", pSubChInfo->protectLevel);
					result = 1;
					break;
			}
			break;
		default:
			//PRINTF(NULL, "Unknown Option %d\n", pSubChInfo->option);
			result = 1;
			break;
	}

	return result;
}

int SubChOrgan2DidpReg(subChInfo_t *pSubChInfo, didpInfo_t *pDidp)
{
	int index, bitrate, level;
	int result = 0, n = 0;
	u16	subChSize = 0;
	u16	dataRate;
	u8  intvDepth = 0;

	if(pSubChInfo->flag != 99) 
		return 1;

	switch(pSubChInfo->mode) {
		case 0:		// T-DMB
			pDidp->mode = pSubChInfo->mode;
			switch (pSubChInfo->formType) {
				case 0: 	// short form  UEP
					pDidp->subChId = pSubChInfo->subChId;
					pDidp->startAddress = pSubChInfo->startAddress;
					pDidp->formType = pSubChInfo->formType;
					subChSize = BitRateProfile[pSubChInfo->tableIndex][0];
					pDidp->speed = BitRateProfile[pSubChInfo->tableIndex][2];

					level = BitRateProfile[pSubChInfo->tableIndex][1];
					bitrate = BitRateProfile[pSubChInfo->tableIndex][2];
					index = BitRate2Index(bitrate);

					if(index < 0) {
						result = 1;
						break;
					}
					
					pDidp->l1  = UEPProfile[index][level-1][0];
					pDidp->l2  = UEPProfile[index][level-1][1]; 
					pDidp->l3  = UEPProfile[index][level-1][2];
					pDidp->l4  = UEPProfile[index][level-1][3];
					pDidp->p1  = (u8)UEPProfile[index][level-1][4];
					pDidp->p2  = (u8)UEPProfile[index][level-1][5];
					pDidp->p3  = (u8)UEPProfile[index][level-1][6];
					pDidp->p4  = (u8)UEPProfile[index][level-1][7];
					pDidp->pad = (u8)UEPProfile[index][level-1][8];
					break;
				case 1:		// long form EEP
					pDidp->subChId = pSubChInfo->subChId;
					pDidp->startAddress = pSubChInfo->startAddress;
					pDidp->formType = pSubChInfo->formType;
					subChSize = pSubChInfo->subChSize;
					pDidp->l3 = 0;
					pDidp->p3 = 0;
					pDidp->l4 = 0;
					pDidp->p4 = 0;
					pDidp->pad = 0;

					if(GetN(pSubChInfo, &n)) {
						result = 1;
						break;
					}

					switch (pSubChInfo->option) {
						case 0: 
							switch (pSubChInfo->protectLevel) {
								case 0:
									pDidp->l1 = 6*n - 3;
									pDidp->l2 = 3;
									pDidp->p1 = 24;
									pDidp->p2 = 23;
									break;
								case 1:
									if(n > 1) {
										pDidp->l1 = 2*n - 3;
										pDidp->l2 = 4*n + 3;
										pDidp->p1 = 14;
										pDidp->p2 = 13;
									} else {
										pDidp->l1 = 5;
										pDidp->l2 = 1;
										pDidp->p1 = 13;
										pDidp->p2 = 12;
									}
									break;
								case 2:
									pDidp->l1 = 6*n - 3;
									pDidp->l2 = 3;
									pDidp->p1 = 8;
									pDidp->p2 = 7;
									break;
								case 3:
									pDidp->l1 = 4*n - 3;
									pDidp->l2 = 2*n + 3;
									pDidp->p1 = 3;
									pDidp->p2 = 2;
									break;
								default:
									result = 1;
									break;
							}
							pDidp->speed = 8*n;
							break;
						case 1:
							switch (pSubChInfo->protectLevel) {
								case 0:
									pDidp->l1 = 24*n - 3;
									pDidp->l2 = 3;
									pDidp->p1 = 10;
									pDidp->p2 = 9;
									break;
								case 1:
									pDidp->l1 = 24*n - 3;
									pDidp->l2 = 3;
									pDidp->p1 = 6;
									pDidp->p2 = 5;
									break;
								case 2:
									pDidp->l1 = 24*n - 3;
									pDidp->l2 = 3;
									pDidp->p1 = 4;
									pDidp->p2 = 3;
									break;
								case 3:
									pDidp->l1 = 24*n - 3;
									pDidp->l2 = 3;
									pDidp->p1 = 2;
									pDidp->p2 = 1;
									break;
								default:
									break;
							}
							pDidp->speed = 32*n;
							break;
						default:
							result = 1;
							break;
					}
					break;
				default:
					result = 1;
					break;
			}

			if(subChSize <= pDidp->subChSize)
				pDidp->reCfgOffset = 0;
			else 
				pDidp->reCfgOffset = 1;

			pDidp->subChSize = subChSize;
			break;
		case 1:		// T-MMB
			pDidp->mode = pSubChInfo->mode;
			pDidp->startAddress = pSubChInfo->startAddress;
			pDidp->subChId = pSubChInfo->subChId;
			pDidp->subChSize = pSubChInfo->subChSize;
			pDidp->modType = pSubChInfo->modType;
			pDidp->encType = pSubChInfo->encType;
			pDidp->intvDepth = pSubChInfo->intvDepth;
			pDidp->pl = pSubChInfo->pl;

			switch(pDidp->modType) {
				case 0:
					n =  pDidp->subChSize / 18;
					break;
				case 1:
					n =  pDidp->subChSize / 12;
					break;
				case 2:
					n =  pDidp->subChSize / 9;
					break;
				default:
					result = 1;
					break;
			}

			switch(pDidp->intvDepth) {
				case 0:
					intvDepth = 16;
					break;
				case 1:
					intvDepth = 32;
					break;
				case 2:
					intvDepth = 64;
					break;
				default:
					result = 1;
					break;
			}

#if 0	// depth 16, 32
			if(pDidp->pl) {
				dataRate = n * 32;
				pDidp->mi = (((dataRate * 3) / 2) * 24) / intvDepth;
			} else {
				dataRate = n * 24;
				pDidp->mi = ((dataRate * 2) * 24) / intvDepth;
			}
#else  // depth 64
			if(result == 1)
				break;
			
			if(pDidp->pl) {
				dataRate = n * 32;
				pDidp->mi = (((((dataRate * 3) / 2) * 24) / intvDepth ) * 3) / 4;
			} else {
				dataRate = n * 24;
				pDidp->mi = ((((dataRate * 2) * 24) / intvDepth) * 3) / 4;
			}
#endif

#if 0
			PRINTF(NULL, "mode = 0x%x\n", pDidp->mode);
			PRINTF(NULL, "startAddress = 0x%x\n", pDidp->startAddress);
			PRINTF(NULL, "subChId = 0x%x\n", pDidp->subChId);
			PRINTF(NULL, "subChSize = 0x%x\n", pDidp->subChSize);
			PRINTF(NULL, "modType = 0x%x\n", pDidp->modType);
			PRINTF(NULL, "encType = 0x%x\n", pDidp->encType);
			PRINTF(NULL, "intvDepth = 0x%x\n", pDidp->intvDepth);
			PRINTF(NULL, "pl = 0x%x\n", pDidp->pl);
			PRINTF(NULL, "mi = 0x%x\n", pDidp->mi);
#endif
			break;
		default:
			break;
	}

	return result;
}

#if 0
int FoundAllLabels(void)
{
	int NumOfSvcs = 0;
	int NumOfSubChs = 0;
	int i;
	int ret = 1;

	subChInfo_t *pSubChInfo;
	svcInfo_t *pSvcInfo;
	
	if(gEsbInfo[0].label[0] == '\0')
		return 0;
#if 0
	for(i=0; i<MAX_SUBCH_NUM; i++) {
		pSubChInfo = &subChInfo[i];
		if(pSubChInfo->flag == 99) {
			//PRINTF(NULL, "pSubChInfo->SId = 0x%X\n", pSubChInfo->SId);
			NumOfSubChs++;
		}
	}


	for(i=0; i<MAX_SUBCH_NUM; i++) {
		pSubChInfo = &subChInfo[i];
		if (pSubChInfo->flag == 99) {
			if(pSubChInfo->SId) {
				pSvcInfo = GetSvcInfo(pSubChInfo->SId);
				if(pSvcInfo == NULL) {
					ret = 0;
					break;
				}

				if((pSvcInfo->flag & 0x07) != 0x07) 
				//if((pSvcInfo->flag & 0x03) != 0x03) 
				{
					ret = 0;
					break;
				}
				NumOfSvcs++;
			}
		}
	}

	if(NumOfSvcs != NumOfSubChs) {
		ret = 0;
	}
	
	//PRINTF(NULL, "NumOfSubChs = 0x%X, NumOfSvcs = 0x%X\n", NumOfSubChs, NumOfSvcs);
#else
	for(i=0; i<MAX_SVC_NUM; i++) {
		pSvcInfo = &gSvcInfo[i];
		if(pSvcInfo->flag == 0x07) {
			if(pSvcInfo->label[0] == '\0') {
				ret = 0;
				break;
			} else {
				NumOfSvcs++;
			}
		}
	}
	
	if(NumOfSvcs == 0) {
		ret = 0;
	}
#endif
	return ret;
}
#else
int FoundAllLabels(void)
{
	msWait(1200); 
	return 1;
}
#endif
