/*
 * Virtual Contiguous Memory header
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

#ifndef __LINUX_VCM_H
#define __LINUX_VCM_H

#include <linux/kref.h>
#include <linux/compiler.h>

struct vcm_driver;
struct vcm_phys;

/**
 * struct vcm - A virtually contiguous memory context.
 * @start:	the smallest possible address available in this context.
 * @size:	size of available address space in bytes; internal, read
 *		only for MMU drivers.
 * @activations:	How many times context was activated; internal,
 *			read only for MMU drivers.
 * @driver:	driver handling this driver; internal.
 *
 * This structure represents a context of virtually contiguous memory
 * managed by a MMU pointed by the @mmu pointer.  This is the main
 * structure used to interact with the VCM framework.
 *
 * Whenever driver wants to reserve virtual address space or allocate
 * backing storage this pointer to this structure must be passed.
 *
 */
struct vcm {
	dma_addr_t		start;
	resource_size_t		size;
	atomic_t		activations;
	const struct vcm_driver	*driver;
};

/**
 * struct vcm_res - A reserved virtually contiguous address space.
 * @start:	bus address of the region in bytes; read only.
 * @bound_size:	number of bytes actually bound to the virtual address;
 *		read only.
 * @res_size:	size of the reserved address space in bytes; read only.
 * @refcnt:	reference count of a reservation to pass ownership of
 *		a reservation in a safe way; internal.
 *		Implemented only when CONFIG_VCM_RES_REFCNT is enabled.
 * @vcm:	VCM context; internal, read only for MMU drivers.
 * @phys:	pointer to physical memory bound to this reservation; NULL
 *		if no physical memory is bound; read only.
 *
 * This structure represents a portion virtually contiguous address
 * space reserved for use with the driver.  Once address space is
 * reserved a physical memory can be bound to it so that it will paint
 * to real memory.
 */
struct vcm_res {
	dma_addr_t		start;
	resource_size_t		bound_size;
	resource_size_t		res_size;
#ifdef CONFIG_VCM_RES_REFCNT
	atomic_t		refcnt;
#endif

	struct vcm		*vcm;
	struct vcm_phys		*phys;
};


/**
 * vcm_destroy() - destroys a VCM context.
 * @vcm:	VCM to destroy.
 */
void vcm_destroy(struct vcm *vcm);

/**
 * vcm_make_binding() - allocates memory and binds it to virtual address space
 * @vcm:	VCM context to reserve virtual address space in
 * @size:	number of bytes to allocate; aligned up to a PAGE_SIZE
 * @alloc_flags:	additional allocator flags; see vcm_alloc() for
 *			description of those.
 * @res_flags:	additional reservation flags; see vcm_reserve() for
 *		description of those.
 *
 * This is a call that binds together three other calls:
 * vcm_reserve(), vcm_alloc() and vcm_bind().  The purpose of this
 * function is that on systems with no IO MMU separate calls to
 * vcm_alloc() and vcm_reserve() may fail whereas when called together
 * they may work correctly.
 *
 * This is a consequence of the fact that with no IO MMU the simulated
 * virtual address must be the same as physical address, thus if first
 * virtual address space were to be reserved and then physical memory
 * allocated, both addresses may not match.
 *
 * With this call, a driver that simulates IO MMU may simply allocate
 * a physical memory and when this succeeds create correct reservation.
 *
 * In short, if device drivers do not need more advanced MMU
 * functionolities, they should limit themselves to this function
 * since then the drivers may be easily ported to systems without IO
 * MMU.
 *
 * To access the vcm_phys structure created by this call a phys field
 * of returned vcm_res structure should be used.
 *
 * On error returns a pointer which yields true when tested with
 * IS_ERR().
 */
struct vcm_res  *__must_check
vcm_make_binding(struct vcm *vcm, resource_size_t size,
		 unsigned alloc_flags, unsigned res_flags);

/**
 * vcm_map() - makes a reservation and binds physical memory to it
 * @vcm:	VCM context
 * @phys:	physical memory to bind.
 * @flags:	additional flags; see vcm_reserve() for	description of
 *		those.
 *
 * This is a call that binds together two other calls: vcm_reserve()
 * and vcm_bind().  If all you need is reserve address space and
 * bind physical memory it's better to use this call since it may
 * create better mappings in some situations.
 *
 * Drivers may be optimised in such a way that it won't be possible to
 * use reservation with a different physical memory.
 *
 * On error returns a pointer which yields true when tested with
 * IS_ERR().
 */
struct vcm_res *__must_check
vcm_map(struct vcm *vcm, struct vcm_phys *phys, unsigned flags);

/**
 * vcm_alloc() - allocates a physical memory for use with vcm_res.
 * @vcm:	VCM context allocation is performed in.
 * @size:	number of bytes to allocate; aligned up to a PAGE_SIZE
 * @flags:	additional allocator flags; XXX FIXME: describe
 *
 * In case of some MMU drivers, the @vcm may be important and later
 * binding (vcm_bind()) may fail if done on another @vcm.
 *
 * On success returns a vcm_phys structure representing an allocated
 * physical memory that can be bound to reserved virtual address
 * space.  On error returns a pointer which yields true when tested with
 * IS_ERR().
 */
struct vcm_phys *__must_check
vcm_alloc(struct vcm *vcm, resource_size_t size, unsigned flags);

