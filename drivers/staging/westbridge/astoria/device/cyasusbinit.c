#define DEBUG_ZERO
#define DEBUG_MSC

#include "../include/linux/westbridge/cyastoria.h"
#include "../include/linux/westbridge/cyasch9.h"
#include "cyasusbinit.h"
#include "cyasusbdescs.h"

//--------------------------------------

TmtpAstDev g_AstDevice;
TmtpAstDev * g_pAstDevice = &g_AstDevice ;

static uint16_t replybuf[512] ;
static uint8_t *replyptr = (uint8_t *) replybuf ;
//static uint8_t	g_BackData;

static uint8_t *GetReplyArea(void)
{
    /*assert(replyptr != 0) ;*/
    replyptr = 0 ;
    return (uint8_t *) replybuf ;
}

static void RestoreReplyArea(void)
{
    replyptr = (uint8_t *) replybuf ;
}

/* Globals */
/*static uint8_t pktbuffer3[512] ;*/

#ifdef CY_AS_USB_TB_FOUR
static uint8_t pktbuffer7[512] ;
#endif

#ifdef CY_AS_USB_TB_SIX
static uint8_t pktbuffer11[512] ;
#endif

/*static uint8_t turbopktbuffer[512] ;*/

static cy_bool gUsbTestDone = cy_false ;
static volatile cy_bool gSetConfig = cy_false ;
static volatile cy_bool gAsyncStallDone = cy_false ;

static volatile cy_bool gStorageReleaseBus0 = cy_false ;
static volatile cy_bool gStorageReleaseBus1 = cy_false ;

static uint8_t MyConfiguration = 0 ;
static CyCh9ConfigurationDesc *desc_p = 0 ;
static CyCh9ConfigurationDesc *other_p = 0 ;
static cy_bool gSetupPending = cy_false ;

static volatile uint8_t gAsyncStallStale = 0;

/* Forward declarations */
static int SetupUSBPPort(cy_as_device_handle h, uint8_t media, cy_bool isTurbo) ;
static void MyUsbEventCallbackMS(cy_as_device_handle h, cy_as_usb_event ev, void *evdata) ;
//static void PrintData(const char *name, uint8_t *data, uint16_t size) ;

extern cy_as_device_handle cyasdevice_getdevhandle (void);
extern cy_as_hal_device_tag cyasdevice_gethaltag (void);

static void
StallCallback(cy_as_device_handle h, cy_as_usb_event status, uint32_t tag, cy_as_funct_c_b_type cbtype, void *cbdata)
{
    (void)h ;
    (void)cbtype ;
    (void)cbdata ;

    if (tag == 1)
        cy_as_hal_print_message("*** Nak callback - status = %d\n", status) ;
    else
        cy_as_hal_print_message("*** Stall callback - status = %d\n", status) ;
}

static void StallCallbackEX(cy_as_device_handle h,
                cy_as_usb_event status,
                uint32_t tag,
                cy_as_funct_c_b_type type,
                void*   data)
{
    (void)h ;
    (void)type ;
    (void)data ;
    (void)status ;

    if(tag == 18)
    {
        cy_as_usb_event ret = cy_as_usb_clear_stall(h, 3, (cy_as_function_callback)StallCallbackEX, 21);
        cy_as_hal_assert(ret == CY_AS_ERROR_SUCCESS) ;
    }
    else
        gAsyncStallDone = cy_true ;
}
#if 0
static void
StallCallbackAsync(cy_as_device_handle h, cy_as_usb_event status, uint32_t tag, cy_as_funct_c_b_type cbtype, void *cbdata)
{
    cy_as_usb_event ret ;
    (void)cbtype ;
    (void)cbdata ;
    (void)tag ;
    (void)status ;

    if(gAsyncStallStale == 0)
    {
        gAsyncStallStale++;
        ret = cy_as_usb_clear_stall(h, 3, (cy_as_function_callback)StallCallbackAsync, 21);
        cy_as_hal_assert(ret == CY_AS_ERROR_SUCCESS) ;
    }
    else
    {
        gAsyncStallDone = cy_true ;
    }
}
#endif
static void
MyStorageEventCBMS(cy_as_device_handle h, cy_as_bus_number_t bus, uint32_t device, cy_as_storage_event evtype, void *evdata)
{
    (void)h ;
    (void)evdata ;

    switch (evtype)
    {
    case cy_as_storage_antioch:
        cy_as_hal_print_message("CyAsStorageAntioch Event: bus=%d, device=%d\n", bus, device) ;
        switch (bus)
        {
        case 0:
            gStorageReleaseBus0 = cy_true ;
            break;
        case 1:
            gStorageReleaseBus1 = cy_true ;
            break;
        default:
            break;
        }
        break;

    case cy_as_storage_processor:
        cy_as_hal_print_message("CyAsStorageProcessor Event: bus=%d, device %d\n", bus, device) ;
        break;

    case cy_as_storage_removed:
        cy_as_hal_print_message("Bus %d, device %d has been removed\n", bus, device) ;
        break;

    case cy_as_storage_inserted:
        cy_as_hal_print_message("Bus %d, device %d has been inserted\n", bus, device) ;
        break;

    default:
        break;
    }
}

