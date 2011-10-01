/*
 * Samsung modem IPC header verstion 4
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
#include <linux/if_phonet.h>
#include <linux/phonet.h>

#include <net/sock.h>
#include <net/phonet/phonet.h>

#include "sipc4.h"

static const char hdlc_start[1] = { HDLC_START };
static const char hdlc_end[1] = { HDLC_END };

/* FIXME: Consider multi svnet */
struct sk_buff_head fmt_multi_list[FMT_MULTI_NR];
struct rfs_frag rfs_frag_tx;

static int sipc4_add_hdlc(struct sipc4_tx_data *data, void *header,
			  int header_size, bool head, bool tail)
{
	struct sk_buff *skb_new;
	int headroom = 0;
	int tailroom = 0;

	if (!head && !tail)
		return 0;

	if (head)
		headroom = sizeof(hdlc_start) + header_size;
	if (tail)
		tailroom = sizeof(hdlc_end);

	skb_new = skb_copy_expand(data->skb, headroom, tailroom, GFP_ATOMIC);
	if (!skb_new)
		return -ENOMEM;

	dev_kfree_skb_any(data->skb);

	if (head) {
		memcpy(skb_push(skb_new, header_size), header, header_size);
		memcpy(skb_push(skb_new, sizeof(hdlc_start)), hdlc_start,
				sizeof(hdlc_start));
	}
	if (tail)
		memcpy(skb_put(skb_new, sizeof(hdlc_end)), hdlc_end,
				sizeof(hdlc_end));

	data->skb = skb_new;

	return 0;
}

static int sipc4_fmt_tx(struct sipc4_tx_data *data)
{
	unsigned int ch = SIPC4_CH(data->res);
	struct fmt_hdr header;

	/* If channel is 0, it is not HDLC frame */
	if (ch == 0)
		return 0;

	header.len = sizeof(struct fmt_hdr) + data->skb->len;
	/* TODO: Multi frame */
	header.control = 0;

	return sipc4_add_hdlc(data, &header, sizeof(struct fmt_hdr), 1, 1);
}

static int sipc4_raw_tx(struct sipc4_tx_data *data)
{
	unsigned int ch = SIPC4_CH(data->res);
	struct raw_hdr header;

	header.len = sizeof(struct raw_hdr) + data->skb->len;
	header.channel = ch;
	header.control = 0;

	return sipc4_add_hdlc(data, &header, sizeof(struct raw_hdr), 1, 1);
}

static int sipc4_rfs_tx(struct sipc4_tx_data *data)
{
	struct sk_buff *skb = data->skb;
	struct rfs_hdr *header = &rfs_frag_tx.header;
	int len = rfs_frag_tx.len + skb->len;
	bool head = 0;
	bool tail = 0;
	int err;

	if (rfs_frag_tx.len == 0) {
		memcpy(header, skb->data, sizeof(struct rfs_hdr));
		head = 1;
	}

	if (header->len == len) {
		rfs_frag_tx.len = 0;
		tail = 1;
	} else if (header->len > len) {
		rfs_frag_tx.len = 0;
		return -EMSGSIZE;
	} else
		rfs_frag_tx.len += skb->len;

	err = sipc4_add_hdlc(data, header, 0, head, tail);
	if (err < 0)
		rfs_frag_tx.len = 0;

	return err;
}

