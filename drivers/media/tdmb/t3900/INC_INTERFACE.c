#include "INC_INCLUDES.h"

#include "tdmb.h"
/*********************************************************************************/
/* Operating Chip set : T3900                                                    */
/* Software version   : version 1.16                                             */
/* Software Update    : 2011.03.17                                               */
/*********************************************************************************/
#define INC_INTERRUPT_LOCK()		
#define INC_INTERRUPT_FREE()

extern struct spi_device *spi_dmb;

ST_SUBCH_INFO		g_stDmbInfo;
ST_SUBCH_INFO		g_stDabInfo;
ST_SUBCH_INFO		g_stDataInfo;
ST_SUBCH_INFO		g_stFIDCInfo;

ENSEMBLE_BAND 		m_ucRfBand 		= KOREA_BAND_ENABLE;

INC_CHANNEL_INFO    g_currChInfo; /* tdmb */
extern int bfirst;
extern int TSBuffpos;
extern int MSCBuffpos;
extern int mp2len;

/*********************************************************************************/
/*  RF Band Select                                                               */
/*                                                                               */
/*  INC_UINT8 m_ucRfBand = KOREA_BAND_ENABLE,                                    */
/*               BANDIII_ENABLE,                                                 */
/*               LBAND_ENABLE,                                                   */
/*               CHINA_ENABLE,                                                   */
/*               EXTERNAL_ENABLE,                                                */
/*********************************************************************************/

CTRL_MODE 			m_ucCommandMode 		= INC_SPI_CTRL;
ST_TRANSMISSION		m_ucTransMode			= TRANSMISSION_MODE1;
UPLOAD_MODE_INFO	m_ucUploadMode 			= STREAM_UPLOAD_SPI;
CLOCK_SPEED			m_ucClockSpeed 			= INC_OUTPUT_CLOCK_4096;
INC_ACTIVE_MODE		m_ucMPI_CS_Active 		= INC_ACTIVE_LOW;
INC_ACTIVE_MODE		m_ucMPI_CLK_Active 		= INC_ACTIVE_HIGH;

INC_UINT16			m_unIntCtrl				= (INC_INTERRUPT_POLARITY_HIGH | \
											   INC_INTERRUPT_PULSE | \
											   INC_INTERRUPT_AUTOCLEAR_ENABLE | \
											   (INC_INTERRUPT_PULSE_COUNT & INC_INTERRUPT_PULSE_COUNT_MASK));


/*********************************************************************************/
/* PLL_MODE			m_ucPLL_Mode                                                 */
/*T3900  Input Clock Setting                                                     */
/*********************************************************************************/
PLL_MODE			m_ucPLL_Mode		= INPUT_CLOCK_19200KHZ; /* INPUT_CLOCK_24576KHZ; */


/*********************************************************************************/
/* INC_DPD_MODE		m_ucDPD_Mode                                                 */
/* T3900  Power Saving mode setting                                              */
/*********************************************************************************/
INC_DPD_MODE		m_ucDPD_Mode		= INC_DPD_OFF;

/*********************************************************************************/
/*  MPI Chip Select and Clock Setup Part                                         */
/*                                                                               */
/*  INC_UINT8 m_ucCommandMode = INC_I2C_CTRL, INC_SPI_CTRL, INC_EBI_CTRL         */
/*                                                                               */
/*  INC_UINT8 m_ucUploadMode = STREAM_UPLOAD_MASTER_SERIAL,                      */
/*                 STREAM_UPLOAD_SLAVE_PARALLEL,                                 */
/*                 STREAM_UPLOAD_TS,                                             */
/*                 STREAM_UPLOAD_SPI,                                            */
/*                                                                               */
/*  INC_UINT8 m_ucClockSpeed = INC_OUTPUT_CLOCK_4096,                            */
/*                 INC_OUTPUT_CLOCK_2048,                                        */
/*                 INC_OUTPUT_CLOCK_1024,                                        */
/*********************************************************************************/

