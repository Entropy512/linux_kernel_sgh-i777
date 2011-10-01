/*
 * USB CDC Samsung virtual network interface driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#define DEBUG

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/cdc.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/if_phonet.h>
#include <linux/phonet.h>
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/modemctl.h>

#include "sipc4.h"
#include "pdp.h"

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>

#define DEFAULT_RAW_WAKE_TIME (6*HZ)
#define DEFAULT_FMT_WAKE_TIME (HZ/2)
#define SVNET_SUSPEND_UNLOCK_DELAY msecs_to_jiffies(20)
#endif

#define USBSVN_NOT_MAIN		1
#define USBSVN_DEVNUM_MAX	4
#define USBSVN_DEV_ADDR		0xa0

#define CONFIG_SVNET_AP_INITIATED_L2

static const unsigned rxq_size = 1;
static int rx_debug;
static int tx_debug;

static struct usbsvn *share_svn;

struct usbsvn_devdata {
	struct usb_interface	*data_intf;
	struct sk_buff		*rx_skb;
	unsigned int		tx_pipe;
	unsigned int		rx_pipe;
	u8			disconnected;
	struct sipc_rx_hdr 	rx_hdr;
};

struct usbsvn {
	struct net_device	*netdev;
	struct usb_device	*usbdev;
	struct usbsvn_devdata	devdata[USBSVN_DEVNUM_MAX];
	struct pdp_device	pdp;
	struct workqueue_struct *tx_workqueue;
	struct sk_buff_head	tx_skb_queue;
	struct work_struct	tx_work;
	struct work_struct	post_resume_work;
	struct delayed_work	pm_runtime_work;
	struct delayed_work	try_reconnect_work;

	unsigned long		driver_info;
	unsigned int		dev_count;
	unsigned int		suspended;
#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock 	wlock;
	struct wake_lock 	dormancy_lock;
	long 			wake_time;
#endif
	int resume_debug;
	int dpm_suspending;
	int usbsvn_connected;
	int skip_hostwakeup;
	int reconnect_cnt;
	int flow_suspend;
	struct urb		*urbs[0];
};

struct usbsvn_rx {
	struct net_device	*netdev;
	int			dev_id;
};

#ifdef CONFIG_HAS_WAKELOCK
enum {
	SVNET_WLOCK_RUNTIME,
	SVNET_WLOCK_DORMANCY,
} SVNET_WLOCK_TYPE;

static inline void _wake_lock_init(struct usbsvn *sn)
{
	wake_lock_init(&sn->wlock, WAKE_LOCK_SUSPEND, "svnet");
	wake_lock_init(&sn->dormancy_lock, WAKE_LOCK_SUSPEND, "svnet-dormancy");
	sn->wake_time = DEFAULT_RAW_WAKE_TIME;
}

static inline void _wake_lock_destroy(struct usbsvn *sn)
{
	wake_lock_destroy(&sn->wlock);
	wake_lock_destroy(&sn->dormancy_lock);
}

static inline void _wake_lock(struct usbsvn *sn, int type)
{
	if (sn->usbsvn_connected)
		switch (type) {
		case SVNET_WLOCK_DORMANCY:
			wake_lock(&sn->dormancy_lock);
			break;
		case SVNET_WLOCK_RUNTIME:
		default:
			wake_lock(&sn->wlock);
			break;
		}
}

static inline void _wake_unlock(struct usbsvn *sn, int type)
{
	if (sn)
		switch (type) {
		case SVNET_WLOCK_DORMANCY:
			wake_unlock(&sn->dormancy_lock);
			break;
		case SVNET_WLOCK_RUNTIME:
		default:
			wake_unlock(&sn->wlock);
			break;
		}
}

static inline void _wake_lock_timeout(struct usbsvn *sn, int type)
{
	switch (type) {
	case SVNET_WLOCK_DORMANCY:
		wake_lock_timeout(&sn->dormancy_lock, sn->wake_time);
		break;
	case SVNET_WLOCK_RUNTIME:
	default:
		wake_lock_timeout(&sn->wlock, SVNET_SUSPEND_UNLOCK_DELAY);
	}
}

static inline void _wake_lock_settime(struct usbsvn *sn, long time)
{
	if (sn)
		sn->wake_time = time;
}

static inline long _wake_lock_gettime(struct usbsvn *sn)
{
	return sn ? sn->wake_time : DEFAULT_RAW_WAKE_TIME;
}
#else
#define _wake_lock_init(sn) do { } while (0)
#define _wake_lock_destroy(sn) do { } while (0)
#define _wake_lock(sn, type) do { } while (0)
#define _wake_unlock(sn, type) do { } while (0)
#define _wake_lock_timeout(sn, type) do { } while (0)
#define _wake_lock_settime(sn, time) do { } while (0)
#define _wake_lock_gettime(sn) (0)
#endif

#define wake_lock_pm(sn)	_wake_lock(sn, SVNET_WLOCK_RUNTIME)
#define wake_lock_data(sn)	_wake_lock(sn, SVNET_WLOCK_DORMANCY)
#define wake_unlock_pm(sn)	_wake_unlock(sn, SVNET_WLOCK_RUNTIME)
#define wake_unlock_data(sn)	_wake_unlock(sn, SVNET_WLOCK_DORMANCY)
#define wake_lock_timeout_pm(sn) _wake_lock_timeout(sn, SVNET_WLOCK_RUNTIME)
#define wake_lock_timeout_data(sn) _wake_lock_timeout(sn, SVNET_WLOCK_DORMANCY)

#ifdef CONFIG_PM_RUNTIME
int usbsvn_request_resume(void)
{
	struct device *dev;
	int err=0;

	if (!share_svn->usbdev)
		return -EFAULT;

	dev = &share_svn->usbdev->dev;

	if (share_svn->dpm_suspending) {
		share_svn->skip_hostwakeup = 1;
		printk(KERN_DEBUG "%s: suspending skip host wakeup\n",
			__func__);
		return 0;
	}
	usb_mark_last_busy(share_svn->usbdev);
	if (share_svn->resume_debug >= 1) {
		printk(KERN_DEBUG "%s: resumeing, return\n", __func__);
		return 0;
	}

	if (dev->power.status != DPM_OFF) {
		wake_lock_pm(share_svn);
		printk(KERN_DEBUG "%s:run time resume\n", __func__);
		share_svn->resume_debug = 1;
		err = pm_runtime_resume(dev);
		if (!err && dev->power.timer_expires == 0
			&& dev->power.request_pending == false) {
			printk(KERN_DEBUG "%s:run time idle\n", __func__);
			pm_runtime_idle(dev);
		}
		share_svn->resume_debug = 0;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(usbsvn_request_resume);

static void usbsvn_runtime_start(struct work_struct *work)
{
	struct usbsvn *svn =
		container_of(work, struct usbsvn, pm_runtime_work.work);
	struct device *dev, *ppdev;

	dev = &svn->usbdev->dev;
	if (svn->usbdev && dev->parent) {
		ppdev = dev->parent->parent;
		/*enable runtime feature - once after boot*/
		pm_runtime_allow(dev);
		dev_dbg(dev, "usbsvn Runtime PM Start!!\n");
		pm_runtime_allow(ppdev); /*ehci*/
	}
}
#else
struct device *usbsvn_get_runtime_pm_dev(void){return 0; }
int usbsvn_request_resume(void){return 0; }
static int usbsvn_runtime_start(void){ }
#endif

