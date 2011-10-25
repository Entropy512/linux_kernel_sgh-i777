/* sound/soc/s3c24xx/s5p-rp.c
 *
 * SRP Audio driver for Samsung s5pc210
 *
 * Copyright (c) 2010 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
#include <mach/media.h>
#include <plat/media.h>
#endif

#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <mach/irqs.h>

#include "s3c-idma.h"
#include "s5p-rp_reg.h"
#include "s5p-rp_fw.h"
#include "s5p-rp_ioctl.h"

#define _USE_START_WITH_BUF0_		/* Start RP after IBUF0 fill */
#define _USE_AUTO_PAUSE_		/* Pause RP, when two IBUF are empty */
/*#define _USE_FW_ENDIAN_CONVERT_*/	/* Endian conversion */
#define _USE_PCM_DUMP_			/* PCM snoop for Android */
#define _USE_EOS_TIMEOUT_		/* Timeout option for EOS */
/*#define _USE_CRC_CHK_*/
/*#define _USE_POSTPROCESS_SKIP_TEST_*/	/* Decoding only without audio output */

#if (defined CONFIG_ARCH_S5PV310) || (defined CONFIG_ARCH_S5PC210)
	#define _IMEM_MAX_		(64 * 1024)	/* 64KBytes */
	#define _DMEM_MAX_		(128 * 1024)	/* 128KBytes */
	#define _IBUF_SIZE_		(128 * 1024)	/* 128KBytes in DRAM */
	#define _WBUF_SIZE_		(_IBUF_SIZE_ * 2)	/* in DRAM */
	#define _FWBUF_SIZE_		(4 * 1024)	/* 4KBytes in Firmware */

	#define _IMEM_OFFSET_		(0x00400)	/* 1KB offset ok */
	#define _OBUF_SIZE_AB_		(45056)		/* 9Frames */
	#define _OBUF0_OFFSET_AB_	(0x0A000)
	#define _OBUF1_OFFSET_AB_	(0x15000)
	#define _OBUF_SIZE_C_		(4608 * 2)	/* 2Frames */
	#define _OBUF0_OFFSET_C_	(0x19800)
	#define _OBUF1_OFFSET_C_	(0x1CC00)

	#define _VLIW_SIZE_		(128 * 1024)	/* 128KBytes */
	#define _DATA_SIZE_		(128 * 1024)	/* 128KBytes */
	#define _CGA_SIZE_		(36 * 1024)	/* 36KBytes */

	#define _PCM_DUMP_SIZE_		(4 * 1024)	/* 4KBytes */

	/* Reserved memory on DRAM */
	#define _BASE_MEM_SIZE_		(CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP << 10)
	#define _VLIW_SIZE_MAX_		(256 * 1024)	/* Enough? */
	#define _CGA_SIZE_MAX_		(64 * 1024)
	#define _DATA_SIZE_MAX_		(128 * 1024)
#else
	#error CONFIG_ARCH not found
#endif

#define _BITSTREAM_SIZE_MAX_		(0x7FFFFFFF)

#ifdef _USE_FW_ENDIAN_CONVERT_
#define ENDIAN_CHK_CONV(VAL)		\
	(((VAL >> 24) & 0x000000FF) |	\
	((VAL >>  8) & 0x0000FF00) |	\
	((VAL <<  8) & 0x00FF0000) |	\
	((VAL << 24) & 0xFF000000))
#else
#define ENDIAN_CHK_CONV(VAL)	(VAL)
#endif

#define RP_DEV_MINOR		(250)
#define RP_CTRL_DEV_MINOR	(251)

#ifdef CONFIG_SND_S5P_RP_DEBUG
#define s5pdbg(x...) printk(KERN_INFO "S5P_RP: " x)
#else
#define s5pdbg(x...)
#endif


struct s5p_rp_info {
	void __iomem  *iram;
	void __iomem  *sram;
	void __iomem  *clkcon;
	void __iomem  *commbox;

	void __iomem  *iram_imem;
	void __iomem  *obuf0;
	void __iomem  *obuf1;

	void __iomem  *dmem;
	void __iomem  *icache;
	void __iomem  *cmem;
	void __iomem  *special;

	int ibuf_next;				/* IBUF index for next write */
	int ibuf_empty[2];			/* Empty flag of IBUF0/1 */
#ifdef _USE_START_WITH_BUF0_
	int ibuf_req_skip;			/* IBUF req can be skipped */
#endif
	unsigned long ibuf_size;		/* IBUF size byte */
	unsigned long frame_size;		/* 1 frame size = 1152 or 576 */
	unsigned long frame_count;		/* Decoded frame counter */
	unsigned long frame_count_base;
	unsigned long channel;			/* Mono = 1, Stereo = 2 */

	int block_mode;				/* Block Mode */
	int decoding_started;			/* Decoding started flag */
	int wait_for_eos;			/* Wait for End-Of-Stream */
	int stop_after_eos;			/* State for Stop-after-EOS */
	int pause_request;			/* Pause request from ioctl */
	int auto_paused;			/* Pause by IBUF underrun */
	int suspend_during_eos;			/* Suspend state during EOS */
#ifdef _USE_EOS_TIMEOUT_
	int timeout_eos_enabled;		/* Timeout switch during EOS */
	struct timeval timeout_eos;		/* Timeout at acctual EOS */
	unsigned long timeout_read_size;	/* Last READ_BITSTREAM_SIZE */
#endif
	unsigned long error_info;		/* Error Information */
	unsigned long gain;			/* Gain */
	unsigned long gain_subl;		/* Gain sub left */
	unsigned long gain_subr;		/* Gain sub right */
	int dram_in_use;			/* DRAM is accessed by SRP */
	int op_mode;				/* Operation mode: typeA/B/C */
	int i2s_irq_enabled;			/* I2S irq state */
	int early_suspend_entered;		/* Early suspend state */
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

	unsigned char *fw_code_vliw;		/* VLIW */
	unsigned char *fw_code_cga;		/* CGA */
	unsigned char *fw_code_cga_sa;		/* CGA for SoundAlive */
	unsigned char *fw_data;			/* DATA */
	int alt_fw_loaded;                      /* Alt-Firmware State */

	dma_addr_t fw_mem_base;			/* Base memory for FW */
	unsigned long fw_mem_base_pa;		/* Physical address of base */
	unsigned long fw_code_vliw_pa;		/* Physical address of VLIW */
	unsigned long fw_code_cga_pa;		/* Physical address of CGA */
	unsigned long fw_code_cga_sa_pa;	/* Physical address of CGA_SA */
	unsigned long fw_data_pa;		/* Physical address of DATA */
	unsigned long fw_code_vliw_size;	/* Size of VLIW */
	unsigned long fw_code_cga_size;		/* Size of CGA */
	unsigned long fw_code_cga_sa_size;	/* Size of CGA for SoundAlive */
	unsigned long fw_data_size;		/* Size of DATA */

	unsigned char *ibuf0;			/* IBUF0 in DRAM */
	unsigned char *ibuf1;			/* IBUF1 in DRAM */
	unsigned long ibuf0_pa;			/* Physical address */
	unsigned long ibuf1_pa;			/* Physical address */
	unsigned long obuf0_pa;			/* Physical address */
	unsigned long obuf1_pa;			/* Physical address */
	unsigned long obuf_size;		/* Current OBUF size */
	unsigned long vliw_rp;			/* Current VLIW address */
	unsigned char *wbuf;			/* WBUF in DRAM */
	unsigned long wbuf_pa;			/* Physical address */
	unsigned long wbuf_pos;			/* Write pointer */
	unsigned long wbuf_fill_size;		/* Total size by user write() */

	unsigned char *pcm_dump;		/* PCM dump buffer in DRAM */
	unsigned long pcm_dump_pa;		/* Physical address */
	unsigned long pcm_dump_enabled;		/* PCM dump switch */
	unsigned long pcm_dump_idle;		/* PCM dump count from RP */
	int pcm_dump_cnt;			/* PCM dump open count */

	unsigned long effect_enabled;		/* Effect enable switch */
	unsigned long effect_def;		/* Effect definition */
	unsigned long effect_eq_user;		/* Effect EQ user */
	unsigned long effect_speaker;		/* Effect Speaker mode */
};

static struct s5p_rp_info s5p_rp;

static DEFINE_MUTEX(rp_mutex);

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Write);
DECLARE_WAIT_QUEUE_HEAD(WaitQueue_EOS);

#ifdef CONFIG_SND_S5P_RP_DEBUG
struct timeval time_irq, time_write;
static char rp_fw_name[4][16] = {
	"VLIW  ", "CGA   ", "CGA-SA", "DATA  "
};
#endif

/* Below functions should be accessed by i2s driver only. */
volatile int s5p_rp_is_opened = 0;
volatile int s5p_rp_is_running = 0;
EXPORT_SYMBOL(s5p_rp_is_opened);
EXPORT_SYMBOL(s5p_rp_is_running);

/* I2S resume/suspend are required for AudioSubsystem. by s3c64xx-i2s-v4.c */
void s5p_i2s_do_resume_for_rp(void);
void s5p_i2s_do_suspend_for_rp(void);

static void s5p_rp_set_effect_apply(void);
static void s5p_rp_set_gain_apply(void);

