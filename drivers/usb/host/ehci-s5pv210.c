/* ehci-s5pv210.c - Driver for USB HOST on Samsung S5PV210 processor
 *
 * Bus Glue for SAMSUNG S5PV210 USB HOST EHCI Controller
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * Author: Jingoo Han <jg1.han@samsung.com>
 *
 * Based on "ehci-au1xxx.c" by by K.Boge <karsten.boge@amd.com>
 * Modified for SAMSUNG s5pv210 EHCI by Jingoo Han <jg1.han@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>

#define CP_PORT		 2  /* HSIC0 in S5PC210 */
#define RETRY_CNT_LIMIT 30  /* Max 300ms wait for cp resume*/

#ifdef CONFIG_MACH_C1_NA_SPR_REV02
int s5pv210_onoff_flag=0;
#endif

struct usb_hcd *s5pv210_hcd;	/* For ehci on/off sysfs */

extern int usb_disabled(void);

extern void usb_host_phy_init(void);
extern void usb_host_phy_off(void);
extern void usb_host_phy_suspend(void);
extern int usb_host_phy_resume(void);
extern void usb_host_phy_sleep(void);
extern void usb_host_phy_wakeup(void);

#ifdef CONFIG_SAMSUNG_PHONE_SVNET
extern int mc_control_active_state(int val);
#endif

static void s5pv210_start_ehc(void);
static void s5pv210_stop_ehc(void);
static int ehci_hcd_s5pv210_drv_probe(struct platform_device *pdev);
static int ehci_hcd_s5pv210_drv_remove(struct platform_device *pdev);

static struct regulator *usb_reg_a, *usb_reg_d;

void usb_host_phy_power_init(struct platform_device *pdev)
{
	int retval;

	if (!usb_reg_a) {
		usb_reg_a = regulator_get(&pdev->dev, "vusb_1.1v");
		if (IS_ERR(usb_reg_a)) {
			retval = PTR_ERR(usb_reg_a);
			dev_err(&pdev->dev, "No VDD_USB_1.1V regualtor: %d\n",
				retval);
			usb_reg_a = NULL;
		}
	}

	if (!usb_reg_d) {
		usb_reg_d = regulator_get(NULL, "vusb_3.3v");
		if (IS_ERR(usb_reg_d)) {
			retval = PTR_ERR(usb_reg_d);
			dev_err(&pdev->dev, "No VDD_USB_3.3V regualtor: %d\n",
				retval);
			usb_reg_d = NULL;
		}
	}

	if (usb_reg_a)
		regulator_enable(usb_reg_a);
	if (usb_reg_d)
		regulator_enable(usb_reg_d);

	printk(KERN_DEBUG "%s: ldo on\n", __func__);
	retval = regulator_is_enabled(usb_reg_d);
	printk(KERN_DEBUG "ehci check ldo vcc_d(%d)\n", retval);
	retval = regulator_is_enabled(usb_reg_a);
	printk(KERN_DEBUG "ehci check ldo vcc_a(%d)\n", retval);
}


void usb_host_phy_power_off(void)
{
	int retval;

	if (usb_reg_d)
		regulator_disable(usb_reg_d);

	if (usb_reg_a)
		regulator_disable(usb_reg_a);

	printk(KERN_DEBUG "%s: ldo off\n", __func__);
	retval = regulator_is_enabled(usb_reg_d);
	printk(KERN_DEBUG "ehci check ldo vcc_d(%d)\n", retval);
	retval = regulator_is_enabled(usb_reg_a);
	printk(KERN_DEBUG "ehci check ldo vcc_a(%d)\n", retval);
}

#ifdef CONFIG_PM
static int s5pv210_ehci_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc = 0;

	if (s5pv210_hcd == NULL) {
		dev_info(dev, "Nothing to do for the device (power off)\n");
		usb_host_phy_power_off();
		return 0;
	}

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */

	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED && hcd->state != HC_STATE_HALT) {
		spin_unlock_irqrestore(&ehci->lock, flags);
		return -EINVAL;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	/* make sure snapshot being resumed re-enumerates everything */
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	spin_unlock_irqrestore(&ehci->lock, flags);
	s5pv210_stop_ehc();
	usb_host_phy_power_off();

	return rc;
}

