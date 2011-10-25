/*****************************************************************************
 Copyright(c) 2008 SEC Corp. All Rights Reserved
 
 File name : DMBDrv_wrap_FC8050.c
 
 Description : fc8050 tuner control driver
 
 History : 
 ----------------------------------------------------------------------
 2009/01/19 	changsul.park		initial
 2009/09/23 	jason			porting QSC6270
*******************************************************************************/

//#include <string.h>
//#include <stdlib.h>

#include "DMBDrv_wrap_FC8050.h"
#include "fci_types.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fc8050_regs.h"
#include "fic.h"
#include "fci_tun.h"
//#include "msg.h"


extern int TDMBDrv_FIC_CALLBACK(u32 userdata, u8 *data, int length);
extern int TDMBDrv_MSC_CALLBACK(u32 userdata, u8 subChId, u8 *data, int length);
extern void tdmb_data_restore(unsigned char* pData, unsigned long dwDataSize, unsigned char ucSubChannel, unsigned char ucSvType);

SubChInfoTypeDB gDMBSubChInfo;
SubChInfoTypeDB gDABSubChInfo;

static u32 gBer = 3000;
static u32 gInitFlag = 0;

unsigned char gCurSvcType = 0x18;
unsigned char gCurSubChId = 0;
extern int bfirst;
extern int TSBuffpos;
extern int MSCBuffpos;
extern int mp2len;

int TDMBDrv_FIC_CALLBACK(u32 userdata, u8 *data, int length)
{
	FIC_DEC_Put((Fic *)data, length);

	return 0;
}

int TDMBDrv_MSC_CALLBACK(u32 userdata, u8 subChId, u8 *data, int length)
{
  
	tdmb_data_restore(&data[0], length, gCurSubChId, gCurSvcType);

	return 0;
}

static int viterbi_rt_ber_read(unsigned int* ber)
{
	u32 vframe, esum;
	u8  vt_ctrl=0;

	int res = BBM_OK;
	
	BBM_READ(NULL, BBM_VT_CONTROL, &vt_ctrl);
	vt_ctrl |= 0x10;
	BBM_WRITE(NULL, BBM_VT_CONTROL, vt_ctrl);
	
	BBM_LONG_READ(NULL,BBM_VT_RT_BER_PERIOD, &vframe);
	BBM_LONG_READ(NULL,BBM_VT_RT_ERROR_SUM, &esum);
	
	vt_ctrl &= ~0x10;
	BBM_WRITE(NULL,BBM_VT_CONTROL, vt_ctrl);
	
	if(vframe == 0) {
		*ber = 0;
		return BBM_NOK;
	}

//	*ber = ((float)esum / (float)vframe) * 10000.0f;
	*ber = (esum * 10000 / vframe);
	
	return res;
}

static int GetSignalLevel(u32 ber, u8 *level)
{
	if(ber >= 900) *level = 0;
	else if((ber >= 800) && (ber < 900)) *level = 1;
	else if((ber >= 700) && (ber < 800)) *level = 2;
	else if((ber >= 600) && (ber < 700)) *level = 3;
	else if((ber >= 500) && (ber < 600)) *level = 4;
	else if((ber >= 400) && (ber < 500)) *level = 5;
	else if(ber < 400) *level = 6;

	return BBM_OK;
}

void DMBDrv_ISR()
{
	BBM_ISR(NULL);
}


