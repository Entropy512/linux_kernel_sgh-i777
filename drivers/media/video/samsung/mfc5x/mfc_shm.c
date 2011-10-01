/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_shm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Shared memory interface file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#ifdef CONFIG_ARCH_S5PV210
#include <linux/dma-mapping.h>
#endif

#include "mfc_inst.h"
#include "mfc_mem.h"
#include "mfc_buf.h"

int init_shm(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_SHM_SIZE, ALIGN_4B, MBT_SHM | PORT_A);

	if (alloc != NULL) {
		ctx->shm = alloc->addr;
		ctx->shmofs = mfc_mem_base_ofs(alloc->real);

		memset((void *)ctx->shm, 0, MFC_SHM_SIZE);

		mfc_mem_cache_clean((void *)ctx->shm, MFC_SHM_SIZE);

		return 0;
	}

	ctx->shm = NULL;
	ctx->shmofs = 0;

	return -1;
}

void write_shm(struct mfc_inst_ctx *ctx, unsigned int data, unsigned int offset)
{
	writel(data, (ctx->shm + offset));

#if defined(CONFIG_ARCH_S5PV210)
	dma_cache_maint((void *)(ctx->shm + offset), 4, DMA_TO_DEVICE);
#elif defined(CONFIG_ARCH_S5PV310)
	mfc_mem_cache_clean((void *)((unsigned int)(ctx->shm) + offset), 4);
#endif
}

unsigned int read_shm(struct mfc_inst_ctx *ctx, unsigned int offset)
{
#if defined(CONFIG_ARCH_S5PV210)
	dma_cache_maint((void *)(ctx->shm + offset), 4, DMA_FROM_DEVICE);
#elif defined(CONFIG_ARCH_S5PV310)
	mfc_mem_cache_inv((void *)((unsigned int)(ctx->shm) + offset), 4);
#endif
	return readl(ctx->shm + offset);
}

