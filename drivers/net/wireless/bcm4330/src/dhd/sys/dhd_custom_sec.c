#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>

#include <proto/ethernet.h>
#include <dngl_stats.h>
#include <bcmutils.h>
#include <dhd.h>
#include <dhd_dbg.h>

#include <linux/fcntl.h>
#include <linux/fs.h>

struct dhd_info;
extern int _dhd_set_mac_address(struct dhd_info *dhd, int ifidx, struct ether_addr *addr);


#ifdef READ_MACADDR

int
dhd_read_macaddr(struct dhd_info *dhd, struct ether_addr *mac)
{
    struct file *fp      = NULL;
    struct file *fpnv      = NULL;
    char macbuffer[18]   = {0};
    mm_segment_t oldfs   = {0};
	char randommac[3]    = {0};
	char buf[18]         = {0};
	char* filepath       = "/data/.mac.info";
#ifdef CONFIG_TARGET_LOCALE_VZW
    char* nvfilepath       = "/data/misc/wifi/.nvmac.info";
#else
    char* nvfilepath       = "/data/.nvmac.info";
#endif
	int ret = 0;

	//MAC address copied from nv
	fpnv = filp_open(nvfilepath, O_RDONLY, 0);
	if (IS_ERR(fpnv)) {
start_readmac:
		fpnv = NULL;
		fp = filp_open(filepath, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			/* File Doesn't Exist. Create and write mac addr.*/
			fp = filp_open(filepath, O_RDWR | O_CREAT, 0666);
			if(IS_ERR(fp)) {
				DHD_ERROR(("[WIFI] %s: File open error\n", filepath));
				return -1;
			}

			oldfs = get_fs();
			set_fs(get_ds());

			/* Generating the Random Bytes for 3 last octects of the MAC address */
			get_random_bytes(randommac, 3);
			
			sprintf(macbuffer,"%02X:%02X:%02X:%02X:%02X:%02X\n",
				0x60,0xd0,0xa9,randommac[0],randommac[1],randommac[2]);
			DHD_ERROR(("[WIFI] The Random Generated MAC ID : %s\n", macbuffer));

			if(fp->f_mode & FMODE_WRITE) {			
				ret = fp->f_op->write(fp, (const char *)macbuffer, sizeof(macbuffer), &fp->f_pos);
				if(ret < 0)
					DHD_ERROR(("[WIFI] Mac address [%s] Failed to write into File: %s\n", macbuffer, filepath));
				else
					DHD_INFO(("[WIFI] Mac address [%s] written into File: %s\n", macbuffer, filepath));
			}
			set_fs(oldfs);
		}
		/* Reading the MAC Address from .mac.info file( the existed file or just created file)*/
		//rtn_value=kernel_read(fp, fp->f_pos, buf, 18);	
		ret = kernel_read(fp, 0, buf, 18);	
	}
	else {
		/* Reading the MAC Address from .nvmac.info file( the existed file or just created file)*/
		ret = kernel_read(fpnv, 0, buf, 18);
		buf[17] ='\0';   // to prevent abnormal string display when mac address is displayed on the screen. 
		DHD_ERROR(("Read MAC : [%s] [%d] \r\n" , buf, strncmp(buf , "00:00:00:00:00:00" , 17)));
		if(strncmp(buf , "00:00:00:00:00:00" , 17) == 0) {
			filp_close(fpnv, NULL);
			goto start_readmac;
		}

		fp = filp_open(filepath, O_RDWR | O_CREAT, 0666); // File is always created.
		if(IS_ERR(fp)) {
			DHD_ERROR(("[WIFI] %s: File open error\n", filepath));
			if (fpnv)
				filp_close(fpnv, NULL);
			return -1;
		}
		else {
			oldfs = get_fs();
			set_fs(get_ds());
			
			if(fp->f_mode & FMODE_WRITE) {
				ret = fp->f_op->write(fp, (const char *)buf, sizeof(buf), &fp->f_pos);
				if(ret < 0)
					DHD_ERROR(("[WIFI] Mac address [%s] Failed to write into File: %s\n", buf, filepath));
				else
					DHD_INFO(("[WIFI] Mac address [%s] written into File: %s\n", buf, filepath));
			}       
			set_fs(oldfs);

			ret = kernel_read(fp, 0, buf, 18);	
		}

	}

	if(ret)
		sscanf(buf,"%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned int *)&(mac->octet[0]),
			(unsigned int *)&(mac->octet[1]),
			(unsigned int *)&(mac->octet[2]),
			(unsigned int *)&(mac->octet[3]),
			(unsigned int *)&(mac->octet[4]),
			(unsigned int *)&(mac->octet[5]));
	else
		DHD_ERROR(("dhd_bus_start: Reading from the '%s' returns 0 bytes\n", filepath));

	if (fp)
		filp_close(fp, NULL);
	if (fpnv)
		filp_close(fpnv, NULL);    	

	/* Writing Newly generated MAC ID to the Dongle */
	if (0 == _dhd_set_mac_address(dhd, 0, mac))
		DHD_INFO(("dhd_bus_start: MACID is overwritten\n"));
	else
		DHD_ERROR(("dhd_bus_start: _dhd_set_mac_address() failed\n"));

    return 0;
}

