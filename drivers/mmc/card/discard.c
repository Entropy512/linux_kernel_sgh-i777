/**
 * @file    discard.c
 * @brief
 *
 * @author
 * @version $Id$
 */

#include <linux/moduleparam.h>
#include <linux/module.h>

#include <linux/mmc/card.h>
#include <linux/mmc/discard.h>


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* round down 'addr' to a multiple of 'size' */
#define DISCARD_ALIGN_LOW(addr, size) ((addr) & ~((size) - 1))
/* round up 'addr' to a multiple of 'size' */
#define DISCARD_ALIGN_HIGH(addr, size) (((addr) + ((size) - 1)) & ~((size) - 1))

#define DISCARD_SEND_ALL            0x1      /* if set, discard all region */
#define DISCARD_RW_REQUEST          0x2      /* if set, discard overlapped region with rw request */

#define DISCARD_LRU_DELETE          0x1    /* if set, delete and add */
#define DISCARD_LRU_ADD_HEAD        0x2    /* if not set, add tail   */

#define DISCARD_FINAL_LIMIT_TIME HZ


/* debug utility functions */
#ifdef CONFIG_MMC_DISCARD_DEBUG
#define DISCARD_RGN_MAGIC           0x7472696D
#define DISCARD_RGN_MAGIC_UNSET     0x2934234F
#define _dbg_set_magic(rgn)         ((rgn)->magic = DISCARD_RGN_MAGIC)
#define _dbg_unset_magic(rgn)       ((rgn)->magic = DISCARD_RGN_MAGIC_UNSET)
static void _dbg_check_magic(struct discard_region *rgn);
static void _dbg_print_rbtree(struct discard_context *dc);
static void _dbg_print_lru(struct discard_context *dc);
#else
#define _dbg_set_magic(rgn)         ({})
#define _dbg_unset_magic(rgn)       ({})
#define _dbg_check_magic(rgn)
#define _dbg_print_rbtree(dc)       ({})
#define _dbg_print_lru(dc)          ({})
#endif /*CONFIG_MMC_DISCARD_DEBUG */

static void _create_proc_entry(struct discard_context *dc);
static void _remove_proc_entry(struct discard_context *dc);
#define _set_stats(x, y)            ((x) = (y));
#define _inc_stats(x, y)            ((x) += (y));

static struct kmem_cache *mmc_discard_region_slab;

static inline struct discard_region* _alloc_region(struct discard_context *dc)
{
    struct discard_region* rgn;

    if (dc->region_nr == dc->max_region_nr)
        return ERR_PTR(-EAGAIN);

    rgn = kmem_cache_alloc(mmc_discard_region_slab, GFP_NOFS);
    if (!rgn)
        return ERR_PTR(-ENOMEM);
    _dbg_set_magic(rgn);
    dc->region_nr++;

    return rgn;
}

static inline void _evict_region(struct discard_context *dc, struct discard_region *rgn)
{
    rb_erase(&rgn->node, &dc->rb_root);
    list_del(&rgn->list);
}

static inline void _remove_region(struct discard_context *dc, struct discard_region *rgn)
{
    rb_erase(&rgn->node, &dc->rb_root);
    list_del(&rgn->list);
    dc->region_nr --;
    _dbg_unset_magic(rgn);
    kmem_cache_free(mmc_discard_region_slab, rgn);
}

static inline void _add_to_lru(struct discard_context *dc, struct discard_region *rgn, int flags)
{
    unsigned int start, end;
    struct list_head *lru;
    if (flags & DISCARD_LRU_DELETE)
        list_del(&rgn->list);

    start = DISCARD_ALIGN_HIGH(rgn->start, dc->optimal_size);
    end = DISCARD_ALIGN_LOW(rgn->start + rgn->len, dc->optimal_size);
    lru = start < end ? &dc->hlru : &dc->llru;
    if (flags & DISCARD_LRU_ADD_HEAD)
        list_add(&rgn->list, lru);
    else
        list_add_tail(&rgn->list, lru);
}

static struct discard_region* find_first_region(struct discard_context *dc ,unsigned int start ,unsigned int len)
{
    struct discard_region *rgn, *right_rgn = NULL;
    struct rb_node *n;