/*
* This function exercises the USB module
*/
int CyAsAPIUsbInit(void)
{
	cy_as_usb_event ret ;
    /*char buffer[16] ;*/
	cy_as_device_handle h = cyasdevice_getdevhandle();
	cy_as_hal_device_tag tag = cyasdevice_gethaltag();

	memset(g_pAstDevice,0, sizeof(g_AstDevice));
	g_pAstDevice->astHalTag = tag ;
	g_pAstDevice->astDevHandle = h ;

    /*
     *  AS!!!: configre HAL dma xfer mode for the endpoints
     */
     // -----------USB ENDPOINTS -----------
    cy_as_hal_set_ep_dma_mode(2, false);  // disable SG assisted xfers for EP 4
    cy_as_hal_set_ep_dma_mode(6, false);
    
    ret = cy_as_storage_start(h, 0, 0) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsAPIUsbInit: CyAsStorageStart returned error code %d\n", ret) ;
        return 0 ;
    }

    /*
    * Register a storage event call-back so that the USB attached storage can be
    * release when the USB connection has been made.
    */
    ret = cy_as_storage_register_callback(h, MyStorageEventCBMS) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsAPIUsbInit: CyAsStorageRegisterCallbackMS returned error code %d\n", ret) ;
        return 0 ;
    }

#if 0
    ret = cy_as_misc_set_low_speed_sd_freq(h, CY_AS_SD_RATED_FREQ, 0, 0) ;
    if ((ret != CY_AS_ERROR_SUCCESS) && (ret != CY_AS_ERROR_INVALID_RESPONSE))
    {
        cy_as_hal_print_message("CyAsMiscSetLowSpeedSDFreq returned error code %d\n",ret) ;
        return 0 ;
    }

		cy_as_misc_read_m_c_u_register(h, 0xe6fb, &g_BackData, 0, 0);
		cy_as_misc_write_m_c_u_register(h, 0xe6fb, 0x0, 0x03, 0, 0);
#endif
	/*
    * We are using P Port based enumeration
    */
    if (!SetupUSBPPort(h, 2, 0))
        return 1 ;
    /*
    * Now we let the enumeration process happen via callbacks.  When the set configuration
    * request is processed, we are done with enumeration and ready to perform our function.
    */
    while (!gSetConfig)
    {
			cy_as_hal_sleep(100) ;
    }
		while(1)
		{
			if (gStorageReleaseBus0)
			{
				ret = cy_as_storage_release(h, 0, 0, 0, 0) ;
				if (ret != CY_AS_ERROR_SUCCESS)
				{
					cy_as_hal_print_message("CyAsStorageReleaseMS returned error code %d\n", ret) ;
					return 1 ;
				}
				gStorageReleaseBus0 = cy_false ;
				break;
			}
			if (gStorageReleaseBus1)
			{
				ret = cy_as_storage_release(h, 1, 0, 0, 0) ;
				if (ret != CY_AS_ERROR_SUCCESS)
				{
					cy_as_hal_print_message("CyAsStorageReleaseMS returned error code %d\n", ret) ;
					return 1 ;
				}
				gStorageReleaseBus1 = cy_false ;
				break;
			}
			cy_as_hal_sleep(100) ;
		}
	cy_as_hal_print_message("*** Configuration complete, starting echo function\n") ;

    return 0 ;
}
EXPORT_SYMBOL (CyAsAPIUsbInit) ;

