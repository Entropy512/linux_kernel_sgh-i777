/*
 * Broadcom Dongle Host Driver (DHD), common DHD core.
 *
 * Copyright (C) 1999-2011, Broadcom Corporation
 * 
 *         Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_common.c,v 1.57.2.21.4.1 2011/02/01 19:36:23 Exp $
 */
#include <typedefs.h>
#include <osl.h>

#include <epivers.h>
#include <bcmutils.h>

#include <bcmendian.h>
#include <dngl_stats.h>
#include <wlioctl.h>
#include <dhd.h>

#include <proto/bcmevent.h>

#include <dhd_bus.h>
#include <dhd_proto.h>
#include <dhd_dbg.h>
#include <msgtrace.h>

#ifdef WLBTAMP
#include <proto/bt_amp_hci.h>
#include <dhd_bta.h>
#endif




#ifdef WLMEDIA_HTSF
extern void htsf_update(struct dhd_info *dhd, void *data);
#endif
int dhd_msg_level;


#include <wl_iw.h>

char fw_path[MOD_PARAM_PATHLEN];
char nv_path[MOD_PARAM_PATHLEN];

/* Last connection success/failure status */
uint32 dhd_conn_event;
uint32 dhd_conn_status;
uint32 dhd_conn_reason;

#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i

extern void dhd_ind_scan_confirm(void *h, bool status);
extern int dhd_iscan_in_progress(void *h);
void dhd_iscan_lock(void);
void dhd_iscan_unlock(void);
extern int dhd_change_mtu(dhd_pub_t *dhd, int new_mtu, int ifidx);

/* Packet alignment for most efficient SDIO (can change based on platform) */
#ifndef DHD_SDALIGN
#define DHD_SDALIGN	32
#endif
#if !ISPOWEROF2(DHD_SDALIGN)
#error DHD_SDALIGN is not a power of 2!
#endif

#ifdef DHD_DEBUG
const char dhd_version[] = "Dongle Host Driver, version " EPI_VERSION_STR "\nCompiled on "
	__DATE__ " at " __TIME__;
#else
const char dhd_version[] = "Dongle Host Driver, version " EPI_VERSION_STR;
#endif

void dhd_set_timer(void *bus, uint wdtick);

/* IOVar table */
enum {
	IOV_VERSION = 1,
	IOV_MSGLEVEL,
	IOV_BCMERRORSTR,
	IOV_BCMERROR,
	IOV_WDTICK,
	IOV_DUMP,
	IOV_CLEARCOUNTS,
	IOV_LOGDUMP,
	IOV_LOGCAL,
	IOV_LOGSTAMP,
	IOV_GPIOOB,
	IOV_IOCTLTIMEOUT,
#ifdef WLBTAMP
	IOV_HCI_CMD,						/* HCI command */
	IOV_HCI_ACL_DATA,					/* HCI data packet */
	IOV_SCB_AUTHORIZE,					/* scb_authorize */
	IOV_SCB_DEAUTHORIZE,				/* scb_deauthorize */
	IOV_SCB_DEAUTHENTICATE,				/* scb_deauth */
	IOV_SCB_DEAUTHENTICATE_FOR_REASON,	/* scb_deauth_for_reason */
	IOV_BSSID,							/* bssid */
#endif
#if defined(DHD_DEBUG)
	IOV_CONS,
	IOV_DCONSOLE_POLL,
#endif /* defined(DHD_DEBUG) */
	IOV_BUS_TYPE,
#ifdef WLMEDIA_HTSF
	IOV_WLPKTDLYSTAT_SZ,
#endif
	IOV_CHANGEMTU,
	IOV_LAST
};

const bcm_iovar_t dhd_iovars[] = {
	{"version", 				IOV_VERSION,						0,	IOVT_BUFFER,	sizeof(dhd_version) },
#ifdef DHD_DEBUG
	{"msglevel",				IOV_MSGLEVEL,						0,	IOVT_UINT32,	0 },
#endif /* DHD_DEBUG */
	{"bcmerrorstr", 			IOV_BCMERRORSTR,					0,	IOVT_BUFFER,	BCME_STRLEN },
	{"bcmerror",				IOV_BCMERROR,						0,	IOVT_INT8,		0 },
	{"wdtick",					IOV_WDTICK, 						0,	IOVT_UINT32,	0 },
	{"dump",					IOV_DUMP,							0,	IOVT_BUFFER,	DHD_IOCTL_MAXLEN },
#ifdef DHD_DEBUG
	{"cons",	IOV_CONS,	0,	IOVT_BUFFER,	0 },
	{"dconpoll",	IOV_DCONSOLE_POLL, 0,	IOVT_UINT32,	0 },
#endif
	{"clearcounts", 			IOV_CLEARCOUNTS,					0,	IOVT_VOID,		0 },
	{"gpioob",					IOV_GPIOOB,							0,	IOVT_UINT32,	0 },
	{"ioctl_timeout",			IOV_IOCTLTIMEOUT,					0,	IOVT_UINT32,	0 },
#ifdef WLBTAMP
	{"HCI_cmd",					IOV_HCI_CMD,						0,	IOVT_BUFFER,	0},
	{"HCI_ACL_data",			IOV_HCI_ACL_DATA,					0,	IOVT_BUFFER,	0},
	{"scb_authorize",			IOV_SCB_AUTHORIZE,					0,	IOVT_BUFFER,	0},
	{"scb_deauthorize", 		IOV_SCB_DEAUTHORIZE,				0,	IOVT_BUFFER,	0},
	{"scb_deauth", 				IOV_SCB_DEAUTHENTICATE,				0,	IOVT_BUFFER,	0},
	{"scb_deauth_for_reason",	IOV_SCB_DEAUTHENTICATE_FOR_REASON,	0,	IOVT_BUFFER,	0},
	{"bssid",					IOV_BSSID, 							0,	IOVT_BUFFER,	0},
#endif
	{"bustype", IOV_BUS_TYPE, 0, IOVT_UINT32, 0},
#ifdef WLMEDIA_HTSF
	{"pktdlystatsz", IOV_WLPKTDLYSTAT_SZ, 0, IOVT_UINT8, 0 },
#endif
	{"changemtu", IOV_CHANGEMTU, 0, IOVT_UINT32, 0 },
	{NULL, 0, 0, 0, 0 }
};

struct dhd_cmn *
dhd_common_init(osl_t *osh)
{
	dhd_cmn_t *cmn;

	/* Init global variables at run-time, not as part of the declaration.
	 * This is required to support init/de-init of the driver. Initialization
	 * of globals as part of the declaration results in non-deterministic
	 * behavior since the value of the globals may be different on the
	 * first time that the driver is initialized vs subsequent initializations.
	 */
	dhd_msg_level |= DHD_ERROR_VAL;
	/* Allocate private bus interface state */
	if (!(cmn = MALLOC(osh, sizeof(dhd_cmn_t)))) {
		DHD_ERROR(("%s: MALLOC failed\n", __FUNCTION__));
		return NULL;
	}
	memset(cmn, 0, sizeof(dhd_cmn_t));
	cmn->osh = osh;
	fw_path[0] = '\0';
	nv_path[0] = '\0';

	return cmn;
}

void
dhd_common_deinit(dhd_pub_t *dhd_pub)
{
	osl_t *osh;
	dhd_cmn_t *cmn = dhd_pub->cmn;

	if (!cmn)
		return;

	osh = cmn->osh;
	dhd_pub->cmn = NULL;
	MFREE(osh, cmn, sizeof(dhd_cmn_t));
}

