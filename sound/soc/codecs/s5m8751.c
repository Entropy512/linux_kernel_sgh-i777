/*
 * s5m8751.c  --  S5M8751 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>
#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/audio.h>
#include <linux/mfd/s5m8751/pdata.h>

#define S5M8751_VERSION "0.7"

struct s5m8751_priv {
	struct snd_soc_codec codec;
	struct s5m8751 *s5m8751;
};

struct sample_rate {
	unsigned int rate;
	uint8_t reg_rate;
};

struct sample_rate s5m8751_rate[] = {
	{ 8000, S5M8751_SAMP_RATE_8K },
	{ 11025, S5M8751_SAMP_RATE_11_025K },
	{ 16000, S5M8751_SAMP_RATE_16K },
	{ 22050, S5M8751_SAMP_RATE_22_05K },
	{ 32000, S5M8751_SAMP_RATE_32K },
	{ 44100, S5M8751_SAMP_RATE_44_1K },
	{ 48000, S5M8751_SAMP_RATE_48K },
	{ 64000, S5M8751_SAMP_RATE_64K },
	{ 88200, S5M8751_SAMP_RATE_88_2K },
	{ 96000, S5M8751_SAMP_RATE_96K },
};

static inline unsigned int s5m8751_codec_read(struct snd_soc_codec *codec,
					unsigned int reg)
{
	uint8_t value;
	struct s5m8751_priv *priv = snd_soc_codec_get_drvdata(codec);

	s5m8751_reg_read(priv->s5m8751, reg, &value);
	return value;
}

static int s5m8751_codec_write(struct snd_soc_codec *codec, unsigned int reg,
					unsigned int value)
{
	struct s5m8751_priv *priv = snd_soc_codec_get_drvdata(codec);
	return s5m8751_reg_write(priv->s5m8751, reg, value);
}

static int s5m8751_outpga_put_volsw_vu(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int ret;
	u16 val;

	ret = snd_soc_put_volsw(kcontrol, ucontrol);
	if (ret < 0)
		return ret;

	val = s5m8751_codec_read(codec, reg);
	return s5m8751_codec_write(codec, reg, val);
}

#define SOC_S5M8751_SINGLE_R_TLV(xname, reg, shift, max, invert,\
	 tlv_array) {\
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		  SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw, \
	.get = snd_soc_get_volsw, .put = s5m8751_outpga_put_volsw_vu, \
	.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }

static const DECLARE_TLV_DB_LINEAR(out_dac_tlv, 1200, -8400);

static const struct snd_kcontrol_new s5m8751_snd_controls[] = {
SOC_S5M8751_SINGLE_R_TLV("Left DAC Digital Volume",
	S5M8751_DA_VOLL,
	S5M8751_DA_VOLL_SHIFT,
	S5M8751_DA_VOLL_MASK,
	0,
	out_dac_tlv),
SOC_S5M8751_SINGLE_R_TLV("Right DAC Digital Volume",
	S5M8751_DA_VOLR,
	S5M8751_DA_VOLR_SHIFT,
	S5M8751_DA_VOLR_MASK,
	0,
	out_dac_tlv),

SOC_SINGLE("Right Lineout mute control", S5M8751_AMP_MUTE,
	S5M8751_AMP_MUTE_RLINE_SHIFT, 1, 1),
SOC_SINGLE("Left Lineout mute control", S5M8751_AMP_MUTE,
	S5M8751_AMP_MUTE_LLINE_SHIFT, 1, 1),
SOC_SINGLE("Right Headphone mute control", S5M8751_AMP_MUTE,
	S5M8751_AMP_MUTE_RHP_SHIFT, 1, 1),
SOC_SINGLE("Left Headphone mute control", S5M8751_AMP_MUTE,
	S5M8751_AMP_MUTE_LHP_SHIFT, 1, 1),
SOC_SINGLE("Speaker PGA mute", S5M8751_AMP_MUTE,
	S5M8751_AMP_MUTE_SPK_S2D_SHIFT, 1, 1),

SOC_DOUBLE_R("HeadPhone Volume", S5M8751_HP_VOL1, S5M8751_HP_VOL2,
							 0, 0x7f, 1),
SOC_DOUBLE_R("Line-In Gain", S5M8751_DA_LGAIN, S5M8751_DA_RGAIN,
							 0, 0x7f, 1),
SOC_DOUBLE_R("DAC Volume", S5M8751_DA_VOLL, S5M8751_DA_VOLR,
							 0, 0xff, 1),
SOC_SINGLE("Capless mode", S5M8751_AMP_EN, 4, 1, 0),
SOC_SINGLE("Speaker Slope", S5M8751_SPK_SLOPE, 0, 0xf, 1),
SOC_SINGLE("DAC Mute", S5M8751_DA_DIG2, 7, 1, 0),
SOC_SINGLE("Mute Dither", S5M8751_DA_DIG2, 6, 1, 0),
SOC_SINGLE("Dither Level", S5M8751_DA_DIG2, 3, 7, 0),
SOC_SINGLE("Soft Limit", S5M8751_DA_LIM1, 7, 1, 0),
SOC_SINGLE("Limit Thrsh", S5M8751_DA_LIM1, 0, 0x7f, 1),
SOC_SINGLE("Attack", S5M8751_DA_LIM2, 4, 0xf, 0),
SOC_SINGLE("Release", S5M8751_DA_LIM2, 0, 0xf, 0),
};

/* Mono Analog Mixer */
static const struct snd_kcontrol_new s5m8751_dapm_monomix_controls[] = {
SOC_DAPM_SINGLE("Mono Mixer LMUX", S5M8751_DA_AMIX2,
	S5M8751_LBYP_ML_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Mono Mixer LDAC", S5M8751_DA_AMIX1,
	S5M8751_LDAC_MM_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Mono Mixer RMUX", S5M8751_DA_AMIX2,
	S5M8751_RBYP_ML_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Mono Mixer RDAC", S5M8751_DA_AMIX1,
	S5M8751_RDAC_MM_SHIFT, 1, 0),
};