static void usbsvn_try_reconnect_work(struct work_struct *work)
{
	struct usbsvn *svn =
		container_of(work, struct usbsvn, try_reconnect_work.work);
/*	char *envs[2] = {"MAILBOX=hsic_disconnected", NULL };*/

	if (svn->usbsvn_connected) {
		wake_unlock_pm(svn);
		printk(KERN_INFO "svn re-connected\n");
		goto out;
	}
	if (svn->dpm_suspending) {
		wake_lock_pm(svn);
		printk(KERN_INFO "svn suspending, delay reconnection\n");
		goto retry;
	}
	if (!svn->reconnect_cnt--) {
		wake_unlock_pm(svn);
#ifdef CONFIG_SAMSUNG_PHONE_SVNET
		/*if (!mc_is_modem_active()) {
			kobject_uevent_env(&svn->netdev->dev.kobj, KOBJ_OFFLINE,
				envs); */
			/*panic("HSIC Disconnected");*/
			crash_event(0);
		/*}*/
#endif
		goto out;
	}
#ifdef CONFIG_SAMSUNG_PHONE_SVNET
	mc_reconnect_gpio();
#endif

	/*TODO: EHCI off reconnect*/

retry:
	schedule_delayed_work(&svn->try_reconnect_work, 200);
out:
	return;
}

/*check the usbsvn interface driver status after resume*/
static void usbsvn_post_resume_work(struct work_struct *work)
{
	struct usbsvn *svn =
		container_of(work, struct usbsvn, post_resume_work);
	struct device *dev = &svn->usbdev->dev;
	int spin = 10;
	int err;

#ifdef CONFIG_SAMSUNG_PHONE_SVNET
	if (svn->skip_hostwakeup && mc_is_host_wakeup()) {
		dev_info(dev,
			"post resume host skip=%d, host gpio=%d, rpm_stat=%d",
			svn->skip_hostwakeup, mc_is_host_wakeup(),
			dev->power.runtime_status);
retry:
		switch (dev->power.runtime_status) {
		case RPM_SUSPENDED:
			svn->resume_debug = 1;
			err = pm_runtime_resume(dev);
			if (!err && dev->power.timer_expires == 0
				&& dev->power.request_pending == false) {
				printk(KERN_DEBUG "%s:run time idle\n",
					__func__);
				pm_runtime_idle(dev);
			}
			svn->resume_debug = 0;
			break;
		case RPM_SUSPENDING:
			if (spin--) {
				dev_err(dev,
					"usbsvn suspending when resum spin=%d\n"
					, spin);
				msleep(20);
				goto retry;
			}
		case RPM_RESUMING:
		case RPM_ACTIVE:
			break;
		}
		svn->skip_hostwakeup = 0;
	}
#endif
}

static ssize_t usbsvn_rx_debug_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", rx_debug);
}

