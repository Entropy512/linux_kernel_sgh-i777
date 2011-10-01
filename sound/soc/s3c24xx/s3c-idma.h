/*
 * s3c-idma.h  --  I2S0's Internal Dma driver
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __S3C_IDMA_H_
#define __S3C_IDMA_H_

#ifdef CONFIG_ARCH_S5PC1XX /* S5PC100 */
        #define MAX_LP_BUFF	(128 * 1024)
        #define LP_DMA_PERIOD   (105 * 1024)
        #define LP_TXBUFF_ADDR  (0xC0000000)
#elif defined(CONFIG_ARCH_S5PC210) || defined(CONFIG_ARCH_S5PV310)
        #define MAX_LP_BUFF	(64 * 1024)
        #define LP_DMA_PERIOD   (64 * 1024)
        #define LP_TXBUFF_ADDR  (0x02030000)	/* IRAM, high 64KB area */
        /* On ORION EVT0, IDMA doesnot work with DMEM & I$ */
        #if 0
        #define LP_TXBUFF_ADDR  (0x03000000)	/* DMEM */
        #define LP_TXBUFF_ADDR  (0x03020000)	/* I$ */
        #define LP_TXBUFF_ADDR  (0x02020000)	/* IRAM */
        #endif
#else
        #define MAX_LP_BUFF	(160 * 1024)
        #define LP_DMA_PERIOD   (128 * 1024)
        #define LP_TXBUFF_ADDR  (0xC0000000)
#endif

#define S5P_IISLVLINTMASK (0xf << 20)

/* dma_state */
#define LPAM_DMA_STOP    0
#define LPAM_DMA_START   1

extern struct snd_soc_platform idma_soc_platform;
extern void s5p_idma_init(void *);

extern int i2s_trigger_stop;
extern int audio_clk_gated ;

/* These functions are used for srp driver. */
extern void s5p_i2s_idma_enable(unsigned long frame_size_bytes);
extern void s5p_i2s_idma_pause(void);
extern void s5p_i2s_idma_continue(void);
extern void s5p_i2s_idma_stop(void);
extern int s5p_i2s_idma_irq_callback(void);

#endif /* __S3C_IDMA_H_ */