static int
dhd_dump(dhd_pub_t *dhdp, char *buf, int buflen)
{
	char eabuf[ETHER_ADDR_STR_LEN];

	struct bcmstrbuf b;
	struct bcmstrbuf *strbuf = &b;

	bcm_binit(strbuf, buf, buflen);

	/* Base DHD info */
	bcm_bprintf(strbuf, "%s\n", dhd_version);
	bcm_bprintf(strbuf, "\n");
	bcm_bprintf(strbuf, "pub.up %d pub.txoff %d pub.busstate %d\n",
	            dhdp->up, dhdp->txoff, dhdp->busstate);
	bcm_bprintf(strbuf, "pub.hdrlen %d pub.maxctl %d pub.rxsz %d\n",
	            dhdp->hdrlen, dhdp->maxctl, dhdp->rxsz);
	bcm_bprintf(strbuf, "pub.iswl %d pub.drv_version %ld pub.mac %s\n",
	            dhdp->iswl, dhdp->drv_version, bcm_ether_ntoa(&dhdp->mac, eabuf));
	bcm_bprintf(strbuf, "pub.bcmerror %d tickcnt %d\n", dhdp->bcmerror, dhdp->tickcnt);

	bcm_bprintf(strbuf, "dongle stats:\n");
	bcm_bprintf(strbuf, "tx_packets %ld tx_bytes %ld tx_errors %ld tx_dropped %ld\n",
	            dhdp->dstats.tx_packets, dhdp->dstats.tx_bytes,
	            dhdp->dstats.tx_errors, dhdp->dstats.tx_dropped);
	bcm_bprintf(strbuf, "rx_packets %ld rx_bytes %ld rx_errors %ld rx_dropped %ld\n",
	            dhdp->dstats.rx_packets, dhdp->dstats.rx_bytes,
	            dhdp->dstats.rx_errors, dhdp->dstats.rx_dropped);
	bcm_bprintf(strbuf, "multicast %ld\n", dhdp->dstats.multicast);

	bcm_bprintf(strbuf, "bus stats:\n");
	bcm_bprintf(strbuf, "tx_packets %ld tx_multicast %ld tx_errors %ld\n",
	            dhdp->tx_packets, dhdp->tx_multicast, dhdp->tx_errors);
	bcm_bprintf(strbuf, "tx_ctlpkts %ld tx_ctlerrs %ld\n",
	            dhdp->tx_ctlpkts, dhdp->tx_ctlerrs);
	bcm_bprintf(strbuf, "rx_packets %ld rx_multicast %ld rx_errors %ld \n",
	            dhdp->rx_packets, dhdp->rx_multicast, dhdp->rx_errors);
	bcm_bprintf(strbuf, "rx_ctlpkts %ld rx_ctlerrs %ld rx_dropped %ld\n",
	            dhdp->rx_ctlpkts, dhdp->rx_ctlerrs, dhdp->rx_dropped);
	bcm_bprintf(strbuf, "rx_readahead_cnt %ld tx_realloc %ld\n",
	            dhdp->rx_readahead_cnt, dhdp->tx_realloc);
	bcm_bprintf(strbuf, "\n");

	/* Add any prot info */
	dhd_prot_dump(dhdp, strbuf);
	bcm_bprintf(strbuf, "\n");

	/* Add any bus info */
	dhd_bus_dump(dhdp, strbuf);

	return (!strbuf->size ? BCME_BUFTOOSHORT : 0);
}

int
dhd_wl_ioctl_cmd(dhd_pub_t *dhd_pub, int cmd, void *arg, int len, uint8 set, int ifindex)
{
	wl_ioctl_t ioc;

	ioc.cmd = cmd;
	ioc.buf = arg;
	ioc.len = len;
	ioc.set = set;

	return dhd_wl_ioctl(dhd_pub, ifindex, &ioc, arg, len);
}

int
dhd_wl_ioctl(dhd_pub_t *dhd_pub, int ifindex, wl_ioctl_t *ioc, void *buf, int len)
{
	int ret;

	dhd_os_proto_block(dhd_pub);

	ret = dhd_prot_ioctl(dhd_pub, ifindex, ioc, buf, len);


	dhd_os_proto_unblock(dhd_pub);
	return ret;
}

static int
dhd_doiovar(dhd_pub_t *dhd_pub, const bcm_iovar_t *vi, uint32 actionid, const char *name,
            void *params, int plen, void *arg, int len, int val_size)
{
	int bcmerror = 0;
	int32 int_val = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));
	DHD_TRACE(("%s: actionid = %d; name %s\n", __FUNCTION__, actionid, name));

	if ((bcmerror = bcm_iovar_lencheck(vi, arg, len, IOV_ISSET(actionid))) != 0)
		goto exit;

	if (plen >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	switch (actionid) {
	case IOV_GVAL(IOV_VERSION):
		/* Need to have checked buffer length */
		strncpy((char*)arg, dhd_version, len);
		break;

	case IOV_GVAL(IOV_MSGLEVEL):
		int_val = (int32)dhd_msg_level;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_MSGLEVEL):
		dhd_msg_level = int_val;
		break;

	case IOV_GVAL(IOV_BCMERRORSTR):
		strncpy((char *)arg, bcmerrorstr(dhd_pub->bcmerror), BCME_STRLEN);
		((char *)arg)[BCME_STRLEN - 1] = 0x00;
		break;

	case IOV_GVAL(IOV_BCMERROR):
		int_val = (int32)dhd_pub->bcmerror;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_GVAL(IOV_WDTICK):
		int_val = (int32)dhd_watchdog_ms;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_WDTICK):
		if (!dhd_pub->up) {
			bcmerror = BCME_NOTUP;
			break;
		}
		dhd_os_wd_timer(dhd_pub, (uint)int_val);
		break;

	case IOV_GVAL(IOV_DUMP):
		bcmerror = dhd_dump(dhd_pub, arg, len);
		break;

#ifdef DHD_DEBUG
	case IOV_GVAL(IOV_DCONSOLE_POLL):
		int_val = (int32)dhd_console_ms;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_DCONSOLE_POLL):
		dhd_console_ms = (uint)int_val;
		break;

	case IOV_SVAL(IOV_CONS):
		if (len > 0)
			bcmerror = dhd_bus_console_in(dhd_pub, arg, len - 1);
		break;
#endif /* DHD_DEBUG */

	case IOV_SVAL(IOV_CLEARCOUNTS):
		dhd_pub->tx_packets = dhd_pub->rx_packets = 0;
		dhd_pub->tx_errors = dhd_pub->rx_errors = 0;
		dhd_pub->tx_ctlpkts = dhd_pub->rx_ctlpkts = 0;
		dhd_pub->tx_ctlerrs = dhd_pub->rx_ctlerrs = 0;
		dhd_pub->rx_dropped = 0;
		dhd_pub->rx_readahead_cnt = 0;
		dhd_pub->tx_realloc = 0;
		dhd_pub->wd_dpc_sched = 0;
		memset(&dhd_pub->dstats, 0, sizeof(dhd_pub->dstats));
		dhd_bus_clearcounts(dhd_pub);
		break;


	case IOV_GVAL(IOV_IOCTLTIMEOUT): {
		int_val = (int32)dhd_os_get_ioctl_resp_timeout();
		bcopy(&int_val, arg, sizeof(int_val));
		break;
	}

	case IOV_SVAL(IOV_IOCTLTIMEOUT): {
		if (int_val <= 0)
			bcmerror = BCME_BADARG;
		else
			dhd_os_set_ioctl_resp_timeout((unsigned int)int_val);
		break;
	}

#ifdef WLBTAMP
	case IOV_SVAL(IOV_HCI_CMD): {
		amp_hci_cmd_t *cmd = (amp_hci_cmd_t *)arg;

		/* sanity check: command preamble present */
		if (len < HCI_CMD_PREAMBLE_SIZE)
			return BCME_BUFTOOSHORT;

		/* sanity check: command parameters are present */
		if (len < (int)(HCI_CMD_PREAMBLE_SIZE + cmd->plen))
			return BCME_BUFTOOSHORT;

		dhd_bta_docmd(dhd_pub, cmd, len);
		break;
	}

	case IOV_SVAL(IOV_HCI_ACL_DATA): {
		amp_hci_ACL_data_t *ACL_data = (amp_hci_ACL_data_t *)arg;

		/* sanity check: HCI header present */
		if (len < HCI_ACL_DATA_PREAMBLE_SIZE)
			return BCME_BUFTOOSHORT;

		/* sanity check: ACL data is present */
		if (len < (int)(HCI_ACL_DATA_PREAMBLE_SIZE + ACL_data->dlen))
			return BCME_BUFTOOSHORT;

		dhd_bta_tx_hcidata(dhd_pub, ACL_data, len);
		break;
	}

	case IOV_SVAL(IOV_SCB_AUTHORIZE):
	case IOV_SVAL(IOV_SCB_DEAUTHORIZE):
	case IOV_SVAL(IOV_SCB_DEAUTHENTICATE):
	case IOV_SVAL(IOV_SCB_DEAUTHENTICATE_FOR_REASON): {
		int cmd = 0;
		int buflen = 0;
		uint32 ifindex = 0;
		char *ea, *p;

		switch (actionid) {
		case IOV_SVAL(IOV_SCB_AUTHORIZE):
			cmd = WLC_SCB_AUTHORIZE;
			buflen = ETHER_ADDR_LEN;
			break;
		case IOV_SVAL(IOV_SCB_DEAUTHORIZE):
			cmd = WLC_SCB_DEAUTHORIZE;
			buflen = ETHER_ADDR_LEN;
			break;
		case IOV_SVAL(IOV_SCB_DEAUTHENTICATE):
			cmd = IOV_SCB_DEAUTHENTICATE;
			buflen = ETHER_ADDR_LEN;
			break;
		case IOV_SVAL(IOV_SCB_DEAUTHENTICATE_FOR_REASON):
			cmd = WLC_SCB_DEAUTHENTICATE_FOR_REASON;
			buflen = sizeof(scb_val_t);
			break;
		}
		/* parse out the if bsscfg index and ethernet address supplied */
		p = (char *)arg;

		memcpy(&ifindex, p, sizeof(ifindex));
		ea = p + sizeof(ifindex);

		bcmerror = dhd_wl_ioctl_cmd(dhd_pub, cmd, ea, buflen, TRUE, ifindex);
		break;
	}
	case IOV_GVAL(IOV_BSSID): {
		int cmd = WLC_GET_BSSID;
		int buflen = ETHER_ADDR_LEN;
		uint32 ifindex = 0;
		char *p = (char *)arg;

		p += strlen(p) + 1;
		memcpy(&ifindex, p, sizeof(ifindex));
		bcmerror = dhd_wl_ioctl_cmd(dhd_pub, cmd, arg, buflen, FALSE, ifindex);
		break;
	}
