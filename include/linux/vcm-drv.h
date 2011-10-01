/*
 * Virtual Contiguous Memory driver API header
 * Copyright (c) 2010 by Samsung Electronics.
 * Written by Michal Nazarewicz (m.nazarewicz@samsung.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License or (at your optional) any later version of the license.
 */

/*
 * See Documentation/virtual-contiguous-memory.txt for details.
 */

#ifndef __LINUX_VCM_DRV_H
#define __LINUX_VCM_DRV_H

#include <linux/vcm.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/gfp.h>

#include <asm/atomic.h>

/**
 * struct vcm_driver - the MMU driver operations.
 * @cleanup:	called when vcm objects is destroyed; if omitted,
 *		kfree() will be used.
 * @alloc:	callback function for allocating physical memory and
 *		reserving virtual address space; XXX FIXME: document;
 *		if set, @res and @alloc are ignored.
 * @res:	creates a reservation of virtual address space; XXX FIXME:
 *		document; if @alloc is provided this is ignored.
 * @phys:	allocates a physical memory; XXX FIXME: document; if @alloc
 *		is provided this is ignored.
 * @unreserve:	destroys a virtual address space reservation created by @alloc;
 *		required.
 * @map:	reserves address space and binds a physical memory to it.
 * @bind:	binds a physical memory to a reserved address space.
 * @unbind:	unbinds a physical memory from reserved address space.
 * @activate:	activates the context making all bindings active; once
 *		the context has been activated, this callback is not
 *		called again until context is deactivated and
 *		activated again (so if user calls vcm_activate()
 *		several times only the first call in sequence will
 *		invoke this callback).
 * @deactivate:	deactivates the context making all bindings inactive;
 *		call this callback always accompanies call to the
 *		@activate callback.
 */
struct vcm_driver {
	void (*cleanup)(struct vcm *vcm);

	int (*alloc)(struct vcm *vcm, resource_size_t size,
		     struct vcm_phys **phys, unsigned alloc_flags,
		     struct vcm_res **res, unsigned res_flags);
	struct vcm_res *(*res)(struct vcm *vcm, resource_size_t size,
			       unsigned flags);
	struct vcm_phys *(*phys)(struct vcm *vcm, resource_size_t size,
				 unsigned flags);

	void (*unreserve)(struct vcm_res *res);

	struct vcm_res *(*map)(struct vcm *vcm, struct vcm_phys *phys,
			       unsigned flags);
	int (*bind)(struct vcm_res *res, struct vcm_phys *phys);
	void (*unbind)(struct vcm_res *res);

	int (*activate)(struct vcm *vcm);
	void (*deactivate)(struct vcm *vcm);
};

/**
 * struct vcm_phys - representation of allocated physical memory.
 * @count:	number of contiguous parts the memory consists of; if this
 *		equals one the whole memory block is physically contiguous;
 *		read only.
 * @size:	total size of the allocated memory; read only.
 * @free:	callback function called when memory is freed; internal.
 * @bindings:	how many virtual address space reservations this memory has
 *		been bound to; internal.
 * @parts:	array of @count parts describing each physically contiguous
 *		memory block that the whole area consists of; each element
 *		describes part's physical starting address in bytes
 *		(@parts->start), its size in bytes (@parts->size) and
 *              (optionally) pointer to first struct poge (@parts->page);
 *		read only.
 */
struct vcm_phys {
	unsigned		count;
	resource_size_t		size;

	void (*free)(struct vcm_phys *phys);
	atomic_t		bindings;

	struct vcm_phys_part {
		phys_addr_t	start;
		struct page	*page;
		resource_size_t	size;
	} parts[0];
};

/**
 * vcm_init() - initialises VCM context structure.
 * @vcm:	the VCM context to initialise.
 *
 * This function initialises the vcm structure created by a MMU driver
 * when setting things up.  It sets up all fields of the vcm structure
 * expect for @vcm->start, @vcm->size and @vcm->driver which are
 * validated by this function.  If they have invalid value function
 * produces warning and returns an error-pointer.  If everything is
 * fine, @vcm is returned.
 */
struct vcm *__must_check vcm_init(struct vcm *vcm);

#ifdef CONFIG_VCM_MMU

struct vcm_mmu;

