/* linux/drivers/mmc/host/sdhci-s3c.c
 *
 * Copyright 2008 Openmoko Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * SDHCI (HSMMC) support for Samsung SoC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <linux/mmc/host.h>

#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/regs-sdhci.h>

#include <linux/regulator/consumer.h>

#include "sdhci.h"

#define MAX_BUS_CLK	(4)

/**
 * struct sdhci_s3c - S3C SDHCI instance
 * @host: The SDHCI host created
 * @pdev: The platform device we where created from.
 * @ioarea: The resource created when we claimed the IO area.
 * @pdata: The platform data for this controller.
 * @cur_clk: The index of the current bus clock.
 * @clk_io: The clock for the internal bus interface.
 * @clk_bus: The clocks that are available for the SD/MMC bus clock.
 */
struct sdhci_s3c {
	struct sdhci_host	*host;
	struct platform_device	*pdev;
	struct resource		*ioarea;
	struct s3c_sdhci_platdata *pdata;
	unsigned int		cur_clk;
	int                     ext_cd_irq;
	int                     ext_cd_gpio;

	struct clk		*clk_io;
	struct clk		*clk_bus[MAX_BUS_CLK];
	
	struct workqueue_struct *regulator_workq;
	struct delayed_work	regul_work;
};

static inline struct sdhci_s3c *to_s3c(struct sdhci_host *host)
{
	return sdhci_priv(host);
}

/**
 * get_curclk - convert ctrl2 register to clock source number
 * @ctrl2: Control2 register value.
 */
static u32 get_curclk(u32 ctrl2)
{
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 >>= S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;

	return ctrl2;
}

static void sdhci_s3c_check_sclk(struct sdhci_host *host)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	u32 tmp = readl(host->ioaddr + S3C_SDHCI_CONTROL2);

	if (get_curclk(tmp) != ourhost->cur_clk) {
		dev_dbg(&ourhost->pdev->dev, "restored ctrl2 clock setting\n");

		tmp &= ~S3C_SDHCI_CTRL2_SELBASECLK_MASK;
		tmp |= ourhost->cur_clk << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;
		writel(tmp, host->ioaddr + 0x80);
	}
}

/**
 * sdhci_s3c_get_max_clk - callback to get maximum clock frequency.
 * @host: The SDHCI host instance.
 *
 * Callback to return the maximum clock rate acheivable by the controller.
*/
static unsigned int sdhci_s3c_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	struct clk *busclk;
	unsigned int rate, max;
	int clk;

	if(host->quirks & SDHCI_QUIRK_BROKEN_CLOCK_DIVIDER) {
		rate = clk_round_rate(ourhost->clk_bus[ourhost->cur_clk],
			UINT_MAX);
		return rate;
	}

	/* note, a reset will reset the clock source */

	sdhci_s3c_check_sclk(host);

	for (max = 0, clk = 0; clk < MAX_BUS_CLK; clk++) {
		busclk = ourhost->clk_bus[clk];
		if (!busclk)
			continue;

		rate = clk_get_rate(busclk);
		if (rate > max)
			max = rate;
	}

	return max;
}

/**
 * sdhci_s3c_consider_clock - consider one the bus clocks for current setting
 * @ourhost: Our SDHCI instance.
 * @src: The source clock index.
 * @wanted: The clock frequency wanted.
 */
static unsigned int sdhci_s3c_consider_clock(struct sdhci_s3c *ourhost,
					     unsigned int src,
					     unsigned int wanted)
{
	unsigned long rate;
	struct clk *clksrc = ourhost->clk_bus[src];
	int div;

	if (!clksrc)
		return UINT_MAX;

	if(ourhost->host->quirks & SDHCI_QUIRK_BROKEN_CLOCK_DIVIDER) {
		rate = clk_round_rate(clksrc,wanted);
		return (wanted - rate);
	}

	rate = clk_get_rate(clksrc);

	for (div = 1; div < 256; div *= 2) {
		if ((rate / div) <= wanted)
			break;
	}

	dev_dbg(&ourhost->pdev->dev, "clk %d: rate %ld, want %d, got %ld\n",
		src, rate, wanted, rate / div);

	return (wanted - (rate / div));
}