    if (!dc || start < dc->card_start || start + len > dc->card_start + dc->card_len)
        return ERR_PTR(-EINVAL);

    n = dc->rb_root.rb_node;
    while (n) {
        rgn = rb_entry(n, struct discard_region, node);
        _dbg_check_magic(rgn);
        if (start < rgn->start) {
            n = n->rb_left;
            right_rgn = rgn;    /* rememer right region */
        }
        else if (start >= rgn->start + rgn->len)
            n = n->rb_right;
        else {
            /* overlapped */
            return rgn;
        }
    }
    if (right_rgn && (start + len > right_rgn->start))
        return right_rgn;

    return NULL;
}

static struct discard_region* find_next_region(struct discard_context *dc, unsigned int start, unsigned int len, struct discard_region *rgn)
{
    struct discard_region *next_rgn;
    struct rb_node *next_node;

    if (!dc || start < dc->card_start || start + len > dc->card_start + dc->card_len)
        return ERR_PTR(-EINVAL);
    _dbg_check_magic(rgn);
    if ((start + len <= rgn->start) || (start >= rgn->start + rgn->len))
        return ERR_PTR(-EINVAL); // there's no overlap between rgn and (start, len)

    next_node = rb_next(&rgn->node);
    if (next_node) {
        next_rgn = rb_entry(next_node, struct discard_region, node);
        _dbg_check_magic(next_rgn);
        if (start + len > next_rgn->start)
            return next_rgn;
    }

    return NULL;
}

static struct discard_region* insert_region(struct discard_context *dc, unsigned int start,
                    unsigned int len, struct discard_region *new_rgn)
{
    struct discard_region *rgn = NULL, *right_rgn = NULL, *next_rgn;
    struct rb_node **n, *p = NULL;

    if (!dc || start < dc->card_start || start + len > dc->card_start + dc->card_len)
        return ERR_PTR(-EINVAL);

    n = &dc->rb_root.rb_node;
    while (*n) {
        p = *n;
        rgn = rb_entry(p, struct discard_region, node);
        _dbg_check_magic(rgn);
        if (start < rgn->start) {
            n = &(*n)->rb_left;
            right_rgn = rgn;
        }
        else if (start > rgn->start + rgn->len)
            n = &(*n)->rb_right;
        else
            goto found_overlapped;
    }
    if (right_rgn && (start + len >= right_rgn->start)) {
        rgn = right_rgn;
        goto found_overlapped;
    }
    else
        goto not_overlapped;

found_overlapped:
    if (start < rgn->start) {
        /* left expand */
        rgn->len = rgn->start + rgn->len - start;
        rgn->start = start;
    }
    if (start + len > rgn->start + rgn->len) {
        /* right expand */
        rgn->len = start + len - rgn->start;
    }

    /* check next right node if mergeable */
    p = rb_next(&rgn->node);
    while(p) {
        next_rgn = rb_entry(p, struct discard_region, node);
        _dbg_check_magic(next_rgn);
        if (next_rgn->start > rgn->start + rgn->len)
            /* not overlapped */
            break;
        else if (next_rgn->start + next_rgn->len > rgn->start + rgn->len) {
            /* partial overlapped: expand region and remove next region */
            rgn->len = next_rgn->start + next_rgn->len - rgn->start;
            _remove_region(dc, next_rgn);
            break;
        }
        /* fully overlapped: remove next_region and check another */
        p = rb_next(p);
        _remove_region(dc, next_rgn);
    }
    _add_to_lru(dc, rgn, DISCARD_LRU_DELETE);
    _inc_stats(dc->dcd_mgd_cnt, 1);

    return rgn;

not_overlapped:
    /* not overlapped: try allocation */
    if (!new_rgn) {
        new_rgn = _alloc_region(dc);
        if (IS_ERR(new_rgn))
            return new_rgn;
    }

    /* set new region */
    new_rgn->start = start;
    new_rgn->len = len;
    _add_to_lru(dc, new_rgn, 0);

    /* insert to rbtree */
    rb_link_node(&new_rgn->node, p, n);
    rb_insert_color(&new_rgn->node, &dc->rb_root);

    _inc_stats(dc->dcd_new_cnt, 1);

    return new_rgn;
}

