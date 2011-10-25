/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fci_oal.c
 
 Description : OS Adaptation Layer
 
 History : 
 ----------------------------------------------------------------------
 2009/09/13 	jason		initial
*******************************************************************************/
#include <linux/delay.h>

#include "fci_types.h"
//#include <stdio.h>
//#include <stdarg.h>


void PRINTF(HANDLE hDevice, char *fmt,...)
{

#if 0// setup_pascal
	va_list ap;
	char str[256];

	va_start(ap,fmt);
	vsprintf(str,fmt,ap);
	va_end(ap);
#endif	
}

void msWait(int ms)
{
  msleep(ms);
}