#endif /* READ_MACADDR */
#ifdef RDWR_MACADDR
static int g_iMacFlag;

enum {
	MACADDR_NONE =0 ,
	MACADDR_MOD,
	MACADDR_MOD_RANDOM,
	MACADDR_MOD_NONE,
	MACADDR_COB,
	MACADDR_COB_RANDOM
};

int WriteRDWR_Macaddr(struct ether_addr *mac)
{
	char* filepath			= "/data/.mac.info";
	struct file *fp_mac	= NULL;
	char buf[18]			= {0};
	mm_segment_t oldfs		= {0};
	int ret = -1;

	if ((g_iMacFlag != MACADDR_COB) && (g_iMacFlag != MACADDR_MOD))
		return 0;
	
	sprintf(buf,"%02X:%02X:%02X:%02X:%02X:%02X\n",
			mac->octet[0],mac->octet[1],mac->octet[2],
			mac->octet[3],mac->octet[4],mac->octet[5]);

	fp_mac = filp_open(filepath, O_RDWR | O_CREAT, 0666); // File is always created.
	if(IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath));
		return -1;
	}
	else {
		oldfs = get_fs();
		set_fs(get_ds());
		
		if(fp_mac->f_mode & FMODE_WRITE) {
			ret = fp_mac->f_op->write(fp_mac, (const char *)buf, sizeof(buf), &fp_mac->f_pos);
			if(ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s] Failed to write into File: %s\n", buf, filepath));
			else
				DHD_INFO(("[WIFI] Mac address [%s] written into File: %s\n", buf, filepath));
		}       
		set_fs(oldfs);
		filp_close(fp_mac, NULL);
	}

	return 0;
	
}

#if 0 /* disable because it's not used yet */
int ReadMacAddress_OTP(dhd_pub_t *dhdp, char cur_mac[])
{
	int ret = -1;

	dhd_os_proto_block(dhdp);
	strcpy(cur_mac, "cur_etheraddr");
	ret = dhdcdc_query_ioctl(dhdp, 0, WLC_GET_VAR, cur_mac, sizeof(cur_mac));
	if (ret < 0) {
		DHD_ERROR(("Current READ MAC error \r\n"));
		memset(cur_mac , 0 , ETHER_ADDR_LEN);
		return -1;
	}
	else {
		DHD_ERROR(("READ MAC (OTP) : [%02X][%02X][%02X][%02X][%02X][%02X] \r\n" , 
			cur_mac[0], cur_mac[1], cur_mac[2], cur_mac[3], cur_mac[4], cur_mac[5]));
	}
	dhd_os_proto_unblock(dhdp);

	return 0;
}
#endif

