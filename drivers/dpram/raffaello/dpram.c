/****************************************************************************
**
** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED
**
**                IDPRAM Device Driver
**
****************************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif    /* CONFIG_PROC_FS */

#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/netdevice.h>
#ifdef CONFIG_KERNEL_DEBUG_SEC    
#include <linux/kernel_sec_common.h>
#endif
//#include <mach/ds_manager.h>

#include "dpram.h"
#include <mach/system.h>
#include <mach/sec_debug.h>

/***************************************************************************/
/*                              GPIO SETTING                               */
/***************************************************************************/
#include <mach/gpio.h>

int sec_debug_dpram(void);

/* Added to store irq num */
int dpram_wakeup_irq, phone_active_irq;
extern u32 rec_lock; // DPRAM Recovery

#define GPIO_QSC_INT        GPIO_C210_DPRAM_INT_N
#define IRQ_QSC_INT         GPIO_QSC_INT


#ifdef IRQ_DPRAM_AP_INT_N
#undef IRQ_DPRAM_AP_INT_N
#endif
#define IRQ_DPRAM_AP_INT_N  IRQ_MODEM_IF     // IDPRAM's special interrupt in AP

//#define DEBUG_DPRAM_INT_HANDLER
#define PRINT_DPRAM_PWR_CTRL
//#define PRINT_DPRAM_WRITE
//#define PRINT_DPRAM_READ
//#define PRINT_DPRAM_HEAD_TAIL

/*
 * GLOBALS
 */
volatile void __iomem       *idpram_base = NULL;
volatile IDPRAM_SFR __iomem *idpram_sfr_base = NULL;

static volatile u16         *dpram_mbx_BA;      //send mail box
static volatile u16         *dpram_mbx_AB;      //received mail box

struct wake_lock            dpram_wake_lock = {.name = NULL};
static atomic_t             dpram_write_lock;
static atomic_t             dpram_read_lock;

static int                  g_phone_sync = 0;
static int                  g_dump_on = 0;
static int                  g_dpram_wpend = IDPRAM_WPEND_UNLOCK;
struct completion           g_complete_dpramdown;
static int                  g_cp_reset_cnt = 0;
static int                  g_phone_power_off_sequence = 0;

static struct tty_driver    *dpram_tty_driver;
static struct tty_struct    *dpram_tty[MAX_INDEX];
static struct ktermios      *dpram_termios[MAX_INDEX];
static struct ktermios      *dpram_termios_locked[MAX_INDEX];
static struct pdp_info      *pdp_table[MAX_PDP_CONTEXT];

static dpram_tasklet_data_t dpram_tasklet_data[MAX_INDEX];

static dpram_device_t       dpram_table[MAX_INDEX] =
{
    {
        .in_head_addr = DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS,
        .in_tail_addr = DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS,
        .in_buff_addr = DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS,
        .in_buff_size = DPRAM_PHONE2PDA_FORMATTED_BUFFER_SIZE,
        .in_head_saved = 0,
        .in_tail_saved = 0,

        .out_head_addr = DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS,
        .out_tail_addr = DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS,
        .out_buff_addr = DPRAM_PDA2PHONE_FORMATTED_BUFFER_ADDRESS,
        .out_buff_size = DPRAM_PDA2PHONE_FORMATTED_BUFFER_SIZE,
        .out_head_saved = 0,
        .out_tail_saved = 0,

        .mask_req_ack = INT_MASK_REQ_ACK_F,
        .mask_res_ack = INT_MASK_RES_ACK_F,
        .mask_send = INT_MASK_SEND_F,
    },
    {
        .in_head_addr = DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS,
        .in_tail_addr = DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS,
        .in_buff_addr = DPRAM_PHONE2PDA_RAW_BUFFER_ADDRESS,
        .in_buff_size = DPRAM_PHONE2PDA_RAW_BUFFER_SIZE,
        .in_head_saved = 0,
        .in_tail_saved = 0,

        .out_head_addr = DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS,
        .out_tail_addr = DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS,
        .out_buff_addr = DPRAM_PDA2PHONE_RAW_BUFFER_ADDRESS,
        .out_buff_size = DPRAM_PDA2PHONE_RAW_BUFFER_SIZE,
        .out_head_saved = 0,
        .out_tail_saved = 0,

        .mask_req_ack = INT_MASK_REQ_ACK_R,
        .mask_res_ack = INT_MASK_RES_ACK_R,
        .mask_send = INT_MASK_SEND_R,
    },
};

static unsigned int __log_level__ = (DL_IPC|DL_WARN|DL_NOTICE|DL_INFO|DL_DEBUG);

static char print_buff[128];

static atomic_t raw_txq_req_ack_rcvd;
static atomic_t fmt_txq_req_ack_rcvd;
struct delayed_work phone_active_work;


/*
** TASKLET DECLARATIONs
*/
static void res_ack_tasklet_handler(unsigned long data);
static void fmt_rcv_tasklet_handler(unsigned long data);
static void raw_rcv_tasklet_handler(unsigned long data);

static DECLARE_TASKLET(fmt_send_tasklet, fmt_rcv_tasklet_handler, 0);
static DECLARE_TASKLET(raw_send_tasklet, raw_rcv_tasklet_handler, 0);
static DECLARE_TASKLET(fmt_res_ack_tasklet, res_ack_tasklet_handler, (unsigned long)&dpram_table[FORMATTED_INDEX]);
static DECLARE_TASKLET(raw_res_ack_tasklet, res_ack_tasklet_handler, (unsigned long)&dpram_table[RAW_INDEX]);
static DEFINE_MUTEX(pdp_lock);


/*
** FUNCTION PROTOTYPEs
*/
static void dpram_drop_data(dpram_device_t *device);
static int  kernel_sec_dump_cp_handle2(void);
static int  dpram_phone_getstatus(void);
static void dpram_send_mbx_BA(u16 irq_mask);
static void dpram_send_mbx_BA_cmd(u16 irq_mask);
static void dpram_power_down(void);
static void dpram_powerup_start(void);
static int register_interrupt_handler(void);
static irqreturn_t dpram_irq_handler(int irq, void *dev_id);


static inline struct pdp_info * pdp_get_dev(u8 id);
static inline void check_pdp_table(char*, int);
static void cmd_error_display_handler(void);


#ifdef _ENABLE_ERROR_DEVICE
static unsigned int dpram_err_len;
static char         dpram_err_buf[DPRAM_ERR_MSG_LEN];
static unsigned int dpram_err_cause = 0;

struct class *dpram_class;

static DECLARE_WAIT_QUEUE_HEAD(dpram_err_wait_q);

static struct fasync_struct *dpram_err_async_q;

extern void usb_switch_mode(int);
#endif    /* _ENABLE_ERROR_DEVICE */


#ifndef DISABLE_IPC_DEBUG

typedef struct {
   unsigned char start_flag;
   unsigned char hdlc_length[2];
   unsigned char hdlc_control;
   unsigned char ipc_length[2];
   unsigned char msg_seq;
   unsigned char ack_seq;
   unsigned char main_cmd;
   unsigned char sub_cmd;
   unsigned char cmd_type;
   unsigned char parameter[1024];
} dpram_fmt_frame;

typedef enum {
   IPC_PWR_CMD = 1,
   IPC_CALL_CMD,
   IPC_DATA_CMD,
   IPC_SMS_CMD,
   IPC_SEC_CMD,
   IPC_PB_CMD,
   IPC_DISPLAY_CMD,
   IPC_NET_CMD,
   IPC_SND_CMD,
   IPC_MISC_CMD,
   IPC_SVC_CMD,
   IPC_SS_CMD,
   IPC_DATA_GPRS_CMD,
   IPC_GEN_RESP = 0x80,          //General Response
/*--------------------------*/
   IPC_LTE_DM_CMD = 0xA0,
   IPC_LTE_CT_CMD,              // 0xA1
   IPC_LTE_CM_CMD,              // 0xA2
   IPC_LTE_MTM_CMD,             // 0xA3
   IPC_LTE_FOTA_CMD             // 0xA4
} tipc_main_cmd;

typedef enum {
    CM_SUBCMD_CMIF_VERIFY = 0x00,
    CM_SUBCMD_MM_SERVICE_STATE,             //0x01
    CM_SUBCMD_ATTACH,                       //0x02
    CM_SUBCMD_IP_INFORMATION,               //0x03
    CM_SUBCMD_PDP_CONTEXT_ACTIVATION,       //0x04
    CM_SUBCMD_PDP_CONTEXT_DEACTIVATION,     //0x05
    CM_SUBCMD_2ND_PDP_CONTEXT_ACTIVATION,   //0x06
    CM_SUBCMD_RAT_MODE,                     //0x07
    CM_SUBCMD_SIGNAL_LEVEL,                 //0x08
    CM_SUBCMD_PLMN_LIST,                    //0x09
    CM_SUBCMD_PLMN_SELECTION_MODE,          //0x0A
    CM_SUBCMD_PLMN_SELECTION,               //0x0B
    CM_SUBCMD_INTERFACE_ONOFF,              //0x0C
    CM_SUBCMD_SS_RESET,                     //0x0D
    CM_SUBCMD_SW_VERSION,                   //0x0E
    CM_SUBCMD_SS_MAC_ADDRESS,               //0x0F
    CM_SUBCMD_SYNC,                         //0x10
    CM_SUBCMD_PIN_ACTION,                   //0x11
    CM_SUBCMD_SIM_STATUS,                   //0x12
    CM_SUBCMD_SIM_INFORMATION,              //0x13
    CM_SUBCMD_MMC_MODE_OPERATION,           //0x14
    CM_SUBCMD_SS_STATE,                     //0x15
    CM_SUBCMD_HW_VERSION,                   //0x16
    CM_SUBCMD_DRX,                          //0x17
    CM_SUBCMD_IMEI,                         //0x18
    CM_SUBCMD_SS_POWER_DOWN,                //0x19
    CM_SUBCMD_MODEM_INTERFACE_TYPE,         //0x1A
    CM_SUBCMD_PHONE_DATA_TEST_MODE,         //0x1B
    CM_SUBCMD_UART_PORT_TYPE,               //0x1C
    CM_SUBCMD_PORTS_RETURN,                 //0x1D
    CM_SUBCMD_SLEEP_MODE,                   //0x1E
    CM_SUBCMD_TEST_NV_INFO                  //0x1F
} tipc_cm_subcmd;
#endif

/*
** tty related functions.
*/
static inline void dpram_byte_align(unsigned long dest, unsigned long src)
{
    volatile u16 *p_dest;
    u16          *p_src;

    if ( (dest & 1) && (src & 1) )
    {
        p_dest = (u16 *)(dest - 1);
        p_src = (u16 *)(src - 1);

        *p_dest = (*p_dest & 0x00FF) | (*p_src & 0xFF00);
    }
    else if ( (dest & 1) && !(src & 1) )
    {
        p_dest = (u16 *)(dest - 1);
        p_src = (u16 *)src;

        *p_dest = (*p_dest & 0x00FF) | ((*p_src << 8) & 0xFF00);
    }
    else if ( !(dest & 1) && (src & 1) )
    {
        p_dest = (u16 *)dest;
        p_src = (u16 *)(src - 1);

        *p_dest = (*p_dest & 0xFF00) | ((*p_src >> 8) & 0x00FF);
    }
    else //if ( !(dest & 1) && !(src & 1) )
    {
        p_dest = (u16 *)dest;
        p_src = (u16 *)src;

        *p_dest = (*p_dest & 0xFF00) | (*p_src & 0x00FF);
    }
#if 0
    else
    {
        LOGE("oops.~\n");
    }
#endif
}

static inline void _memcpy(void *p_dest, const void *p_src, int size)
{
    unsigned long dest = (unsigned long)p_dest;
    unsigned long src = (unsigned long)p_src;

    if (size <= 0)
        return;

    if (dest & 1)
    {
        // If the destination address is odd, store the first byte at first.
        dpram_byte_align(dest, src);
        dest++, src++;
        size--;
    }

    if (size & 1)
    {
        // If the size is odd, store the last byte at first.
        dpram_byte_align(dest + size - 1, src + size - 1);
        size--;
    }

    if (src & 1)
    {
        volatile u16  *d = (u16 *)dest;
        unsigned char *s = (u8 *)src;

        size >>= 1;

        while (size--)
        {
            *d++ = s[0] | (s[1] << 8);
            s += 2;
        }
    }
    else
    {
        volatile u16 *d = (u16 *)dest;
        u16          *s = (u16 *)src;

        size >>= 1;

        while (size--)
        {
            *d++ = *s++;
        }
    }
}

/* Note the use of non-standard return values (0=match, 1=no-match) */
static inline int _memcmp(u8 *dest, u8 *src, int size)
{
    while( size-- )
    {
        if( *dest++ != *src++ )
            return 1 ;
    }

    return 0 ;
}


#define WRITE_TO_DPRAM(dest, src, size) \
    _memcpy((void *)(DPRAM_VBASE + dest), src, size)

#define READ_FROM_DPRAM(dest, src, size) \
    _memcpy(dest, (void *)(DPRAM_VBASE + src), size)


static inline int WRITE_TO_DPRAM_VERIFY(u32 dest, void *src, int size)
{
    int cnt = 3;

    while (cnt--)
    {
        _memcpy((void *)(DPRAM_VBASE + dest), (void *)src, size);

        if (!_memcmp((u8 *)(DPRAM_VBASE + dest), (u8 *)src, size))
            return 0;
    }

    return -1;
}

static inline int READ_FROM_DPRAM_VERIFY(void *dest, u32 src, int size)
{
    int cnt = 3;

    while (cnt--)
    {
        _memcpy((void *)dest, (void *)(DPRAM_VBASE + src), size);

        if (!_memcmp((u8 *)dest, (u8 *)(DPRAM_VBASE + src), size))
            return 0;
    }

    return -1;
}

static int dpram_get_lock_write(void)
{
    return atomic_read(&dpram_write_lock);
}