int CyAsAPIUsbExit(void)
{
    cy_as_usb_event ret ;

		cy_as_usb_disconnect(g_pAstDevice->astDevHandle, 0, 0) ;


    ret = cy_as_usb_stop(g_pAstDevice->astDevHandle, 0, 0) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsUsbStart failed with error code %d\n", ret) ;
        return 0 ;
    }

	#if 0
    ret = cy_as_misc_release_resource(h, cy_as_bus_u_s_b) ;
    if (ret != CY_AS_ERROR_SUCCESS && ret != CY_AS_ERROR_RESOURCE_NOT_OWNED)
    {
        cy_as_hal_print_message("Cannot Release USB reousrce: CyAsMiscReleaseResourceMS failed with error code %d\n", ret) ;
        return 0 ;
    }
	#endif
	
    //cy_as_misc_write_m_c_u_register(g_pAstDevice->astDevHandle, 0xe6fb, 0x0, g_backdata, 0, 0);

		ret = cy_as_storage_stop(g_pAstDevice->astDevHandle, 0, 0) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsAPIUsbInit: CyAsStorageStart returned error code %d\n", ret) ;
        return 0 ;
    }


    cy_as_hal_print_message("*** Exit UMS complete\n") ;

    return 0 ;
}
EXPORT_SYMBOL (CyAsAPIUsbExit) ;
#if 0
static void MyCyAsMTPEventCallback(cy_as_device_handle handle, cy_as_mtp_event evtype,  void* evdata)
{

	(void) handle;
	switch(evtype)
	{
	case cy_as_mtp_send_object_complete:
		{
		    //do_gettimeofday(&tv0);

			cy_as_mtp_send_object_complete_data* sendObjData = (cy_as_mtp_send_object_complete_data*) evdata ;
            /* NOTE: PRINTING IN THE CALLBACK IS HIGHLY UNDESIRABLE, it takes up to 16 ms*/
			cy_as_hal_print_message("<6>MTP EVENT: SendObjectComplete\n");
			cy_as_hal_print_message("<6>Bytes sent = %d\nSend status = %d",sendObjData->byte_count,sendObjData->status);

			g_pAstDevice->tmtpSendCompleteData.byte_count = sendObjData->byte_count;
			g_pAstDevice->tmtpSendCompleteData.status = sendObjData->status;
			g_pAstDevice->tmtpSendCompleteData.transaction_id = sendObjData->transaction_id ;
			g_pAstDevice->tmtpSendComplete = cy_true ;

			//do_gettimeofday(&tv1);
			//printk("<1> MTP_ev cb time:%d", (uint32_t)(tv1.tv_usec - tv0.tv_usec));
			break;
		}
	case cy_as_mtp_get_object_complete:
		{
			cy_as_mtp_get_object_complete_data*  getObjData = (cy_as_mtp_get_object_complete_data*) evdata ;
			cy_as_hal_print_message("<6>MTP EVENT: GetObjectComplete\n");
			cy_as_hal_print_message("<6>Bytes got = %d\nGet status = %d",getObjData->byte_count,getObjData->status);
			g_pAstDevice->tmtpGetCompleteData.byte_count = getObjData->byte_count;
			g_pAstDevice->tmtpGetCompleteData.status = getObjData->status ;
			g_pAstDevice->tmtpGetComplete = cy_true ;
			break;
		}
	case cy_as_mtp_block_table_needed:
		g_pAstDevice->tmtpNeedNewBlkTbl = cy_true ;
		break;
	default:
		;
	}

}
#endif
/*
* This function is responsible for initializing the USB function within West Bridge.  This
* function initializes West Bridge for P port based enumeration.
*/
int SetupUSBPPort(cy_as_device_handle h, uint8_t bus, cy_bool isTurbo)
{
    cy_as_usb_event ret ;
    cy_as_usb_enum_control config ;
//    uint32_t count = 0 ;
//    char *media_name = "SD";

    gUsbTestDone = cy_false ;

    /*
    * Intialize the primary descriptor to be the full speed descriptor and the
    * other descriptor to by the high speed descriptor.  This will swap if we see a
    * high speed event.
    */
    desc_p = (CyCh9ConfigurationDesc *)&ZeroDesc ;
    other_p = (CyCh9ConfigurationDesc *)&ZeroDesc ;

    /* Step 1: Release the USB D+ and D- pins
    *
    * This code releases control of the D+ and D- pins if they have been previously
    * acquired by the P Port processor.  The physical D+ and D- pins are controlled either by
    * West Bridge, or by some other hardware external to West Bridge.  If external hardware is using
    * these pins, West Bridge must put these pins in a high impedence state in order to insure there
    * is no coflict over the use of the pins.  Before we can initialize the USB capabilities of
    * West Bridge, we must be sure West Bridge has ownership of the D+ and D- signals.  West Bridge will take
    * ownership of these pins as long as the P port processor has released them.  This call
    * releases control of these pins.  Before calling the CyAsMiscReleaseResource(), the P port API
    * must configure the hardware to release control of the D+ and D- pins by any external hardware.
    *
    * Note that this call can be made anywhere in the intialization sequence as long as it is done
    * before the call to CyAsUsbConnect().  If not, when the CyAsUsbConnect() call is made, West Bridge
    * will detect that it does not own the D+ and D- pins and the call to CyAsUsbConnect will fail.
    */
    ret = cy_as_misc_release_resource(h, cy_as_bus_u_s_b) ;
    if (ret != CY_AS_ERROR_SUCCESS && ret != CY_AS_ERROR_RESOURCE_NOT_OWNED)
    {
        cy_as_hal_print_message("Cannot Release USB reousrce: CyAsMiscReleaseResourceMS failed with error code %d\n", ret) ;
        return 0 ;
    }

    /*
    * Step 2: Start the USB stack
    *
    * This code initializes the USB stack.  It takes a handle to an West Bridge device
    * previously created with a call to CyAsMiscCreateDevice().
    */
    ret = cy_as_usb_start(h, 0, 0) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsUsbStart failed with error code %d\n", ret) ;
        return 0 ;
    }

    /*
    * Step 3: Register a callback
    *
    * This code registers a callback to handle USB events.  This callback function will handle
    * all setup packets during enumeration as well as other USB events (SUSPEND, RESUME, CONNECT,
    * DISCONNECT, etc.)
    */
    ret = cy_as_usb_register_callback(h, MyUsbEventCallbackMS) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsUsbRegisterCallbackMS failed with error code %d\n", ret) ;
        return 0 ;
    }

    /*
    * Step 4: Setup the enumeration mode
    *
    * This code tells the West Bridge API how enumeration will be done.  Specifically in this
    * example we are configuring the API for P Port processor based enumeraton.  This will cause
    * all setup packets to be relayed to the P port processor via the USB event callback.  See
    * the function CyAsUsbRegisterEventCallback() for more information about this callback.
    */
    config.antioch_enumeration = cy_false ;                      /* P port will do enumeration, not West Bridge */
    config.mtp_interface = 0 ;
    config.mass_storage_interface = 1 ;
    config.mass_storage_callbacks = cy_true ;
    config.devices_to_enumerate[0][0] = cy_true;
    config.devices_to_enumerate[1][0] = cy_true;

    ret = cy_as_usb_set_enum_config(h, &config, 0, 0) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsUsbSetEnumConfigMS failed with error code %d\n", ret) ;
        return 0 ;
    }

    /*
    * Step 6: Connect to the USB host.
    *
    * This code actually connects the D+ and D- signals internal to West Bridge to the D+ and D- pins
    * on the device.  If the host is already physically connected, this will begin the enumeration
    * process.  Otherwise, the enumeration process will being when the host is connected.
    */
    ret = cy_as_usb_connect(h, 0, 0) ;
    if (ret != CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("CyAsUsbConnect failed with error code %d\n", ret) ;
        return 0 ;
    }
    return 1 ;
}