static int adjust_region(struct discard_context *dc, unsigned int start, unsigned int len,
                    struct discard_region *rgn, struct discard_region *new_rgn)
{
    struct rb_node **n, *p;

    if (!dc || start < dc->card_start || start + len > dc->card_start + dc->card_len)
        return -EINVAL;
    _dbg_check_magic(rgn);
    if ((start < rgn->start) || (start + len > rgn->start + rgn->len))
        return -EINVAL;

    if (start == rgn->start) {
        if (len == rgn->len)
        /* remove */
        _remove_region(dc, rgn);
        else {
            /* remove left part */
            rgn->start = start + len;
            rgn->len = rgn->len - len;
            _add_to_lru(dc, rgn, DISCARD_LRU_DELETE | DISCARD_LRU_ADD_HEAD);
        }
    }
    else if (start + len == rgn->start + rgn->len) {
        /* remove right part */
        rgn->len = rgn->len - len;
        _add_to_lru(dc, rgn, DISCARD_LRU_DELETE | DISCARD_LRU_ADD_HEAD);
    }
    else {
        /* split: try allocation */
        if (!new_rgn) {
        new_rgn = _alloc_region(dc);
        if (IS_ERR(new_rgn))
            return PTR_ERR(new_rgn);
        }

        /* set new region and shrink old region */
        new_rgn->start = start + len;
        new_rgn->len = rgn->start + rgn->len - start - len;
        rgn->len = start - rgn->start;
        _add_to_lru(dc, rgn, DISCARD_LRU_DELETE | DISCARD_LRU_ADD_HEAD);
        _add_to_lru(dc, new_rgn, DISCARD_LRU_ADD_HEAD);

        /* insert to rbtree */
        p = &rgn->node;
        n = &p->rb_right;
        while (*n) {
            p = *n;
            n = &p->rb_left;
        }
        rb_link_node(&new_rgn->node, p, n);
        rb_insert_color(&new_rgn->node, &dc->rb_root);
    }
    return 0;
}

static struct discard_region* select_region(struct discard_context *dc)
{
    struct discard_region *rgn;

    if (!dc)
        return ERR_PTR(-EINVAL);
    if (list_empty(&dc->hlru))
        return NULL;

    rgn = list_entry(dc->hlru.next, struct discard_region, list);
    _dbg_check_magic(rgn);
    return rgn;
}

static int evict_region(struct discard_context *dc, unsigned int start,
                    unsigned int len, struct discard_region **evict_rgn)
{
    struct discard_region *rgn, *tmp;

    if (!dc)
        return -EINVAL;
    if (list_empty(&dc->llru))
        return -EAGAIN;

    list_for_each_entry_safe(rgn, tmp, &dc->llru, list) {
        if (rgn->start >= start + len  || rgn->start + rgn->len <= start ) {
            _dbg_check_magic(rgn);
            _inc_stats(dc->evic_stats, rgn->len);
            if (evict_rgn) {
                _evict_region(dc, rgn);
                *evict_rgn = rgn;
            } else
            _remove_region(dc, rgn);
            return 0;
        }
    }
    return -EAGAIN;
}

static int try_adjust_region(struct discard_context *dc, struct discard_region *rgn,
                    unsigned int adj_start, unsigned int adj_len,
                    unsigned int evt_start, unsigned int evt_len)
{
    struct discard_region *new_rgn;
    int ret;

    ret = adjust_region(dc, adj_start, adj_len, rgn, NULL);
    if (ret == -EAGAIN || ret == -ENOMEM) {
        ret = evict_region(dc, evt_start, evt_len, &new_rgn);
        if (ret == 0) {
            ret = adjust_region(dc, adj_start, adj_len, rgn, new_rgn);
            if (ret)
                _err_msg("adjust_region failure (%d).\n", ret);
        } else if (ret == -EAGAIN) {  // Low LRU is empty
            ret = adjust_region(dc, rgn->start, adj_start - rgn->start + adj_len, rgn, NULL);
            if (ret)
                _err_msg("adjust_region failure (%d).\n", ret);
        } else
            _err_msg("evict_region failure (%d).\n", ret);
    } else if (ret)
        _err_msg("adjust_region failure (%d).\n", ret);

    return ret;
}