int sipc4_tx(struct sipc4_tx_data *data)
{
	struct sk_buff *skb = data->skb;
	struct phonethdr *ph;
	unsigned int format;
	int err;

	if (!skb)
		return -EINVAL;

	if (skb->protocol == htons(ETH_P_PHONET)) {
		ph = pn_hdr(skb);
		data->res = ph->pn_res;

		/* Remove phonet header + address len(1byte)? */
		skb_pull(skb, sizeof(struct phonethdr) + 1);
	}

	format = SIPC4_FORMAT(data->res);

	switch (format) {
	case SIPC4_FMT:
		err = sipc4_fmt_tx(data);
		break;
	case SIPC4_RAW:
		err = sipc4_raw_tx(data);
		break;
	case SIPC4_RFS:
		err = sipc4_rfs_tx(data);
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}
EXPORT_SYMBOL_GPL(sipc4_tx);

static struct sk_buff *sipc4_alloc_skb(struct net_device *dev, int len)
{
	struct sk_buff *skb =
		netdev_alloc_skb(dev, sizeof(struct phonethdr) + len);

	if (unlikely(!skb))
		return NULL;

	skb_reserve(skb, sizeof(struct phonethdr));

	return skb;
}

static int sipc4_netif_rx(struct net_device *dev, struct sk_buff *skb, int res)
{
	struct phonethdr *ph;
	int err;

	skb->protocol = htons(ETH_P_PHONET);

	ph = (struct phonethdr *)skb_push(skb,
			sizeof(struct phonethdr));

	ph->pn_rdev = dev->dev_addr[0];
	ph->pn_sdev = 0;
	ph->pn_res = res;
	ph->pn_length =
		__cpu_to_be16(skb->len + 2 - sizeof(*ph));
	ph->pn_robj = 0;
	ph->pn_sobj = 0;

	skb->dev = dev;
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;

	skb_reset_mac_header(skb);

	err = netif_rx(skb);
	if (err != NET_RX_SUCCESS)
		dev_err(&dev->dev, "rx error: %d\n", err);

	return err;
}

static int sipc4_get_header_size(int format)
{
	switch (format) {
	case SIPC4_FMT:
		return sizeof(struct fmt_hdr);
	case SIPC4_RAW:
		return sizeof(struct raw_hdr);
	case SIPC4_RFS:
		return sizeof(struct rfs_hdr);
	default:
		break;
	}

	return 0;
}

static int sipc4_get_hdlc_size(void *buf, int format)
{
	struct fmt_hdr *fmt_header;
	struct raw_hdr *raw_header;
	struct rfs_hdr *rfs_header;

	switch (format) {
	case SIPC4_FMT:
		fmt_header = (struct fmt_hdr *)buf;
		return fmt_header->len;
	case SIPC4_RAW:
		raw_header = (struct raw_hdr *)buf;
		return raw_header->len;
	case SIPC4_RFS:
		rfs_header = (struct rfs_hdr *)buf;
		return rfs_header->len;
	default:
		break;
	}

	return 0;
}

static int sipc4_get_hdlc_ch(void *buf, int format)
{
	struct raw_hdr *raw_header;

	switch (format) {
	case SIPC4_FMT:
		return SIPC4_DEFAULT_CH;
	case SIPC4_RAW:
		raw_header = (struct raw_hdr *)buf;
		return raw_header->channel;
	case SIPC4_RFS:
		return SIPC4_DEFAULT_CH;
	default:
		break;
	}

	return SIPC4_DEFAULT_CH;
}

static int sipc4_get_hdlc_control(void *buf, int format)
{
	struct fmt_hdr *fmt_header;
	struct raw_hdr *raw_header;

	switch (format) {
	case SIPC4_FMT:
		fmt_header = (struct fmt_hdr *)buf;
		return fmt_header->control;
	case SIPC4_RAW:
		raw_header = (struct raw_hdr *)buf;
		return raw_header->control;
	case SIPC4_RFS:
	default:
		break;
	}

	return 0;
}

static int sipc4_check_hdlc_start(struct sipc4_rx_data *data, char *buf)
{
	struct net_device *dev = data->dev;

	if (data->skb)
		return 0;

	if (strncmp(buf, hdlc_start, sizeof(hdlc_start))) {
		dev_err(&dev->dev, "Wrong HDLC start: 0x%x\n", *buf);
		return -EBADMSG;
	}

	return sizeof(hdlc_start);
}

static void print_buf(const char *buf, size_t len, const char* tag)
{
	int i;
	char *b = buf;

	for(i=0;i < len/16 + 1; i++) {
		printk(KERN_DEBUG
		"%s:%02X %02X %02X %02X  %02X %02X %02X %02X  "
		"%02X %02X %02X %02X  %02X %02X %02X %02X\n",
		(tag) ? tag : "buf", *b++, *b++, *b++, *b++, *b++, *b++, *b++,
		*b++, *b++, *b++, *b++, *b++, *b++, *b++, *b++, *b++
		);
	}
}

#define sipc_min(a, b) (((a) < (b)) ? (a) : (b))
#define sipc_max(a, b) (((a) > (b)) ? (a) : (b))
static int sipc4_check_header(struct sipc4_rx_data *data, char *buf, int rest)
{
	struct sipc_rx_hdr *hdr = data->rx_hdr;
	int head_size = sipc4_get_header_size(data->format);
	int done_len = 0;
	int len;

	if (!hdr->start) {
		len = sipc4_check_hdlc_start(data, buf);
		if (len < 0)
			return len;
		memcpy(&hdr->start, hdlc_start, sizeof(hdlc_start));
		hdr->len = 0;

		switch (data->format) {
		case SIPC4_FMT:
		/*case SIPC4_RAW:*/
		/*case SIPC4_RFS:*/
			print_buf(buf, 0, NULL);
			break;
		}
		buf += len;
		rest -= len;
		done_len += len;
	}

	if (hdr->len < head_size) {
		len = sipc_min(rest, head_size - hdr->len);
		memcpy(hdr->hdr + hdr->len, buf, len);
		hdr->len += len;
		done_len += len;
	}
	return done_len;
}

static int sipc4_hdlc_format_rx(struct sipc4_rx_data *data);

static int sipc4_check_data(struct sipc4_rx_data *data, char *buf, int rest)
{
	struct net_device *dev = data->dev;
	struct sk_buff *skb = data->skb;
	struct sipc_rx_hdr *hdr = data->rx_hdr;
	int head_size = sipc4_get_header_size(data->format);
	int data_size = sipc4_get_hdlc_size(hdr->hdr, data->format) - head_size;
	int alloc_size = data_size;
	int len;
	struct sk_buff *skb_new;
	int err;

	if (!skb) {
		switch (data->format) {
		case SIPC4_RFS:
			/* Send the RFS header to RILD*/
			alloc_size = sipc_min(data_size, rest) + head_size;
			skb = sipc4_alloc_skb(dev, alloc_size);
			if (unlikely(!skb))
				return -ENOMEM;
			memcpy(skb_put(skb, head_size), hdr->hdr, head_size);
			break;

		default:
			skb = sipc4_alloc_skb(dev, alloc_size);
			if (unlikely(!skb))
				return -ENOMEM;
			break;
		}
		data->skb = skb;
	} else {
		switch (data->format) {
		case SIPC4_RFS:
			alloc_size = sipc_min(data_size - hdr->flag_len, rest);
			skb_new = sipc4_alloc_skb(data->dev, alloc_size);
			if (unlikely(!skb_new))
				return -ENOMEM;

			err = sipc4_hdlc_format_rx(data);
			if (err < 0) {
				dev_kfree_skb_any(skb_new);
				return err;
			}
			skb = data->skb = skb_new;
			break;

		default:
			/* do noting, IPC,RAW MTU is 1500byte */
			break;
		}
	}

	/* handle data */
	len = sipc_min(rest, alloc_size - skb->len);
	memcpy(skb_put(skb, len), buf, len);
	hdr->flag_len += len;

	return len;
}

static int sipc4_check_hdlc_end(struct sipc4_rx_data *data, char *buf)
{
	struct net_device *dev = data->dev;

	if (strncmp(buf, hdlc_end, sizeof(hdlc_end))) {
		dev_err(&dev->dev, "Wrong HDLC end: 0x%x\n", *buf);
		return -EBADMSG;
	}

	return sizeof(hdlc_end);
}

static int sipc4_fmt_rx(struct sipc4_rx_data *data)
{
	struct net_device *dev = data->dev;
	struct sk_buff *skb = data->skb;
	struct sipc_rx_hdr *hdr = data->rx_hdr;
	int ch = sipc4_get_hdlc_ch(hdr->hdr, SIPC4_FMT);
	int control = sipc4_get_hdlc_control(hdr->hdr, SIPC4_FMT);
	int info_id = control & FMT_INFOID_MASK;

	if (control & FMT_MORE_BIT) {
		skb_queue_tail(&fmt_multi_list[info_id], skb);
		return 0;
	}

	if (fmt_multi_list[info_id].qlen) {
		/* TODO: multi frame */
		;
	}

	sipc4_netif_rx(dev, skb, SIPC4_RES(SIPC4_FMT, ch));

	return 0;
}

static int sipc4_raw_rx(struct sipc4_rx_data *data)
{
	struct net_device *dev = data->dev;
	struct sk_buff *skb = data->skb;
	struct sipc_rx_hdr *hdr = data->rx_hdr;
	int ch = sipc4_get_hdlc_ch(hdr->hdr, SIPC4_RAW);

	/* check pdp channel */
	if (ch >= CHID_PSD_DATA1 && ch <= CHID_PSD_DATA15)
		pdp_netif_rx(dev, skb, ch);
	else
		sipc4_netif_rx(dev, skb, SIPC4_RES(SIPC4_RAW, ch));

	return 0;
}

static int sipc4_rfs_rx(struct sipc4_rx_data *data)
{
	struct net_device *dev = data->dev;
	struct sk_buff *skb = data->skb;
	struct sipc_rx_hdr *hdr = data->rx_hdr;
	int ch = sipc4_get_hdlc_ch(hdr->hdr, SIPC4_RFS);

	return sipc4_netif_rx(dev, skb, SIPC4_RES(SIPC4_RFS, ch));
}

static int sipc4_hdlc_format_rx(struct sipc4_rx_data *data)
{
	int err;

	switch (data->format) {
	case SIPC4_FMT:
		/*print_buf(data->skb->data, 15, "IPC");*/
		err = sipc4_fmt_rx(data);
		break;
	case SIPC4_RAW:
		/*print_buf(data->skb->data, 31, "RAW");*/
		err = sipc4_raw_rx(data);
		break;
	case SIPC4_RFS:
		/*print_buf(data->skb->data, 15, "RFS");*/
		err = sipc4_rfs_rx(data);
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int sipc4_hdlc_rx(struct sipc4_rx_data *data)
{
	int rest = data->size;
	char *buf = page_address(data->page);
	int len;
	int err;

	if (rest <= 0)
		goto end;

	if (data->format == SIPC4_FMT)
		printk(KERN_DEBUG "IPC:RX size=%d\n", data->size);

next_frame:
	err = len = sipc4_check_header(data, buf, rest);
	if (err < 0)
		goto end;
	buf += len;
	rest -= len;
	if (rest <= 0)
		goto end;

	err = len = sipc4_check_data(data, buf, rest);
	if (err < 0)
		goto end;
	buf += len;
	rest -= len;
	if (rest <= 0)
		goto end;

	err = len = sipc4_check_hdlc_end(data, buf);
	if (err < 0)
		goto end;
	buf += len;
	rest -= len;
	if (rest < 0)
		goto end;

	err = sipc4_hdlc_format_rx(data);
	if (err < 0)
		goto end;
	memset(data->rx_hdr, 0x00, sizeof(struct sipc_rx_hdr));

	data->skb = NULL;

	if (rest)
		goto next_frame;

end:
	netdev_free_page(data->dev, data->page);

	if (rest < 0)
		err = -ERANGE;

	if (err < 0 && data->skb) {
		dev_kfree_skb_any(data->skb);
		data->skb = NULL;
	}

	return err;
}

int sipc4_rx(struct sipc4_rx_data *data)
{
	struct net_device *dev = data->dev;
	struct sk_buff *skb;

	/* non HDLC frame */
	if (!(data->flags & SIPC4_RX_HDLC)) {
		int ch = SIPC4_NOT_HDLC_CH;

		/* get socket buffer */
		if (!data->skb) {
			skb = sipc4_alloc_skb(dev, 0);
			if (!skb)
				return -ENOMEM;
			data->skb = skb;
		} else
			skb = data->skb;

		/* handle data */
		skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, data->page, 0,
				data->size);

		/* rx data */
		if (data->flags & SIPC4_RX_LAST) {
			data->skb = NULL;
			sipc4_netif_rx(dev, skb, SIPC4_RES(SIPC4_FMT, ch));
		}

		return 0;
	}

	return sipc4_hdlc_rx(data);
}
EXPORT_SYMBOL_GPL(sipc4_rx);