/*
* Print a block of data, useful for displaying data during debug.
*/
#if 0
static void PrintData(const char *name_p, uint8_t *data, uint16_t size)
{
    uint32_t i = 0 ;
    uint32_t linecnt = 0 ;

    while (i < size)
    {
        if (linecnt == 0)
            cy_as_hal_print_message("%s @ %02x:", name_p, i) ;

        cy_as_hal_print_message(" %02x", data[i]) ;

        linecnt++ ;
        i++ ;

        if (linecnt == 16)
        {
            cy_as_hal_print_message("\n") ;
            linecnt = 0 ;
        }
    }

    if (linecnt != 0)
        cy_as_hal_print_message("\n") ;
}
#endif
/**
 * This is the write callback for writes that happen as part of the setup operation.
 **/
static void SetupWriteCallback(cy_as_device_handle h, cy_as_end_point_number_t ep, uint32_t count, void *buf_p, cy_as_usb_event status)
{
    (void)count ;
    (void)h ;
    (void)buf_p ;

    /*assert(ep == 0) ;
    assert(buf_p == replybuf) ;*/

    RestoreReplyArea() ;
    if (status != CY_AS_ERROR_SUCCESS)
        cy_as_hal_print_message("Error returned in SetupWriteCallback - %d\n", status) ;

    gSetupPending = cy_false ;
}

/**
 *
 **/
static cy_as_usb_event SetupWrite(cy_as_device_handle h, uint32_t requested, uint32_t dsize, void *data)
{
    cy_bool spacket = cy_true ;

    if (requested == dsize)
        spacket = cy_false ;
    // spacket =1 , after data is sent, also send a short packet (with Zero data)
    return cy_as_usb_write_data_async(h, 0, dsize, data, spacket, (cy_as_usb_io_callback)SetupWriteCallback) ;
}

/*
* Send the USB host a string descriptor.  If the index is zero, send the
* array of supported languages, otherwise send the string itself per the USB
* Ch9 specification.
*/
static void SendStringDescriptor(cy_as_device_handle h, uint8_t *data)
{
    cy_as_usb_event ret = CY_AS_ERROR_SUCCESS;
    int i = data[2] ; // string index, if 0 - return the descriptor
    // data[3] - not used ?
    int langid = data[4] | (data[5] << 8) ;
    uint16_t reqlen = data[6] | (data[7] << 8) ;

    cy_as_hal_print_message("**** CY_CH9_GD_STRING - %d\n", i) ;
    if (i == 0) // string index 0 , return the list of supported languages
    {
        uint8_t *reply ;

        reply = GetReplyArea() ; // returns pointer to the beg of the reply buffer
        reply[0] = 4 ;           // descriptor size in bytes
        reply[1] = CY_CH9_GD_STRING ; // descriptor dype
        reply[2] = CY_CH9_LANGID_ENGLISH_UNITED_STATES & 0xff ; // supported lang codes ( 16 bit value, US = 0X0409) )
        reply[3] = (CY_CH9_LANGID_ENGLISH_UNITED_STATES >> 8) & 0xff ;

        /* reply with the string descriptor */
        ret = SetupWrite(h, reqlen, 4, reply) ;
    }
    //AS!!!: else if (i <= sizeof(UsbStrings)/sizeof(UsbStrings[0]) && langid == CY_CH9_LANGID_ENGLISH_UNITED_STATES)
    else if  ( (i <= NUM_OF_STRINGS ) /* && (langid == CY_CH9_LANGID_ENGLISH_UNITED_STATES)*/ )
    {                                   // even if Host askes for a string with invalid LANG ID = 0000, give it the freaggin string!!!!
        uint8_t *reply ;
        uint16_t len = (uint16_t)strlen(UsbStrings[i - 1]) ;

        cy_as_hal_print_message("*** Sending string '%s'\n", UsbStrings[i - 1]) ;

        reply = GetReplyArea() ;
        reply[0] = (uint8_t)(len * 2 + 2) ;
        reply[1] = CY_CH9_GD_STRING ;

        /* nxz-linux-port */
        /*memcpy(reply + 2, UsbStrings[i - 1], len ) ;
        ret = SetupWrite(h, reqlen, len  + 2, reply) ;*/
        {
            uint16_t index ;
            uint16_t *rpy = (uint16_t *)(reply + 2) ;
            for (index = 0; index < len; index++)
            {
                *rpy = (uint16_t)(UsbStrings[i - 1][index]) ;   // WCHAR STRINGS
                rpy++ ;
            }
        }
        ret = SetupWrite(h, reqlen, len * 2 + 2, reply) ;
    }
    else
    {
        /*
        * If the host asks for an invalid string, we must stall EP 0
        */
        ret = cy_as_usb_set_stall(h, 0, (cy_as_function_callback)StallCallback, 0) ;
        if (ret != CY_AS_ERROR_SUCCESS)
            cy_as_hal_print_message("**** cannot set stall state on EP 0\n") ;

        cy_as_hal_print_message("Host asked for invalid string or langid, index = 0x%04x, langid = 0x%04x\n", i, langid) ;

    }

    if (ret != CY_AS_ERROR_SUCCESS)
        cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
    else
        cy_as_hal_print_message("** Write Sucessful\n") ;
}