/* mmc operations */
int mmc_send_discard(struct mmc_card *card, struct discard_region *rgn,
                     int flags, unsigned int *start, unsigned int *len)
{
    unsigned int trim_start, trim_end;
    unsigned int rw_start, rw_end;
    unsigned int optimal_size, trim_len;
    int ret;

    if (!card || !rgn || !start || !len) {
        _err_msg("input param is null.\n");
        return -EINVAL;
    }

    optimal_size = card->discard_ctx.optimal_size;
    trim_start = DISCARD_ALIGN_HIGH(rgn->start, optimal_size);
    trim_end = DISCARD_ALIGN_LOW(rgn->start + rgn->len, optimal_size);

    if (flags & DISCARD_RW_REQUEST) {
        rw_start = DISCARD_ALIGN_LOW(*start, optimal_size);
        rw_end = DISCARD_ALIGN_HIGH(*start + *len, optimal_size);
        trim_start = MAX(trim_start, rw_start);
        trim_end = MIN(trim_end, rw_end);
    }

    *start = trim_start;
    if (trim_start >= trim_end) {
        *len = 0;
        return 0;
    }
    trim_len = trim_end - trim_start;

    if (!(flags & DISCARD_SEND_ALL))
        trim_len = MIN(trim_len, card->discard_ctx.maximum_size);
    *len = trim_len;

    ret = mmc_erase(card, trim_start, trim_len, MMC_TRIM_ARG);
    if (!ret) {
        _inc_stats(card->discard_ctx.send_stats, trim_len);
        _inc_stats(card->discard_ctx.send_cnt, 1);
    } else
        _err_msg("mmc_erase failure (%d).\n", ret);

    return ret;
}

void mmc_do_rw_ops(struct mmc_card *card, unsigned int rw_start, int rw_len)
{
    struct discard_context *dc = &card->discard_ctx;
    struct discard_region *found_rgn;
    struct discard_region *rgn;
    unsigned int overlap_start, trim_start, adjust_start;
    unsigned int overlap_len, trim_len, adjust_len;
    int ret;

    found_rgn = find_first_region(dc, rw_start, rw_len);

    while (found_rgn && !IS_ERR(found_rgn)) {
        _inc_stats(dc->overlap_cnt, 1);

        rgn = found_rgn;
        found_rgn = find_next_region(dc, rw_start, rw_len, rgn);

        overlap_start = MAX(rgn->start, rw_start);
        overlap_len = MIN(rgn->start + rgn->len - overlap_start, rw_start + rw_len - overlap_start);

        trim_start = overlap_start;
        trim_len = overlap_len;

        _dbg_msg("receive discard start(%u), len(%u)\n", trim_start, trim_len);
        ret = mmc_send_discard(card, rgn, DISCARD_RW_REQUEST | DISCARD_SEND_ALL, &trim_start, &trim_len);
        if (ret) {
            _err_msg("send_trim failure (%d).\n", ret);
            adjust_region(dc, rgn->start, rgn->len, rgn, NULL);
            continue;
        }

        if (trim_len > 0) {
            adjust_start = MIN(overlap_start, trim_start);
            adjust_len = MAX(overlap_start + overlap_len - adjust_start, trim_start + trim_len - adjust_start);
        }
        else {
            adjust_start = overlap_start;
            adjust_len = overlap_len;
        }

        /*
         * When evicting a region, we need to avoid the region that we are currently trimming.
         * And also, the next region that we already found. So we avoid the whole rw region.
         */
        try_adjust_region(dc, rgn, adjust_start, adjust_len, rw_start, rw_len);

    }

    if (IS_ERR(found_rgn)) {
        _err_msg("region search failure (%d).\n", (int)found_rgn);
    }
}