static int dpram_lock_write(const char* func)
{    
    int lock_value;

    lock_value = (g_dpram_wpend == IDPRAM_WPEND_LOCK) ? -3 : atomic_inc_return(&dpram_write_lock);
    if ( lock_value != 1 )
        LOGE("lock_value (%d) != 1\n", lock_value);

    return lock_value;
}

static void dpram_unlock_write(const char* func)
{
    int lock_value;

    lock_value = atomic_dec_return(&dpram_write_lock);

    if ( lock_value != 0 )
        LOGE("lock_value (%d) != 0\n", lock_value);
}

// TODO:
// because it is dpram device, I think read_lock don't need but I will use for holding wake_lock
static int dpram_get_lock_read(void)
{
    return atomic_read(&dpram_read_lock);
}

static int dpram_lock_read(const char* func)
{    
    int lock_value;

    lock_value = atomic_inc_return(&dpram_read_lock);
    if(lock_value !=1)
        LOGE("(%s, lock) lock_value: %d\n", func, lock_value);
    wake_lock_timeout(&dpram_wake_lock, HZ*6);
    return 0;    
}

static void dpram_unlock_read(const char* func)
{
    int lock_value;

    lock_value = atomic_dec_return(&dpram_read_lock);

    if(lock_value !=0)
        LOGE("(%s, lock) lock_value: %d\n", func, lock_value);

    wake_unlock(&dpram_wake_lock);
}


#if defined(PRINT_DPRAM_WRITE) || defined(PRINT_DPRAM_READ)
static void dpram_print_packet(char *buf, int len)
{
    int i;

#ifndef DISABLE_IPC_DEBUG

    int plen;
    unsigned int hdlc_len, ipc_len;
    unsigned char *param;
    dpram_fmt_frame dff;
    dpram_fmt_frame *frame;

    memcpy(&dff, buf, len);
    hdlc_len = ((unsigned int) (dff.hdlc_length[1]) << 8)
            + (unsigned int) (dff.hdlc_length[0]);
    ipc_len = ((unsigned int) (dff.ipc_length[1]) << 8)
            + (unsigned int) (dff.ipc_length[0]);
    frame = &dff;
    param = dff.parameter;
    plen = (int) ipc_len - 7;
    if (plen > 1024)
        plen = 1024;

    LOGA("==================================================\n");
    LOGA("            View DPRAM formatted frame\n");
    LOGA("--------------------------------------------------\n");
    LOGA("HDLC Length  = %d\n", hdlc_len);
    LOGA("HDLC Control = 0x%02X\n", dff.hdlc_control);
    LOGA("IPC Length  = %d\n", ipc_len);
    LOGA("IPC Msg Seq  = 0x%02X\n", dff.msg_seq);
    LOGA("IPC Ack Seq  = 0x%02X\n", dff.ack_seq);
    LOGA("IPC Main Cmd = 0x%02X\n", dff.main_cmd);
    LOGA("IPC Sub Cmd  = 0x%02X\n", dff.sub_cmd);
    LOGA("IPC Cmd Type = 0x%02X\n", dff.cmd_type);

    if(frame->main_cmd == IPC_PWR_CMD) LOGA("mainCMD:IPC_PWR_CMD = 1,\n");
    if(frame->main_cmd == IPC_CALL_CMD) LOGA("mainCMD:IPC_CALL_CMD,\n");
    if(frame->main_cmd == IPC_DATA_CMD) LOGA("mainCMD:IPC_DATA_CMD,\n");
    if(frame->main_cmd == IPC_SMS_CMD) LOGA("mainCMD:IPC_SMS_CMD,\n");
    if(frame->main_cmd == IPC_SEC_CMD) LOGA("mainCMD:IPC_SEC_CMD,\n");
    if(frame->main_cmd == IPC_PB_CMD) LOGA("mainCMD:IPC_PB_CMD,\n");
    if(frame->main_cmd == IPC_DISPLAY_CMD) LOGA("mainCMD:IPC_DISPLAY_CMD,\n");
    if(frame->main_cmd == IPC_NET_CMD) LOGA("mainCMD:IPC_NET_CMD,\n");
    if(frame->main_cmd == IPC_SND_CMD) LOGA("mainCMD:IPC_SND_CMD,\n");
    if(frame->main_cmd == IPC_MISC_CMD) LOGA("mainCMD:IPC_MISC_CMD,\n");
    if(frame->main_cmd == IPC_SVC_CMD) LOGA("mainCMD:IPC_SVC_CMD,\n");
    if(frame->main_cmd == IPC_SS_CMD) LOGA("mainCMD:IPC_SS_CMD,\n");
    if(frame->main_cmd == IPC_DATA_GPRS_CMD) LOGA("mainCMD:IPC_DATA_GPRS_CMD,\n");
    if(frame->main_cmd == IPC_GEN_RESP) LOGA("mainCMD:IPC_GEN_RESP = 0x80          //General Response\n");
    if(frame->main_cmd == IPC_LTE_DM_CMD) LOGA("mainCMD:IPC_LTE_DM_CMD = 0xA0,\n");
    if(frame->main_cmd == IPC_LTE_CT_CMD) LOGA("mainCMD:IPC_LTE_CT_CMD,              // 0xA1\n");
    if(frame->main_cmd == IPC_LTE_CM_CMD) LOGA("mainCMD:IPC_LTE_CM_CMD,              // 0xA2\n");
    if(frame->main_cmd == IPC_LTE_MTM_CMD) LOGA("mainCMD:IPC_LTE_MTM_CMD,             // 0xA3\n");
    if(frame->main_cmd == IPC_LTE_FOTA_CMD) LOGA("mainCMD:IPC_LTE_FOTA_CMD             // 0xA4\n");

    //        if (frame->main_cmd == IPC_LTE_CM_CMD)
    //   {
    switch (frame->sub_cmd) {
    case CM_SUBCMD_SIM_STATUS:
        LOGA("CM_SUBCMD_SIM_STATUS\n");
        break;

    case CM_SUBCMD_IMEI:
        LOGA("CM_SUBCMD_IMEI\n");
        break;

    case CM_SUBCMD_SS_STATE:
        LOGA("CM_SUBCMD_SS_STATE\n");
        break;

    case CM_SUBCMD_MM_SERVICE_STATE:
        LOGA("CM_SUBCMD_MM_SERVICE_STATE\n");
        break;

    case CM_SUBCMD_ATTACH:
        LOGA("CM_SUBCMD_ATTACH\n");
        break;

    case CM_SUBCMD_IP_INFORMATION:
        LOGA("CM_SUBCMD_IP_INFORMATION\n");
        break;

    case CM_SUBCMD_DRX:
        LOGA("CM_SUBCMD_DRX\n");
        break;

    case CM_SUBCMD_SIGNAL_LEVEL:
        LOGA("CM_SUBCMD_SIGNAL_LEVEL\n");
        break;

    case CM_SUBCMD_SW_VERSION:
        LOGA("CM_SUBCMD_SW_VERSION\n");
        break;

    case CM_SUBCMD_HW_VERSION:
        LOGA("CM_SUBCMD_HW_VERSION\n");
        break;

    case CM_SUBCMD_PIN_ACTION:
        LOGA("CM_SUBCMD_PIN_ACTION\n");
        break;

    case CM_SUBCMD_MODEM_INTERFACE_TYPE:
        LOGA("CM_SUBCMD_MODEM_INTERFACE_TYPE\n");
        break;

    case CM_SUBCMD_PHONE_DATA_TEST_MODE:
        LOGA("CM_SUBCMD_PHONE_DATA_TEST_MODE\n");
        break;

    case CM_SUBCMD_UART_PORT_TYPE:
        LOGA("CM_SUBCMD_UART_PORT_TYPE\n");
        break;

    case CM_SUBCMD_PORTS_RETURN:
        LOGA("CM_SUBCMD_PORTS_RETURN\n");
        break;

    case CM_SUBCMD_TEST_NV_INFO:
        if (frame->parameter[0] == 0)
        {
            LOGA("\n");
        }
        else if (frame->parameter[0] == 1)
        {
            LOGA("[CM] TEST NV Status == Pass\n");
        }
        else if (frame->parameter[0] == 2)
        {
            LOGA("[CM] TEST NV Status == Fail\n");
        }
        else
        {
            LOGA("[CM] TEST NV Status == Unknown\n");
        }

    break;

    default:
        LOGA("[CM] sub_cmd = 0x%02X, cmd_type = 0x%02X\n", frame->sub_cmd, frame->cmd_type);
    break;
}
//   } //if main command


//Too many logs here        for (i = 0; i < plen; i++) LOGA("IPC Param[%02d] = 0x%02X\n", i, param[i]);
        LOGA("==================================================\n");
#endif /*!DISABLE_IPC_DEBUG*/


    LOGA("len = %d\n", len);

    print_buff[0] = '\0';

    for (i = 0; i < len; i++)
    {
        sprintf((print_buff + (i%16)*3), "%02x \0", *((char *)buf + i));
        if ( (i%16) == 15 )
        {
            LOGA("%s\n", print_buff);
            print_buff[0] = '\0';
        }
    }

    if ( strlen(print_buff) )
    {
        LOGA("%s\n", print_buff);
    }
}
#endif /*PRINT_DPRAM_WRITE || PRINT_DPRAM_READ*/

static int dpram_write(dpram_device_t *device, const unsigned char *buf, int len)
{
    int retval = 0;
    int size = 0;
    u16 head, tail, magic, access ;
    u16 irq_mask = 0;

#ifdef PRINT_DPRAM_WRITE
    PRINT_FUNC();
    dpram_print_packet(buf, len);
#endif

    //  If the phone is down, let's reset everything and fail the write.
#ifdef CDMA_IPC_C210_IDPRAM
    magic = ioread16((void *)(DPRAM_VBASE + DPRAM_MAGIC_CODE_ADDRESS));
    access = ioread16((void *)(DPRAM_VBASE + DPRAM_ACCESS_ENABLE_ADDRESS));
#else
    READ_FROM_DPRAM_VERIFY(&magic, DPRAM_MAGIC_CODE_ADDRESS, sizeof(magic));
    READ_FROM_DPRAM_VERIFY(&access, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(access));
#endif

    if (g_phone_sync != 1 || !access || magic != 0xAA)
    {
        LOGE("Phone has not booted yet!!! (phone_sync = %d)\n", g_phone_sync);
        return -EFAULT;
    }

    if ( dpram_lock_write(__func__) < 0 )
    {
        return -EAGAIN;
    }

#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + device->out_head_addr));
    tail = ioread16((void *)(DPRAM_VBASE + device->out_tail_addr));
#else
    READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));
#endif

    // Make sure the queue tail pointer is valid. 
    if( tail >= device->out_buff_size || head >= device->out_buff_size )
    {
        head = tail = 0;
#ifdef CDMA_IPC_C210_IDPRAM
        iowrite16(head, (void *)(DPRAM_VBASE + device->out_head_addr));
        iowrite16(tail, (void *)(DPRAM_VBASE + device->out_tail_addr));
#else
        WRITE_TO_DPRAM_VERIFY(device->out_head_addr, &head, sizeof(head));
        WRITE_TO_DPRAM_VERIFY(device->out_tail_addr, &tail, sizeof(tail));
#endif
        return 0 ;
    }

#ifdef PRINT_DPRAM_WRITE
    LOGA("head: %d, tail: %d\n", head, tail);
#endif

    // +++++++++ head ---------- tail ++++++++++ //
    if (head < tail) {
        size = tail - head - 1;
        size = (len > size) ? size : len;
        WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
        retval = size;
    }

    // tail +++++++++++++++ head --------------- //
    else if (tail == 0) {
        size = device->out_buff_size - head - 1;
        size = (len > size) ? size : len;
        WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
        retval = size;
    }

    // ------ tail +++++++++++ head ------------ //
    else {
        size = device->out_buff_size - head;
        size = (len > size) ? size : len;
        WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
        retval = size;

        if (len > retval) {
            size = (len - retval > tail - 1) ? tail - 1 : len - retval;
            WRITE_TO_DPRAM(device->out_buff_addr, buf + retval, size);
            retval += size;
        }
    }

    /* @LDK@ calculate new head */
    head = (u16)((head + retval) % device->out_buff_size);
#ifdef CDMA_IPC_C210_IDPRAM
    iowrite16(head, (void *)(DPRAM_VBASE + device->out_head_addr));
#else
    WRITE_TO_DPRAM_VERIFY(device->out_head_addr, &head, sizeof(head));
#endif
    device->out_head_saved = head;
    device->out_tail_saved = tail;

    /* @LDK@ send interrupt to the phone, if.. */
    irq_mask = INT_MASK_VALID;

    if (retval > 0)
        irq_mask |= device->mask_send;

    if (len > retval)
        irq_mask |= device->mask_req_ack;

    dpram_unlock_write(__func__);
    dpram_send_mbx_BA(irq_mask);

    return retval;
}

static inline int dpram_tty_insert_data(dpram_device_t *device, const u8 *psrc, u16 size)
{
#define CLUSTER_SEGMENT    1500

    u16 copied_size = 0;
    int retval = 0;
    
#ifdef PRINT_DPRAM_READ
    dpram_print_packet(psrc, size);
#endif

    if ( size > CLUSTER_SEGMENT && (device->serial.tty->index == 1) )
    {
        while (size)
        {
            copied_size = (size > CLUSTER_SEGMENT) ? CLUSTER_SEGMENT : size;
            tty_insert_flip_string(device->serial.tty, psrc + retval, copied_size);

            size -= copied_size;
            retval += copied_size;
        }

        return retval;
    }

    return tty_insert_flip_string(device->serial.tty, psrc, size);
}

static int dpram_read_fmt(dpram_device_t *device, const u16 non_cmd)
{
    int retval = 0;
    int retval_add = 0;
    int size = 0;
    u16 head, tail;

    dpram_lock_read(__func__);

#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + device->in_head_addr));
    tail = ioread16((void *)(DPRAM_VBASE + device->in_tail_addr));
#else
    READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));
#endif

#ifdef PRINT_DPRAM_READ
    LOGA("head: %d, tail: %d\n", head, tail);