static ssize_t usbsvn_rx_debug_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = strict_strtoul(buf, 10, &val);
	if (err < 0)
		return err;

	rx_debug = val > 0 ? 1 : 0;

	return count;
}

static ssize_t usbsvn_tx_debug_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tx_debug);
}

static ssize_t usbsvn_tx_debug_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = strict_strtoul(buf, 10, &val);
	if (err < 0)
		return err;

	tx_debug = val > 0 ? 1 : 0;

	return count;
}

static ssize_t usbsvn_connection_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct usbsvn *svn = share_svn;

	if (!svn)
		return sprintf(buf, "0\n");

	return sprintf(buf, "%d\n", svn->usbsvn_connected);
}

static ssize_t show_waketime(struct device *d,
		struct device_attribute *attr, char *buf)
{
	struct usbsvn *svn = share_svn;
	char *p = buf;
	unsigned int msec;
	unsigned long j;

	if (!svn)
		return 0;

	j = _wake_lock_gettime(svn);
	msec = jiffies_to_msecs(j);
	p += sprintf(p, "%u\n", msec);

	return p - buf;
}

static ssize_t store_waketime(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct usbsvn *svn = share_svn;
	unsigned long msec;
	unsigned long j;
	int r;

	if (!svn)
		return count;

	r = strict_strtoul(buf, 10, &msec);
	if (r)
		return count;

	j = msecs_to_jiffies(msec);
	_wake_lock_settime(svn, j);

	return count;
}

static DEVICE_ATTR(rx_debug, 0664, usbsvn_rx_debug_show, usbsvn_rx_debug_store);
static DEVICE_ATTR(tx_debug, 0664, usbsvn_tx_debug_show, usbsvn_tx_debug_store);
static DEVICE_ATTR(connected, S_IRUGO | S_IRUSR, usbsvn_connection_show, NULL);
static DEVICE_ATTR(waketime, 0664, show_waketime, store_waketime);

static struct attribute *usbsvn_attrs[] = {
	&dev_attr_rx_debug.attr,
	&dev_attr_tx_debug.attr,
	&dev_attr_connected.attr,
	&dev_attr_waketime.attr,
	NULL
};

static const struct attribute_group usbsvn_attr_group = {
	.attrs = usbsvn_attrs,
};

static void tx_complete(struct urb *req);
static void rx_complete(struct urb *req);

#ifdef CONFIG_PM_RUNTIME
static int usbsvn_initiated_resume(struct net_device *ndev)
{
	int err;

	if (share_svn->usbdev) {
		struct device *dev = &share_svn->usbdev->dev;
		int spin = 10, spin2 = 30;
		int host_wakeup_done = 0;
		int _host_high_cnt = 0, _host_timeout_cnt = 0;
retry:
		switch (dev->power.runtime_status) {
		case RPM_SUSPENDED:
			if (share_svn->dpm_suspending || host_wakeup_done) {
				dev_dbg(&ndev->dev,
					"DPM Suspending, spin:%d\n", spin2);
				if (spin2-- == 0) {
					dev_err(&ndev->dev,
					"dpm resume timeout\n");
					return -ETIMEDOUT;
				}
				msleep(30);
				goto retry;
			}
#ifdef CONFIG_SAMSUNG_PHONE_SVNET
			err = mc_prepare_resume(200);
			switch (err) {
			case MC_SUCCESS:
				host_wakeup_done = 1;
				_host_timeout_cnt = 0;
				_host_high_cnt = 0;
				goto retry; /*wait until RPM_ACTIVE states*/

			case MC_HOST_TIMEOUT:
				_host_timeout_cnt++;
				break;

			case MC_HOST_HIGH:
				_host_high_cnt++;
				break;
			}
#endif
			if (spin2-- == 0) {
				dev_err(&ndev->dev,
				"svn initiated resume, RPM_SUSPEND timeout\n");
/*				share_svn->resume_debug = 1;
				err = pm_runtime_resume(dev);
				if (!err && dev->power.timer_expires == 0
				&& dev->power.request_pending == false) {
					printk(KERN_DEBUG
						"%s:run time idle\n", __func__);
					pm_runtime_idle(dev);
				}
				share_svn->resume_debug = 0;*/
#ifdef CONFIG_SAMSUNG_PHONE_SVNET
				crash_event(0);
#endif
				return -ETIMEDOUT;
			}
			msleep(20);
			goto retry;

		case RPM_SUSPENDING:
			dev_dbg(&ndev->dev,
				"RPM Suspending, spin:%d\n", spin);
			if (spin-- == 0) {
				dev_err(&ndev->dev,
				"Modem suspending timeout\n");
				return -ETIMEDOUT;
			}
			msleep(100);
			goto retry;
		case RPM_RESUMING:
			dev_dbg(&ndev->dev,
				"RPM Resuming, spin:%d\n", spin2);
			if (spin2-- == 0) {
				dev_err(&ndev->dev,
				"Modem resume timeout\n");
				return -ETIMEDOUT;
			}
			msleep(50);
			goto retry;
		case RPM_ACTIVE:
			break;
		default:
			return -EIO;
		}
	}
	return 0;
}
#else
static int usbsvn_initiated_resume(struct net_device *ndev) {return 0; }
#endif

