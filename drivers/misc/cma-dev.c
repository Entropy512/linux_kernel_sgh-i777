/*
 * Contiguous Memory Allocator userspace driver
 * Copyright (c) 2010 by Samsung Electronics.
 * Written by Michal Nazarewicz (m.nazarewicz@samsung.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License or (at your optional) any later version of the license.
 */

#define pr_fmt(fmt) "cma: " fmt

#ifdef CONFIG_CMA_DEBUG
#  define DEBUG
#endif

#include <linux/errno.h>       /* Error numbers */
#include <linux/err.h>         /* IS_ERR_VALUE() */
#include <linux/fs.h>          /* struct file */
#include <linux/mm.h>          /* Memory stuff */
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/module.h>      /* Standard module stuff */
#include <linux/device.h>      /* struct device, dev_dbg() */
#include <linux/types.h>       /* Just to be safe ;) */
#include <linux/uaccess.h>     /* __copy_{to,from}_user */
#include <linux/miscdevice.h>  /* misc_register() and company */

#include <linux/cma.h>

static int  cma_file_open(struct inode *inode, struct file *file);
static int  cma_file_release(struct inode *inode, struct file *file);
static long cma_file_ioctl(struct file *file, unsigned cmd, unsigned long arg);
static int  cma_file_mmap(struct file *file, struct vm_area_struct *vma);


static struct miscdevice cma_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "cma",
	.fops  = &(const struct file_operations) {
		.owner          = THIS_MODULE,
		.open           = cma_file_open,
		.release        = cma_file_release,
		.unlocked_ioctl = cma_file_ioctl,
		.mmap           = cma_file_mmap,
	},
};
#define cma_dev (cma_miscdev.this_device)


#define cma_file_start(file) (((dma_addr_t *)(file)->private_data)[0])
#define cma_file_size(file)  (((dma_addr_t *)(file)->private_data)[1])


static int  cma_file_open(struct inode *inode, struct file *file)
{
	dev_dbg(cma_dev, "%s(%p)\n", __func__, (void *)file);

	file->private_data = NULL;

	return 0;
}


static int  cma_file_release(struct inode *inode, struct file *file)
{
	dev_dbg(cma_dev, "%s(%p)\n", __func__, (void *)file);

	if (file->private_data) {
		cma_free(cma_file_start(file));
		kfree(file->private_data);
	}

	return 0;
}


static long cma_file_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	struct cma_alloc_request req;
	unsigned long addr;
	long ret;

	dev_dbg(cma_dev, "%s(%p)\n", __func__, (void *)file);

	if (cmd != IOCTL_CMA_ALLOC)
		return -ENOTTY;

	if (!arg)
		return -EINVAL;

	if (file->private_data) /* Already allocated */
		return -EBADFD;

	if (copy_from_user(&req, (void *)arg, sizeof req))
		return -EFAULT;

	if (req.magic != CMA_MAGIC)
		return -ENOTTY;

	if (req.type != CMA_REQ_DEV_KIND && req.type != CMA_REQ_FROM_REG)
		return -EINVAL;

	/* May happen on 32 bit system. */
	if (req.size > ~(typeof(req.size))0 ||
	    req.alignment > ~(typeof(req.alignment))0)
		return -EINVAL;

	if (strnlen(req.spec, sizeof req.spec) >= sizeof req.spec)
		return -EINVAL;


	file->private_data = kmalloc(2 * sizeof(dma_addr_t), GFP_KERNEL);
	if (!file->private_data)
		return -ENOSPC;


	if (req.type == CMA_REQ_DEV_KIND) {
		struct device fake_device;
		char *kind;

		fake_device.init_name = req.spec;
		fake_device.kobj.name = req.spec;

		kind = strrchr(req.spec, '/');
		if (kind)
			*kind++ = '\0';

		addr = cma_alloc(&fake_device, kind, req.size, req.alignment);
	} else {
		addr = cma_alloc_from(req.spec, req.size, req.alignment);
	}

	if (IS_ERR_VALUE(addr)) {
		ret = addr;
		goto error_priv;
	}


	if (put_user(addr, (typeof(req.start) *)(arg + offsetof(typeof(req),
								start)))) {
		ret = -EFAULT;
		goto error_put;
	}

	cma_file_start(file) = addr;
	cma_file_size(file) = req.size;

	dev_dbg(cma_dev, "allocated %p@%p\n",
		(void *)(dma_addr_t)req.size, (void *)addr);

	return 0;

error_put:
	cma_free(addr);
error_priv:
	kfree(file->private_data);
	file->private_data = NULL;
	return ret;
}


static int  cma_file_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long pgoff, offset, length;

	dev_dbg(cma_dev, "%s(%p)\n", __func__, (void *)file);

	if (!file->private_data)
		return -EBADFD;

	pgoff  = vma->vm_pgoff;
	offset = pgoff << PAGE_SHIFT;
	length = vma->vm_end - vma->vm_start;

	if (offset          >= cma_file_size(file)
	 || length          >  cma_file_size(file)
	 || offset + length >  cma_file_size(file))
		return -ENOSPC;

	return remap_pfn_range(vma, vma->vm_start,
			       __phys_to_pfn(cma_file_start(file) + offset),
			       length, vma->vm_page_prot);
}



static int __init cma_dev_init(void)
{
	int ret = misc_register(&cma_miscdev);
	pr_debug("miscdev: register returned: %d\n", ret);
	return ret;
}
module_init(cma_dev_init);

static void __exit cma_dev_exit(void)
{
	dev_dbg(cma_dev, "deregisterring\n");
	misc_deregister(&cma_miscdev);
}
module_exit(cma_dev_exit);