/**
 * struct vcm_mmu_driver - a driver used for real MMUs.
 * @orders:	array of orders of pages supported by the MMU sorted from
 *		the largest to the smallest.  The last element is always
 *		zero (which means 4K page).
 * @cleanup:	Function called when the VCM context is destroyed;
 *		optional, if not provided, kfree() is used.
 * @activate:	callback function for activating a single mapping; it's
 *		role is to set up the MMU so that reserved address space
 *		donated by res will point to physical memory donated by
 *		phys; called under spinlock with IRQs disabled - cannot
 *		sleep; required unless @activate_page and @deactivate_page
 *		are both provided
 * @deactivate:	this reverses the effect of @activate; called under spinlock
 *		with IRQs disabled - cannot sleep; required unless
 *		@deactivate_page is provided.
 * @activate_page:	callback function for activating a single page; it is
 *			ignored if @activate is provided; it's given a single
 *			page such that its order (given as third argument) is
 *			one of the supported orders specified in @orders;
 *			called under spinlock with IRQs disabled - cannot
 *			sleep; required unless @activate is provided.
 * @deactivate_page:	this reverses the effect of the @activate_page
 *			callback; called under spinlock with IRQs disabled
 *			- cannot sleep; required unless @activate and
 *			@deactivate are both provided.
 */
struct vcm_mmu_driver {
	const unsigned char	*orders;

	void (*cleanup)(struct vcm *vcm);
	int (*activate)(struct vcm_res *res, struct vcm_phys *phys);
	void (*deactivate)(struct vcm_res *res, struct vcm_phys *phys);
	int (*activate_page)(dma_addr_t vaddr, dma_addr_t paddr,
			     unsigned order, void *vcm);
	int (*deactivate_page)(dma_addr_t vaddr, dma_addr_t paddr,
			       unsigned order, void *vcm);
};

/**
 * struct vcm_mmu - VCM MMU context
 * @vcm:	VCM context.
 * @driver:	VCM MMU driver's operations.
 * @pool:	virtual address space allocator; internal.
 * @bound_res:	list of bound reservations; internal.
 * @lock:	protects @bound_res and calls to activate/deactivate
 *		operations; internal.
 * @activated:	whether VCM context has been activated; internal.
 */
struct vcm_mmu {
	struct vcm			vcm;
	const struct vcm_mmu_driver	*driver;
	/* internal */
	struct gen_pool			*pool;
	struct list_head		bound_res;
	/* Protects operations on bound_res list. */
	spinlock_t			lock;
	int				activated;
};

/**
 * vcm_mmu_init() - initialises a VCM context for a real MMU.
 * @mmu:	the vcm_mmu context to initialise.
 *
 * This function initialises the vcm_mmu structure created by a MMU
 * driver when setting things up.  It sets up all fields of the
 * structure expect for @mmu->vcm.start, @mmu.vcm->size and
 * @mmu->driver which are validated by this function.  If they have
 * invalid value function produces warning and returns an
 * error-pointer.  On any other error, an error-pointer is returned as
 * well.  If everything is fine, address of @mmu->vcm is returned.
 */
struct vcm *__must_check vcm_mmu_init(struct vcm_mmu *mmu);

#endif

#ifdef CONFIG_VCM_O2O

/**
 * struct vcm_o2o_driver - VCM One-to-One driver
 * @cleanup:	cleans up the VCM context; if not specified. kfree() is used.
 * @phys:	allocates a physical contiguous memory block; this is used in
 *		the same way &struct vcm_driver's phys is used expect it must
 *		provide a contiguous block (ie. exactly one part); required.
 */
struct vcm_o2o_driver {
	void (*cleanup)(struct vcm *vcm);
	struct vcm_phys *(*phys)(struct vcm *vcm, resource_size_t size,
				 unsigned flags);
};

/**
 * struct vcm_o2o - VCM One-to-One context
 * @vcm:	VCM context.
 * @driver:	VCM One-to-One driver's operations.
 */
struct vcm_o2o {
	struct vcm			vcm;
	const struct vcm_o2o_driver	*driver;
};

/**
 * vcm_o2o_init() - initialises a VCM context for a one-to-one context.
 * @o2o:	the vcm_o2o context to initialise.
 *
 * This function initialises the vcm_o2o structure created by a O2O
 * driver when setting things up.  It sets up all fields of the
 * structure expect for @o2o->vcm.start, @o2o->vcm.size and
 * @o2o->driver which are validated by this function.  If they have
 * invalid value function produces warning and returns an
 * error-pointer.  On any other error, an error-pointer is returned as
 * well.  If everything is fine, address of @o2o->vcm is returned.
 */
struct vcm *__must_check vcm_o2o_init(struct vcm_o2o *o2o);

#endif

#ifdef CONFIG_VCM_PHYS

/**
 * __vcm_phys_alloc() - allocates physical discontiguous space
 * @size:	size of the block to allocate.
 * @flags:	additional allocation flags; XXX FIXME: document
 * @orders:	array of orders of pages supported by the MMU sorted from
 *		the largest to the smallest.  The last element is always
 *		zero (which means 4K page).
 * @gfp:	the gfp flags for pages to allocate.
 *
 * This function tries to allocate a physical discontiguous space in
 * such a way that it allocates the largest possible blocks from the
 * sizes donated by the @orders array.  So if @orders is { 8, 0 }
 * (which means 1MiB and 4KiB pages are to be used) and requested
 * @size is 2MiB and 12KiB the function will try to allocate two 1MiB
 * pages and three 4KiB pages (in that order).  If big page cannot be
 * allocated the function will still try to allocate more smaller
 * pages.
 */
