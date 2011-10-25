/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_dev.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Driver interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include <linux/sched.h>
#include <linux/firmware.h>
#ifdef CONFIG_CPU_FREQ
#include <mach/cpufreq.h>
#endif
#include <mach/regs-pmu.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "mfc_dev.h"
#include "mfc_interface.h"
#include "mfc_reg.h"
#include "mfc_log.h"
#include "mfc_ctrl.h"
#include "mfc_buf.h"
#include "mfc_inst.h"
#include "mfc_pm.h"
#include "mfc_dec.h"
#include "mfc_enc.h"
#include "mfc_mem.h"
#include "mfc_cmd.h"

#ifdef SYSMMU_MFC_ON
#include <plat/sysmmu.h>
#endif

#ifdef CONFIG_PM_RUNTIME
#include <linux/clk.h>
#endif

#define MFC_MINOR	252
#define MFC_FW_NAME	"mfc_fw.bin"

static struct mfc_dev *mfcdev;

static int get_free_inst_id(struct mfc_dev *dev)
{
	int slot = 0;

	while (dev->inst_ctx[slot]) {
		slot++;
		if (slot >= MFC_MAX_INSTANCE_NUM)
			return -1;
	}

	return slot;
}

static int mfc_open(struct inode *inode, struct file *file)
{
	struct mfc_inst_ctx *mfc_ctx;
	int ret;
	enum mfc_ret_code retcode;

	/* prevent invalid reference */
	file->private_data = NULL;

	mutex_lock(&mfcdev->lock);

	if (atomic_read(&mfcdev->inst_cnt) == 0) {
		/* reload F/W for first instance again */
		mfcdev->fw.state = mfc_load_firmware(mfcdev->fw.info->data, mfcdev->fw.info->size);
		if (!mfcdev->fw.state) {
			mfc_err("MFC F/W not load yet\n");
			ret = -ENODEV;
			goto err_fw_state;
		}
		printk(KERN_INFO "MFC F/W reloaded for first Instance successfully (size: %d)\n", mfcdev->fw.info->size);

		ret = mfc_power_on();
		if (ret < 0) {
			mfc_err("power enable failed\n");
			goto err_pwr_enable;
		}

#ifndef CONFIG_PM_RUNTIME
#ifdef SYSMMU_MFC_ON
		mfc_clock_on();

		sysmmu_on(SYSMMU_MFC_L);
		sysmmu_on(SYSMMU_MFC_R);

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
		vcm_set_pgtable_base(VCM_DEV_MFC);
#else /* CONFIG_S5P_VMEM or kernel virtual memory allocator */
		sysmmu_set_tablebase_pgd(SYSMMU_MFC_L, __pa(swapper_pg_dir));
		sysmmu_set_tablebase_pgd(SYSMMU_MFC_R, __pa(swapper_pg_dir));

		/*
		 * RMVME: the power-gating work really (on <-> off),
		 * all TBL entry was invalidated already when the power off
		 */
		sysmmu_tlb_invalidate(SYSMMU_MFC_L);
		sysmmu_tlb_invalidate(SYSMMU_MFC_R);
#endif
		mfc_clock_off();
#endif
#endif
		/* MFC hardware initialization */
		retcode = mfc_start(mfcdev);
		if (retcode != MFC_OK) {
			mfc_err("MFC H/W init failed: %d\n", retcode);
			ret = -ENODEV;
			goto err_start_hw;
		}
	}

	mfc_ctx = mfc_create_inst();
	if (!mfc_ctx) {
		mfc_err("failed to create instance context\n");
		ret = -ENOMEM;
		goto err_inst_ctx;
	}

	mfc_ctx->id = get_free_inst_id(mfcdev);
	if (mfc_ctx->id < 0) {
		ret = -EINVAL;
		goto err_inst_ctx;
	}

	printk(KERN_INFO"[%s]Opened Instance id %d \n",__func__,mfc_ctx->id);
	mfc_ctx->dev = mfcdev;

	atomic_inc(&mfcdev->inst_cnt);
	mfcdev->inst_ctx[mfc_ctx->id] = mfc_ctx;

	file->private_data = (struct mfc_inst_ctx *)mfc_ctx;

	mutex_unlock(&mfcdev->lock);

	return 0;

err_inst_ctx:
err_start_hw:
	if (atomic_read(&mfcdev->inst_cnt) == 0) {
		if (mfc_power_off() < 0)
			mfc_err("power disable failed\n");
	}

err_pwr_enable:
err_fw_state:
	mutex_unlock(&mfcdev->lock);

	return ret;
}