#endif /* WLBTAMP */

	case IOV_GVAL(IOV_BUS_TYPE):
	/* The dhd application query the driver to check if its usb or sdio.  */
#ifdef BCMDHDUSB
		int_val = BUS_TYPE_USB;
#endif
		int_val = BUS_TYPE_SDIO;
		bcopy(&int_val, arg, val_size);
		break;


#ifdef WLMEDIA_HTSF
	case IOV_GVAL(IOV_WLPKTDLYSTAT_SZ):
		int_val = dhd_pub->htsfdlystat_sz;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_WLPKTDLYSTAT_SZ):
		dhd_pub->htsfdlystat_sz = int_val & 0xff;
		printf("Setting tsfdlystat_sz:%d\n", dhd_pub->htsfdlystat_sz);
		break;
#endif
	case IOV_SVAL(IOV_CHANGEMTU):
		int_val &= 0xffff;
		bcmerror = dhd_change_mtu(dhd_pub, int_val, 0);
		break;

	default:
		bcmerror = BCME_UNSUPPORTED;
		break;
	}

exit:
	DHD_TRACE(("%s: actionid %d, bcmerror %d\n", __FUNCTION__, actionid, bcmerror));
	return bcmerror;
}

/* Store the status of a connection attempt for later retrieval by an iovar */
void
dhd_store_conn_status(uint32 event, uint32 status, uint32 reason)
{
	/* Do not overwrite a WLC_E_PRUNE with a WLC_E_SET_SSID
	 * because an encryption/rsn mismatch results in both events, and
	 * the important information is in the WLC_E_PRUNE.
	 */
	if (!(event == WLC_E_SET_SSID && status == WLC_E_STATUS_FAIL &&
	      dhd_conn_event == WLC_E_PRUNE)) {
		dhd_conn_event = event;
		dhd_conn_status = status;
		dhd_conn_reason = reason;
	}
}

bool
dhd_prec_enq(dhd_pub_t *dhdp, struct pktq *q, void *pkt, int prec)
{
	void *p;
	int eprec = -1;		/* precedence to evict from */
	bool discard_oldest;

	/* Fast case, precedence queue is not full and we are also not
	 * exceeding total queue length
	 */
	if (!pktq_pfull(q, prec) && !pktq_full(q)) {
		pktq_penq(q, prec, pkt);
		return TRUE;
	}

	/* Determine precedence from which to evict packet, if any */
	if (pktq_pfull(q, prec))
		eprec = prec;
	else if (pktq_full(q)) {
		p = pktq_peek_tail(q, &eprec);
		ASSERT(p);
		if (eprec > prec)
			return FALSE;
	}

	/* Evict if needed */
	if (eprec >= 0) {
		/* Detect queueing to unconfigured precedence */
		ASSERT(!pktq_pempty(q, eprec));
		discard_oldest = AC_BITMAP_TST(dhdp->wme_dp, eprec);
		if (eprec == prec && !discard_oldest)
			return FALSE;		/* refuse newer (incoming) packet */
		/* Evict packet according to discard policy */
		p = discard_oldest ? pktq_pdeq(q, eprec) : pktq_pdeq_tail(q, eprec);
		ASSERT(p);

		PKTFREE(dhdp->osh, p, TRUE);
	}

	/* Enqueue */
	p = pktq_penq(q, prec, pkt);
	ASSERT(p);

	return TRUE;
}

static int
dhd_iovar_op(dhd_pub_t *dhd_pub, const char *name,
	void *params, int plen, void *arg, int len, bool set)
{
	int bcmerror = 0;
	int val_size;
	const bcm_iovar_t *vi = NULL;
	uint32 actionid;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	ASSERT(name);
	ASSERT(len >= 0);

	/* Get MUST have return space */
	ASSERT(set || (arg && len));

	/* Set does NOT take qualifiers */
	ASSERT(!set || (!params && !plen));

	if ((vi = bcm_iovar_lookup(dhd_iovars, name)) == NULL) {
		bcmerror = BCME_UNSUPPORTED;
		goto exit;
	}

	DHD_CTL(("%s: %s %s, len %d plen %d\n", __FUNCTION__,
		name, (set ? "set" : "get"), len, plen));

	/* set up 'params' pointer in case this is a set command so that
	 * the convenience int and bool code can be common to set and get
	 */
	if (params == NULL) {
		params = arg;
		plen = len;
	}

	if (vi->type == IOVT_VOID)
		val_size = 0;
	else if (vi->type == IOVT_BUFFER)
		val_size = len;
	else
		/* all other types are integer sized */
		val_size = sizeof(int);

	actionid = set ? IOV_SVAL(vi->varid) : IOV_GVAL(vi->varid);

	bcmerror = dhd_doiovar(dhd_pub, vi, actionid, name, params, plen, arg, len, val_size);

exit:
	return bcmerror;
}

int
dhd_ioctl(dhd_pub_t * dhd_pub, dhd_ioctl_t *ioc, void * buf, uint buflen)
{
	int bcmerror = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (!buf) {
		return BCME_BADARG;
	}

	switch (ioc->cmd) {
	case DHD_GET_MAGIC:
		if (buflen < sizeof(int))
			bcmerror = BCME_BUFTOOSHORT;
		else
			*(int*)buf = DHD_IOCTL_MAGIC;
		break;

	case DHD_GET_VERSION:
		if (buflen < sizeof(int))
			bcmerror = -BCME_BUFTOOSHORT;
		else
			*(int*)buf = DHD_IOCTL_VERSION;
		break;

	case DHD_GET_VAR:
	case DHD_SET_VAR: {
		char *arg;
		uint arglen;

		/* scan past the name to any arguments */
		for (arg = buf, arglen = buflen; *arg && arglen; arg++, arglen--)
			;

		if (*arg) {
			bcmerror = BCME_BUFTOOSHORT;
			break;
		}

		/* account for the NUL terminator */
		arg++, arglen--;

		/* call with the appropriate arguments */
		if (ioc->cmd == DHD_GET_VAR)
			bcmerror = dhd_iovar_op(dhd_pub, buf, arg, arglen,
			buf, buflen, IOV_GET);
		else
			bcmerror = dhd_iovar_op(dhd_pub, buf, NULL, 0, arg, arglen, IOV_SET);
		if (bcmerror != BCME_UNSUPPORTED)
			break;

		/* not in generic table, try protocol module */
		if (ioc->cmd == DHD_GET_VAR)
			bcmerror = dhd_prot_iovar_op(dhd_pub, buf, arg,
				arglen, buf, buflen, IOV_GET);
		else
			bcmerror = dhd_prot_iovar_op(dhd_pub, buf,
				NULL, 0, arg, arglen, IOV_SET);
		if (bcmerror != BCME_UNSUPPORTED)
			break;

		/* if still not found, try bus module */
		if (ioc->cmd == DHD_GET_VAR) {
			bcmerror = dhd_bus_iovar_op(dhd_pub, buf,
				arg, arglen, buf, buflen, IOV_GET);
		} else {
			bcmerror = dhd_bus_iovar_op(dhd_pub, buf,
				NULL, 0, arg, arglen, IOV_SET);
		}

		break;
	}

	default:
		bcmerror = BCME_UNSUPPORTED;
	}

	return bcmerror;
}