int mmc_do_discard_ops(struct mmc_card* card, unsigned int start, unsigned int len)
{
    struct discard_context *dc = &card->discard_ctx;
    struct discard_region *rgn, *new_rgn;
    unsigned int trim_start = 0;
    unsigned int trim_len = 0;
    int ret;
    _inc_stats(dc->recv_stats, len);
    _inc_stats(dc->discard_cnt, 1);

    _dbg_msg("receive discard start(%u), len(%u)\n", start, len);
    rgn = insert_region(dc, start, len, NULL);
    if (IS_ERR(rgn)) {
        ret = PTR_ERR(rgn);
        if (ret == -EAGAIN || ret == -ENOMEM) {
            ret = evict_region(dc, start, len, &new_rgn);
            if (ret == 0) {
                rgn = insert_region(dc, start, len, new_rgn);
                if (IS_ERR(rgn)) {
                    ret = PTR_ERR(rgn);
                    _err_msg("insert_region failure (%d).\n", ret);
                } else
                    goto out;
            } else if (ret == -EAGAIN) {  // Low LRU is empty
        // if tree insertion failed,  try to send trim ??
        if (len >= dc->threshold_size) {
            struct discard_region tmp_rgn;
            tmp_rgn.start = start;
            tmp_rgn.len = len;
                    ret = mmc_send_discard(card, &tmp_rgn, 0, &trim_start, &trim_len);
                    if (ret)
                        _err_msg("mmc_send_discard failure (%d).\n", ret);
        }
            } else
                _err_msg("evict_region failure (%d).\n", ret);
        } else if (ret)
            _err_msg("insert_region failure (%d).\n", ret);
        return ret;
    }

out:
    if (rgn->len < dc->threshold_size) {
        return 0;
    }
    ret = mmc_send_discard(card, rgn, 0, &trim_start, &trim_len);

    if (ret) {
        _err_msg("mmc_send_discard failure (%d).\n", ret);
        return ret;
    } else if (trim_len == 0)
        return 0;

    return try_adjust_region(dc, rgn, trim_start, trim_len, rgn->start, rgn->len);
}


int mmc_do_idle_ops(struct mmc_card *card)
{
    struct discard_context *dc = &card->discard_ctx;
    struct discard_region *rgn;
    unsigned int start, len;
    int ret;

    if (!card) {
        _err_msg("card is null.\n");
        return 0;
    }

    rgn = select_region(dc);
    if (!rgn)  // High LRU is empty
        return 0;
    else if (IS_ERR(rgn)) {
        _err_msg("select_region failure (%ld).\n", IS_ERR(rgn));
        return 0;
    }
    _inc_stats(dc->idle_cnt, 1);

    _dbg_msg("try send discard start(%u), len(%u)\n", rgn->start, rgn->len);
    ret = mmc_send_discard(card, rgn, 0, &start, &len);
    if (ret) {
        _err_msg("mmc_send_discard failure (%d).\n", ret);
        return 0;
    }

    try_adjust_region(dc, rgn, start, len, rgn->start, rgn->len);

    return 1;
}

int mmc_read_idle(struct mmc_card *card)
{
    return atomic_read(&card->discard_ctx.idle_flag);
}

void mmc_set_idle(struct mmc_card *card, int flag)
{
    struct discard_context *dc = &card->discard_ctx;

    atomic_set(&dc->idle_flag, flag);
    del_singleshot_timer_sync(&dc->idle_timer);
}

void mmc_clear_idle(struct mmc_card *card)
{
    struct discard_context *dc = &card->discard_ctx;

    atomic_cmpxchg(&dc->idle_flag, DCS_IDLE_OPS_TURNED_ON, DCS_IDLE_OPS_TURNED_OFF);
}

void mmc_trigger_idle_timer(struct mmc_card *card)
{
    struct discard_context *dc = &card->discard_ctx;

    dc->idle_timer.expires = jiffies + dc->idle_expires;
    add_timer(&dc->idle_timer);
}

static void mmc_idle_timeout(unsigned long data)
{
    struct discard_context *dc = (struct discard_context *)data;

    if (atomic_cmpxchg(&dc->idle_flag, DCS_IDLE_OPS_TURNED_OFF, DCS_IDLE_OPS_TURNED_ON)
        == DCS_IDLE_OPS_TURNED_OFF)
        wake_up_process(dc->idle_thread);
}

/*init/uninit discard context*/
static void _init_statistics(struct discard_context *dc)
{
    _set_stats(dc->recv_stats, 0);
    _set_stats(dc->send_stats, 0);
    _set_stats(dc->evic_stats, 0);
    _set_stats(dc->discard_cnt, 0);
    _set_stats(dc->dcd_new_cnt, 0);
    _set_stats(dc->dcd_mgd_cnt, 0);
    _set_stats(dc->idle_cnt, 0);
    _set_stats(dc->overlap_cnt, 0);
    _set_stats(dc->send_cnt, 0);
}