static int mfc_release(struct inode *inode, struct file *file)
{
	struct mfc_inst_ctx *mfc_ctx;
	struct mfc_dev *dev;
	int ret;

	mfc_ctx = (struct mfc_inst_ctx *)file->private_data;
	if (!mfc_ctx)
		return -EINVAL;

	dev = mfc_ctx->dev;

	mutex_lock(&dev->lock);

#ifdef CONFIG_CPU_FREQ
	/* Release MFC & Bus Frequency lock for High resolution */
	if (mfc_ctx->busfreq_flag == true){
		atomic_dec(&dev->busfreq_lock_cnt);
		mfc_ctx->busfreq_flag = false;
		if (atomic_read(&dev->busfreq_lock_cnt) == 0){
			/* release Freq lock back to normal */
			s5pv310_busfreq_lock_free(DVFS_LOCK_ID_MFC);
			mfc_dbg("[%s] Bus Freq lock Released Normal !!\n",__func__);
		}
	}
#endif

	file->private_data = NULL;

	printk(KERN_INFO"[%s]Released Instance id %d \n",__func__,mfc_ctx->id);
	dev->inst_ctx[mfc_ctx->id] = NULL;
	atomic_dec(&dev->inst_cnt);

	mfc_destroy_inst(mfc_ctx);

	if (atomic_read(&dev->inst_cnt) == 0) {
		ret = mfc_power_off();
		if (ret < 0) {
			mfc_err("power disable failed.\n");
			goto err_pwr_disable;
		}
	} else {
#if defined(SYSMMU_MFC_ON) && !defined(CONFIG_VIDEO_MFC_VCM_UMP)
	mfc_clock_on();

	sysmmu_tlb_invalidate(SYSMMU_MFC_L);
	sysmmu_tlb_invalidate(SYSMMU_MFC_R);

	mfc_clock_off();
#endif
	}

	ret = 0;

err_pwr_disable:
	mutex_unlock(&dev->lock);

	return ret;
}