#ifdef CONFIG_SND_S5P_RP_DEBUG
static char rp_op_level_str[][10] = { "LPA", "AFTR" };
#endif
int s5p_rp_get_op_level(void)
{
	int op_lvl;

	if (s5p_rp_is_running) {
#ifdef _USE_PCM_DUMP_
		if (s5p_rp.pcm_dump_enabled)
			op_lvl = 1;				/* AFTR */
		else
#endif
			op_lvl = s5p_rp.dram_in_use ? 1 : 0;	/* AFTR / LPA */
	} else {
		op_lvl = 0;					/* IDLE */
	}

#ifdef CONFIG_SND_S5P_RP_DEBUG
	s5pdbg("OP level [%s]\n", rp_op_level_str[op_lvl]);
#endif
	return op_lvl;
}
EXPORT_SYMBOL(s5p_rp_get_op_level);

static void s5p_rp_commbox_init(void)
{
	s5pdbg("Commbox initialized\n");

	writel(0x00000011, s5p_rp.commbox + RP_CFGR);

	writel(0x00000000, s5p_rp.commbox + RP_INTERRUPT);
	writel(0x00000000, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);
	writel(0x00000001, s5p_rp.commbox + RP_PENDING);

	writel(0x00000000, s5p_rp.commbox + RP_FRAME_INDEX);
	writel(0x00000000, s5p_rp.commbox + RP_EFFECT_DEF);
	writel(0x00000000, s5p_rp.commbox + RP_EQ_USER_DEF);

#ifdef _USE_POSTPROCESS_SKIP_TEST_
	/* Enable ARM postprocessing mode for MP3 decoding test */
	/* C110 only
	writel(0x00000001, s5p_rp.commbox + RP_SW_DEF);
	*/
#endif

	writel(s5p_rp.ibuf0_pa, s5p_rp.commbox + RP_BITSTREAM_BUFF_DRAM_ADDR0);
	writel(s5p_rp.ibuf1_pa, s5p_rp.commbox + RP_BITSTREAM_BUFF_DRAM_ADDR1);

	writel(0x00000001, s5p_rp.commbox + RP_CFGR);	/* TEXT in IMEM */
	writel(0x00000000, s5p_rp.special + 0x007C);	/* Clear VLIW address */

	writel(s5p_rp.fw_data_pa, s5p_rp.commbox + RP_DATA_START_ADDR);
	writel(s5p_rp.pcm_dump_pa, s5p_rp.commbox + RP_PCM_DUMP_ADDR);
	writel(s5p_rp.fw_code_cga_pa, s5p_rp.commbox + RP_CONF_START_ADDR);
	writel(s5p_rp.fw_code_cga_sa_pa, s5p_rp.commbox + RP_LOAD_CGA_SA_ADDR);

	s5p_rp_set_effect_apply();
	s5p_rp_set_gain_apply();
}

static void s5p_rp_commbox_deinit(void)
{
	s5pdbg("Commbox deinitialized\n");

	/* Reset value */
	writel(0x00000001, s5p_rp.commbox + RP_PENDING);
	writel(0x00000000, s5p_rp.commbox + RP_INTERRUPT);
	writel(0x00000000, s5p_rp.special + 0x007C);	/* Clear VLIW address */
}

static void s5p_rp_fw_download(void)
{
	unsigned long n;
	unsigned long *pval;

#ifdef CONFIG_SND_S5P_RP_DEBUG
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	/* Fill ICACHE with first 64KB area */
	writel(0x00000011, s5p_rp.commbox + RP_CFGR);	/* ARM access I$ */

	pval = (unsigned long *)s5p_rp.fw_code_vliw;
	for (n = 0; n < 0x10000; n += 4, pval++)
		writel(ENDIAN_CHK_CONV(*pval), s5p_rp.icache + n);

	/* SRP access I$ */
	if (s5p_rp.op_mode == RP_ARM_INTR_CODE_ULP_CTYPE)
		writel(0x0000001D, s5p_rp.commbox + RP_CFGR);
	else
		writel(0x0000005D, s5p_rp.commbox + RP_CFGR);	/* I2S wakeup */

	/* Copy VLIW code to iRAM (Operation mode C) */
	if (s5p_rp.op_mode == RP_ARM_INTR_CODE_ULP_CTYPE) {
		pval = (unsigned long *)s5p_rp.fw_code_vliw;
		for (n = 0; n < s5p_rp.fw_code_vliw_size; n += 4, pval++)
			writel(ENDIAN_CHK_CONV(*pval), s5p_rp.iram_imem + n);
	}

#ifdef CONFIG_SND_S5P_RP_DEBUG
	do_gettimeofday(&end);

	s5pdbg("Firmware Download Time : %lu.%06lu seconds.\n",
		end.tv_sec - begin.tv_sec, end.tv_usec - begin.tv_usec);
#endif
}

static void s5p_rp_set_default_fw(void)
{
	/* Initialize Commbox & default parameters */
	s5p_rp_commbox_init();

	/* Download default Firmware */
	s5p_rp_fw_download();
}

static void s5p_rp_flush_ibuf(void)
{
	memset(s5p_rp.ibuf0, 0xFF, s5p_rp.ibuf_size);
	memset(s5p_rp.ibuf1, 0xFF, s5p_rp.ibuf_size);

	s5p_rp.ibuf_next = 0;			/* Next IBUF is IBUF0 */
	s5p_rp.ibuf_empty[0] = 1;
	s5p_rp.ibuf_empty[1] = 1;
#ifdef _USE_START_WITH_BUF0_
	s5p_rp.ibuf_req_skip = 1;
#endif
	s5p_rp.wbuf_pos = 0;
	s5p_rp.wbuf_fill_size = 0;
}

static void s5p_rp_flush_obuf(void)
{
	int n;

	if (s5p_rp.obuf_size) {
		for (n = 0; n < s5p_rp.obuf_size; n += 4) {
			writel(0, s5p_rp.obuf0 + n);
			writel(0, s5p_rp.obuf1 + n);
		}
	}
}

static void s5p_rp_reset_frame_counter(void)
{
	s5p_rp.frame_count = 0;
	s5p_rp.frame_count_base = 0;
}

static unsigned long s5p_rp_get_frame_counter(void)
{
	unsigned long val;

	val = readl(s5p_rp.commbox + RP_FRAME_INDEX);
	s5p_rp.frame_count = s5p_rp.frame_count_base + val;

	return s5p_rp.frame_count;
}

static void s5p_rp_check_stream_info(void)
{
	if (!s5p_rp.channel) {
		s5p_rp.channel = readl(s5p_rp.commbox
				+ RP_ARM_INTERRUPT_CODE);
		s5p_rp.channel >>= RP_ARM_INTR_CODE_CHINF_SHIFT;
		s5p_rp.channel &= RP_ARM_INTR_CODE_CHINF_MASK;
		if (s5p_rp.channel) {
			s5pdbg("Channel = %lu\n", s5p_rp.channel);
		}
	}

	if (!s5p_rp.frame_size) {
		switch (readl(s5p_rp.commbox
			+ RP_ARM_INTERRUPT_CODE)
			& RP_ARM_INTR_CODE_FRAME_MASK) {
		case RP_ARM_INTR_CODE_FRAME_1152:
			s5p_rp.frame_size = 1152;
			break;
		case RP_ARM_INTR_CODE_FRAME_1024:
			s5p_rp.frame_size = 1024;
			break;
		case RP_ARM_INTR_CODE_FRAME_576:
			s5p_rp.frame_size = 576;
			break;
		case RP_ARM_INTR_CODE_FRAME_384:
			s5p_rp.frame_size = 384;
			break;
		default:
			s5p_rp.frame_size = 0;
			break;
		}
		if (s5p_rp.frame_size) {
			s5pdbg("Frame size = %lu\n", s5p_rp.frame_size);
		}
	}
}

static void s5p_rp_reset(void)
{
	wake_up_interruptible(&WaitQueue_Write);

	writel(0x00000001, s5p_rp.commbox + RP_PENDING);

	/* Operation mode (A/B/C) & PCM dump */
	writel((s5p_rp.pcm_dump_enabled ? RP_ARM_INTR_CODE_PCM_DUMP_ON : 0) |
		s5p_rp.op_mode, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);
	writel(s5p_rp.vliw_rp, s5p_rp.commbox + RP_CODE_START_ADDR);
	writel(s5p_rp.obuf0_pa, s5p_rp.commbox + RP_PCM_BUFF0);
	writel(s5p_rp.obuf1_pa, s5p_rp.commbox + RP_PCM_BUFF1);
	writel(s5p_rp.obuf_size >> 2, s5p_rp.commbox + RP_PCM_BUFF_SIZE);

	writel(0x00000000, s5p_rp.commbox + RP_FRAME_INDEX);
	writel(0x00000000, s5p_rp.commbox + RP_READ_BITSTREAM_SIZE);
	writel(s5p_rp.ibuf_size, s5p_rp.commbox + RP_BITSTREAM_BUFF_DRAM_SIZE);
	writel(_BITSTREAM_SIZE_MAX_, s5p_rp.commbox + RP_BITSTREAM_SIZE);
	s5p_rp_set_effect_apply();
	writel(0x00000000, s5p_rp.commbox + RP_CONT);	/* RESET */
	writel(0x00000001, s5p_rp.commbox + RP_INTERRUPT);

	/* VLIW address should be set after rp reset */
	writel(s5p_rp.vliw_rp, s5p_rp.special + 0x007C);

	s5p_rp.error_info = 0;				/* Clear Error Info */
	s5p_rp.frame_count_base = s5p_rp.frame_count;	/* Store Total Count */
	s5p_rp.wait_for_eos = 0;
	s5p_rp.stop_after_eos = 0;
	s5p_rp.pause_request = 0;
	s5p_rp.auto_paused = 0;
	s5p_rp.decoding_started = 0;
	s5p_rp.dram_in_use = 0;
	s5p_rp.pcm_dump_idle = 0;
#ifdef _USE_EOS_TIMEOUT_
	s5p_rp.timeout_eos_enabled = 0;
	s5p_rp.timeout_read_size = _BITSTREAM_SIZE_MAX_;
#endif
/* Do not control s5p_rp_is_running state
	s5p_rp_is_running = 0;
*/
}