/**
 * sdhci_s3c_set_clock - callback on clock change
 * @host: The SDHCI host being changed
 * @clock: The clock rate being requested.
 *
 * When the card's clock is going to be changed, look at the new frequency
 * and find the best clock source to go with it.
*/
static void sdhci_s3c_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	unsigned int best = UINT_MAX;
	unsigned int delta;
	int best_src = 0;
	int src;
	u32 ctrl;
	unsigned int timeout;

	/* don't bother if the clock is going off. */
	if (clock == 0) {
		writew(0, host->ioaddr + SDHCI_CLOCK_CONTROL);
		host->clock = clock;
		return;
	}

	for (src = 0; src < MAX_BUS_CLK; src++) {
		delta = sdhci_s3c_consider_clock(ourhost, src, clock);
		if (delta < best) {
			best = delta;
			best_src = src;
		}
	}

	dev_dbg(&ourhost->pdev->dev,
		"selected source %d, clock %d, delta %d\n",
		 best_src, clock, best);

	/* select the new clock source */

	if (ourhost->cur_clk != best_src) {
		struct clk *clk = ourhost->clk_bus[best_src];

		/* turn clock off to card before changing clock source */
		writew(0, host->ioaddr + SDHCI_CLOCK_CONTROL);

		ourhost->cur_clk = best_src;
		host->max_clk = clk_get_rate(clk);

		ctrl = readl(host->ioaddr + S3C_SDHCI_CONTROL2);
		ctrl &= ~S3C_SDHCI_CTRL2_SELBASECLK_MASK;
		ctrl |= best_src << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;
		writel(ctrl, host->ioaddr + S3C_SDHCI_CONTROL2);
	}

	/* reconfigure the hardware for new clock rate */

	{
		struct mmc_ios ios;

		ios.clock = clock;

		if (ourhost->pdata->cfg_card)
			(ourhost->pdata->cfg_card)(ourhost->pdev, host->ioaddr,
						   &ios, NULL);
	}

	if(host->quirks & SDHCI_QUIRK_BROKEN_CLOCK_DIVIDER) {
		writew(0, host->ioaddr + SDHCI_CLOCK_CONTROL);
		clk_set_rate(ourhost->clk_bus[ourhost->cur_clk], clock);

		writew(SDHCI_CLOCK_INT_EN, host->ioaddr + SDHCI_CLOCK_CONTROL);

		/* Wait max 20 ms */
		timeout = 20;
		while (!((sdhci_readw(host, SDHCI_CLOCK_CONTROL))
			& SDHCI_CLOCK_INT_STABLE)) {
			if (timeout == 0) {
				printk(KERN_ERR "%s: Internal clock never "
					"stabilised.\n", mmc_hostname(host->mmc));
				return;
			}
			timeout--;
			mdelay(1);
		}

		writew(SDHCI_CLOCK_INT_EN | SDHCI_CLOCK_CARD_EN,
				host->ioaddr + SDHCI_CLOCK_CONTROL);

		host->clock = clock;
	}
}

/**
 * sdhci_s3c_get_min_clock - callback to get minimal supported clock value
 * @host: The SDHCI host being queried
 *
 * To init mmc host properly a minimal clock value is needed. For high system
 * bus clock's values the standard formula gives values out of allowed range.
 * The clock still can be set to lower values, if clock source other then
 * system bus is selected.
*/
static unsigned int sdhci_s3c_get_min_clock(struct sdhci_host *host)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	unsigned int delta, min = UINT_MAX;
	int src;

	if(host->quirks & SDHCI_QUIRK_BROKEN_CLOCK_DIVIDER)
		return clk_round_rate(ourhost->clk_bus[ourhost->cur_clk],
			400000);

	for (src = 0; src < MAX_BUS_CLK; src++) {
		delta = sdhci_s3c_consider_clock(ourhost, src, 0);
		if (delta == UINT_MAX)
			continue;
		/* delta is a negative value in this case */
		if (-delta < min)
			min = -delta;
	}
	return min;
}

/**
 * sdhci_s3c_get_ro - callback for get_ro
 * @host: The SDHCI host being changed
 *
 * If the WP pin is connected with GPIO, can get the value which indicates
 * the card is locked or not.
*/
static int sdhci_s3c_get_ro(struct mmc_host *mmc)
{
	struct sdhci_s3c *ourhost = to_s3c(mmc_priv(mmc));

	return 0;

	return gpio_get_value(ourhost->pdata->wp_gpio);
}