void INC_DELAY(INC_UINT16 uiDelay)
{
    msleep(uiDelay);
}

void INC_MSG_PRINTF(INC_INT8 nFlag, INC_INT8* pFormat, ...)
{
	va_list Ap;
	INC_UINT16 nSize;
	INC_INT8 acTmpBuff[1024] = {0};

	va_start(Ap, pFormat);
	nSize = vsprintf(acTmpBuff, pFormat, Ap);
	va_end(Ap);

	/* TODO Serial code here... */
}

INC_UINT16 INC_I2C_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
	INC_UINT16 uiRcvData = 0;
	/* TODO I2C read code here... */
	
	return uiRcvData;
}

INC_UINT8 INC_I2C_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData)
{
	/* TODO I2C write code here... */
	return INC_SUCCESS;
}

INC_UINT8 INC_I2C_READ_BURST(INC_UINT8 ucI2CID,  INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize)
{
	/* TODO I2C burst read code here... */
	return INC_SUCCESS;
}

INC_UINT8 INC_EBI_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData)
{
	INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGWRITE_CMD) | 1;
	INC_UINT16 uiNewAddr = (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;

	INC_INTERRUPT_LOCK();

	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;

	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = (uiData >> 8) & 0xff;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS =  uiData & 0xff;

	INC_INTERRUPT_FREE();
	return INC_SUCCESS;
}

INC_UINT16 INC_EBI_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
	INC_UINT16 uiRcvData = 0;
	INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGREAD_CMD) | 1;
	INC_UINT16 uiNewAddr = (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;

	INC_INTERRUPT_LOCK();
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
	*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;

	uiRcvData  = (*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS  & 0xff) << 8;
	uiRcvData |= (*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS & 0xff);
	
	INC_INTERRUPT_FREE();
	return uiRcvData;
}

INC_UINT8 INC_EBI_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize)
{
	INC_UINT16 uiLoop, nIndex = 0, anLength[2], uiCMD, unDataCnt;
	INC_UINT16 uiNewAddr = (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;

	if(nSize > INC_MPI_MAX_BUFF) return INC_ERROR;
	memset((INC_INT8*)anLength, 0, sizeof(anLength));

	if(nSize > INC_TDMB_LENGTH_MASK) {
		anLength[nIndex++] = INC_TDMB_LENGTH_MASK;
		anLength[nIndex++] = nSize - INC_TDMB_LENGTH_MASK;
	}
	else anLength[nIndex++] = nSize;

	INC_INTERRUPT_LOCK();
	for(uiLoop = 0; uiLoop < nIndex; uiLoop++){

		uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (anLength[uiLoop] & INC_TDMB_LENGTH_MASK);

		*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
		*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
		*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
		*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;

		for(unDataCnt = 0 ; unDataCnt < anLength[uiLoop]; unDataCnt++){
			*pData++ = *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS & 0xff;
		}
	}
	INC_INTERRUPT_FREE();

	return INC_SUCCESS;
}

INC_UINT16 INC_SPI_REG_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
	INC_UINT16 uiRcvData = 0;
	INC_UINT16 uiNewAddr = (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGREAD_CMD) | 1;
	INC_UINT8 auiBuff[6];
	INC_UINT8 cCnt = 0;
    INC_UINT8 acRxBuff[2];
	
	auiBuff[cCnt++] = uiNewAddr >> 8;
	auiBuff[cCnt++] = uiNewAddr & 0xff;
	auiBuff[cCnt++] = uiCMD >> 8;
	auiBuff[cCnt++] = uiCMD & 0xff;
	INC_INTERRUPT_LOCK();
	/* TODO SPI Write code here... */
	
    struct spi_message              msg;
	struct spi_transfer             transfer[2];
	unsigned char                   status;
	
	memset( &msg, 0, sizeof( msg ) );
	memset( transfer, 0, sizeof( transfer ) );
	spi_message_init( &msg );

	msg.spi=spi_dmb;

	transfer[0].tx_buf = (u8 *) auiBuff;
	transfer[0].rx_buf = (u8 *) NULL;
	transfer[0].len = 4;
	transfer[0].bits_per_word = 8;
	transfer[0].delay_usecs = 0;
	spi_message_add_tail( &(transfer[0]), &msg );

	transfer[1].tx_buf = (u8 *) NULL;
	transfer[1].rx_buf = (u8 *) acRxBuff;
	transfer[1].len = 2;
	transfer[1].bits_per_word = 8;
	transfer[1].delay_usecs = 0;
	spi_message_add_tail( &(transfer[1]), &msg );

	status = spi_sync(spi_dmb, &msg);
	uiRcvData = (INC_UINT16)(acRxBuff[0] << 8)|(INC_UINT16)acRxBuff[1];


	/* TODO SPI Read code here... */
	INC_INTERRUPT_FREE();
	return uiRcvData;
}