static void s5p_rp_pause(void)
{
	unsigned long arm_intr_code = readl(s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);

#if 0	/* C210 SRP controls idma by itself */
	s5p_i2s_idma_pause();
#endif
	arm_intr_code |= RP_ARM_INTR_CODE_PAUSE_REQ;
	writel(arm_intr_code, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);

/* Do not control s5p_rp_is_running state
	s5p_rp_is_running = 0;
*/
}

static void s5p_rp_pause_request(void)
{
#if 0	/* for EVT0 W/A */
	/* RP will pause at next I2S interrupt */
	s5pdbg("Pause requsted\n");
	s5p_rp.pause_request = 1;
#elif 1
	int n;
	unsigned long arm_intr_code = readl(s5p_rp.commbox +
					RP_ARM_INTERRUPT_CODE);

	s5pdbg("Pause requsted\n");
	if (!s5p_rp_is_running) {
		s5pdbg("Pause ignored\n");
		return;
	}

	arm_intr_code |= RP_ARM_INTR_CODE_PAUSE_REQ;
	writel(arm_intr_code, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);

	for (n = 0; n < 100; n++) {
		if (readl(s5p_rp.commbox + RP_ARM_INTERRUPT_CODE) &
			RP_ARM_INTR_CODE_PAUSE_STA)
			break;
		msleep_interruptible(10);
	}
#if 0	/* C210 SRP controls idma by itself */
	s5p_i2s_idma_pause();
#endif
	s5p_rp_is_running = 0;
	s5pdbg("Pause done\n");
#endif
}

static void s5p_rp_continue(void)
{
	unsigned long arm_intr_code = readl(s5p_rp.commbox +
					RP_ARM_INTERRUPT_CODE);

	arm_intr_code &= ~(RP_ARM_INTR_CODE_PAUSE_REQ |
				RP_ARM_INTR_CODE_PAUSE_STA);
	writel(arm_intr_code, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);

/* Do not control s5p_rp_is_running state
	s5p_rp_is_running = 1;
*/
}

static void s5p_rp_stop(void)
{
	writel(0x00000001, s5p_rp.commbox + RP_PENDING);
	s5p_i2s_idma_stop();
	s5p_rp_flush_obuf();
	s5p_rp.pause_request = 0;

/* Do not control s5p_rp_is_running state
	s5p_rp_is_running = 0;
*/
}

#ifdef _USE_PCM_DUMP_
static void s5p_rp_set_pcm_dump(int on)
{
	unsigned long arm_intr_code;

	if (s5p_rp.pcm_dump_enabled != on) {
		s5pdbg("PCM Dump [%s]\n", on ? "ON" : "OFF");
		arm_intr_code = readl(s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);

		if (on)
			arm_intr_code |= RP_ARM_INTR_CODE_PCM_DUMP_ON;
		else
			arm_intr_code &= ~RP_ARM_INTR_CODE_PCM_DUMP_ON;

		s5p_rp.pcm_dump_enabled = on;
		writel(arm_intr_code, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);

		if (!s5p_rp.pcm_dump_enabled)	/* Clear dump buffer */
			memset(s5p_rp.pcm_dump, 0, _PCM_DUMP_SIZE_);
	}
}
#endif

static void s5p_rp_set_effect_apply(void)
{
	unsigned long arm_intr_code = readl(s5p_rp.commbox +
					RP_ARM_INTERRUPT_CODE);

	writel(s5p_rp.effect_def | s5p_rp.effect_speaker,
		s5p_rp.commbox + RP_EFFECT_DEF);
	writel(s5p_rp.effect_eq_user, s5p_rp.commbox + RP_EQ_USER_DEF);

	arm_intr_code &= ~RP_ARM_INTR_CODE_SA_ON;
	arm_intr_code |= s5p_rp.effect_enabled ? RP_ARM_INTR_CODE_SA_ON : 0;
	writel(arm_intr_code, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);
}

static void s5p_rp_effect_trigger(void)
{
	unsigned long arm_intr_code = readl(s5p_rp.commbox +
					RP_ARM_INTERRUPT_CODE);

	writel(s5p_rp.effect_def | s5p_rp.effect_speaker,
		s5p_rp.commbox + RP_EFFECT_DEF);
	writel(s5p_rp.effect_eq_user, s5p_rp.commbox + RP_EQ_USER_DEF);

	arm_intr_code &= ~RP_ARM_INTR_CODE_SA_ON;
	arm_intr_code |= s5p_rp.effect_enabled ? RP_ARM_INTR_CODE_SA_ON : 0;
	writel(arm_intr_code, s5p_rp.commbox + RP_ARM_INTERRUPT_CODE);
}

static void s5p_rp_set_gain_apply(void)
{
	writel((s5p_rp.gain * s5p_rp.gain_subl) / 100,
			s5p_rp.commbox + RP_GAIN_CTRL_FACTOR_L);
	writel((s5p_rp.gain * s5p_rp.gain_subr) / 100,
			s5p_rp.commbox + RP_GAIN_CTRL_FACTOR_R);
}

static irqreturn_t s5p_rp_i2s_irq(int irqno, void *dev_id)
{
	if (s5p_rp_is_running) {
		if (s5p_i2s_idma_irq_callback()) {
			if (s5p_rp.pause_request) {
				s5pdbg("Pause_req after IDMA irq...\n");
				s5p_rp_pause();
				s5p_rp.pause_request = 0;
				s5p_rp_is_running = 0;
			}

			writel(0x00000000, s5p_rp.commbox + RP_PENDING);
		}
	}
	return IRQ_HANDLED;
}

static int s5p_rp_i2s_request_irq(void)
{
	int ret;

	if (!s5p_rp.i2s_irq_enabled) {
		ret = request_irq(IRQ_I2S0, s5p_rp_i2s_irq, 0, "s3c-i2s", &s5p_rp);
		if (ret < 0) {
			s5pdbg("fail to claim i2s irq, ret = %d\n", ret);
			return ret;
		}
		s5p_rp.i2s_irq_enabled = 1;
	}

	return 0;
}

static int s5p_rp_i2s_free_irq(void)
{
	if (s5p_rp.i2s_irq_enabled) {
		free_irq(IRQ_I2S0, &s5p_rp);
		s5p_rp.i2s_irq_enabled = 0;
	}

	return 0;
}

static void s5p_rp_init_op_mode(int mode)
{
	s5p_rp.op_mode = mode;

	if (s5p_rp.op_mode == RP_ARM_INTR_CODE_ULP_CTYPE) {
		s5p_rp.vliw_rp   = RP_IRAM_BASE + _IMEM_OFFSET_;
		s5p_rp.obuf0_pa  = RP_IRAM_BASE + _OBUF0_OFFSET_C_;
		s5p_rp.obuf1_pa  = RP_IRAM_BASE + _OBUF1_OFFSET_C_;
		s5p_rp.obuf0     = s5p_rp.iram  + _OBUF0_OFFSET_C_;
		s5p_rp.obuf1     = s5p_rp.iram  + _OBUF1_OFFSET_C_;
		s5p_rp.obuf_size = _OBUF_SIZE_C_;
		s5p_rp_i2s_free_irq();
	} else {
		s5p_rp.vliw_rp   = s5p_rp.fw_code_vliw_pa;
		s5p_rp.obuf0_pa  = RP_IRAM_BASE + _OBUF0_OFFSET_AB_;
		s5p_rp.obuf1_pa  = RP_IRAM_BASE + _OBUF1_OFFSET_AB_;
		s5p_rp.obuf0     = s5p_rp.iram  + _OBUF0_OFFSET_AB_;
		s5p_rp.obuf1     = s5p_rp.iram  + _OBUF1_OFFSET_AB_;
		s5p_rp.obuf_size = _OBUF_SIZE_AB_;
		s5p_rp_i2s_request_irq();
	}
}

#ifdef _USE_EOS_TIMEOUT_
static void s5p_rp_setup_timeout_eos(void)
{
	unsigned long remaining_bytes;
	unsigned long remaining_msec;

	s5p_rp.timeout_eos_enabled = 1;
	s5p_rp.timeout_read_size = readl(s5p_rp.commbox + RP_READ_BITSTREAM_SIZE);

	remaining_bytes = readl(s5p_rp.commbox + RP_BITSTREAM_SIZE)
				- s5p_rp.timeout_read_size;

	if (remaining_bytes > (s5p_rp.ibuf_size * 3))
		remaining_bytes = s5p_rp.ibuf_size * 3;

	remaining_bytes += _FWBUF_SIZE_;
	remaining_msec = remaining_bytes * 1000 / 4096;	/* 32kbps at worst */
	remaining_msec += (s5p_rp.obuf_size * 2) * 1000 / 44100 / 4;

	do_gettimeofday(&s5p_rp.timeout_eos);
	s5p_rp.timeout_eos.tv_sec += remaining_msec / 1000;
	s5p_rp.timeout_eos.tv_usec += (remaining_msec % 1000) * 1000;
	if (s5p_rp.timeout_eos.tv_usec >= 1000000) {
		s5p_rp.timeout_eos.tv_sec++;
		s5p_rp.timeout_eos.tv_usec -= 1000000;
	}
}