/* FIXME: add request firmware ioctl */
static long mfc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	struct mfc_inst_ctx *mfc_ctx;
	int ret, ex_ret;
	struct mfc_common_args in_param;
	struct mfc_buf_alloc_arg buf_arg;
	int port;

	struct mfc_dev *dev;
	int i;

	struct mfc_set_config_arg *set_cnf_arg;

	mfc_ctx = (struct mfc_inst_ctx *)file->private_data;
	if (!mfc_ctx)
		return -EINVAL;

	dev = mfc_ctx->dev;

	mutex_lock(&dev->lock);

	ret = copy_from_user(&in_param, (struct mfc_common_args *)arg,
			sizeof(struct mfc_common_args));
	if (ret < 0) {
		mfc_err("failed to copy parameters\n");
		ret = -EIO;
		in_param.ret_code = MFC_INVALID_PARAM_FAIL;
		goto out_ioctl;
	}

	mutex_unlock(&dev->lock);

	/* FIXME: add locking */

	mfc_dbg("cmd: 0x%08x\n", cmd);

	switch (cmd) {

	case IOCTL_MFC_DEC_INIT:
		mutex_lock(&dev->lock);

		if (mfc_chk_inst_state(mfc_ctx, INST_STATE_CREATE) < 0) {
			mfc_err("IOCTL_MFC_DEC_INIT invalid state: 0x%08x\n",
				 mfc_ctx->state);
			in_param.ret_code = MFC_STATE_INVALID;
			ret = -EINVAL;

			break;
		}

		mfc_clock_on();
		in_param.ret_code = mfc_init_decoding(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mfc_clock_off();

		mutex_unlock(&dev->lock);
		break;

	case IOCTL_MFC_ENC_INIT:
		mutex_lock(&dev->lock);

		if (mfc_chk_inst_state(mfc_ctx, INST_STATE_CREATE) < 0) {
			mfc_err("IOCTL_MFC_ENC_INIT invalid state: 0x%08x\n",
				 mfc_ctx->state);
			in_param.ret_code = MFC_STATE_INVALID;
			ret = -EINVAL;

			break;
		}

		mfc_clock_on();
		in_param.ret_code = mfc_init_encoding(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mfc_clock_off();

		mutex_unlock(&dev->lock);
		break;

	case IOCTL_MFC_DEC_EXE:
		mutex_lock(&dev->lock);

		mfc_clock_on();
		in_param.ret_code = mfc_exec_decoding(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mfc_clock_off();

		mutex_unlock(&dev->lock);
		break;

	case IOCTL_MFC_ENC_EXE:
		mutex_lock(&dev->lock);

		mfc_clock_on();
		in_param.ret_code = mfc_exec_encoding(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mfc_clock_off();

		mutex_unlock(&dev->lock);
		break;

	case IOCTL_MFC_GET_IN_BUF:
		if (in_param.args.mem_alloc.type == ENCODER) {
			buf_arg.type = ENCODER;
			port = 1;
		} else {
			buf_arg.type = DECODER;
			port = 0;
		}

		/* FIXME: consider the size */
		buf_arg.size = in_param.args.mem_alloc.buff_size;
		/*
		buf_arg.mapped = in_param.args.mem_alloc.mapped_addr;
		*/
		/* FIXME: encodeing linear: 2KB, tile: 8KB */
		buf_arg.align = ALIGN_2KB;

		if (buf_arg.type == ENCODER)
			in_param.ret_code = mfc_alloc_buf(mfc_ctx, &buf_arg, MBT_DPB | port);
		else
			in_param.ret_code = mfc_alloc_buf(mfc_ctx, &buf_arg, MBT_CPB | port);
#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
		in_param.args.mem_alloc.secure_id = buf_arg.secure_id;
#elif defined(CONFIG_S5P_VMEM)
		in_param.args.mem_alloc.cookie = buf_arg.cookie;
#else
		in_param.args.mem_alloc.offset = buf_arg.offset;
#endif
		ret = in_param.ret_code;

		break;

	case IOCTL_MFC_FREE_BUF:
		in_param.ret_code =
			mfc_free_buf(mfc_ctx, in_param.args.mem_free.key);
		ret = in_param.ret_code;

		break;

	case IOCTL_MFC_GET_REAL_ADDR:
		in_param.args.real_addr.addr =
			mfc_get_buf_real(mfc_ctx->id, in_param.args.real_addr.key);

		mfc_dbg("real addr: 0x%08x", in_param.args.real_addr.addr);

		if (in_param.args.real_addr.addr)
			in_param.ret_code = MFC_OK;
		else
			in_param.ret_code = MFC_MEM_INVALID_ADDR_FAIL;

		ret = in_param.ret_code;

		break;

	case IOCTL_MFC_GET_MMAP_SIZE:
		if (mfc_chk_inst_state(mfc_ctx, INST_STATE_CREATE) < 0) {
			mfc_err("IOCTL_MFC_GET_MMAP_SIZE invalid state: \
				0x%08x\n", mfc_ctx->state);
			in_param.ret_code = MFC_STATE_INVALID;
			ret = -EINVAL;

			break;
		}

		in_param.ret_code = MFC_OK;
		ret = 0;
		for (i = 0; i < dev->mem_ports; i++)
			ret += mfc_mem_data_size(i);

		break;

#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
	case IOCTL_MFC_SET_IN_BUF:
		if (in_param.args.mem_alloc.type == ENCODER) {
			buf_arg.secure_id = in_param.args.mem_alloc.secure_id;
			buf_arg.align = ALIGN_2KB;
			port = 1;
			ret = mfc_vcm_bind_from_others(mfc_ctx, &buf_arg, MBT_OTHER | port);
		}
		else {
		in_param.args.real_addr.addr =
			mfc_ump_get_virt(in_param.args.real_addr.key);

		mfc_dbg("real addr: 0x%08x", in_param.args.real_addr.addr);

		if (in_param.args.real_addr.addr)
			in_param.ret_code = MFC_OK;
		else
			in_param.ret_code = MFC_MEM_INVALID_ADDR_FAIL;

		ret = in_param.ret_code;
		}

		break;
#endif

	case IOCTL_MFC_SET_CONFIG:
		/* FIXME: mfc_chk_inst_state*/
		/* RMVME: need locking ? */
		mutex_lock(&dev->lock);

		/* in_param.ret_code = mfc_set_config(mfc_ctx, &(in_param.args)); */

		set_cnf_arg = (struct mfc_set_config_arg *)&in_param.args;

		in_param.ret_code = mfc_set_inst_cfg(mfc_ctx, set_cnf_arg->in_config_param, set_cnf_arg->in_config_value);
		ret = in_param.ret_code;

		mutex_unlock(&dev->lock);
		break;

	case IOCTL_MFC_GET_CONFIG:
		/* FIXME: */
		/* FIXME: mfc_chk_inst_state */
		/* RMVME: need locking ? */

		in_param.ret_code = MFC_OK;
		ret = MFC_OK;
		break;

	case IOCTL_MFC_SET_BUF_CACHE:
		mfc_ctx->buf_cache_type = in_param.args.mem_alloc.buf_cache_type;
		in_param.ret_code = MFC_OK;
		break;

	default:
		mfc_err("failed to execute ioctl cmd: 0x%08x\n", cmd);

		in_param.ret_code = MFC_INVALID_PARAM_FAIL;
		ret = -EINVAL;
	}

out_ioctl:
	ex_ret = copy_to_user((struct mfc_common_args *)arg,
			&in_param,
			sizeof(struct mfc_common_args));
	if (ex_ret < 0) {
		mfc_err("Outparm copy to user error\n");
		ret = -EIO;
	}

	mfc_dbg("return = %d\n", ret);

	return ret;
}

static void mfc_vm_open(struct vm_area_struct *vma)
{
	/* FIXME:
	struct mfc_inst_ctx *mfc_ctx = (struct mfc_inst_ctx *)vma->vm_private_data;

	mfc_dbg("id: %d\n", mfc_ctx->id);
	*/

	/* FIXME: atomic_inc(mapped count) */
}

static void mfc_vm_close(struct vm_area_struct *vma)
{
	/* FIXME:
	struct mfc_inst_ctx *mfc_ctx = (struct mfc_inst_ctx *)vma->vm_private_data;

	mfc_dbg("id: %d\n", mfc_ctx->id);
	*/

	/* FIXME: atomic_dec(mapped count) */
}

static int mfc_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{

	/* FIXME:
	struct mfc_inst_ctx *mfc_ctx = (struct mfc_inst_ctx *)vma->vm_private_data;
	struct page *pg = NULL;

	mfc_dbg("id: %d, pgoff: 0x%08lx, user: 0x%08lx\n",
		mfc_ctx->id, vmf->pgoff, (unsigned long)(vmf->virtual_address));

	if (mfc_ctx == NULL)
		return VM_FAULT_SIGBUS;

	mfc_dbg("addr: 0x%08lx\n",
		(unsigned long)(_mfc_get_buf_addr(mfc_ctx->id, vmf->virtual_address)));

	pg = vmalloc_to_page(_mfc_get_buf_addr(mfc_ctx->id, vmf->virtual_address));

	if (!pg)
		return VM_FAULT_SIGBUS;

	vmf->page = pg;
	*/

	return 0;
}

static const struct vm_operations_struct mfc_vm_ops = {
	.open	= mfc_vm_open,
	.close	= mfc_vm_close,
	.fault	= mfc_vm_fault,
};

static int mfc_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long user_size = vma->vm_end - vma->vm_start;
	unsigned long real_size;
	struct mfc_inst_ctx *mfc_ctx;
#if !(defined(CONFIG_VIDEO_MFC_VCM_UMP) || defined(CONFIG_S5P_VMEM))
	/* mmap support */
	unsigned long pfn;
	unsigned long remap_offset, remap_size;
	struct mfc_dev *dev;
#ifdef SYSMMU_MFC_ON
	/* kernel virtual memory allocator */
	char *ptr;
	unsigned long start, size;
#endif
#endif
	mfc_ctx = (struct mfc_inst_ctx *)file->private_data;
	if (!mfc_ctx)
		return -EINVAL;

#if !(defined(CONFIG_VIDEO_MFC_VCM_UMP) || defined(CONFIG_S5P_VMEM))
	dev = mfc_ctx->dev;
#endif

	mfc_dbg("vm_start: 0x%08lx, vm_end: 0x%08lx, size: %ld(%ldMB)\n",
		vma->vm_start, vma->vm_end, user_size, (user_size >> 20));

	real_size = (unsigned long)(mfc_mem_data_size(0) + mfc_mem_data_size(1));

	mfc_dbg("port 0 size: %d, port 1 size: %d, total: %ld\n",
		mfc_mem_data_size(0),
		mfc_mem_data_size(1),
		real_size);

	/*
	 * if memory size required from appl. mmap() is bigger than max data memory
	 * size allocated in the driver.
	 */
	if (user_size > real_size) {
		mfc_err("user requeste mem(%ld) is bigger than available mem(%ld)\n",
			user_size, real_size);
		return -EINVAL;
	}
#ifdef SYSMMU_MFC_ON
#if (defined(CONFIG_VIDEO_MFC_VCM_UMP) || defined(CONFIG_S5P_VMEM))
	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_ops = &mfc_vm_ops;
	vma->vm_private_data = mfc_ctx;

	mfc_ctx->userbase = vma->vm_start;
#else	/* not CONFIG_VIDEO_MFC_VCM_UMP && not CONFIG_S5P_VMEM */
	/* kernel virtual memory allocator */
	if (dev->mem_ports == 1) {
		remap_offset = 0;
		remap_size = user_size;

		vma->vm_flags |= VM_RESERVED | VM_IO;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		/*
		 * Port 0 mapping for stream buf & frame buf (chroma + MV + luma)
		 */
		ptr = (char *)mfc_mem_data_base(0);
		start = remap_offset;
		size = remap_size;
		while (size > 0) {
			pfn = vmalloc_to_pfn(ptr);
			if (remap_pfn_range(vma, vma->vm_start + start, pfn,
				PAGE_SIZE, vma->vm_page_prot)) {

				mfc_err("failed to remap port 0\n");
				return -EAGAIN;
			}

			start += PAGE_SIZE;
			ptr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	} else {
		remap_offset = 0;
		remap_size = min((unsigned long)mfc_mem_data_size(0), user_size);

		vma->vm_flags |= VM_RESERVED | VM_IO;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		/*
		 * Port 0 mapping for stream buf & frame buf (chroma + MV)
		 */
		ptr = (char *)mfc_mem_data_base(0);
		start = remap_offset;
		size = remap_size;
		while (size > 0) {
			pfn = vmalloc_to_pfn(ptr);
			if (remap_pfn_range(vma, vma->vm_start + start, pfn,
				PAGE_SIZE, vma->vm_page_prot)) {

				mfc_err("failed to remap port 0\n");
				return -EAGAIN;
			}

			start += PAGE_SIZE;
			ptr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}

		remap_offset = remap_size;
		remap_size = min((unsigned long)mfc_mem_data_size(1),
			user_size - remap_offset);

		vma->vm_flags |= VM_RESERVED | VM_IO;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		/*
		 * Port 1 mapping for frame buf (luma)
		 */
		ptr = (void *)mfc_mem_data_base(1);
		start = remap_offset;
		size = remap_size;
		while (size > 0) {
			pfn = vmalloc_to_pfn(ptr);
			if (remap_pfn_range(vma, vma->vm_start + start, pfn,
				PAGE_SIZE, vma->vm_page_prot)) {

				mfc_err("failed to remap port 1\n");
				return -EAGAIN;
			}

			start += PAGE_SIZE;
			ptr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	}

	mfc_ctx->userbase = vma->vm_start;

	mfc_dbg("user request mem = %ld, available data mem = %ld\n",
		  user_size, real_size);

	if ((remap_offset + remap_size) < real_size)
		mfc_warn("The MFC reserved memory dose not mmap fully [%ld: %ld]\n",
		  real_size, (remap_offset + remap_size));
#endif	/* end of CONFIG_VIDEO_MFC_VCM_UMP */
#else	/* not SYSMMU_MFC_ON */
	/* early allocator */
	/* CMA or bootmem(memblock) */
	if (dev->mem_ports == 1) {
		remap_offset = 0;
		remap_size = user_size;

		vma->vm_flags |= VM_RESERVED | VM_IO;

		if(mfc_ctx->buf_cache_type == NO_CACHE){
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
			mfc_info("CONFIG_VIDEO_MFC_CACHE is not enabled\n");
		}else
			mfc_info("CONFIG_VIDEO_MFC_CACHE is enabled\n");


		/*
		 * Port 0 mapping for stream buf & frame buf (chroma + MV + luma)
		 */
		pfn = __phys_to_pfn(mfc_mem_data_base(0));
		if (remap_pfn_range(vma, vma->vm_start + remap_offset, pfn,
			remap_size, vma->vm_page_prot)) {

			mfc_err("failed to remap port 0\n");
			return -EINVAL;
		}
	} else {
		remap_offset = 0;
		remap_size = min((unsigned long)mfc_mem_data_size(0), user_size);

		vma->vm_flags |= VM_RESERVED | VM_IO;

		if(mfc_ctx->buf_cache_type == NO_CACHE){
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
			mfc_info("CONFIG_VIDEO_MFC_CACHE is not enabled\n");
		}else
			mfc_info("CONFIG_VIDEO_MFC_CACHE is enabled\n");



		/*
		 * Port 0 mapping for stream buf & frame buf (chroma + MV)
		 */
		pfn = __phys_to_pfn(mfc_mem_data_base(0));
		if (remap_pfn_range(vma, vma->vm_start + remap_offset, pfn,
			remap_size, vma->vm_page_prot)) {

			mfc_err("failed to remap port 0\n");
			return -EINVAL;
		}

		remap_offset = remap_size;
		remap_size = min((unsigned long)mfc_mem_data_size(1),
			user_size - remap_offset);

		vma->vm_flags |= VM_RESERVED | VM_IO;
		
		if(mfc_ctx->buf_cache_type == NO_CACHE)
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);


		/*
		 * Port 1 mapping for frame buf (luma)
		 */
		pfn = __phys_to_pfn(mfc_mem_data_base(1));
		if (remap_pfn_range(vma, vma->vm_start + remap_offset, pfn,
			remap_size, vma->vm_page_prot)) {

			mfc_err("failed to remap port 1\n");
			return -EINVAL;
		}
	}

	mfc_ctx->userbase = vma->vm_start;

	mfc_dbg("user request mem = %ld, available data mem = %ld\n",
		  user_size, real_size);

	if ((remap_offset + remap_size) < real_size)
		mfc_warn("The MFC reserved memory dose not mmap fully [%ld: %ld]\n",
		  real_size, (remap_offset + remap_size));
#endif	/* end of SYSMMU_MFC_ON */
	return 0;
}

static const struct file_operations mfc_fops = {
	.owner		= THIS_MODULE,
	.open		= mfc_open,
	.release	= mfc_release,
	.unlocked_ioctl	= mfc_ioctl,
	.mmap		= mfc_mmap,
};

static struct miscdevice mfc_miscdev = {
	.minor	= MFC_MINOR,
	.name	= MFC_DEV_NAME,
	.fops	= &mfc_fops,
};

static void mfc_firmware_request_complete_handler(const struct firmware *fw,
						  void *context)
{
	if (fw != NULL) {
		mfcdev->fw.state = mfc_load_firmware(fw->data, fw->size);
		printk(KERN_INFO "MFC F/W loaded successfully (size: %d)\n", fw->size);

		mfcdev->fw.info = fw;
	} else {
		printk(KERN_ERR "failed to load MFC F/W, MFC will not working\n");
	}
}

/* FIXME: check every exception case (goto) */
static int __devinit mfc_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	mfcdev = kzalloc(sizeof(struct mfc_dev), GFP_KERNEL);
	if (unlikely(mfcdev == NULL)) {
		dev_err(&pdev->dev, "failed to allocate control memory\n");
		return -ENOMEM;
	}

	/* init. control structure */
	sprintf(mfcdev->name, "%s", MFC_DEV_NAME);

	mutex_init(&mfcdev->lock);
	init_waitqueue_head(&mfcdev->wait_sys);
	init_waitqueue_head(&mfcdev->wait_codec[0]);
	init_waitqueue_head(&mfcdev->wait_codec[1]);
	atomic_set(&mfcdev->inst_cnt, 0);
	atomic_set(&mfcdev->busfreq_lock_cnt, 0);

	mfcdev->device = &pdev->dev;

	platform_set_drvdata(pdev, mfcdev);

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "no memory resource specified\n");
		ret = -ENOENT;
		goto err_mem_res;
	}

	mfcdev->reg.rsrc_start = res->start;
	mfcdev->reg.rsrc_len = resource_size(res);

	/* request mem region for MFC register (0x0000 ~ 0xE008) */
	res = request_mem_region(mfcdev->reg.rsrc_start,
			mfcdev->reg.rsrc_len, pdev->name);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_mem_req;
	}

	/* ioremap for MFC register */
	mfcdev->reg.base = ioremap(mfcdev->reg.rsrc_start, mfcdev->reg.rsrc_len);

	if (unlikely(!mfcdev->reg.base)) {
		dev_err(&pdev->dev, "failed to ioremap memory region\n");
		ret = -EINVAL;
		goto err_mem_map;
	}

	init_reg(mfcdev->reg.base);

	mfcdev->irq = platform_get_irq(pdev, 0);
	if (unlikely(mfcdev->irq < 0)) {
		dev_err(&pdev->dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_irq_res;
	}

	ret = request_irq(mfcdev->irq, mfc_irq, IRQF_DISABLED, mfcdev->name, mfcdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to allocate irq (%d)\n", ret);
		goto err_irq_req;
	}

	/*
	 * initialize PM(power, clock) interface
	 */
	ret = mfc_init_pm(mfcdev);
	if (ret < 0) {
		printk(KERN_ERR "failed to init. MFC PM interface\n");
		goto err_pm_if;
	}

	/*
	 * initialize memory manager
	 */
	ret = mfc_init_mem_mgr(mfcdev);
	if (ret < 0) {
		printk(KERN_ERR "failed to init. MFC memory manager\n");
		goto err_mem_mgr;
	}

	/*
	 * loading firmware
	 */
	ret = request_firmware_nowait(THIS_MODULE,
				      FW_ACTION_HOTPLUG,
				      MFC_FW_NAME,
				      &pdev->dev,
				      GFP_KERNEL,
				      pdev,
				      mfc_firmware_request_complete_handler);
	if (ret) {
		dev_err(&pdev->dev, "could not load firmware (err=%d)\n", ret);
		goto err_fw_req;
	}

#if defined(SYSMMU_MFC_ON) && defined(CONFIG_VIDEO_MFC_VCM_UMP)
	ret = vcm_activate(mfcdev->vcm_info.sysmmu_vcm);
	if (ret < 0) {
		mfc_err("failed to activate VCM: %d", ret);

		goto err_act_vcm;
	}
#endif

	/*
	 * initialize buffer manager
	 */
	mfc_init_buf();

	/* FIXME: final dec & enc */
	mfc_init_decoders();
	mfc_init_encoders();

	ret = misc_register(&mfc_miscdev);

	if (ret) {
		mfc_err("MFC can't misc register on minor=%d\n", MFC_MINOR);
		goto err_misc_reg;
	}

	mfc_info("MFC(Multi Function Codec - FIMV v5.x) registered successfully\n");

	return 0;

err_misc_reg:
	mfc_final_buf();

#ifdef SYSMMU_MFC_ON
#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	mfc_clock_on();

	vcm_deactivate(mfcdev->vcm_info.sysmmu_vcm);

	mfc_clock_off();

err_act_vcm:
#endif
	mfc_clock_on();

	sysmmu_off(SYSMMU_MFC_L);
	sysmmu_off(SYSMMU_MFC_R);

	mfc_clock_off();
#endif
	if (mfcdev->fw.info)
		release_firmware(mfcdev->fw.info);

err_fw_req:
	/* FIXME: make kenel dump when probe fail */
	mfc_clock_on();

	mfc_final_mem_mgr(mfcdev);

	mfc_clock_off();

err_mem_mgr:
	mfc_final_pm(mfcdev);

err_pm_if:
	free_irq(mfcdev->irq, mfcdev);

err_irq_req:
err_irq_res:
	iounmap(mfcdev->reg.base);

err_mem_map:
	release_mem_region(mfcdev->reg.rsrc_start, mfcdev->reg.rsrc_len);

err_mem_req:
err_mem_res:
	platform_set_drvdata(pdev, NULL);
	mutex_destroy(&mfcdev->lock);
	kfree(mfcdev);

	return ret;
}