int CheckRDWR_Macaddr(	struct dhd_info *dhd, dhd_pub_t *dhdp, struct ether_addr *mac)
{
	struct file *fp_mac	= NULL;
	struct file *fp_nvm	= NULL;
	char macbuffer[18]		= {0};
	char randommac[3]		= {0};
	char buf[18]			= {0};
	char* filepath			= "/data/.mac.info";
	#ifdef CONFIG_TARGET_LOCALE_NA	
    char* nvfilepath       = "/data/misc/wifi/.nvmac.info";
	#else	
	char* nvfilepath		= "/data/.nvmac.info";
	#endif
	char cur_mac[128]		= {0};
	char dummy_mac[ETHER_ADDR_LEN]		= { 0x00, 0x90, 0x4C, 0xC5, 0x12, 0x38 };
	char cur_macbuffer[18]	= {0};
	int ret = -1;

	g_iMacFlag = MACADDR_NONE;	

	fp_nvm = filp_open(nvfilepath, O_RDONLY, 0);
	if(IS_ERR(fp_nvm)) { // file is not exist
		//read MAC Address;
		//ReadMacAddress_OTP(dhd,cur_mac);
		//dhd_os_proto_block(dhdp);
		strcpy(cur_mac, "cur_etheraddr");
		//ret = dhdcdc_query_ioctl(dhdp, 0, WLC_GET_VAR, cur_mac, sizeof(cur_mac));
		ret = dhd_wl_ioctl_cmd(dhdp, WLC_GET_VAR, cur_mac, sizeof(cur_mac), 0, 0);
		if (ret < 0) {
			DHD_ERROR(("Current READ MAC error \r\n"));
			memset(cur_mac , 0 , ETHER_ADDR_LEN);
			return -1;
		}
		else {
			DHD_ERROR(("MAC (OTP) : [%02X:%02X:%02X:%02X:%02X:%02X] \r\n" , 
			cur_mac[0], cur_mac[1], cur_mac[2], cur_mac[3], cur_mac[4], cur_mac[5]));
		}
		//dhd_os_proto_unblock(dhdp);
		
		sprintf(cur_macbuffer,"%02X:%02X:%02X:%02X:%02X:%02X\n",
			cur_mac[0],cur_mac[1],cur_mac[2],cur_mac[3],cur_mac[4],cur_mac[5]);

		fp_mac = filp_open(filepath, O_RDONLY, 0);
		if(IS_ERR(fp_mac)) { // file is not exist
		
			if(memcmp(cur_mac,dummy_mac,ETHER_ADDR_LEN) == 0) { // read mac is 00:90:4C:C5:12:38
				g_iMacFlag = MACADDR_MOD_RANDOM;
			}
			else if(strncmp(buf , "00:00:00:00:00:00" , 17) == 0) {
				g_iMacFlag = MACADDR_MOD_RANDOM;
			}
			else {
				g_iMacFlag = MACADDR_MOD;
			}
		}
		else {
			int is_zeromac;

			ret = kernel_read(fp_mac, 0, buf, 18);
			filp_close(fp_mac, NULL);
			buf[17] ='\0';

			is_zeromac = strncmp(buf, "00:00:00:00:00:00", 17);
			DHD_ERROR(("MAC (FILE): [%s] [%d] \r\n" , buf, is_zeromac));

			if (is_zeromac == 0) {
				DHD_ERROR(("Zero MAC detected. Trying Random MAC.\n"));
				g_iMacFlag = MACADDR_MOD_RANDOM;
			} else {
				sscanf(buf,"%02X:%02X:%02X:%02X:%02X:%02X",
					(unsigned int *)&(mac->octet[0]),
					(unsigned int *)&(mac->octet[1]),
					(unsigned int *)&(mac->octet[2]),
					(unsigned int *)&(mac->octet[3]),
					(unsigned int *)&(mac->octet[4]),
					(unsigned int *)&(mac->octet[5]));
				if(memcmp(cur_mac,mac->octet,ETHER_ADDR_LEN) == 0) { // read mac is same
					g_iMacFlag = MACADDR_NONE;
				}
				else { // change mac..
					if (0 == _dhd_set_mac_address(dhd, 0, mac)) {
						DHD_INFO(("dhd_bus_start: MACID is overwritten\n"));
						g_iMacFlag = MACADDR_MOD;
					}
					else {
						DHD_ERROR(("dhd_bus_start: _dhd_set_mac_address() failed\n"));
						g_iMacFlag = MACADDR_NONE;
					}
				}
			}
		}
	}
	else {
		// COB type. only COB.
		/* Reading the MAC Address from .nvmac.info file( the existed file or just created file)*/
		ret = kernel_read(fp_nvm, 0, buf, 18);
		buf[17] ='\0';   // to prevent abnormal string display when mac address is displayed on the screen. 
		DHD_ERROR(("Read MAC : [%s] [%d] \r\n" , buf, strncmp(buf , "00:00:00:00:00:00" , 17)));
		if(strncmp(buf , "00:00:00:00:00:00" , 17) == 0) {
			filp_close(fp_nvm, NULL);
			g_iMacFlag = MACADDR_COB_RANDOM;
		}
		else {
			sscanf(buf,"%02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned int *)&(mac->octet[0]),
				(unsigned int *)&(mac->octet[1]),
				(unsigned int *)&(mac->octet[2]),
				(unsigned int *)&(mac->octet[3]),
				(unsigned int *)&(mac->octet[4]),
				(unsigned int *)&(mac->octet[5]));
			/* Writing Newly generated MAC ID to the Dongle */
			if (0 == _dhd_set_mac_address(dhd, 0, mac)) {
				DHD_INFO(("dhd_bus_start: MACID is overwritten\n"));
				g_iMacFlag = MACADDR_COB;
			}
			else {
				DHD_ERROR(("dhd_bus_start: _dhd_set_mac_address() failed\n"));
			}
		}
		filp_close(fp_nvm, NULL);
	}
	
	if((g_iMacFlag == MACADDR_COB_RANDOM) || (g_iMacFlag == MACADDR_MOD_RANDOM)) {
		get_random_bytes(randommac, 3);
		sprintf(macbuffer,"%02X:%02X:%02X:%02X:%02X:%02X\n",
				0x60,0xd0,0xa9,randommac[0],randommac[1],randommac[2]);		
		DHD_ERROR(("[WIFI] The Random Generated MAC ID : %s\n", macbuffer));
		sscanf(macbuffer,"%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned int *)&(mac->octet[0]),
			(unsigned int *)&(mac->octet[1]),
			(unsigned int *)&(mac->octet[2]),
			(unsigned int *)&(mac->octet[3]),
			(unsigned int *)&(mac->octet[4]),
			(unsigned int *)&(mac->octet[5]));
		if (0 == _dhd_set_mac_address(dhd, 0, mac)) {
			DHD_INFO(("dhd_bus_start: MACID is overwritten\n"));
			g_iMacFlag = MACADDR_COB;
		}
		else {
			DHD_ERROR(("dhd_bus_start: _dhd_set_mac_address() failed\n"));
		}
	}

	
	return 0;
}