#ifdef SHOW_EVENTS
static void
wl_show_host_event(wl_event_msg_t *event, void *event_data)
{
	uint i, status, reason;
	bool group = FALSE, flush_txq = FALSE, link = FALSE;
	const char *auth_str;
	const char *event_name;
	uchar *buf;
	char err_msg[256], eabuf[ETHER_ADDR_STR_LEN];
	uint event_type, flags, auth_type, datalen;

	event_type = ntoh32(event->event_type);
	flags = ntoh16(event->flags);
	status = ntoh32(event->status);
	reason = ntoh32(event->reason);
	auth_type = ntoh32(event->auth_type);
	datalen = ntoh32(event->datalen);

	/* debug dump of event messages */
	sprintf(eabuf, "%02x:%02x:%02x:%02x:%02x:%02x",
	        (uchar)event->addr.octet[0]&0xff,
	        (uchar)event->addr.octet[1]&0xff,
	        (uchar)event->addr.octet[2]&0xff,
	        (uchar)event->addr.octet[3]&0xff,
	        (uchar)event->addr.octet[4]&0xff,
	        (uchar)event->addr.octet[5]&0xff);

	event_name = "UNKNOWN";
	for (i = 0; i < (uint)bcmevent_names_size; i++)
		if (bcmevent_names[i].event == event_type)
			event_name = bcmevent_names[i].name;

	if (flags & WLC_EVENT_MSG_LINK)
		link = TRUE;
	if (flags & WLC_EVENT_MSG_GROUP)
		group = TRUE;
	if (flags & WLC_EVENT_MSG_FLUSHTXQ)
		flush_txq = TRUE;

	switch (event_type) {
	case WLC_E_START:
	case WLC_E_DEAUTH:
	case WLC_E_DISASSOC:
		DHD_EVENT(("MACEVENT: %s, MAC %s\n", event_name, eabuf));
		break;

	case WLC_E_ASSOC_IND:
	case WLC_E_REASSOC_IND:

		DHD_EVENT(("MACEVENT: %s, MAC %s\n", event_name, eabuf));
		break;

	case WLC_E_ASSOC:
	case WLC_E_REASSOC:
		if (status == WLC_E_STATUS_SUCCESS) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, SUCCESS\n", event_name, eabuf));
		} else if (status == WLC_E_STATUS_TIMEOUT) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, TIMEOUT\n", event_name, eabuf));
		} else if (status == WLC_E_STATUS_FAIL) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, FAILURE, reason %d\n",
			       event_name, eabuf, (int)reason));
		} else {
			DHD_EVENT(("MACEVENT: %s, MAC %s, unexpected status %d\n",
			       event_name, eabuf, (int)status));
		}
		break;

	case WLC_E_DEAUTH_IND:
	case WLC_E_DISASSOC_IND:
		DHD_EVENT(("MACEVENT: %s, MAC %s, reason %d\n", event_name, eabuf, (int)reason));
		break;

	case WLC_E_AUTH:
	case WLC_E_AUTH_IND:
		if (auth_type == DOT11_OPEN_SYSTEM)
			auth_str = "Open System";
		else if (auth_type == DOT11_SHARED_KEY)
			auth_str = "Shared Key";
		else {
			sprintf(err_msg, "AUTH unknown: %d", (int)auth_type);
			auth_str = err_msg;
		}
		if (event_type == WLC_E_AUTH_IND) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, %s\n", event_name, eabuf, auth_str));
		} else if (status == WLC_E_STATUS_SUCCESS) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, %s, SUCCESS\n",
				event_name, eabuf, auth_str));
		} else if (status == WLC_E_STATUS_TIMEOUT) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, %s, TIMEOUT\n",
				event_name, eabuf, auth_str));
		} else if (status == WLC_E_STATUS_FAIL) {
			DHD_EVENT(("MACEVENT: %s, MAC %s, %s, FAILURE, reason %d\n",
			       event_name, eabuf, auth_str, (int)reason));
		}

		break;

	case WLC_E_JOIN:
	case WLC_E_ROAM:
	case WLC_E_SET_SSID:
		if (status == WLC_E_STATUS_SUCCESS) {
			DHD_EVENT(("MACEVENT: %s, MAC %s\n", event_name, eabuf));
		} else if (status == WLC_E_STATUS_FAIL) {
			DHD_EVENT(("MACEVENT: %s, failed\n", event_name));
		} else if (status == WLC_E_STATUS_NO_NETWORKS) {
			DHD_EVENT(("MACEVENT: %s, no networks found\n", event_name));
		} else {
			DHD_EVENT(("MACEVENT: %s, unexpected status %d\n",
				event_name, (int)status));
		}
		break;

	case WLC_E_BEACON_RX:
		if (status == WLC_E_STATUS_SUCCESS) {
			DHD_EVENT(("MACEVENT: %s, SUCCESS\n", event_name));
		} else if (status == WLC_E_STATUS_FAIL) {
			DHD_EVENT(("MACEVENT: %s, FAIL\n", event_name));
		} else {
			DHD_EVENT(("MACEVENT: %s, status %d\n", event_name, status));
		}
		break;

	case WLC_E_LINK:
		DHD_EVENT(("MACEVENT: %s %s\n", event_name, link?"UP":"DOWN"));
		break;

	case WLC_E_MIC_ERROR:
		DHD_EVENT(("MACEVENT: %s, MAC %s, Group %d, Flush %d\n",
		       event_name, eabuf, group, flush_txq));
		break;

	case WLC_E_ICV_ERROR:
	case WLC_E_UNICAST_DECODE_ERROR:
	case WLC_E_MULTICAST_DECODE_ERROR:
		DHD_EVENT(("MACEVENT: %s, MAC %s\n",
		       event_name, eabuf));
		break;

	case WLC_E_TXFAIL:
		DHD_EVENT(("MACEVENT: %s, RA %s\n", event_name, eabuf));
		break;

	case WLC_E_SCAN_COMPLETE:
	case WLC_E_PMKID_CACHE:
		DHD_EVENT(("MACEVENT: %s\n", event_name));
		break;

	case WLC_E_PFN_NET_FOUND:
	case WLC_E_PFN_NET_LOST:
	case WLC_E_PFN_SCAN_COMPLETE:
		DHD_EVENT(("PNOEVENT: %s\n", event_name));
		break;

	case WLC_E_PSK_SUP:
	case WLC_E_PRUNE:
		DHD_EVENT(("MACEVENT: %s, status %d, reason %d\n",
		           event_name, (int)status, (int)reason));
		break;

#ifdef WIFI_ACT_FRAME
	case WLC_E_ACTION_FRAME:
		DHD_TRACE(("MACEVENT: %s Bssid %s\n", event_name, eabuf));
		break;
#endif /* WIFI_ACT_FRAME */

	case WLC_E_TRACE: {
			static uint32 seqnum_prev = 0;
			msgtrace_hdr_t hdr;
			uint32 nblost;
			char *s, *p;

			buf = (uchar *) event_data;
			memcpy(&hdr, buf, MSGTRACE_HDRLEN);

			if (hdr.version != MSGTRACE_VERSION) {
				printf("\nMACEVENT: %s [unsupported version --> "
				       "dhd version:%d dongle version:%d]\n",
				       event_name, MSGTRACE_VERSION, hdr.version);
				/* Reset datalen to avoid display below */
				datalen = 0;
				break;
			}

			/* There are 2 bytes available at the end of data */
			buf[MSGTRACE_HDRLEN + ntoh16(hdr.len)] = '\0';

			if (ntoh32(hdr.discarded_bytes) || ntoh32(hdr.discarded_printf)) {
				printf("\nWLC_E_TRACE: [Discarded traces in dongle -->"
				       "discarded_bytes %d discarded_printf %d]\n",
				       ntoh32(hdr.discarded_bytes), ntoh32(hdr.discarded_printf));
			}

			nblost = ntoh32(hdr.seqnum) - seqnum_prev - 1;
			if (nblost > 0) {
				printf("\nWLC_E_TRACE: [Event lost --> seqnum %d nblost %d\n",
				        ntoh32(hdr.seqnum), nblost);
			}
			seqnum_prev = ntoh32(hdr.seqnum);

			/* Display the trace buffer. Advance from \n to \n to avoid display big
			 * printf (issue with Linux printk )
			 */
			p = (char *)&buf[MSGTRACE_HDRLEN];
			while ((s = strstr(p, "\n")) != NULL) {
				*s = '\0';
				printf("FW: %s\n", p);
				p = s+1;
			}
			printf("FW: %s\n", p);

			/* Reset datalen to avoid display below */
			datalen = 0;
		break;
	}

#ifdef WLBTAMP
	case WLC_E_BTA_HCI_EVENT:
		DHD_EVENT(("MACEVENT: %s %d\n", event_name, ntoh32(*((int *)event_data))));
#ifdef DHD_BTAMP_DBG
		dhd_bta_hcidump_evt(NULL, event_data);
#endif
		break;
#endif

	case WLC_E_RSSI:
		DHD_EVENT(("MACEVENT: %s %d\n", event_name, ntoh32(*((int *)event_data))));
		break;

	default:
		DHD_EVENT(("MACEVENT: %s %d, MAC %s, status %d, reason %d, auth %d\n",
		       event_name, event_type, eabuf, (int)status, (int)reason,
		       (int)auth_type));
		break;
	}

	/* show any appended data */
	if (datalen) {
		buf = (uchar *) event_data;
		DHD_EVENT((" data (%d) : ", datalen));
		for (i = 0; i < datalen; i++)
			DHD_EVENT((" 0x%02x ", *buf++));
		DHD_EVENT(("\n"));
	}
}
#endif /* SHOW_EVENTS */

#ifdef SOFTAP
extern bool 	ap_fw_loaded;
#endif
#define P2P_INTERFACE_NAME "p2p"