static int s5pv210_ehci_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	usb_host_phy_power_init(pdev);
	if (s5pv210_hcd == NULL) {
		dev_info(dev, "Nothing to do for the device (power off)\n");
		return 0;
	}
	ehci_info(ehci, "resume\n");
	pm_runtime_resume(&pdev->dev);
	s5pv210_start_ehc();

#if defined(CONFIG_ARCH_S5PV310)
	writel(0x03C00000, hcd->regs + 0x90);
#endif

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_info(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;


	return 0;
}
#ifdef CONFIG_USB_SUSPEND
static int ehci_hcd_s5pv210_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "phy suspend\n");
	usb_host_phy_suspend();
	return 0;
}

static int ehci_hcd_s5pv210_runtime_resume(struct device *dev)
{
	int rc = 0;
	if (dev->power.status != DPM_RESUMING)
		rc = usb_host_phy_resume();
	if (rc) {
		struct usb_hcd *hcd = s5pv210_hcd;
		struct ehci_hcd *ehci = hcd_to_ehci(hcd);
		/* emptying the schedule aborts any urbs */
		spin_lock_irq(&ehci->lock);
		if (ehci->reclaim)
			end_unlink_async(ehci);
		ehci_work(ehci);
		spin_unlock_irq(&ehci->lock);

#if defined(CONFIG_ARCH_S5PV310)
		writel(0x03C00000, hcd->regs + 0x90);
#endif
		ehci_writel(ehci, ehci->command, &ehci->regs->command);
		ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
		ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

		/* here we "know" root ports should always stay powered */
		ehci_port_power(ehci, 1);

		hcd->state = HC_STATE_SUSPENDED;
		usb_root_hub_lost_power(hcd->self.root_hub);
	}
	dev_dbg(dev, "phy resume, p.s=%d, rc=%d\n", dev->power.status, rc);
	return 0;
}
#endif

static const struct dev_pm_ops s5pv210_ehci_dev_pm_ops = {
	.suspend = s5pv210_ehci_suspend,
	.resume = s5pv210_ehci_resume,
#ifdef CONFIG_USB_SUSPEND
	.runtime_suspend = ehci_hcd_s5pv210_runtime_suspend,
	.runtime_resume = ehci_hcd_s5pv210_runtime_resume,
#endif
};

#define S5PV210_DEV_PM_OPS (&s5pv210_ehci_dev_pm_ops)
#else
#define S5PV210_DEV_PM_OPS NULL
#endif

static void s5pv210_wait_for_cp_resume(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 __iomem	*portsc ;
	u32 val32, retry_cnt = 0;

	portsc = &ehci->regs->port_status[CP_PORT-1];
#ifdef CONFIG_SAMSUNG_PHONE_SVNET
	mc_control_active_state(1); /* CP USB Power On */
#endif
	do {
		msleep(10);
		val32 = ehci_readl(ehci, portsc);
	} while (++retry_cnt < RETRY_CNT_LIMIT && !(val32 & PORT_CONNECT));
	printk(KERN_DEBUG "%s: retry_cnt = %d\n", __func__, retry_cnt);
}

static void s5pv210_start_ehc(void)
{
	usb_host_phy_init();
        #ifdef CONFIG_SAMSUNG_PHONE_SVNET
	/*mc_control_active_state(1);*/
        #endif
}

static void s5pv210_stop_ehc(void)
{
	usb_host_phy_off();

        #ifdef CONFIG_SAMSUNG_PHONE_SVNET
	mc_control_active_state(0);
        #endif
}

static int ehci_hcd_update_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int rc = 0;

	/* No working yet -jaewang- */
	return rc;

	if (!udev->parent) /* udev is root hub itself, impossible */
		rc = -1;
	/* we only support lpm device connected to root hub yet */
	if (ehci->has_lpm && !udev->parent->parent) {
		rc = ehci_lpm_set_da(ehci, udev->devnum, udev->portnum);
		if (!rc)
			rc = ehci_lpm_check(ehci, udev->portnum);
	}
	return rc;
}

