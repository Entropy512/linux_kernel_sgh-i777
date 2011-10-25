/*
 * Samsung Virtual Network driver: PDP
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/if_arp.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include "sipc4.h"
#include "pdp.h"

static LIST_HEAD(pdp_device_list);

static int pdp_open(struct net_device *ndev)
{
	netif_start_queue(ndev);
	return 0;
}

static int pdp_stop(struct net_device *ndev)
{
	netif_stop_queue(ndev);
	return 0;
}

static void pdp_tx_worker(struct work_struct *work)
{
	struct pdp_priv *priv = container_of(work, struct pdp_priv, tx_work);
	struct pdp_parent *parent = priv->parent;
	struct net_device *ndev = priv->ndev;
	struct sk_buff *skb;
	struct sipc4_tx_data tx_data;
	int err;

	skb = skb_dequeue(&priv->tx_skb_queue);
	while (skb) {
		if (skb->protocol != htons(ETH_P_IP))
			goto drop;

		tx_data.skb = skb;
		tx_data.res = SIPC4_RES(SIPC4_RAW, PDP_CH_SVNET(priv->ch));

		err = sipc4_tx(&tx_data);
		if (err < 0)
			goto drop;

		err = parent->ndev_tx(parent->ndev, &tx_data);
		if (err < 0)
			goto drop;

		goto dequeue;

drop:
		dev_kfree_skb(tx_data.skb);
		ndev->stats.tx_dropped++;

dequeue:
		skb = skb_dequeue(&priv->tx_skb_queue);
	}
}

static netdev_tx_t pdp_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct pdp_priv *priv = netdev_priv(ndev);
	int *fsuspend = priv->parent->flowctl;

	skb_queue_tail(&priv->tx_skb_queue, skb);
	if (*fsuspend) {
		netif_stop_queue(ndev);
		printk("%s: svn flowctl pdp suspended, ch=%d\n", __func__,
			priv->ch);
	} else {
		if (!work_pending(&priv->tx_work))
			queue_work(priv->parent->tx_workqueue, &priv->tx_work);
	}

	return NETDEV_TX_OK;
}

static struct net_device_ops pdp_ops = {
	.ndo_open	= pdp_open,
	.ndo_stop	= pdp_stop,
	.ndo_start_xmit	= pdp_xmit,
};

static void pdp_setup(struct net_device *ndev)
{
	ndev->netdev_ops	= &pdp_ops;
	ndev->type		= ARPHRD_PPP;
	ndev->flags		= IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	ndev->mtu		= ETH_DATA_LEN;
	ndev->hard_header_len	= 0;
	ndev->addr_len		= 0;
	ndev->tx_queue_len	= 1000;
	ndev->watchdog_timeo	= 5 * HZ;
}

static struct net_device *create_pdp(struct pdp_parent *parent, int ch)
{
	struct net_device *ndev;
	struct pdp_priv *priv;
	char devname[IFNAMSIZ];
	int err;

	if (!parent)
		return ERR_PTR(-EINVAL);

	sprintf(devname, "pdp%d", ch - PDP_CH_MIN);
	ndev = alloc_netdev(sizeof(struct pdp_priv), devname, pdp_setup);
	if (!ndev)
		return ERR_PTR(-ENOMEM);

	priv = netdev_priv(ndev);
	netif_stop_queue(ndev);

	priv->ndev = ndev;
	priv->parent = parent;
	skb_queue_head_init(&priv->tx_skb_queue);
	INIT_WORK(&priv->tx_work, pdp_tx_worker);
	priv->ch = ch;

	err = register_netdev(ndev);
	if (err) {
		free_netdev(ndev);
		return ERR_PTR(err);
	}

	return ndev;
}

static void destroy_pdp(struct net_device *ndev)
{
	struct pdp_priv *priv = netdev_priv(ndev);

	if (!ndev)
		return;

	flush_work(&priv->tx_work);
	unregister_netdev(ndev);
	free_netdev(ndev);
}

static struct pdp_device *pdp_get_device(struct net_device *parent_ndev)
{
	struct pdp_device *pdp;

	list_for_each_entry(pdp, &pdp_device_list, node) {
		if (pdp->parent.ndev == parent_ndev)
			return pdp;
	}

	return NULL;
}

static int pdp_activate(struct pdp_device *pdp, int ch)
{
	struct net_device *ndev;
	int index;

	if (!pdp)
		return -EINVAL;

	if (ch < PDP_CH_MIN || ch > PDP_CH_MAX)
		return -EINVAL;

	index = ch - PDP_CH_MIN;

	mutex_lock(&pdp->pdp_mutex);

	if (pdp->pdp_devs[index]) {
		mutex_unlock(&pdp->pdp_mutex);
		return -EBUSY;
	}

	ndev = create_pdp(&pdp->parent, ch);
	if (IS_ERR(ndev)) {
		mutex_unlock(&pdp->pdp_mutex);
		return PTR_ERR(ndev);
	}

	pdp->pdp_devs[index] = ndev;
	pdp->pdp_cnt++;

	mutex_unlock(&pdp->pdp_mutex);
	return 0;
}

static int pdp_deactivate(struct pdp_device *pdp, int ch)
{
	int index;

	if (ch < PDP_CH_MIN || ch > PDP_CH_MAX)
		return -EINVAL;

	index = ch - PDP_CH_MIN;

	mutex_lock(&pdp->pdp_mutex);

	if (!pdp->pdp_devs[index]) {
		mutex_unlock(&pdp->pdp_mutex);
		return -EBUSY;
	}

	destroy_pdp(pdp->pdp_devs[index]);
	pdp->pdp_devs[index] = NULL;
	clear_bit(index, &pdp->pdp_suspend_bitmap);
	pdp->pdp_cnt--;

	mutex_unlock(&pdp->pdp_mutex);

	return 0;
}

static ssize_t pdp_activate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = to_net_dev(dev);
	struct pdp_device *pdp = pdp_get_device(ndev);
	int count = 0;
	int i;

	if (!pdp)
		return -EINVAL;

	mutex_lock(&pdp->pdp_mutex);

	for (i = 0; i < PDP_CH_NR; i++) {
		if (pdp->pdp_devs[i])
			count += sprintf(buf + count, "%d\n", (i + PDP_CH_MIN));
	}

	mutex_unlock(&pdp->pdp_mutex);

	return count;
}

static ssize_t pdp_activate_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = to_net_dev(dev);
	struct pdp_device *pdp = pdp_get_device(ndev);
	unsigned long ch;
	int err;

	if (!pdp)
		return -EINVAL;

	err = strict_strtoul(buf, 10, &ch);
	if (err < 0)
		return err;

	err = pdp_activate(pdp, ch);
	if (err < 0) {
		dev_err(dev, "Failed to activate pdp channel %lu: %d\n",
				ch, err);
		return err;
	}

	dev_info(dev, "pdp channel %ld activated\n", ch);

	return count;
}

static ssize_t pdp_deactivate_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = to_net_dev(dev);
	struct pdp_device *pdp = pdp_get_device(ndev);
	unsigned long ch;
	int err;

	if (!pdp)
		return -EINVAL;

	err = strict_strtoul(buf, 10, &ch);
	if (err < 0)
		return err;

	err = pdp_deactivate(pdp, ch);
	if (err < 0) {
		dev_err(dev, "Failed to deactivate pdp channel %lu: %d\n",
				ch, err);
		return err;
	}

	dev_info(dev, "pdp channel %ld deactivated\n", ch);

	return count;
}

static ssize_t pdp_suspend_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = to_net_dev(dev);
	struct pdp_device *pdp = pdp_get_device(ndev);
	int count = 0;
	int i;

	if (!pdp)
		return -EINVAL;

	mutex_lock(&pdp->pdp_mutex);

	for (i = 0; i < PDP_CH_NR; i++) {
		if (test_bit(i, &pdp->pdp_suspend_bitmap))
			count += sprintf(buf + count, "%d\n", (i + PDP_CH_MIN));
	}

	mutex_unlock(&pdp->pdp_mutex);

	return count;
}

static ssize_t pdp_suspend_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = to_net_dev(dev);
	struct pdp_device *pdp = pdp_get_device(ndev);
	unsigned long ch;
	int index;
	int err;

	if (!pdp)
		return -EINVAL;

	err = strict_strtoul(buf, 10, &ch);
	if (err < 0)
		return err;

	if (ch < PDP_CH_MIN || ch > PDP_CH_MAX)
		return -EINVAL;

	index = ch - PDP_CH_MIN;

	mutex_lock(&pdp->pdp_mutex);

	set_bit(index, &pdp->pdp_suspend_bitmap);

	if (pdp->pdp_devs[index])
		netif_stop_queue(pdp->pdp_devs[index]);

	mutex_unlock(&pdp->pdp_mutex);

	return count;
}

static ssize_t pdp_resume_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = to_net_dev(dev);
	struct pdp_device *pdp = pdp_get_device(ndev);
	unsigned long ch;
	int index;
	int err;

	if (!pdp)
		return -EINVAL;

	err = strict_strtoul(buf, 10, &ch);
	if (err < 0)
		return err;

	if (ch < PDP_CH_MIN || ch > PDP_CH_MAX)
		return -EINVAL;

	index = ch - PDP_CH_MIN;

	mutex_lock(&pdp->pdp_mutex);

	clear_bit(index, &pdp->pdp_suspend_bitmap);

	if (pdp->pdp_devs[index])
		netif_wake_queue(pdp->pdp_devs[index]);

	mutex_unlock(&pdp->pdp_mutex);

	return count;
}

static DEVICE_ATTR(activate, 0664, pdp_activate_show, pdp_activate_store);
static DEVICE_ATTR(deactivate, 0664, NULL, pdp_deactivate_store);
static DEVICE_ATTR(suspend, 0664, pdp_suspend_show, pdp_suspend_store);
static DEVICE_ATTR(resume, 0664, NULL, pdp_resume_store);

static struct attribute *pdp_attrs[] = {
	&dev_attr_activate.attr,
	&dev_attr_deactivate.attr,
	&dev_attr_suspend.attr,
	&dev_attr_resume.attr,
	NULL,
};

static const struct attribute_group pdp_attr_group = {
	.name = "pdp",
	.attrs = pdp_attrs,
};

/* This is called from sipc4_raw_rx */
int pdp_netif_rx(struct net_device *parent_ndev, struct sk_buff *skb,
		 int svnet_ch)
{
	struct pdp_device *pdp = pdp_get_device(parent_ndev);
	struct net_device *ndev;
	int index = SVNET_CH_PDP(svnet_ch) - PDP_CH_MIN;
	int err;

	ndev = pdp->pdp_devs[index];
	if (!ndev)
		return NET_RX_DROP;

	skb->protocol = htons(ETH_P_IP);

	skb->dev = ndev;
	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += skb->len;

	skb_reset_mac_header(skb);

	err = netif_rx(skb);
	if (err != NET_RX_SUCCESS)
		dev_err(&ndev->dev, "rx error: %d\n", err);

	return err;
}
EXPORT_SYMBOL_GPL(pdp_netif_rx);

int pdp_register(struct net_device *ndev, struct pdp_device *pdp)
{
	mutex_init(&pdp->pdp_mutex);

	list_add_tail(&pdp->node, &pdp_device_list);

	return sysfs_create_group(&ndev->dev.kobj, &pdp_attr_group);
}
EXPORT_SYMBOL_GPL(pdp_register);

void pdp_unregister(struct net_device *ndev)
{
	struct pdp_device *pdp = pdp_get_device(ndev);
	int i;

	if (!pdp)
		return;

	for (i = 0; i < PDP_CH_NR; i++)
		pdp_deactivate(pdp, i + PDP_CH_MIN);

	list_del_init(&pdp->node);
	sysfs_remove_group(&ndev->dev.kobj, &pdp_attr_group);
}
EXPORT_SYMBOL_GPL(pdp_unregister);