INC_UINT8 INC_SPI_REG_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData)
{
	INC_UINT16 uiNewAddr = (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGWRITE_CMD) | 1;
	INC_UINT8 auiBuff[6];
	INC_UINT8 cCnt = 0;

	auiBuff[cCnt++] = uiNewAddr >> 8;
	auiBuff[cCnt++] = uiNewAddr & 0xff;
	auiBuff[cCnt++] = uiCMD >> 8;
	auiBuff[cCnt++] = uiCMD & 0xff;
	auiBuff[cCnt++] = uiData >> 8;
	auiBuff[cCnt++] = uiData & 0xff;
	INC_INTERRUPT_LOCK();
	/* TODO SPI SDO Send code here... */
  struct spi_message              msg;
	struct spi_transfer             transfer;
	unsigned char                   status;
	
	memset( &msg, 0, sizeof( msg ) );
	memset( &transfer, 0, sizeof( transfer ) );
	spi_message_init( &msg );

	msg.spi=spi_dmb;

	transfer.tx_buf = (u8 *) auiBuff;
	transfer.rx_buf = NULL;
	transfer.len = 6;
	transfer.bits_per_word = 8;
	transfer.delay_usecs = 0;
	spi_message_add_tail( &transfer, &msg );

	status = spi_sync(spi_dmb, &msg);

	/* TODO SPI SDO Send code here... */ 
	INC_INTERRUPT_FREE();
	return INC_SUCCESS;
}

