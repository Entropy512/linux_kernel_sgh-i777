/*
 * s3c-idma.c  --  I2S0's Internal Dma driver
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "regs-i2s-v2.h"
#include "s3c-dma.h"
#include "s3c-idma.h"

/** Debug **/
#include <mach/map.h>
#include <mach/regs-clock.h>
#define dump_i2s()	do {	\
				printk(KERN_INFO	\
					"%s:%s:%d\n",	\
					__FILE__, __func__, __LINE__);	\
				printk(KERN_INFO	\
					"\tS3C_IISCON : %x",	\
					readl(s3c_idma.regs + S3C2412_IISCON));\
				printk(KERN_INFO	\
					"\tS3C_IISMOD : %x\n",	\
					readl(s3c_idma.regs + S3C2412_IISMOD));\
				printk(KERN_INFO	\
					"\tS3C_IISFIC : %x",	\
					readl(s3c_idma.regs + S3C2412_IISFIC));\
				printk(KERN_INFO	\
					"\tS3C_IISFICS : %x",	\
					readl(s3c_idma.regs + S5P_IISFICS));\
				printk(KERN_INFO	\
					"\tS3C_IISPSR : %x\n",	\
					readl(s3c_idma.regs + S3C2412_IISPSR));\
				printk(KERN_INFO	\
					"\tS3C_IISAHB : %x\n",	\
					readl(s3c_idma.regs + S5P_IISAHB));\
				printk(KERN_INFO	\
					"\tS3C_IISSTR : %x\n",	\
					readl(s3c_idma.regs + S5P_IISSTR));\
				printk(KERN_INFO	\
					"\tS3C_IISSIZE : %x\n",	\
					readl(s3c_idma.regs + S5P_IISSIZE));\
				printk(KERN_INFO	\
					"\tS3C_IISADDR0 : %x\n",	\
					readl(s3c_idma.regs + S5P_IISADDR0));\
				printk(KERN_INFO	\
					"\tS5P_CLKGATE_D20 : %x\n",	\
					readl(S5P_CLKGATE_D20));\
				printk(KERN_INFO	\
					"\tS5P_LPMP_MODE_SEL : %x\n",\
					readl(S5P_LPMP_MODE_SEL));\
			} while (0)

static const struct snd_pcm_hardware s3c_idma_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED |
		    SNDRV_PCM_INFO_BLOCK_TRANSFER |
		    SNDRV_PCM_INFO_MMAP |
		    SNDRV_PCM_INFO_MMAP_VALID |
		    SNDRV_PCM_INFO_PAUSE |
		    SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_S16_LE |
		    SNDRV_PCM_FMTBIT_U16_LE |
		    SNDRV_PCM_FMTBIT_S24_LE |
		    SNDRV_PCM_FMTBIT_U24_LE |
		    SNDRV_PCM_FMTBIT_U8 |
		    SNDRV_PCM_FMTBIT_S8,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = MAX_LP_BUFF,
	.period_bytes_min = 128,
	.period_bytes_max = 16 * 1024,
	.periods_min = 2,
	.periods_max = 128,
	.fifo_size = 64,
};

struct lpam_i2s_pdata {
	spinlock_t	lock;
	int		state;
	dma_addr_t	start;
	dma_addr_t	pos;
	dma_addr_t	end;
	dma_addr_t	period;
	dma_addr_t	periodsz;
	void		*token;
	void		(*cb)(void *dt, int bytes_xfer);
};

	/********************
	 * Internal DMA i/f *
	 ********************/
static struct s3c_idma_info {
	spinlock_t    lock;
	void __iomem  *regs;
} s3c_idma;

static void s3c_idma_getpos(dma_addr_t *src)
{
	*src = LP_TXBUFF_ADDR +
		(readl(s3c_idma.regs + S5P_IISTRNCNT) & 0xffffff) * 4;
}

void i2sdma_getpos(dma_addr_t *src)
{
	if (!audio_clk_gated)
		*src = LP_TXBUFF_ADDR +
			(readl(s3c_idma.regs + S5P_IISTRNCNT) & 0xffffff) * 4;
	else
		*src = LP_TXBUFF_ADDR;
}