static const struct hc_driver ehci_s5pv210_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "s5pv210 EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	.irq			= ehci_irq,
	.flags			= HCD_MEMORY|HCD_USB2,

	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	.get_frame_number	= ehci_get_frame,

	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	/* call back when device connected and addressed */
	.update_device		= ehci_hcd_update_device,
	.wait_for_device	= s5pv210_wait_for_cp_resume,
	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static ssize_t show_ehci_power(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "EHCI Power %s\n", (s5pv210_hcd) ? "on" : "off");
}

static ssize_t store_ehci_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct usb_hcd *hcd = bus_to_hcd(dev_get_drvdata(dev));
	int power_on;
	int retval;

	if (sscanf(buf, "%d", &power_on) != 1)
		return -EINVAL;

	device_lock(dev);
	if (power_on == 0 && s5pv210_hcd != NULL) {
		printk(KERN_DEBUG "%s: EHCI turns off\n", __func__);
		usb_remove_hcd(hcd);
		s5pv210_stop_ehc();
		s5pv210_hcd = NULL;
		/*HSIC IPC control the ACTIVE_STATE*/
                #ifdef CONFIG_SAMSUNG_PHONE_SVNET
		mc_control_active_state(0);
                #endif
	} else if (power_on == 1) {
		printk(KERN_DEBUG "%s: EHCI turns on\n", __func__);
		if (s5pv210_hcd != NULL) {
			usb_remove_hcd(hcd);
			/*HSIC IPC control the ACTIVE_STATE*/
                        #ifdef CONFIG_SAMSUNG_PHONE_SVNET
			mc_control_active_state(0);
                        #endif
		}
		s5pv210_start_ehc();

#if defined(CONFIG_ARCH_S5PV310)
		writel(0x03C00000, hcd->regs + 0x90);
#endif

		retval = usb_add_hcd(hcd, IRQ_UHOST,
				IRQF_DISABLED | IRQF_SHARED);
		if (retval < 0) {
			dev_err(dev, "Power On Fail\n");
			goto exit;
		}
		/*HSIC IPC control the ACTIVE_STATE*/
                #ifdef CONFIG_SAMSUNG_PHONE_SVNET
		mc_control_active_state(1);
                #endif

		s5pv210_hcd = hcd;
	}
exit:
	device_unlock(dev);
	return count;
}
static DEVICE_ATTR(ehci_power, 0664, show_ehci_power, store_ehci_power);

#ifdef CONFIG_PM_RUNTIME
static const char ctrl_auto[] = "auto";
static const char ctrl_on[] = "on";

static ssize_t ehci_runtime_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",
				dev->power.runtime_auto ? ctrl_auto : ctrl_on);
}

static ssize_t ehci_runtime_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	char *cp;
	int len = n;

	cp = memchr(buf, '\n', n);
	if (cp)
		len = cp - buf;
	if (len == sizeof ctrl_auto - 1 && strncmp(buf, ctrl_auto, len) == 0)
		pm_runtime_allow(dev);
	else if (len == sizeof ctrl_on - 1 && strncmp(buf, ctrl_on, len) == 0)
		pm_runtime_forbid(dev);
	else
		return -EINVAL;
	return n;
}
static DEVICE_ATTR(ehci_runtime, 0664, ehci_runtime_show, ehci_runtime_store);
#endif

static inline int create_ehci_sys_file(struct ehci_hcd *ehci)
{
	device_create_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_ehci_power);
	return device_create_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_ehci_runtime);
}

static inline void remove_ehci_sys_file(struct ehci_hcd *ehci)
{
	device_remove_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_ehci_runtime);
	device_remove_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_ehci_power);
}