int mmc_discard_init(struct mmc_card* card)
{
    struct discard_context *dc;
    static int dc_id = 0;

    if (!card)
        return -EINVAL;

    dc = &card->discard_ctx;
    dc->rb_root.rb_node = NULL;
    INIT_LIST_HEAD(&dc->hlru);
    INIT_LIST_HEAD(&dc->llru);
    init_timer(&dc->idle_timer);
    dc->id = dc_id++;
    dc->idle_flag = (atomic_t)ATOMIC_INIT(DCS_IDLE_OPS_TURNED_OFF);

    return 0;
}

int mmc_discard_set_prop(struct mmc_card* card, unsigned int start,
            unsigned int len, struct task_struct *thread)
{
    struct discard_context *dc;

    if (!card || !thread)
        return -EINVAL;

    dc = &card->discard_ctx;
    dc->region_nr = 0;
    dc->max_region_nr = 4096;
    dc->optimal_size = card->pref_trim;
    dc->threshold_size = 64*1024;       /* in sectors, 32 MB */
    dc->maximum_size = 512*1024;        /* in sectors, 256 MB */
    dc->card_start = start;
    dc->card_len = len;
    dc->idle_timer.function = mmc_idle_timeout;
    dc->idle_timer.data = (unsigned long)dc;
    dc->idle_expires = msecs_to_jiffies(500);  /* 500 ms delay */
    dc->idle_thread = thread;

    /* proc_statistics init */
    _init_statistics(dc);
    _create_proc_entry(dc);

    return 0;
}

int mmc_discard_final(struct mmc_card* card)
{
    int ret;
    struct discard_context *dc;
    struct discard_region *rgn;
    unsigned int start, len;
    unsigned long s_time, now;
    int time_over = 0;

    if(!card)
        return -EINVAL;

    dc = &card->discard_ctx;
    s_time = jiffies;
    while(1)
    {
        rgn = select_region(dc);
        if(IS_ERR(rgn) || rgn == NULL)
            break;
        if(!time_over)
        {
            now = jiffies;
            if(now - s_time < DISCARD_FINAL_LIMIT_TIME)
            {
                ret = mmc_send_discard(card, rgn, DISCARD_SEND_ALL, &start, &len);
                if(ret)
                    _err_msg("send_trim failure.\n");
            }
            else
                time_over = 1;
        }
        adjust_region(dc, rgn->start, rgn->len, rgn, NULL);
    }

    while(1)
    {
        ret = evict_region(dc, 0, 0, NULL);
        if(ret != 0)
            break;
    }
    _remove_proc_entry(dc);

    return 0;
}

int mmc_create_slab(void)
{
    mmc_discard_region_slab = kmem_cache_create("mmc_discard_region_slab",
                                                sizeof(struct discard_region),
                                                0, SLAB_RECLAIM_ACCOUNT, NULL);
    if (mmc_discard_region_slab == NULL)
        return -ENOMEM;
    return 0;
}

void mmc_destroy_slab(void)
{
    kmem_cache_destroy(mmc_discard_region_slab);
    mmc_discard_region_slab = NULL;
}

#ifdef CONFIG_MMC_DISCARD_DEBUG
static void _dbg_check_magic(struct discard_region *rgn)
{
    if (unlikely(rgn->magic != DISCARD_RGN_MAGIC)) {
        _err_msg("bad discard region magic 0x%08x, must be 0x%08x\n", rgn->magic, DISCARD_RGN_MAGIC);
        BUG();
    }
}
static void _dbg_print_rbtree(struct discard_context *dc)
{
    struct rb_node* node;
    struct discard_region * rgn ;

    if (!dc)
        return;

    printk(KERN_INFO "rbtree: ");
    for (node = rb_first(&dc->rb_root); node; node = rb_next(node)) {
        rgn = rb_entry(node, struct discard_region, node);
        printk(KERN_INFO "(%8d, %8d) ", rgn->start, rgn->len);
    }
    printk(KERN_INFO "rgns_nr: %d\n", dc->region_nr);
}
static void _dbg_print_lru(struct discard_context *dc)
{
    struct list_head *node;
    struct discard_region *rgn;
    int i;

    if (!dc)
        return;

    printk(KERN_INFO "hlru   : ");
    for (node = dc->hlru.next, i = 1; node != &dc->hlru; node = node->next, i++) {
        rgn = list_entry(node, struct discard_region, list);
        printk(KERN_INFO "(%8d, %8d) ", rgn->start, rgn->len);
    }
    printk (KERN_INFO "\nllru   : ");
    for (node = dc->llru.next; node != &dc->llru; node = node->next) {
        rgn = list_entry(node, struct discard_region, list);
        printk(KERN_INFO "(%8d, %8d) ", rgn->start, rgn->len);
    }
    printk(KERN_INFO"\n");
}
#endif /* !CONFIG_MMC_DISCARD_DEBUG*/

