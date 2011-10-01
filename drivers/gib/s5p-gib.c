/* s3c-gib.c
 *
 * Copyright (C) 2010 Samsung Electronics Co. Ltd.
 *
 * S3C GPS Interface Block 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-rtc.h>

#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gib.h>


#include "gib-dev.h"
#include "s3c-gib.h"

#undef debug
#ifdef debug
#define DBG(x...)       printk(x)
#define GIB_DBG		printk("%s :: %d\n",__FUNCTION__,__LINE__)
#else
#define GIB_DBG
#define DBG(x...)       do { } while (0)
#endif

struct bit_data {
	unsigned int	uart_rxd;
	unsigned int	uart_txd;

	unsigned int	gps_bb_mclk;
	unsigned int	gps_bb_sync;
	unsigned int	gps_bb_epoch;
	unsigned int	gps_slpn;
	unsigned int	gps_rstout;
	unsigned int	bt_rstout;
	unsigned int	rf_reset;
	unsigned int	clkreq;

	unsigned int	bb_scl;
	unsigned int	bb_sda;

	unsigned int	gps_qsign;
	unsigned int	gps_qmag;
	unsigned int	gps_isign;
	unsigned int	gps_imag;

	unsigned int	gps_gpio0;
	unsigned int	gps_gpio1;
	unsigned int	gps_gpio2;
	unsigned int	gps_gpio3;
	unsigned int	gps_gpio4;
	unsigned int	gps_gpio5;
	unsigned int	gps_gpio6;
	unsigned int	gps_gpio7;

	unsigned int	gpa4;
	unsigned int	gpa5;
};

static struct bit_data bit_data;
struct bit_data *g_gpio = &bit_data;


static void s3c_gibgpio_init(struct bit_data *gpio)
{
	GIB_DBG;

	gpio->gpa4= S5PV310_GPA1(4);
	gpio->gpa5= S5PV310_GPA1(5);

	s3c_gpio_cfgpin(gpio->gpa4, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gpa5, S3C_GPIO_SFN(2));

	gpio->gps_bb_sync= S5PV310_GPL0(0);
	gpio->gps_isign= S5PV310_GPL0(1);
	gpio->gps_imag= S5PV310_GPL0(2);
	gpio->gps_qsign = S5PV310_GPL0(3);
	gpio->gps_qmag= S5PV310_GPL0(4);
	gpio->gps_bb_mclk= S5PV310_GPL0(5);
	gpio->rf_reset= S5PV310_GPL0(6);
	gpio->clkreq= S5PV310_GPL0(7);
	gpio->bb_scl= S5PV310_GPL1(0);
	gpio->bb_sda= S5PV310_GPL1(1);
	gpio->gps_bb_epoch= S5PV310_GPL1(2);




	gpio->gps_gpio0 = S5PV310_GPL2(0);
	gpio->gps_gpio1 = S5PV310_GPL2(1);
	gpio->gps_gpio2 = S5PV310_GPL2(2);
	gpio->gps_gpio3 = S5PV310_GPL2(3);
	gpio->gps_gpio4 = S5PV310_GPL2(4);
	gpio->gps_gpio5 = S5PV310_GPL2(5);
	gpio->gps_gpio6 = S5PV310_GPL2(6);
	gpio->gps_gpio7 = S5PV310_GPL2(7);

	s3c_gpio_cfgpin(gpio->gps_bb_sync, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_isign, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_imag, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_qsign, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_qmag, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_bb_mclk, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->rf_reset, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->clkreq, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->bb_scl, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->bb_sda, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_bb_epoch, S3C_GPIO_SFN(2));

	s3c_gpio_cfgpin(gpio->gps_gpio0, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_gpio1, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_gpio2, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->gps_gpio3, S3C_GPIO_SFN(2));
	
	s3c_gpio_setpull(gpio->gps_bb_sync, 0x1);
	s3c_gpio_setpull(gpio->gps_isign, 0x1);
	s3c_gpio_setpull(gpio->gps_imag, 0x1);
	s3c_gpio_setpull(gpio->gps_qsign, 0x1);
	s3c_gpio_setpull(gpio->gps_qmag, 0x1);
	s3c_gpio_setpull(gpio->gps_bb_mclk, 0x1);
	s3c_gpio_setpull(gpio->rf_reset, 0x1);
	s3c_gpio_setpull(gpio->clkreq, 0x1);
	s3c_gpio_setpull(gpio->bb_scl, 0x1);	
	s3c_gpio_setpull(gpio->bb_sda, 0x1);
	s3c_gpio_setpull(gpio->gps_bb_epoch, 0x1);

	
}

/*Set MUX
0: Connect debug, 1: Connect BB
*/
static void s3c_gib_ubp_debug( unsigned int flag)
{
	unsigned int  tmp;
	GIB_DBG;

	tmp = __raw_readl(S5PV310_GPS_CON);
	if (flag) {
		tmp &= ~(GPS_MUX_SEL);
		DBG("Changed to UBP Debug Mode\n");
	}else {
		tmp |= (GPS_MUX_SEL);
		DBG("Connected: BB_UART1-->AP_UART4\n");
	}
	__raw_writel(tmp, S5PV310_GPS_CON);
}