/**
 * sdhci_s3c_cfg_wp - configure GPIO for WP pin
 * @gpio_num: GPIO number which connected with WP line from SD/MMC slot
 *
 * Configure GPIO for using WP line
*/
static void sdhci_s3c_cfg_wp(unsigned int gpio_num)
{
	s3c_gpio_cfgpin(gpio_num, S3C_GPIO_INPUT);
	s3c_gpio_setpull(gpio_num, S3C_GPIO_PULL_UP);
}

static void sdhci_s3c_set_ios(struct sdhci_host *host,
			      struct mmc_ios *ios)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	struct s3c_sdhci_platdata *pdata = ourhost->pdata;
	int width;
	u8 tmp;

	sdhci_s3c_check_sclk(host);

	if (ios->power_mode != MMC_POWER_OFF) {
		switch (ios->bus_width) {
		case MMC_BUS_WIDTH_8:
			width = 8;
			tmp = readb(host->ioaddr + SDHCI_HOST_CONTROL);
			writeb(tmp | SDHCI_CTRL_8BITBUS,
				host->ioaddr + SDHCI_HOST_CONTROL);
			printk("%s: Enable 8Bit Data bus. ## ", mmc_hostname(host->mmc));
			break;
		case MMC_BUS_WIDTH_4:
			width = 4;
			break;
		case MMC_BUS_WIDTH_1:
			width = 1;
			break;
		default:
			BUG();
		}

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(ourhost->pdev, width);
	}

	if (pdata->cfg_card)
		pdata->cfg_card(ourhost->pdev, host->ioaddr,
				ios, host->mmc->card);

	mdelay(1);
}

static struct sdhci_ops sdhci_s3c_ops = {
	.get_max_clock		= sdhci_s3c_get_max_clk,
	.set_clock		= sdhci_s3c_set_clock,
	.get_min_clock          = sdhci_s3c_get_min_clock,
	.set_ios		= sdhci_s3c_set_ios,
};

void sdhci_s3c_force_presence_change(struct platform_device *pdev)
{
        struct sdhci_host *host = platform_get_drvdata(pdev);
	printk(KERN_DEBUG "%s : Enter",__FUNCTION__);
	mmc_detect_change(host->mmc, msecs_to_jiffies(0));
}
EXPORT_SYMBOL_GPL(sdhci_s3c_force_presence_change);

static void sdhci_s3c_requlator_work(struct work_struct *work)
{
	struct sdhci_s3c *sc =
		container_of(work, struct sdhci_s3c, regul_work.work);
	struct sdhci_host *host = sc->host;

	if (host->vmmc) {
		if (!gpio_get_value(sc->ext_cd_gpio)) {
			regulator_enable(host->vmmc);
		} else {
			regulator_disable(host->vmmc);
		}
	}
}

static void sdhci_s3c_notify_change(struct platform_device *dev, int state)
{
	struct sdhci_host *host;
	struct sdhci_s3c *sc;
	unsigned long flags;

	local_irq_save(flags);
	
	host = platform_get_drvdata(dev);
	
	sc = sdhci_priv(host);
	
	if (host) {
		if (state) {
			dev_dbg(&dev->dev, "card inserted.\n");
			pr_info("%s : card inserted\n", __func__);
			host->flags &= ~SDHCI_DEVICE_DEAD;
			host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
			if (sc->regulator_workq)
				queue_delayed_work(sc->regulator_workq, \
					&sc->regul_work, 0);			
			tasklet_schedule(&host->card_tasklet);
		} else {
			dev_dbg(&dev->dev, "card removed.\n");
			pr_info("%s : card removed\n", __func__);
			host->flags |= SDHCI_DEVICE_DEAD;
			host->quirks &= ~SDHCI_QUIRK_BROKEN_CARD_DETECTION;
			if (sc->regulator_workq)
				queue_delayed_work(sc->regulator_workq, \
					&sc->regul_work, 0);			
			tasklet_schedule(&host->card_tasklet);
		}
	}
	local_irq_restore(flags);
}