unsigned char DMBDrv_init(void)
{
	u8 data;
	u16 wdata;
	u32 ldata;	
	int i;
	u8 temp = 0x1e;


#ifdef CONFIG_TDMB_SPI
	if(BBM_HOSTIF_SELECT(NULL, BBM_SPI))
		return TDMB_FAIL;
#elif defined(CONFIG_TDMB_EBI)
	if(BBM_HOSTIF_SELECT(NULL, BBM_PPI))
		return TDMB_FAIL;
#endif

  if(BBM_PROBE(NULL) != BBM_OK)  // check for factory  chip interface test
  {
    return TDMB_FAIL; 
  }
  
	BBM_FIC_CALLBACK_REGISTER(NULL, TDMBDrv_FIC_CALLBACK);
	BBM_MSC_CALLBACK_REGISTER(NULL, TDMBDrv_MSC_CALLBACK);

	BBM_INIT(NULL);
	BBM_TUNER_SELECT(NULL, FC8050_TUNER, BAND3_TYPE);

#if 0
	BBM_WRITE(NULL, 0x05, 0xa7);
	BBM_READ(NULL, 0x05, &data);
	BBM_READ(NULL, 0x12, &data);
	BBM_WORD_READ(NULL, 0x12, &wdata);
	BBM_WORD_WRITE(NULL, 0x310, 0x0b);
	BBM_WORD_READ(NULL, 0x310, &wdata);
	BBM_WRITE(NULL, 0x312, 0xc0);
	BBM_READ(NULL, 0x312, &data);

	BBM_TUNER_READ(NULL, 0x01, 0x01, &data, 0x01);
#endif

#if 0
	for(i=0;i<1000;i++)
	{
//		dog_kick();
		BBM_WRITE(NULL, 0x05, i & 0xff);
		BBM_READ(NULL, 0x05, &data);
		if((i & 0xff) != data)
			DPRINTK("FC8000 byte test (0x%x,0x%x)\r\n", i & 0xff, data);
	}
	for(i=0;i<1000;i++)
	{
		BBM_WORD_WRITE(NULL, 0x0210, i & 0xffff);
		BBM_WORD_READ(NULL, 0x0210, &wdata);
		if((i & 0xffff) != wdata)
			DPRINTK("FC8000 word test (0x%x,0x%x)\r\n", i & 0xffff, wdata);
	}
	for(i=0;i<1000;i++)
	{
		BBM_LONG_WRITE(NULL, 0x0210, i & 0xffffffff);
		BBM_LONG_READ(NULL, 0x0210, &ldata);
		if((i & 0xffffffff) != ldata)
			DPRINTK("FC8000 long test (0x%x,0x%x)\r\n", i & 0xffffffff, ldata);
	}
	for(i=0;i<1000;i++)
	{
	  temp = i&0xff;
		BBM_TUNER_WRITE(NULL, 0x12, 0x01, &temp, 0x01);
		BBM_TUNER_READ(NULL, 0x12, 0x01, &data, 0x01);
		if((i & 0xff) != data)
			DPRINTK("FC8000 tuner test (0x%x,0x%x)\r\n", i & 0xff, data);
	}
	temp = 0x51;
	BBM_TUNER_WRITE(NULL, 0x12, 0x01, &temp, 0x01 );	
	
#endif
	gBer = 3000;
	gInitFlag = 1;

	return TDMB_SUCCESS;
}

unsigned char DMBDrv_DeInit(void)
{
	gInitFlag = 0;

	BBM_VIDEO_DESELECT(NULL, 0, 0, 0);
	BBM_AUDIO_DESELECT(NULL, 0, 3);
	BBM_DATA_DESELECT(NULL, 0, 2);
	BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 0x00);

	msWait(100);

	BBM_DEINIT(NULL);

	BBM_FIC_CALLBACK_DEREGISTER(NULL);
	BBM_MSC_CALLBACK_DEREGISTER(NULL);

	BBM_HOSTIF_DESELECT(NULL);

	return TDMB_SUCCESS;
}

unsigned char DMBDrv_ScanCh(unsigned long ulFrequency)
{
	esbInfo_t* esb;

	if(!gInitFlag)
		return TDMB_FAIL;

	FIC_DEC_SubChInfoClean();

	BBM_WORD_WRITE(NULL, BBM_BUF_INT, 0x01ff); 

	if(BBM_TUNER_SET_FREQ(NULL, ulFrequency)) {
		BBM_WORD_WRITE(NULL, BBM_BUF_INT, 0x00ff);
		return TDMB_FAIL;
	} 

	if(BBM_SCAN_STATUS(NULL)) {
		BBM_WORD_WRITE(NULL, BBM_BUF_INT, 0x00ff);
		return TDMB_FAIL;
	}

	// wait 1.2 sec for gathering fic information
	msWait(1200);   // 1200
	
	BBM_WORD_WRITE(NULL, BBM_BUF_INT, 0x00ff);

	esb = FIC_DEC_GetEsbInfo(0);
	if(esb->flag != 99) {
		FIC_DEC_SubChInfoClean();
		return TDMB_FAIL;
	}

	if(strlen(esb->label) <= 0) {
		FIC_DEC_SubChInfoClean();
		return TDMB_FAIL;
	}
		
	return TDMB_SUCCESS;
}