INC_UINT8 INC_SPI_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pBuff, INC_UINT16 wSize)
{
	INC_UINT16 uiNewAddr = (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;

	INC_UINT16 uiLoop, nIndex = 0, anLength[2], uiCMD, unDataCnt;
	INC_UINT8 auiBuff[6];

	struct spi_message              msg;
	struct spi_transfer             transfer[2];
	unsigned char                   status;
	
	auiBuff[0] = uiNewAddr >> 8;
	auiBuff[1] = uiNewAddr & 0xff;
	uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (wSize & 0xFFF);
    auiBuff[2] = uiCMD >> 8;
    auiBuff[3] = uiCMD & 0xff;
	
   	memset( &msg, 0, sizeof( msg ) );
	memset( transfer, 0, sizeof( transfer ) );
	spi_message_init( &msg );

	msg.spi=spi_dmb;

	transfer[0].tx_buf = (u8 *) auiBuff;
	transfer[0].rx_buf = (u8 *)NULL;
	transfer[0].len = 4;
	transfer[0].bits_per_word = 8;
	transfer[0].delay_usecs = 0;
	spi_message_add_tail( &(transfer[0]), &msg );
	
	transfer[1].tx_buf = (u8 *) NULL;
	transfer[1].rx_buf = (u8 *)pBuff;
	transfer[1].len = wSize;
	transfer[1].bits_per_word = 8;
	transfer[1].delay_usecs = 0;
	spi_message_add_tail( &(transfer[1]), &msg );
	status = spi_sync(spi_dmb, &msg);

/*
	if(wSize > INC_MPI_MAX_BUFF) return INC_ERROR;
	memset((INC_INT8*)anLength, 0, sizeof(anLength));

	if(wSize > INC_TDMB_LENGTH_MASK) {
		anLength[nIndex++] = INC_TDMB_LENGTH_MASK;
		anLength[nIndex++] = wSize - INC_TDMB_LENGTH_MASK;
	}
	else anLength[nIndex++] = wSize;

	INC_INTERRUPT_LOCK();
	for(uiLoop = 0; uiLoop < nIndex; uiLoop++){

		auiBuff[0] = uiNewAddr >> 8;
		auiBuff[1] = uiNewAddr & 0xff;
		uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (anLength[uiLoop] & INC_TDMB_LENGTH_MASK);
		auiBuff[2] = uiCMD >> 8;
		auiBuff[3] = uiCMD & 0xff;
		
		//TODO SPI[SDO] command Write code here..
		for(unDataCnt = 0 ; unDataCnt < anLength[uiLoop]; unDataCnt++){
			//TODO SPI[SDI] data Receive code here.. 
		}
	}
	INC_INTERRUPT_FREE();
*/	
	return INC_SUCCESS;
}

INC_UINT8 INTERFACE_DBINIT(void)
{
	memset(&g_stDmbInfo,	0, sizeof(ST_SUBCH_INFO));
	memset(&g_stDabInfo,	0, sizeof(ST_SUBCH_INFO));
	memset(&g_stDataInfo,	0, sizeof(ST_SUBCH_INFO));
	memset(&g_stFIDCInfo,	0, sizeof(ST_SUBCH_INFO));
	return INC_SUCCESS;
}

void INTERFACE_UPLOAD_MODE(INC_UINT8 ucI2CID, UPLOAD_MODE_INFO ucUploadMode)
{
	m_ucUploadMode = ucUploadMode;
	INC_UPLOAD_MODE(ucI2CID);
}

INC_UINT8 INTERFACE_PLL_MODE(INC_UINT8 ucI2CID, PLL_MODE ucPllMode)
{
	m_ucPLL_Mode = ucPllMode;
	return INC_PLL_SET(ucI2CID);
}

/*  초기 전원 입력시 호출    */
INC_UINT8 INTERFACE_INIT(INC_UINT8 ucI2CID)
{
	return INC_INIT(ucI2CID);
}

/* 에러 발생시 에러코드 읽기  */
INC_ERROR_INFO INTERFACE_ERROR_STATUS(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	return pInfo->nBbpStatus;
}

/*********************************************************************************/
/* 단일 채널 선택하여 시작하기....                                               */  
/* pChInfo->ucServiceType, pChInfo->ucSubChID, pChInfo->ulRFFreq                 */
/* pChInfo->nSetCnt 은 선택채널 개수는 반드시 넘겨주어야 한다.                   */
/* DMB채널 선택시 pChInfo->ucServiceType = 0x18                                  */
/* DAB, DATA채널 선택시 pChInfo->ucServiceType = 0으로 설정을 해야함.            */
/*********************************************************************************/
INC_UINT8 INTERFACE_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo)
{
    g_currChInfo = pChInfo->astSubChInfo[0]; /* for dab  */
    bfirst = 1;
    TSBuffpos = 0;
    MSCBuffpos = 0;
    mp2len = 0;    
	return INC_CHANNEL_START(ucI2CID, pChInfo);
}

/* For 공정	*/
INC_UINT8 INTERFACE_START_TEST(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo)
{
    g_currChInfo = pChInfo->astSubChInfo[0]; /* for dab	*/
    bfirst = 1;
    TSBuffpos = 0;
    MSCBuffpos = 0;
    mp2len = 0;    

    return INC_CHANNEL_START_TEST(ucI2CID, pChInfo);
}