int
wl_host_event(dhd_pub_t *dhd_pub, int *ifidx, void *pktdata,
              wl_event_msg_t *event, void **data_ptr)
{
	/* check whether packet is a BRCM event pkt */
	bcm_event_t *pvt_data = (bcm_event_t *)pktdata;
	char *event_data;
	uint32 type, status, reason, datalen;
	uint16 flags;
	int evlen;

	if (bcmp(BRCM_OUI, &pvt_data->bcm_hdr.oui[0], DOT11_OUI_LEN)) {
		DHD_ERROR(("%s: mismatched OUI, bailing\n", __FUNCTION__));
		return (BCME_ERROR);
	}

	/* BRCM event pkt may be unaligned - use xxx_ua to load user_subtype. */
	if (ntoh16_ua((void *)&pvt_data->bcm_hdr.usr_subtype) != BCMILCP_BCM_SUBTYPE_EVENT) {
		DHD_ERROR(("%s: mismatched subtype, bailing\n", __FUNCTION__));
		return (BCME_ERROR);
	}

	*data_ptr = &pvt_data[1];
	event_data = *data_ptr;

	/* memcpy since BRCM event pkt may be unaligned. */
	memcpy(event, &pvt_data->event, sizeof(wl_event_msg_t));

	type = ntoh32_ua((void *)&event->event_type);
	flags = ntoh16_ua((void *)&event->flags);
	status = ntoh32_ua((void *)&event->status);
	reason = ntoh32_ua((void *)&event->reason);
	datalen = ntoh32_ua((void *)&event->datalen);
	evlen = datalen + sizeof(bcm_event_t);

	switch (type) {

	case WLC_E_IF:
		{
			dhd_if_event_t *ifevent = (dhd_if_event_t *)event_data;
				
#ifdef SOFTAP
			if (ap_fw_loaded == FALSE) {
#endif			 	
                if (strncmp(pvt_data->event.ifname,"wl",2) == 0) {
                     memmove(&(pvt_data->event.ifname[1]),&(pvt_data->event.ifname[0]),6);
                     memcpy(pvt_data->event.ifname,P2P_INTERFACE_NAME,strlen(P2P_INTERFACE_NAME));
					 memcpy(event, &pvt_data->event, sizeof(wl_event_msg_t));
                }
#ifdef SOFTAP
			}
#endif

			if (ifevent->ifidx > 0 && ifevent->ifidx < DHD_MAX_IFS) {
				if (ifevent->action == WLC_E_IF_ADD)
					dhd_add_if(dhd_pub->info, ifevent->ifidx,
						NULL, event->ifname,
					    event->addr.octet,
						ifevent->flags, ifevent->bssidx);
				else
					dhd_del_if(dhd_pub->info, ifevent->ifidx);
			} else {
				DHD_ERROR(("%s: Invalid ifidx %d for %s\n",
					__FUNCTION__, ifevent->ifidx, event->ifname));
			}
		}
		/* send up the if event: btamp user needs it */
		*ifidx = dhd_ifname2idx(dhd_pub->info, event->ifname);
		/* push up to external supp/auth */
		dhd_event(dhd_pub->info, (char *)pvt_data, evlen, *ifidx);
		break;


#ifdef WLMEDIA_HTSF
	case WLC_E_HTSFSYNC:
		htsf_update(dhd_pub->info, event_data);
		break;
#endif /* WLMEDIA_HTSF */
	case WLC_E_NDIS_LINK: {
		uint32 temp = hton32(WLC_E_LINK);

		memcpy((void *)(&pvt_data->event.event_type), &temp,
		       sizeof(pvt_data->event.event_type));
	}
		/* These are what external supplicant/authenticator wants */
		/* fall through */
		case WLC_E_LINK:
		case WLC_E_DEAUTH:
		case WLC_E_DEAUTH_IND:
		case WLC_E_DISASSOC:
		case WLC_E_DISASSOC_IND:
			DHD_EVENT(("%s: Link event %d, flags %x, status %x\n",
			           __FUNCTION__, type, flags, status));
		/* fall through */
		default:
		*ifidx = dhd_ifname2idx(dhd_pub->info, event->ifname);
		/* push up to external supp/auth */
		dhd_event(dhd_pub->info, (char *)pvt_data, evlen, *ifidx);
		DHD_TRACE(("%s: MAC event %d, flags %x, status %x\n",
		           __FUNCTION__, type, flags, status));

		/* put it back to WLC_E_NDIS_LINK */
		if (type == WLC_E_NDIS_LINK) {
			uint32 temp;

			temp = ntoh32_ua((void *)&event->event_type);
			DHD_TRACE(("Converted to WLC_E_LINK type %d\n", temp));

			temp = ntoh32(WLC_E_NDIS_LINK);
			memcpy((void *)(&pvt_data->event.event_type), &temp,
			       sizeof(pvt_data->event.event_type));
		}
			break;
	}

#ifdef SHOW_EVENTS
	wl_show_host_event(event, (void *)event_data);
#endif /* SHOW_EVENTS */

	return (BCME_OK);
}

void
wl_event_to_host_order(wl_event_msg_t * evt)
{
	/* Event struct members passed from dongle to host are stored in network
	 * byte order. Convert all members to host-order.
	 */
	evt->event_type = ntoh32(evt->event_type);
	evt->flags = ntoh16(evt->flags);
	evt->status = ntoh32(evt->status);
	evt->reason = ntoh32(evt->reason);
	evt->auth_type = ntoh32(evt->auth_type);
	evt->datalen = ntoh32(evt->datalen);
	evt->version = ntoh16(evt->version);
}

void
dhd_print_buf(void *pbuf, int len, int bytes_per_line)
{
	int i, j = 0;
	unsigned char *buf = pbuf;

	if (bytes_per_line == 0) {
		bytes_per_line = len;
	}

	for (i = 0; i < len; i++) {
		printf("%2.2x", *buf++);
		j++;
		if (j == bytes_per_line) {
			printf("\n");
			j = 0;
		} else {
			printf(":");
		}
	}
	printf("\n");
}

#define strtoul(nptr, endptr, base) bcm_strtoul((nptr), (endptr), (base))

/* Convert user's input in hex pattern to byte-size mask */
static int
wl_pattern_atoh(char *src, char *dst)
{
	int i;
	if (strncmp(src, "0x", 2) != 0 &&
	    strncmp(src, "0X", 2) != 0) {
		DHD_ERROR(("Mask invalid format. Needs to start with 0x\n"));
		return -1;
	}
	src = src + 2; /* Skip past 0x */
	if (strlen(src) % 2 != 0) {
		DHD_ERROR(("Mask invalid format. Needs to be of even length\n"));
		return -1;
	}
	for (i = 0; *src != '\0'; i++) {
		char num[3];
		strncpy(num, src, 2);
		num[2] = '\0';
		dst[i] = (uint8)strtoul(num, NULL, 16);
		src += 2;
	}
	return i;
}

extern int dhd_preinit_ioctls(dhd_pub_t *dhd);
extern int dhdcdc_set_ioctl(dhd_pub_t *dhd, int ifidx, uint cmd, void *buf, uint len, uint8 action);

void
dhd_pktfilter_offload_enable(dhd_pub_t * dhd, char *arg, int enable, int master_mode)
{
	char				*argv[8];
	int					i = 0;
	const char 			*str;
	int					buf_len;
	int					str_len;
	char				*arg_save = 0, *arg_org = 0;
	int					rc;
	char				buf[128];
	wl_pkt_filter_enable_t	enable_parm;
	wl_pkt_filter_enable_t	* pkt_filterp;

	if (!(arg_save = MALLOC(dhd->osh, strlen(arg) + 1))) {
		DHD_ERROR(("%s: kmalloc failed\n", __FUNCTION__));
		goto fail;
	}
	arg_org = arg_save;
	memcpy(arg_save, arg, strlen(arg) + 1);

	argv[i] = bcmstrtok(&arg_save, " ", 0);

	i = 0;
	if (NULL == argv[i]) {
		DHD_ERROR(("No args provided\n"));
		goto fail;
	}

	str = "pkt_filter_enable";
	str_len = strlen(str);
	strncpy(buf, str, str_len);
	buf[str_len] = '\0';
	buf_len = str_len + 1;

	pkt_filterp = (wl_pkt_filter_enable_t *)(buf + str_len + 1);

	/* Parse packet filter id. */
	enable_parm.id = htod32(strtoul(argv[i], NULL, 0));

	/* Parse enable/disable value. */
	enable_parm.enable = htod32(enable);

	buf_len += sizeof(enable_parm);
	memcpy((char *)pkt_filterp,
	       &enable_parm,
	       sizeof(enable_parm));

	/* Enable/disable the specified filter. */
	rc = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, buf_len, TRUE, 0);
	rc = rc >= 0 ? 0 : rc;
	if (rc)
		DHD_TRACE(("%s: failed to add pktfilter %s, retcode = %d\n",
		__FUNCTION__, arg, rc));
	else
		DHD_TRACE(("%s: successfully added pktfilter %s\n",
		__FUNCTION__, arg));

	/* Contorl the master mode */
	bcm_mkiovar("pkt_filter_mode", (char *)&master_mode, 4, buf, sizeof(buf));
	rc = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, sizeof(buf), TRUE, 0);
	rc = rc >= 0 ? 0 : rc;
	if (rc)
		DHD_TRACE(("%s: failed to add pktfilter %s, retcode = %d\n",
		__FUNCTION__, arg, rc));

fail:
	if (arg_org)
		MFREE(dhd->osh, arg_org, strlen(arg) + 1);
}

