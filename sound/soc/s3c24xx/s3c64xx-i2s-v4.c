/* sound/soc/s3c24xx/s3c64xx-i2s-v4.c
 *
 * ALSA SoC Audio Layer - S3C64XX I2Sv4 driver
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 * 	Author: Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <plat/audio.h>

#include <mach/map.h>
#include <mach/dma.h>
#include <mach/regs-audss.h>

#include "s5p-i2s_sec.h"
#include "s3c-dma.h"
#include "regs-i2s-v2.h"
#include "s3c64xx-i2s.h"

/* runtime pm is disabled, current AUDIO PD is always enabled. */
#ifdef CONFIG_S5PV310_DEV_PD
#undef CONFIG_S5PV310_DEV_PD
#endif

static struct s3c2410_dma_client s3c64xx_dma_client_out = {
	.name		= "I2Sv4 PCM Stereo out"
};

static struct s3c2410_dma_client s3c64xx_dma_client_in = {
	.name		= "I2Sv4 PCM Stereo in"
};

static struct s3c_dma_params s3c64xx_i2sv4_pcm_stereo_out;
static struct s3c_dma_params s3c64xx_i2sv4_pcm_stereo_in;
static struct s3c_i2sv2_info s3c64xx_i2sv4;

struct snd_soc_dai s3c64xx_i2s_v4_dai;
EXPORT_SYMBOL_GPL(s3c64xx_i2s_v4_dai);

int audio_clk_gated = 0;	/* At first, clock is enabled in probe() */
static int tx_clk_enabled = 0;
static int rx_clk_enabled = 0;
#ifdef CONFIG_SND_S5P_RP
static int srp_clk_enabled = 0;
#endif
static int suspended_by_pm = 0;
static DEFINE_SPINLOCK(i2s_clk_lock);

static int s5p_i2s_sec_used(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef CONFIG_SND_S5P_RP
		/* Primary port for Legacy Audio, Secondary port for SRP Audio */
		return 0;
#elif defined CONFIG_S5P_INTERNAL_DMA
		/* Secondary port for Generic Audio */
		return 1;
#else
		/* Primary port for Generic Audio */
		return 0;
#endif
	} else {
		/* Capture is possible via Primary port only */
		return 0;
	}
}

static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

void s5p_i2s_set_clk_enabled(struct snd_soc_dai *dai, bool state)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	pr_debug("..entering %s \n", __func__);

/*	CLKMUX_ASS(FOUT_EPLL) & MUX_I2S(MainClk=MOUT_ASS) are used,
	  instead of sclk_audio at the moment.
	if (i2s->sclk_audio == NULL) return;
*/

	if (state) {
		if (!audio_clk_gated) {
			pr_debug("already audio clock is enabled! \n");
			return;
		}
#ifdef CONFIG_S5PV310_DEV_PD
		pm_runtime_get_sync(dai->dev);
#endif
		if (dai->id == 0) {     /* I2S V5.1? */
			/*clk_enable(i2s->iis_cclk);*/
			clk_enable(i2s->iis_pclk);
#ifdef CONFIG_SND_S5P_RP
			clk_enable(i2s->audss_srp);
#endif
		}
		audio_clk_gated = 0;
	} else {
		if (audio_clk_gated) {
			pr_debug("already audio clock is gated! \n");
			return;
		}
		if (dai->id == 0) {     /* I2S V5.1? */
#ifdef CONFIG_SND_S5P_RP
			clk_disable(i2s->audss_srp);
#endif
			clk_disable(i2s->iis_pclk);
			/*clk_disable(i2s->iis_cclk);*/
		}
#ifdef CONFIG_S5PV310_DEV_PD
		pm_runtime_put_sync(dai->dev);
#endif
		audio_clk_gated = 1;
	}
}

void s5p_i2s_do_suspend(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	unsigned long flags;

	spin_lock_irqsave(&i2s_clk_lock, flags);

	if (!audio_clk_gated) {         /* Clk/Pwr is alive? */
		i2s->suspend_iiscon = readl(i2s->regs + S3C2412_IISCON);
		i2s->suspend_iismod = readl(i2s->regs + S3C2412_IISMOD);
		i2s->suspend_iispsr = readl(i2s->regs + S3C2412_IISPSR);
		i2s->suspend_iisahb = readl(i2s->regs + S5P_IISAHB);
		/* Is this dai for I2Sv5? (I2S0) */
		if (dai->id == 0) {
			i2s->suspend_audss_clkdiv = readl(S5P_CLKDIV_AUDSS);
			i2s->suspend_audss_clksrc = readl(S5P_CLKSRC_AUDSS);
			i2s->suspend_audss_clkgate = readl(S5P_CLKGATE_AUDSS);
			/* CLKMUX_ASS & MUX_I2S as XUSBXTI to reduce power */
			writel(0x0, S5P_CLKSRC_AUDSS);
		}
		s5p_i2s_set_clk_enabled(dai, 0);        /* Gating Clk/Pwr */
		pr_debug("Registers stored and suspend.\n");
	}

	spin_unlock_irqrestore(&i2s_clk_lock, flags);
}