#endif

    if (head != tail)
    {
        u16 up_tail = 0;

        // ------- tail ########## head -------- //
        if (head > tail)
        {
            size = head - tail;
            retval = dpram_tty_insert_data(device, (u8 *)(DPRAM_VBASE + (device->in_buff_addr + tail)), size);
            if (size != retval)
                LOGE("size: %d, retval: %d\n", size, retval);
        }
        // ####### head ------------ tail ###### //
        else
        {
            int tmp_size = 0;

            // Total Size.
            size = device->in_buff_size - tail + head;

            // 1. tail -> buffer end.
            tmp_size = device->in_buff_size - tail;
            retval = dpram_tty_insert_data(device, (u8 *)(DPRAM_VBASE + (device->in_buff_addr + tail)), tmp_size);
            if (tmp_size != retval)
            {
                LOGE("size: %d, retval: %d\n", size, retval);
            }

            // 2. buffer start -> head.
            if (size > tmp_size)
            {
                retval_add = dpram_tty_insert_data(device, (u8 *)(DPRAM_VBASE + device->in_buff_addr), size - tmp_size);
                retval += retval_add;
        
                if((size - tmp_size)!= retval_add)
                {
                    LOGE("size - tmp_size: %d, retval_add: %d\n", size - tmp_size, retval_add);
                }
            }
        }

        /* new tail */
        up_tail = (u16)((tail + retval) % device->in_buff_size);
#ifdef CDMA_IPC_C210_IDPRAM
        iowrite16(up_tail, (void *)(DPRAM_VBASE + device->in_tail_addr));
#else
        WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &up_tail, sizeof(up_tail));
#endif
    }
        

    device->in_head_saved = head;
    device->in_tail_saved = tail;

    dpram_unlock_read(__func__);

    if ( atomic_read(&fmt_txq_req_ack_rcvd) > 0 || (non_cmd & device->mask_req_ack) )
    {
        // there is a situation where the q become full after we reached the tasklet.
        // so this logic will allow us to send the RES_ACK as soon as we read 1 packet and CP get a chance to
        // write another buffer.
//        LOGA("Sending INT_MASK_RES_ACK_F\n");
        dpram_send_mbx_BA(INT_NON_COMMAND(device->mask_res_ack));
        atomic_set(&fmt_txq_req_ack_rcvd, 0);
    }

    return retval;
    
}

static int dpram_read_raw(dpram_device_t *device, const u16 non_cmd)
{
    int retval = 0;
    int size = 0;
    u16 head, tail;
    u16 up_tail = 0;
    int ret;
    size_t len;
    struct pdp_info *dev = NULL;
    struct pdp_hdr hdr;
    u16 read_offset;
    u8 len_high, len_low, id, control;
    u16 pre_data_size; //pre_hdr_size,
    u8 ch;

    dpram_lock_read(__func__);

#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + device->in_head_addr));
    tail = ioread16((void *)(DPRAM_VBASE + device->in_tail_addr));
#else
    READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));
#endif

#ifdef PRINT_DPRAM_HEAD_TAIL
    LOGA("head: %d, tail: %d\n", head, tail);
#endif

    if (head != tail)
    {
        up_tail = 0;

        if (head > tail)
            /* ----- (tail) 7f 00 00 7e (head) ----- */
            size = head - tail; 
        else
            /* 00 7e (head) ----------- (tail) 7f 00 */
            size = device->in_buff_size - tail + head;

        read_offset = 0;
#ifdef PRINT_DPRAM_HEAD_TAIL
        LOGA("head: %d, tail: %d, size: %d\n", head, tail, size);
#endif
        while (size)
        {            
            READ_FROM_DPRAM(&ch, device->in_buff_addr +((u16)(tail + read_offset) % device->in_buff_size), sizeof(ch));

            if (ch == 0x7F)
            {
                read_offset ++;
            }
            else
            {
                LOGE("First byte: 0x%02x, drop bytes: %d, buff addr: 0x%08x, read addr: 0x%08x\n", 
                     ch, size, (device->in_buff_addr), (device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size)));

                dpram_drop_data(device);
                dpram_unlock_read(__func__);
                return -1;
            }

            len_high = len_low = id = control = 0;
            READ_FROM_DPRAM(&len_low, device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size) ,sizeof(len_high));
            read_offset ++;
            READ_FROM_DPRAM(&len_high, device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size) ,sizeof(len_low));
            read_offset ++;
            READ_FROM_DPRAM(&id, device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size) ,sizeof(id));
            read_offset ++;
            READ_FROM_DPRAM(&control, device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size) ,sizeof(control));
            read_offset ++;

            hdr.len = len_high <<8 | len_low;
            hdr.id = id;
            hdr.control = control;
    
            len = hdr.len - sizeof(struct pdp_hdr);    
            if (len <= 0)
            {
                LOGE("oops... read_offset: %d, len: %d, hdr.id: %d\n", read_offset, len, hdr.id);
                dpram_drop_data(device);
                dpram_unlock_read(__func__);
                return -1;
            }

            dev = pdp_get_dev(hdr.id);
#ifdef PRINT_DPRAM_READ
            LOGA("read_offset: %d, len: %d, hdr.id: %d\n", read_offset, len, hdr.id);
#endif
            if (!dev)
            {
                LOGE("RAW READ Failed.. NULL dev detected \n");
                check_pdp_table(__func__, __LINE__);
                dpram_drop_data(device);
                dpram_unlock_read(__func__);
                return -1;
            }
                
            if (dev->vs_dev.tty != NULL && dev->vs_dev.refcount)
            {
                if((u16)(tail + read_offset) % device->in_buff_size + len < device->in_buff_size)
                {
                    ret = tty_insert_flip_string(dev->vs_dev.tty, (u8 *)(DPRAM_VBASE + (device->in_buff_addr + (u16)(tail + read_offset) % device->in_buff_size)), len);
		    dev->vs_dev.tty->low_latency = 0;
                    tty_flip_buffer_push(dev->vs_dev.tty);
                }
                else
                {
                    pre_data_size = device->in_buff_size - (tail + read_offset); 
                    ret = tty_insert_flip_string(dev->vs_dev.tty, (u8 *)(DPRAM_VBASE + (device->in_buff_addr + tail + read_offset)), pre_data_size);
                    ret += tty_insert_flip_string(dev->vs_dev.tty, (u8 *)(DPRAM_VBASE + (device->in_buff_addr)),len - pre_data_size);
		    dev->vs_dev.tty->low_latency = 0;
                    tty_flip_buffer_push(dev->vs_dev.tty);
                    LOGE("RAW pre_data_size: %d, len-pre_data_size: %d, ret: %d\n", pre_data_size, len- pre_data_size, ret);
                }
            }
            else
            {
                LOGE("tty channel(id:%d) is not opened!!!\n", dev->id);
                ret = len;
            }
            
            if (!ret)
            {
                LOGE("(tty_insert_flip_string) drop bytes: %d, buff addr: 0x%08x\n, read addr: 0x%08x\n", 
                     size, (device->in_buff_addr), (device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size)));
                dpram_drop_data(device);
                dpram_unlock_read(__func__);
                return -1;
            }
            
            read_offset += ret;
#ifdef PRINT_DPRAM_READ
            LOGA("read_offset: %d, ret: %d\n", read_offset, ret);
#endif
            READ_FROM_DPRAM(&ch, (device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size)), sizeof(ch));
            if (ch == 0x7e)
            {
                read_offset++;
            }
            else
            {
                LOGE("Last byte: 0x%02x, drop bytes: %d, buff addr: 0x%08x, read addr: 0x%08x\n",
                      ch, size, (device->in_buff_addr), (device->in_buff_addr + ((u16)(tail + read_offset) % device->in_buff_size)) );
                dpram_drop_data(device);
                dpram_unlock_read(__func__);
                return -1;
            }

            size -= (ret + sizeof(struct pdp_hdr) + 2);
            retval += (ret + sizeof(struct pdp_hdr) + 2);

            if (size < 0)
            {
                LOGE("something wrong....\n");
                break;
            }

        }

        up_tail = (u16)((tail + read_offset) % device->in_buff_size);
#ifdef CDMA_IPC_C210_IDPRAM
        iowrite16(up_tail, (void *)(DPRAM_VBASE + device->in_tail_addr));
#else
        WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &up_tail, sizeof(up_tail));
#endif
    }

    device->in_head_saved = head;
    device->in_tail_saved = tail;

    dpram_unlock_read(__func__);

    if ( atomic_read(&raw_txq_req_ack_rcvd) > 0 || (non_cmd & device->mask_req_ack) )
    {
        // there is a situation where the q become full after we reached the tasklet.
        // so this logic will allow us to send the RES_ACK as soon as we read 1 packet and CP get a chance to
        // write another buffer.
//        LOGA("Sending INT_MASK_RES_ACK_R\n");
        dpram_send_mbx_BA(INT_NON_COMMAND(device->mask_res_ack));
        atomic_set(&raw_txq_req_ack_rcvd, 0);
    }

    return retval;
}

#ifdef _ENABLE_ERROR_DEVICE
void request_phone_reset(void)
{
    char buf[DPRAM_ERR_MSG_LEN];
    unsigned long flags;

    memset((void *)buf, 0, sizeof (buf));

    LOGE("CDMA reset cnt = %d\n", g_cp_reset_cnt);
    if (g_cp_reset_cnt > 5)
    {
        buf[0] = '1';
        buf[1] = ' ';
        memcpy(buf+2, "$CDMA-DEAD", sizeof("$CDMA-DEAD"));
    }
    else
    {
        buf[0] = '8';
        buf[1] = ' ';
        memcpy(buf+2, "$PHONE-OFF", sizeof("$PHONE-OFF"));
    }

    LOGE("[PHONE ERROR] ->> %s\n", buf);
    local_irq_save(flags);
    memcpy(dpram_err_buf, buf, DPRAM_ERR_MSG_LEN);
    dpram_err_len = 64;
    local_irq_restore(flags);
    
    wake_up_interruptible(&dpram_err_wait_q);
    kill_fasync(&dpram_err_async_q, SIGIO, POLL_IN);
}
#endif


static void dpram_send_mbx_BA(u16 irq_mask)
{
    if (g_dump_on)
        return;

#ifdef CDMA_IPC_C210_IDPRAM
    iowrite16(irq_mask, (void *)dpram_mbx_BA);
#else
    *dpram_mbx_BA = irq_mask;
#endif

#ifdef PRINT_DPRAM_WRITE
    LOG("mbx_BA = 0x%04X\n", irq_mask);
#endif
}


/*
 * dpram_send_mbx_BA_cmd()
 * - for prevent CP interrupt command miss issue,
 *  below function check the CP dpram interrupt level before send interrupt
 */
#define DPRAM_CMD_SEND_RETRY_CNT 0x5
static void dpram_send_mbx_BA_cmd(u16 irq_mask)
{
    int retry_cnt = DPRAM_CMD_SEND_RETRY_CNT;

    if (g_dump_on)
        return;

    while (gpio_get_value(GPIO_DPRAM_INT_CP_N) == 0 && retry_cnt--)
    {
        msleep(1);
        LOGE("send cmd intr, retry cnt = %d\n", (DPRAM_CMD_SEND_RETRY_CNT-retry_cnt));
    }

#ifdef CDMA_IPC_C210_IDPRAM
    iowrite16(irq_mask, (void *)dpram_mbx_BA);
#else
    *dpram_mbx_BA = irq_mask;
#endif

#ifdef PRINT_DPRAM_WRITE
    LOGA("mbx_BA = 0x%04X\n", irq_mask);
#endif
}


static void dpram_clear(void)
{
    long i = 0;
    long size = 0;
    unsigned long flags;
    
    u16 value = 0;

    size = DPRAM_SIZE - (DPRAM_INTERRUPT_PORT_SIZE * 4);

    /* @LDK@ clear DPRAM except interrupt area */
    local_irq_save(flags);

    for (i = DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS; i < size; i += 2)
    {
        iowrite16(0, (void *)(DPRAM_VBASE + i));
    }

    local_irq_restore(flags);

    value = ioread16((void *)dpram_mbx_AB);
}

static int dpram_init_and_report(void)
{
    const u16 init_start = INT_COMMAND(MBX_CMD_INIT_START);
    const u16 init_end = INT_COMMAND(MBX_CMD_INIT_END | AP_PLATFORM_ANDROID);
    const u16 magic_code = 0x00AA;
    u16       acc_code = 0;

#ifdef DEBUG_DPRAM_INT_HANDLER
    PRINT_FUNC();
#endif

    dpram_send_mbx_BA(init_start);

    if ( dpram_lock_write(__func__) < 0 )
        return -EAGAIN;

    /* @LDK@ write DPRAM disable code */
    iowrite16(acc_code, (void *)(DPRAM_VBASE + DPRAM_ACCESS_ENABLE_ADDRESS));

    /* @LDK@ dpram clear */
    dpram_clear();

    /* @LDK@ write DPRAM magic code & enable code */
    iowrite16(magic_code, (void *)(DPRAM_VBASE + DPRAM_MAGIC_CODE_ADDRESS));
    acc_code = 0x0001;
    iowrite16(acc_code, (void *)(DPRAM_VBASE + DPRAM_ACCESS_ENABLE_ADDRESS));

    /* @LDK@ send init end code to phone */
    dpram_unlock_write(__func__);

    dpram_send_mbx_BA(init_end);

#ifdef DEBUG_DPRAM_INT_HANDLER
    LOGA("Sent CMD_INIT_END(0x%04X) to Phone!!!\n", init_end);
#endif

    g_phone_sync = 1;
    g_cp_reset_cnt = 0;

    return 0;
}

static inline int dpram_get_read_available(dpram_device_t *device)
{
    u16 head = 0, tail = 0, size = 0;

#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + device->in_head_addr));
    tail = ioread16((void *)(DPRAM_VBASE + device->in_tail_addr));
#else
    READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));
#endif

    if ( tail >= device->in_buff_size || head >= device->in_buff_size )
    {
        return 0;
    }

    size = ( head >= tail )? (head - tail) : (device->in_buff_size - tail + head);