/* Left Analog Mixer */
static const struct snd_kcontrol_new s5m8751_dapm_leftmix_controls[] = {
SOC_DAPM_SINGLE("Left Mixer LMUX", S5M8751_DA_AMIX2,
	S5M8751_LBYP_ML_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Left Mixer LDAC", S5M8751_DA_AMIX1,
	S5M8751_LDAC_ML_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Left Mixer RMUX", S5M8751_DA_AMIX2,
	S5M8751_RBYP_ML_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Left Mixer RDAC", S5M8751_DA_AMIX1,
	S5M8751_RDAC_ML_SHIFT, 1, 0),
};

/* Right Analog Mixer */
static const struct snd_kcontrol_new s5m8751_dapm_rightmix_controls[] = {
SOC_DAPM_SINGLE("Right Mixer LMUX", S5M8751_DA_AMIX2,
	S5M8751_LBYP_MR_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Right Mixer LDAC", S5M8751_DA_AMIX1,
	S5M8751_LDAC_MR_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Right Mixer RMUX", S5M8751_DA_AMIX2,
	S5M8751_RBYP_MR_SHIFT, 1, 0),
SOC_DAPM_SINGLE("Right Mixer RDAC", S5M8751_DA_AMIX1,
	S5M8751_RDAC_MR_SHIFT, 1, 0),
};

