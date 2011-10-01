/*
 *  c1_mc1n2.c
 *
 *  Copyright (c) 2009 Samsung Electronics Co. Ltd
 *  Author: aitdark.park  <aitdark.park@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or  modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <mach/regs-clock.h>
#include "../codecs/mc1n2/mc1n2.h"
#include "s3c-dma.h"
#include "s3c-pcm.h"
#include "s3c64xx-i2s.h"
#include "s3c-dma-wrapper.h"


int mc1n2_set_mclk_source(bool on)
{
	u32 val;
	u32 __iomem *xusbxti_sys_pwr;
	u32 __iomem *pmu_debug;


	xusbxti_sys_pwr = ioremap(0x10021280, 4);
	pmu_debug = ioremap(0x10020A00, 4);

	if (on) {
		val = readl(xusbxti_sys_pwr);
		val |= 0x0001;			/* SYS_PWR_CFG is enabled */
		writel(val, xusbxti_sys_pwr);

		val = readl(pmu_debug);
		val &= ~(0x000F << 8);
		val |= 0x0009 << 8;		/* Selected XUSBXTI */
		val &= ~(0x0001);		/* CLKOUT is enabled */
		writel(val, pmu_debug);
	} else {
		val = readl(xusbxti_sys_pwr);
		val &= ~(0x0001);		/* SYS_PWR_CFG is disabled */
		writel(val, xusbxti_sys_pwr);

		val = readl(pmu_debug);
		val |= 0x0001;			/* CLKOUT is disabled */
		writel(val, pmu_debug);
	}

	iounmap(xusbxti_sys_pwr);
	iounmap(pmu_debug);

	mdelay(10);


	return 0;
}
EXPORT_SYMBOL(mc1n2_set_mclk_source);

static int c1_hifi_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int ret;

	s5p_i2s_do_resume_stream(substream);

	/* Set the codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* Set the cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_CDCLK,
				0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_MUX,
				0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(codec_dai, MC1N2_BCLK_MULT, MC1N2_LRCK_X32);

	if (ret < 0)
		return ret;

	return 0;
}

/* C1 Audio  extra controls */
#define C1_AUDIO_OFF	0
#define C1_HEADSET_OUT	1
#define C1_MIC_IN	2
#define C1_LINE_IN	3

static int c1_aud_mode;

static const char *c1_aud_scenario[] = {
	[C1_AUDIO_OFF] = "Off",
	[C1_HEADSET_OUT] = "Playback Headphones",
	[C1_MIC_IN] = "Capture Mic",
	[C1_LINE_IN] = "Capture Line",
};

static void c1_aud_ext_control(struct snd_soc_codec *codec)
{
	switch (c1_aud_mode) {
	case C1_HEADSET_OUT:
		snd_soc_dapm_enable_pin(codec, "Headset Out");
		snd_soc_dapm_disable_pin(codec, "Mic In");
		snd_soc_dapm_disable_pin(codec, "Line In");
		break;
	case C1_MIC_IN:
		snd_soc_dapm_disable_pin(codec, "Headset Out");
		snd_soc_dapm_enable_pin(codec, "Mic In");
		snd_soc_dapm_disable_pin(codec, "Line In");
		break;
	case C1_LINE_IN:
		snd_soc_dapm_disable_pin(codec, "Headset Out");
		snd_soc_dapm_disable_pin(codec, "Mic In");
		snd_soc_dapm_enable_pin(codec, "Line In");
		break;
	case C1_AUDIO_OFF:
	default:
		snd_soc_dapm_disable_pin(codec, "Headset Out");
		snd_soc_dapm_disable_pin(codec, "Mic In");
		snd_soc_dapm_disable_pin(codec, "Line In");
		break;
	}

	snd_soc_dapm_sync(codec);
}

static int c1_mc1n2_getp(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = c1_aud_mode;
	return 0;
}

static int c1_mc1n2_setp(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (c1_aud_mode == ucontrol->value.integer.value[0])
		return 0;

	c1_aud_mode = ucontrol->value.integer.value[0];
	c1_aud_ext_control(codec);

	return 1;
}

static const struct snd_soc_dapm_widget c1_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headset Out", NULL),
	SND_SOC_DAPM_MIC("Mic In", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

static const struct soc_enum c1_aud_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(c1_aud_scenario),
			    c1_aud_scenario),
};

static const struct snd_kcontrol_new c1_aud_controls[] = {
	SOC_ENUM_EXT("C1 Audio Mode", c1_aud_enum[0],
		     c1_mc1n2_getp, c1_mc1n2_setp),
};

static const struct snd_soc_dapm_route c1_dapm_routes[] = {
	/* Connections to Headset */
	{"Headset Out", NULL, "HPOUT1L"},
	{"Headset Out", NULL, "HPOUT1R"},
	/* Connections to Mics */
	{"IN1LN", NULL, "Mic In"},
	{"IN1RN", NULL, "Mic In"},
	/* Connections to Line In */
	{"IN2LN", NULL, "Line In"},
	{"IN2RN", NULL, "Line In"},
};

static int c1_hifiaudio_init(struct snd_soc_codec *codec)
{
	return 0;
}

/*
 * C1 MC1N2 DAI operations.
 */
static struct snd_soc_ops c1_hifi_ops = {
	.hw_params = c1_hifi_hw_params,
};