#define PROCFS_NAME_MAXLEN  16
#define PROCFS_NAME         "mmc_discard"
static int _dbg_proc_read(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
    struct discard_context *dc = (struct discard_context*)data;
    struct list_head *node;
    int hc,lc;
    char *p;
    int len;

    if (offset > 0)
        len = 0;
    else {
        p = page;

        p += sprintf(p, "<Device Info>\n");
        p += sprintf(p, "     (start, length)         : (0x%x, 0x%x)\n", dc->card_start, dc->card_len);
        p += sprintf(p, "     (opti, thres, maxi)     : (0x%x, 0x%x, 0x%x)\n", dc->optimal_size, dc->threshold_size, dc->maximum_size);
        p += sprintf(p, "     (idle flag)             : (%d)\n", dc->idle_flag);

        for (node = dc->hlru.next, hc = 0; node != &dc->hlru; node = node->next, hc++);
        for (node = dc->llru.next, lc = 0; node != &dc->llru; node = node->next, lc++);    
	p += sprintf(p, "\n<Current buffered regions>\n");
        p += sprintf(p, "     (high, low, total)      : (%d, %d, %d)\n", hc, lc, dc->region_nr);

        p += sprintf(p, "\n<Sectors statistics>\n");
        p += sprintf(p, "     (receive, send, evict)  : (0x%x, 0x%x, 0x%x)\n", dc->recv_stats, dc->send_stats, dc->evic_stats);

        p += sprintf(p, "\n<Buffering counts statistics>\n");
        p += sprintf(p, "     (new, merged, total)    : (%d, %d, %d)\n", dc->dcd_new_cnt, dc->dcd_mgd_cnt, dc->discard_cnt);

        p += sprintf(p, "\n<Sending counts statistics>\n");
        p += sprintf(p, "     (overlap, idle, send)   : (%d, %d, %d)\n", dc->overlap_cnt, dc->idle_cnt, dc->send_cnt);

        len = p - page;
    }

    return len;
}

static int _dbg_proc_write(struct file * file, const char* buffer, unsigned long count, void* data)
{
    struct discard_context* dc = (struct discard_context*) data;
    char command;
    if ( count <= 0 )
        return 1;
    if ( copy_from_user( &command, buffer, 1 ))
        return -EFAULT;

    switch(command)
    {
    case 'r':
        _dbg_print_rbtree(dc);
        break;
    case 'l':
        _dbg_print_lru(dc);
        break;
    case '0':
        _init_statistics(dc);
        break;
    default :
        break;
    };
    return count;
}
static void _create_proc_entry(struct discard_context *dc)
{
    char name[PROCFS_NAME_MAXLEN];

    sprintf(name, PROCFS_NAME "%d", dc->id);
    dc->proc_entry = create_proc_entry(name, S_IWUGO | S_IRUGO | S_IFREG, NULL);
    if(dc->proc_entry == NULL) {
        _err_msg("Error: could not initialize /proc/%s\n", PROCFS_NAME);
        return;
    }
    dc->proc_entry->read_proc = _dbg_proc_read;
    dc->proc_entry->write_proc = _dbg_proc_write;
    dc->proc_entry->data = (void*)dc;
}

static void _remove_proc_entry(struct discard_context *dc)
{
    char name[PROCFS_NAME_MAXLEN];

    if(dc->proc_entry) {
        sprintf(name, PROCFS_NAME "%d", dc->id);
        remove_proc_entry(name, NULL);
    }
}

/* end of discard.c */

