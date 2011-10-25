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

#define PDP_CH_MIN		1
#define PDP_CH_MAX		15
#define PDP_CH_NR		(PDP_CH_MAX - PDP_CH_MIN + 1)
#define PDP_CH_SVNET(ch)	(ch + CHID_PSD_DATA1 - PDP_CH_MIN)
#define SVNET_CH_PDP(ch)	(ch - CHID_PSD_DATA1 + PDP_CH_MIN)

struct pdp_parent {
	struct net_device	*ndev;
	int			(*ndev_tx)(struct net_device *ndev,
					   struct sipc4_tx_data *tx_data);
	struct workqueue_struct *tx_workqueue;
	int 			*flowctl;
};

struct pdp_device {
	struct list_head	node;
	struct pdp_parent	parent;
	struct mutex		pdp_mutex;
	struct net_device	*pdp_devs[PDP_CH_NR];
	int pdp_cnt;
	unsigned long		pdp_suspend_bitmap;
};

struct pdp_priv {
	struct net_device	*ndev;
	struct pdp_parent	*parent;
	struct work_struct	tx_work;
	struct sk_buff_head	tx_skb_queue;
	int			ch;
};

extern int pdp_register(struct net_device *ndev, struct pdp_device *pdp);
extern void pdp_unregister(struct net_device *ndev);
