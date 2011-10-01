/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : ficdecoder.h
 
 Description : fic parser
 
 History : 
 ----------------------------------------------------------------------
*******************************************************************************/
#ifndef __ficdecodera_h__
#define __ficdecodera_h__

#include "fci_types.h"

#define MAX_ESB_NUM	1
#define MAX_SVC_NUM	128
#define MAX_SC_NUM	128
#define MAX_SUBCH_NUM	64
#define MAX_DIDP_NUM 	8

#define MAX_USER_APPL_NUM       15
#define MAX_USER_APPL_DATA_SIZE 24

typedef struct {
	u8 	head;
	u8	data[29];
} Fig;

typedef struct {
	u8	data[30];
	u16	crc;
} Fib;

typedef struct {
	//Fib 	fib[12];
	Fib 	fib[32];
} Fic;

typedef struct {
	u8	head;
	u8	data[28];
} FigData;

typedef struct {
	u8	flag;
	u16	EId;
	u8	label[32];
} esbInfo_t;

typedef struct {
	u8	flag;
	u32	SId;
	u16	SCId;
	u8	ASCTy;
	u8	DSCTy;
	u8	FIDCId;
	u8	addrType;		// PD
	u8	TMId;
	u8	SubChId;
	u8	nscps;
	u8	label[32];

#if 0 // Because of Visual Radio	
	u16	UAtype;
#else
    u8  NumberofUserAppl;
    u16 UserApplType[MAX_USER_APPL_NUM];
    u8  UserApplLength[MAX_USER_APPL_NUM];
    u8  UserApplData[MAX_USER_APPL_NUM][MAX_USER_APPL_DATA_SIZE];
#endif
} svcInfo_t;

typedef struct {
	u8	flag;
	u16	SCId;
	u8	DSCTy;
	u8	SubChId;
	u8	SCCAFlag;
	u8	DGFlag;
	u16	PacketAddress;
	u16	SCCA;
	u8	label[32];
} scInfo_t;

typedef struct {
	u8	flag;
	u8 	subChId;			//
	u16	startAddress;			//
	u8	formType;			//
	u8 	tableIndex;			//
	u8	tableSwitch;		
	u8 	option;				//
	u8 	protectLevel;			//
	u16	subChSize;			//
	u32	SId;
	u8	svcChId;			//
	u8	reCfg;
#if 1	/* T-MMB */
	u8	mode;			// 0 T-DMB, 1 T-MMB
	u8	modType;
	u8	encType;
	u8	intvDepth;
	u8	pl;
#endif	/* T-MMB */

#if 1 /* FEC */
	u8  fecScheme;
#endif

} subChInfo_t;

typedef struct {
	u8	flag;
	u8	reCfgOffset;
	u8	subChId;
	u16	startAddress;
	u8	formType;
	u16	subChSize;
	u16	speed;			// kbsp
	u16	l1;
	u8	p1;
	u16	l2;
	u8	p2;
	u16	l3;
	u8	p3;
	u16	l4;
	u8	p4;
	u8	pad;
#if 1	/* T-MMB */
	u8	mode;			// 0 T-DMB, 1 T-MMB
	u8	modType;
	u8	encType;
	u8	intvDepth;
	u8	pl;
	u16	mi;			// kies use
#endif 	/* T-MMB */
} didpInfo_t;

#ifdef __cplusplus
	extern "C" {
#endif

extern esbInfo_t   	gEsbInfo[MAX_ESB_NUM];
extern svcInfo_t 	gSvcInfo[MAX_SVC_NUM];
extern subChInfo_t 	subChInfo[MAX_SUBCH_NUM];
extern didpInfo_t 	didpInfo[MAX_DIDP_NUM];


extern int          fic_decoder(Fic *pFic, u16 length);
extern int          fib_decoder(Fib *pFib);
extern esbInfo_t*   GetEsbInfo(void);
extern subChInfo_t* GetSubChInfo(u8 subChId);
extern svcInfo_t*   GetSvcInfo(u32 SId);
extern scInfo_t*    GetScInfo(u16 SCId);
extern svcInfo_t*   GetSvcInfoList(u8 SvcIdx);
extern void         SubChannelOrganizationClean(void);
extern void         SubChannelOrganizationPrn(int subChId);
extern int          FoundAllLabels(void);
extern void         DidpPrn(didpInfo_t *pDidp);
extern int          SetDidpReg(int svcChId, didpInfo_t *pDidp);
extern int          SubChOrgan2DidpReg(subChInfo_t *pSubChInfo, didpInfo_t *pDidp);

#ifdef __cplusplus
	} // extern "C" {
#endif

#endif /* __ficdecoder_h__ */