/* FIXME: check mfc_remove funtionalilty */
static int __devexit mfc_remove(struct platform_device *pdev)
{
	struct mfc_dev *dev = platform_get_drvdata(pdev);

	/* FIXME: close all instance? or check active instance? */

	misc_deregister(&mfc_miscdev);

	mfc_final_buf();
#ifdef SYSMMU_MFC_ON
	mfc_clock_on();

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	vcm_deactivate(mfcdev->vcm_info.sysmmu_vcm);
#endif

	sysmmu_off(SYSMMU_MFC_L);
	sysmmu_off(SYSMMU_MFC_R);

	mfc_clock_off();
#endif
	if (dev->fw.info)
		release_firmware(dev->fw.info);
	mfc_final_mem_mgr(dev);
	mfc_final_pm(dev);
	free_irq(dev->irq, dev);
	iounmap(dev->reg.base);
	release_mem_region(dev->reg.rsrc_start, dev->reg.rsrc_len);
	platform_set_drvdata(pdev, NULL);
	mutex_destroy(&dev->lock);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int mfc_suspend(struct device *dev)
{
	struct mfc_dev *m_dev = platform_get_drvdata(to_platform_device(dev));
	int ret;

	if (atomic_read(&m_dev->inst_cnt) == 0)
		return 0;

	mutex_lock(&m_dev->lock);

	ret = mfc_sleep(m_dev);

	mutex_unlock(&m_dev->lock);

	if (ret != MFC_OK)
		return ret;

	return 0;
}

static int mfc_resume(struct device *dev)
{
	struct mfc_dev *m_dev = platform_get_drvdata(to_platform_device(dev));
	int ret;
	u32 timeout;

	if (atomic_read(&m_dev->inst_cnt) == 0)
		return 0;

#ifdef SYSMMU_MFC_ON
	mfc_clock_on();

	sysmmu_on(SYSMMU_MFC_L);
	sysmmu_on(SYSMMU_MFC_R);

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	vcm_set_pgtable_base(VCM_DEV_MFC);
#else /* CONFIG_S5P_VMEM or kernel virtual memory allocator */
	sysmmu_set_tablebase_pgd(SYSMMU_MFC_L, __pa(swapper_pg_dir));
	sysmmu_set_tablebase_pgd(SYSMMU_MFC_R, __pa(swapper_pg_dir));
#endif

	mfc_clock_off();
#endif

	mutex_lock(&m_dev->lock);

	__raw_writel(S5P_INT_LOCAL_PWR_EN, S5P_PMU_MFC_CONF);

	/* Wait max 1ms */
	timeout = 10;
	while ((__raw_readl(S5P_PMU_MFC_CONF + 0x4) & S5P_INT_LOCAL_PWR_EN)
		!= S5P_INT_LOCAL_PWR_EN) {
		if (timeout == 0) {
			printk(KERN_ERR "Power domain MFC enable failed.\n");
			break;
		}
		timeout--;
		udelay(100);
	}

	ret = mfc_wakeup(m_dev);

	mutex_unlock(&m_dev->lock);

	if (ret != MFC_OK)
		return ret;

	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int mfc_runtime_suspend(struct device *dev)
{
	struct mfc_dev *m_dev = platform_get_drvdata(to_platform_device(dev));

	atomic_set(&m_dev->pm.power, 0);

	return 0;
}

static int mfc_runtime_idle(struct device *dev)
{
	return 0;
}

static int mfc_runtime_resume(struct device *dev)
{
	struct mfc_dev *m_dev = platform_get_drvdata(to_platform_device(dev));
	int pre_power;

	pre_power = atomic_read(&m_dev->pm.power);
	atomic_set(&m_dev->pm.power, 1);

#ifdef SYSMMU_MFC_ON
	if (pre_power == 0) {
		mfc_clock_on();

		sysmmu_on(SYSMMU_MFC_L);
		sysmmu_on(SYSMMU_MFC_R);

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
		vcm_set_pgtable_base(VCM_DEV_MFC);
#else /* CONFIG_S5P_VMEM or kernel virtual memory allocator */
		sysmmu_set_tablebase_pgd(SYSMMU_MFC_L, __pa(swapper_pg_dir));
		sysmmu_set_tablebase_pgd(SYSMMU_MFC_R, __pa(swapper_pg_dir));
#endif

		mfc_clock_off();
	}
#endif

	return 0;
}
#endif

#else
#define mfc_suspend NULL
#define mfc_resume NULL
#ifdef CONFIG_PM_RUNTIME
#define mfc_runtime_idle 	NULL
#define mfc_runtime_suspend 	NULL
#define mfc_runtime_resume	NULL
#endif
#endif

static const struct dev_pm_ops mfc_pm_ops = {
	.suspend		= mfc_suspend,
	.resume			= mfc_resume,
#ifdef CONFIG_PM_RUNTIME
	.runtime_idle		= mfc_runtime_idle,
	.runtime_suspend	= mfc_runtime_suspend,
	.runtime_resume		= mfc_runtime_resume,
#endif
};

static struct platform_driver mfc_driver = {
	.probe		= mfc_probe,
	.remove		= __devexit_p(mfc_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MFC_DEV_NAME,
		.pm	= &mfc_pm_ops,
	},
};

static int __init mfc_init(void)
{
	if (platform_driver_register(&mfc_driver) != 0) {
		printk(KERN_ERR "FIMV MFC platform device registration failed.. \n");
		return -1;
	}

	return 0;
}

static void __exit mfc_exit(void)
{
	platform_driver_unregister(&mfc_driver);
	mfc_info("FIMV MFC(Multi Function Codec) V5.x exit.\n");
}

module_init(mfc_init);
module_exit(mfc_exit);

MODULE_AUTHOR("Jeongtae, Park");
MODULE_AUTHOR("Jaeryul, Oh");
MODULE_DESCRIPTION("FIMV MFC(Multi Function Codec) V5.x Device Driver");
MODULE_LICENSE("GPL");