#ifdef PRINT_DPRAM_READ
    if ( size > 0 )
        LOGA("Data size = %d\n", size);
#endif

    return size;
}

static void dpram_drop_data(dpram_device_t *device)
{
    u16 head, tail;

#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + device->in_head_addr));
    tail = ioread16((void *)(DPRAM_VBASE + device->in_tail_addr));
#else
    READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));
#endif

    if( head >= device->in_buff_size || tail >= device->in_buff_size )
    {
        head = tail = 0 ;
#ifdef CDMA_IPC_C210_IDPRAM
        iowrite16(head, (void *)(DPRAM_VBASE + device->in_head_addr));
#else
        WRITE_TO_DPRAM_VERIFY(device->in_head_addr, &head, sizeof(head));
#endif
    }

#ifdef CDMA_IPC_C210_IDPRAM
    iowrite16(head, (void *)(DPRAM_VBASE + device->in_tail_addr));
#else
    WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &head, sizeof(head));
#endif

    LOGA("DROP head: %d, tail: %d\n", head, tail);
}


static void dpram_phone_power_on(void)
{

    PRINT_FUNC();

    gpio_direction_output(GPIO_QSC_PHONE_RST, GPIO_LEVEL_HIGH);
    gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);
    msleep(100);
    
    gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_HIGH);
    msleep(400);
    msleep(400);
    msleep(200);
    gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);
}


static void dpram_phone_power_off(void)
{
    g_phone_power_off_sequence = 1;
    int phone_wait_cnt = 0;

    LOGA("Set g_phone_power_off_sequence flags!!!\n");
    
    gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);
	
// confirm phone off
	while (1) {
  		if (gpio_get_value(GPIO_QSC_PHONE_ACTIVE)) {
    		printk(KERN_ALERT"%s: Try to Turn Phone Off by CP_RST\n",__func__);
   		gpio_set_value(GPIO_QSC_PHONE_RST, 0);
   		if (phone_wait_cnt > 1) {
    		printk(KERN_ALERT"%s: PHONE OFF Failed\n",__func__);
    		break;
   		}
   		phone_wait_cnt++;
   		mdelay(1000);
  		} 
		else {
   			printk(KERN_ALERT"%s: PHONE OFF Success\n", __func__);
   			break;
  		     }
 		}
	
}

static int dpram_phone_getstatus(void)
{
    return gpio_get_value(GPIO_QSC_PHONE_ACTIVE);
}

static void dpram_phone_reset(void)
{
    PRINT_FUNC();

    gpio_set_value(GPIO_QSC_PHONE_RST, GPIO_LEVEL_LOW);
    //mdelay(100);
    msleep(100);
    gpio_set_value(GPIO_QSC_PHONE_RST, GPIO_LEVEL_HIGH);

    g_cp_reset_cnt++;
}

static int dpram_extra_mem_rw(struct _param_em *param)
{

    if(param->offset + param->size > 0xFFF800)
    {
        LOGE("Wrong rage of external memory access\n");
        return -1;
    }

    if (param->rw)
    {    //write
        if(dpram_lock_write(__func__) < 0)
            return -EAGAIN;
        WRITE_TO_DPRAM(param->offset, param->addr, param->size);
        dpram_unlock_write(__func__);
    }
    else
    {    //read
        dpram_lock_read(__func__);
        READ_FROM_DPRAM(param->addr, param->offset, param->size);
        dpram_unlock_read(__func__);
    }
    return 0;
}

static int dpram_qsc_timeout_handler(void)
{
    const u16 rdump_flag1 = 0xdead;
    const u16 rdump_flag2 = 0xdead;
    const u16 temp1, temp2;

#ifdef CONFIG_KERNEL_DEBUG_SEC    
    t_kernel_sec_mmu_info mmu_info; 
    
    LOGA("Ram Dump ON.\n");

    WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS,    &rdump_flag1, sizeof(rdump_flag1));
    WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &rdump_flag2, sizeof(rdump_flag2));

    READ_FROM_DPRAM((void *)&temp1, DPRAM_MAGIC_CODE_ADDRESS, sizeof(temp1));
    READ_FROM_DPRAM((void *)&temp2, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(temp2));
    LOGE(KERN_ERR "flag1: %x flag2: %x\n", temp1, temp2);

    g_dump_on = 1;

    // If it is configured to dump both AP and CP, reset both AP and CP here.
    LOGA("Configure to restart AP and collect dump on restart...\n");
    kernel_sec_set_cause_strptr("QSC_Timeout", 12);
    kernel_sec_cdma_dpram_dump();
    kernel_sec_set_upload_magic_number();
    kernel_sec_get_mmu_reg_dump(&mmu_info);
    kernel_sec_set_upload_cause(UPLOAD_CAUSE_CDMA_TIMEOUT);
    kernel_sec_hw_reset(false);
#endif

    return 0;
}


static int dpram_phone_ramdump_on(void)
{
    const u16 rdump_flag1 = 0xdead;
    const u16 rdump_flag2 = 0xdead;
    const u16 temp1, temp2;
    
    LOGL(DL_INFO,"Ramdump ON.\n");
    if(dpram_lock_write(__func__) < 0)
        return -EAGAIN;

    WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS,    &rdump_flag1, sizeof(rdump_flag1));
    WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &rdump_flag2, sizeof(rdump_flag2));

    READ_FROM_DPRAM((void *)&temp1, DPRAM_MAGIC_CODE_ADDRESS, sizeof(temp1));
    READ_FROM_DPRAM((void *)&temp2, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(temp2));
    LOGL(DL_INFO,"flag1: %x flag2: %x\n", temp1, temp2);

    /* @LDK@ send init end code to phone */
    dpram_unlock_write(__func__);

    g_dump_on = 1;
    // If it is configured to dump both AP and CP, reset both AP and CP here.
    kernel_sec_dump_cp_handle2();
    
    return 0;

}

static int dpram_phone_ramdump_off(void)
{
    const u16 rdump_flag1 = 0x00aa;
    const u16 rdump_flag2 = 0x0001;

    LOGL(DL_INFO, "Ramdump OFF.\n");
    
    g_dump_on = 0;
    if(dpram_lock_write(__func__) < 0)
        return -EAGAIN;

    WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS,    &rdump_flag1, sizeof(rdump_flag1));
    WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &rdump_flag2, sizeof(rdump_flag2));
    /* @LDK@ send init end code to phone */
    dpram_unlock_write(__func__);

    //usb_switch_mode(1); #############Need to be enabled later
    
    g_phone_sync = 0;

    dpram_phone_reset();
    return 0;

}

#ifdef CONFIG_PROC_FS
static int dpram_read_proc(char *page, char **start, off_t off,
        int count, int *eof, void *data)
{
    char *p = page;
    int len;

    u16 magic, enable;
    u16 fmt_in_head, fmt_in_tail, fmt_out_head, fmt_out_tail;
    u16 raw_in_head, raw_in_tail, raw_out_head, raw_out_tail;
    u16 in_interrupt = 0, out_interrupt = 0;

    int fih, fit, foh, fot;
    int rih, rit, roh, rot;

#ifdef _ENABLE_ERROR_DEVICE
    char buf[DPRAM_ERR_MSG_LEN];
    unsigned long flags;
#endif    /* _ENABLE_ERROR_DEVICE */

    READ_FROM_DPRAM((void *)&magic, DPRAM_MAGIC_CODE_ADDRESS, sizeof(magic));
    READ_FROM_DPRAM((void *)&enable, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(enable));
    READ_FROM_DPRAM((void *)&fmt_in_head, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS, sizeof(fmt_in_head));
    READ_FROM_DPRAM((void *)&fmt_in_tail, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS, sizeof(fmt_in_tail));
    READ_FROM_DPRAM((void *)&fmt_out_head, DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS, sizeof(fmt_out_head));
    READ_FROM_DPRAM((void *)&fmt_out_tail, DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS, sizeof(fmt_out_tail));
    READ_FROM_DPRAM((void *)&raw_in_head, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS, sizeof(raw_in_head));
    READ_FROM_DPRAM((void *)&raw_in_tail, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS, sizeof(raw_in_tail));
    READ_FROM_DPRAM((void *)&raw_out_head, DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS, sizeof(raw_out_head));
    READ_FROM_DPRAM((void *)&raw_out_tail, DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS, sizeof(raw_out_tail));

    fih = dpram_table[FORMATTED_INDEX].in_head_saved;
    fit = dpram_table[FORMATTED_INDEX].in_tail_saved;
    foh = dpram_table[FORMATTED_INDEX].out_head_saved;
    fot = dpram_table[FORMATTED_INDEX].out_tail_saved;
    rih = dpram_table[RAW_INDEX].in_head_saved;
    rit = dpram_table[RAW_INDEX].in_tail_saved;
    roh = dpram_table[RAW_INDEX].out_head_saved;
    rot = dpram_table[RAW_INDEX].out_tail_saved;
    out_interrupt = ioread16((void *)dpram_mbx_BA);
    in_interrupt = ioread16((void *)dpram_mbx_AB);

#ifdef _ENABLE_ERROR_DEVICE
    memset((void *)buf, '\0', DPRAM_ERR_MSG_LEN);
    local_irq_save(flags);
    memcpy(buf, dpram_err_buf, DPRAM_ERR_MSG_LEN - 1);
    local_irq_restore(flags);
#endif    /* _ENABLE_ERROR_DEVICE */

    p += sprintf(p,
            "-------------------------------------\n"
            "| NAME\t\t\t| VALUE\n"
            "-------------------------------------\n"
            "|R MAGIC CODE\t\t| 0x%04x\n"
            "|R ENABLE CODE\t\t| 0x%04x\n"
            "|R PHONE->PDA FMT HEAD\t| %u\n"
            "|R PHONE->PDA FMT TAIL\t| %u\n"
            "|R PDA->PHONE FMT HEAD\t| %u\n"
            "|R PDA->PHONE FMT TAIL\t| %u\n"
            "|R PHONE->PDA RAW HEAD\t| %u\n"
            "|R RPHONE->PDA RAW TAIL\t| %u\n"
            "|R PDA->PHONE RAW HEAD\t| %u\n"
            "|R PDA->PHONE RAW TAIL\t| %u\n"
            "-------------------------------------\n"
            "| FMT PHONE->PDA HEAD\t| %d\n"
            "| FMT PHONE->PDA TAIL\t| %d\n"
            "| FMT PDA->PHONE HEAD\t| %d\n"
            "| FMT PDA->PHONE TAIL\t| %d\n"
            "-------------------------------------\n"
            "| RAW PHONE->PDA HEAD\t| %d\n"
            "| RAW PHONE->PDA TAIL\t| %d\n"
            "| RAW PDA->PHONE HEAD\t| %d\n"
            "| RAW PDA->PHONE TAIL\t| %d\n"
            "-------------------------------------\n"
            "| PHONE->PDA MAILBOX\t| 0x%04x\n"
            "| PDA->PHONE MAILBOX\t| 0x%04x\n"
            "-------------------------------------\n"
#ifdef _ENABLE_ERROR_DEVICE
            "| LAST PHONE ERR MSG\t| %s\n"
#endif    /* _ENABLE_ERROR_DEVICE */
            "| PHONE ACTIVE\t\t| %s\n"
            "| DPRAM INT Level\t| %d\n"
            "-------------------------------------\n",
            magic, enable,
            fmt_in_head, fmt_in_tail, fmt_out_head, fmt_out_tail,
            raw_in_head, raw_in_tail, raw_out_head, raw_out_tail,
            fih, fit, foh, fot, 
            rih, rit, roh, rot,
            in_interrupt, out_interrupt,

#ifdef _ENABLE_ERROR_DEVICE
            (buf[0] != '\0' ? buf : "NONE"),
#endif    /* _ENABLE_ERROR_DEVICE */
            (dpram_phone_getstatus() ? "ACTIVE" : "INACTIVE"),
                gpio_get_value(IRQ_QSC_PHONE_ACTIVE)
        );

    len = (p - page) - off;
    if (len < 0) {
        len = 0;
    }

    *eof = (len <= count) ? 1 : 0;
    *start = page + off;

    return len;
}
#endif /* CONFIG_PROC_FS */

/* dpram tty file operations. */
static int dpram_tty_open(struct tty_struct *tty, struct file *file)
{
    dpram_device_t *device = &dpram_table[tty->index];

    device->serial.tty = tty;
    device->serial.open_count++;

    if (device->serial.open_count > 1)
    {
        device->serial.open_count--;
        return -EBUSY;
    }

    tty->driver_data = (void *)device;
    tty->low_latency = 1;
    return 0;
}

static void dpram_tty_close(struct tty_struct *tty, struct file *file)
{
    dpram_device_t *device = (dpram_device_t *)tty->driver_data;

    if ( device && (device == &dpram_table[tty->index]) )
    {
        down(&device->serial.sem);
        device->serial.open_count--;
        device->serial.tty = NULL;
        up(&device->serial.sem);
    }
}

static int dpram_tty_write(struct tty_struct *tty, const unsigned char *buffer, int count)
{
    dpram_device_t *device = (dpram_device_t *)tty->driver_data;

    if (!device)
        return 0;

    return dpram_write(device, buffer, count);
}

static int dpram_tty_write_room(struct tty_struct *tty)
{
    int avail;
    u16 head, tail;

    dpram_device_t *device = (dpram_device_t *)tty->driver_data;

    if (device != NULL)
    {
        head = device->out_head_saved;
        tail = device->out_tail_saved;

        avail = (head < tail) ? (tail - head - 1) : (device->out_buff_size + tail - head - 1);

        return avail;
    }

    return 0;
}

