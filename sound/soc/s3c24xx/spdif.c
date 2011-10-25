/* sound/soc/s3c24xx/spdif.c
 *
 * ALSA SoC Audio Layer - Samsung S/PDIF Controller driver
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/clk.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <plat/audio.h>
#include <mach/dma.h>

#include "s3c-dma.h"
#include "spdif.h"

static struct s3c2410_dma_client spdif_dma_client_out = {
	.name		= "S/PDIF Stereo out",
};

static struct s3c_dma_params spdif_stereo_out;
static struct samsung_spdif_info spdif_info;
struct snd_soc_dai samsung_spdif_dai;
EXPORT_SYMBOL_GPL(samsung_spdif_dai);

static inline struct samsung_spdif_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

static void spdif_snd_txctrl(struct samsung_spdif_info *spdif, int on)
{
	void __iomem *regs = spdif->regs;
	u32 clkcon;

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	clkcon = readl(regs + CLKCON);

	if (on)
		writel(clkcon | CLKCTL_PWR_ON, regs + CLKCON);
	else
		writel(clkcon & ~CLKCTL_PWR_ON, regs + CLKCON);
}

static int spdif_set_sysclk(struct snd_soc_dai *cpu_dai,
				int clk_id, unsigned int freq, int dir)
{
	struct samsung_spdif_info *spdif = to_info(cpu_dai);

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	if (dir == SND_SOC_CLOCK_IN)
		spdif->use_int_clk = 0;
	else
		spdif->use_int_clk = 1;

	spdif->clk_rate = freq;

	return 0;
}

static int spdif_trigger(struct snd_pcm_substream *substream, int cmd,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rt = substream->private_data;
	struct samsung_spdif_info *spdif = to_info(rt->dai->cpu_dai);
	unsigned long flags;

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		spin_lock_irqsave(&spdif->lock, flags);
		spdif_snd_txctrl(spdif, 1);
		spin_unlock_irqrestore(&spdif->lock, flags);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		spin_lock_irqsave(&spdif->lock, flags);
		spdif_snd_txctrl(spdif, 0);
		spin_unlock_irqrestore(&spdif->lock, flags);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int spdif_sysclk_ratios[] = {
	512, 384, 256,
};

static int spdif_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *socdai)
{
	struct snd_soc_pcm_runtime *rt = substream->private_data;
	struct snd_soc_dai_link *dai = rt->dai;
	struct samsung_spdif_info *spdif = to_info(dai->cpu_dai);
	struct s3c_dma_params *dma_data;
	void __iomem *regs = spdif->regs;
	u32 con, cstas, clkcon;
	unsigned long flags;
	int i, ratio;

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = spdif->dma_playback;
	else {
		printk(KERN_ERR "capture is not supported\n");
		return -EINVAL;
	}

	snd_soc_dai_set_dma_data(dai->cpu_dai, substream, dma_data);

	spin_lock_irqsave(&spdif->lock, flags);

	con = readl(regs + CON);
	cstas = readl(regs + CSTAS);
	clkcon = readl(regs + CLKCON);

	con &= ~CON_FIFO_TH_MASK;
	con |= (0x7 << CON_FIFO_TH_SHIFT);
	con |= CON_USERDATA_23RDBIT;
	con |= CON_PCM_DATA;

	con &= ~CON_PCM_MASK;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		con |= CON_PCM_16BIT;
		break;
	default:
		dev_err(spdif->dev, "Unsupported data size.\n");
		goto err;
	}

	if (spdif->use_int_clk)
		clkcon &= ~CLKCTL_MCLK_EXT;
	else
		clkcon |= CLKCTL_MCLK_EXT;

	ratio = spdif->clk_rate / params_rate(params);
	for (i = 0; i < ARRAY_SIZE(spdif_sysclk_ratios); i++)
		if (ratio == spdif_sysclk_ratios[i])
			break;
	if (i == ARRAY_SIZE(spdif_sysclk_ratios)) {
		dev_err(spdif->dev, "Invalid clock ratio %d/%d\n",
			spdif->clk_rate, params_rate(params));
		goto err;
	}

	if (spdif->use_int_clk)
		clk_set_rate(spdif->sclk, spdif->clk_rate);

	con &= ~CON_MCLKDIV_MASK;
	switch (ratio) {
	case 256:
		con |= CON_MCLKDIV_256FS;
		break;
	case 384:
		con |= CON_MCLKDIV_384FS;
		break;
	case 512:
		con |= CON_MCLKDIV_512FS;
		break;
	}

	cstas &= ~CSTAS_SAMP_FREQ_MASK;
	switch (params_rate(params)) {
	case 44100:
		cstas |= CSTAS_SAMP_FREQ_44;
		break;
	case 48000:
		cstas |= CSTAS_SAMP_FREQ_48;
		break;
	case 32000:
		cstas |= CSTAS_SAMP_FREQ_32;
		break;
	case 96000:
		cstas |= CSTAS_SAMP_FREQ_96;
		break;
	default:
		goto err;
	}

	cstas &= ~CSTAS_CATEGORY_MASK;
	cstas |= CSTAS_CATEGORY_CODE_CDP;
	cstas |= CSTAS_NO_COPYRIGHT;

	writel(con, regs + CON);
	writel(cstas, regs + CSTAS);
	writel(clkcon, regs + CLKCON);

	spin_unlock_irqrestore(&spdif->lock, flags);

	return 0;

err:
	spin_unlock_irqrestore(&spdif->lock, flags);

	return -EINVAL;
}

static void spdif_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rt = substream->private_data;
	struct samsung_spdif_info *spdif = to_info(rt->dai->cpu_dai);
	void __iomem *regs = spdif->regs;
	u32 con, clkcon;

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	con = readl(regs + CON);
	clkcon = readl(regs + CLKCON);

	writel(con | CON_SW_RESET, regs + CON);
	cpu_relax();

	writel(clkcon & ~CLKCTL_PWR_ON, regs + CLKCON);
}

#ifdef CONFIG_PM
static int spdif_suspend(struct snd_soc_dai *cpu_dai)
{
	struct samsung_spdif_info *spdif = to_info(cpu_dai);
	u32 con = spdif->saved_con;

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	spdif->saved_clkcon = readl(spdif->regs	+ CLKCON);
	spdif->saved_con = readl(spdif->regs + CON);
	spdif->saved_cstas = readl(spdif->regs + CSTAS);

	writel(CLKCTL_PWR_ON, spdif->regs + CLKCON);
	writel(con | CON_SW_RESET, spdif->regs + CON);
	cpu_relax();

	writel(~CLKCTL_PWR_ON, spdif->regs + CLKCON);

	return 0;
}

static int spdif_resume(struct snd_soc_dai *cpu_dai)
{
	struct samsung_spdif_info *spdif = to_info(cpu_dai);

	dev_dbg(spdif->dev, "Entered %s\n", __func__);

	writel(spdif->saved_clkcon, spdif->regs	+ CLKCON);
	writel(spdif->saved_con, spdif->regs + CON);
	writel(spdif->saved_cstas, spdif->regs + CSTAS);

	return 0;
}
#else
#define spdif_suspend NULL
#define spdif_resume NULL
#endif

static struct snd_soc_dai_ops spdif_dai_ops = {
	.set_sysclk	= spdif_set_sysclk,
	.trigger	= spdif_trigger,
	.hw_params	= spdif_hw_params,
	.shutdown	= spdif_shutdown,
};

#define S5P_SPDIF_RATES	(SNDRV_PCM_RATE_32000 | \
			SNDRV_PCM_RATE_44100 | \
			SNDRV_PCM_RATE_48000 | \
			SNDRV_PCM_RATE_96000)
#define S5P_SPDIF_FORMATS	SNDRV_PCM_FMTBIT_S16_LE

static __devinit int spdif_probe(struct platform_device *pdev)
{
	int ret;
	struct samsung_spdif_info *spdif;
	struct snd_soc_dai *dai;
	struct resource *mem_res, *dma_res;
	struct s3c_audio_pdata *spdif_pdata;

	spdif_pdata = pdev->dev.platform_data;

	dev_dbg(&pdev->dev, "Entered %s\n", __func__);

	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!dma_res) {
		dev_err(&pdev->dev, "Unable to get dma resource.\n");
		return -ENXIO;
	}

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		dev_err(&pdev->dev, "Unable to get register resource.\n");
		return -ENXIO;
	}

	if (spdif_pdata && spdif_pdata->cfg_gpio
			&& spdif_pdata->cfg_gpio(pdev)) {
		dev_err(&pdev->dev, "Unable to configure GPIO pins\n");
		return -EINVAL;
	}

	/* Fill-up basic DAI features */
	samsung_spdif_dai.name = "samsung-spdif";
	samsung_spdif_dai.ops = &spdif_dai_ops;
	samsung_spdif_dai.suspend = spdif_suspend;
	samsung_spdif_dai.resume = spdif_resume;
	samsung_spdif_dai.playback.channels_min = 2;
	samsung_spdif_dai.playback.channels_max = 2;
	samsung_spdif_dai.playback.rates = S5P_SPDIF_RATES;
	samsung_spdif_dai.playback.formats = S5P_SPDIF_FORMATS;

	spdif = &spdif_info;
	spdif->dev = &pdev->dev;

	spin_lock_init(&spdif->lock);

	spdif->pclk = clk_get(&pdev->dev, "spdif");
	if (IS_ERR(spdif->pclk)) {
		dev_err(&pdev->dev, "failed to get SPDIF PCLK\n");
		ret = -ENOENT;
		goto err0;
	}
	clk_enable(spdif->pclk);

	spdif->use_int_clk = 1;
	spdif->clk_rate = 0;

	/* Get source clock that can be set rate. Actually 'sclk_spdif'
	 * has no divider to set it's rate, so we should have to use
	 * parent clock of 'sclk_spdif' that has divider.
	 */
	spdif->sclk = clk_get_parent(clk_get(&pdev->dev, "sclk_spdif"));
	if (IS_ERR(spdif->sclk)) {
		dev_err(&pdev->dev, "failed to get SPDIF source clock\n");
		ret = -ENOENT;
		goto err1;
	}
	clk_enable(spdif->sclk);

	/* Request SPDIF Register's memory region */
	if (!request_mem_region(mem_res->start,
				resource_size(mem_res), "samsung-spdif")) {
		dev_err(&pdev->dev, "Unable to request register region\n");
		ret = -EBUSY;
		goto err2;
	}

	spdif->regs = ioremap(mem_res->start, 0x100);
	if (spdif->regs == NULL) {
		dev_err(&pdev->dev, "Cannot ioremap registers\n");
		ret = -ENXIO;
		goto err4;
	}

	/* Register cpu dai */
	dai = &samsung_spdif_dai;
	dai->dev = &pdev->dev;
	dai->private_data = spdif;

	ret = snd_soc_register_dai(dai);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to register cpu dai\n");
		goto err5;
	}

	spdif_stereo_out.dma_size = 2;
	spdif_stereo_out.client = &spdif_dma_client_out;
	spdif_stereo_out.dma_addr = mem_res->start + DATA_OUTBUF;
	spdif_stereo_out.channel = dma_res->start;

	spdif->dma_playback = &spdif_stereo_out;

	return 0;