void s5p_i2s_do_resume(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	unsigned long flags;

	spin_lock_irqsave(&i2s_clk_lock, flags);

	if (audio_clk_gated) {          /* Clk/Pwr is gated? */
		s5p_i2s_set_clk_enabled(dai, 1);        /* Enable Clk/Pwr */
		writel(i2s->suspend_iiscon, i2s->regs + S3C2412_IISCON);
		writel(i2s->suspend_iismod, i2s->regs + S3C2412_IISMOD);
		writel(i2s->suspend_iispsr, i2s->regs + S3C2412_IISPSR);
		writel(i2s->suspend_iisahb, i2s->regs + S5P_IISAHB);
		/* Is this dai for I2Sv5? (I2S0) */
		if (dai->id == 0) {
			writel(i2s->suspend_audss_clkdiv, S5P_CLKDIV_AUDSS);
			writel(i2s->suspend_audss_clksrc, S5P_CLKSRC_AUDSS);
			writel(i2s->suspend_audss_clkgate, S5P_CLKGATE_AUDSS);
		}
		/* Flush FIFO */
		writel(S3C2412_IISFIC_RXFLUSH | S3C2412_IISFIC_TXFLUSH,
			i2s->regs + S3C2412_IISFIC);
		writel(S3C2412_IISFIC_TXFLUSH, i2s->regs + S5P_IISFICS);
		ndelay(250);
		writel(0x0, i2s->regs + S3C2412_IISFIC);
		writel(0x0, i2s->regs + S5P_IISFICS);

		pr_debug("Resume and registers restored.\n");
	}

	spin_unlock_irqrestore(&i2s_clk_lock, flags);
}

#ifdef CONFIG_SND_S5P_RP
void s5p_i2s_do_suspend_for_rp(void)
{
	u32 clkdiv_audss;

	/* SRP ratio (15+1)= EPLL/16 = 181/16 = 11MHz
	   BUS ratio (1+1) = SRP/2 = 11/2 = 5MHz */
	clkdiv_audss = readl(S5P_CLKDIV_AUDSS);
	clkdiv_audss &= ~0x0FF;
	clkdiv_audss |= 0x01F;
	writel(clkdiv_audss, S5P_CLKDIV_AUDSS);

	srp_clk_enabled = 0;
	if (!tx_clk_enabled && !rx_clk_enabled)
		s5p_i2s_do_suspend(&s3c64xx_i2s_v4_dai);
}
EXPORT_SYMBOL(s5p_i2s_do_suspend_for_rp);

void s5p_i2s_do_resume_for_rp(void)
{
	u32 clkdiv_audss;

	s5p_i2s_do_resume(&s3c64xx_i2s_v4_dai);
	srp_clk_enabled = 1;

	/* SRP ratio (0+1) = EPLL/1 = 181MHz
	   BUS ratio (3+1) = SRP/4 = 181/4 = 45MHz */
	clkdiv_audss = readl(S5P_CLKDIV_AUDSS);
	clkdiv_audss &= ~0x0FF;
	clkdiv_audss |= 0x030;
	writel(clkdiv_audss, S5P_CLKDIV_AUDSS);
}
EXPORT_SYMBOL(s5p_i2s_do_resume_for_rp);
#endif

void s5p_i2s_do_suspend_stream(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		tx_clk_enabled = 0;
	else
		rx_clk_enabled = 0;

#ifdef CONFIG_SND_S5P_RP
	if (!tx_clk_enabled && !rx_clk_enabled && !srp_clk_enabled)
#else
	if (!tx_clk_enabled && !rx_clk_enabled)
#endif
		s5p_i2s_do_suspend(&s3c64xx_i2s_v4_dai);
}
EXPORT_SYMBOL(s5p_i2s_do_suspend_stream);