static int dpram_tty_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int val;

    switch (cmd)
    {
        case DPRAM_PHONE_ON:
            g_phone_sync = 0;
            g_dump_on = 0;
            g_phone_power_off_sequence = 0;
            LOGA("IOCTL cmd = DPRAM_PHONE_ON\n");
            dpram_phone_power_on();
            return 0;

        case DPRAM_PHONE_GETSTATUS:
            LOGA("IOCTL cmd = DPRAM_PHONE_GETSTATUS\n");
            val = dpram_phone_getstatus();
            return copy_to_user((unsigned int *)arg, &val, sizeof(val));

        case DPRAM_PHONE_RESET:
            g_phone_sync = 0;
            g_phone_power_off_sequence = 0;
            LOGA("IOCTL cmd = DPRAM_PHONE_RESET\n");
            dpram_phone_reset();
            return 0;

        case DPRAM_PHONE_OFF:
            g_phone_sync = 0;
            g_phone_power_off_sequence = 1;
            LOGA("IOCTL cmd = DPRAM_PHONE_OFF\n");
            dpram_phone_power_off();
            return 0;

        // Silent reset
        case MBX_CMD_PHONE_RESET:
            LOGA("IOCTL cmd = MBX_CMD_PHONE_RESET\n");
            request_phone_reset();
            return 0;

        case DPRAM_PHONE_RAMDUMP_ON:
            LOGA("IOCTL cmd = DPRAM_PHONE_RAMDUMP_ON\n");
            dpram_phone_ramdump_on();
            return 0;

        case DPRAM_PHONE_RAMDUMP_OFF:
            LOGA("IOCTL cmd = DPRAM_PHONE_RAMDUMP_OFF\n");
            dpram_phone_ramdump_off();
            return 0;

        case DPRAM_PHONE_UNSET_UPLOAD:
            LOGA("IOCTL cmd = DPRAM_PHONE_UNSET_UPLOAD\n");
#ifdef CONFIG_KERNEL_DEBUG_SEC    
            kernel_sec_clear_upload_magic_number();
#endif
            break;

        case DPRAM_PHONE_SET_AUTOTEST:
            LOGA("IOCTL cmd = DPRAM_PHONE_SET_AUTOTEST\n");
#ifdef CONFIG_KERNEL_DEBUG_SEC    
            kernel_sec_set_autotest();
#endif
            break;

        case DPRAM_PHONE_GET_DEBUGLEVEL:
            LOGA("IOCTL cmd = DPRAM_PHONE_GET_DEBUGLEVEL\n");
#ifdef CONFIG_KERNEL_DEBUG_SEC    
            switch(kernel_sec_get_debug_level())
            {
                case KERNEL_SEC_DEBUG_LEVEL_LOW:
                    val = 0xA0A0; 
                    break;
                case KERNEL_SEC_DEBUG_LEVEL_MID:
                    val = 0xB0B0;
                    break;
                case KERNEL_SEC_DEBUG_LEVEL_HIGH:
                    val = 0xC0C0;
                    break;
                default:
                    val = 0xFFFF;
                    break;
            }
            LOGA("DPRAM_PHONE_GET_DEBUGLEVEL = %x, %d\n", kernel_sec_get_debug_level(), val);
#endif            
            return copy_to_user((unsigned int *)arg, &val, sizeof(val));

            break;

        case DPRAM_PHONE_SET_DEBUGLEVEL:
            LOGA("IOCTL cmd = DPRAM_PHONE_SET_DEBUGLEVEL\n");
#ifdef CONFIG_KERNEL_DEBUG_SEC    
            switch(kernel_sec_get_debug_level())
            {
                case KERNEL_SEC_DEBUG_LEVEL_LOW:
                    kernel_sec_set_debug_level(KERNEL_SEC_DEBUG_LEVEL_MID);
                    break;
                case KERNEL_SEC_DEBUG_LEVEL_MID:
                    kernel_sec_set_debug_level(KERNEL_SEC_DEBUG_LEVEL_HIGH);
                    break;
                case KERNEL_SEC_DEBUG_LEVEL_HIGH:
                    kernel_sec_set_debug_level(KERNEL_SEC_DEBUG_LEVEL_LOW);
                    break;
                default:
                    break;
            }
#endif                    
            return 0;

        case DPRAM_EXTRA_MEM_RW:
        {
            struct _param_em param;

            LOGA("IOCTL cmd = DPRAM_EXTRA_MEM_RW\n");

            val = copy_from_user((void *)&param, (void *)arg, sizeof(param));
            if (dpram_extra_mem_rw(&param) < 0)
            {
                LOGE("External memory access fail..\n");
                return -1;
            }

            if (!param.rw)  //read
            {
                return copy_to_user((unsigned long *)arg, &param, sizeof(param));
            }

            return 0;
        }

        default:
            LOGA("IOCTL cmd = 0x%X\n", cmd);
            break;
    }

    return -ENOIOCTLCMD;
}

static int dpram_tty_chars_in_buffer(struct tty_struct *tty)
{
    int data;
    u16 head, tail;

    dpram_device_t *device = (dpram_device_t *)tty->driver_data;

    if (device != NULL)
    {
        head = device->out_head_saved;
        tail = device->out_tail_saved;

        data = (head > tail) ? (head - tail - 1) : (device->out_buff_size - tail + head);

        return data;
    }

    return 0;
}

#ifdef _ENABLE_ERROR_DEVICE
static int dpram_err_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
    DECLARE_WAITQUEUE(wait, current);

    unsigned long flags;
    ssize_t ret;
    size_t ncopy;

    add_wait_queue(&dpram_err_wait_q, &wait);
    set_current_state(TASK_INTERRUPTIBLE);

    while (1) {
        local_irq_save(flags);

        if (dpram_err_len) {
            ncopy = min(count, dpram_err_len);

            if (copy_to_user(buf, dpram_err_buf, ncopy)) {
                ret = -EFAULT;
            }

            else {
                ret = ncopy;
            }

            dpram_err_len = 0;
            
            local_irq_restore(flags);
            break;
        }

        local_irq_restore(flags);

        if (filp->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            break;
        }

        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            break;
        }

        schedule();
    }

    set_current_state(TASK_RUNNING);
    remove_wait_queue(&dpram_err_wait_q, &wait);

    return ret;
}

static int dpram_err_fasync(int fd, struct file *filp, int mode)
{
    return fasync_helper(fd, filp, mode, &dpram_err_async_q);
}

static unsigned int dpram_err_poll(struct file *filp,
        struct poll_table_struct *wait)
{
    poll_wait(filp, &dpram_err_wait_q, wait);
    return ((dpram_err_len) ? (POLLIN | POLLRDNORM) : 0);
}
#endif    /* _ENABLE_ERROR_DEVICE */

/* handlers. */
static void res_ack_tasklet_handler(unsigned long data)
{
    dpram_device_t *device = (dpram_device_t *)data;

    if (device && device->serial.tty) {
        struct tty_struct *tty = device->serial.tty;

        if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
                tty->ldisc->ops->write_wakeup) {
            (tty->ldisc->ops->write_wakeup)(tty);
        }

        wake_up_interruptible(&tty->write_wait);
    }
}

static void fmt_rcv_tasklet_handler(unsigned long data)
{
    dpram_tasklet_data_t *tasklet_data = (dpram_tasklet_data_t *)data;

    dpram_device_t *device = tasklet_data->device;
    u16 non_cmd = tasklet_data->non_cmd;

    int ret = 0;
    int cnt = 0;

    if (device && device->serial.tty)
    {
        struct tty_struct *tty = device->serial.tty;

        while (dpram_get_read_available(device))
        {
            ret = dpram_read_fmt(device, non_cmd);

            if (!ret)
                cnt++;

            if (cnt > 10)
            {
                dpram_drop_data(device);
                break;
            }

            if (ret < 0)
            {
                LOGE("FMT dpram_read_fmt failed\n");
                /* TODO: ... wrong.. */
            }
	    tty->low_latency = 0;
            tty_flip_buffer_push(tty);
        }
    }
    else
    {
        dpram_drop_data(device);
    }
}

static void raw_rcv_tasklet_handler(unsigned long data)
{
    dpram_tasklet_data_t *tasklet_data = (dpram_tasklet_data_t *)data;

    dpram_device_t *device = tasklet_data->device;
    u16 non_cmd = tasklet_data->non_cmd;

    int ret = 0;

    while ( dpram_get_read_available(device) )
    {
        ret = dpram_read_raw(device, non_cmd);

        if (ret < 0)
        {
            LOGE("RAW dpram_read_raw failed\n");
            /* TODO: ... wrong.. */
        }
    }
}

static void cmd_req_active_handler(void)
{
    dpram_send_mbx_BA(INT_COMMAND(MBX_CMD_RES_ACTIVE));
}

/* static void cmd_error_display_handler(void)
 * 
 * this fucntion was called by dpram irq handler and phone active irq hander
 * first this fucntion check the log level then jump to send error message to ril
 * or CP Upload mode
 */
static void cmd_error_display_handler(void)
{
#ifdef _ENABLE_ERROR_DEVICE
    unsigned short intr;
    unsigned long  flags;
    char buf[DPRAM_ERR_MSG_LEN];
    
    memset((void *)buf, 0, sizeof (buf));

    // check the reset or crash
//    if (!dpram_phone_getstatus()) {  --- can't catch the CDMA watchdog reset!!

#ifdef CONFIG_KERNEL_DEBUG_SEC    
    if (dpram_err_cause == UPLOAD_CAUSE_CDMA_RESET)
    {
        memcpy((void *)buf, "8 $PHONE-OFF", sizeof("8 $PHONE-OFF"));
    }
    else
    {
        buf[0] = 'C';
        buf[1] = 'D';
        buf[2] = 'M';
        buf[3] = 'A';
        buf[4] = ' ';

        READ_FROM_DPRAM((buf + 5), DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS, (sizeof(buf) - 6));
    }

    LOGE("[PHONE ERROR] ->> %s\n", buf);

    local_irq_save(flags);
    memcpy(dpram_err_buf, buf, DPRAM_ERR_MSG_LEN);

    dpram_err_len = 64;

    // goto Upload mode
    if (kernel_sec_get_debug_level() != KERNEL_SEC_DEBUG_LEVEL_LOW)
    {
        LOGE("Upload Mode!!!\n");
        intr = ioread16((void *)dpram_mbx_AB);
        if (intr == (MBX_CMD_CDMA_DEAD|INT_MASK_VALID|INT_MASK_COMMAND)) 
        {
            LOGE("mbx_AB = 0x%04X\n", intr);
        }

        kernel_sec_dump_cp_handle2();
    }

    local_irq_restore(flags);

    wake_up_interruptible(&dpram_err_wait_q);
    kill_fasync(&dpram_err_async_q, SIGIO, POLL_IN);
#endif    
#endif    /* _ENABLE_ERROR_DEVICE */
}


static void cmd_phone_start_handler(void)
{
    LOGA("Received CMD_PHONE_START!!!\n");

    if (g_phone_sync == 0)
    {
        dpram_init_and_report();
    }
}


static void cmd_req_time_sync_handler(void)
{
    /* TODO: add your codes here.. */
}

static void cmd_phone_deep_sleep_handler(void)
{
    /* TODO: add your codes here.. */
}

static void dpram_command_handler(u16 cmd)
{
    switch (cmd)
    {
        case MBX_CMD_REQ_ACTIVE:
            cmd_req_active_handler();
            break;

        case MBX_CMD_CDMA_DEAD:
            LOGE("received MBX_CMD_CDMA_DEAD\n");
            cmd_error_display_handler();
            break;

        case MBX_CMD_ERR_DISPLAY:
            LOGE("received MBX_CMD_ERR_DISPLAY\n");
            cmd_error_display_handler();
            break;

        case MBX_CMD_PHONE_START:
            cmd_phone_start_handler();
            break;

        case MBX_CMD_REQ_TIME_SYNC:
            cmd_req_time_sync_handler();
            break;

        case MBX_CMD_PHONE_DEEP_SLEEP:
            cmd_phone_deep_sleep_handler();
            break;

        case MBX_CMD_DPRAM_DOWN:
            dpram_power_down();
            break;

        case MBX_CMD_CP_WAKEUP_START:
            dpram_powerup_start();
            break;

        default:
            LOGA("Unknown command (0x%04X)\n", cmd);
    }
}

static void dpram_data_handler(u16 non_cmd)
{
    u16 head, tail;

    /* @LDK@ formatted check. */
#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS));
    tail = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS));
#else
    READ_FROM_DPRAM_VERIFY(&head, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS, sizeof(tail));
#endif

    if (head != tail)
    {
        non_cmd |= INT_MASK_SEND_F;

        if (non_cmd & INT_MASK_REQ_ACK_F)
            atomic_inc(&fmt_txq_req_ack_rcvd);
    }
    else
    {
        if (non_cmd & INT_MASK_REQ_ACK_F)
        {
            LOGA("FMT DATA EMPTY & REQ_ACK_F (non_cmd:0x%x)\n", non_cmd);
            dpram_send_mbx_BA(INT_NON_COMMAND(INT_MASK_RES_ACK_F));
            atomic_set(&fmt_txq_req_ack_rcvd, 0);
        }
    }
    
    /* @LDK@ raw check. */
#ifdef CDMA_IPC_C210_IDPRAM
    head = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS));
    tail = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS));
#else
    READ_FROM_DPRAM_VERIFY(&head, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS, sizeof(head));
    READ_FROM_DPRAM_VERIFY(&tail, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS, sizeof(tail));
#endif

    if (head != tail)
    {
        non_cmd |= INT_MASK_SEND_R;

        if (non_cmd & INT_MASK_REQ_ACK_R)
            atomic_inc(&raw_txq_req_ack_rcvd);
    }
    else
    {
        if (non_cmd & INT_MASK_REQ_ACK_R)
        {
	        LOGA("RAW DATA EMPTY & REQ_ACK_R (non_cmd:0x%x)\n", non_cmd);
            dpram_send_mbx_BA(INT_NON_COMMAND(INT_MASK_RES_ACK_R));
            atomic_set(&raw_txq_req_ack_rcvd, 0);
	    }
    }

    /* @LDK@ +++ scheduling.. +++ */
    if (non_cmd & INT_MASK_SEND_F)
    {
        dpram_tasklet_data[FORMATTED_INDEX].device = &dpram_table[FORMATTED_INDEX];
        dpram_tasklet_data[FORMATTED_INDEX].non_cmd = non_cmd;
        fmt_send_tasklet.data = (unsigned long)&dpram_tasklet_data[FORMATTED_INDEX];
        tasklet_schedule(&fmt_send_tasklet);
    }

    if (non_cmd & INT_MASK_SEND_R)
    {
        dpram_tasklet_data[RAW_INDEX].device = &dpram_table[RAW_INDEX];
        dpram_tasklet_data[RAW_INDEX].non_cmd = non_cmd;
        raw_send_tasklet.data = (unsigned long)&dpram_tasklet_data[RAW_INDEX];
        /* @LDK@ raw buffer op. -> soft irq level. */
        tasklet_hi_schedule(&raw_send_tasklet);
    }

    if (non_cmd & INT_MASK_RES_ACK_F)
    {
        tasklet_schedule(&fmt_res_ack_tasklet);
    }

    if (non_cmd & INT_MASK_RES_ACK_R)
    {
        tasklet_hi_schedule(&raw_res_ack_tasklet);
    }
}

