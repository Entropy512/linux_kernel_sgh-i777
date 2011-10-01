/*
*    gib-dev.c - GPS Interface Block[GIB] driver, char device interface
*
*	Copyright (C) 2010 Samsung Electronics Co. Ltd.
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include "gib-dev.h"
#include <linux/uaccess.h>

#undef debug
#define debug
#ifdef debug
#define DBG(x...)       printk(x)
#define CRDEBUG	printk("%s :: %d\n",__FUNCTION__,__LINE__)
#else
#define CRDEBUG
#define DBG(x...)       do { } while (0)
#endif

#define GIB_MINORS	0

static struct gib_dev *gib_dev_array[GIB_MINORS];

static struct gib_dev *attach_to_gib_dev_array(struct gib_dev *gib_dev)
{
	CRDEBUG;

	if (gib_dev_array[gib_dev->minor]) {
		dev_err(&gib_dev->dev, "gib-dev already has a device assigned to this adapter\n");
		goto error;
	}
	
	gib_dev_array[gib_dev->minor] = gib_dev;

	return gib_dev;
error:
	return ERR_PTR(-ENODEV);
}

static void return_gib_dev(struct gib_dev *gib_dev)
{
	CRDEBUG;
        printk("%s\n",__func__);
	gib_dev_array[gib_dev->minor] = NULL;
}

int gib_attach_gibdev(struct gib_dev *gibdev)
{
	struct gib_dev *gib_dev;
	
	CRDEBUG;
	gib_dev = attach_to_gib_dev_array(gibdev);
	if (IS_ERR(gib_dev))
		return PTR_ERR(gib_dev);

	dev_dbg(&gib_dev->dev, "Registered as minor %d\n", gib_dev->minor);

	return 0;
}

int gib_detach_gibdev(struct gib_dev *gib_dev)
{
	CRDEBUG;

	return_gib_dev(gib_dev);

	dev_dbg(&gib_dev->dev, "Adapter unregistered\n");
	return 0;
}

struct gib_dev *gib_dev_get_by_minor(unsigned index)
{
	struct gib_dev *gib_dev;

	CRDEBUG;
	gib_dev = gib_dev_array[index];

	return gib_dev;
}



int gibdev_ioctl (struct file *file, unsigned int cmd,
                  unsigned long arg)
{
	struct gib_dev *gib_dev = (struct gib_dev *)file->private_data;

	CRDEBUG;

	switch ( cmd ) {
		case SET_BB_RESET_LOW:
			gib_dev->g_reset->core_reset(0);
                        return 1;
			break;
		case SET_BB_RESET_HIGH:
			gib_dev->g_reset->core_reset(1);
                        return 1;
			break;
		case SET_UBP_DEBUG_MODE:
			gib_dev->ubp_debug_flag = 1;
			gib_dev->g_udp_debug->ubp_debug(gib_dev->ubp_debug_flag);
			break;
		case SET_UBP_NO_DEBUG_MODE:
			gib_dev->ubp_debug_flag =  0;
			gib_dev->g_udp_debug->ubp_debug(gib_dev->ubp_debug_flag);
			break;
		default:
			printk(KERN_WARNING "Invalid ioctl option\n");
			break;
	}
	return 0;
}
static int gibdev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct gib_dev *gib_dev;

	CRDEBUG;

	gib_dev = gib_dev_get_by_minor(minor);
	if (!gib_dev)
		return -ENODEV;

	/* registered with adapter, passed as client to user */
	file->private_data = gib_dev;

	return 0;
}

int gib_master_close(struct gib_dev *gib_dev)
{
	CRDEBUG;
	return 0;
}

static int gibdev_release(struct inode *inode, struct file *file)
{
	struct gib_dev *gib_dev = (struct gib_dev *)file->private_data;
	int ret;
	CRDEBUG;

	ret = gib_master_close(gib_dev);
	file->private_data = NULL;

	return 0;
}

static struct file_operations gibdev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.open		= gibdev_open,
	.ioctl		= gibdev_ioctl,
	.release	= gibdev_release,
};

static int gib_dev_init(void)
{
	int res;

	CRDEBUG;
	DBG("GPS Interface Block(GIB) entries driver\n");

	res = register_chrdev(GIB_MAJOR, "gib", &gibdev_fops);
	if (res)
		goto out;

	return 0;
out:
	printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);
	return res;
}

static void gib_dev_exit(void)
{
	CRDEBUG;
	unregister_chrdev(GIB_MAJOR,"gib");
}

MODULE_AUTHOR("Taeyong Lee");
MODULE_DESCRIPTION("GPS Interface Block(GIB) entries driver");
MODULE_LICENSE("GPL");

module_init(gib_dev_init);
module_exit(gib_dev_exit);

EXPORT_SYMBOL(gib_attach_gibdev);
EXPORT_SYMBOL(gib_detach_gibdev);