#ifdef CONFIG_MACH_C1_NA_SPR_REV02
void usb_host_hsic_power_init()
{
        int retval;
        struct regulator *usb_hsic_reg;

         usb_hsic_reg = regulator_get(NULL, "vhsic");
         if (IS_ERR(usb_hsic_reg)) {
                        retval = PTR_ERR(usb_hsic_reg);
                        //dev_err(&pdev->dev, "No VHSIC_1.2V regualtor: %d\n",
                          //      retval);
                        usb_hsic_reg = NULL;
                }

        if (usb_hsic_reg)
                retval=regulator_enable(usb_hsic_reg);

		if (s5pv210_onoff_flag == 0) {
			s5pv210_start_ehc();
			
            #if defined(CONFIG_ARCH_S5PV310)
			writel(0x03C00000, s5pv210_hcd->regs + 0x90);
            #endif
			
			retval = usb_add_hcd(s5pv210_hcd, IRQ_UHOST,
					IRQF_DISABLED | IRQF_SHARED);
			if (retval < 0) {
				//dev_err(dev, "Power On Fail\n");
				//goto exit;
			}
			s5pv210_onoff_flag = 1;
		}		

}
EXPORT_SYMBOL(usb_host_hsic_power_init);
void usb_host_hsic_power_off()
{
        int retval;
        struct regulator *usb_hsic_reg;

		 if (s5pv210_onoff_flag == 1) {
			 usb_remove_hcd(s5pv210_hcd);
			 s5pv210_stop_ehc();
			 s5pv210_onoff_flag = 0;
		 }

         usb_hsic_reg = regulator_get(NULL, "vhsic");
         if (IS_ERR(usb_hsic_reg)) {
                        retval = PTR_ERR(usb_hsic_reg);
                        //dev_err(, "No VHSIC_1.2V regualtor: %d\n",
                          //      retval);
                        usb_hsic_reg = NULL;
                }

        if (usb_hsic_reg)
                retval=regulator_disable(usb_hsic_reg);

}
EXPORT_SYMBOL(usb_host_hsic_power_off);
#endif

static int ehci_hcd_s5pv210_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd  *hcd = NULL;
	struct ehci_hcd *ehci = NULL;
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;


	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		dev_err(&pdev->dev, "resource[1] is not IORESOURCE_IRQ.\n");
		return -ENODEV;
	}

	hcd = usb_create_hcd(&ehci_s5pv210_hc_driver, &pdev->dev, "s5pv210");
	if (!hcd) {
		dev_err(&pdev->dev, "usb_create_hcd failed!\n");
		return -ENODEV;
	}
	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		dev_err(&pdev->dev, "request_mem_region failed!\n");
		retval = -EBUSY;
		goto err1;
	}

	usb_host_phy_power_init(pdev);
	s5pv210_start_ehc();

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed!\n");
		retval = -ENOMEM;
		goto err2;
	}

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

#if defined(CONFIG_ARCH_S5PV210) || defined(CONFIG_ARCH_S5P6450)
	writel(0x000F0000, hcd->regs + 0x90);
#endif

#if defined(CONFIG_ARCH_S5PV310)
	writel(0x03C00000, hcd->regs + 0x90);
#endif

	create_ehci_sys_file(ehci);

	retval = usb_add_hcd(hcd, pdev->resource[1].start,
				IRQF_DISABLED | IRQF_SHARED);

	if (retval == 0) {
		platform_set_drvdata(pdev, hcd);
		s5pv210_hcd = hcd;

#ifdef CONFIG_USB_SUSPEND
		pm_runtime_set_active(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
		pm_runtime_forbid(&pdev->dev);
#endif

		/*HSIC IPC control the ACTIVE_STATE*/
                #ifdef CONFIG_SAMSUNG_PHONE_SVNET
		mc_control_active_state(1);
                #endif
		return retval;
	}

	remove_ehci_sys_file(ehci);
	s5pv210_stop_ehc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return retval;
}

static int ehci_hcd_s5pv210_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	s5pv210_hcd = NULL;
	remove_ehci_sys_file(hcd_to_ehci(hcd));
#ifdef CONFIG_USB_SUSPEND
	pm_runtime_disable(&pdev->dev);
#endif
	usb_remove_hcd(hcd);
	/*HSIC IPC control the ACTIVE_STATE*/
        #ifdef CONFIG_SAMSUNG_PHONE_SVNET
	mc_control_active_state(0);
        #endif
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	s5pv210_stop_ehc();
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver  ehci_hcd_s5pv210_driver = {
	.probe		= ehci_hcd_s5pv210_drv_probe,
	.remove		= ehci_hcd_s5pv210_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver = {
		.name = "s5p-ehci",
		.owner = THIS_MODULE,
		.pm     = S5PV210_DEV_PM_OPS,
	}
};

MODULE_ALIAS("platform:s5p-ehci");