void s5p_i2s_do_resume_stream(struct snd_pcm_substream *substream)
{
	s5p_i2s_do_resume(&s3c64xx_i2s_v4_dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		tx_clk_enabled = 1;
	else
		rx_clk_enabled = 1;
}
EXPORT_SYMBOL(s5p_i2s_do_resume_stream);

static int s3c64xx_i2sv4_probe(struct platform_device *pdev,
			     struct snd_soc_dai *dai)
{
	/* do nothing */
	return 0;
}

static int s3c_i2sv4_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *cpu_dai)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	struct s3c_dma_params *dma_data;
	u32 iismod;
	u32 dma_tsfr_size = 0;

	dev_dbg(cpu_dai->dev, "Entered %s\n", __func__);

	/* TODO */
	switch (params_channels(params)) {
		case 1:
			dma_tsfr_size = 2;
			break;
		case 2:
			dma_tsfr_size = 4;
			break;
		case 4:
			break;
		case 6:
			break;
		default:
			break;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data = i2s->dma_playback;
		i2s->dma_playback->dma_size = dma_tsfr_size;
	} else {
		dma_data = i2s->dma_capture;
		i2s->dma_capture->dma_size = dma_tsfr_size;
	}

	snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

	iismod = readl(i2s->regs + S3C2412_IISMOD);
	dev_dbg(cpu_dai->dev, "%s: r: IISMOD: %x\n", __func__, iismod);

	iismod &= ~S3C64XX_IISMOD_BLC_MASK;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C64XX_IISMOD_BLC_8BIT;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		iismod |= S3C64XX_IISMOD_BLC_24BIT;
		break;
	}

	writel(iismod, i2s->regs + S3C2412_IISMOD);
	dev_dbg(cpu_dai->dev, "%s: w: IISMOD: %x\n", __func__, iismod);

	return 0;
}

static int s5p_i2s_wr_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	if (s5p_i2s_sec_used(substream))
		s5p_i2s_hw_params(substream, params, dai);
	else
		s3c_i2sv4_hw_params(substream, params, dai);

	s5p_i2s_do_suspend_stream(substream);

	return 0;
}
static int s5p_i2s_wr_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		s5p_i2s_do_resume_stream(substream);
		break;
	default:
		break;
	}

	if (s5p_i2s_sec_used(substream))
		s5p_i2s_trigger(substream, cmd, dai);
	else
		s3c2412_i2s_trigger(substream, cmd, dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		s5p_i2s_do_suspend_stream(substream);
		break;
	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_PM
static int s5p_i2s_wr_suspend(struct snd_soc_dai *dai)
{
	if (!audio_clk_gated) {         /* Clk/Pwr is alive? */
		suspended_by_pm = 1;
		s5p_i2s_do_suspend(dai);
	}

	return 0;
}

static int s5p_i2s_wr_resume(struct snd_soc_dai *dai)
{
	if (suspended_by_pm) {
		suspended_by_pm = 0;
		s5p_i2s_do_resume(dai);
	}

	return 0;
}
#else
#define s5p_i2s_wr_suspend NULL
#define s5p_i2s_wr_resume  NULL
#endif  /* CONFIG_PM */

#ifdef CONFIG_S5PV310_DEV_PD
static int s5p_i2s_wr_runtime_suspend(struct device *dev)
{
	return 0;
}

static int s5p_i2s_wr_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops s3c64xx_i2sv4_pm_ops = {
	.runtime_suspend = s5p_i2s_wr_runtime_suspend,
	.runtime_resume = s5p_i2s_wr_runtime_resume,
};
#endif

static struct snd_soc_dai_ops s3c64xx_i2sv4_dai_ops = {
	.hw_params	= s5p_i2s_wr_hw_params,
	.trigger	= s5p_i2s_wr_trigger,
};

static __devinit int s3c64xx_i2sv4_dev_probe(struct platform_device *pdev)
{
	struct s3c_audio_pdata *i2s_pdata;
	struct s3c_i2sv2_info *i2s;
	struct snd_soc_dai *dai;
	struct resource *res;
	int ret;

	i2s = &s3c64xx_i2sv4;
	dai = &s3c64xx_i2s_v4_dai;

	if (dai->dev) {
		dev_dbg(dai->dev, "%s: \
			I2Sv4 instance already registered!\n", __func__);
		return -EBUSY;
	}

#ifdef CONFIG_S5PV310_DEV_PD
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
/*	pm_runtime_resume(&pdev->dev);*/
#endif

	dai->dev = &pdev->dev;
	dai->name = "s3c64xx-i2s-v4";
	dai->id = 0;
	dai->symmetric_rates = 0;
	dai->playback.channels_min = 1;
	dai->playback.channels_max = 2;
	dai->playback.rates = S3C64XX_I2S_RATES;
	dai->playback.formats = S3C64XX_I2S_FMTS;
	dai->capture.channels_min = 1;
	dai->capture.channels_max = 2;
	dai->capture.rates = S3C64XX_I2S_RATES;
	dai->capture.formats = S3C64XX_I2S_FMTS;
	dai->probe = s3c64xx_i2sv4_probe;
	dai->ops = &s3c64xx_i2sv4_dai_ops;
	dai->suspend = s5p_i2s_wr_suspend;
	dai->resume = s5p_i2s_wr_resume;

	i2s->feature |= S3C_FEATURE_CDCLKCON;

	i2s->dma_capture = &s3c64xx_i2sv4_pcm_stereo_in;
	i2s->dma_playback = &s3c64xx_i2sv4_pcm_stereo_out;

	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-TX dma resource\n");
		return -ENXIO;
	}
	i2s->dma_playback->channel = res->start;

	res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-RX dma resource\n");
		return -ENXIO;
	}
	i2s->dma_capture->channel = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S SFR address\n");
		return -ENXIO;
	}

	if (!request_mem_region(res->start, resource_size(res),
				"s3c64xx-i2s-v4")) {
		dev_err(&pdev->dev, "Unable to request SFR region\n");
		return -EBUSY;
	}

	i2s->dma_capture->dma_addr = res->start + S3C2412_IISRXD;
	i2s->dma_playback->dma_addr = res->start + S3C2412_IISTXD;

	i2s->dma_capture->client = &s3c64xx_dma_client_in;
	i2s->dma_capture->dma_size = 4;
	i2s->dma_playback->client = &s3c64xx_dma_client_out;
	i2s->dma_playback->dma_size = 4;

	i2s->iis_cclk = clk_get(&pdev->dev, "audio-bus");
	if (IS_ERR(i2s->iis_cclk)) {
		dev_err(&pdev->dev, "failed to get audio-bus\n");
		ret = PTR_ERR(i2s->iis_cclk);
		goto err;
	}

	clk_enable(i2s->iis_cclk);

	ret = s3c_i2sv2_probe(pdev, dai, i2s,
			i2s->dma_playback->dma_addr - S3C2412_IISTXD);
	if (ret)
		goto err_clk;

	i2s_pdata = pdev->dev.platform_data;
	if (i2s_pdata && i2s_pdata->cfg_gpio && i2s_pdata->cfg_gpio(pdev)) {
		dev_err(&pdev->dev, "Unable to configure gpio\n");
		/*return -EINVAL;*/
		goto err_clk;
	}