static int s3c_idma_enqueue(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct lpam_i2s_pdata *prtd = substream->runtime->private_data;
	u32 val;

	pr_debug("%s: %x@%x\n", __func__, MAX_LP_BUFF, LP_TXBUFF_ADDR);

	spin_lock(&prtd->lock);
	prtd->token = (void *) substream;
	spin_unlock(&prtd->lock);

	/* Internal DMA Level0 Interrupt Address */
	val = LP_TXBUFF_ADDR + prtd->periodsz;
	writel(val, s3c_idma.regs + S5P_IISADDR0);

	/* Start address0 of I2S internal DMA operation. */
	val = readl(s3c_idma.regs + S5P_IISSTR);
	val = LP_TXBUFF_ADDR;
	writel(val, s3c_idma.regs + S5P_IISSTR);

	/*
	 * Transfer block size for I2S internal DMA.
	 * Should decide transfer size before start dma operation
	 */
	val = readl(s3c_idma.regs + S5P_IISSIZE);
	val &= ~(S5P_IISSIZE_TRNMSK << S5P_IISSIZE_SHIFT);

	val |= (((runtime->dma_bytes >> 2) &
			S5P_IISSIZE_TRNMSK) << S5P_IISSIZE_SHIFT);
	writel(val, s3c_idma.regs + S5P_IISSIZE);

	return 0;
}

static void s3c_idma_setcallbk(struct snd_pcm_substream *substream,
				void (*cb)(void *, int))
{
	struct lpam_i2s_pdata *prtd = substream->runtime->private_data;

	spin_lock(&prtd->lock);
	prtd->cb = cb;
	spin_unlock(&prtd->lock);

	pr_debug("%s:%d dma_period=%x\n", __func__, __LINE__, prtd->periodsz);
}

static void s3c_idma_ctrl(int op)
{
	u32 val;

	pr_debug("Entered %s\n", __func__);

	val = readl(s3c_idma.regs + S5P_IISAHB);

	switch (op) {
	case LPAM_DMA_START:
		val |= (S5P_IISAHB_INTENLVL0 | S5P_IISAHB_DMAEN);
		break;
	case LPAM_DMA_STOP:
		/* Disable LVL Interrupt and DMA Operation */
		val &= ~(S5P_IISAHB_INTENLVL0 | S5P_IISAHB_DMAEN);
		break;
	default:
		spin_unlock(&s3c_idma.lock);
		return;
	}

	writel(val, s3c_idma.regs + S5P_IISAHB);
}

static void s3c_idma_done(void *id, int bytes_xfer)
{
	struct snd_pcm_substream *substream = id;
	struct lpam_i2s_pdata *prtd = substream->runtime->private_data;

	pr_debug("%s:%d\n", __func__, __LINE__);

	if (prtd && (prtd->state & ST_RUNNING))
		snd_pcm_period_elapsed(substream);
}

static int s3c_idma_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct lpam_i2s_pdata *prtd = substream->runtime->private_data;

	pr_debug("Entered %s\n", __func__);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);
	memset(runtime->dma_area, 0, runtime->dma_bytes);

	prtd->start = prtd->pos = runtime->dma_addr;
	prtd->period = params_periods(params);
	prtd->periodsz = params_period_bytes(params);
	prtd->end = LP_TXBUFF_ADDR + runtime->dma_bytes;

	s3c_idma_setcallbk(substream, s3c_idma_done);

	pr_info("DmaAddr=@%x Total=%dbytes PrdSz=%d #Prds=%d dmaEnd=0x%x\n",
			prtd->start, runtime->dma_bytes, prtd->periodsz,
			prtd->period, prtd->end);

	return 0;
}

static int s3c_idma_hw_free(struct snd_pcm_substream *substream)
{
	pr_debug("Entered %s\n", __func__);

	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int s3c_idma_prepare(struct snd_pcm_substream *substream)
{
	struct lpam_i2s_pdata *prtd = substream->runtime->private_data;

	pr_debug("Entered %s\n", __func__);

	prtd->pos = prtd->start;

	/* flush the DMA channel */
	s3c_idma_ctrl(LPAM_DMA_STOP);
	s3c_idma_enqueue(substream);

	return 0;
}

static int s3c_idma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct lpam_i2s_pdata *prtd = substream->runtime->private_data;
	int ret = 0;

	pr_debug("Entered %s\n", __func__);

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;
		s3c_idma_ctrl(LPAM_DMA_START);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
		s3c_idma_ctrl(LPAM_DMA_STOP);
		break;

	default:
		spin_unlock(&prtd->lock);
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t
	s3c_idma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct lpam_i2s_pdata *prtd = runtime->private_data;
	dma_addr_t src;
	unsigned long res;

	spin_lock(&prtd->lock);

	s3c_idma_getpos(&src);
	res = src - prtd->start;

	spin_unlock(&prtd->lock);

	if (res >= snd_pcm_lib_buffer_bytes(substream)) {
		if (res == snd_pcm_lib_buffer_bytes(substream))
			res = 0;
	}

	return bytes_to_frames(substream->runtime, res);
}