/*GPS RF + BB SW Reset
*/
static void s3c_gib_core_reset(unsigned int flag)
{
	unsigned int  tmp;
	GIB_DBG;

	tmp = __raw_readl(S5PV310_GPS_CON);

	if(flag == 0) {
		tmp |= (GPS_SRST);
		DBG("low\n");
	} else if (flag == 1) {
		tmp &= ~(GPS_SRST);
		DBG("high\n");
	}
	__raw_writel(tmp, S5PV310_GPS_CON);
}

#if 0
static int s3c_gib_close(struct gib_dev *gib_dev)
{
	GIB_DBG;

	return 0;
}
#endif

static struct gib_reset s3c_gib_reset = {
	.core_reset		= s3c_gib_core_reset,
};

static struct gib_udp_debug s3c_gib_udp_debug = {
	.ubp_debug           = s3c_gib_ubp_debug,
};

static struct s3c_gib s3c_gib[2] = {
	[0] = {
		.gibdev	= {
			.g_reset			= &s3c_gib_reset,
			.g_udp_debug		= &s3c_gib_udp_debug,

		}
	},
	[1] = {
		.gibdev	= {
			.g_reset			= &s3c_gib_reset,
			.g_udp_debug		= &s3c_gib_udp_debug,
		}
	},
};


/* s3c_gib_probe
 *
 * called by the bus driver when a suitable device is found
*/

static int s3c_gib_probe(struct platform_device *pdev)
{
	struct s3c_gib *gib = &s3c_gib[pdev->id];
	struct resource *res;
	int ret;

	GIB_DBG;

	gib->nr = pdev->id;
	gib->dev = &pdev->dev;

	/* map the registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res == NULL) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto out;
	}


	gib->regs = ioremap(res->start, (res->end - res->start) + 1);

	if (gib->regs == NULL) {
		dev_err(&pdev->dev, "cannot map IO\n");
		ret = -ENXIO;
		goto out;
	}


	/* setup info block for the gib core */

	gib->gibdev.dev.parent = &pdev->dev;
	gib->gibdev.minor = gib->nr;

	ret = gib_attach_gibdev(&gib->gibdev);

	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add adapter to gib core\n");
		goto out;
	}

	dev_set_drvdata(&pdev->dev, gib);

	s3c_gibgpio_init(g_gpio);

	s3c_gib_ubp_debug(0);

out:

	return ret;
}

/* s3c_gib_remove
 *
 * called when device is removed from the bus
*/
static int s3c_gib_remove(struct platform_device *pdev)
{
	struct s3c_gib *gib = dev_get_drvdata(&pdev->dev);

	GIB_DBG;
	
	if (gib != NULL) {
		gib_detach_gibdev(&gib->gibdev);
		dev_set_drvdata(&pdev->dev, NULL);
	}

	return 0;
}

#ifdef CONFIG_PM
static int s3c_gib_suspend(struct platform_device *pdev, pm_message_t msg)
{
	unsigned int tmp, val, ctr;

	s5p_gpio_set_conpdn(g_gpio->rf_reset,1);
	
	tmp = __raw_readl(S5PV310_GPS_GPSIO);
        val = tmp & GPS_OUT10;
       if (val) {
		DBG("\nGPS BB is now in sleep state. \n");
	 }else {
	 	printk(KERN_WARNING "\n[WARNING] GPS BB has not been in sleep state. Please wait 3 seconds. \n");
                ctr = 0;
	 	do {
			tmp = __raw_readl(S5PV310_GPS_GPSIO);
        		val = tmp & GPS_OUT10;
                        msleep(100);
                        ctr++;
	 	} while ( (!val) & (ctr < 30) );
	 	     
		if (val)     
			DBG("GPS BB is now in sleep state after %d tries. \n", ctr);
		else if (ctr >= 30)     
			printk(KERN_ERR "[ERROR] GPS sleep is failed because GPS BB is not in sleep state. \n");
	 }
	 
	return 0;
}

static int s3c_gib_resume(struct platform_device *pdev)
{
	s3c_gib_ubp_debug(0);
	
	return 0;
}
#else
#define s3c_gib_suspend NULL
#define s3c_gib_resume  NULL
#endif

/* device driver for platform bus bits */
static struct platform_driver s3c_gib_driver = {
	.probe		= s3c_gib_probe,
	.remove		= s3c_gib_remove,
#ifdef CONFIG_PM
	.suspend	= s3c_gib_suspend,
	.resume		= s3c_gib_resume,
#endif
	.driver		= {
		.name	= "s3c-gib",
		.owner	= THIS_MODULE,
		.bus    = &platform_bus_type,
	},
};

static int __init s3c_gib_driver_init(void)
{
	GIB_DBG;

	return platform_driver_register(&s3c_gib_driver);
}

static void __exit s3c_gib_driver_exit(void)
{
	GIB_DBG;
	platform_driver_unregister(&s3c_gib_driver);
}

module_init(s3c_gib_driver_init);
module_exit(s3c_gib_driver_exit);

MODULE_DESCRIPTION("S3C GIB driver");
MODULE_AUTHOR("Taeyong Lee<taeyong00.lee@samsung.com>");
MODULE_LICENSE("GPL");