static int s5p_rp_is_timeout_eos(void)
{
	struct timeval time_now;

	do_gettimeofday(&time_now);
	if ((time_now.tv_sec > s5p_rp.timeout_eos.tv_sec) ||
		((time_now.tv_sec == s5p_rp.timeout_eos.tv_sec) &&
		(time_now.tv_usec > s5p_rp.timeout_eos.tv_usec))) {
		printk(KERN_INFO "EOS Timeout at %lu.%06lu seconds.\n",
			time_now.tv_sec, time_now.tv_usec);
		return 1;
	}

	return 0;
}
#endif

#ifdef _USE_POSTPROCESS_SKIP_TEST_
struct timeval time_open, time_release;
#endif
static int s5p_rp_open(struct inode *inode, struct file *file)
{
	mutex_lock(&rp_mutex);
	if (s5p_rp_is_opened) {
		s5pdbg("s5p_rp_open() - SRP is already opened.\n");
		mutex_unlock(&rp_mutex);
		return -1;			/* Fail */
	}

#ifdef _USE_POSTPROCESS_SKIP_TEST_
	do_gettimeofday(&time_open);
#endif

	s5p_rp_is_opened = 1;
	mutex_unlock(&rp_mutex);

	s5p_i2s_do_resume_for_rp();

	if (!(file->f_flags & O_NONBLOCK)) {
		s5pdbg("s5p_rp_open() - Block Mode\n");
		s5p_rp.block_mode = 1;
	} else {
		s5pdbg("s5p_rp_open() - NonBlock Mode\n");
		s5p_rp.block_mode = 0;
	}

	s5p_rp.channel = 0;
	s5p_rp.frame_size = 0;
	s5p_rp_reset_frame_counter();
	s5p_rp_set_default_fw();

	return 0;
}

static int s5p_rp_release(struct inode *inode, struct file *file)
{
	s5pdbg("s5p_rp_release()\n");

	if (s5p_rp_is_running) {		/* Still running? */
		s5pdbg("Stop (release)\n");
		s5p_rp_stop();
		s5p_rp_is_running = 0;
		s5p_rp.decoding_started = 0;
	}

	s5p_rp_commbox_deinit();		/* Reset commbox */
	s5p_rp_is_opened = 0;
	s5p_i2s_do_suspend_for_rp();		/* I2S suspend */

#ifdef _USE_POSTPROCESS_SKIP_TEST_
	do_gettimeofday(&time_release);
	printk(KERN_INFO "S5P_RP: Usage period : %lu.%06lu seconds.\n",
		time_release.tv_sec - time_open.tv_sec,
		time_release.tv_usec - time_open.tv_usec);
#endif

	return 0;
}

static ssize_t s5p_rp_read(struct file *file, char *buffer,
				size_t size, loff_t *pos)
{
	s5pdbg("s5p_rp_read()\n");

	mutex_lock(&rp_mutex);

	mutex_unlock(&rp_mutex);

	return -1;
}

static ssize_t s5p_rp_write(struct file *file, const char *buffer,
				size_t size, loff_t *pos)
{
	unsigned long frame_idx;

	s5pdbg("s5p_rp_write(%d bytes)\n", size);

	mutex_lock(&rp_mutex);
	if (s5p_rp.decoding_started &&
		(!s5p_rp_is_running || s5p_rp.auto_paused)) {
		s5pdbg("Resume RP\n");
		s5p_rp_flush_obuf();
		s5p_rp_continue();
		s5p_rp_is_running = 1;
		s5p_rp.auto_paused = 0;
	}
	mutex_unlock(&rp_mutex);

	if (s5p_rp.wbuf_pos > s5p_rp.ibuf_size * 2) {
		printk(KERN_ERR "S5P_RP: wbuf_pos is full (0x%08lX), frame(0x%08X)\n",
			s5p_rp.wbuf_pos, readl(s5p_rp.commbox + RP_FRAME_INDEX));
		return 0;
	} else if (size > s5p_rp.ibuf_size) {
		printk(KERN_ERR "S5P_RP: wr size error (0x%08X)\n", size);
		return -EFAULT;
	} else {
		if (copy_from_user(&s5p_rp.wbuf[s5p_rp.wbuf_pos], buffer, size))
			return -EFAULT;
	}

	s5p_rp.wbuf_pos += size;
	s5p_rp.wbuf_fill_size += size;
	if (s5p_rp.wbuf_pos < s5p_rp.ibuf_size) {
		frame_idx = readl(s5p_rp.commbox + RP_FRAME_INDEX);
		while (!s5p_rp.early_suspend_entered &&
			s5p_rp.decoding_started && s5p_rp_is_running) {
			if (readl(s5p_rp.commbox + RP_READ_BITSTREAM_SIZE)
				+ s5p_rp.ibuf_size * 2 >= s5p_rp.wbuf_fill_size)
				break;
			if (readl(s5p_rp.commbox + RP_FRAME_INDEX) > frame_idx + 2)
				break;
			msleep_interruptible(2);
		}

		return size;
	}

	if (!s5p_rp.ibuf_empty[s5p_rp.ibuf_next]) {	/* IBUF not available */
		if (file->f_flags & O_NONBLOCK) {
			return -1;	/* return Error at NonBlock mode */
		}

		/* Sleep until IBUF empty interrupt */
		s5pdbg("s5p_rp_write() enter to sleep until IBUF empty INT\n");
		interruptible_sleep_on_timeout(&WaitQueue_Write, HZ / 2);
		s5pdbg("s5p_rp_write() wake up\n");
		if (!s5p_rp.ibuf_empty[s5p_rp.ibuf_next]) {	/* not ready? */
			return size;
		}
	}

	mutex_lock(&rp_mutex);
	if (s5p_rp.ibuf_next == 0) {
		memcpy(s5p_rp.ibuf0, s5p_rp.wbuf, s5p_rp.ibuf_size);
		s5pdbg("Fill IBUF0\n");
		s5p_rp.ibuf_empty[0] = 0;
		s5p_rp.ibuf_next = 1;
	} else {
		memcpy(s5p_rp.ibuf1, s5p_rp.wbuf, s5p_rp.ibuf_size);
		s5pdbg("Fill IBUF1\n");
		s5p_rp.ibuf_empty[1] = 0;
		s5p_rp.ibuf_next = 0;
	}

	memcpy(s5p_rp.wbuf, &s5p_rp.wbuf[s5p_rp.ibuf_size], s5p_rp.ibuf_size);
	s5p_rp.wbuf_pos -= s5p_rp.ibuf_size;

#ifndef _USE_START_WITH_BUF0_
	if (!s5p_rp.ibuf_empty[0] && !s5p_rp.ibuf_empty[1]) {
#endif
		if (!s5p_rp.decoding_started) {
			s5pdbg("Start RP decoding!!\n");
			writel(0x00000000, s5p_rp.commbox + RP_PENDING);
			s5p_rp_is_running = 1;
			s5p_rp.decoding_started = 1;
		}
#ifndef _USE_START_WITH_BUF0_
	}
#endif

#ifdef CONFIG_SND_S5P_RP_DEBUG
	do_gettimeofday(&time_write);
	s5pdbg("IRQ to write-func Time : %lu.%06lu seconds.\n",
		time_write.tv_sec - time_irq.tv_sec,
		time_write.tv_usec - time_irq.tv_usec);
#endif
	mutex_unlock(&rp_mutex);

	if (s5p_rp.error_info) {	/* RP Decoding Error occurred? */
		return -1;
	}

	return size;
}

static void s5p_rp_write_last(void)
{
	s5pdbg("Send remained data, %lu Bytes (Total: %08lX)\n",
		s5p_rp.wbuf_pos ? s5p_rp.wbuf_pos : s5p_rp.ibuf_size,
		s5p_rp.wbuf_fill_size);
	memset(&s5p_rp.wbuf[s5p_rp.wbuf_pos], 0xFF,
		s5p_rp.ibuf_size - s5p_rp.wbuf_pos);
	if (s5p_rp.ibuf_next == 0) {
		memcpy(s5p_rp.ibuf0, s5p_rp.wbuf, s5p_rp.ibuf_size);
		s5pdbg("Fill IBUF0 (final)\n");
		s5p_rp.ibuf_empty[0] = 0;
		s5p_rp.ibuf_next = 1;
	} else {
		memcpy(s5p_rp.ibuf1, s5p_rp.wbuf, s5p_rp.ibuf_size);
		s5pdbg("Fill IBUF1 (final)\n");
		s5p_rp.ibuf_empty[1] = 0;
		s5p_rp.ibuf_next = 0;
	}
	s5p_rp.wbuf_pos = 0;
	writel(s5p_rp.wbuf_fill_size, s5p_rp.commbox + RP_BITSTREAM_SIZE);

#ifdef _USE_EOS_TIMEOUT_
	s5p_rp_setup_timeout_eos();
#endif
}