static int s3c_idma_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long size, offset;
	int ret;

	pr_debug("Entered %s\n", __func__);

	/* From snd_pcm_lib_mmap_iomem */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_IO;
	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;
	ret = io_remap_pfn_range(vma, vma->vm_start,
			(runtime->dma_addr + offset) >> PAGE_SHIFT,
			size, vma->vm_page_prot);

	return ret;
}

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
	struct lpam_i2s_pdata *prtd = (struct lpam_i2s_pdata *)dev_id;
	u32 iiscon, iisahb, val, addr;

	/* dump_i2s(); */
	iisahb  = readl(s3c_idma.regs + S5P_IISAHB);
	iiscon  = readl(s3c_idma.regs + S3C2412_IISCON);

	if (iiscon & (1<<26)) {
		pr_info("RxFIFO overflow interrupt\n");
		writel(iiscon | (1<<26), s3c_idma.regs+S3C2412_IISCON);
	}
	if (iiscon & S5P_IISCON_FTXSURSTAT) {
		iiscon |= S5P_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_idma.regs + S3C2412_IISCON);
		pr_debug("TX_S underrun interrupt IISCON = 0x%08x\n",
				readl(s3c_idma.regs + S3C2412_IISCON));
	}

	if (iiscon & S5P_IISCON_FTXURSTATUS) {
		iiscon &= ~S5P_IISCON_FTXURINTEN;
		iiscon |= S5P_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_idma.regs + S3C2412_IISCON);
		pr_debug("TX_P underrun interrupt IISCON = 0x%08x\n",
				readl(s3c_idma.regs + S3C2412_IISCON));
	}

	/* Check internal DMA level interrupt. */
	if (iisahb & S5P_IISAHB_LVL0INT)
		val = S5P_IISAHB_CLRLVL0;
	else
		val = 0;

	if (val) {
		iisahb |= val;
		writel(iisahb, s3c_idma.regs + S5P_IISAHB);

		addr = readl(s3c_idma.regs + S5P_IISADDR0);
		addr += prtd->periodsz;

		if (addr >= prtd->end)
			addr = LP_TXBUFF_ADDR;

		writel(addr, s3c_idma.regs + S5P_IISADDR0);

		/* Finished dma transfer ? */
		if (iisahb & S5P_IISLVLINTMASK) {
			if (prtd->cb)
				prtd->cb(prtd->token, prtd->periodsz);
		}
	}

	return IRQ_HANDLED;
}

static int s3c_idma_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct lpam_i2s_pdata *prtd;
	int ret;

	pr_debug("Entered %s\n", __func__);

	snd_soc_set_runtime_hwparams(substream, &s3c_idma_hardware);

	prtd = kzalloc(sizeof(struct lpam_i2s_pdata), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	ret = request_irq(IRQ_I2S0, s3c_iis_irq, 0, "s3c-i2s", prtd);
	if (ret < 0) {
		pr_err("fail to claim i2s irq , ret = %d\n", ret);
		kfree(prtd);
		return ret;
	}

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;

	return 0;
}

static int s3c_idma_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct lpam_i2s_pdata *prtd = runtime->private_data;

	pr_debug("Entered %s, prtd = %p\n", __func__, prtd);

	free_irq(IRQ_I2S0, prtd);

	if (!prtd)
		pr_err("s3c_idma_close called with prtd == NULL\n");

	kfree(prtd);

	return 0;
}

static struct snd_pcm_ops s3c_idma_ops = {
	.open		= s3c_idma_open,
	.close		= s3c_idma_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.trigger	= s3c_idma_trigger,
	.pointer	= s3c_idma_pointer,
	.mmap		= s3c_idma_mmap,
	.hw_params	= s3c_idma_hw_params,
	.hw_free	= s3c_idma_hw_free,
	.prepare	= s3c_idma_prepare,
};

static void s3c_idma_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	pr_debug("Entered %s\n", __func__);

	substream = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;;

	iounmap(buf->area);

	buf->area = NULL;
	buf->addr = 0;
}

static int s3c_idma_preallocate_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;

	pr_debug("Entered %s\n", __func__);
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	/* Assign PCM buffer pointers */
	buf->dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
	buf->addr = LP_TXBUFF_ADDR;
	buf->bytes = s3c_idma_hardware.buffer_bytes_max;
	buf->area = (unsigned char *)ioremap(buf->addr, buf->bytes);
	pr_info("%s:  VA-%p  PA-%X  %ubytes\n",
			__func__, buf->area, buf->addr, buf->bytes);

	return 0;
}

static u64 s3c_idma_mask = DMA_BIT_MASK(32);

static int s3c_idma_pcm_new(struct snd_card *card,
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;

	pr_debug("Entered %s\n", __func__);

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &s3c_idma_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (dai->playback.channels_min)
		ret = s3c_idma_preallocate_buffer(pcm,
				SNDRV_PCM_STREAM_PLAYBACK);

	return ret;
}