static const struct snd_soc_dapm_widget s5m8751_dapm_widgets[] = {
/* Input Side */
/* Input Lines */
SND_SOC_DAPM_INPUT("LIN_L1_L2"),
SND_SOC_DAPM_INPUT("LIN_L1"),
SND_SOC_DAPM_INPUT("LIN_L2"),
SND_SOC_DAPM_INPUT("LIN_R1_R2"),
SND_SOC_DAPM_INPUT("LIN_R1"),
SND_SOC_DAPM_INPUT("LIN_R2"),

SND_SOC_DAPM_INPUT("internal DAC"),

/* Output Side */
/* DAC */
SND_SOC_DAPM_DAC("LDAC", "Playback", S5M8751_DA_PDB1,
	S5M8751_PDB_DACL, 0),
SND_SOC_DAPM_DAC("RDAC", "Playback", S5M8751_DA_PDB1,
	S5M8751_PDB_DACR, 0),

SND_SOC_DAPM_DAC("LMUX", "Playback", S5M8751_DA_LGAIN,
	S5M8751_PDB_LIN_MASK, 0),
SND_SOC_DAPM_DAC("RMUX", "Playback", S5M8751_DA_RGAIN,
	S5M8751_PDB_RIN_MASK, 0),

/* Mono Analog Mixer */
SND_SOC_DAPM_MIXER("Mono Mixer", S5M8751_DA_PDB1,
			S5M8751_PDB_MM_SHIFT, 0,
			&s5m8751_dapm_monomix_controls[0],
			ARRAY_SIZE(s5m8751_dapm_monomix_controls)),

/* Left Analog Mixer */
SND_SOC_DAPM_MIXER("Left Mixer", S5M8751_DA_PDB1,
			S5M8751_PDB_ML_SHIFT, 0,
			&s5m8751_dapm_leftmix_controls[0],
			ARRAY_SIZE(s5m8751_dapm_leftmix_controls)),

/* Right Analog Mixer */
SND_SOC_DAPM_MIXER("Right Mixer", S5M8751_DA_PDB1,
			S5M8751_PDB_MR_SHIFT, 0,
			&s5m8751_dapm_rightmix_controls[0],
			ARRAY_SIZE(s5m8751_dapm_rightmix_controls)),

/* */
SND_SOC_DAPM_PGA("RL Enable", S5M8751_AMP_EN,
	S5M8751_AMP_EN_RLINE_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("LL Enable", S5M8751_AMP_EN,
	S5M8751_AMP_EN_LLINE_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("LHP Enable", S5M8751_AMP_EN,
	S5M8751_AMP_EN_RHP_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("RHP Enable", S5M8751_AMP_EN,
	S5M8751_AMP_EN_LHP_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("SPK Enable", S5M8751_AMP_EN,
	S5M8751_AMP_EN_SPK_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("S2D Enable", S5M8751_AMP_EN,
	S5M8751_AMP_EN_SPK_S2D_SHIFT, 0, NULL, 0),

/* Output Lines */
SND_SOC_DAPM_OUTPUT("RL"),
SND_SOC_DAPM_OUTPUT("LL"),
SND_SOC_DAPM_OUTPUT("RH"),
SND_SOC_DAPM_OUTPUT("LH"),
SND_SOC_DAPM_OUTPUT("SPK"),
};

static const struct snd_soc_dapm_route audio_map[] = {
	/* Make DACs turn on when playing even if not mixed into any outputs */
	{ "LDAC", NULL, "internal DAC" },
	{ "RDAC", NULL, "internal DAC" },

	{ "LMUX", NULL, "LIN_L1_L2" },
	{ "LMUX", NULL, "LIN_L1" },
	{ "LMUX", NULL, "LIN_L2" },
	{ "RMUX", NULL, "LIN_R1_R2" },
	{ "RMUX", NULL, "LIN_R1" },
	{ "RMUX", NULL, "LIN_R2" },

	{ "Mono Mixer", "Mono Mixer LMUX", "LMUX" },
	{ "Mono Mixer", "Mono Mixer LDAC", "LDAC" },
	{ "Mono Mixer", "Mono Mixer RMUX", "RMUX" },
	{ "Mono Mixer", "Mono Mixer RDAC", "RDAC" },

	{ "Left Mixer", "Left Mixer LMUX", "LMUX" },
	{ "Left Mixer", "Left Mixer LDAC", "LDAC" },
	{ "Left Mixer", "Left Mixer RMUX", "RMUX" },
	{ "Left Mixer", "Left Mixer RDAC", "RDAC" },

	{ "Right Mixer", "Right Mixer LMUX", "LMUX" },
	{ "Right Mixer", "Right Mixer LDAC", "LDAC" },
	{ "Right Mixer", "Right Mixer RMUX", "RMUX" },
	{ "Right Mixer", "Right Mixer RDAC", "RDAC" },

	{ "RL Enable", NULL, "Right Mixer" },
	{ "LL Enable", NULL, "Left Mixer" },
	{ "LHP Enable", NULL, "Left Mixer" },
	{ "RHP Enable", NULL, "Right Mixer" },
	{ "S2D Enable", NULL, "Mono Mixer" },
	{ "SPK Enable", NULL, "S2D Enable" },

	{ "RL", NULL,  "RL Enable" },
	{ "LL", NULL,  "LL Enable" },
	{ "RH", NULL,  "RHP Enable" },
	{ "LH", NULL,  "LHP Enable" },
	{ "SPK", NULL, "SPK Enable" },
};

static int s5m8751_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, s5m8751_dapm_widgets,
				  ARRAY_SIZE(s5m8751_dapm_widgets));
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));
	snd_soc_dapm_new_widgets(codec);

	return 0;
}

static int s5m8751_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int count, ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	unsigned char in1_ctrl2, in1_ctrl1, amp_mute;

	amp_mute = S5M8751_ALL_AMP_MUTE;
	s5m8751_codec_write(codec, S5M8751_AMP_MUTE, amp_mute);

	in1_ctrl1 = s5m8751_codec_read(codec, S5M8751_IN1_CTRL1);
	in1_ctrl1 &= ~S5M8751_SAMP_RATE1_MASK;	/* Sampling Rate field */

	in1_ctrl2 = s5m8751_codec_read(codec, S5M8751_IN1_CTRL2);
	in1_ctrl2 &= ~S5M8751_I2S_DL1_MASK;	 /* I2S data length field */

	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		in1_ctrl2 |= S5M8751_I2S_DL_16BIT;
		break;
	case SNDRV_PCM_FORMAT_S18_3LE:
		in1_ctrl2 |= S5M8751_I2S_DL_18BIT;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		in1_ctrl2 |= S5M8751_I2S_DL_20BIT;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		in1_ctrl2 |= S5M8751_I2S_DL_24BIT;
		break;
	default:
		return -EINVAL;
	}

	for (count = 0 ; count < ARRAY_SIZE(s5m8751_rate) ; count++) {
		if (params_rate(params) == s5m8751_rate[count].rate) {
			in1_ctrl1 |= s5m8751_rate[count].reg_rate;
			ret = 0;
			break;
		} else
			ret = -EINVAL;
	}

	s5m8751_codec_write(codec, S5M8751_IN1_CTRL1, in1_ctrl1);
	s5m8751_codec_write(codec, S5M8751_IN1_CTRL2, in1_ctrl2);

	return ret;
}

