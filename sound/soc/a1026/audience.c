#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

//#include <mach/gpio-jupiter.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>

#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/freezer.h>

#include "audience.h"

//#define AUDIENCE_DEBUG
#define SUBJECT "audience.c"

#define audiencedebug_error(format,...)\
	printk ("["SUBJECT "(%d)] " format "\n", __LINE__, ## __VA_ARGS__);

#ifdef AUDIENCE_DEBUG
#define audiencedebug(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define audiencedebug(format,...)
#endif

int audience_probe(void);
int audience_bypass(u8 *p);
int audience_closetalk(u8 *p);
int audience_fartalk(u8 *p);
int audience_NS0(u8 *p);
int audience_state(void);
int factory_sub_mic_status(void);

static int audience_open(struct inode *inode, struct file *file);
static int audience_release(struct inode *inode, struct file *file);
static int audience_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int audience_write(struct file *filp, const char *buf, size_t count, loff_t *fpos);
static int audiencestate;
static int submicstatus = FACTORY_SUB_MIC_OFF;

enum AUDIENCEFLAGE {AUDIENCE_NCHANGE,AUDIENCE_CHANGE};

int bypass_tunningdata[200];  // 0x000x
enum AUDIENCEFLAGE bypass_tunningdata_flag=AUDIENCE_NCHANGE;
int bypass_num=0;

int closetalk_tunningdata[200]; // 0x100x
enum AUDIENCEFLAGE closetalk_tunningdata_flag=AUDIENCE_NCHANGE;
int closetalk_num=0;

int fartalk_tunningdata[200]; // 0x200x
enum AUDIENCEFLAGE fartalk_tunningdata_flag=AUDIENCE_NCHANGE;
int fartalk_num=0;

int NS0_tunningdata[200]; // 0x300x
enum AUDIENCEFLAGE NS0_tunningdata_flag=AUDIENCE_NCHANGE;
int NS0_num=0;

static int audience_open(struct inode *inode, struct file *file)
{
	audiencedebug("");
	return 0;
}
static int audience_release(struct inode *inode, struct file *file)
{
	audiencedebug("");
	return 0;
}
static int audience_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long state;
	audiencedebug("cmd=%d",cmd);
	switch(cmd)
	{
		case AUDIENCE_OFF:
			audiencestate=AUDIENCE_OFF;
			audiencedebug("AUDIENCE_BYPASS\n");
			break;
		case AUDIENCE_ON:
			audiencestate=AUDIENCE_ON;
			audiencedebug("AUDIENCE_ACTIVE\n");
			break;
		case AUDIENCE_STATE:
			printk("audience state %d\n",audiencestate);
			state=audiencestate;
			copy_to_user((void *)arg,&state,sizeof(state));
			break;
		case FACTORY_SUB_MIC_OFF:
			submicstatus = FACTORY_SUB_MIC_OFF;
			printk("submic status %d\n",submicstatus);
			break;
		case FACTORY_SUB_MIC_ON:
			submicstatus = FACTORY_SUB_MIC_ON;
			printk("submic status %d\n",submicstatus);
			break;

		default:
			break;
	}

	return 0;
}

static int audience_write(struct file *filp, const char *buf, size_t count, loff_t *fpos)
{
	int data[200]={0};
	int i=0,ret=0;

	audiencedebug("count=%d",count);
	if(count<=1) return -1;
	ret=copy_from_user(data,buf,count);
	if(ret!=0) return ret;

	if(data[0]==0)
	{
		memset(bypass_tunningdata,0,sizeof(bypass_tunningdata));
		memcpy(bypass_tunningdata,&data[1],count);
		bypass_tunningdata_flag=AUDIENCE_CHANGE;
		bypass_num=(count-1)/4;

#if 0
		for(i=0;i<bypass_num;i++) printk("0x%x ",bypass_tunningdata[i]);
		printk("\n");

		printk("bypass flag=%d, num=%d\n",bypass_tunningdata_flag,bypass_num);
#endif
	}
	else if(data[0]==1)
	{
		memset(closetalk_tunningdata,0,sizeof(closetalk_tunningdata));
		memcpy(closetalk_tunningdata,&data[1],count);
		closetalk_tunningdata_flag=AUDIENCE_CHANGE;
		closetalk_num=(count-1)/4;

#if 0
		for(i=0;i<closetalk_num;i++) printk("0x%x ",closetalk_tunningdata[i]);
		printk("\n");

		printk("closetalk flag=%d, num=%d\n",closetalk_tunningdata_flag,closetalk_num);
#endif
	}
	else if(data[0]==2)
	{
		memset(fartalk_tunningdata,0,sizeof(fartalk_tunningdata));
		memcpy(fartalk_tunningdata,&data[1],count);
		fartalk_tunningdata_flag=AUDIENCE_CHANGE;
		fartalk_num=(count-1)/4;

#if 0
		for(i=0;i<fartalk_num;i++) printk("0x%x ",fartalk_tunningdata[i]);
		printk("\n");

		printk("fartalk flag=%d, num=%d\n",fartalk_tunningdata_flag,fartalk_num);
#endif
	}
	else if(data[0]==3)
	{
		memset(NS0_tunningdata,0,sizeof(NS0_tunningdata));
		memcpy(NS0_tunningdata,&data[1],count);
		NS0_tunningdata_flag=AUDIENCE_CHANGE;
		NS0_num=(count-1)/4;

#if 0
		for(i=0;i<NS0_num;i++) printk("0x%x ",NS0_tunningdata[i]);
		printk("\n");

		printk("NS0 flag=%d, num=%d\n",NS0_tunningdata_flag,NS0_num);
#endif
	}
	return 0;
}