static inline void check_int_pin_level(void)
{
    u16 mask = 0, cnt = 0;

    while (cnt++ < 3)
    {
        mask = ioread16((void *)dpram_mbx_AB);
        if ( gpio_get_value(IRQ_DPRAM_AP_INT_N) )
            break;
    }
}

static void phone_active_work_func(struct work_struct *work)
{
	u32 reset_code;

	if(gpio_get_value(IRQ_QSC_PHONE_ACTIVE))
		return;

	LOGA("PHONE_ACTIVE level: %s, phone_sync: %d\n",
		gpio_get_value(GPIO_QSC_PHONE_ACTIVE) ? "HIGH" : "LOW ",
							g_phone_sync);

#ifdef _ENABLE_ERROR_DEVICE
	/* If CDMA was reset by watchdog, phone active low time is very short So, change the IRQ type
	 * to FALING EDGE and don't check the GPIO_QSC_PHONE_ACTIVE
	 */
	//if((g_phone_sync) && (!gpio_get_value(GPIO_QSC_ACTIVE)))
	if(g_phone_sync) {
		READ_FROM_DPRAM((void *)&reset_code, DPRAM_MAGIC_CODE_ADDRESS, sizeof(reset_code));
		if(reset_code != CP_RESET_CODE) {
			#ifdef CONFIG_KERNEL_DEBUG_SEC
				dpram_err_cause = UPLOAD_CAUSE_CDMA_RESET;
			#endif
			//request_phone_reset();
			if (g_phone_power_off_sequence != 1)
				cmd_error_display_handler();
		}
	}
#endif

}

/* @LDK@ interrupt handlers. */
static irqreturn_t dpram_irq_handler(int irq, void *dev_id)
{
    unsigned long flags;
    u16 irq_mask = 0;
#ifdef PRINT_DPRAM_HEAD_TAIL    
    u16 fih, fit, foh, fot;
    u16 rih, rit, roh, rot;
#endif

    local_irq_save(flags);
    local_irq_disable();
#if 1  //Used for DPRAM Recovery
	if(rec_lock) {

    		IDPRAM_INT_CLEAR();
		local_irq_restore(flags);
		return IRQ_HANDLED;
	}
#endif 
    irq_mask = ioread16((void *)dpram_mbx_AB);
    //check_int_pin_level();

#ifdef DEBUG_DPRAM_INT_HANDLER
    LOGA("INT2AP: 0x%04X\n", irq_mask);
#endif

    /* valid bit verification. @LDK@ */
    if ( !(irq_mask & INT_MASK_VALID) )
    {
        LOGE("Invalid interrupt mask: 0x%04x\n", irq_mask);
        IDPRAM_INT_CLEAR();
        local_irq_restore(flags);
        return IRQ_NONE;
    }

    /* command or non-command? @LDK@ */
    if ( irq_mask & INT_MASK_COMMAND )
    {
        irq_mask &= ~(INT_MASK_VALID | INT_MASK_COMMAND);
        dpram_command_handler(irq_mask);
    }
    else
    {
#ifdef PRINT_DPRAM_HEAD_TAIL
        fih = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS));
        fit = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS));
        foh = ioread16((void *)(DPRAM_VBASE + DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS));
        fot = ioread16((void *)(DPRAM_VBASE + DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS));
        rih = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS));
        rit = ioread16((void *)(DPRAM_VBASE + DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS));
        roh = ioread16((void *)(DPRAM_VBASE + DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS));
        rot = ioread16((void *)(DPRAM_VBASE + DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS));
        LOGA(" FMT_IN  (H:%4d, T:%4d, M:%4d)\n FMT_OUT (H:%4d, T:%4d, M:%4d)\n RAW_IN  (H:%4d, T:%4d, M:%4d)\n RAW_OUT (H:%4d, T:%4d, M:%4d)\n",
             fih, fit, DPRAM_PHONE2PDA_FORMATTED_BUFFER_SIZE,
             foh, fot, DPRAM_PDA2PHONE_FORMATTED_BUFFER_SIZE,
             rih, rit, DPRAM_PHONE2PDA_RAW_BUFFER_SIZE,
             roh, rot, DPRAM_PDA2PHONE_RAW_BUFFER_SIZE);
#endif
        irq_mask &= ~INT_MASK_VALID;
        dpram_data_handler(irq_mask);
    }

    IDPRAM_INT_CLEAR();

    local_irq_restore(flags);
    return IRQ_HANDLED;
}


static irqreturn_t dpram_wake_from_CP_irq_handler(int irq, void *dev_id)
{
#ifdef DEBUG_DPRAM_INT_HANDLER
    LOGA("wake_lock_timeout() in 5 seconds.\n", __func__);
#endif

    wake_lock_timeout(&dpram_wake_lock, 5*HZ);

    return IRQ_HANDLED;
}

static irqreturn_t phone_active_irq_handler(int irq, void *dev_id)
{
	schedule_delayed_work(&phone_active_work, msecs_to_jiffies(1));
	return IRQ_HANDLED;
}

static int kernel_sec_dump_cp_handle2(void)
{
#ifdef CONFIG_KERNEL_DEBUG_SEC    
     t_kernel_sec_mmu_info mmu_info;    
    
    LOGA("Configure to restart AP and collect dump on restart...\n");

    // output high
	gpio_set_value(S5PV310_GPE0_3_MDM_IRQn, GPIO_LEVEL_HIGH);

    // GPIO output mux
	s3c_gpio_cfgpin(S5PV310_GPE0_0_MDM_WEn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_1_MDM_CSn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_2_MDM_Rn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_3_MDM_IRQn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_4_MDM_ADVN, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

    if (dpram_err_len)
        kernel_sec_set_cause_strptr(dpram_err_buf, dpram_err_len);
    
#if 0
    kernel_sec_set_upload_magic_number();
    kernel_sec_get_mmu_reg_dump(&mmu_info);
    kernel_sec_cdma_dpram_dump();
    kernel_sec_set_upload_cause((dpram_err_cause==0)?UPLOAD_CAUSE_CDMA_ERROR:dpram_err_cause);
    kernel_sec_hw_reset(false);
#else

    panic("CP Crash");
#endif
#endif
    return 0;
}

/* basic functions. */
#ifdef _ENABLE_ERROR_DEVICE
static struct file_operations dpram_err_ops = {
    .owner  = THIS_MODULE,
    .read   = dpram_err_read,
    .fasync = dpram_err_fasync,
    .poll   = dpram_err_poll,
    .llseek = no_llseek,

    /* TODO: add more operations */
};
#endif    /* _ENABLE_ERROR_DEVICE */

static struct tty_operations dpram_tty_ops = {
    .open            = dpram_tty_open,
    .close           = dpram_tty_close,
    .write           = dpram_tty_write,
    .write_room      = dpram_tty_write_room,
    .ioctl           = dpram_tty_ioctl,
    .chars_in_buffer = dpram_tty_chars_in_buffer,

    /* TODO: add more operations */
};

#ifdef _ENABLE_ERROR_DEVICE

static void unregister_dpram_err_device(void)
{
    unregister_chrdev(DRIVER_MAJOR_NUM, DPRAM_ERR_DEVICE);
    class_destroy(dpram_class);
}

static int register_dpram_err_device(void)
{
    /* @LDK@ 1 = formatted, 2 = raw, so error device is '0' */
    struct device *dpram_err_dev_t;
    int ret = register_chrdev(DRIVER_MAJOR_NUM, DPRAM_ERR_DEVICE, &dpram_err_ops);

    if ( ret < 0 )
    {
        return ret;
    }

    dpram_class = class_create(THIS_MODULE, "err");

    if (IS_ERR(dpram_class))
    {
        unregister_dpram_err_device();
        return -EFAULT;
    }

    dpram_err_dev_t = device_create(dpram_class, NULL,
            MKDEV(DRIVER_MAJOR_NUM, 0), NULL, DPRAM_ERR_DEVICE);

    if (IS_ERR(dpram_err_dev_t))
    {
        unregister_dpram_err_device();
        return -EFAULT;
    }

    return 0;
}
#endif    /* _ENABLE_ERROR_DEVICE */

static int register_dpram_driver(void)
{
    int retval = 0;

    /* @LDK@ allocate tty driver */
    dpram_tty_driver = alloc_tty_driver(MAX_INDEX);

    if (!dpram_tty_driver) {
        return -ENOMEM;
    }

    /* @LDK@ initialize tty driver */
    dpram_tty_driver->owner = THIS_MODULE;
    dpram_tty_driver->magic = TTY_DRIVER_MAGIC;
    dpram_tty_driver->driver_name = DRIVER_NAME;
    dpram_tty_driver->name = "dpram";
    dpram_tty_driver->major = DRIVER_MAJOR_NUM;
    dpram_tty_driver->minor_start = 1;
    dpram_tty_driver->num = MAX_INDEX;
    dpram_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
    dpram_tty_driver->subtype = SERIAL_TYPE_NORMAL;
    dpram_tty_driver->flags = TTY_DRIVER_REAL_RAW;
    dpram_tty_driver->init_termios = tty_std_termios;
    dpram_tty_driver->init_termios.c_cflag = (B115200 | CS8 | CREAD | CLOCAL | HUPCL);
    tty_set_operations(dpram_tty_driver, &dpram_tty_ops);

    dpram_tty_driver->ttys = dpram_tty;
    dpram_tty_driver->termios = dpram_termios;
    dpram_tty_driver->termios_locked = dpram_termios_locked;

    /* @LDK@ register tty driver */
    retval = tty_register_driver(dpram_tty_driver);
    if (retval) {
        LOGE("tty_register_driver error\n");
        put_tty_driver(dpram_tty_driver);
        return retval;
    }

    return 0;
}

static void unregister_dpram_driver(void)
{
    tty_unregister_driver(dpram_tty_driver);
}

/*
 * MULTI PDP FUNCTIONs
 */

static int multipdp_ioctl(struct inode *inode, struct file *file, 
                  unsigned int cmd, unsigned long arg)
{
    return -EINVAL;
}

static struct file_operations multipdp_fops = {
    .owner =    THIS_MODULE,
    .ioctl =    multipdp_ioctl,
    .llseek =    no_llseek,
};

static struct miscdevice multipdp_dev = {
    .minor =    132, //MISC_DYNAMIC_MINOR,
    .name =        APP_DEVNAME,
    .fops =        &multipdp_fops,
};

static inline struct pdp_info * pdp_get_serdev(const char *name)
{
    int slot;
    struct pdp_info *dev;

    for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
        dev = pdp_table[slot];
        if (dev && dev->type == DEV_TYPE_SERIAL &&
            strcmp(name, dev->vs_dev.tty_name) == 0) {
            return dev;
        }
    }
    return NULL;
}

static inline struct pdp_info * pdp_remove_dev(u8 id)
{
    int slot;
    struct pdp_info *dev;

    for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
        if (pdp_table[slot] && pdp_table[slot]->id == id) {
            dev = pdp_table[slot];
            pdp_table[slot] = NULL;
            return dev;
        }
    }
    return NULL;
}

static int vs_open(struct tty_struct *tty, struct file *filp)
{
    struct pdp_info *dev;

    dev = pdp_get_serdev(tty->driver->name); // 2.6 kernel porting

    if (dev == NULL) {
        return -ENODEV;
    }

    tty->driver_data = (void *)dev;
    tty->low_latency = 1;
    dev->vs_dev.tty = tty;
    dev->vs_dev.refcount++;

    return 0;
}

static void vs_close(struct tty_struct *tty, struct file *filp)
{
    struct pdp_info *dev;

    dev = pdp_get_serdev(tty->driver->name); 

    if (!dev )
        return;
    dev->vs_dev.refcount--;

    return;
}

static int pdp_mux(struct pdp_info *dev, const void *data, size_t len   )
{
    int ret;
    size_t nbytes;
    u8 *tx_buf;
    struct pdp_hdr *hdr;
    const u8 *buf;

    tx_buf = dev->tx_buf;
    hdr = (struct pdp_hdr *)(tx_buf + 1);
    buf = data;

    hdr->id = dev->id;
    hdr->control = 0;

    while (len)
    {
        if (len > MAX_PDP_DATA_LEN)
        {
            nbytes = MAX_PDP_DATA_LEN;
        }
        else
        {
            nbytes = len;
        }
        hdr->len = nbytes + sizeof(struct pdp_hdr);

        tx_buf[0] = 0x7f;
        
        memcpy(tx_buf + 1 + sizeof(struct pdp_hdr), buf,  nbytes);
        
        tx_buf[1 + hdr->len] = 0x7e;

        ret = dpram_write(&dpram_table[RAW_INDEX], tx_buf, hdr->len + 2);

        if (ret < 0)
        {
            LOGE("write_to_dpram() failed: %d\n", ret);
            return ret;
        }
        buf += nbytes;
        len -= nbytes;
    }
    return 0;
}


static int vs_write(struct tty_struct *tty,
        const unsigned char *buf, int count)
{
    int ret;
    struct pdp_info *dev = (struct pdp_info *)tty->driver_data;

    ret = pdp_mux(dev, buf, count);

    if (ret == 0)
    {
        ret = count;
    }

    return ret;
}