#endif

#ifdef USE_CID_CHECK

static int write_cid_file(const char *filepath, const char *buf, int buf_len)
{
	struct file *fp = NULL;
	mm_segment_t oldfs = {0};
	int ret = 0;

	fp = filp_open(filepath, O_RDWR | O_CREAT, 0666); // File is always created.
	if(IS_ERR(fp)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath));
		return -1;
	}
	else {
		oldfs = get_fs();
		set_fs(get_ds());
		
		if(fp->f_mode & FMODE_WRITE) {
			ret = fp->f_op->write(fp, buf, buf_len, &fp->f_pos);
			if(ret < 0)
				DHD_ERROR(("[WIFI] Failed to write CIS[%s] into '%s'\n", buf, filepath));
			else
				DHD_ERROR(("[WIFI] CID [%s] written into '%s'\n", buf, filepath));
		}		
		set_fs(oldfs);
	}
	filp_close(fp, NULL);

	return 0;
}

#ifdef DUMP_CIS
static void dump_cis(const unsigned char *buf, int size)
{
	int i;
	for(i=0; i<size; i++) {
		DHD_ERROR(("%02X ", buf[i]));
		if ((i%15) == 15) DHD_ERROR(("\n"));
	}
	DHD_ERROR(("\n"));
}
#endif

#define CIS_CID_OFFSET 31

int check_module_cid(dhd_pub_t *dhd)
{
	int ret = -1;
	unsigned char cis_buf[128] = {0};
	unsigned char cid_buf[10] = {0};
	const char* cidfilepath = "/data/.cid.info";

	/* Try reading out from CIS */
	cis_rw_t *cish = (cis_rw_t *)&cis_buf[8];
	struct file *fp_cid = NULL;

	fp_cid = filp_open(cidfilepath, O_RDONLY, 0);
	if (!IS_ERR(fp_cid)) { 
		kernel_read(fp_cid, fp_cid->f_pos, cid_buf, sizeof(cid_buf));
		if(strstr(cid_buf,"samsung")||strstr(cid_buf,"murata")) {
		/* file does exist, just return */
		filp_close(fp_cid, NULL);
		return 0;
	}

		DHD_ERROR(("[WIFI].cid.info file already exists but it contains an unknown id [%s]\n", cid_buf));
	}
	cish->source = 0;
	cish->byteoff = 0;
	cish->nbytes = sizeof(cis_buf);

	strcpy(cis_buf, "cisdump");
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, cis_buf, sizeof(cis_buf), 0, 0);
	if (ret < 0) {
		DHD_ERROR(("%s: CIS reading failed, err=%d\n", __FUNCTION__, ret));
	} else {
		unsigned char id[4] = {0x80, 0x06, 0x81, 0x00};
#ifdef DUMP_CIS
		dump_cis(cis_buf, 48);
#endif
		if (memcmp(&cis_buf[CIS_CID_OFFSET], id, 4) == 0) {
			DHD_ERROR(("CID MATCH FOUND\n"));
			write_cid_file(cidfilepath, "murata", 6);
		} else {
			DHD_ERROR(("CID MISMATCH 0x%02X 0x%02X 0x%02X 0x%02X\n", 
				cis_buf[CIS_CID_OFFSET], cis_buf[CIS_CID_OFFSET+1], 
				cis_buf[CIS_CID_OFFSET+2], cis_buf[CIS_CID_OFFSET+3]));
			write_cid_file(cidfilepath, "samsung", 7);
		}
	}

	return ret;
}
#endif