int DMBDrv_GetDMBSubChCnt()
{
	svcInfo_t *pSvcInfo;
	int i,n;

	if(!gInitFlag)
		return 0;

	n = 0;
	for(i=0; i <MAX_SVC_NUM; i++) {
		pSvcInfo = FIC_DEC_GetSvcInfoList(i);

		if((pSvcInfo->flag &0x07) == 0x07) {
			if((pSvcInfo->TMId == 0x01) && (pSvcInfo->DSCTy == 0x18))	
				n++;
		}
	}

	return n;
}

int DMBDrv_GetDABSubChCnt()
{
	svcInfo_t *pSvcInfo;
	int i, n;

	if(!gInitFlag)
		return 0;

	n = 0;
	for(i=0; i < MAX_SVC_NUM; i++) {
		pSvcInfo = FIC_DEC_GetSvcInfoList(i);

		if((pSvcInfo->flag &0x07) == 0x07) {
			if((pSvcInfo->TMId == 0x00) && (pSvcInfo->ASCTy == 0x00))	
				n++;
		}
	}

	return n;
}

char* DMBDrv_GetEnsembleLabel()
{
	esbInfo_t* esb;
	
	if(!gInitFlag)
		return NULL;
	
	esb = FIC_DEC_GetEsbInfo(0);

	if(esb->flag == 99)
		return (char*)esb->label;

	return NULL;
}

char* DMBDrv_GetSubChDMBLabel(int nSubChCnt)
{
	int i,n;
	svcInfo_t *pSvcInfo;
	char* label = NULL;

	if(!gInitFlag)
		return NULL;

	n = 0;
	for(i=0; i < MAX_SVC_NUM; i++) {
		pSvcInfo = FIC_DEC_GetSvcInfoList(i);

		if((pSvcInfo->flag & 0x07) == 0x07) {
			if((pSvcInfo->TMId == 0x01) && (pSvcInfo->DSCTy == 0x18)) {
				if(n == nSubChCnt) {
					label = (char*) pSvcInfo->label;
					break;
				}
				n++;
			}
		}
	}

	return label;
}

char* DMBDrv_GetSubChDABLabel(int nSubChCnt)
{
	int i, n;
	svcInfo_t *pSvcInfo;
	char* label = NULL;

	if(!gInitFlag)
		return NULL;

	n = 0;
	for(i=0; i < MAX_SVC_NUM; i++) {
		pSvcInfo = FIC_DEC_GetSvcInfoList(i);

		if((pSvcInfo->flag &0x07) == 0x07) {
			if((pSvcInfo->TMId == 0x00) && (pSvcInfo->ASCTy == 0x00)) {
				if(n == nSubChCnt) {
					label = (char*) pSvcInfo->label;
					break;
				}
				n++;
			}
		}
	}

	return label;
}

SubChInfoTypeDB* DMBDrv_GetFICDMB(int nSubChCnt)
{
	int i, n, j;
	esbInfo_t* esb;
	svcInfo_t *pSvcInfo;
	u8 NumberofUserAppl;

	if(!gInitFlag)
		return NULL;

	memset((void*)&gDMBSubChInfo, 0, sizeof(gDMBSubChInfo));

	n = 0;
	for(i=0; i < MAX_SVC_NUM; i++) {
		pSvcInfo = FIC_DEC_GetSvcInfoList(i);

		if((pSvcInfo->flag &0x07) == 0x07) {
			if((pSvcInfo->TMId == 0x01) && (pSvcInfo->DSCTy == 0x18)) {
				if(n == nSubChCnt) {
					gDMBSubChInfo.ucSubchID         = pSvcInfo->SubChId;
					gDMBSubChInfo.uiStartAddress    = 0;
					gDMBSubChInfo.ucTMId            = pSvcInfo->TMId;
					gDMBSubChInfo.ucServiceType     = pSvcInfo->DSCTy;
					gDMBSubChInfo.ulServiceID       = pSvcInfo->SId;

					NumberofUserAppl = pSvcInfo->NumberofUserAppl;
					gDMBSubChInfo.NumberofUserAppl  = NumberofUserAppl;
					for(j = 0; j < NumberofUserAppl; j++) {
						gDMBSubChInfo.UserApplType[j] = pSvcInfo->UserApplType[j];
						gDMBSubChInfo.UserApplLength[j] = pSvcInfo->UserApplLength[j];
						memcpy(&gDMBSubChInfo.UserApplData[j][0], &pSvcInfo->UserApplData[j][0], gDMBSubChInfo.UserApplLength[j]);
					}

					esb = FIC_DEC_GetEsbInfo(0);
					if(esb->flag == 99) 
						gDMBSubChInfo.uiEnsembleID = esb->EId;
					else                
						gDMBSubChInfo.uiEnsembleID = 0;
					
					break;
				}
				n++;
			}
		}
	}

	return &gDMBSubChInfo;
}