void s5p_idma_init(void *regs)
{
	spin_lock_init(&s3c_idma.lock);
	s3c_idma.regs = regs;
}

#ifdef CONFIG_SND_S5P_RP
int s5p_i2s_idma_irq_callback(void)
{
	u32 iisahb;
	int ret = 0;

	iisahb = readl(s3c_idma.regs + S5P_IISAHB);

	if (iisahb & S5P_IISAHB_LVL0INT) {
		iisahb |= S5P_IISAHB_CLRLVL0;
		ret = 1;
	}

	if (iisahb & S5P_IISAHB_LVL1INT) {
		iisahb |= S5P_IISAHB_CLRLVL1;
		ret = 1;
	}

	if (ret)
		writel(iisahb, s3c_idma.regs + S5P_IISAHB);

	return ret;
}
EXPORT_SYMBOL(s5p_i2s_idma_irq_callback);

void s5p_i2s_idma_enable(unsigned long frame_size_bytes)
{
#if 0	/* not required for C210 SRP */
	u32 iisahb, iismod;

	iismod  = readl(s3c_idma.regs + S3C2412_IISMOD);
	iismod |= S5P_IISMOD_TXSLP;
	writel(iismod, s3c_idma.regs + S3C2412_IISMOD);

	iisahb  = readl(s3c_idma.regs + S5P_IISAHB);
	iisahb |= S5P_IISAHB_DMA_STRADDRRST | S5P_IISAHB_DMA_STRADDRTOG
			| S5P_IISAHB_DMARLD | S5P_IISAHB_DISRLDINT
			| S5P_IISAHB_DMACLR;

	/* OBUF Address / Size */
	writel(0x0202A000, s3c_idma.regs + S5P_IISSTR);		/* OBUF0 */
	writel(0x02035000, s3c_idma.regs + S5P_IISSTR1);	/* OBUF1 */
	writel(((frame_size_bytes >> 2) & S5P_IISSIZE_TRNMSK) << S5P_IISSIZE_SHIFT,
		s3c_idma.regs + S5P_IISSIZE);

	writel(iisahb, s3c_idma.regs + S5P_IISAHB);

	/* Enable DMA */
	iisahb |= S5P_IISAHB_DMAEN;
	writel(iisahb, s3c_idma.regs + S5P_IISAHB);
#endif
}
EXPORT_SYMBOL(s5p_i2s_idma_enable);

void s5p_i2s_idma_pause(void)
{
#if 0	/* not required for C210 SRP */
	u32 val;

	val  = readl(s3c_idma.regs + S5P_IISAHB);
	val &= ~S5P_IISAHB_DMARLD;
	writel(val, s3c_idma.regs + S5P_IISAHB);
#endif
}
EXPORT_SYMBOL(s5p_i2s_idma_pause);

void s5p_i2s_idma_continue(void)
{
#if 0	/* not required for C210 SRP */
	u32 val;

	val  = readl(s3c_idma.regs + S5P_IISAHB);
	val |= S5P_IISAHB_DMARLD | S5P_IISAHB_DMAEN;
	writel(val, s3c_idma.regs + S5P_IISAHB);
#endif
}
EXPORT_SYMBOL(s5p_i2s_idma_continue);

void s5p_i2s_idma_stop(void)
{
	u32 val;

	val  = readl(s3c_idma.regs + S5P_IISAHB);
	val &= ~S5P_IISAHB_DMARLD;
	val |= S5P_IISAHB_DMA_STRADDRRST;
	val &= ~S5P_IISAHB_DMAEN;
	val &= ~(S5P_IISAHB_LVL0INT | S5P_IISAHB_LVL1INT);
	val |= S5P_IISAHB_CLRLVL0 | S5P_IISAHB_CLRLVL1;
	writel(val, s3c_idma.regs + S5P_IISAHB);

	writel(0, s3c_idma.regs + S5P_IISAHB);
	writel(0x00000000, s3c_idma.regs + S5P_IISADDR0);
	writel(0x00000000, s3c_idma.regs + S5P_IISADDR1);
}
EXPORT_SYMBOL(s5p_i2s_idma_stop);
#endif	/* _ULP_AUDIO_ */

struct snd_soc_platform idma_soc_platform = {
	.name = "s5p-lp-audio",
	.pcm_ops = &s3c_idma_ops,
	.pcm_new = s3c_idma_pcm_new,
	.pcm_free = s3c_idma_pcm_free,
};
EXPORT_SYMBOL_GPL(idma_soc_platform);

MODULE_AUTHOR("Jaswinder Singh, jassi.brar@samsung.com");
MODULE_DESCRIPTION("Samsung S5P LP-Audio DMA module");
MODULE_LICENSE("GPL");