void
dhd_pktfilter_offload_set(dhd_pub_t * dhd, char *arg)
{
	const char 			*str;
	wl_pkt_filter_t		pkt_filter;
	wl_pkt_filter_t		*pkt_filterp;
	int					buf_len;
	int					str_len;
	int 				rc;
	uint32				mask_size;
	uint32				pattern_size;
	char				*argv[8], * buf = 0;
	int					i = 0;
	char				*arg_save = 0, *arg_org = 0;
#define BUF_SIZE		2048

	if (!(arg_save = MALLOC(dhd->osh, strlen(arg) + 1))) {
		DHD_ERROR(("%s: kmalloc failed\n", __FUNCTION__));
		goto fail;
	}

	arg_org = arg_save;

	if (!(buf = MALLOC(dhd->osh, BUF_SIZE))) {
		DHD_ERROR(("%s: kmalloc failed\n", __FUNCTION__));
		goto fail;
	}

	memcpy(arg_save, arg, strlen(arg) + 1);

	if (strlen(arg) > BUF_SIZE) {
		DHD_ERROR(("Not enough buffer %d < %d\n", (int)strlen(arg), (int)sizeof(buf)));
		goto fail;
	}

	argv[i] = bcmstrtok(&arg_save, " ", 0);
	while (argv[i++])
		argv[i] = bcmstrtok(&arg_save, " ", 0);

	i = 0;
	if (NULL == argv[i]) {
		DHD_ERROR(("No args provided\n"));
		goto fail;
	}

	str = "pkt_filter_add";
	str_len = strlen(str);
	strncpy(buf, str, str_len);
	buf[ str_len ] = '\0';
	buf_len = str_len + 1;

	pkt_filterp = (wl_pkt_filter_t *) (buf + str_len + 1);

	/* Parse packet filter id. */
	pkt_filter.id = htod32(strtoul(argv[i], NULL, 0));

	if (NULL == argv[++i]) {
		DHD_ERROR(("Polarity not provided\n"));
		goto fail;
	}

	/* Parse filter polarity. */
	pkt_filter.negate_match = htod32(strtoul(argv[i], NULL, 0));

	if (NULL == argv[++i]) {
		DHD_ERROR(("Filter type not provided\n"));
		goto fail;
	}

	/* Parse filter type. */
	pkt_filter.type = htod32(strtoul(argv[i], NULL, 0));

	if (NULL == argv[++i]) {
		DHD_ERROR(("Offset not provided\n"));
		goto fail;
	}

	/* Parse pattern filter offset. */
	pkt_filter.u.pattern.offset = htod32(strtoul(argv[i], NULL, 0));

	if (NULL == argv[++i]) {
		DHD_ERROR(("Bitmask not provided\n"));
		goto fail;
	}

	/* Parse pattern filter mask. */
	mask_size =
		htod32(wl_pattern_atoh(argv[i], (char *) pkt_filterp->u.pattern.mask_and_pattern));

	if (NULL == argv[++i]) {
		DHD_ERROR(("Pattern not provided\n"));
		goto fail;
	}

	/* Parse pattern filter pattern. */
	pattern_size =
		htod32(wl_pattern_atoh(argv[i],
	         (char *) &pkt_filterp->u.pattern.mask_and_pattern[mask_size]));

	if (mask_size != pattern_size) {
		DHD_ERROR(("Mask and pattern not the same size\n"));
		goto fail;
	}

	pkt_filter.u.pattern.size_bytes = mask_size;
	buf_len += WL_PKT_FILTER_FIXED_LEN;
	buf_len += (WL_PKT_FILTER_PATTERN_FIXED_LEN + 2 * mask_size);

	/* Keep-alive attributes are set in local	variable (keep_alive_pkt), and
	** then memcpy'ed into buffer (keep_alive_pktp) since there is no
	** guarantee that the buffer is properly aligned.
	*/
	memcpy((char *)pkt_filterp,
	       &pkt_filter,
	       WL_PKT_FILTER_FIXED_LEN + WL_PKT_FILTER_PATTERN_FIXED_LEN);

	rc = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, buf_len, TRUE, 0);
	rc = rc >= 0 ? 0 : rc;

	if (rc)
		DHD_TRACE(("%s: failed to add pktfilter %s, retcode = %d\n",
		__FUNCTION__, arg, rc));
	else
		DHD_TRACE(("%s: successfully added pktfilter %s\n",
		__FUNCTION__, arg));

fail:
	if (arg_org)
		MFREE(dhd->osh, arg_org, strlen(arg) + 1);

	if (buf)
		MFREE(dhd->osh, buf, BUF_SIZE);
}

void
dhd_arp_offload_set(dhd_pub_t * dhd, int arp_mode)
{
	char iovbuf[32];
	int retcode;

	bcm_mkiovar("arp_ol", (char *)&arp_mode, 4, iovbuf, sizeof(iovbuf));
	retcode = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);
	retcode = retcode >= 0 ? 0 : retcode;
	if (retcode)
		DHD_TRACE(("%s: failed to set ARP offload mode to 0x%x, retcode = %d\n",
		__FUNCTION__, arp_mode, retcode));
	else
		DHD_TRACE(("%s: successfully set ARP offload mode to 0x%x\n",
		__FUNCTION__, arp_mode));
}

void
dhd_arp_offload_enable(dhd_pub_t * dhd, int arp_enable)
{
	char iovbuf[32];
	int retcode;

	bcm_mkiovar("arpoe", (char *)&arp_enable, 4, iovbuf, sizeof(iovbuf));
	retcode = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);
	retcode = retcode >= 0 ? 0 : retcode;
	if (retcode)
		DHD_TRACE(("%s: failed to enabe ARP offload to %d, retcode = %d\n",
		__FUNCTION__, arp_enable, retcode));
	else
		DHD_TRACE(("%s: successfully enabed ARP offload to %d\n",
		__FUNCTION__, arp_enable));
}

/* send up locally generated event */
void
dhd_sendup_event_common(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data)
{
	switch (ntoh32(event->event_type)) {
#ifdef WLBTAMP
	case WLC_E_BTA_HCI_EVENT:
		break;
#endif /* WLBTAMP */

	default:
		break;
	}

	/* Call per-port handler. */
	dhd_sendup_event(dhdp, event, data);
}



#ifdef SIMPLE_ISCAN

uint iscan_thread_id;
iscan_buf_t * iscan_chain = 0;

iscan_buf_t *
dhd_iscan_allocate_buf(dhd_pub_t *dhd, iscan_buf_t **iscanbuf)
{
	iscan_buf_t *iscanbuf_alloc = 0;
	iscan_buf_t *iscanbuf_head;

	DHD_TRACE(("%s: Entered\n", __FUNCTION__));
	dhd_iscan_lock();

	iscanbuf_alloc = (iscan_buf_t*)MALLOC(dhd->osh, sizeof(iscan_buf_t));
	if (iscanbuf_alloc == NULL)
		goto fail;

	iscanbuf_alloc->next = NULL;
	iscanbuf_head = *iscanbuf;

	DHD_ISCAN(("%s: addr of allocated node = 0x%X"
		   "addr of iscanbuf_head = 0x%X dhd = 0x%X\n",
		   __FUNCTION__, iscanbuf_alloc, iscanbuf_head, dhd));

	if (iscanbuf_head == NULL) {
		*iscanbuf = iscanbuf_alloc;
		DHD_ISCAN(("%s: Head is allocated\n", __FUNCTION__));
		goto fail;
	}

	while (iscanbuf_head->next)
		iscanbuf_head = iscanbuf_head->next;

	iscanbuf_head->next = iscanbuf_alloc;

fail:
	dhd_iscan_unlock();
	return iscanbuf_alloc;
}

void
dhd_iscan_free_buf(void *dhdp, iscan_buf_t *iscan_delete)
{
	iscan_buf_t *iscanbuf_free = 0;
	iscan_buf_t *iscanbuf_prv = 0;
	iscan_buf_t *iscanbuf_cur;
	dhd_pub_t *dhd = dhd_bus_pub(dhdp);
	DHD_TRACE(("%s: Entered\n", __FUNCTION__));

	dhd_iscan_lock();

	iscanbuf_cur = iscan_chain;

	/* If iscan_delete is null then delete the entire 
	 * chain or else delete specific one provided
	 */
	if (!iscan_delete) {
		while (iscanbuf_cur) {
			iscanbuf_free = iscanbuf_cur;
			iscanbuf_cur = iscanbuf_cur->next;
			iscanbuf_free->next = 0;
			MFREE(dhd->osh, iscanbuf_free, sizeof(iscan_buf_t));
		}
		iscan_chain = 0;
	} else {
		while (iscanbuf_cur) {
			if (iscanbuf_cur == iscan_delete)
				break;
			iscanbuf_prv = iscanbuf_cur;
			iscanbuf_cur = iscanbuf_cur->next;
		}
		if (iscanbuf_prv)
			iscanbuf_prv->next = iscan_delete->next;

		iscan_delete->next = 0;
		MFREE(dhd->osh, iscan_delete, sizeof(iscan_buf_t));

		if (!iscanbuf_prv)
			iscan_chain = 0;
	}
	dhd_iscan_unlock();
}

iscan_buf_t *
dhd_iscan_result_buf(void)
{
	return iscan_chain;
}