/*********************************************************************************/
/* 다중 채널 선택하여 시작하기....                                               */  
/* pChInfo->ucServiceType, pChInfo->ucSubChID, pChInfo->ulRFFreq 는              */
/* 반드시 넘겨주어야 한다.                                                       */
/* DMB채널 선택시 pChInfo->ucServiceType = 0x18                                  */
/* DAB, DATA채널 선택시 pChInfo->ucServiceType = 0으로 설정을 해야함.            */
/* pMultiInfo->nSetCnt은 선택한 서브채널 개수임.                                 */
/* FIC Data를 같이  선택시 INC_MULTI_CHANNEL_FIC_UPLOAD 매크로를 풀어야 한다.    */
/* DMB  채널은 최대 2개채널 선택이 가능하다.                                     */
/* DAB  채널은 최대 3개채널 선택이 가능하다.                                     */
/* DATA 채널은 최대 3개채널 선택이 가능하다.                                     */
/*********************************************************************************/

/*********************************************************************************/
/* 스캔시  호출한다.                                                             */
/* 주파수 값은 받드시넘겨주어야 한다.                                            */
/* Band를 변경하여 스캔시는 m_ucRfBand를 변경하여야 한다.                        */
/* 주파수 값은 받드시넘겨주어야 한다.                                            */
/*********************************************************************************/
INC_UINT8 INTERFACE_SCAN(INC_UINT8 ucI2CID, INC_UINT32 ulFreq)
{
	INTERFACE_DBINIT();

	if(!INC_ENSEMBLE_SCAN(ucI2CID, ulFreq)) return INC_ERROR;

	INC_DB_UPDATE(ulFreq, &g_stDmbInfo, &g_stDabInfo, &g_stDataInfo, &g_stFIDCInfo);

	INC_BUBBLE_SORT(&g_stDmbInfo,  INC_SUB_CHANNEL_ID);
	INC_BUBBLE_SORT(&g_stDabInfo,  INC_SUB_CHANNEL_ID);
	INC_BUBBLE_SORT(&g_stDataInfo, INC_SUB_CHANNEL_ID);
	INC_BUBBLE_SORT(&g_stFIDCInfo, INC_SUB_CHANNEL_ID);

	return INC_SUCCESS;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 DMB채널 개수를 리턴한다.                             */
/*********************************************************************************/
INC_UINT16 INTERFACE_GETDMB_CNT(void)
{
	return (INC_UINT16)g_stDmbInfo.nSetCnt;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 DAB채널 개수를 리턴한다.                             */
/*********************************************************************************/
INC_UINT16 INTERFACE_GETDAB_CNT(void)
{
	return (INC_UINT16)g_stDabInfo.nSetCnt;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 DATA채널 개수를 리턴한다.                            */
/*********************************************************************************/
INC_UINT16 INTERFACE_GETDATA_CNT(void)
{
	return (INC_UINT16)g_stDataInfo.nSetCnt;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 Ensemble label을 리턴한다.                           */
/*********************************************************************************/
INC_UINT8* INTERFACE_GETENSEMBLE_LABEL(INC_UINT8 ucI2CID)
{
	ST_FICDB_LIST*	pList;
	pList = INC_GET_FICDB_LIST();
	return pList->aucEnsembleName;
}

/*********************************************************************************/
/* DMB 채널 정보를 리턴한다.                                                     */
/*********************************************************************************/
INC_CHANNEL_INFO* INTERFACE_GETDB_DMB(INC_INT16 uiPos)
{
	if(uiPos >= MAX_SUBCH_SIZE) return INC_NULL;
	if(uiPos >= g_stDmbInfo.nSetCnt) return INC_NULL;
	return &g_stDmbInfo.astSubChInfo[uiPos];
}

/*********************************************************************************/
/* DAB 채널 정보를 리턴한다.                                                     */
/*********************************************************************************/
INC_CHANNEL_INFO* INTERFACE_GETDB_DAB(INC_INT16 uiPos)
{
	if(uiPos >= MAX_SUBCH_SIZE) return INC_NULL;
	if(uiPos >= g_stDabInfo.nSetCnt) return INC_NULL;
	return &g_stDabInfo.astSubChInfo[uiPos];
}

/*********************************************************************************/
/* DATA 채널 정보를 리턴한다.                                                    */
/*********************************************************************************/
INC_CHANNEL_INFO* INTERFACE_GETDB_DATA(INC_INT16 uiPos)
{
	if(uiPos >= MAX_SUBCH_SIZE) return INC_NULL;
	if(uiPos >= g_stDataInfo.nSetCnt) return INC_NULL;
	return &g_stDataInfo.astSubChInfo[uiPos];
}

/*	시청 중 FIC 정보 변경되었는지를 체크	*/
INC_UINT8 INTERFACE_RECONFIG(INC_UINT8 ucI2CID)
{
	return INC_FIC_RECONFIGURATION_HW_CHECK(ucI2CID);
}

INC_UINT8 INTERFACE_STATUS_CHECK(INC_UINT8 ucI2CID)
{
	return INC_STATUS_CHECK(ucI2CID);
}

INC_UINT16 INTERFACE_GET_CER(INC_UINT8 ucI2CID)
{
	return INC_GET_CER(ucI2CID);
}

INC_UINT8 INTERFACE_GET_SNR(INC_UINT8 ucI2CID)
{
	return INC_GET_SNR(ucI2CID);
}

INC_UINT32 INTERFACE_GET_POSTBER(INC_UINT8 ucI2CID)
{
	return INC_GET_POSTBER(ucI2CID);
}

INC_UINT32 INTERFACE_GET_PREBER(INC_UINT8 ucI2CID)
{
	return INC_GET_PREBER(ucI2CID);
}

/*********************************************************************************/
/* Scan, 채널 시작시에 강제로 중지시 호출한다.                                      */
/*********************************************************************************/
void INTERFACE_USER_STOP(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	pInfo->ucStop = 1;
}

void INTERFACE_USER_STOP_CLEAR(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	pInfo->ucStop = 0;
}

/*	인터럽트 인에이블...	*/
void INTERFACE_INT_ENABLE(INC_UINT8 ucI2CID, INC_UINT16 unSet)
{
	INC_INTERRUPT_SET(ucI2CID, unSet);
}

/*	Use when polling mode	*/
INC_UINT8 INTERFACE_INT_CHECK(INC_UINT8 ucI2CID)
{
	INC_UINT16 nValue = 0;

	nValue = INC_CMD_READ(ucI2CID, APB_INT_BASE+ 0x01);
	if(!(nValue & INC_MPI_INTERRUPT_ENABLE))
		return INC_ERROR;

	return INC_SUCCESS;
}

/* 인터럽스 클리어	*/
void INTERFACE_INT_CLEAR(INC_UINT8 ucI2CID, INC_UINT16 unClr)
{
	INC_INTERRUPT_CLEAR(ucI2CID, unClr);
}

/* 인터럽트 서비스 루틴... // SPI Slave Mode or MPI Slave Mode	*/
INC_UINT8 INTERFACE_ISR(INC_UINT8 ucI2CID, INC_UINT8* pBuff)
{
	INC_UINT16 unDataLength;
    unDataLength = INC_CMD_READ(ucI2CID, APB_MPI_BASE+0x6);
	if(unDataLength < INC_INTERRUPT_SIZE)
	{
		return INC_ERROR;
	}

	INC_CMD_READ_BURST(ucI2CID, APB_STREAM_BASE, pBuff, INC_INTERRUPT_SIZE);

	if((m_unIntCtrl & INC_INTERRUPT_LEVEL) && (!(m_unIntCtrl & INC_INTERRUPT_AUTOCLEAR_ENABLE)))
		INTERFACE_INT_CLEAR(ucI2CID, INC_MPI_INTERRUPT_ENABLE);

	return INC_SUCCESS;
}


