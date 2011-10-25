#ifndef __DMBDRV_WRAP_FC8050_H
#define __DMBDRV_WRAP_FC8050_H

#include <linux/string.h>
#include <linux/delay.h>

#define TDMB_SUCCESS            1
#define TDMB_FAIL               0

#define USER_APPL_NUM_MAX       12
#define USER_APPL_DATA_SIZE_MAX 24

typedef struct _tagCHANNELDB_INFO
{
	unsigned short uiEnsembleID;
	unsigned char	ucSubchID;
	unsigned short uiStartAddress;
	unsigned char ucTMId;
	unsigned char ucServiceType;
	unsigned long ulServiceID;
	unsigned char NumberofUserAppl;
	unsigned short UserApplType[USER_APPL_NUM_MAX];
	unsigned char UserApplLength[USER_APPL_NUM_MAX];
	unsigned char UserApplData[USER_APPL_NUM_MAX][USER_APPL_DATA_SIZE_MAX];
} SubChInfoTypeDB;

void DMBDrv_ISR(void);
unsigned char DMBDrv_init(void);
unsigned char DMBDrv_DeInit(void);
unsigned char DMBDrv_ScanCh(unsigned long ulFrequency);
int DMBDrv_GetDMBSubChCnt(void);
int DMBDrv_GetDABSubChCnt(void);
char* DMBDrv_GetEnsembleLabel(void);
char* DMBDrv_GetSubChDMBLabel(int nSubChCnt);
char* DMBDrv_GetSubChDABLabel(int nSubChCnt);
SubChInfoTypeDB* DMBDrv_GetFICDMB(int nSubChCnt);
SubChInfoTypeDB* DMBDrv_GetFICDAB(int nSubChCnt);
unsigned char DMBDrv_SetCh(unsigned long ulFrequency, unsigned char ucSubChannel, unsigned char ucSvType);
unsigned short DMBDrv_GetBER(void);
unsigned char DMBDrv_GetAntLevel(void);
signed short DMBDrv_GetRSSI(void);
void DMBDrv_ReSynchCheck(void);
void DMBDrv_ReSynch(void);
int DMBDrv_GetPowerOnStatus(void);
#endif