/*
* delete disappeared AP from specific scan cache
*/
int
dhd_iscan_delete_bss(/* TBD void *dhdp, */ void *addr)
{
	int i = 0, j = 0, l = 0;
	iscan_buf_t *iscan_cur;
	wl_iscan_results_t *list;
	wl_scan_results_t *results;
	wl_bss_info_t UNALIGNED *bi, *bi_new, *bi_next;

	uchar *s_addr = addr;
	DHD_TRACE(("%s: Entered\n", __FUNCTION__));

	dhd_iscan_lock();
	DHD_TRACE(("%s: BSS to remove %X:%X:%X:%X:%X:%X\n",
	__FUNCTION__, s_addr[0], s_addr[1], s_addr[2],
	s_addr[3], s_addr[4], s_addr[5]));

	DHD_TRACE(("%s: Scan cache before delete\n",
	__FUNCTION__));

	iscan_cur = dhd_iscan_result_buf();

	while (iscan_cur) {
			list = (wl_iscan_results_t *)iscan_cur->iscan_buf;
			if (!list)
				break;

			results = (wl_scan_results_t *)&list->results;
			if (!results)
				break;

			if (results->version != WL_BSS_INFO_VERSION) {
			DHD_ERROR(("%s: results->version %d != WL_BSS_INFO_VERSION\n",
				__FUNCTION__, results->version));
				goto done;
			}

			bi = results->bss_info;
			for (i = 0; i < results->count; i++) {
				if (!bi)
					break;

				if (!memcmp(bi->BSSID.octet, addr, ETHER_ADDR_LEN)) {
					DHD_TRACE(("%s: Del BSS[%2.2d:%2.2d] %X:%X:%X:%X:%X:%X\n",
					__FUNCTION__, l, i, bi->BSSID.octet[0], bi->BSSID.octet[1],
					bi->BSSID.octet[2], bi->BSSID.octet[3], bi->BSSID.octet[4],
					bi->BSSID.octet[5]));

					bi_new = bi;
					bi = (wl_bss_info_t *)((uintptr)bi + dtoh32(bi->length));


					for (j = i; j < results->count; j++) {
					DHD_TRACE(("%s: Moved up BSS[%2.2d:%2.2d]"
					" %X:%X:%X:%X:%X:%X\n",
					__FUNCTION__, l, j, bi->BSSID.octet[0], bi->BSSID.octet[1],
					bi->BSSID.octet[2], bi->BSSID.octet[3], bi->BSSID.octet[4],
					bi->BSSID.octet[5]));

					bi_next = (wl_bss_info_t *)((uintptr)bi +
						dtoh32(bi->length));
							bcopy(bi, bi_new, dtoh32(bi->length));
					bi_new = (wl_bss_info_t *)((uintptr)bi_new +
						dtoh32(bi_new->length));
							bi = bi_next;
						}
				results->count--;
					if (results->count == 0) {
						/* Prune now empty partial scan list */
						goto done;
					}
					break;
				}

				bi = (wl_bss_info_t *)((uintptr)bi + dtoh32(bi->length));
			}
		iscan_cur = iscan_cur->next;
		l++;
	}

done:
	DHD_TRACE(("%s: Scan cache after delete\n",
		__FUNCTION__));
	dhd_iscan_unlock();
	return 0;
}

int
dhd_iscan_request(void * dhdp, uint16 action)
{
	int rc;
	wl_iscan_params_t params;
	dhd_pub_t *dhd = dhd_bus_pub(dhdp);
	char buf[WLC_IOCTL_SMLEN];

	DHD_TRACE(("%s: Entered\n", __FUNCTION__));

	memset(&params, 0, sizeof(wl_iscan_params_t));
	memcpy(&params.params.bssid, &ether_bcast, ETHER_ADDR_LEN);

	params.params.bss_type = DOT11_BSSTYPE_ANY;
	params.params.scan_type = DOT11_SCANTYPE_ACTIVE;

	params.params.nprobes = htod32(-1);
	params.params.active_time = htod32(-1);
	params.params.passive_time = htod32(-1);
	params.params.home_time = htod32(-1);
	params.params.channel_num = htod32(0);

	params.version = htod32(ISCAN_REQ_VERSION);
	params.action = htod16(action);
	params.scan_duration = htod16(0);

	bcm_mkiovar("iscan", (char *)&params, sizeof(wl_iscan_params_t), buf, WLC_IOCTL_SMLEN);
	rc = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, WLC_IOCTL_SMLEN, TRUE, 0);

	return rc;
}

static int
dhd_iscan_get_partial_result(void *dhdp, uint *scan_count)
{
	wl_iscan_results_t *list_buf;
	wl_iscan_results_t list;
	wl_scan_results_t *results;
	iscan_buf_t *iscan_cur;
	int status = -1;
	dhd_pub_t *dhd = dhd_bus_pub(dhdp);
	int rc;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	iscan_cur = dhd_iscan_allocate_buf(dhd, &iscan_chain);
	if (!iscan_cur) {
		DHD_ERROR(("%s: Failed to allocate node\n", __FUNCTION__));
		dhd_iscan_free_buf(dhdp, 0);
		dhd_iscan_request(dhdp, WL_SCAN_ACTION_ABORT);
		dhd_ind_scan_confirm(dhdp, FALSE);
		goto fail;
	}

	dhd_iscan_lock();

	memset(iscan_cur->iscan_buf, 0, WLC_IW_ISCAN_MAXLEN);
	list_buf = (wl_iscan_results_t*)iscan_cur->iscan_buf;
	results = &list_buf->results;
	results->buflen = WL_ISCAN_RESULTS_FIXED_SIZE;
	results->version = 0;
	results->count = 0;

	memset(&list, 0, sizeof(list));
	list.results.buflen = htod32(WLC_IW_ISCAN_MAXLEN);
	bcm_mkiovar("iscanresults", (char *)&list, WL_ISCAN_RESULTS_FIXED_SIZE,
		iscan_cur->iscan_buf, WLC_IW_ISCAN_MAXLEN);
	rc = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, iscan_cur->iscan_buf,
	                      WLC_IW_ISCAN_MAXLEN, FALSE, 0);

	results->buflen = dtoh32(results->buflen);
	results->version = dtoh32(results->version);
	*scan_count = results->count = dtoh32(results->count);
	status = dtoh32(list_buf->status);
	DHD_TRACE(("%s: Got %d resuls\n", __FUNCTION__, results->count));

	dhd_iscan_unlock();

	if (!(*scan_count)) {
		dhd_iscan_free_buf(dhdp, iscan_cur);
	}
fail:
	return status;
}

#endif 

#ifdef PNO_SUPPORT
int dhd_pno_clean(dhd_pub_t *dhd)
{
	char iovbuf[128];
	int pfn_enabled = 0;
	int iov_len = 0;
	int ret;

	/* Disable pfn */
	iov_len = bcm_mkiovar("pfn", (char *)&pfn_enabled, 4, iovbuf, sizeof(iovbuf));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) >= 0) {
		/* clear pfn */
		iov_len = bcm_mkiovar("pfnclear", 0, 0, iovbuf, sizeof(iovbuf));
		if (iov_len) {
			if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf,
			                            iov_len, TRUE, 0)) < 0) {
				DHD_ERROR(("%s failed code %d\n", __FUNCTION__, ret));
			}
		}
		else {
			ret = -1;
			DHD_ERROR(("%s failed code %d\n", __FUNCTION__, iov_len));
		}
	}
	else
		DHD_ERROR(("%s failed code %d\n", __FUNCTION__, ret));

	return ret;
}

int dhd_pno_enable(dhd_pub_t *dhd, int pfn_enabled)
{
	char iovbuf[128];
	int ret = -1;

	if ((!dhd) && ((pfn_enabled != 0) || (pfn_enabled != 1))) {
		DHD_ERROR(("%s error exit\n", __FUNCTION__));
		return ret;
	}

	/* Enable/disable PNO */
	if ((ret = bcm_mkiovar("pfn", (char *)&pfn_enabled, 4, iovbuf, sizeof(iovbuf))) > 0) {
		if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf,
		                            sizeof(iovbuf), TRUE, 0)) < 0) {
			DHD_ERROR(("%s failed for error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		else {
			dhd->pno_enable = pfn_enabled;
			DHD_TRACE(("%s set pno as %d\n", __FUNCTION__, dhd->pno_enable));
		}
	}
	else DHD_ERROR(("%s failed err=%d\n", __FUNCTION__, ret));

	return ret;
}