#ifdef CONFIG_S5P_INTERNAL_DMA
	s5p_i2s_sec_init(i2s->regs, i2s->dma_playback->dma_addr - S3C2412_IISTXD);
#endif

	ret = s3c_i2sv2_register_dai(dai);
	if (ret != 0)
		goto err_i2sv2;

	s5p_i2s_do_suspend(dai);	/* Disable Clk/Pwr */

	return 0;

err_i2sv2:
	/* Not implemented for I2Sv2 core yet */
err_clk:
	clk_put(i2s->iis_cclk);
#ifdef CONFIG_S5PV310_DEV_PD
	pm_runtime_disable(&pdev->dev);
#endif
err:
	return ret;
}

static __devexit int s3c64xx_i2sv4_dev_remove(struct platform_device *pdev)
{
	dev_err(&pdev->dev, "Device removal not yet supported\n");

#ifdef CONFIG_S5PV310_DEV_PD
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

static struct platform_driver s3c64xx_i2sv4_driver = {
	.probe  = s3c64xx_i2sv4_dev_probe,
	.remove = s3c64xx_i2sv4_dev_remove,
	.driver = {
		.name	= "s3c64xx-iis-v4",
		.owner	= THIS_MODULE,
#ifdef CONFIG_S5PV310_DEV_PD
		.pm	= &s3c64xx_i2sv4_pm_ops,
#endif
	},
};

static int __init s3c64xx_i2sv4_init(void)
{
	return platform_driver_register(&s3c64xx_i2sv4_driver);
}
module_init(s3c64xx_i2sv4_init);

static void __exit s3c64xx_i2sv4_exit(void)
{
	platform_driver_unregister(&s3c64xx_i2sv4_driver);
}
module_exit(s3c64xx_i2sv4_exit);

/* Module information */
MODULE_AUTHOR("Jaswinder Singh, <jassi.brar@samsung.com>");
MODULE_DESCRIPTION("S3C64XX I2Sv4 SoC Interface");
MODULE_LICENSE("GPL");
