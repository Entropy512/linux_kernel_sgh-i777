/* drivers/char/s5p_vmem.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Header file for s5p_vmem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#if !defined(S5P_VMEM_H)
#define S5P_VMEM_H

#define MEM_IOCTL_MAGIC			'M'

#define S5P_VMEM_ALLOC		_IOWR(MEM_IOCTL_MAGIC, 0, struct s5p_vmem_alloc)
#define S5P_VMEM_FREE		_IOWR(MEM_IOCTL_MAGIC, 1, struct s5p_vmem_alloc)

#define S5P_VMEM_SHARE_ALLOC	_IOWR(MEM_IOCTL_MAGIC, 4, struct s5p_vmem_alloc)
#define S5P_VMEM_SHARE_FREE	_IOWR(MEM_IOCTL_MAGIC, 5, struct s5p_vmem_alloc)

#define S5P_VMEM_CACHEABLE_ALLOC \
	_IOWR(MEM_IOCTL_MAGIC, 6, struct s5p_vmem_alloc)
#define S5P_VMEM_CACHEABLE_SHARE_ALLOC	\
	_IOWR(MEM_IOCTL_MAGIC, 7, struct s5p_vmem_alloc)
#define S5P_VMEM_RESET		_IOWR(MEM_IOCTL_MAGIC, 8, struct s5p_vmem_alloc)

#define S5P_VMEM_MAJOR  1
#define S5P_VMEM_MINOR  14
#undef USE_DMA_ALLOC

extern int s5p_vmem_mmap(struct file *filp, struct vm_area_struct *vma);
extern int s5p_vmem_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg);
extern int s5p_vmem_open(struct inode *inode, struct file *file);
extern int s5p_vmem_release(struct inode *inode, struct file *file);

struct s5p_vmem_alloc {
	size_t size;
	unsigned long vir_addr;
	unsigned int cookie;
	unsigned long vir_addr_k;
};

#endif /* S5P_VMEM_H */