/**
 * vcm_free() - frees an allocated physical memory
 * @phys:	physical memory to free.
 *
 * If the physical memory is bound to any reserved address space it
 * must be unbound first.  Otherwise a warning will be issued and
 * the memory won't be freed causing memory leaks.
 */
void vcm_free(struct vcm_phys *phys);

/**
 * vcm_reserve() - reserves a portion of virtual address space.
 * @vcm:	VCM context reservation is performed in.
 * @size:	number of bytes to allocate; aligned up to a PAGE_SIZE
 * @flags:	additional reservation flags; XXX FIXME: describe
 * @alignment:	required alignment of the reserved space; must be
 *		a power of two or zero.
 *
 * On success returns a vcm_res structure representing a reserved
 * (contiguous) virtual address space that physical memory can be
 * bound to (using vcm_bind()).  On error returns a pointer which
 * yields true when tested with IS_ERR().
 */
struct vcm_res *__must_check
vcm_reserve(struct vcm *vcm, resource_size_t size, unsigned flags);

/**
 * vcm_ref_reserve() - acquires the ownership of a reservation.
 * @res:	a valid reservation to access
 *
 * On success returns 0 and leads the same effect as vcm_reserve() in the
 * context of the caller of this function. In other words, once a function
 * acquire the ownership of a reservation with vcm_ref_reserve(), it must
 * release the ownership with vcm_release() as soon as it does not need
 * the reservation.
 *
 * On error returns -EINVAL. The only reason of the error is passing an invalid
 * reservation like NULL or an unreserved reservation.
 */
#ifdef CONFIG_VCM_RES_REFCNT
int __must_check vcm_ref_reserve(struct vcm_res *res);
#endif

/**
 * vcm_unreserve() - destroyers a virtual address space reservation
 * @res:	reservation to destroy.
 *
 * If any physical memory is bound to the reserved address space it
 * must be unbound first.  Otherwise it will be unbound and warning
 * will be issued.
 */
void vcm_unreserve(struct vcm_res *res);

/**
 * vcm_bind() - binds a physical memory to virtual address space
 * @res:	virtual address space to bind the physical memory.
 * @phys:	physical memory to bind to the virtual addresses.
 *
 * The mapping won't be active unless vcm_activate() on the VCM @res
 * was created in context of was called.
 *
 * If @phys is already bound to @res this function returns -EALREADY.
 * If some other physical memory is bound to @res -EADDRINUSE is
 * returned.  If size of the physical memory is larger then the
 * virtual space -ENOSPC is returned.  In all other cases the physical
 * memory is bound to the virtual address and on success zero is
 * returned, on error a negative number.
 */
int  __must_check vcm_bind(struct vcm_res *res, struct vcm_phys *phys);

/**
 * vcm_unbind() - unbinds a physical memory from virtual address space
 * @res:	virtual address space to unbind the physical memory from.
 *
 * This reverses the effect of the vcm_bind() function.  Function
 * returns physical space that was bound to the reservation (or NULL
 * if no space was bound in which case also a warning is issued).
 */
struct vcm_phys *vcm_unbind(struct vcm_res *res);

/**
 * vcm_destroy_binding() - destroys the binding
 * @res:	a bound reserved address space to destroy.
 *
 * This function incorporates three functions: vcm_unbind(),
 * vcm_free() and vcm_unreserve() (in that order) in one call.
 */
void vcm_destroy_binding(struct vcm_res *res);

/**
 * vcm_unmap() - unbinds physical memory and unreserves address space
 * @res:	reservation to destroy
 *
 * This is a call that binds together two other calls: vcm_unbind()
 * and vcm_unreserve().
 */
void vcm_unmap(struct vcm_res *res);

/**
 * vcm_activate() - activates bindings in VCM.
 * @vcm:	VCM to activate bindings in.
 *
 * All of the bindings on the @vcm done before this function is called
 * are inactive and do not take effect.  The call to this function
 * guarantees that all bindings are sent to the hardware MMU (if any).
 *
 * After VCM is activated all bindings will be automatically updated
 * on the hardware MMU, so there is no need to call this function
 * after each vcm_bind()/vcm_unbind().
 *
 * Each call to vcm_activate() should be later accompanied by a call
 * to vcm_deactivate().  Otherwise a warning will be issued when VCM
 * context is destroyed (vcm_destroy()).  This function can be called
 * several times.
 *
 * On success returns zero, on error a negative error code.
 */
int  __must_check vcm_activate(struct vcm *vcm);

/**
 * vcm_deactivate() - deactivates bindings in VCM.
 * @vcm:	VCM to deactivate bindings in.
 *
 * This function reverts effect of the vcm_activate() function.  After
 * calling this function caller has no guarantee that bindings defined
 * in VCM are active.
 *
 * If this is called without calling the vcm_activate() warning is
 * issued.
 */
void vcm_deactivate(struct vcm *vcm);

/**
 * vcm_vmm - VMM context
 *
 * Context for manipulating kernel virtual mappings.  Reserve as well
 * as rebinding is not supported by this driver.  Also, all mappings
 * are always active (till unbound) regardless of calls to
 * vcm_activate().
 *
 * After mapping, the start field of struct vcm_res should be cast to
 * pointer to void and interpreted as a valid kernel space pointer.
 */
extern struct vcm vcm_vmm[1];

#endif