static cy_as_usb_event SendSetupData(cy_as_device_handle h, uint32_t reqlen, uint32_t size, void *data_p)
{
    cy_as_usb_event ret ;
    uint8_t *reply ;

    /*
    * Never send more data than was requested
    */
    if (size > reqlen)
        size = reqlen ;

    reply = GetReplyArea() ;
    /*assert(reply != 0) ;*/

    memcpy(reply, data_p, size) ;

    /** As!!! print that we actually send  **/
    //cyashal_prn_buf(reply, 0, size);

    ret = SetupWrite(h, reqlen, size, reply) ;
    if (ret != CY_AS_ERROR_SUCCESS)
        RestoreReplyArea() ; // doesn't do squat

    return ret ;
}

/*
* This function processes the GET DESCRIPTOR usb request.
*/
static void ProcessGetDescriptorRequest(cy_as_device_handle h, uint8_t *data)
{
    cy_as_usb_event ret ;

    uint16_t reqlen = data[6] | (data[7] << 8);  /* DATA LEN that hosts expecting the DEVICE to send back */

    if (data[3] == CY_CH9_GD_DEVICE)
    {
        /*
        * Return the device descriptor
        */
        cy_as_hal_print_message("<1>**** CY_CH9_GD_DEVICE (size = %d)\n", sizeof(pport_device_desc)) ;
        //PrintData("DD", (uint8_t *)&pport_device_desc, sizeof(pport_device_desc)) ;

        ret = SendSetupData(h, reqlen, sizeof(pport_device_desc), &pport_device_desc);

        if (ret != CY_AS_ERROR_SUCCESS)
            cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
        else
            cy_as_hal_print_message("** Write Sucessful\n") ;
    }
    else if (data[3] == CY_CH9_GD_DEVICE_QUALIFIER)
    {
        /*
        * Return the device descriptor
        */
        cy_as_hal_print_message("*** CY_CH9_GD_DEVICE_QUALIFIER (size = %d)\n", sizeof(device_qualifier)) ;
        //PrintData("DD", (uint8_t *)&device_qualifier, sizeof(device_qualifier)) ;

        ret = SendSetupData(h, reqlen, sizeof(device_qualifier), &device_qualifier);

        if (ret != CY_AS_ERROR_SUCCESS)
            cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
        else
            cy_as_hal_print_message("** Write Sucessful\n") ;
    }
    else if (data[3] == CY_CH9_GD_CONFIGURATION)
    {
        const char *desc_name_p ;
        uint16_t size ;

        /*
        * Return the CONFIGURATION descriptor.
        */
        //printk("<1> AS: ConfigHSDesc =%p zdesc=%p desc_p=%p\n", &ConfigHSDesc, &ZeroDesc, desc_p);


        if (desc_p == (CyCh9ConfigurationDesc *)&ConfigHSDesc)
        {
            desc_name_p = "HighSpeed" ;
            size = sizeof(ConfigHSDesc) ;
        }
        else if (desc_p == (CyCh9ConfigurationDesc *)&ConfigFSDesc)
        {
            desc_name_p = "FullSpeed" ;
            size = sizeof(ConfigFSDesc) ;
        }
        else if (desc_p == &ZeroDesc)
        {
            desc_name_p = "ZeroDesc" ;
            size = sizeof(ZeroDesc) ;
        }
        else
        {
            desc_name_p = "UNKNOWN" ;
            size = 0 ;
        }

        cy_as_hal_print_message("<1>**** CY_CH9_GD_CONFIGURATION - %s (Des_size = %d), req_len:%2x\n", desc_name_p, size, reqlen) ;
        if (size > 0)
        {
            //PrintData("CFG", (uint8_t *)desc_p, size) ;

            desc_p->bDescriptorType = CY_CH9_GD_CONFIGURATION;

            ret = SendSetupData(h, reqlen, size, desc_p) ;

            if (ret != CY_AS_ERROR_SUCCESS)
                cy_as_hal_print_message("<1>****** ERROR WRITING USB DATA - %d\n", ret) ;
            else
                cy_as_hal_print_message("<1> Write Sucessful\n") ;
        }
    }
    else if (data[3] == CY_CH9_GD_OTHER_SPEED_CONFIGURATION)
    {
        const char *desc_name_p ;
        uint16_t size ;

        /*
        * Return the CONFIGURATION descriptor.
        */
        if (other_p == (CyCh9ConfigurationDesc *)&ConfigHSDesc)
        {
            desc_name_p = "HighSpeed" ;
            size = sizeof(ConfigHSDesc) ;
        }
        else if (other_p == (CyCh9ConfigurationDesc *)&ConfigFSDesc)
        {
            desc_name_p = "FullSpeed" ;
            size = sizeof(ConfigFSDesc) ;
        }
        else if (other_p == &ZeroDesc)
        {
            desc_name_p = "ZeroDesc" ;
            size = sizeof(ZeroDesc) ;
        }
        else
        {
            desc_name_p = "UNKNOWN" ;
            size = 0 ;
        }
        cy_as_hal_print_message("<1>**** CY_CH9_GD_OTHER_SPEED_CONFIGURATION - %s (size = %d)\n", desc_name_p, size) ;
        if (size > 0)
        {
            //PrintData("CFG", (uint8_t *)other_p, size) ;
            other_p->bDescriptorType = CY_CH9_GD_OTHER_SPEED_CONFIGURATION;
            ret = SendSetupData(h, reqlen, size, other_p) ;
            if (ret != CY_AS_ERROR_SUCCESS)
                cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
            else
                cy_as_hal_print_message("** Write Sucessful\n") ;
        }
    }
    else if (data[3] == CY_CH9_GD_STRING)
    {
        SendStringDescriptor(h, data) ;
    }
    else if (data[3] == CY_CH9_GD_REPORT)
    {
        cy_as_hal_print_message("**** CY_CH9_GD_REPORT\n") ;
    }
    else if (data[3] == CY_CH9_GD_HID)
    {
        cy_as_hal_print_message("**** CY_CH9_GD_HID\n") ;
    }
    else
    {
        cy_as_hal_print_message("**** Unknown Descriptor request\n") ;
    }
}