SubChInfoTypeDB* DMBDrv_GetFICDAB(int nSubChCnt)
{
	int i,n;
	esbInfo_t* esb;
	svcInfo_t *pSvcInfo;

	if(!gInitFlag)
		return NULL;
	
	memset((void*)&gDABSubChInfo, 0, sizeof(gDABSubChInfo));

	n = 0;
	for(i=0; i < MAX_SVC_NUM; i++) {
		pSvcInfo = FIC_DEC_GetSvcInfoList(i);

		if((pSvcInfo->flag &0x07) == 0x07) {
			if((pSvcInfo->TMId == 0x00) && (pSvcInfo->ASCTy == 0x00)) {
				if(n == nSubChCnt) {
					gDABSubChInfo.ucSubchID         = pSvcInfo->SubChId;
					gDABSubChInfo.uiStartAddress    = 0;
					gDABSubChInfo.ucTMId            = pSvcInfo->TMId;
					gDABSubChInfo.ucServiceType     = pSvcInfo->ASCTy;
					gDABSubChInfo.ulServiceID       = pSvcInfo->SId;
					esb = FIC_DEC_GetEsbInfo(0);
					if(esb->flag == 99) 
						gDMBSubChInfo.uiEnsembleID = esb->EId;
					else                
						gDMBSubChInfo.uiEnsembleID = 0;

					break;
				}
				n++;
			}
		}
	}

	return &gDABSubChInfo;
}

unsigned char DMBDrv_SetCh(unsigned long ulFrequency, unsigned char ucSubChannel, unsigned char ucSvType)
{
	if(!gInitFlag)
		return TDMB_FAIL;

  bfirst = 1;
  TSBuffpos = 0;
  MSCBuffpos = 0;
  mp2len = 0;

  gCurSvcType = ucSvType;
  gCurSubChId = ucSubChannel;
  
	BBM_VIDEO_DESELECT(NULL, 0, 0, 0);
	BBM_AUDIO_DESELECT(NULL, 0, 3);
	BBM_DATA_DESELECT(NULL, 0, 2);

	BBM_WORD_WRITE(NULL, BBM_BUF_INT, 0x00ff);

	if(BBM_TUNER_SET_FREQ(NULL, ulFrequency) != BBM_OK) {
		return TDMB_FAIL;
	}

	if(ucSvType == 0x18) {
		BBM_VIDEO_SELECT(NULL, ucSubChannel, 0, 0);
	} else if(ucSvType == 0x00) {
		BBM_AUDIO_SELECT(NULL, ucSubChannel, 3);
	} else {
		BBM_DATA_SELECT(NULL, ucSubChannel, 2);
	}

	return TDMB_SUCCESS;
}

unsigned short DMBDrv_GetBER()
{
	return gBer;
}

unsigned char DMBDrv_GetAntLevel(void)
{
	u8 level = 0;
	unsigned int ber;

	if(!gInitFlag) {
		gBer = 3000;
		return 0;
	}
	
	if(viterbi_rt_ber_read(&ber)) {
		gBer = 3000;
		return 0;
	}

	if(ber <= 10)
		ber = 0;
	
	gBer = ber;
	if(GetSignalLevel(ber, &level))
		return 0;

	return level;
}

signed short DMBDrv_GetRSSI()
{
	s32 RSSI;
	
	if(!gInitFlag)
		return -110;
	
	BBM_TUNER_GET_RSSI(NULL, &RSSI);
	
	return (signed short)RSSI;
}

void DMBDrv_ReSynchCheck()
{
}

void DMBDrv_ReSynch()
{
}