static int usbsvn_write(struct net_device *dev, struct sipc4_tx_data *tx_data)
{
	struct usbsvn *svn = netdev_priv(dev);
	struct sk_buff *skb;
	struct usbsvn_devdata *devdata;
	struct urb *req;
	int dev_id;
	int err;

	if (!svn->usbdev)
		return -1;

	/*hold on active mode until xmit*/
	usb_mark_last_busy(svn->usbdev);
	wake_lock_pm(svn);

	err = usbsvn_initiated_resume(dev);
	if (err < 0) {
		printk(KERN_ERR "%s: usbsvn_initated_resume fail\n", __func__);
		goto exit;
	}

	skb = tx_data->skb;
	dev_id = SIPC4_FORMAT(tx_data->res);
	devdata = &svn->devdata[dev_id];

	req = usb_alloc_urb(0, GFP_ATOMIC);
	if (!req) {
		printk(KERN_ERR "%s: can't get urb\n", __func__);
		err = -ENOMEM;
		goto exit;
	}

	usb_fill_bulk_urb(req, svn->usbdev, devdata->tx_pipe,
			skb->data, skb->len, tx_complete, skb);

	if (tx_debug) {
		char *buf = skb->data;
		int i;

		printk(KERN_ERR
			"[TX] dev_id: %d, size: %d\n", dev_id, skb->len);
		for (i = 0; i < skb->len; i++)
			printk(KERN_ERR "%x ", *(buf + i));
		if (skb->len)
			printk(KERN_ERR "\n");
	}

	req->transfer_flags = URB_ZERO_PACKET;
	err = usb_submit_urb(req, GFP_ATOMIC);
	if (err < 0) {
		printk(KERN_ERR "%s:usb_submit_urb fail\n", __func__);
		usb_free_urb(req);
		goto exit;
	}
	usb_mark_last_busy(svn->usbdev);
	if (dev_id == SIPC4_RAW)
		wake_lock_timeout_data(svn);

exit:
	return err;
}

static netdev_tx_t usbsvn_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct usbsvn *svn = netdev_priv(dev);

	skb_queue_tail(&svn->tx_skb_queue, skb);
	if(!work_pending(&svn->tx_work))
		queue_work(svn->tx_workqueue, &svn->tx_work);

	return NETDEV_TX_OK;
}

static void usbsvn_tx_worker(struct work_struct *work)
{
	struct usbsvn *svn = container_of(work, struct usbsvn, tx_work);
	struct net_device *dev = svn->netdev;
	struct sk_buff *skb;
	struct sipc4_tx_data tx_data;
	int err;

	skb = skb_dequeue(&svn->tx_skb_queue);
	while (skb) {
		if (skb->protocol != htons(ETH_P_PHONET))
			goto drop;

		tx_data.skb = skb;

		err = sipc4_tx(&tx_data);
		if (err < 0)
			goto drop;

		err = usbsvn_write(dev, &tx_data);
		if (err < 0) {
			printk("%s:svn write error=(%d)\n", __func__, err);
			skb = tx_data.skb;
			goto drop;
		}
		goto dequeue;

drop:
		printk(KERN_ERR "%s drop\n", __func__);
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;

dequeue:
		skb = skb_dequeue(&svn->tx_skb_queue);
	}
}

static void tx_complete(struct urb *req)
{
	struct sk_buff *skb = req->context;
	/* This netdev can be pdp netdevice, so don't use netdev_priv */
	struct net_device *dev = skb->dev;

	switch (req->status) {
	case 0:
		dev->stats.tx_bytes += skb->len;
		break;

	case -ENOENT:
	case -ECONNRESET:
	case -ESHUTDOWN:
		dev->stats.tx_aborted_errors++;
	default:
		dev->stats.tx_errors++;
		dev_err(&dev->dev, "TX error (%d)\n", req->status);
	}
	dev->stats.tx_packets++;

	dev_kfree_skb_any(skb);
	usb_mark_last_busy(req->dev);
	usb_free_urb(req);
}

int usbsvn_cmd_rx(struct sipc4_rx_data *data, struct usbsvn *svn)
{
	struct net_device *ndev = data->dev;
	struct pdp_device *pdp = &svn->pdp;
	char *buf = page_address(data->page);
	unsigned short *cmd = (unsigned short *)buf;
	int i;
	int data_size = data->size;

	printk(KERN_DEBUG
		"svn flowctl CMD:size=%d:"
		"%02X %02X %02X %02x  %02X %02X %02X %02X  "
		"%02X %02X %02X %02X  %02X %02X %02X %02X\n",
		data->size,
		*(buf), *(buf+1), *(buf+2), *(buf+3),
		*(buf+4), *(buf+5), *(buf+6), *(buf+7),
		*(buf+8), *(buf+9), *(buf+10), *(buf+11),
		*(buf+12), *(buf+13), *(buf+14), *(buf+15)
	);

	while(data_size) {
		switch (*cmd) {
		case SIPC4_CMD_SUSPEND:
			/*dev_dbg(&ndev->dev, "svn flowctl suspend\n");*/
			svn->flow_suspend = 1;
			break;

		case SIPC4_CMD_RESUME:
			netif_wake_queue(ndev);
			/*dev_dbg(&ndev->dev, "svn flowctl resume\n");*/
			svn->flow_suspend = 0;
			for (i=0 ; i<PDP_CH_MAX; i++) {
				if(pdp->pdp_devs[i]) {
					netif_wake_queue(pdp->pdp_devs[i]);
					dev_dbg(&ndev->dev,
					"svn flowctl wake queue i=%d\n", i);
				}
			}
			break;
		}
		data_size -= sizeof(unsigned short);
		cmd++;
	}
}