/*
static void EP0DataCallback(cy_as_device_handle h, cy_as_end_point_number_t ep, uint32_t count, void *buf_p, cy_as_usb_event status)
{
    (void)ep ;
    (void)h ;

    if (status == CY_AS_ERROR_SUCCESS)
    {
        cy_as_hal_print_message("Read data phase of setup packet from EP0\n") ;
        //PrintData("SetupData", buf_p, (uint16_t)count) ;
    }
    else
    {
        cy_as_hal_print_message("Error reading data from EP0\n") ;
    }
}
*/

/**
 AS!!! DBG STUFF
**/
void TmtpReadCallback(cy_as_device_handle	handle,		/* Handle to the device to configure */
                    cy_as_end_point_number_t ep,		/* The endpoint that has completed an operation */
                    uint32_t		  count,		/* THe amount of data transferred to/from USB */
                    void *			  buffer,		/* The data buffer for the operation */
                    cy_as_usb_event status)		/* The error status of the operation */
{
    //printk("<1>cyaschardev: tmtp rd callback: stat:%d\n", status);

}


/**
 *
 **/
static void ProcessSetupPacketRequest(cy_as_device_handle h, uint8_t *data)
{
    cy_as_usb_event ret ;
    uint16_t reqlen = data[6] | (data[7] << 8) ;

    RestoreReplyArea() ; // actually it doesn't do anything

    if ((data[0] & CY_CH9_SETUP_TYPE_MASK) == CY_CH9_SETUP_STANDARD_REQUEST) // reqyest type is in bits {x,d6,d5,x, x,x,x,x}
    {
        // standard request ( d6,d5 = 00)
        switch(data[1]) // d1 is breqest field - request type
        {
        case CY_CH9_SC_GET_DESCRIPTOR:
            cy_as_hal_print_message("USB EP0 : CY_CH9_SC_GET_DESCRIPTOR request\n") ;
            ProcessGetDescriptorRequest(h, data) ;
            break ;

        case CY_CH9_SC_GET_INTERFACE:
            {
                uint8_t *response = GetReplyArea() ;

                *response = 0 ;
                cy_as_hal_print_message("************* USB EP0: CY_CH9_SC_GET_INTERFACE request - RETURNING ZERO\n") ;
                ret = SetupWrite(h, reqlen, 1, response) ; // issues aSync USB write
                if (ret != CY_AS_ERROR_SUCCESS)
                {
                    cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
                }
            }
            break ;

        case CY_CH9_SC_SET_INTERFACE:
            cy_as_hal_print_message("USB EP0 : CY_CH9_SC_SET_INTERFACE request\n") ;
            // select the specified interface if we have more then one
            break ;

        case CY_CH9_SC_SET_CONFIGURATION:
			{	/* Set configuration is the last step before host send MTP data to EP2 */

				/*cy_as_usb_event ret = CY_AS_ERROR_SUCCESS ;*/

                cy_as_hal_print_message("=========================\n");
				cy_as_hal_print_message("USB EP0: CY_CH9_SC_SET_CONFIGURATION request (%02x)\n", data[2]) ;
                cy_as_hal_print_message("=========================\n");
				{
					gAsyncStallDone = cy_false ;
					gAsyncStallStale = 0;
					cy_as_usb_set_stall(h, 3, (cy_as_function_callback)StallCallbackEX, 18);
				}
				gSetConfig = cy_true ;
				g_pAstDevice->configDone = 1 ;
				MyConfiguration = data[2];

			}
            break ;

        case CY_CH9_SC_GET_CONFIGURATION:
            {
                uint8_t *response = GetReplyArea() ;

                *response = MyConfiguration ;
                cy_as_hal_print_message("USB EP0 : CY_CH9_SC_GET_INTERFACE request\n") ;
                ret = SetupWrite(h, reqlen, 1, response) ;
                if (ret != CY_AS_ERROR_SUCCESS)
                {
                    cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
                }
            }
            cy_as_hal_print_message("USB EP0 : CY_CH9_SC_GET_CONFIGURATION request\n") ;
            break ;

        case CY_CH9_SC_GET_STATUS:
            {
                uint16_t *response = (uint16_t *)GetReplyArea() ;

                *response = 0 ;
                cy_as_hal_print_message("USB EP0 : CY_CH9_SC_GET_STATUS request\n") ;
                ret = SetupWrite(h, reqlen, 2, response) ;
                if (ret != CY_AS_ERROR_SUCCESS)
                {
                    cy_as_hal_print_message("****** ERROR WRITING USB DATA - %d\n", ret) ;
                }
            }
            break ;

        case CY_CH9_SC_CLEAR_FEATURE:
            {
                uint16_t feature = data[2] | (data[3] << 8) ;
                cy_as_hal_print_message("USB EP0 : CY_CH9_SC_CLEAR_FEATURE request\n") ;

                if ((data[0] & CY_CH9_SETUP_DEST_MASK) == CY_CH9_SETUP_DEST_ENDPOINT && feature == 0)
                {
                    cy_as_end_point_number_t ep = data[4] | (data[5] << 8) ;
                    /* This is a clear feature/endpoint halt on an endpoint */
                    cy_as_hal_print_message("Calling ClearStall on EP %d\n", ep) ;
                    ret = cy_as_usb_clear_stall(h, ep, (cy_as_function_callback)StallCallback, 0) ;
                    if (ret != CY_AS_ERROR_SUCCESS)
                    {
                        cy_as_hal_print_message("******* ERROR SEND CLEAR STALL REQUEST - %d\n", ret) ;
                    }
                }
            }
            break ;

        case CY_CH9_SC_SET_FEATURE:
            {
                uint16_t feature = data[2] | (data[3] << 8) ;
                cy_as_hal_print_message("USB EP0 : CY_CH9_SC_SET_FEATURE request\n") ;

                if ((data[0] & CY_CH9_SETUP_DEST_MASK) == CY_CH9_SETUP_DEST_ENDPOINT && feature == 0)
                {
                    cy_as_end_point_number_t ep = data[4] | (data[5] << 8) ;
                    /* This is a clear feature/endpoint halt on an endpoint */
                    cy_as_hal_print_message("Calling SetStall on EP %d\n", ep) ;
                    ret = cy_as_usb_set_stall(h, ep, (cy_as_function_callback)StallCallback, 0) ;
                    if (ret != CY_AS_ERROR_SUCCESS)
                    {
                        cy_as_hal_print_message("******* ERROR SEND SET STALL REQUEST - %d\n", ret) ;
                    }
                }
            }
            break;
        }
    }
    else if ((data[0] & CY_CH9_SETUP_TYPE_MASK) == CY_CH9_SETUP_CLASS_REQUEST)
    {
        /*
        * Handle class requests other than Mass Storage
        */
        ret = cy_as_usb_set_stall(h, 0, (cy_as_function_callback)StallCallback, 0) ;
        cy_as_hal_print_message("Sending stall request\n") ;
        if (ret != CY_AS_ERROR_SUCCESS)
            cy_as_hal_print_message("**** cannot set stall state on EP 0\n") ;
    }
    else
    {
        static char buf[1024] ;

        if ((data[0] & 0x80) == 0)
        {
            if (reqlen != 0)
            {
				cy_as_hal_print_message("OUT setup request with additional data\n") ;
                /* This is an OUT setup request, with additional data to follow */
                /*ret = CyAsUsbReadDataAsync(h, 0, cy_false, reqlen, buf, EP0DataCallback) ;
				if(ret != CY_AS_ERROR_SUCCESS && ret != CY_AS_ERROR_ASYNC_PENDING)
				{
					cy_as_hal_print_message("CyAsMtpApp: CyAsUsbReadDataAsync Failed. Reason code: %d\n",ret) ;
				}*/
            }
            else
            {
                cy_as_hal_print_message("Call setnak\n") ;
                ret = cy_as_usb_set_nak(h, 3, (cy_as_function_callback)StallCallback, 1) ;
                if (ret != CY_AS_ERROR_SUCCESS)
                {
                    cy_as_hal_print_message("Error in CyAsUsbSetNak - %d\n", ret) ;
                }
            }
        }
        else
        {
            if (reqlen != 0)
            {
                /*
                * This is an unknown setup packet, probably some type of generated packet or a class
                * packet we do not understand.  We just send back some data.
                */
                cy_as_hal_mem_set(buf, 0x44, sizeof(buf)) ;
                ret = SendSetupData(h, reqlen, reqlen, buf) ;
                if (ret != CY_AS_ERROR_SUCCESS)
                {
                    cy_as_hal_print_message("Error ending setup data in response to unknown packet\n") ;
                }
                else
                {
                    cy_as_hal_print_message("Sent setup data associated with the unknown setup packet\n") ;
                }
            }
            else
            {
                cy_as_hal_print_message("Call setnak\n") ;
                ret = cy_as_usb_set_nak(h, 3, (cy_as_function_callback)StallCallback, 1) ;
                if (ret != CY_AS_ERROR_SUCCESS)
                {
                    cy_as_hal_print_message("Error in CyAsUsbSetNak - %d\n", ret) ;
                }
            }
        }
    }
}



