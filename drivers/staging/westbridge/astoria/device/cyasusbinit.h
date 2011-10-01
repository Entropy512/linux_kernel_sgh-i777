
#ifndef ASTORIAUSBINIT_H
#define ASTORIAUSBINIT_H

/* The BUS bits */
#define CY_TEST_BUS_0               (0x01)
#define CY_TEST_BUS_1               (0x02)

typedef struct TmtpAstDev
{
    cy_as_hal_device_tag    astHalTag;
    cy_as_device_handle 		astDevHandle;

        /* EP related stats */
    volatile uint8_t    astEPDataAvail;
    uint32_t            astEPBuflength;
    uint8_t             astEPBuf[512];

    /* Current Transaction Id */
    uint32_t tId ;

    /* Data members to be used by user-implemented MTPEventCallback() via the relevant interface methods */
    volatile cy_bool     tmtpSendComplete;
    volatile cy_bool     tmtpGetComplete;
    volatile cy_bool     tmtpNeedNewBlkTbl;

    cy_as_storage_query_device_data  dev_data;
    cy_as_storage_query_unit_data    unit_data;

    /* Data member used to store the SendObjectComplete event data */
    cy_as_mtp_send_object_complete_data tmtpSendCompleteData;

    /* Data member used to store the GetObjectComplete event data */
    cy_as_mtp_get_object_complete_data tmtpGetCompleteData;

    uint8_t configDone ;
} TmtpAstDev;


int CyAsAPIUsbInit(void);

#endif