int audience_bypass(u8 *p)
{
	audiencedebug("");
	u8 bypass_tunningdata_tmp[400];
	int i, idx;

	if(!bypass_tunningdata_flag) return 0;
		bypass_tunningdata_flag=AUDIENCE_NCHANGE;
	for(idx=0; idx<bypass_num; idx++)
	{
			bypass_tunningdata_tmp[(idx*2)] = bypass_tunningdata[idx] >> 8;
			bypass_tunningdata_tmp[(idx*2)+1] = bypass_tunningdata[idx] & 0x00ff;

	}

//	for(i=0;i<idx*2;i++) printk("0x%x ",bypass_tunningdata_tmp[i]);

	memcpy(p,bypass_tunningdata_tmp,idx*2);
	return idx*2;
}

int audience_closetalk(u8 *p)
{
	audiencedebug("");

	u8 closetalk_tunningdata_tmp[400];
	int i, idx;

	if(!closetalk_tunningdata_flag) return 0;
		closetalk_tunningdata_flag=AUDIENCE_NCHANGE;

	for(idx=0; idx<closetalk_num; idx++)
	{
			closetalk_tunningdata_tmp[(idx*2)] = closetalk_tunningdata[idx] >> 8;
			closetalk_tunningdata_tmp[(idx*2)+1] = closetalk_tunningdata[idx] & 0x00ff;

	}

//	for(i=0;i<idx*2;i++) printk("0x%x ",closetalk_tunningdata_tmp[i]);

	memcpy(p,closetalk_tunningdata_tmp,idx*2);
	return idx*2;
}

int audience_fartalk(u8 *p)
{
	audiencedebug("");


	u8 fartalk_tunningdata_tmp[400];
	int i, idx;

	if(!fartalk_tunningdata_flag) return 0;
		fartalk_tunningdata_flag=AUDIENCE_NCHANGE;

	for(idx=0; idx<fartalk_num; idx++)
	{
			fartalk_tunningdata_tmp[(idx*2)] = fartalk_tunningdata[idx] >> 8;
			fartalk_tunningdata_tmp[(idx*2)+1] = fartalk_tunningdata[idx] & 0x00ff;

	}

//	for(i=0;i<idx*2;i++) printk("0x%x ",fartalk_tunningdata_tmp[i]);

	memcpy(p,fartalk_tunningdata_tmp,idx*2);
	return idx*2;

}

int audience_NS0(u8 *p)
{
	audiencedebug("");

	u8 NS0_tunningdata_tmp[400];
	int i, idx;

	if(!NS0_tunningdata_flag) return 0;
		NS0_tunningdata_flag=AUDIENCE_NCHANGE;

	for(idx=0; idx<NS0_num; idx++)
	{
			NS0_tunningdata_tmp[(idx*2)] = NS0_tunningdata[idx] >> 8;
			NS0_tunningdata_tmp[(idx*2)+1] = NS0_tunningdata[idx] & 0x00ff;

	}

//	for(i=0;i<idx*2;i++) printk("0x%x ",NS0_tunningdata_tmp[i]);

	memcpy(p,NS0_tunningdata_tmp,idx*2);
	return idx*2;
}

int audience_state(void)
{
	return audiencestate;
}
int factory_sub_mic_status(void)
{
	return submicstatus;
}
static struct file_operations audience_fops = {
	.owner = THIS_MODULE,
	.open = audience_open,
	.ioctl = audience_ioctl,
	.release = audience_release,
	.write = audience_write,
};

static struct miscdevice audience_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "audience_miscdev",
	.fops = &audience_fops,
};

int audience_probe(void)
{
	int ret=0;
	audiencedebug("");

	ret = misc_register(&audience_miscdev);
	if(ret)
	{
		printk("audience misc device register failed\n");
	}

	return ret;
}