static int vs_write_room(struct tty_struct *tty) 
{
//    return TTY_FLIPBUF_SIZE;
    return 8192*2;
}

static int vs_chars_in_buffer(struct tty_struct *tty) 
{
    return 0;
}

static int vs_ioctl(struct tty_struct *tty, struct file *file, 
            unsigned int cmd, unsigned long arg)
{
    return -ENOIOCTLCMD;
}

static struct tty_operations multipdp_tty_ops = {
    .open         = vs_open,
    .close         = vs_close,
    .write         = vs_write,
    .write_room = vs_write_room,
    .ioctl         = vs_ioctl,
    .chars_in_buffer = vs_chars_in_buffer,

    /* TODO: add more operations */
};

static struct tty_driver* get_tty_driver_by_id(struct pdp_info *dev)
{
    int index = 0;

    switch (dev->id) {
        case 1:        index = 0;    break;
        case 7:        index = 1;    break;
        case 9:        index = 2;    break;
        case 27:    index = 3;    break;
        default:    index = 0;
    }

    return &dev->vs_dev.tty_driver[index];
}

static int get_minor_start_index(int id)
{
    int start = 0;

    switch (id) {
        case 1:        start = 0;    break;
        case 7:        start = 1;    break;
        case 9:        start = 2;    break;
        case 27:    start = 3;    break;
        default:    start = 0;
    }

    return start;
}

static int vs_add_dev(struct pdp_info *dev)
{
    struct tty_driver *tty_driver;

    tty_driver = get_tty_driver_by_id(dev);

    if (!tty_driver)
    {
        LOGE("tty driver == NULL!\n");
        return -1;
    }

    kref_init(&tty_driver->kref);

    tty_driver->magic    = TTY_DRIVER_MAGIC;
    tty_driver->driver_name    = APP_DEVNAME;//"multipdp";
    tty_driver->name    = dev->vs_dev.tty_name;
    tty_driver->major    = CSD_MAJOR_NUM;
    tty_driver->minor_start = get_minor_start_index(dev->id);
    tty_driver->num        = 1;
    tty_driver->type    = TTY_DRIVER_TYPE_SERIAL;
    tty_driver->subtype    = SERIAL_TYPE_NORMAL;
    tty_driver->flags    = TTY_DRIVER_REAL_RAW;
//    kref_set(&tty_driver->kref, dev->vs_dev.refcount);
    tty_driver->ttys    = dev->vs_dev.tty_table; // 2.6 kernel porting
    tty_driver->termios    = dev->vs_dev.termios;
    tty_driver->termios_locked    = dev->vs_dev.termios_locked;

    tty_set_operations(tty_driver, &multipdp_tty_ops);
    return tty_register_driver(tty_driver);
}

static void vs_del_dev(struct pdp_info *dev)
{
    struct tty_driver *tty_driver = NULL;

    tty_driver = get_tty_driver_by_id(dev);
    tty_unregister_driver(tty_driver);
}

static inline void check_pdp_table(char * func, int line)
{
    int slot;
    for (slot = 0; slot < MAX_PDP_CONTEXT; slot++)
    {
        if (pdp_table[slot])
            LOGA("[%s,%d] addr: %x slot: %d id: %d, name: %s\n", func, line, pdp_table[slot], slot, pdp_table[slot]->id, pdp_table[slot]->vs_dev.tty_name);
    }
}

static inline struct pdp_info * pdp_get_dev(u8 id)
{
    int slot;

    for (slot = 0; slot < MAX_PDP_CONTEXT; slot++)
    {
        if (pdp_table[slot] && pdp_table[slot]->id == id)
        {
            return pdp_table[slot];
        }
    }
    return NULL;
}

static inline int pdp_add_dev(struct pdp_info *dev)
{
    int slot;

    if (pdp_get_dev(dev->id)) {
        return -EBUSY;
    }

    for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
        if (pdp_table[slot] == NULL) {
            pdp_table[slot] = dev;
            return slot;
        }
    }
    return -ENOSPC;
}

static int pdp_activate(pdp_arg_t *pdp_arg, unsigned type, unsigned flags)
{
    int ret;
    struct pdp_info *dev;

    LOGL(DL_INFO, "id: %d\n", pdp_arg->id);

    dev = kmalloc(sizeof(struct pdp_info) + MAX_PDP_PACKET_LEN, GFP_KERNEL);
    if (dev == NULL) {
        LOGE("out of memory\n");
        return -ENOMEM;
    }
    memset(dev, 0, sizeof(struct pdp_info));

    dev->id = pdp_arg->id;

    dev->type = type;
    dev->flags = flags;
    dev->tx_buf = (u8 *)(dev + 1);

    if (type == DEV_TYPE_SERIAL) {
        init_MUTEX(&dev->vs_dev.write_lock);
        strcpy(dev->vs_dev.tty_name, pdp_arg->ifname);

        ret = vs_add_dev(dev);
        if (ret < 0) {
            kfree(dev);
            return ret;
        }

        mutex_lock(&pdp_lock);
        ret = pdp_add_dev(dev);
        if (ret < 0) {
            LOGE("pdp_add_dev() failed\n");
            mutex_unlock(&pdp_lock);
            vs_del_dev(dev);
            kfree(dev);
            return ret;
        }
        mutex_unlock(&pdp_lock);

        {
            struct tty_driver * tty_driver = get_tty_driver_by_id(dev);

            LOGL(DL_INFO, "%s(id: %u) serial device is created.\n",
                    tty_driver->name, dev->id);
        }
    }

    return 0;
}

static int multipdp_init(void)
{
    int i;

    pdp_arg_t pdp_args[NUM_PDP_CONTEXT] = {
        { .id = 1, .ifname = "ttyCSD" },
        { .id = 7, .ifname = "ttyCDMA" },
        { .id = 9, .ifname = "ttyTRFB" },
        { .id = 27, .ifname = "ttyCIQ" },
    };


    /* create serial device for Circuit Switched Data */
    for (i = 0; i < NUM_PDP_CONTEXT; i++) {
        if (pdp_activate(&pdp_args[i], DEV_TYPE_SERIAL, DEV_FLAG_STICKY) < 0) {
            LOGE("failed to create a serial device for %s\n", pdp_args[i].ifname);
        }
    }

    return 0;
}

/*
 * DPRAM DRIVER INITIALIZE FUNCTIONs
 */
