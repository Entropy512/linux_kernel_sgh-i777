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

/*
 * phonet resource
 * [0:4] - channel, 1 is default
 * [5:6] - format
 *
 * If format is SIPC4_FMT and channel is 0, it transmits only raw datas, so it
 * is used for special case such as modem boot.
 */
enum sipc4_format {
	SIPC4_FMT,
	SIPC4_RAW,
	SIPC4_RFS,
	SIPC4_CMD,
	SIPC4_FORMAT_MAX,
};

#define SIPC4_FORMAT(res)	(((res) >> 5) & 0x3)
#define SIPC4_CH(res)		((res) & 0x1f)
#define SIPC4_RES(format, ch)	((((format) & 0x3) << 5) | ((ch) & 0x1f))

#define SIPC4_NOT_HDLC_CH	0
#define SIPC4_DEFAULT_CH	1

#define SIPC4_RX_HDLC		(1 << 0)
#define SIPC4_RX_LAST		(1 << 1)

#define HDLC_START		0x7F
#define HDLC_END		0x7E

#define CMD_VALID		0x80
#define CMD_INT			0x40

#define SIPC4_CMD_SUSPEND		(CMD_VALID|CMD_INT|0xA)
#define SIPC4_CMD_RESUME		(CMD_VALID|CMD_INT|0xB)

#define SIPC4_MAX_HDR_SIZE 	48

struct sipc4_tx_data {
	struct sk_buff		*skb;
	unsigned char		res;
};

struct sipc_rx_hdr {
	int len;
	int flag_len;
	char start;
	char hdr[SIPC4_MAX_HDR_SIZE];
};

struct sipc4_rx_data {
	struct net_device	*dev;
	struct sk_buff		*skb;
	struct page		*page;
	int			size;
	int			format;
	int			flags;
	struct sipc_rx_hdr 	*rx_hdr;
};

struct sipc4_rx_frag {
	int			data_len;
};

/* Formatted IPC frame */
struct fmt_hdr {
	u16	len;
	u8	control;
} __attribute__ ((packed));

#define FMT_MORE_BIT		(1 << 7)
#define FMT_MULTI_NR		0x80
#define FMT_INFOID_MASK		0x7f

/* RAW IPC frame */
struct raw_hdr {
	u32	len;
	u8	channel;
	u8	control;
} __attribute__ ((packed));

/*
 * RAW IPC frame channel ID
 */
enum {
	CHID_0 = 0,
	CHID_CSD_VT_DATA,
	CHID_PDS_PVT_CONTROL,
	CHID_PDS_VT_AUDIO,
	CHID_PDS_VT_VIDEO,
	CHID_5,                /* 5 */
	CHID_6,
	CHID_CDMA_DATA,
	CHID_PCM_DATA,
	CHID_TRANSFER_SCREEN,
	CHID_PSD_DATA1,        /* 10 */
	CHID_PSD_DATA2,
	CHID_PSD_DATA3,
	CHID_PSD_DATA4,
	CHID_PSD_DATA5,
	CHID_PSD_DATA6,        /* 15 */
	CHID_PSD_DATA7,
	CHID_PSD_DATA8,
	CHID_PSD_DATA9,
	CHID_PSD_DATA10,
	CHID_PSD_DATA11,       /* 20 */
	CHID_PSD_DATA12,
	CHID_PSD_DATA13,
	CHID_PSD_DATA14,
	CHID_PSD_DATA15,
	CHID_BT_DUN,           /* 25 */
	CHID_CIQ_BRIDGE_DATA,
	CHID_27,
	CHID_CP_LOG1,
	CHID_CP_LOG2,
	CHID_30,               /* 30 */
	CHID_31,
	CHID_MAX
};

/* RFS IPC Frame */
struct rfs_hdr {
	u32 len;
	u8 cmd;
	u8 id;
} __attribute__ ((packed));

struct rfs_frag {
	struct rfs_hdr header;
	int len;
};

extern int sipc4_tx(struct sipc4_tx_data *tx_data);
extern int sipc4_rx(struct sipc4_rx_data *rx_data);
extern int pdp_netif_rx(struct net_device *parent_ndev, struct sk_buff *skb,
			int svnet_ch);