/**
 *  ************* westbridge USB_Event callback *********************
 *
 **/
static void MyUsbEventCallbackMS(cy_as_device_handle h, cy_as_usb_event ev, void *evdata)
{
    cy_as_hal_print_message("----- chardev: MyUsbEventCallbackMS()  Enter -----\n") ;
    switch(ev)
    {
    case cy_as_event_usb_suspend:
        cy_as_hal_print_message("CyAsEventUsbSuspend received\n") ;

        //** AS!!! notify the user level, that the device is no longer connected, or will reconfigure
        g_pAstDevice->configDone = 0;

        break ;
    case cy_as_event_usb_resume:
		{
			/*cy_as_usb_event ret = CY_AS_ERROR_SUCCESS;*/
			cy_as_hal_print_message("CyAsEventUsbResume received\n") ;
		}
        break ;
    case cy_as_event_usb_reset:

      //** AS!!!
     g_pAstDevice->configDone = 0;

        desc_p = (CyCh9ConfigurationDesc *)&ZeroDesc ;
        other_p = (CyCh9ConfigurationDesc *)&ZeroDesc ;
        cy_as_hal_print_message("CyAsEventUsbReset received\n") ;
        break ;


    case cy_as_event_usb_speed_change:
       {
         uint16_t speed = *((uint16_t *)evdata);
	desc_p = (CyCh9ConfigurationDesc *)&ZeroDesc ;
        other_p = (CyCh9ConfigurationDesc *)&ZeroDesc ;
            /** speed = 1 high, 0 full**/
            if (speed==1) {
                cy_as_hal_print_message("CyAsEventUsbSpeedChanged: 2 HIGH\n") ;
            } else {
                cy_as_hal_print_message("CyAsEventUsbSpeedChanged: 2 FULL\n") ;
            }

        }
        break ;


    case cy_as_event_usb_set_config:
        cy_as_hal_print_message("CyAsEventUsbSetConfig received\n") ;
        gSetConfig = cy_true ;
        break ;


    case cy_as_event_usb_setup_packet:
        //PrintData("CyAsEventUsbSetupPacket received: ", evdata, 8) ;
        ProcessSetupPacketRequest(h, (uint8_t *)evdata) ;
        break ;


    case cy_as_event_usb_status_packet:
        cy_as_hal_print_message("CyAsEventUsbStatusPacket received\n") ;
        break ;


    case cy_as_event_usb_inquiry_before:
        cy_as_hal_print_message("CyAsEventUsbInquiryBefore received\n") ;
        {
            cy_as_usb_inquiry_data *data = (cy_as_usb_inquiry_data *)evdata ;
            data->updated = cy_true ;
            data = data ;
        }
        break ;


    case cy_as_event_usb_inquiry_after:
        cy_as_hal_print_message("CyAsEventUsbInquiryAfter received\n") ;
        break ;


    case cy_as_event_usb_start_stop:
        cy_as_hal_print_message("CyAsEventUsbStartStop received\n") ;
        {
            cy_as_usb_start_stop_data *data = (cy_as_usb_start_stop_data *)evdata ;
            data = data ;
        }
        break ;


    default:
        break;
    }
    cy_as_hal_print_message("----- chardev: MyUsbEventCallbackMS()  Exit -----\n\n") ;
}