static irqreturn_t sdhci_s3c_gpio_card_detect_isr(int irq, void *dev_id)
{
	struct sdhci_s3c *sc = dev_id;
	int status = gpio_get_value(sc->ext_cd_gpio);

	if (sc->pdata->ext_cd_gpio_invert)
		status = !status;

	sdhci_s3c_notify_change(sc->pdev, status);

	return IRQ_HANDLED;
}

extern struct class *sec_class;
static struct device *tflash_detection_cmd_dev;

static ssize_t tflash_detection_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sdhci_s3c *sc = dev_get_drvdata(dev);
	unsigned int detect;
	if (sc && sc->ext_cd_gpio)
		detect = gpio_get_value(sc->ext_cd_gpio);
	else {
		pr_info("%s : External  SD detect pin Error\n", __func__);
		return  sprintf(buf, "Error\n");
	}

	if (sc->pdata->ext_cd_gpio_invert) {
		pr_info("%s : Invert External  SD detect pin\n", __func__);
		detect = !detect;
	}

	pr_info("%s : detect = %d.\n", __func__,  detect);
	if (detect) {
		printk(KERN_DEBUG "sdhci: card inserted.\n");
		return sprintf(buf, "Insert\n");
	} else {
		printk(KERN_DEBUG "sdhci: card removed.\n");
		return sprintf(buf, "Remove\n");
	}
}
 
static DEVICE_ATTR(tftest, 0444, tflash_detection_cmd_show, NULL);


static int __devinit sdhci_s3c_probe(struct platform_device *pdev)
{
	struct s3c_sdhci_platdata *pdata = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	struct sdhci_host *host;
	struct sdhci_s3c *sc;
	struct resource *res;
	int ret, irq, ptr, clks;

	if (!pdata) {
		dev_err(dev, "no device data specified\n");
		return -ENOENT;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "no irq specified\n");
		return irq;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "no memory specified\n");
		return -ENOENT;
	}

	host = sdhci_alloc_host(dev, sizeof(struct sdhci_s3c));
	if (IS_ERR(host)) {
		dev_err(dev, "sdhci_alloc_host() failed\n");
		return PTR_ERR(host);
	}

	sc = sdhci_priv(host);

	sc->host = host;
	sc->pdev = pdev;
	sc->pdata = pdata;
	sc->ext_cd_gpio = -1;

	platform_set_drvdata(pdev, host);

	sc->clk_io = clk_get(dev, "hsmmc");
	if (IS_ERR(sc->clk_io)) {
		dev_err(dev, "failed to get io clock\n");
		ret = PTR_ERR(sc->clk_io);
		goto err_io_clk;
	}

	/* enable the local io clock and keep it running for the moment. */
	clk_enable(sc->clk_io);

	for (clks = 0, ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
		struct clk *clk;
		char *name = pdata->clocks[ptr];

		if (name == NULL)
			continue;

		clk = clk_get(dev, name);
		if (IS_ERR(clk)) {
			dev_err(dev, "failed to get clock %s\n", name);
			continue;
		}

		clks++;
		sc->clk_bus[ptr] = clk;
		clk_enable(clk);
		sc->cur_clk = ptr;

		dev_info(dev, "clock source %d: %s (%ld Hz)\n",
			 ptr, name, clk_get_rate(clk));
	}

	if (clks == 0) {
		dev_err(dev, "failed to find any bus clocks\n");
		ret = -ENOENT;
		goto err_no_busclks;
	}

	sc->ioarea = request_mem_region(res->start, resource_size(res),
					mmc_hostname(host->mmc));
	if (!sc->ioarea) {
		dev_err(dev, "failed to reserve register area\n");
		ret = -ENXIO;
		goto err_req_regs;
	}

	host->ioaddr = ioremap_nocache(res->start, resource_size(res));
	if (!host->ioaddr) {
		dev_err(dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_req_regs;
	}

	/* Ensure we have minimal gpio selected CMD/CLK/Detect */
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev, pdata->max_width);

	host->hw_name = "samsung-hsmmc";
	host->ops = &sdhci_s3c_ops;
	host->quirks = 0;
	host->irq = irq;

	/* Setup quirks for the controller */
	host->quirks |= SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC;