struct vcm_phys *__must_check
__vcm_phys_alloc(resource_size_t size, unsigned flags,
		 const unsigned char *orders, gfp_t gfp);

/**
 * __vcm_phys_alloc_coherent() - allocates coherent physical discontiguous space
 * @size:	size of the block to allocate.
 * @flags:	additional allocation flags; XXX FIXME: document
 * @orders:	array of orders of pages supported by the MMU sorted from
 *		the largest to the smallest.  The last element is always
 *		zero (which means 4K page).
 * @gfp:	the gfp flags for pages to allocate.
 *
 * Everything is same to __vcm_phys_alloc() except, this function invalidates
 * all H/W cache lines as soon as it allocates physical memory.
 */
struct vcm_phys *__must_check
__vcm_phys_alloc_coherent(resource_size_t size, unsigned flags,
		 const unsigned char *orders, gfp_t gfp);

/**
 * vcm_phys_alloc_raw() - allocates physical discontiguous space
 * @size:	size of the block to allocate.
 * @flags:	additional allocation flags; XXX FIXME: document
 * @orders:	array of orders of pages supported by the MMU sorted from
 *		the largest to the smallest.  The last element is always
 *		zero (which means 4K page).
 *
 * This function tries to allocate a physical discontiguous space in
 * such a way that it allocates the largest possible blocks from the
 * sizes donated by the @orders array.  So if @orders is { 8, 0 }
 * (which means 1MiB and 4KiB pages are to be used) and requested
 * @size is 2MiB and 12KiB the function will try to allocate two 1MiB
 * pages and three 4KiB pages (in that order).  If big page cannot be
 * allocated the function will still try to allocate more smaller
 * pages.
 */
static inline struct vcm_phys *__must_check
vcm_phys_alloc_raw(resource_size_t size, unsigned flags,
	       const unsigned char *orders) {
	return __vcm_phys_alloc(size, flags, orders, GFP_DMA32);
}

/**
 * vcm_phys_alloc() - allocates coherent physical discontiguous space
 * @size:	size of the block to allocate.
 * @flags:	additional allocation flags; XXX FIXME: document
 * @orders:	array of orders of pages supported by the MMU sorted from
 *		the largest to the smallest.  The last element is always
 *		zero (which means 4K page).
 *
 * This function exactly same as vcm_phys_alloc_raw() except, this function
 * guarentees that the allocated page frames are not cached either H/W inner and
 * outer caches.
 */
static inline struct vcm_phys *__must_check
vcm_phys_alloc(resource_size_t size, unsigned flags,
	       const unsigned char *orders) {
	return __vcm_phys_alloc_coherent(size, flags, orders, GFP_DMA32);
}

/**
 * vcm_phys_walk() - helper function for mapping physical pages
 * @vaddr:	virtual address to map/unmap physical space to/from
 * @phys:	physical space
 * @orders:	array of orders of pages supported by the MMU sorted from
 *		the largest to the smallest.  The last element is always
 *		zero (which means 4K page).
 * @callback:	function called for each page.
 * @recover:	function called for each page when @callback returns
 *		negative number; if it also returns negative number
 *		function terminates; may be NULL.
 * @priv:	private data for the callbacks.
 *
 * This function walks through @phys trying to mach largest possible
 * page size donated by @orders.  For each such page @callback is
 * called.  If @callback returns negative number the function calls
 * @recover for each page @callback was called successfully.
 *
 * So, for instance, if we have a physical memory which consist of
 * 1Mib part and 8KiB part and @orders is { 8, 0 } (which means 1MiB
 * and 4KiB pages are to be used), @callback will be called first with
 * 1MiB page and then two times with 4KiB page.  This is of course
 * provided that @vaddr has correct alignment.
 *
 * The idea is for hardware MMU drivers to call this function and
 * provide a callbacks for mapping/unmapping a single page.  The
 * function divides the region into pages that the MMU can handle.
 *
 * If @callback at one point returns a negative number this is the
 * return value of the function; otherwise zero is returned.
 */
int vcm_phys_walk(dma_addr_t vaddr, const struct vcm_phys *phys,
		  const unsigned char *orders,
		  int (*callback)(dma_addr_t vaddr, dma_addr_t paddr,
				  unsigned order, void *priv),
		  int (*recovery)(dma_addr_t vaddr, dma_addr_t paddr,
				  unsigned order, void *priv),
		  void *priv);

#endif

#endif