static int dpram_init_hw(void)
{
   int rv; 

    PRINT_FUNC();
  
 
    //1) Initialize the interrupt pins
    set_irq_type(IRQ_DPRAM_AP_INT_N, IRQ_TYPE_LEVEL_LOW);

    rv = gpio_request(IRQ_QSC_INT, "gpx1_0");
    if(rv < 0) {
        printk("IDPRAM: [%s] failed to get gpio GPX1_0\n",__func__);
	goto err;
    }

    dpram_wakeup_irq = gpio_to_irq(IRQ_QSC_INT);

    s3c_gpio_cfgpin(GPIO_C210_DPRAM_INT_N, S3C_GPIO_SFN(0xFF));   
    s3c_gpio_setpull(GPIO_C210_DPRAM_INT_N, S3C_GPIO_PULL_NONE);   
    set_irq_type(dpram_wakeup_irq, IRQ_TYPE_EDGE_RISING);

    rv = gpio_request(IRQ_QSC_PHONE_ACTIVE, "gpx1_6");
    if(rv < 0) {
        printk("IDPRAM: [%s] failed to get gpio GPX1_6\n",__func__);
	goto err1;
    }

    phone_active_irq = gpio_to_irq(IRQ_QSC_PHONE_ACTIVE);

    s3c_gpio_setpull(GPIO_QSC_PHONE_ACTIVE, S3C_GPIO_PULL_NONE);
    s3c_gpio_setpull(GPIO_QSC_PHONE_RST, S3C_GPIO_PULL_NONE);
//    set_irq_type(IRQ_QSC_ACTIVE, IRQ_TYPE_EDGE_BOTH);
    set_irq_type(phone_active_irq, IRQ_TYPE_EDGE_FALLING);

    // 2)EINT15 : QSC_ACTIVE (GPIO_INTERRUPT)
//    #define FILTER_EINT15_EN (0x1<<31)
//    #define FILTER_EINT15_SEL_DEGIT (0x1<<30)
//__raw_writel(__raw_readl(S5PV210_EINT1FLTCON1)|(FILTER_EINT15_EN & (~FILTER_EINT15_SEL_DEGIT)),S5PV210_EINT1FLTCON1);

    //3)EINT09 : QSC_INT (GPIO_INTERRUPT)
//    #define FILTER_EINT9_EN (0x1<<15)
//    #define FILTER_EINT9_SEL_DEGIT (0x1<<14)
//__raw_writel(__raw_readl(S5PV210_EINT1FLTCON0)|(FILTER_EINT9_EN & (~FILTER_EINT9_SEL_DEGIT)),S5PV210_EINT1FLTCON0);


    // 4)gpio e3-e4 are for Modem if

	s3c_gpio_cfgpin(S5PV310_GPE3_0_MDM_DATA_0, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_1_MDM_DATA_1, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_2_MDM_DATA_2, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_3_MDM_DATA_3, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_4_MDM_DATA_4, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_5_MDM_DATA_5, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_6_MDM_DATA_6, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_7_MDM_DATA_7, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

	s3c_gpio_cfgpin(S5PV310_GPE4_0_MDM_DATA_8, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_1_MDM_DATA_9, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_2_MDM_DATA_10, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_3_MDM_DATA_11, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_4_MDM_DATA_12, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_5_MDM_DATA_13, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_6_MDM_DATA_14, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_7_MDM_DATA_15, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

	s3c_gpio_cfgpin(S5PV310_GPE0_0_MDM_WEn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_1_MDM_CSn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_2_MDM_Rn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_3_MDM_IRQn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_4_MDM_ADVN, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

	rv = gpio_request(GPIO_PDA_ACTIVE, "GPY4_2");
	if(rv < 0) {
	        printk("IDPRAM: [%s] failed to get gpio GPY4_2\n",__func__);
		goto err2;
	}

	rv = gpio_request(GPIO_QSC_PHONE_ON, "GPC1_1");
	if(rv < 0) {
        	printk("IDPRAM: [%s] failed to get gpio GPC1_1\n",__func__);
		goto err3;
	}

	rv = gpio_request(GPIO_QSC_PHONE_RST, "GPX1_4");
	if(rv < 0) {
        	printk("IDPRAM: [%s] failed to get gpio GPX1_4\n",__func__);
		goto err4;
	}

	s5p_gpio_set_drvstr(GPIO_QSC_PHONE_RST, S5P_GPIO_DRVSTR_LV4); //To increase driving strength of this pin

	//To config Not Connected pin GPY4_6 to I/P with no pullup
	rv = gpio_request(GPIO_CP_REQ_RESET, "GPY4_6");
        if(rv < 0) {
                printk("IDPRAM: [%s] failed to get gpio GPY4_6\n",__func__);
                goto err5;
        }
	
	gpio_direction_input(GPIO_CP_REQ_RESET);
    	s3c_gpio_setpull(GPIO_CP_REQ_RESET, S3C_GPIO_PULL_NONE);   

   // 5)PDA_ACTIVE, QSC_PHONE_ON, QSC_RESET_N
    gpio_direction_output(GPIO_PDA_ACTIVE, GPIO_LEVEL_HIGH);
    gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);
    s3c_gpio_setpull(GPIO_QSC_PHONE_ON, S3C_GPIO_PULL_NONE);   
    gpio_direction_output(GPIO_QSC_PHONE_RST, GPIO_LEVEL_LOW);

	return 0;

err5:	gpio_free(GPIO_QSC_PHONE_RST);
err4:	gpio_free(GPIO_QSC_PHONE_ON);
err3:	gpio_free(GPIO_PDA_ACTIVE);
err2:	gpio_free(IRQ_QSC_PHONE_ACTIVE);
err1:	gpio_free(IRQ_QSC_INT);
err:	return rv;
}


static int dpram_shared_bank_remap(void)
{

//Clock Settings for Modem IF ====>FixMe
    u32 val;
    void __iomem *regs = ioremap(0x10030000, 0x10000);
   
    //Enabling Clock for ModemIF in Reg CLK_GATE_IP_PERIL:0x1003C950
    val = readl(regs + 0xC950);	
    val |= 0x10000000;
    writel(val, regs + 0xC950);
    iounmap(regs);
    
    //3 Get internal DPRAM / SFR Virtual address [[
    // 1) dpram base
    idpram_base = (volatile void __iomem *)ioremap_nocache(IDPRAM_PHYSICAL_ADDR, IDPRAM_SIZE); 
    if (idpram_base == NULL)
    {
        LOGE("Failed!!! (idpram_base == NULL)\n");
        return -1;
    }
    LOGA("BUF PA = 0x%08X, VA = 0x%08X\n", IDPRAM_PHYSICAL_ADDR, idpram_base);

    // 2) sfr base
    idpram_sfr_base = (volatile IDPRAM_SFR __iomem *)ioremap_nocache(IDPRAM_SFR_PHYSICAL_ADDR, IDPRAM_SFR_SIZE); 
    if (idpram_sfr_base == NULL)
    {
        LOGE("Failed!!! (idpram_sfr_base == NULL)\n");
        iounmap(idpram_base);
        return -1;
    }
    LOGA("SFR PA = 0x%08X, VA = 0x%08X\n", IDPRAM_SFR_PHYSICAL_ADDR, idpram_sfr_base);
    //3 ]]

    // 3) Initialize the Modem interface block(internal DPRAM) 
    // TODO : Use DMA controller? ask to sys.lsi
    // set Modem interface config register
    idpram_sfr_base->mifcon = (IDPRAM_MIFCON_FIXBIT|IDPRAM_MIFCON_INT2APEN|IDPRAM_MIFCON_INT2MSMEN); //FIXBIT enable, interrupt enable AP,MSM(CP)
    idpram_sfr_base->mifpcon = (IDPRAM_MIFPCON_ADM_MODE); //mux mode

    dpram_mbx_BA = (volatile u16*)(idpram_base + IDPRAM_AP2MSM_INT_OFFSET);
    dpram_mbx_AB = (volatile u16*)(idpram_base + IDPRAM_MSM2AP_INT_OFFSET);
    LOGA("VA of mbx_BA = 0x%08X, VA of mbx_AB = 0x%08X\n", dpram_mbx_BA, dpram_mbx_AB);

    // write the normal boot magic key for CDMA boot
    *((unsigned int *)idpram_base) = DPRAM_BOOT_NORMAL;

    atomic_set(&dpram_read_lock, 0);
    atomic_set(&dpram_write_lock, 0);

    return 0;
}

static void dpram_init_devices(void)
{
    int i;

    for (i = 0; i < MAX_INDEX; i++) {
        init_MUTEX(&dpram_table[i].serial.sem);

        dpram_table[i].serial.open_count = 0;
        dpram_table[i].serial.tty = NULL;
    }
}

void dpram_wakeup_init(void)
{
    idpram_sfr_base->mifcon = (IDPRAM_MIFCON_FIXBIT|IDPRAM_MIFCON_INT2APEN|IDPRAM_MIFCON_INT2MSMEN);
    idpram_sfr_base->mifpcon = (IDPRAM_MIFPCON_ADM_MODE);

    // mux GPIO_DPRAM_INT_CP_N to dpram interrupt

        s3c_gpio_cfgpin(S5PV310_GPE0_0_MDM_WEn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_1_MDM_CSn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_2_MDM_Rn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_3_MDM_IRQn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_4_MDM_ADVN, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

}

#define WAKESTART_TIMEOUT (HZ/10)
#define WAKESTART_TIMEOUT_RETRY 5

void dpram_wait_wakeup_start(void)
{
    int wakeup_retry = WAKESTART_TIMEOUT_RETRY;
    int timeout_ret = 0;

    do {
        init_completion(&g_complete_dpramdown);
        dpram_send_mbx_BA_cmd(INT_COMMAND(MBX_CMD_PDA_WAKEUP));
        timeout_ret = wait_for_completion_timeout(&g_complete_dpramdown, WAKESTART_TIMEOUT);
    } while(!timeout_ret && wakeup_retry--);

    if (!timeout_ret)
    {
        LOGE("T-I-M-E-O-U-T !!!\n");
        //dpram_qsc_timeout_handler();
    }

    g_dpram_wpend = IDPRAM_WPEND_UNLOCK; // dpram write unlock
}

static void kill_tasklets(void)
{
    tasklet_kill(&fmt_res_ack_tasklet);
    tasklet_kill(&raw_res_ack_tasklet);

    tasklet_kill(&fmt_send_tasklet);
    tasklet_kill(&raw_send_tasklet);
}

static int register_interrupt_handler(void)
{
    unsigned int dpram_irq;
    int retval = 0;
    
    dpram_irq = IRQ_DPRAM_AP_INT_N;

    dpram_clear();

    //1)dpram interrupt 
    IDPRAM_INT_CLEAR();
    retval = request_irq(dpram_irq, dpram_irq_handler, IRQF_DISABLED, "DPRAM irq", NULL);
    if (retval)
    {
        LOGE("DPRAM interrupt handler failed.\n");
        unregister_dpram_driver();
        return -1;
    }

    //2) wake up for internal dpram

    retval = request_irq(dpram_wakeup_irq, dpram_wake_from_CP_irq_handler, IRQF_DISABLED, "QSC_INT irq", NULL);
    if (retval)
    {
        LOGE("DPRAM wakeup interrupt handler failed.\n");
        free_irq(dpram_irq, NULL);
        unregister_dpram_driver();
        return -1;
    }

    //3) phone active interrupt

    retval = request_irq(phone_active_irq, phone_active_irq_handler, IRQF_DISABLED, "QSC_ACTIVE", NULL);
    if (retval)
    {
        LOGE("Phone active interrupt handler failed.\n");
        free_irq(dpram_irq, NULL);
        free_irq(dpram_wakeup_irq, NULL);
        unregister_dpram_driver();
        return -1;
    }

    return 0;
}

static void check_miss_interrupt(void)
{
    unsigned long flags;

    if ( gpio_get_value(GPIO_QSC_PHONE_ACTIVE) && !gpio_get_value(IRQ_DPRAM_AP_INT_N) )
    {
        local_irq_save(flags);
        dpram_irq_handler(IRQ_DPRAM_AP_INT_N, NULL);
        local_irq_restore(flags);
    }
}

/*
 * INTERANL DPRAM POWER DOWN FUNCTION
 */


/*
 * void dpram_power_down()
 *
 * This function release the wake_lock 
 * Phone send DRPAM POWER DOWN interrupt and handler call this function.
 */
static void dpram_power_down(void)
{
#ifdef PRINT_DPRAM_PWR_CTRL
    LOGA("Received MBX_CMD_DPRAM_DOWN (lock count = %d)!!!\n",
          dpram_get_lock_read());
#endif
    complete(&g_complete_dpramdown);
}


static void dpram_powerup_start(void)
{
#ifdef PRINT_DPRAM_PWR_CTRL
    LOGA("Received MBX_CMD_CP_WAKEUP_START!!!\n");
#endif
    complete(&g_complete_dpramdown);
}


/*
 * void dpram_power_up()
 *
 * Initialize dpram when ap wake up and send WAKEUP_INT to phone
 * This function will be called by Onedram_resume()
 */
static inline void dpram_power_up(void)
{
    const u16 magic_code = 0x00AA;
    u16 acc_code = 0x0001;

    dpram_clear();

    WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS, &magic_code, sizeof(magic_code));
    WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &acc_code, sizeof(acc_code));

    // Initialize the dpram controller
    dpram_wakeup_init();

#ifdef PRINT_DPRAM_PWR_CTRL
    // Check for QSC_INT for debugging
    LOGA("dpram_wakeup_init() ==> QSC_INT = %s\n",
          gpio_get_value(GPIO_QSC_INT) ? "HIGH" : "LOW");
#endif
}


/*
 * DPRAM DRIVER FUNCTIONs
 */
#define DPRAMDOWN_TIMEOUT       (HZ * 3)
#define DPRAM_PDNINTR_RETRY_CNT 2

/*
** lock the AP write dpram and send the SLEEP INT to Phone 
*/
static int dpram_suspend(struct platform_device *dev, pm_message_t state)
{
    u16 in_intr = 0;
    u16 timeout_ret = 0;
    u16 suspend_retry = DPRAM_PDNINTR_RETRY_CNT;

    g_dpram_wpend = IDPRAM_WPEND_LOCK;        // dpram write lock

    /*
    ** if some intrrupt was received by cp, dpram hold the wake lock and pass the suspend mode.
    */
    if (dpram_get_lock_read() == 0)
    {
        /*
        ** retry sending MBX_CMD_PDA_SLEEP if CP does not send in timout
        */
        do {
            init_completion(&g_complete_dpramdown);
            dpram_send_mbx_BA_cmd(INT_COMMAND(MBX_CMD_PDA_SLEEP));
            //timeout_ret = wait_for_completion_timeout(&g_complete_dpramdown, DPRAMDOWN_TIMEOUT);
            timeout_ret = wait_for_completion_timeout(&g_complete_dpramdown, msecs_to_jiffies(500));
#ifdef PRINT_DPRAM_PWR_CTRL
            LOGA("suspend_enter cnt = %d\n", (DPRAM_PDNINTR_RETRY_CNT - suspend_retry));
#endif
        } while ( !timeout_ret && suspend_retry-- );

        in_intr = ioread16((void *)dpram_mbx_AB); 
        if (in_intr != (INT_MASK_VALID|INT_MASK_COMMAND|MBX_CMD_DPRAM_DOWN))
        {
            LOGE("T-I-M-E-O-U-T !!! (intr = 0x%04X)\n", in_intr);
            //dpram_qsc_timeout_handler();
            return -1;
        }
        
        /*
        * Because, if dpram was powered down, cp dpram random intr was ocurred,
        * So, fixed by muxing cp dpram intr pin to GPIO output high,..
        */
	gpio_set_value(S5PV310_GPE0_3_MDM_IRQn, GPIO_LEVEL_HIGH);

        s3c_gpio_cfgpin(S5PV310_GPE0_0_MDM_WEn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_1_MDM_CSn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_2_MDM_Rn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_3_MDM_IRQn, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
        s3c_gpio_cfgpin(S5PV310_GPE0_4_MDM_ADVN, S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

        gpio_set_value(GPIO_PDA_ACTIVE, GPIO_LEVEL_LOW);

	flush_work(&phone_active_work.work);

	//To configure AP_WAKE wakable interrupt
	disable_irq(dpram_wakeup_irq);
	enable_irq_wake(dpram_wakeup_irq);
    }
    else
    {
        //wake_lock_timeout(&dpram_wake_lock, HZ/4);
        LOGA("Skip the suspned mode - read lock \n");
        return -1;
    }

    return 0;
}


static int dpram_resume(struct platform_device *dev)
{
    gpio_set_value(GPIO_PDA_ACTIVE, GPIO_LEVEL_HIGH);

    dpram_power_up();
    dpram_wait_wakeup_start(); // wait the CP ack 0xCE

	//To configure AP_WAKE normal interrupt
        disable_irq_wake(dpram_wakeup_irq);
        enable_irq(dpram_wakeup_irq);

    check_miss_interrupt();
    return 0;
}


static int __devinit dpram_probe(struct platform_device *dev)
{
    int retval;

    PRINT_FUNC();

    retval = register_dpram_driver();
    if (retval) {
        LOGE("Failed to register dpram (tty) driver.\n");
        return -1;
    }

    LOGA("register_dpram_driver() success!!!\n");

#ifdef _ENABLE_ERROR_DEVICE
    retval = register_dpram_err_device();
    if (retval)
    {
        LOGE("Failed to register dpram error device.\n");
        unregister_dpram_driver();
        return -1;
    }
    memset((void *)dpram_err_buf, '\0', sizeof dpram_err_buf);
#endif /* _ENABLE_ERROR_DEVICE */

    LOGA("register_dpram_err_device() success!!!\n");

    retval = misc_register(&multipdp_dev);    /* create app. interface device */
    if (retval < 0)
    {
        LOGE("misc_register() failed\n");
        return -1;
    }

    multipdp_init();

    retval = dpram_init_hw();
    if (retval < 0)
    {
        LOGE("dpram_init_hw() failed\n");
        return -1;
    }

    dpram_shared_bank_remap();

    //3 DJ07 Dr.J ADD [[
    iowrite16(0, (void *)(DPRAM_VBASE + DPRAM_MAGIC_CODE_ADDRESS));
    iowrite16(0, (void *)(DPRAM_VBASE + DPRAM_ACCESS_ENABLE_ADDRESS));
    dpram_clear();
    //3 ]]

    dpram_init_devices();

    atomic_set(&raw_txq_req_ack_rcvd, 0);
    atomic_set(&fmt_txq_req_ack_rcvd, 0);
    
    wake_lock_init(&dpram_wake_lock, WAKE_LOCK_SUSPEND, "DPRAM_PWDOWN");

    retval = register_interrupt_handler();
    if ( retval < 0 )
    {
        LOGE("register_interrupt_handler() failed!!!\n");

	gpio_free(GPIO_QSC_PHONE_RST);
	gpio_free(GPIO_QSC_PHONE_ON);
	gpio_free(GPIO_PDA_ACTIVE);
	gpio_free(IRQ_QSC_PHONE_ACTIVE);
	gpio_free(IRQ_QSC_INT);

        return -1;
    }

#ifdef CONFIG_PROC_FS
    create_proc_read_entry(DRIVER_PROC_ENTRY, 0, 0, dpram_read_proc, NULL);
#endif    /* CONFIG_PROC_FS */
//DI20 Dr.J ...    cdma_slot_switch_handler = slot_switch_handler2;

    //check_miss_interrupt();
    INIT_DELAYED_WORK(&phone_active_work, phone_active_work_func);
    init_completion(&g_complete_dpramdown);
    printk("IDPRAM: DPRAM driver is registered !!!\n");

    return 0;
}

static int __devexit dpram_remove(struct platform_device *dev)
{
    PRINT_FUNC();

    wake_lock_destroy(&dpram_wake_lock);

    /* @LDK@ unregister dpram (tty) driver */
    unregister_dpram_driver();

    /* @LDK@ unregister dpram error device */
#ifdef _ENABLE_ERROR_DEVICE
    unregister_dpram_err_device();
#endif
    
    /* remove app. interface device */
    misc_deregister(&multipdp_dev);

    free_irq(IRQ_DPRAM_AP_INT_N, NULL);
    free_irq(phone_active_irq, NULL);
    free_irq(dpram_wakeup_irq, NULL);

    gpio_free(GPIO_QSC_PHONE_RST);
    gpio_free(GPIO_QSC_PHONE_ON);
    gpio_free(GPIO_PDA_ACTIVE);
    gpio_free(IRQ_QSC_PHONE_ACTIVE);
    gpio_free(IRQ_QSC_INT);

    flush_work(&phone_active_work.work);
    kill_tasklets();

    return 0;
}

static int dpram_shutdown(struct platform_device *dev)
{
   int ret = FALSE;
   
    PRINT_FUNC();
	ret = dpram_remove(dev);
	return ret;
}

u32 dpram_get_phone_dump_stat(void)
{
    return g_dump_on;   
}

EXPORT_SYMBOL(dpram_get_phone_dump_stat);

static struct platform_driver platform_dpram_driver = {
    .probe    = dpram_probe,
    .remove   = __devexit_p(dpram_remove),
    .suspend  = dpram_suspend,
    .resume   = dpram_resume,    
    .shutdown = dpram_shutdown,
    .driver   = {
        .name = "dpram-device",
    },
};

/* init & cleanup. */
static int __init dpram_init(void)
{

    return platform_driver_register(&platform_dpram_driver);
}

static void __exit dpram_exit(void)
{
    platform_driver_unregister(&platform_dpram_driver);
}

module_init(dpram_init);
module_exit(dpram_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("Internal DPRAM Device Driver.");
MODULE_LICENSE("GPL");