/* Function to execute combined scan */
int
dhd_pno_set(dhd_pub_t *dhd, wlc_ssid_t* ssids_local, int nssid, uchar scan_fr)
{
	int err = -1;
	char iovbuf[128];
	int k, i;
	wl_pfn_param_t pfn_param;
	wl_pfn_t	pfn_element;

	DHD_TRACE(("%s nssid=%d nchan=%d\n", __FUNCTION__, nssid, scan_fr));

	if ((!dhd) && (!ssids_local)) {
		DHD_ERROR(("%s error exit\n", __FUNCTION__));
		err = -1;
	}

	/* Check for broadcast ssid */
	for (k = 0; k < nssid; k++) {
		if (!ssids_local[k].SSID_len) {
			DHD_ERROR(("%d: Broadcast SSID is ilegal for PNO setting\n", k));
			return err;
		}
	}
/* #define  PNO_DUMP 1 */
#ifdef PNO_DUMP
	{
		int j;
		for (j = 0; j < nssid; j++) {
			DHD_ERROR(("%d: scan  for  %s size =%d\n", j,
				ssids_local[j].SSID, ssids_local[j].SSID_len));
		}
	}
#endif /* PNO_DUMP */

	/* clean up everything */
	if  ((err = dhd_pno_clean(dhd)) < 0) {
		DHD_ERROR(("%s failed error=%d\n", __FUNCTION__, err));
		return err;
	}
	memset(&pfn_param, 0, sizeof(pfn_param));
	memset(&pfn_element, 0, sizeof(pfn_element));

	/* set pfn parameters */
	pfn_param.version = htod32(PFN_VERSION);
	pfn_param.flags = htod16((PFN_LIST_ORDER << SORT_CRITERIA_BIT));

	/* set up pno scan fr */
	if (scan_fr  != 0)
		pfn_param.scan_freq = htod32(scan_fr);

	bcm_mkiovar("pfn_set", (char *)&pfn_param, sizeof(pfn_param), iovbuf, sizeof(iovbuf));
	dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);

	/* set all pfn ssid */
	for (i = 0; i < nssid; i++) {

		pfn_element.bss_type = htod32(DOT11_BSSTYPE_INFRASTRUCTURE);
		pfn_element.auth = (DOT11_OPEN_SYSTEM);
		pfn_element.wpa_auth = htod32(WPA_AUTH_PFN_ANY);
		pfn_element.wsec = htod32(0);
		pfn_element.infra = htod32(1);

		memcpy((char *)pfn_element.ssid.SSID, ssids_local[i].SSID, ssids_local[i].SSID_len);
		pfn_element.ssid.SSID_len = ssids_local[i].SSID_len;

		if ((err =
		bcm_mkiovar("pfn_add", (char *)&pfn_element,
			sizeof(pfn_element), iovbuf, sizeof(iovbuf))) > 0) {
			if ((err =
			dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
				DHD_ERROR(("%s failed for i=%d error=%d\n",
					__FUNCTION__, i, err));
				return err;
			}
		}
		else DHD_ERROR(("%s failed err=%d\n", __FUNCTION__, err));
	}

	/* Enable PNO */
	/* dhd_pno_enable(dhd, 1); */
	return err;
}

int dhd_pno_get_status(dhd_pub_t *dhd)
{
	int ret = -1;

	if (!dhd)
		return ret;
	else
		return (dhd->pno_enable);
}

#endif /* PNO_SUPPORT */


/* Android ComboSCAN support */

/*
 *  data parsing from ComboScan tlv list
*/
int
wl_iw_parse_data_tlv(char** list_str, void *dst, int dst_size, const char token,
                     int input_size, int *bytes_left)
{
	char* str = *list_str;
	uint16 short_temp;
	uint32 int_temp;

	if ((list_str == NULL) || (*list_str == NULL) ||(bytes_left == NULL) || (*bytes_left < 0)) {
		DHD_ERROR(("%s error paramters\n", __FUNCTION__));
		return -1;
	}

	/* Clean all dest bytes */
	memset(dst, 0, dst_size);
	while (*bytes_left > 0) {

		if (str[0] != token) {
			DHD_TRACE(("%s NOT Type=%d get=%d left_parse=%d \n",
				__FUNCTION__, token, str[0], *bytes_left));
			return -1;
		}

		*bytes_left -= 1;
		str += 1;

		if (input_size == 1) {
			memcpy(dst, str, input_size);
		}
		else if (input_size == 2) {
			memcpy(dst, (char *)htod16(memcpy(&short_temp, str, input_size)),
				input_size);
		}
		else if (input_size == 4) {
			memcpy(dst, (char *)htod32(memcpy(&int_temp, str, input_size)),
				input_size);
		}

		*bytes_left -= input_size;
		str += input_size;
		*list_str = str;
		return 1;
	}
	return 1;
}

/*
 *  channel list parsing from cscan tlv list
*/
int
wl_iw_parse_channel_list_tlv(char** list_str, uint16* channel_list,
                             int channel_num, int *bytes_left)
{
	char* str = *list_str;
	int idx = 0;

	if ((list_str == NULL) || (*list_str == NULL) ||(bytes_left == NULL) || (*bytes_left < 0)) {
		DHD_ERROR(("%s error paramters\n", __FUNCTION__));
		return -1;
	}

	while (*bytes_left > 0) {

		if (str[0] != CSCAN_TLV_TYPE_CHANNEL_IE) {
			*list_str = str;
			DHD_TRACE(("End channel=%d left_parse=%d %d\n", idx, *bytes_left, str[0]));
			return idx;
		}
		/* Get proper CSCAN_TLV_TYPE_CHANNEL_IE */
		*bytes_left -= 1;
		str += 1;

		if (str[0] == 0) {
			/* All channels */
			channel_list[idx] = 0x0;
		}
		else {
			channel_list[idx] = (uint16)str[0];
			DHD_TRACE(("%s channel=%d \n", __FUNCTION__,  channel_list[idx]));
		}
		*bytes_left -= 1;
		str += 1;

		if (idx++ > 255) {
			DHD_ERROR(("%s Too many channels \n", __FUNCTION__));
			return -1;
		}
	}

	*list_str = str;
	return idx;
}

/*
 *  SSIDs list parsing from cscan tlv list
 */
int
wl_iw_parse_ssid_list_tlv(char** list_str, wlc_ssid_t* ssid, int max, int *bytes_left)
{
	char* str =  *list_str;
	int idx = 0;

	if ((list_str == NULL) || (*list_str == NULL) || (*bytes_left < 0)) {
		DHD_ERROR(("%s error paramters\n", __FUNCTION__));
		return -1;
	}

	while (*bytes_left > 0) {

		if (str[0] != CSCAN_TLV_TYPE_SSID_IE) {
			*list_str = str;
			DHD_TRACE(("nssid=%d left_parse=%d %d\n", idx, *bytes_left, str[0]));
			return idx;
		}

		/* Get proper CSCAN_TLV_TYPE_SSID_IE */
		*bytes_left -= 1;
		str += 1;

		if (str[0] == 0) {
			/* Broadcast SSID */
			ssid[idx].SSID_len = 0;
			memset((char*)ssid[idx].SSID, 0x0, DOT11_MAX_SSID_LEN);
			*bytes_left -= 1;
			str += 1;

			DHD_TRACE(("BROADCAST SCAN  left=%d\n", *bytes_left));
		}
		else if (str[0] <= DOT11_MAX_SSID_LEN) {
			/* Get proper SSID size */
			ssid[idx].SSID_len = str[0];
			*bytes_left -= 1;
			str += 1;

			/* Get SSID */
			if (ssid[idx].SSID_len > *bytes_left) {
				DHD_ERROR(("%s out of memory range len=%d but left=%d\n",
				__FUNCTION__, ssid[idx].SSID_len, *bytes_left));
				return -1;
			}

			memcpy((char*)ssid[idx].SSID, str, ssid[idx].SSID_len);

			*bytes_left -= ssid[idx].SSID_len;
			str += ssid[idx].SSID_len;

			DHD_TRACE(("%s :size=%d left=%d\n",
				(char*)ssid[idx].SSID, ssid[idx].SSID_len, *bytes_left));
		}
		else {
			DHD_ERROR(("### SSID size more that %d\n", str[0]));
			return -1;
		}

		if (idx++ >  max) {
			DHD_ERROR(("%s number of SSIDs more that %d\n", __FUNCTION__, idx));
			return -1;
		}
	}

	*list_str = str;
	return idx;
}

/* Parse a comma-separated list from list_str into ssid array, starting
 * at index idx.  Max specifies size of the ssid array.  Parses ssids
 * and returns updated idx; if idx >= max not all fit, the excess have
 * not been copied.  Returns -1 on empty string, or on ssid too long.
 */
int
wl_iw_parse_ssid_list(char** list_str, wlc_ssid_t* ssid, int idx, int max)
{
	char* str, *ptr;

	if ((list_str == NULL) || (*list_str == NULL))
		return -1;

	for (str = *list_str; str != NULL; str = ptr) {

		/* check for next TAG */
		if (!strncmp(str, GET_CHANNEL, strlen(GET_CHANNEL))) {
			*list_str	 = str + strlen(GET_CHANNEL);
			return idx;
		}

		if ((ptr = strchr(str, ',')) != NULL) {
			*ptr++ = '\0';
		}

		if (strlen(str) > DOT11_MAX_SSID_LEN) {
			DHD_ERROR(("ssid <%s> exceeds %d\n", str, DOT11_MAX_SSID_LEN));
			return -1;
		}

		if (strlen(str) == 0)
			ssid[idx].SSID_len = 0;

		if (idx < max) {
			strcpy((char*)ssid[idx].SSID, str);
			ssid[idx].SSID_len = strlen(str);
		}
		idx++;
	}
	return idx;
}

/*
 * Parse channel list from iwpriv CSCAN
 */
int
wl_iw_parse_channel_list(char** list_str, uint16* channel_list, int channel_num)
{
	int num;
	int val;
	char* str;
	char* endptr = NULL;

	if ((list_str == NULL)||(*list_str == NULL))
		return -1;

	str = *list_str;
	num = 0;
	while (strncmp(str, GET_NPROBE, strlen(GET_NPROBE))) {
		val = (int)strtoul(str, &endptr, 0);
		if (endptr == str) {
			printf("could not parse channel number starting at"
				" substring \"%s\" in list:\n%s\n",
				str, *list_str);
			return -1;
		}
		str = endptr + strspn(endptr, " ,");

		if (num == channel_num) {
			DHD_ERROR(("too many channels (more than %d) in channel list:\n%s\n",
				channel_num, *list_str));
			return -1;
		}

		channel_list[num++] = (uint16)val;
	}
	*list_str = str;
	return num;
}