static int s5p_rp_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	unsigned long val;
	int ret_val = 0;

	mutex_lock(&rp_mutex);

	s5pdbg("s5p_rp_ioctl(cmd:: %08X)\n", cmd);

	switch (cmd) {
	case S5P_RP_INIT:
		val = arg;
		if ((val >= 4*1024) && (val <= _IBUF_SIZE_)) {
			s5pdbg("Init, IBUF size [%ld], OBUF size [%ld]\n",
				val, s5p_rp.obuf_size);
			s5p_rp.ibuf_size = val;
			s5p_rp_flush_ibuf();
			s5p_rp_reset();
			s5p_rp_is_running = 0;
		} else {
			s5pdbg("Init error, IBUF size [%ld]\n", val);
			ret_val = -1;
		}
		break;

	case S5P_RP_DEINIT:
		s5pdbg("Deinit\n");
		writel(0x00000001, s5p_rp.commbox + RP_PENDING);
		s5p_rp_is_running = 0;
		break;

	case S5P_RP_PAUSE:
		s5pdbg("Pause\n");
#ifdef _USE_EOS_TIMEOUT_
		s5p_rp.timeout_eos_enabled = 0;
#endif
		s5p_rp_pause_request();
		break;

	case S5P_RP_STOP:
		s5pdbg("Stop\n");
		s5p_rp_stop();
		s5p_rp_is_running = 0;
		s5p_rp.decoding_started = 0;
		break;

	case S5P_RP_FLUSH:
		/* Do not change s5p_rp_is_running state during flush */
		s5pdbg("Flush\n");
		s5p_rp_stop();
		s5p_rp_flush_ibuf();
		s5p_rp_set_default_fw();
		s5p_rp_reset();
		break;

	case S5P_RP_SEND_EOS:
		s5pdbg("Send EOS\n");
#ifdef _USE_EOS_TIMEOUT_
		printk(KERN_INFO "S5P_RP: Send EOS with timeout\n");
		s5p_rp_setup_timeout_eos();
#endif
		if (s5p_rp.wbuf_fill_size == 0) {	/* No data? */
			s5p_rp.stop_after_eos = 1;
		} else if (s5p_rp.wbuf_fill_size < s5p_rp.ibuf_size * 2) {
			s5p_rp_write_last();
			if (s5p_rp.ibuf_empty[s5p_rp.ibuf_next])
				s5p_rp_write_last();	/* Fill one more IBUF */

			s5pdbg("Start RP decoding!!\n");
			writel(0x00000000, s5p_rp.commbox + RP_PENDING);
			s5p_rp_is_running = 1;
			s5p_rp.wait_for_eos = 1;
		} else if (s5p_rp.ibuf_empty[s5p_rp.ibuf_next]) {
			s5p_rp_write_last();		/* Last data */
			s5p_rp.wait_for_eos = 1;
		} else {
			s5p_rp.wait_for_eos = 1;
		}
		break;

	case S5P_RP_RESUME_EOS:
		s5pdbg("Resume after EOS pause\n");
		s5p_rp_flush_obuf();
		s5p_rp_continue();
		s5p_rp_is_running = 1;
		s5p_rp.auto_paused = 0;
#ifdef _USE_EOS_TIMEOUT_
		s5p_rp_setup_timeout_eos();
#endif
		break;

	case S5P_RP_STOP_EOS_STATE:
		val = s5p_rp.stop_after_eos;
#ifdef _USE_EOS_TIMEOUT_
		if (s5p_rp.wait_for_eos && s5p_rp.timeout_eos_enabled) {
			if (s5p_rp_is_timeout_eos()) {
				s5p_rp.stop_after_eos = 1;
				val = 1;
			} else if (readl(s5p_rp.commbox + RP_READ_BITSTREAM_SIZE)
				!= s5p_rp.timeout_read_size) {
				s5p_rp_setup_timeout_eos();	/* update timeout */
			}
		}
#endif
		s5pdbg("RP Stop [%s]\n", val == 1 ? "ON" : "OFF");
		if (val) {
			printk(KERN_INFO "S5P_RP: Stop at EOS [0x%08lX:0x%08X]\n",
			s5p_rp.wbuf_fill_size,
			readl(s5p_rp.commbox + RP_READ_BITSTREAM_SIZE));
		}
		ret_val = copy_to_user((unsigned long *)arg,
			&val, sizeof(unsigned long));
		break;

	case S5P_RP_PENDING_STATE:
		val = readl(s5p_rp.commbox + RP_PENDING);
		s5pdbg("RP Pending [%s]\n", val == 1 ? "ON" : "OFF");
		ret_val = copy_to_user((unsigned long *)arg,
			&val, sizeof(unsigned long));
		break;

	case S5P_RP_ERROR_STATE:
		s5pdbg("Error Info [%08lX]\n", s5p_rp.error_info);
		ret_val = copy_to_user((unsigned long *)arg,
			&s5p_rp.error_info, sizeof(unsigned long));
		s5p_rp.error_info = 0;
		break;

	case S5P_RP_DECODED_FRAME_NO:
		val = s5p_rp_get_frame_counter();
		s5pdbg("Decoded Frame No [%ld]\n", val);
		ret_val = copy_to_user((unsigned long *)arg,
			&val, sizeof(unsigned long));
		break;

	case S5P_RP_DECODED_ONE_FRAME_SIZE:
		if (s5p_rp.frame_size) {
			s5pdbg("One Frame Size [%lu]\n", s5p_rp.frame_size);
			ret_val = copy_to_user((unsigned long *)arg,
				&s5p_rp.frame_size, sizeof(unsigned long));
		} else {
			s5pdbg("Frame not decoded yet...\n");
		}
		break;

	case S5P_RP_DECODED_FRAME_SIZE:
		if (s5p_rp.frame_size) {
			val = s5p_rp_get_frame_counter() * s5p_rp.frame_size;
			s5pdbg("Decoded Frame Size [%lu]\n", val);
			ret_val = copy_to_user((unsigned long *)arg,
				&val, sizeof(unsigned long));
		} else {
			s5pdbg("Frame not decoded yet...\n");
		}
		break;

	case S5P_RP_CHANNEL_COUNT:
		if (s5p_rp.channel) {
			s5pdbg("Channel Count [%lu]\n", s5p_rp.channel);
			ret_val = copy_to_user((unsigned long *)arg,
				&s5p_rp.channel, sizeof(unsigned long));
		}
		break;

	default:
		ret_val = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&rp_mutex);

	return ret_val;
}

#ifdef CONFIG_SND_S5P_RP_DEBUG
static unsigned long elapsed_usec_old;
#endif
static irqreturn_t s5p_rp_irq(int irqno, void *dev_id)
{
	int wakeup_req = 0;
	int wakeupEOS_req = 0;
	int pendingoff_req = 0;
	unsigned long irq_code = readl(s5p_rp.commbox + RP_INTERRUPT_CODE);
	unsigned long irq_info = readl(s5p_rp.commbox + RP_INFORMATION);
/*	unsigned long err_info = readl(s5p_rp.commbox + RP_ERROR_CODE);*/
	unsigned long irq_code_req;
#ifdef CONFIG_SND_S5P_RP_DEBUG
	unsigned long elapsed_usec;
#endif
	unsigned long read_bytes;

	read_bytes = readl(s5p_rp.commbox + RP_READ_BITSTREAM_SIZE);

	s5pdbg("IRQ: Code [%08lX], Pending [%s], SPE [%08X], Decoded [%08lX]\n",
		irq_code, readl(s5p_rp.commbox + RP_PENDING) ? "ON" : "OFF",
		readl(s5p_rp.special + 0x007C), read_bytes);
	irq_code &= RP_INTR_CODE_MASK;
	irq_info &= RP_INTR_INFO_MASK;

	if (irq_code & RP_INTR_CODE_REQUEST) {
		irq_code_req = irq_code & RP_INTR_CODE_REQUEST_MASK;
		switch (irq_code_req) {
		case RP_INTR_CODE_NOTIFY_INFO:
			s5p_rp_check_stream_info();
			break;

		case RP_INTR_CODE_IBUF_REQUEST:
		case RP_INTR_CODE_IBUF_REQUEST_ULP:
			if (irq_code_req == RP_INTR_CODE_IBUF_REQUEST_ULP)
				s5p_rp.dram_in_use = 0;
			else
				s5p_rp.dram_in_use = 1;

			s5p_rp_check_stream_info();
#ifdef _USE_START_WITH_BUF0_
			if (s5p_rp.ibuf_req_skip) {	/* Ignoring first req */
				s5p_rp.ibuf_req_skip = 0;
				break;
			}
#endif
			if ((irq_code & RP_INTR_CODE_IBUF_MASK) ==
				RP_INTR_CODE_IBUF0_EMPTY)
				s5p_rp.ibuf_empty[0] = 1;
			else
				s5p_rp.ibuf_empty[1] = 1;

			if (s5p_rp.decoding_started) {
				if (s5p_rp.ibuf_empty[0] && s5p_rp.ibuf_empty[1]) {
					if (s5p_rp.wait_for_eos) {
						s5pdbg("Stop at EOS (buffer empty)\n");
						s5p_rp.stop_after_eos = 1;
						writel(RP_INTR_CODE_POLLINGWAIT,
						s5p_rp.commbox + RP_INTERRUPT_CODE);
						return IRQ_HANDLED;
#ifdef _USE_AUTO_PAUSE_
					} else if (s5p_rp_is_running) {
						s5pdbg("Auto-Pause\n");
						s5p_rp_pause();
						s5p_rp.auto_paused = 1;
						/* Leave running state for i2s */
#endif
					}
				} else {
					pendingoff_req = 1;
					if (s5p_rp.wait_for_eos && s5p_rp.wbuf_pos)
						s5p_rp_write_last();
				}
			}
#ifdef CONFIG_SND_S5P_RP_DEBUG
			do_gettimeofday(&time_irq);
			elapsed_usec = time_irq.tv_sec * 1000000 +
					time_irq.tv_usec;
			s5pdbg("IRQ: IBUF empty ------- Interval [%lu.%06lu]\n",
				(elapsed_usec - elapsed_usec_old) / 1000000,
				(elapsed_usec - elapsed_usec_old) % 1000000);
			elapsed_usec_old = elapsed_usec;
#endif
			if (s5p_rp.block_mode && !s5p_rp.stop_after_eos)
				wakeup_req = 1;
			break;

		case RP_INTR_CODE_ULP:
			s5p_rp.dram_in_use = 0;
			s5p_rp_check_stream_info();
			break;

		case RP_INTR_CODE_DRAM_REQUEST:
			s5p_rp.dram_in_use = 1;
			if (s5p_rp.block_mode && !s5p_rp.stop_after_eos)
				wakeup_req = 1;
			break;

		default:
			break;
		}
	}

	if (irq_code & (RP_INTR_CODE_PLAYDONE | RP_INTR_CODE_ERROR)) {
		s5pdbg("Stop at EOS\n");
/*		s5p_rp_is_running = 0;	leave rp-state as running */
		s5pdbg("Total decoded: %ld frames (RP_read:%08X)\n",
			s5p_rp_get_frame_counter(),
			readl(s5p_rp.commbox + RP_READ_BITSTREAM_SIZE));
		s5p_rp.stop_after_eos = 1;
		writel(RP_INTR_CODE_POLLINGWAIT,
			s5p_rp.commbox + RP_INTERRUPT_CODE);

		return IRQ_HANDLED;
	}

	if (irq_code & RP_INTR_CODE_UART_OUTPUT) {
		printk(KERN_INFO "S5P_RP: UART Code received [0x%08X]\n",
			readl(s5p_rp.commbox + RP_UART_INFORMATION));
		pendingoff_req = 1;
	}

	writel(0x00000000, s5p_rp.commbox + RP_INTERRUPT_CODE);
/*	writel(0x00000000, s5p_rp.commbox + RP_INFORMATION);*/
	writel(0x00000000, s5p_rp.commbox + RP_INTERRUPT);

	if (pendingoff_req)
		writel(0x00000000, s5p_rp.commbox + RP_PENDING);

	if (wakeup_req)
		wake_up_interruptible(&WaitQueue_Write);

	if (wakeupEOS_req)
		wake_up_interruptible(&WaitQueue_EOS);

	return IRQ_HANDLED;
}