static int rx_submit(struct usbsvn *svn, int dev_id, struct urb *req,
		gfp_t gfp_flags)
{
	struct net_device *dev = svn->netdev;
	struct usbsvn_devdata *devdata = &svn->devdata[dev_id];
	struct usbsvn_rx *svn_rx;
	struct page *page;
	int err;

	svn_rx = kzalloc(sizeof(struct usbsvn_rx), gfp_flags);
	if (!svn_rx)
		return -ENOMEM;

	page = __netdev_alloc_page(dev, gfp_flags);
	if (!page) {
		kfree(svn_rx);
		return -ENOMEM;
	}

	svn_rx->netdev = dev;
	svn_rx->dev_id = dev_id;

	usb_fill_bulk_urb(req, svn->usbdev, devdata->rx_pipe,
			page_address(page), PAGE_SIZE, rx_complete, svn_rx);
	req->transfer_flags = 0;

	err = usb_submit_urb(req, gfp_flags);
	if (unlikely(err)) {
		dev_err(&dev->dev, "RX submit error (%d)\n", err);
		kfree(svn_rx);
		netdev_free_page(dev, page);
	}
	usb_mark_last_busy(req->dev);

	return err;
}

static void rx_complete(struct urb *req)
{
	struct usbsvn_rx *svn_rx = req->context;
	struct net_device *dev = svn_rx->netdev;
	struct usbsvn *svn = netdev_priv(dev);
	struct page *page = virt_to_page(req->transfer_buffer);
	struct sipc4_rx_data rx_data;
	int dev_id = svn_rx->dev_id;
	int flags = 0;
	int err;

	usb_mark_last_busy(svn->usbdev);

	switch (req->status) {
	case -ENOENT:
		if (req->actual_length == 0) {
			req = NULL;
			break;
		}
		printk(KERN_DEBUG "%s: Rx ENOENT", __func__);

	case 0:
		if (!svn->driver_info)
			flags |= SIPC4_RX_HDLC;
		if (req->actual_length < PAGE_SIZE)
			flags |= SIPC4_RX_LAST;

		rx_data.dev = dev;
		rx_data.skb = svn->devdata[dev_id].rx_skb;
		rx_data.page = page;
		rx_data.size = req->actual_length;
		rx_data.format = dev_id;
		rx_data.flags = flags;
		rx_data.rx_hdr = &svn->devdata[dev_id].rx_hdr;

		page = NULL;

		if (rx_debug) {
			char *buf = req->transfer_buffer;
			int i;

			printk(KERN_DEBUG "[RX] dev_id: %d, size: %d\n", dev_id,
					req->actual_length);
			for (i = 0; i < req->actual_length; i++)
				printk(KERN_DEBUG "%x ", *(buf + i));
		}

		if (dev_id == SIPC4_CMD)
			err = usbsvn_cmd_rx(&rx_data, svn);
		else
			err = sipc4_rx(&rx_data);
		if (err < 0) {
			svn->devdata[dev_id].rx_skb = NULL;
			break;
		}
		svn->devdata[dev_id].rx_skb = rx_data.skb;

		if (dev_id == SIPC4_RAW)
			wake_lock_timeout_data(svn);

		goto resubmit;

	case -ECONNRESET:
	case -ESHUTDOWN:
		if (!svn->suspended)
			printk(KERN_DEBUG "%s: RX complete Status(%d)\n",
				__func__, req->status);
		req = NULL;
		break;

	case -EOVERFLOW:
		dev->stats.rx_over_errors++;
		dev_err(&dev->dev, "RX overflow\n");
		break;

	case -EILSEQ:
		dev->stats.rx_crc_errors++;
		break;
	}

	dev->stats.rx_errors++;

resubmit:
	kfree(svn_rx);

	if (page)
		netdev_free_page(dev, page);
	if (req && req->status != -ENOENT) {
		rx_submit(svn, dev_id, req, GFP_ATOMIC);
	}
}

static void usbsvn_stop(struct usbsvn *svn)
{
	struct net_device *dev = svn->netdev;
	int dev_id;
	int i;

	netif_stop_queue(dev);

	for (dev_id = svn->dev_count - 1; dev_id >= 0; dev_id--) {
		for (i = 0; i < rxq_size; i++) {
			int index = dev_id * rxq_size + i;
			struct urb *req = svn->urbs[index];

			if (!req)
				continue;

			usb_kill_urb(req);
			usb_free_urb(req);
			svn->urbs[index] = NULL;
		}
	}
}