#ifndef CONFIG_MMC_SDHCI_S3C_DMA

	/* we currently see overruns on errors, so disable the SDMA
	 * support as well. */
	host->quirks |= SDHCI_QUIRK_BROKEN_DMA;

#endif /* CONFIG_MMC_SDHCI_S3C_DMA */

	/* It seems we do not get an DATA transfer complete on non-busy
	 * transfers, not sure if this is a problem with this specific
	 * SDHCI block, or a missing configuration that needs to be set. */
	host->quirks |= SDHCI_QUIRK_NO_BUSY_IRQ;

	if (pdata->cd_type == S3C_SDHCI_CD_NONE)
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;

	host->quirks |= (SDHCI_QUIRK_32BIT_DMA_ADDR |
			 SDHCI_QUIRK_32BIT_DMA_SIZE);

	/* HSMMC on Samsung SoCs uses SDCLK as timeout clock */
	host->quirks |= SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK;

	/* IF SD controller's WP pin donsn't connected with SD card and there is an
	 * allocated GPIO for getting WP data form SD card, use this quirk and send
	 * the GPIO number in pdata->wp_gpio. */
	if (pdata->has_wp_gpio && gpio_is_valid(pdata->wp_gpio)) {
		sdhci_s3c_ops.get_ro = sdhci_s3c_get_ro;
		host->quirks |= SDHCI_QUIRK_NO_WP_BIT;
		sdhci_s3c_cfg_wp(pdata->wp_gpio);
	}

	host->quirks |= SDHCI_QUIRK_NO_HISPD_BIT;

#ifdef CONFIG_ARCH_S5PV310
	host->quirks |= SDHCI_QUIRK_NONSTANDARD_CLOCK;
	host->quirks |= SDHCI_QUIRK_BROKEN_CLOCK_DIVIDER;
#endif

	if(pdata->host_caps)
		host->mmc->caps |= pdata->host_caps;

/* Fill External GPIO Value */
	if (pdata->cd_type == S3C_SDHCI_CD_PERMANENT) {
		pr_info("%s : S3C_SDHCI_CD_PERMANENT\n", __func__);
		if(gpio_is_valid(pdata->ext_cd_gpio)) {
			pr_info("%s : MMC GPIO Level is %d\n", __func__
				, gpio_get_value(pdata->ext_cd_gpio));
			gpio_request(pdata->ext_cd_gpio, "SDHCI EXT CD");
			sc->ext_cd_gpio = gpio_get_value(pdata->ext_cd_gpio);

		}					
		}					

	if (pdata->cd_type == S3C_SDHCI_CD_GPIO &&
		gpio_is_valid(pdata->ext_cd_gpio)) {
		pr_info("%s : S3C_SDHCI_CD_GPIO\n", __func__);
		gpio_request(pdata->ext_cd_gpio, "SDHCI EXT CD");
		sc->ext_cd_gpio = pdata->ext_cd_gpio;

		sc->regulator_workq = create_singlethread_workqueue("ktflash_requlatord");
		if (!sc->regulator_workq) {
			pr_info("%s : ERROR: workqueue for Tflash's regulator.\n"
				"Regulator for Tflash will be always ON\n",
				__func__);
		}
		INIT_DELAYED_WORK(&sc->regul_work, sdhci_s3c_requlator_work);
		
		sc->ext_cd_irq = gpio_to_irq(pdata->ext_cd_gpio);
		
		if (sc->ext_cd_irq &&
			request_irq(sc->ext_cd_irq, sdhci_s3c_gpio_card_detect_isr,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				dev_name(&pdev->dev), sc)) {
			dev_err(&pdev->dev, "cannot request irq for card detect\n");
			sc->ext_cd_irq = 0;
		}

		if (tflash_detection_cmd_dev == NULL && sc->ext_cd_gpio) {
			
			tflash_detection_cmd_dev = device_create(sec_class, NULL, 0, NULL, "tctest");
			if (IS_ERR(tflash_detection_cmd_dev))
				pr_err("%s : Failed to create device(ts)!\n", __func__);
	
			if (device_create_file(tflash_detection_cmd_dev, &dev_attr_tftest) < 0)
				pr_err("%s : Failed to create device file(%s)!\n", __func__, dev_attr_tftest.attr.name);
			
			dev_set_drvdata(tflash_detection_cmd_dev, sc);
		}

		/* T-Flash EINT for CD SHOULD be wakeup source */
		set_irq_wake(sc->ext_cd_irq, 1);
	}
	
	/* Call sdhci_host */
	ret = sdhci_add_host(host);
	if (ret) {
		dev_err(dev, "sdhci_add_host() failed\n");
		goto err_add_host;
	}
	if (pdata->cd_type == S3C_SDHCI_CD_PERMANENT) {
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
		host->mmc->caps |= MMC_CAP_NONREMOVABLE;
	}

	/* pdata->ext_cd_init might call sdhci_s3c_notify_change immediately,
	   so it can be called only after sdhci_add_host() */
	if (pdata->cd_type == S3C_SDHCI_CD_EXTERNAL && pdata->ext_cd_init) {
		pr_info("%s : S3C_SDHCI_CD_EXTERNAL and external init\n", __func__);
		pdata->ext_cd_init(&sdhci_s3c_notify_change);
	}

	if (pdata->cd_type == S3C_SDHCI_CD_GPIO &&
		gpio_is_valid(pdata->ext_cd_gpio)) {
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
	}

	return 0;

 err_add_host:
	release_resource(sc->ioarea);
	kfree(sc->ioarea);

 err_req_regs:
	for (ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
		clk_disable(sc->clk_bus[ptr]);
		clk_put(sc->clk_bus[ptr]);
	}

 err_no_busclks:
	clk_disable(sc->clk_io);
	clk_put(sc->clk_io);

 err_io_clk:
	sdhci_free_host(host);

	return ret;
}