static struct snd_soc_dai_link c1_dai[] = {
	{
		.name = "MC1N2 AIF1",
		.stream_name = "hifiaudio",
		.cpu_dai = &s3c64xx_i2s_v4_dai,
		.codec_dai = &mc1n2_dai[0],
		.init = c1_hifiaudio_init,
		.ops = &c1_hifi_ops,
	 }
};

static struct snd_soc_card c1_snd_card = {
	.name = "C1-YMU823",
	.platform = &s3c_dma_wrapper,
	.dai_link = c1_dai,
	.num_links = ARRAY_SIZE(c1_dai),
};

static struct mc1n2_setup c1_mc1n2_setup = {
	{  /* MCDRV_INIT_INFO */
		MCDRV_CKSEL_CMOS, /* bCkSel */
		41,               /* bDivR0 */
		126,              /* bDivF0 */
		41,               /* bDivR1 */
		126,              /* bDivF1 */
		0,                /* bRange0 */
		0,                /* bRange1 */
		0,                /* bBypass */
		MCDRV_DAHIZ_LOW,  /* bDioSdo0Hiz */
		MCDRV_DAHIZ_LOW,  /* bDioSdo1Hiz */
		MCDRV_DAHIZ_LOW,  /* bDioSdo2Hiz */
		MCDRV_DAHIZ_HIZ,  /* bDioClk0Hiz */
		MCDRV_DAHIZ_HIZ,  /* bDioClk1Hiz */
		MCDRV_DAHIZ_HIZ,  /* bDioClk2Hiz */
		MCDRV_PCMHIZ_HIZ, /* bPcmHiz */
		MCDRV_LINE_STEREO,/* bLineIn1Dif */
		0,                /* bLineIn2Dif */
		MCDRV_LINE_STEREO,/* bLineOut1Dif */
		MCDRV_LINE_STEREO,/* bLineOUt2Dif */
		MCDRV_SPMN_ON,    /* bSpmn */
		MCDRV_MIC_DIF,    /* bMic1Sng */
		MCDRV_MIC_DIF,    /* bMic2Sng */
		MCDRV_MIC_DIF,    /* bMic3Sng */
		MCDRV_POWMODE_NORMAL, /* bPowerMode */
		MCDRV_SPHIZ_PULLDOWN, /* bSpHiz */
		MCDRV_LDO_ON,     /* bLdo */
		MCDRV_PAD_GPIO,   /* bPad0Func */
		MCDRV_PAD_GPIO,   /* bPad1Func */
		MCDRV_PAD_GPIO,   /* bPad2Func */
		MCDRV_OUTLEV_4,   /* bAvddLev */
		0,                /* bVrefLev */
		MCDRV_DCLGAIN_12, /* bDclGain */
		MCDRV_DCLLIMIT_0, /* bDclLimit */
		1,                   /* set Hi-power mode 0: HP mode 1: normal */
		0,                /* bReserved1 */
		0,                /* bReserved2 */
		0,                /* bReserved3 */
		0,                /* bReserved4 */
		0,                /* bReserved5 */
		{                 /* sWaitTime */
			130000,         /* dAdHpf */
			25000,          /* dMic1Cin */
			25000,          /* dMic2Cin */
			25000,          /* dMic3Cin */
			25000,          /* dLine1Cin */
			25000,          /* dLine2Cin */
			5000,           /* dVrefRdy1 */
			15000,          /* dVrefRdy2 */
			9000,           /* dHpRdy */
			13000,          /* dSpRdy */
			0,              /* dPdm */
			1000,           /* dAnaRdyInterval */
			1000,           /* dSvolInterval */
			1000,           /* dAnaRdyTimeOut */
			1000            /* dSvolTimeOut */
		}
	}, /* MCDRV_INIT_INFO end */
	{  /* pcm_extend */
		0, 0, 0
	}, /* pcm_extend end */
	{  /* pcm_hiz_redge */
		MCDRV_PCMHIZTIM_FALLING, MCDRV_PCMHIZTIM_FALLING, MCDRV_PCMHIZTIM_FALLING
	}, /* pcm_hiz_redge end */
	{  /* pcm_hperiod */
		1, 1, 1
	}, /* pcm_hperiod end */
	{  /* slot */
		{ {0, 1}, {0, 1} },
		{ {0, 1}, {0, 1} },
		{ {0, 1}, {0, 1} }
	}  /* slot end */
};

static struct snd_soc_device c1_snd_devdata = {
	.card = &c1_snd_card,
	.codec_dev = &soc_codec_dev_mc1n2,
	.codec_data = &c1_mc1n2_setup,
};

static struct platform_device *c1_snd_device;

static int __init c1_audio_init(void)
{
	int ret;

	mc1n2_set_mclk_source(1);

	c1_snd_device = platform_device_alloc("soc-audio", -1);
	if (!c1_snd_device)
		return -ENOMEM;

	platform_set_drvdata(c1_snd_device, &c1_snd_devdata);
	c1_snd_devdata.dev = &c1_snd_device->dev;

	ret = platform_device_add(c1_snd_device);
	if (ret)
		platform_device_put(c1_snd_device);
	return ret;
}

static void __exit c1_audio_exit(void)
{
	platform_device_unregister(c1_snd_device);
}

module_init(c1_audio_init);

MODULE_AUTHOR("aitdark, aitdark.park@samsung.com");
MODULE_DESCRIPTION("ALSA SoC C1 MC1N2");
MODULE_LICENSE("GPL");
