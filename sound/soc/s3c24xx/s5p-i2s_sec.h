/* sound/soc/s3c24xx/s5p-i2s_sec.h
 *
 * ALSA SoC Audio Layer - S5P I2S Secondary fifo driver
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_S3C24XX_S5P_I2S_SEC_H
#define __SND_SOC_S3C24XX_S5P_I2S_SEC_H __FILE__

extern int s5p_i2s_startup(struct snd_soc_dai *dai);
extern int s5p_i2s_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai);
extern int s5p_i2s_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_dai *dai);
extern void s5p_i2s_sec_init(void *, dma_addr_t);

#endif /* __SND_SOC_S3C24XX_S5P_I2S_SEC_H */