static int usbsvn_start(struct usbsvn *svn)
{
	struct net_device *dev = svn->netdev;
	int dev_id;
	int i;

	for (dev_id = 0; dev_id < svn->dev_count; dev_id++) {
		for (i = 0; i < rxq_size; i++) {
			int index = dev_id * rxq_size + i;
			struct urb *req = usb_alloc_urb(0, GFP_KERNEL);

			if (!req || rx_submit(svn, dev_id, req, GFP_KERNEL)) {
				usbsvn_stop(svn);
				return -ENOMEM;
			}
			svn->urbs[index] = req;
		}
	}

	netif_wake_queue(dev);

	return 0;
}

static int usbsvn_open(struct net_device *dev)
{
	struct usbsvn *svn = netdev_priv(dev);
	int err;

	err = usbsvn_initiated_resume(dev);
	if (err < 0)
		return err;

	return usbsvn_start(svn);
}

static int usbsvn_close(struct net_device *dev)
{
	struct usbsvn *svn = netdev_priv(dev);
	int err;

	err = usbsvn_initiated_resume(dev);
	if (err < 0)
		return err;

	usbsvn_stop(svn);
	return 0;
}

static int usbsvn_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct if_phonet_req *req = (struct if_phonet_req *)ifr;

	switch (cmd) {
	case SIOCPNGAUTOCONF:
		req->ifr_phonet_autoconf.device = USBSVN_DEV_ADDR;
		return 0;
	}
	return -ENOIOCTLCMD;
}

static int usbsvn_set_mtu(struct net_device *dev, int new_mtu)
{
	if ((new_mtu < PHONET_MIN_MTU) || (new_mtu > PHONET_MAX_MTU))
		return -EINVAL;

	dev->mtu = new_mtu;
	return 0;
}

static const struct net_device_ops usbsvn_ops = {
	.ndo_open	= usbsvn_open,
	.ndo_stop	= usbsvn_close,
	.ndo_start_xmit = usbsvn_xmit,
	.ndo_do_ioctl	= usbsvn_ioctl,
	.ndo_change_mtu = usbsvn_set_mtu,
};

static void usbsvn_setup(struct net_device *dev)
{
	dev->features		= 0;
	dev->netdev_ops		= &usbsvn_ops,
	dev->header_ops		= &phonet_header_ops;
	dev->type		= ARPHRD_PHONET;
	dev->flags		= IFF_POINTOPOINT | IFF_NOARP;
	dev->mtu		= PHONET_MAX_MTU;
	dev->hard_header_len	= 1;
	dev->dev_addr[0]	= USBSVN_DEV_ADDR;
	dev->addr_len		= 1;
	dev->tx_queue_len	= 1000;

	dev->destructor		= free_netdev;
}

static struct usb_device_id usbsvn_ids[] = {
	{ USB_DEVICE(0x058b, 0x0041), /* XMM6260 Boot Rom */
	.driver_info = USBSVN_NOT_MAIN,
	},
	{ USB_DEVICE(0x058b, 0x0015), /* XMM6260 PSI */
	.driver_info = USBSVN_NOT_MAIN,
	},
	{ USB_DEVICE(0x1519, 0x0020), }, /* XMM6260 Main Modem Binary */
	{ },
};

MODULE_DEVICE_TABLE(usb, usbsvn_ids);

static struct usb_driver usbsvn_driver;

int usbsvn_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	const struct usb_cdc_union_desc *union_header = NULL;
	const struct usb_host_interface *data_desc;
	static struct usbsvn *svn;
	struct usb_interface *data_intf;
	struct usb_device *usbdev = interface_to_usbdev(intf);
	u8 *data = intf->altsetting->extra;
	int len = intf->altsetting->extralen;
	int dev_id;
	int err;

	if (!share_svn) {
		printk(KERN_ERR "%s: netdev not registed\n", __func__);
		return -EINVAL;
	}
	svn = share_svn;

	if (!len) {
		if (intf->cur_altsetting->endpoint->extralen &&
				intf->cur_altsetting->endpoint->extra) {
			dev_dbg(&intf->dev,
				"Seeking extra descriptors on endpoint\n");
			len = intf->cur_altsetting->endpoint->extralen;
			data = intf->cur_altsetting->endpoint->extra;
		} else {
			dev_err(&intf->dev,
				"Zero length descriptor references\n");
			return -EINVAL;
		}
	}

	while (len > 0) {
		/* bDescriptorType */
		if (data[1] == USB_DT_CS_INTERFACE) {
			/* bDescriptorSubType */
			switch (data[2]) {
			case USB_CDC_UNION_TYPE:
				if (union_header)
					break;
				union_header =
					(struct usb_cdc_union_desc *)data;
				break;
			default:
				break;
			}
		}

		data += data[0];
		len -= data[0];
	}

	if (!union_header) {
		dev_err(&intf->dev, "USB CDC isn't union type\n");
		return -EINVAL;
	}

	data_intf = usb_ifnum_to_if(usbdev, union_header->bSlaveInterface0);
	if (!data_intf)
		return -ENODEV;

	data_desc = data_intf->altsetting;

	/* To detect usb device order probed */
	dev_id = intf->altsetting->desc.bInterfaceNumber / 2;

	if (dev_id >= USBSVN_DEVNUM_MAX) {
		dev_err(&intf->dev, "Device id %d cannot support\n", dev_id);
		return -EINVAL;
	}

	printk(KERN_ERR "%s: probe dev_id=%d\n", __func__, dev_id);
	if (dev_id > 0)
		goto skip_netdev;

	svn->usbdev = usbdev;
	svn->driver_info = (unsigned long)id->driver_info;

	/* FIXME: Does need this indeed? */
	usbdev->autosuspend_delay = msecs_to_jiffies(200);      /* 200ms */

	if (!svn->driver_info) {
		schedule_delayed_work(&svn->pm_runtime_work,
			msecs_to_jiffies(10000));
	}
	svn->usbsvn_connected = 1;
	svn->flow_suspend = 0;