err5:
	iounmap(spdif->regs);
err4:
	release_mem_region(mem_res->start, resource_size(mem_res));
err2:
	clk_disable(spdif->sclk);
	clk_put(spdif->sclk);
err1:
	clk_disable(spdif->pclk);
	clk_put(spdif->pclk);
err0:
	return ret;
}

static __devexit int spdif_remove(struct platform_device *pdev)
{
	struct snd_soc_dai *dai = &samsung_spdif_dai;
	struct samsung_spdif_info *spdif = &spdif_info;
	struct resource *mem_res;

	snd_soc_unregister_dai(dai);

	iounmap(spdif->regs);

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem_res->start, resource_size(mem_res));

	clk_disable(spdif->pclk);
	clk_disable(spdif->sclk);
	clk_put(spdif->pclk);
	clk_put(spdif->sclk);

	return 0;
}

static struct platform_driver samsung_spdif_driver = {
	.probe	= spdif_probe,
	.remove	= spdif_remove,
	.driver	= {
		.name	= "samsung-spdif",
		.owner	= THIS_MODULE,
	},
};

static int __init spdif_init(void)
{
	return platform_driver_register(&samsung_spdif_driver);
}
module_init(spdif_init);

static void __exit spdif_exit(void)
{
	platform_driver_unregister(&samsung_spdif_driver);
}
module_exit(spdif_exit);

MODULE_AUTHOR("Seungwhan Youn, <sw.youn@samsung.com>");
MODULE_DESCRIPTION("Samsung S/PDIF Controller Driver");
MODULE_LICENSE("GPL");
