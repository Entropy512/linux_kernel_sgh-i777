#ifndef USBDESCS_H
#define USBDESCS_H

#include "../include/linux/westbridge/cyastoria.h"
#include "../include/linux/westbridge/cyasmtp.h"

#define NUM_OF_STRINGS 6
char *UsbStrings[NUM_OF_STRINGS] =
{
    "Cypress Semiconductor",            /* Manufacturer (device) - index 1 */
#ifdef DEBUG_MSC
    "Astoria MSC Device Linux",		/* 2 product name string */
#else
    "Astoria TMTP Device Linux",		/* 2 product name string */
#endif
    "2222222222",				       	/* 3 serial number string */
};

CyCh9DeviceQualifierDesc device_qualifier =
{
    sizeof(CyCh9DeviceQualifierDesc),
    CY_CH9_GD_DEVICE_QUALIFIER,		/* DEVICE QUALIFIER TYPE */
    0x200,				/* USB VERSION */
    0x00,				/* DEVICE CLASS */
    0x00,				/* DEVICE SUBCLASS */
    0x00,				/* PROTOCOL */
    64,					/* EP0 PACKET SIZE */
    1,					/* NUMBER OF CONFIGURATIONS */
    0
} ;

/**
 * This is the basic device descriptor for this West Bridge test bench.  This descriptor is returned
 * by this software for P port based enumeration.
 **/
CyCh9DeviceDesc pport_device_desc =
{
    sizeof(CyCh9DeviceDesc),
    CY_CH9_GD_DEVICE,   /* DESCRIPTOR TYPE */
    0x200,			/* USB VERSION */
    0xff,			/* DEVICE CLASS: vendor specific */
    0x01,			/* DEVICE SUBCLASS , was 00, FPGA platform it's FF - custom */
    0x00,			/* PROTOCOL */
    64,				/* EP0 packet size */
    0x04b4,			/* CYPRESS VENDOR ID */
    0x4718,			/* West Bridge (MADEUP) for composite device */
    0x2,			/* DEVICE VERSION */
    0x01,			/* STRING ID FOR MANUF. */
    0x02,			/* STRING ID FOR PRODUCT */
    0x03,			/* STRING ID FOR SERIAL NUMBER */
    0x01			/* NUMBER OF CONFIGURATIONS */
} ;

/**
 * this one is returned after the reset , before speed change to HS **
 **/
CyCh9ConfigurationDesc ZeroDesc =
{
    sizeof(CyCh9ConfigurationDesc),		/* Descriptor Length */
    CY_CH9_GD_CONFIGURATION,			/* Descriptor Type */
    sizeof(CyCh9ConfigurationDesc),
    0,						/* Number of Interfaces */
    1,						/* Configuration Number */
    0,						/* Configuration String */
    CY_CH9_CONFIG_ATTR_RESERVED_BIT7,		/* Configuration Attributes */
    50						/* Power Requirements */
} ;


#pragma pack(push,1)
typedef struct MyConfigDesc
{
	CyCh9ConfigurationDesc m_config ;
	CyCh9InterfaceDesc     m_interface ;
	CyCh9EndpointDesc      m_endpoints[3] ;

} MyConfigDesc ;
#pragma pack(pop)


/**
 * DEVICE CONFIGURATION DESCRIPTOR FOR FULL SPEED
 **/
CyCh9ConfigurationDesc ConfigFSDesc =
{
    sizeof(CyCh9ConfigurationDesc),		/* Descriptor Length */
    CY_CH9_GD_CONFIGURATION,			/* Descriptor Type */
    sizeof(CyCh9ConfigurationDesc),
    0,                                   /* Number of Interfaces */
    1,									/* Configuration Number */
    0,									/* Configuration String */
    CY_CH9_CONFIG_ATTR_RESERVED_BIT7,	/* Configuration attributes */
    50									/* Power requirements */
} ;


/** ************** TMTP device configuration at HS **************************/
#pragma pack(push,1)
static MyConfigDesc ConfigHSDesc =
{

	/*Configuration Descriptor*/
	{
		sizeof(CyCh9ConfigurationDesc),	/* Desc Length */
		CY_CH9_GD_CONFIGURATION,	/* Desc Type */
		/*nxz-debug-z sizeof(CyCh9ConfigurationDesc)+ sizeof(CyCh9InterfaceDesc) + 2 * sizeof(CyCh9EndpointDesc),*/
		sizeof(CyCh9ConfigurationDesc)+ sizeof(CyCh9InterfaceDesc) + 3 * sizeof(CyCh9EndpointDesc) - 30/* substract Astoria added INterface */,
		0,				/* Num of Interfaces - 1 (since aStoria will add 1to this field!!!) */
		1,				/* Configuration Value */
		0,			    /*STRING index  for this cfg*/
		CY_CH9_CONFIG_ATTR_RESERVED_BIT7, /* Configuration Attributes */
		50              /* n* 2ma = 100 ma*/
	}
	,
	/* Interface Descriptor */
	{
		sizeof(CyCh9InterfaceDesc),	/* Desc Length */
		CY_CH9_GD_INTERFACE,		/* Desc Type */
		0,				    /* Interface Num */
		0, 				    /* Alternate Interface */
		3,                  /* Number of Endpoints */
		0xff,				/* IF class: ff - vendor specific */
		0x01,				/* IF sub-class */
		0x00,				/* IF protocol */
		6				    /* IF string */
	}
	,
	{
		/* EP2_OUT */
		{
			sizeof(CyCh9EndpointDesc),	/* Desc Length */
			CY_CH9_GD_ENDPOINT,		/* Desc Type */
			CY_CH9_MK_EP_ADDR(CY_CH9_ENDPOINT_DIR_OUT, 2),
			CY_CH9_ENDPOINT_ATTR_BULK,
			512,
			0
		},
		/* EP6_IN */
		{
			sizeof(CyCh9EndpointDesc),	/* Desc Length */
			CY_CH9_GD_ENDPOINT,		/* Desc Type */
			CY_CH9_MK_EP_ADDR(CY_CH9_ENDPOINT_DIR_IN, 6),
			CY_CH9_ENDPOINT_ATTR_BULK,
			512,
			0
		},
		/* AS!!! EP1_INT */
		{
			sizeof(CyCh9EndpointDesc),	/* Desc Length */
			CY_CH9_GD_ENDPOINT,		/* Desc Type */
			CY_CH9_MK_EP_ADDR(CY_CH9_ENDPOINT_DIR_IN, 1),
			CY_CH9_ENDPOINT_ATTR_INTERRUPT,
			64,
			0x01
		}
	}
};
#pragma pack(pop)
/********************************************************** */




#endif