static int s5p_rp_ctrl_open(struct inode *inode, struct file *file)
{
/*	s5pdbg("s5p_rp_ctrl_open()\n");*/

	return 0;
}

static int s5p_rp_ctrl_release(struct inode *inode, struct file *file)
{
/*	s5pdbg("s5p_rp_ctrl_release()\n");*/

	return 0;
}

static ssize_t s5p_rp_ctrl_read(struct file *file, char * buffer,
				size_t size, loff_t *pos)
{
/*	s5pdbg("s5p_rp_ctrl_read()\n");*/

	return -1;
}

static ssize_t s5p_rp_ctrl_write(struct file *file, const char *buffer,
				size_t size, loff_t *pos)
{
/*	s5pdbg("s5p_rp_ctrl_write()\n");*/

	return -1;
}

static int s5p_rp_ctrl_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int ret_val = 0;
	unsigned long val, index, size;

#if 0
	s5pdbg("s5p_rp_ctrl_ioctl(cmd:: %08X)\n", cmd);
#endif
	switch (cmd) {
	case S5P_RP_CTRL_SET_GAIN:
		s5pdbg("CTRL: Gain [0x%08lX]\n", arg);
		s5p_rp.gain = arg;
		if (s5p_rp_is_opened)		/* Change gain immediately */
			s5p_rp_set_gain_apply();
		break;

	case S5P_RP_CTRL_SET_GAIN_SUB_LR:
		s5p_rp.gain_subl = arg >> 16;
		if (s5p_rp.gain_subl > 100)
			s5p_rp.gain_subl = 100;

		s5p_rp.gain_subr = arg & 0xFFFF;
		if (s5p_rp.gain_subr > 100)
			s5p_rp.gain_subr = 100;

		s5pdbg("CTRL: Gain sub [L:%03ld, R:%03ld]\n",
			s5p_rp.gain_subl, s5p_rp.gain_subr);
		if (s5p_rp_is_opened)		/* Change gain immediately */
			s5p_rp_set_gain_apply();
		break;

	case S5P_RP_CTRL_GET_PCM_1KFRAME:
		s5pdbg("CTRL: Get PCM 1K Frame\n");
		ret_val = copy_to_user((unsigned long *)arg,
					s5p_rp.pcm_dump, _PCM_DUMP_SIZE_);
		break;

#ifdef _USE_PCM_DUMP_
	case S5P_RP_CTRL_PCM_DUMP_OP:
		if (arg == 1 && s5p_rp.early_suspend_entered == 0) {
			s5p_rp.pcm_dump_cnt++;
			if (s5p_rp.pcm_dump_cnt == 1)
				s5p_rp_set_pcm_dump(1);
		} else {
			s5p_rp.pcm_dump_cnt--;
			if (s5p_rp.pcm_dump_cnt <= 0) {
				s5p_rp.pcm_dump_cnt = 0;
				s5p_rp_set_pcm_dump(0);
			}

		}
		break;
#endif

	case S5P_RP_CTRL_EFFECT_ENABLE:
		arg &= 0x01;
		s5pdbg("CTRL: Effect switch %s\n", arg ? "ON" : "OFF");
		if (s5p_rp.effect_enabled != arg) {
			s5p_rp.effect_enabled = arg;
			if (s5p_rp_is_running)
				s5p_rp_effect_trigger();
			else if (s5p_rp_is_opened)
				s5p_rp_set_effect_apply();
		}
		break;

	case S5P_RP_CTRL_EFFECT_DEF:
		s5pdbg("CTRL: Effect define\n");
		s5p_rp.effect_def = arg & 0xFFFFFFFE;	/* Mask Speaker mode */
		if (s5p_rp_is_running) {
			writel(s5p_rp.effect_def | s5p_rp.effect_speaker,
				s5p_rp.commbox + RP_EFFECT_DEF);
			s5pdbg("Effect [%s], EFFECT_DEF = 0x%08lX,  EQ_USR = 0x%08lX\n",
				s5p_rp.effect_enabled ? "ON" : "OFF",
				s5p_rp.effect_def | s5p_rp.effect_speaker,
				s5p_rp.effect_eq_user);
		} else if (s5p_rp_is_opened) {
			s5p_rp_set_effect_apply();
		}
		break;

	case S5P_RP_CTRL_EFFECT_EQ_USR:
		s5pdbg("CTRL: Effect EQ user\n");
		s5p_rp.effect_eq_user = arg;
		if (s5p_rp_is_running) {
			writel(s5p_rp.effect_eq_user,
				s5p_rp.commbox + RP_EQ_USER_DEF);
		} else if (s5p_rp_is_opened) {
			s5p_rp_set_effect_apply();
		}
		break;

	case S5P_RP_CTRL_EFFECT_SPEAKER:
		arg &= 0x01;
		s5pdbg("CTRL: Effect Speaker mode %s\n", arg ? "ON" : "OFF");
		if (s5p_rp.effect_speaker != arg) {
			s5p_rp.effect_speaker = arg;
			if (s5p_rp_is_running)
				s5p_rp_effect_trigger();
			else if (s5p_rp_is_opened)
				s5p_rp_set_effect_apply();
		}
		break;

	case S5P_RP_CTRL_IS_OPENED:
		val = (unsigned long)s5p_rp_is_opened;
		s5pdbg("CTRL: RP is [%s]\n",
			val == 1 ? "Opened" : "Not Opened");
		ret_val = copy_to_user((unsigned long *)arg,
					&val, sizeof(unsigned long));
		break;

	case S5P_RP_CTRL_IS_RUNNING:
		val = (unsigned long)s5p_rp_is_running;
		s5pdbg("CTRL: RP is [%s]\n", val == 1 ? "Running" : "Pending");
		ret_val = copy_to_user((unsigned long *)arg,
					&val, sizeof(unsigned long));
		break;

	case S5P_RP_CTRL_GET_OP_LEVEL:
		val = (unsigned long)s5p_rp_get_op_level();
		s5pdbg("CTRL: RP op-level [%s]\n", rp_op_level_str[val]);
		ret_val = copy_to_user((unsigned long *)arg,
					&val, sizeof(unsigned long));
		break;

	case S5P_RP_CTRL_IS_PCM_DUMP:
		val = (unsigned long)s5p_rp.pcm_dump_enabled;
		ret_val = copy_to_user((unsigned long *)arg, &val, sizeof(unsigned long));

		break;

	case S5P_RP_CTRL_ALTFW_STATE:
		val = s5p_rp.alt_fw_loaded;	/* Alt-Firmware State */
		s5pdbg("CTRL: Alt-Firmware %sLoaded\n", val ? "" : "Not ");
		ret_val = copy_to_user((unsigned long *)arg,
					&val, sizeof(unsigned long));
		break;

	case S5P_RP_CTRL_ALTFW_LOAD:		/* Alt-Firmware Loading */
		s5p_rp.alt_fw_loaded = 1;
		index = *(unsigned long *)(arg + (128 * 1024));
		size = *(unsigned long *)(arg + (129 * 1024));
		s5pdbg("CTRL: Alt-Firmware Loading: %s (%lu)\n",
			rp_fw_name[index], size);
		switch (index) {
		case S5P_RP_FW_VLIW:
			s5p_rp.fw_code_vliw_size = size;
			ret_val = copy_from_user(s5p_rp.fw_code_vliw,
					(unsigned long *)arg, size);
			break;
		case S5P_RP_FW_CGA:
			s5p_rp.fw_code_cga_size = size;
			ret_val = copy_from_user(s5p_rp.fw_code_cga,
					(unsigned long *)arg, size);
			break;
		case S5P_RP_FW_CGA_SA:
			s5p_rp.fw_code_cga_sa_size = size;
			ret_val = copy_from_user(s5p_rp.fw_code_cga_sa,
					(unsigned long *)arg, size);
			break;
		case S5P_RP_FW_DATA:
			s5p_rp.fw_data_size = size;
			ret_val = copy_from_user(s5p_rp.fw_data,
					(unsigned long *)arg, size);
			break;
		default:
			break;
		}
		break;

	default:
		ret_val = -ENOIOCTLCMD;
		break;
	}

	return ret_val;
}