skip_netdev:
	if (!svn->driver_info) {
		svn = share_svn;
		if (!svn) {
			dev_err(&intf->dev,
			"svnet device doesn't be allocated\n");
			err = ENOMEM;
			goto out;
		}
	}

	usb_get_dev(usbdev);
	svn->devdata[dev_id].data_intf = data_intf;

	/* Endpoints */
	if (usb_pipein(data_desc->endpoint[0].desc.bEndpointAddress)) {
		svn->devdata[dev_id].rx_pipe = usb_rcvbulkpipe(usbdev,
				data_desc->endpoint[0].desc.bEndpointAddress);
		svn->devdata[dev_id].tx_pipe = usb_sndbulkpipe(usbdev,
				data_desc->endpoint[1].desc.bEndpointAddress);
	} else {
		svn->devdata[dev_id].rx_pipe = usb_rcvbulkpipe(usbdev,
				data_desc->endpoint[1].desc.bEndpointAddress);
		svn->devdata[dev_id].tx_pipe = usb_sndbulkpipe(usbdev,
				data_desc->endpoint[0].desc.bEndpointAddress);
	}

	err = usb_driver_claim_interface(&usbsvn_driver, data_intf, svn);
	if (err < 0)
		goto out;

	usb_set_intfdata(intf, svn);
	svn->dev_count++;

	if (dev_id == 0)
		dev_info(&usbdev->dev, "USB CDC SVNET device found\n");

	pm_suspend_ignore_children(&intf->dev, true);
	svn->devdata[dev_id].disconnected = 0;

	return 0;

out:
	usb_set_intfdata(intf, NULL);
	return err;
}

static void usbsvn_disconnect(struct usb_interface *intf)
{
	struct usbsvn *svn = usb_get_intfdata(intf);
	struct usb_device *usbdev = svn->usbdev;
	int dev_id = intf->altsetting->desc.bInterfaceNumber / 2;
	struct device *ppdev;

	if (svn->devdata[dev_id].disconnected)
		return;

	svn->usbsvn_connected = 0;
	svn->flow_suspend = 1;

	dev_err(&usbdev->dev, "%s\n", __func__);
	svn->dev_count--;
	svn->devdata[dev_id].disconnected = 1;
	usb_driver_release_interface(&usbsvn_driver,
			svn->devdata[dev_id].data_intf);

	ppdev = usbdev->dev.parent->parent;
	pm_runtime_forbid(ppdev); /*ehci*/
	usb_put_dev(usbdev);

	if (svn->dev_count == 0) {
		svn->usbdev = NULL;

		cancel_delayed_work_sync(&svn->pm_runtime_work);
		cancel_work_sync(&svn->post_resume_work);
		if (!svn->driver_info) {
			/*TODO:check the Phone ACTIVE pin*/
#ifdef CONFIG_SAMSUNG_PHONE_SVNET
			if (mc_is_modem_active()) {
				svn->reconnect_cnt = 2;
				schedule_delayed_work(&svn->try_reconnect_work,
					10);
			}
#endif
			wake_unlock_pm(svn);
		}
	}
}

int usbsvn_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usbsvn *svn = usb_get_intfdata(intf);
	struct device *dev = &svn->usbdev->dev;

	if (svn->suspended)
		return 0;

	dev_dbg(dev, "svn L2 p.s=%d\n", dev->power.status);

	svn->suspended = 1;
	usbsvn_stop(svn);
	/* to request resume from xmit if there is sending data to CP */
	netif_wake_queue(svn->netdev);
	wake_lock_timeout_pm(svn);
	return 0;
}

int usbsvn_resume(struct usb_interface *intf)
{
	struct usbsvn *svn = usb_get_intfdata(intf);
	struct device *dev = &svn->usbdev->dev;

	if (!svn->suspended)
		return 0;

	if (dev->power.status == DPM_RESUMING)
		return 0;

	if (svn->resume_debug != 1) {
		dev_err(dev, "not expected !!\n");
		dump_stack();
	}

	dev_dbg(dev, "svn L0 p.s=%d\n", dev->power.status);
	wake_lock_pm(svn);
	svn->suspended = 0;

	return usbsvn_start(svn);
}

int usbsvn_reset_resume(struct usb_interface *intf)
{
	struct usbsvn *svn = usb_get_intfdata(intf);
	struct device *dev = &svn->usbdev->dev;
	int err;

	if (!svn->suspended)
		return 0;

	if (!share_svn)
		return -EFAULT;

	dev_dbg(dev, "L0-reset\n");
	wake_lock_pm(svn);
	svn->suspended = 0;
	err = usbsvn_start(svn);
	svn->dpm_suspending = 0;
	svn->skip_hostwakeup = 0;
	return err;
}