#ifdef WRITE_MACADDR
int Write_Macaddr(struct ether_addr *mac)
{
	char* filepath			= "/data/.mac.info";
	struct file *fp_mac	= NULL;
	char buf[18]			= {0};
	mm_segment_t oldfs		= {0};
	int ret = -1;
	int retry_count = 0;

startwrite:
	
	sprintf(buf,"%02X:%02X:%02X:%02X:%02X:%02X\n",
			mac->octet[0],mac->octet[1],mac->octet[2],
			mac->octet[3],mac->octet[4],mac->octet[5]);

	fp_mac = filp_open(filepath, O_RDWR | O_CREAT, 0666); // File is always created.

	if(IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath));
		return -1;
	}
	else {
		oldfs = get_fs();
		set_fs(get_ds());
		
		if(fp_mac->f_mode & FMODE_WRITE) {
			ret = fp_mac->f_op->write(fp_mac, (const char *)buf, sizeof(buf), &fp_mac->f_pos);
			if(ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s] Failed to write into File: %s\n", buf, filepath));
			else
				DHD_INFO(("[WIFI] Mac address [%s] written into File: %s\n", buf, filepath));
		}       
		set_fs(oldfs);
		filp_close(fp_mac, NULL);
	}
	/* check .mac.info file is 0 byte */
	fp_mac = filp_open(filepath, O_RDONLY, 0);
	ret = kernel_read(fp_mac, 0, buf, 18);

	if((ret == 0) && (retry_count++ < 3)){
		filp_close(fp_mac, NULL);
		goto startwrite;
	}	
	
	filp_close(fp_mac, NULL);

	return 0;
	
}
#endif

#ifdef CONFIG_CONTROL_PM
extern bool g_PMcontrol;
void sec_control_pm(dhd_pub_t *dhd, uint *power_mode)
{
	struct file *fp = NULL;
	char *filepath = "/data/.psm.info";
	mm_segment_t oldfs = {0};
	char power_val = 0;
	char iovbuf[WL_EVENTING_MASK_LEN + 12];
	
	g_PMcontrol = FALSE;

	fp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		/* Enable PowerSave Mode */
		dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)power_mode, sizeof(uint), TRUE, 0);

		fp = filp_open(filepath, O_RDWR | O_CREAT, 0666);
		if (IS_ERR(fp) || (fp==NULL)) {
			DHD_ERROR(("[%s, %d] /data/.psm.info open failed\n", __FUNCTION__, __LINE__));
			return;
		}
		else {
			oldfs = get_fs();
			set_fs(get_ds());

			if (fp->f_mode & FMODE_WRITE) {
				power_val = '1';
				fp->f_op->write(fp, (const char *)&power_val, sizeof(char), &fp->f_pos);
			}
			set_fs(oldfs);
		}
	}
	else {
		kernel_read(fp, fp->f_pos, &power_val, 1);
		DHD_ERROR(("POWER_VAL = %c \r\n" , power_val));

		if(power_val == '0') {
#ifdef ROAM_ENABLE
			uint roamvar = 1;
#endif
			*power_mode = PM_OFF;	
			/* Disable PowerSave Mode */
			dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)power_mode, sizeof(uint), TRUE, 0);
			/* Turn off MPC in AP mode */
			bcm_mkiovar("mpc", (char *)power_mode, 4, iovbuf, sizeof(iovbuf));
			dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);
			g_PMcontrol = TRUE;
#ifdef ROAM_ENABLE
			bcm_mkiovar("roam_off", (char *)&roamvar, 4, iovbuf, sizeof(iovbuf));
			dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);
#endif
		} else {
			dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)power_mode, sizeof(uint), TRUE, 0);
		}
	}

	if(fp)
		filp_close(fp, NULL);
}
#endif