static int __devexit sdhci_s3c_remove(struct platform_device *pdev)
{
	struct sdhci_host *host =  platform_get_drvdata(pdev);
	struct sdhci_s3c *sc = sdhci_priv(host);
	int ptr;

	sdhci_remove_host(host, 1);

	for (ptr = 0; ptr < 3; ptr++) {
		clk_disable(sc->clk_bus[ptr]);
		clk_put(sc->clk_bus[ptr]);
	}
	clk_disable(sc->clk_io);
	clk_put(sc->clk_io);

	iounmap(host->ioaddr);
	release_resource(sc->ioarea);
	kfree(sc->ioarea);

	sdhci_free_host(host);
	platform_set_drvdata(pdev, NULL);

	destroy_workqueue(sc->regulator_workq);

	return 0;
}

#ifdef CONFIG_PM

static int sdhci_s3c_suspend(struct platform_device *dev, pm_message_t pm)
{
	struct sdhci_host *host = platform_get_drvdata(dev);

	sdhci_suspend_host(host, pm);
	
	return 0;
}

static int sdhci_s3c_resume(struct platform_device *dev)
{
	struct sdhci_host *host = platform_get_drvdata(dev);
	sdhci_resume_host(host);
	
#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
      	struct s3c_sdhci_platdata *pdata = dev->dev.platform_data;
	u32 ier;
	if (pdata->enable_intr_on_resume) {
                ier = sdhci_readl(host, SDHCI_INT_ENABLE);
                ier |= SDHCI_INT_CARD_INT;
                sdhci_writel(host, ier, SDHCI_INT_ENABLE);
                sdhci_writel(host, ier, SDHCI_SIGNAL_ENABLE);
        } 
#endif

	return 0;
}

#else
static int sdhci_s3c_suspend(struct platform_device *dev, pm_message_t pm)
{
	return 0;
}

static int sdhci_s3c_resume(struct platform_device *dev)
{
	return 0;
}
#endif

static struct platform_driver sdhci_s3c_driver = {
	.probe		= sdhci_s3c_probe,
	.remove		= __devexit_p(sdhci_s3c_remove),
	.suspend	= sdhci_s3c_suspend,
	.resume	        = sdhci_s3c_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-sdhci",
	},
};

static int __init sdhci_s3c_init(void)
{
	return platform_driver_register(&sdhci_s3c_driver);
}

static void __exit sdhci_s3c_exit(void)
{
	platform_driver_unregister(&sdhci_s3c_driver);
}

module_init(sdhci_s3c_init);
module_exit(sdhci_s3c_exit);

MODULE_DESCRIPTION("Samsung SDHCI (HSMMC) glue");
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:s3c-sdhci");