static int s5p_rp_prepare_fw_buff(struct device *dev)
{
#if defined(CONFIG_S5P_MEM_CMA) || defined(CONFIG_S5P_MEM_BOOTMEM)
	unsigned long mem_paddr;

#ifdef CONFIG_S5P_MEM_CMA
	struct cma_info mem_info;
	int err;

	err = cma_info(&mem_info, dev, 0);
	if (err) {
		s5pdbg("Failed to get cma info\n");
		return -ENOMEM;
	}
	s5pdbg("cma_info\n\tstart_addr : 0x%08X\n\tend_addr   : 0x%08X"
		"\n\ttotal_size : 0x%08X\n\tfree_size  : 0x%08X\n",
		mem_info.lower_bound, mem_info.upper_bound,
		mem_info.total_size, mem_info.free_size);
	s5pdbg("Allocate memory %dbytes from CMA\n", _BASE_MEM_SIZE_);
	s5p_rp.fw_mem_base = cma_alloc(dev, "srp", _BASE_MEM_SIZE_, 0);
#else /* for CONFIG_S5P_MEM_BOOTMEM */
	s5pdbg("Allocate memory from BOOTMEM\n");
	s5p_rp.fw_mem_base = s5p_get_media_memory_bank(S5P_MDEV_SRP, 0);
#endif
	s5p_rp.fw_mem_base_pa = (unsigned long)s5p_rp.fw_mem_base;
	s5pdbg("fw_mem_base_pa = 0x%08lX \n", s5p_rp.fw_mem_base_pa);
	mem_paddr = s5p_rp.fw_mem_base_pa;

	if (IS_ERR_VALUE(s5p_rp.fw_mem_base_pa)) {
		return -ENOMEM;
	}

	s5p_rp.fw_code_vliw_pa = mem_paddr;
	s5p_rp.fw_code_vliw = phys_to_virt(s5p_rp.fw_code_vliw_pa);
	mem_paddr += _VLIW_SIZE_MAX_;

	s5p_rp.fw_code_cga_pa = mem_paddr;
	s5p_rp.fw_code_cga = phys_to_virt(s5p_rp.fw_code_cga_pa);
	mem_paddr += _CGA_SIZE_MAX_;
	s5p_rp.fw_code_cga_sa_pa = mem_paddr;
	s5p_rp.fw_code_cga_sa = phys_to_virt(s5p_rp.fw_code_cga_sa_pa);
	mem_paddr += _CGA_SIZE_MAX_;

	s5p_rp.fw_data_pa = mem_paddr;
	s5p_rp.fw_data = phys_to_virt(s5p_rp.fw_data_pa);
	mem_paddr += _DATA_SIZE_MAX_;
#else	/* No CMA or BOOTMEM? */

	s5p_rp.fw_code_vliw = dma_alloc_writecombine(0, _VLIW_SIZE_,
			   (dma_addr_t *)&s5p_rp.fw_code_vliw_pa, GFP_KERNEL);
	s5p_rp.fw_code_cga = dma_alloc_writecombine(0, _CGA_SIZE_,
			   (dma_addr_t *)&s5p_rp.fw_code_cga_pa, GFP_KERNEL);
	s5p_rp.fw_code_cga_sa = dma_alloc_writecombine(0, _CGA_SIZE_,
			   (dma_addr_t *)&s5p_rp.fw_code_cga_sa_pa, GFP_KERNEL);
	s5p_rp.fw_data = dma_alloc_writecombine(0, _DATA_SIZE_,
			   (dma_addr_t *)&s5p_rp.fw_data_pa, GFP_KERNEL);
#endif

	s5p_rp.ibuf0 = dma_alloc_writecombine(0, _IBUF_SIZE_,
			   (dma_addr_t *)&s5p_rp.ibuf0_pa, GFP_KERNEL);
	s5p_rp.ibuf1 = dma_alloc_writecombine(0, _IBUF_SIZE_,
			   (dma_addr_t *)&s5p_rp.ibuf1_pa, GFP_KERNEL);
	s5p_rp.wbuf = dma_alloc_writecombine(0, _WBUF_SIZE_,
			   (dma_addr_t *)&s5p_rp.wbuf_pa, GFP_KERNEL);
	s5p_rp.pcm_dump = dma_alloc_writecombine(0, _PCM_DUMP_SIZE_,
			   (dma_addr_t *)&s5p_rp.pcm_dump_pa, GFP_KERNEL);

#ifdef CONFIG_TARGET_LOCALE_NAATT
	s5p_rp.fw_code_vliw_size = rp_fw_vliw_len_att;
	s5p_rp.fw_code_cga_size = rp_fw_cga_len_att;
	s5p_rp.fw_code_cga_sa_size = rp_fw_cga_sa_len_att;
	s5p_rp.fw_data_size = rp_fw_data_len_att;
#else
	s5p_rp.fw_code_vliw_size = rp_fw_vliw_len;
	s5p_rp.fw_code_cga_size = rp_fw_cga_len;
	s5p_rp.fw_code_cga_sa_size = rp_fw_cga_sa_len;
	s5p_rp.fw_data_size = rp_fw_data_len;
#endif

	s5pdbg("VLIW,       VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.fw_code_vliw, s5p_rp.fw_code_vliw_pa);
	s5pdbg("CGA,        VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.fw_code_cga, s5p_rp.fw_code_cga_pa);
	s5pdbg("CGA_SA,     VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.fw_code_cga_sa, s5p_rp.fw_code_cga_sa_pa);
	s5pdbg("DATA,       VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.fw_data, s5p_rp.fw_data_pa);
	s5pdbg("DRAM IBUF0, VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.ibuf0, s5p_rp.ibuf0_pa);
	s5pdbg("DRAM IBUF1, VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.ibuf1, s5p_rp.ibuf1_pa);
	s5pdbg("PCM DUMP,   VA = 0x%08lX, PA = 0x%08lX\n",
		(unsigned long)s5p_rp.pcm_dump, s5p_rp.pcm_dump_pa);

	/* Clear Firmware memory & IBUF */
	memset(s5p_rp.fw_code_vliw, 0, _VLIW_SIZE_);
	memset(s5p_rp.fw_code_cga, 0, _CGA_SIZE_);
	memset(s5p_rp.fw_code_cga_sa, 0, _CGA_SIZE_);
	memset(s5p_rp.fw_data, 0, _DATA_SIZE_);
	memset(s5p_rp.ibuf0, 0xFF, _IBUF_SIZE_);
	memset(s5p_rp.ibuf1, 0xFF, _IBUF_SIZE_);

	/* Copy Firmware */
#ifdef CONFIG_TARGET_LOCALE_NAATT
	memcpy(s5p_rp.fw_code_vliw, rp_fw_vliw_att, s5p_rp.fw_code_vliw_size);
	memcpy(s5p_rp.fw_code_cga, rp_fw_cga_att, s5p_rp.fw_code_cga_size);
	memcpy(s5p_rp.fw_code_cga_sa, rp_fw_cga_sa_att, s5p_rp.fw_code_cga_sa_size);
	memcpy(s5p_rp.fw_data, rp_fw_data_att, s5p_rp.fw_data_size);
#else
	memcpy(s5p_rp.fw_code_vliw, rp_fw_vliw, s5p_rp.fw_code_vliw_size);
	memcpy(s5p_rp.fw_code_cga, rp_fw_cga, s5p_rp.fw_code_cga_size);
	memcpy(s5p_rp.fw_code_cga_sa, rp_fw_cga_sa, s5p_rp.fw_code_cga_sa_size);
	memcpy(s5p_rp.fw_data, rp_fw_data, s5p_rp.fw_data_size);
#endif

	return 0;
}

static int s5p_rp_remove_fw_buff(void)
{
#if defined CONFIG_S5P_MEM_CMA
#elif defined CONFIG_S5P_MEM_BOOTMEM
#else	/* No CMA or BOOTMEM? */
	dma_free_writecombine(0, _VLIW_SIZE_, s5p_rp.fw_code_vliw,
						s5p_rp.fw_code_vliw_pa);
	dma_free_writecombine(0, _CGA_SIZE_, s5p_rp.fw_code_cga,
						s5p_rp.fw_code_cga_pa);
	dma_free_writecombine(0, _CGA_SIZE_, s5p_rp.fw_code_cga_sa,
						s5p_rp.fw_code_cga_sa_pa);
	dma_free_writecombine(0, _DATA_SIZE_, s5p_rp.fw_data,
						s5p_rp.fw_data_pa);
#endif
	dma_free_writecombine(0, _IBUF_SIZE_, s5p_rp.ibuf0, s5p_rp.ibuf0_pa);
	dma_free_writecombine(0, _IBUF_SIZE_, s5p_rp.ibuf1, s5p_rp.ibuf1_pa);
	dma_free_writecombine(0, _WBUF_SIZE_, s5p_rp.wbuf, s5p_rp.wbuf_pa);

	s5p_rp.fw_code_vliw_pa = 0;
	s5p_rp.fw_code_cga_pa = 0;
	s5p_rp.fw_code_cga_sa_pa = 0;
	s5p_rp.fw_data_pa = 0;
	s5p_rp.ibuf0_pa = 0;
	s5p_rp.ibuf1_pa = 0;
	s5p_rp.wbuf_pa = 0;

	return 0;
}

static struct file_operations s5p_rp_fops = {
	.owner		= THIS_MODULE,
	.read		= s5p_rp_read,
	.write		= s5p_rp_write,
	.ioctl		= s5p_rp_ioctl,
	.open		= s5p_rp_open,
	.release	= s5p_rp_release,

};

static struct miscdevice s5p_rp_miscdev = {
	.minor		= RP_DEV_MINOR,
	.name		= "s5p-rp",
	.fops		= &s5p_rp_fops,
};

static struct file_operations s5p_rp_ctrl_fops = {
	.owner		= THIS_MODULE,
	.read		= s5p_rp_ctrl_read,
	.write		= s5p_rp_ctrl_write,
	.ioctl		= s5p_rp_ctrl_ioctl,
	.open		= s5p_rp_ctrl_open,
	.release	= s5p_rp_ctrl_release,

};

static struct miscdevice s5p_rp_ctrl_miscdev = {
	.minor		= RP_CTRL_DEV_MINOR,
	.name		= "s5p-rp_ctrl",
	.fops		= &s5p_rp_ctrl_fops,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
void s5p_rp_early_suspend(struct early_suspend *h)
{
	s5pdbg("early_suspend\n");

	s5p_rp.early_suspend_entered = 1;
	if (s5p_rp_is_running) {
		if (s5p_rp.pcm_dump_cnt > 0)
			s5p_rp_set_pcm_dump(0);
	}
}

void s5p_rp_late_resume(struct early_suspend *h)
{
	s5pdbg("late_resume\n");

	s5p_rp.early_suspend_entered = 0;

	if (s5p_rp_is_running) {
		if (s5p_rp.pcm_dump_cnt > 0)
			s5p_rp_set_pcm_dump(1);
	}
}
#endif

/*
 * The functions for inserting/removing us as a module.
 */

static int __init s5p_rp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	s5p_i2s_do_resume_for_rp();

	s5p_rp.iram	= ioremap(RP_IRAM_BASE, 0x20000);
	s5p_rp.sram	= ioremap(RP_SRAM_BASE, 0x40000);
	s5p_rp.clkcon	= ioremap(RP_ASSCLK_BASE, 0x0010);
	s5p_rp.commbox	= ioremap(RP_COMMBOX_BASE, 0x0200);
	s5p_rp.special	= ioremap(0x030F0000, 0x0100);	/* Hidden register */

	if ((s5p_rp.iram == NULL) || (s5p_rp.sram == NULL) ||
		(s5p_rp.clkcon == NULL) || (s5p_rp.commbox == NULL) ||
		(s5p_rp.special == NULL)) {
		printk(KERN_ERR "S5P_RP: ioremap error for Audio Subsystem\n");
		s5p_i2s_do_suspend_for_rp();

		return -ENXIO;
	}

	s5p_rp.iram_imem = s5p_rp.iram + _IMEM_OFFSET_;	/* VLIW iRAM offset */
	s5p_rp.dmem	= s5p_rp.sram + 0x00000;
	s5p_rp.icache	= s5p_rp.sram + 0x20000;
	s5p_rp.cmem	= s5p_rp.sram + 0x30000;

	ret = request_irq(IRQ_AUDIO_SS, s5p_rp_irq, 0, "s5p-rp", pdev);
	if (ret < 0) {
		printk(KERN_ERR "S5P_RP: Fail to claim RP(AUDIO_SS) irq\n");
		goto err1;
	}

	s5p_rp.early_suspend_entered = 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
	s5p_rp.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	s5p_rp.early_suspend.suspend = s5p_rp_early_suspend;
	s5p_rp.early_suspend.resume = s5p_rp_late_resume;
	register_early_suspend(&s5p_rp.early_suspend);
#endif

	/* Allocate Firmware buffer */
	ret = s5p_rp_prepare_fw_buff(dev);
	if (ret < 0) {
		printk(KERN_ERR "S5P_RP: Fail to allocate memory\n");
		goto err1;
	}

	/* Information for I2S driver */
	s5p_rp_is_opened = 0;
	s5p_rp_is_running = 0;

	/* I2S irq usage */
	s5p_rp.i2s_irq_enabled = 0;
	/* Default C type (iRAM instruction)*/
	s5p_rp_init_op_mode(RP_ARM_INTR_CODE_ULP_CTYPE);
	/* Set Default Gain to 1.0 */
	s5p_rp.gain = 1<<24;		/* 1.0 * (1<<24) */
	s5p_rp.gain_subl = 100;
	s5p_rp.gain_subr = 100;

	/* Operation level for idle framework */
	s5p_rp.dram_in_use = 0;

	s5p_rp.suspend_during_eos = 0;

	/* PCM dump mode */
	s5p_rp.pcm_dump_enabled = 0;
	s5p_rp.pcm_dump_cnt = 0;

	/* Set Sound Alive Off */
	s5p_rp.effect_def = 0;
	s5p_rp.effect_eq_user = 0;
	s5p_rp.effect_speaker = 1;

	/* Clear alternate Firmware */
	s5p_rp.alt_fw_loaded = 0;

	ret = misc_register(&s5p_rp_miscdev);
	if (ret) {
		printk(KERN_ERR "S5P_RP: Cannot register miscdev on minor=%d\n",
			RP_DEV_MINOR);
		goto err;
	}

	ret = misc_register(&s5p_rp_ctrl_miscdev);
	if (ret) {
		printk(KERN_ERR "S5P_RP: Cannot register miscdev on minor=%d\n",
			RP_CTRL_DEV_MINOR);
		goto err;
	}

	printk(KERN_INFO "S5P_RP: Driver successfully probed\n");

	s5p_i2s_do_suspend_for_rp();

	return 0;

err:
	free_irq(IRQ_AUDIO_SS, pdev);
	s5p_rp_remove_fw_buff();

err1:
	iounmap(s5p_rp.iram);
	iounmap(s5p_rp.sram);
	iounmap(s5p_rp.clkcon);
	iounmap(s5p_rp.commbox);
	iounmap(s5p_rp.special);

	s5p_i2s_do_suspend_for_rp();

	return ret;
}


static int s5p_rp_remove(struct platform_device *pdev)
{
	s5pdbg("s5p_rp_remove() called !\n");

	free_irq(IRQ_AUDIO_SS, pdev);
	s5p_rp_remove_fw_buff();

	iounmap(s5p_rp.iram);
	iounmap(s5p_rp.sram);
	iounmap(s5p_rp.clkcon);
	iounmap(s5p_rp.commbox);
	iounmap(s5p_rp.special);

	return 0;
}

#ifdef CONFIG_PM
static int s5p_rp_suspend(struct platform_device *pdev, pm_message_t state)
{
	s5pdbg("suspend\n");

	if (s5p_rp_is_opened) {
		if (s5p_rp.wait_for_eos && !s5p_rp_is_running)
			s5p_rp.suspend_during_eos = 1;

		s5p_i2s_do_suspend_for_rp();	/* I2S suspend */
	}

	return 0;
}

static int s5p_rp_resume(struct platform_device *pdev)
{
	s5pdbg("resume\n");

	if (s5p_rp_is_opened) {
		s5p_i2s_do_resume_for_rp();	/* I2S resume */
		s5p_rp_set_default_fw();

		s5p_rp_flush_ibuf();
		s5pdbg("Init, IBUF size [%ld]\n", s5p_rp.ibuf_size);
		s5p_rp_reset();
		s5p_rp_is_running = 0;

		if (s5p_rp.suspend_during_eos) {
			s5p_rp.stop_after_eos = 1;
			s5p_rp.suspend_during_eos = 0;
		}
	}

	return 0;
}
#else
#define s5p_rp_suspend NULL
#define s5p_rp_resume  NULL
#endif

static struct platform_driver s5p_rp_driver = {
	.probe		= s5p_rp_probe,
	.remove		= s5p_rp_remove,
	.suspend	= s5p_rp_suspend,
	.resume		= s5p_rp_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s5p-rp",
	},
};

static char banner[] __initdata =
	KERN_INFO "S5PC210 RP driver, (c) 2010 Samsung Electronics\n";

int __init s5p_rp_init(void)
{
	printk(banner);

	return platform_driver_register(&s5p_rp_driver);
}

void __exit s5p_rp_exit(void)
{
	platform_driver_unregister(&s5p_rp_driver);
}

module_init(s5p_rp_init);
module_exit(s5p_rp_exit);

MODULE_AUTHOR("Yeongman Seo <yman.seo@samsung.com>");
MODULE_DESCRIPTION("S5PC210 RP driver");
MODULE_LICENSE("GPL");