static int s5m8751_set_dai_fmt(struct snd_soc_dai *codec_dai,
							unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned char in1_ctrl2, in1_ctrl1, amp_mute;

	amp_mute = S5M8751_ALL_AMP_MUTE;
	s5m8751_codec_write(codec, S5M8751_AMP_MUTE, amp_mute);

	in1_ctrl2 = s5m8751_codec_read(codec, S5M8751_IN1_CTRL2);
	in1_ctrl2 &= ~S5M8751_I2S_DF1_MASK;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		in1_ctrl2 |= S5M8751_I2S_STANDARD;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		in1_ctrl2 |= S5M8751_I2S_LJ;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		in1_ctrl2 |= S5M8751_I2S_RJ;
		break;
	default:
		printk(KERN_ERR "DAIFmt(%d) not supported!\n", fmt
					& SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	in1_ctrl2 &= ~S5M8751_LRCK_POL1_MASK;
	in1_ctrl1 = s5m8751_codec_read(codec, S5M8751_IN1_CTRL1);
	in1_ctrl1 &= ~S5M8751_SCK_POL1_MASK;

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		in1_ctrl1 &= ~S5M8751_SCK_POL;
		in1_ctrl2 &= ~S5M8751_LRCK_POL;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		in1_ctrl1 |= S5M8751_SCK_POL;
		in1_ctrl2 |= S5M8751_LRCK_POL;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		in1_ctrl1 |= S5M8751_SCK_POL;
		in1_ctrl2 &= ~S5M8751_LRCK_POL;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		in1_ctrl1 &= ~S5M8751_SCK_POL;
		in1_ctrl2 |= S5M8751_LRCK_POL;
		break;
	default:
		printk(KERN_ERR "Inv-combo(%d) not supported!\n", fmt &
					 SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	s5m8751_codec_write(codec, S5M8751_IN1_CTRL1, in1_ctrl1);
	s5m8751_codec_write(codec, S5M8751_IN1_CTRL2, in1_ctrl2);

	return 0;
}

static int s5m8751_set_dai_clkdiv(struct snd_soc_dai *codec_dai,
				 int div_id, int val)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned char in1_ctrl1, in1_ctrl2, amp_mute;

	amp_mute = S5M8751_ALL_AMP_MUTE;
	s5m8751_codec_write(codec, S5M8751_AMP_MUTE, amp_mute);

	in1_ctrl2 = s5m8751_codec_read(codec, S5M8751_IN1_CTRL2);
	in1_ctrl2 &= ~S5M8751_I2S_XFS1_MASK;

	switch (div_id) {
	case S5M8751_BCLK:
		switch (val) {
		case 32:
			in1_ctrl2 |= S5M8751_I2S_SCK_32FS;
			break;
		case 48:
			in1_ctrl2 |= S5M8751_I2S_SCK_48FS;
			break;
		case 64:
			in1_ctrl2 |= S5M8751_I2S_SCK_64FS;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	in1_ctrl1 = s5m8751_codec_read(codec, S5M8751_IN1_CTRL1);
	s5m8751_codec_write(codec, S5M8751_IN1_CTRL1, in1_ctrl1);
	s5m8751_codec_write(codec, S5M8751_IN1_CTRL2, in1_ctrl2);

	return 0;
}

static int s5m8751_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	uint8_t reg;

	reg = s5m8751_codec_read(codec, S5M8751_DA_DIG2);
	if (mute)
		reg |= S5M8751_DAC_MUTE;
	else
		reg &= ~S5M8751_DAC_MUTE;

	s5m8751_codec_write(codec, S5M8751_DA_DIG2, reg);
	s5m8751_codec_write(codec, S5M8751_AMP_MUTE, S5M8751_SPK_S2D_MUTEB |
				S5M8751_LHP_MUTEB | S5M8751_RHP_MUTEB);

	return 0;
}

static int s5m8751_set_bias_level(struct snd_soc_codec *codec,
	enum snd_soc_bias_level level)
{
	u8 da_pdb1, amp_en, amp_mute;

	switch (level) {
	case SND_SOC_BIAS_ON:
		amp_mute = (S5M8751_LHP_MUTEB | S5M8751_RHP_MUTEB) |
					S5M8751_SPK_S2D_MUTEB;
		s5m8751_codec_write(codec, S5M8751_AMP_MUTE, amp_mute);
		break;
	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_PREPARE:
		amp_mute = S5M8751_ALL_AMP_MUTE;
		s5m8751_codec_write(codec, S5M8751_AMP_MUTE, amp_mute);
		break;
	case SND_SOC_BIAS_OFF:
		amp_en = S5M8751_ALL_AMP_OFF;
		amp_mute = S5M8751_ALL_AMP_MUTE;
		da_pdb1 = S5M8751_ALL_DA_PDB1_OFF | S5M8751_VMID_250K;

		s5m8751_codec_write(codec, S5M8751_DA_PDB1, da_pdb1);
		s5m8751_codec_write(codec, S5M8751_AMP_MUTE, amp_mute);
		s5m8751_codec_write(codec, S5M8751_AMP_EN, amp_en);
		break;
	}
	codec->bias_level = level;
	return 0;
}

#define S5M8751_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops s5m8751_dai_ops_playback = {
	.hw_params	= s5m8751_hw_params,
	.set_fmt	= s5m8751_set_dai_fmt,
	.set_clkdiv	= s5m8751_set_dai_clkdiv,
	.digital_mute	= s5m8751_digital_mute,
};

struct snd_soc_dai s5m8751_dai = {
	.name = "S5M8751",
	.id = S5M8751_CODEC,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000,
		.formats = S5M8751_FORMATS,
	},
	.ops = &s5m8751_dai_ops_playback,
};
EXPORT_SYMBOL_GPL(s5m8751_dai);