int usbsvn_pre_reset(struct usb_interface *intf)
{
	/* TODO */
	printk(KERN_NOTICE "[%d] %s\n", __LINE__, __func__);
	return 0;
}

int usbsvn_post_reset(struct usb_interface *intf)
{
	/* TODO */
	printk(KERN_NOTICE "[%d] %s\n", __LINE__, __func__);
	return 0;
}

static struct usb_driver usbsvn_driver = {
	.name		= "cdc_svnet",
	.probe		= usbsvn_probe,
	.disconnect	= usbsvn_disconnect,
	.id_table	= usbsvn_ids,
	.suspend	= usbsvn_suspend,
	.resume		= usbsvn_resume,
	.reset_resume	= usbsvn_reset_resume,
	.pre_reset	= usbsvn_pre_reset,
	.post_reset	= usbsvn_post_reset,
	.supports_autosuspend	= 1,
};

static int usbsvn_notifier_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	struct usbsvn *svn;

	if (!share_svn)
		return NOTIFY_DONE;

	svn = share_svn;


	switch (event) {
	case PM_SUSPEND_PREPARE:
		svn->dpm_suspending = 1;
		return NOTIFY_OK;
	case PM_POST_SUSPEND:
		svn->dpm_suspending = 0;
		if (svn->usbsvn_connected)
			schedule_work(&svn->post_resume_work);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block usbsvn_pm_notifier = {
	.notifier_call = usbsvn_notifier_event,
};

static int _netdev_init(void)
{
	static const char ifname[] = "svnet%d";
	struct net_device *netdev = NULL;
	static struct usbsvn *svn;
	int err;

	netdev = alloc_netdev(sizeof(*svn) + sizeof(svn->urbs[0]) * rxq_size *
			USBSVN_DEVNUM_MAX, ifname, usbsvn_setup);
	if (!netdev)
		return -ENOMEM;

	svn = netdev_priv(netdev);
	netif_stop_queue(netdev);
	memset(svn->urbs, 0x00,
		sizeof(svn->urbs[0]) * rxq_size * USBSVN_DEVNUM_MAX);
	svn->netdev = netdev;
	err = register_netdev(netdev);
	if (err < 0)
		goto out;

	skb_queue_head_init(&svn->tx_skb_queue);
	svn->tx_workqueue = create_singlethread_workqueue("svnet_txq");
	INIT_WORK(&svn->tx_work, usbsvn_tx_worker);
	INIT_WORK(&svn->post_resume_work, usbsvn_post_resume_work);
	INIT_DELAYED_WORK(&svn->pm_runtime_work, usbsvn_runtime_start);
	INIT_DELAYED_WORK(&svn->try_reconnect_work, usbsvn_try_reconnect_work);

	svn->pdp.parent.ndev = netdev;
	svn->pdp.parent.ndev_tx = usbsvn_write;
	svn->pdp.parent.tx_workqueue = svn->tx_workqueue;
	svn->pdp.parent.flowctl = &svn->flow_suspend;

	err = pdp_register(netdev, &svn->pdp);
	if (err < 0)
		goto netdev_out;

	err = sysfs_create_group(&netdev->dev.kobj, &usbsvn_attr_group);
	if (err < 0) {
		pdp_unregister(netdev);
		goto netdev_out;
	}

	_wake_lock_init(svn);
	svn->dpm_suspending = 0;
	svn->usbsvn_connected = 0;

	svn->usbdev = NULL;
	share_svn = svn;

	return err;

netdev_out:
	unregister_netdev(netdev);

out:
	return err;
}

static void _netdev_exit(void)
{
	struct usbsvn *svn = share_svn;
	struct net_device *netdev = svn->netdev;

	wake_unlock_pm(svn);
	_wake_lock_destroy(svn);
	cancel_work_sync(&svn->tx_work);
	cancel_delayed_work_sync(&svn->try_reconnect_work);
	destroy_workqueue(svn->tx_workqueue);
	sysfs_remove_group(&netdev->dev.kobj, &usbsvn_attr_group);
	pdp_unregister(netdev);
	unregister_netdev(netdev);
}

static int __init usbsvn_init(void)
{
	int err;

	register_pm_notifier(&usbsvn_pm_notifier);

	err = _netdev_init();
	if (err < 0) {
		printk(KERN_ERR "%s: netdev register fail\n", __func__);
		goto out;
	}

	err = usb_register(&usbsvn_driver);
	if (err < 0) {
		printk(KERN_ERR "%s: usb register fail\n", __func__);
		goto out;
	}
out:
	return err;
}

static void __exit usbsvn_exit(void)
{
	unregister_pm_notifier(&usbsvn_pm_notifier);
	_netdev_exit();
	usb_deregister(&usbsvn_driver);
}

module_init(usbsvn_init);
module_exit(usbsvn_exit);

MODULE_DESCRIPTION("USB CDC Samsung virtual network interface");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_LICENSE("GPL");
