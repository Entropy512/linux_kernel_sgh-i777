/*
 *  linux/include/linux/mmc/discard.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Card driver specific definitions.
 */

#ifndef _LINUX_MMC_DISCARD_H
#define _LINUX_MMC_DISCARD_H

#include <linux/mmc/card.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define DCS_NO_DISCARD_REQ          0x0    /* new discard request doesn't come */
#define DCS_DISCARD_REQ             0x1    /* new discard request comes */
#define DCS_IDLE_TIMER_TRIGGERED    0x2    /* idle timer already triggered */

struct discard_region{
#ifdef CONFIG_MMC_DISCARD_DEBUG
    int              magic;
#endif
    struct rb_node   node;
    struct list_head list;
    unsigned int     start;
    unsigned int     len;
};

struct discard_context{
    struct rb_root           rb_root;
    struct list_head         hlru;
    struct list_head         llru;
    unsigned int             id;
    unsigned int             region_nr;
    unsigned int             max_region_nr;
    unsigned int             optimal_size;
    unsigned int             threshold_size;
    unsigned int             maximum_size;
    unsigned int             card_start;
    unsigned int             card_len;
    struct timer_list        idle_timer;
    unsigned long            idle_expires;
    struct task_struct       *idle_thread;
    atomic_t                 idle_flag;
#define DCS_IDLE_OPS_TURNED_OFF     0x0    /* idle operation turned off */
#define DCS_IDLE_OPS_TURNED_ON      0x1    /* idle operation turned on */
#define DCS_MMC_DEVICE_REMOVED      0x2    /* mmc device is removed */

    /* proc entry statistics */
    unsigned int             recv_stats;
    unsigned int             send_stats;
    unsigned int             evic_stats;
    unsigned int             discard_cnt;
    unsigned int             dcd_new_cnt;
    unsigned int             dcd_mgd_cnt;
    unsigned int             idle_cnt;
    unsigned int             overlap_cnt;
    unsigned int             send_cnt;
    struct proc_dir_entry*   proc_entry;
};

/* idle operations */
extern int mmc_read_idle(struct mmc_card *card);
extern void mmc_set_idle(struct mmc_card *card, int flag);
extern void mmc_clear_idle(struct mmc_card *card);
extern void mmc_trigger_idle_timer(struct mmc_card *card);

/* init/uninit trim context */
extern int mmc_discard_init(struct mmc_card* card);
extern int mmc_discard_set_prop(struct mmc_card* card, unsigned int start,
            unsigned int len, struct task_struct *thread);
extern int mmc_discard_final(struct mmc_card* card);// detach ?
extern int mmc_create_slab(void);
extern void mmc_destroy_slab(void);

/* mmc operations*/
extern int mmc_send_discard(struct mmc_card *card
                            ,struct discard_region *rgn
                            ,int flags
                            ,unsigned int *start
                            ,unsigned int *len);
extern void mmc_do_rw_ops(struct mmc_card *card, unsigned int rw_start, int rw_len);
extern int mmc_do_discard_ops(struct mmc_card* card, unsigned int start, unsigned int len);
extern int mmc_do_idle_ops(struct mmc_card *card);

/* debug utility functions */
#ifdef CONFIG_MMC_DISCARD_DEBUG
#define _dbg_msg(fmt, args...) printk(KERN_INFO "%s(%d): " fmt, __func__, __LINE__, ##args)
#else
#define _dbg_msg(fmt, args...)
#endif /*CONFIG_MMC_DISCARD_DEBUG */
#define _err_msg(fmt, args...) printk(KERN_ERR "%s(%d): " fmt, __func__, __LINE__, ##args)

#endif /*_LINUX_MMC_DISCARD_H */