static struct snd_soc_codec *s5m8751_codec;

static int s5m8751_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;

	if (s5m8751_codec == NULL) {
		dev_err(&pdev->dev, "Codec device isn't registered\n");
		return -ENOMEM;
	}

	socdev->card->codec = s5m8751_codec;
	codec = s5m8751_codec;

	/* register pcm */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to create pcm: %d\n", ret);
		goto pcm_err;
	}

	snd_soc_add_controls(codec, s5m8751_snd_controls,
			ARRAY_SIZE(s5m8751_snd_controls));
	s5m8751_add_widgets(codec);

	return ret;

pcm_err:
	return ret;
}

static int s5m8751_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_s5m8751 = {
	.probe =	s5m8751_probe,
	.remove =	s5m8751_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_s5m8751);

static int s5m8751_codec_probe(struct platform_device *pdev)
{
	struct s5m8751_priv *priv;
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_audio_pdata *pdata = s5m8751_pdata->audio;
	struct snd_soc_codec *codec;
	uint8_t reg_val;
	int ret = 0;

	priv = kzalloc(sizeof(struct s5m8751_priv), GFP_KERNEL);
	if (priv == NULL)
		goto err;

	priv->s5m8751 = s5m8751;
	codec = &priv->codec;

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	snd_soc_codec_set_drvdata(codec, priv);

	codec->name = "S5M8751";
	codec->owner = THIS_MODULE;
	codec->read = s5m8751_codec_read;
	codec->write = s5m8751_codec_write;
	codec->bias_level = SND_SOC_BIAS_OFF;
	codec->set_bias_level = s5m8751_set_bias_level;
	codec->dai = &s5m8751_dai;
	codec->num_dai = 1;

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	s5m8751_codec = codec;

	reg_val = s5m8751_codec_read(codec, S5M8751_AMP_CTRL);
	s5m8751_codec_write(codec, S5M8751_AMP_CTRL, reg_val |
						S5M8751_HP_AMP_MUTE_CONTROL_ON);

	s5m8751_codec_write(codec, S5M8751_DA_PDB1, S5M8751_DA_PDB1_SPK |
						S5M8751_VMID_50K);
	s5m8751_codec_write(codec, S5M8751_DA_AMIX1, S5M8751_DA_AMIX1_SPK);

	reg_val = s5m8751_codec_read(codec, S5M8751_IN1_CTRL1);
	reg_val &= ~S5M8751_LOGIC_PDN;
	s5m8751_codec_write(codec, S5M8751_IN1_CTRL1, reg_val);
	reg_val |= S5M8751_LOGIC_PDN;
	s5m8751_codec_write(codec, S5M8751_IN1_CTRL1, reg_val);

	s5m8751_codec_write(codec, S5M8751_DA_VOLL, pdata->dac_vol);
	s5m8751_codec_write(codec, S5M8751_DA_VOLR, pdata->dac_vol);
	s5m8751_codec_write(codec, S5M8751_HP_VOL1, pdata->hp_vol);
	s5m8751_codec_write(codec, S5M8751_HP_VOL2, pdata->hp_vol);
	s5m8751_codec_write(codec, S5M8751_AMP_EN, S5M8751_SPK_AMP_EN);

	codec->dev = &pdev->dev;
	s5m8751_dai.dev = codec->dev;

	s5m8751_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	s5m8751_codec = codec;

	ret = snd_soc_register_codec(codec);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to register codec: %d\n", ret);
		goto err_codec;
	}

	ret = snd_soc_register_dais(&s5m8751_dai, 1);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to register DAI: %d\n", ret);
		goto err_codec;
	}

	return 0;

err_codec:
	snd_soc_unregister_codec(codec);

err:
	kfree(priv);
	return ret;
}

static int s5m8751_codec_remove(struct platform_device *pdev)
{
	struct s5m8751_priv *priv = snd_soc_codec_get_drvdata(s5m8751_codec);

	snd_soc_unregister_dai(&s5m8751_dai);
	snd_soc_unregister_codec(s5m8751_codec);

	kfree(priv);
	s5m8751_codec = NULL;

	return 0;
}

static struct platform_driver s5m8751_codec_driver = {
	.driver = {
		.name = "s5m8751-codec",
		.owner = THIS_MODULE,
	},
	.probe = s5m8751_codec_probe,
	.remove	= s5m8751_codec_remove,
};

static int __init s5m8751_codec_init(void)
{
	return platform_driver_register(&s5m8751_codec_driver);
}
module_init(s5m8751_codec_init);

static void __exit s5m8751_codec_exit(void)
{
	platform_driver_unregister(&s5m8751_codec_driver);
}
module_exit(s5m8751_codec_exit);

MODULE_DESCRIPTION("S5M8751 Audio DAC driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
