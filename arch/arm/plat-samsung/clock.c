/* linux/arch/arm/plat-s3c24xx/clock.c
 *
 * Copyright 2004-2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX Core clock control support
 *
 * Based on, and code from linux/arch/arm/mach-versatile/clock.c
 **
 **  Copyright (C) 2004 ARM Limited.
 **  Written by Deep Blue Solutions Limited.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include <mach/hardware.h>
#include <asm/irq.h>

#include <plat/cpu-freq.h>

#include <plat/clock.h>
#include <plat/cpu.h>

/* clock information */

static LIST_HEAD(clocks);

/* We originally used an mutex here, but some contexts (see resume)
 * are calling functions such as clk_set_parent() with IRQs disabled
 * causing an BUG to be triggered.
 */
DEFINE_SPINLOCK(clocks_lock);

/* enable and disable calls for use with the clk struct */

static int clk_null_enable(struct clk *clk, int enable)
{
	return 0;
}

/* Clock API calls */

struct clk *clk_get(struct device *dev, const char *id)
{
	unsigned long flags;
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);
	int idno;

#if defined(CONFIG_ARCH_S5P6450)
	if (dev == NULL || dev->bus != &platform_bus_type)
		idno = -1;
	else
	idno = to_platform_device(dev)->id;
#else	
	if (dev == NULL)
		idno = -1;
	else {
		if (to_platform_device(dev)->id)
			idno = to_platform_device(dev)->id;
		else if (dev->bus != &platform_bus_type)
			idno = -1;
		else
			idno = to_platform_device(dev)->id;
	}
#endif
	spin_lock_irqsave(&clocks_lock, flags);

	list_for_each_entry(p, &clocks, list) {
		if (p->id == idno &&
		    strcmp(id, p->name) == 0 &&
		    try_module_get(p->owner)) {
			clk = p;
			break;
		}
	}

	/* check for the case where a device was supplied, but the
	 * clock that was being searched for is not device specific */

	if (IS_ERR(clk)) {
		list_for_each_entry(p, &clocks, list) {
			if (p->id == -1 && strcmp(id, p->name) == 0 &&
			    try_module_get(p->owner)) {
				clk = p;
				break;
			}
		}
	}

	spin_unlock_irqrestore(&clocks_lock, flags);
	return clk;
}

void clk_put(struct clk *clk)
{
	module_put(clk->owner);
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	clk_enable(clk->parent);

	spin_lock_irqsave(&clocks_lock, flags);

	if ((clk->usage++) == 0)
		(clk->enable)(clk, 1);

	spin_unlock_irqrestore(&clocks_lock, flags);
	return 0;
}

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	if (IS_ERR(clk) || clk == NULL)
		return;

	spin_lock_irqsave(&clocks_lock, flags);

	if ((--clk->usage) == 0)
		(clk->enable)(clk, 0);

	spin_unlock_irqrestore(&clocks_lock, flags);
	clk_disable(clk->parent);
}


unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk))
		return 0;

	if (clk->rate != 0)
		return clk->rate;

	if (clk->ops != NULL && clk->ops->get_rate != NULL)
		return (clk->ops->get_rate)(clk);

	if (clk->parent != NULL)
		return clk_get_rate(clk->parent);

	return clk->rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (!IS_ERR(clk) && clk->ops && clk->ops->round_rate)
		return (clk->ops->round_rate)(clk, rate);

	return rate;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long flags;
	int ret;

	if (IS_ERR(clk))
		return -EINVAL;

	/* We do not default just do a clk->rate = rate as
	 * the clock may have been made this way by choice.
	 */

	WARN_ON(clk->ops == NULL);
	WARN_ON(clk->ops && clk->ops->set_rate == NULL);

	if (clk->ops == NULL || clk->ops->set_rate == NULL)
		return -EINVAL;

	spin_lock_irqsave(&clocks_lock, flags);
	ret = (clk->ops->set_rate)(clk, rate);
	spin_unlock_irqrestore(&clocks_lock, flags);

	return ret;
}

struct clk *clk_get_parent(struct clk *clk)
{
	return clk->parent;
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	unsigned long flags;
	int ret = 0;

	if (IS_ERR(clk))
		return -EINVAL;

	spin_lock_irqsave(&clocks_lock, flags);

	if (clk->ops && clk->ops->set_parent)
		ret = (clk->ops->set_parent)(clk, parent);

	spin_unlock_irqrestore(&clocks_lock, flags);

	return ret;
}

EXPORT_SYMBOL(clk_get);
EXPORT_SYMBOL(clk_put);
EXPORT_SYMBOL(clk_enable);
EXPORT_SYMBOL(clk_disable);
EXPORT_SYMBOL(clk_get_rate);
EXPORT_SYMBOL(clk_round_rate);
EXPORT_SYMBOL(clk_set_rate);
EXPORT_SYMBOL(clk_get_parent);
EXPORT_SYMBOL(clk_set_parent);

/* base clocks */

int clk_default_setrate(struct clk *clk, unsigned long rate)
{
	clk->rate = rate;
	return 0;
}

struct clk_ops clk_ops_def_setrate = {
	.set_rate	= clk_default_setrate,
};

struct clk clk_xtal = {
	.name		= "xtal",
	.id		= -1,
	.rate		= 0,
	.parent		= NULL,
	.ctrlbit	= 0,
};

struct clk clk_ext = {
	.name		= "ext",
	.id		= -1,
};

struct clk clk_epll = {
	.name		= "epll",
	.id		= -1,
};

struct clk clk_mpll = {
	.name		= "mpll",
	.id		= -1,
	.ops		= &clk_ops_def_setrate,
};

struct clk clk_upll = {
	.name		= "upll",
	.id		= -1,
	.parent		= NULL,
	.ctrlbit	= 0,
};

struct clk clk_f = {
	.name		= "fclk",
	.id		= -1,
	.rate		= 0,
	.parent		= &clk_mpll,
	.ctrlbit	= 0,
};

struct clk clk_h = {
	.name		= "hclk",
	.id		= -1,
	.rate		= 0,
	.parent		= NULL,
	.ctrlbit	= 0,
	.ops		= &clk_ops_def_setrate,
};

struct clk clk_p = {
	.name		= "pclk",
	.id		= -1,
	.rate		= 0,
	.parent		= NULL,
	.ctrlbit	= 0,
	.ops		= &clk_ops_def_setrate,
};

struct clk clk_usb_bus = {
	.name		= "usb-bus",
	.id		= -1,
	.rate		= 0,
	.parent		= &clk_upll,
};


struct clk s3c24xx_uclk = {
	.name		= "uclk",
	.id		= -1,
};

#ifdef CONFIG_DEBUG_FS

static void print_clk(struct seq_file *s, struct clk *p, int leaf)
{
	if (!p)
		return;

	print_clk(s, p->parent, 0);
	seq_printf(s, "%s", p->name);
	if (p->id >= 0)
		seq_printf(s, ".%d", p->id);

	if (leaf)
		seq_printf(s, "(%ld, %d)\n", clk_get_rate(p), p->usage);
	else
		seq_printf(s, " -> ");
}

static int s3c24xx_clock_show(struct seq_file *s, void *unused)
{
	unsigned long flags;
	struct clk *p;

	spin_lock_irqsave(&clocks_lock, flags);

	list_for_each_entry(p, &clocks, list) {
		if (s->private || p->usage)
			print_clk(s, p, 1);
	}

	spin_unlock_irqrestore(&clocks_lock, flags);

	return 0;
}

static int s3c24xx_clock_open(struct inode *inode, struct file *file)
{
	return single_open(file, s3c24xx_clock_show, inode->i_private);
}

static const struct file_operations s3c24xx_clock_operations = {
	.open		= s3c24xx_clock_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init s3c24xx_clock_debugfs_init(void)
{
	/* /sys/kernel/debug/s3c24xx_clock */
	struct dentry *dir = debugfs_create_dir("s3c24xx_clock", NULL);
	if (dir) {
		debugfs_create_file("all", S_IFREG | S_IRUGO, dir, (void *)1,
				    &s3c24xx_clock_operations);
		debugfs_create_file("on", S_IFREG | S_IRUGO, dir, (void *)0,
				    &s3c24xx_clock_operations);
	}
	return 0;
}
postcore_initcall(s3c24xx_clock_debugfs_init);

#endif

/* initialise the clock system */

/**
 * s3c24xx_register_clock() - register a clock
 * @clk: The clock to register
 *
 * Add the specified clock to the list of clocks known by the system.
 */
int s3c24xx_register_clock(struct clk *clk)
{
	unsigned long flags;
	if (clk->enable == NULL)
		clk->enable = clk_null_enable;

	/* add to the list of available clocks */

	/* Quick check to see if this clock has already been registered. */
	BUG_ON(clk->list.prev != clk->list.next);

	spin_lock_irqsave(&clocks_lock, flags);
	list_add(&clk->list, &clocks);
	spin_unlock_irqrestore(&clocks_lock, flags);

	return 0;
}

/**
 * s3c24xx_register_clocks() - register an array of clock pointers
 * @clks: Pointer to an array of struct clk pointers
 * @nr_clks: The number of clocks in the @clks array.
 *
 * Call s3c24xx_register_clock() for all the clock pointers contained
 * in the @clks list. Returns the number of failures.
 */
int s3c24xx_register_clocks(struct clk **clks, int nr_clks)
{
	int fails = 0;

	for (; nr_clks > 0; nr_clks--, clks++) {
		if (s3c24xx_register_clock(*clks) < 0) {
			struct clk *clk = *clks;
			printk(KERN_ERR "%s: failed to register %p: %s\n",
			       __func__, clk, clk->name);
			fails++;
		}
	}

	return fails;
}

/**
 * s3c_register_clocks() - register an array of clocks
 * @clkp: Pointer to the first clock in the array.
 * @nr_clks: Number of clocks to register.
 *
 * Call s3c24xx_register_clock() on the @clkp array given, printing an
 * error if it fails to register the clock (unlikely).
 */
void __init s3c_register_clocks(struct clk *clkp, int nr_clks)
{
	int ret;

	for (; nr_clks > 0; nr_clks--, clkp++) {
		ret = s3c24xx_register_clock(clkp);

		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}
}

/**
 * s3c_disable_clocks() - disable an array of clocks
 * @clkp: Pointer to the first clock in the array.
 * @nr_clks: Number of clocks to register.
 *
 * for internal use only at initialisation time. disable the clocks in the
 * @clkp array.
 */

void __init s3c_disable_clocks(struct clk *clkp, int nr_clks)
{
	for (; nr_clks > 0; nr_clks--, clkp++)
		(clkp->enable)(clkp, 0);
}

#ifdef CONFIG_S5PV310_FPGA
static struct clk tmp_clocks[] = {
};
#endif

/* initalise all the clocks */

int __init s3c24xx_register_baseclocks(unsigned long xtal)
{
	printk(KERN_INFO "S3C24XX Clocks, Copyright 2004 Simtec Electronics\n");

	clk_xtal.rate = xtal;

	/* register our clocks */

	if (s3c24xx_register_clock(&clk_xtal) < 0)
		printk(KERN_ERR "failed to register master xtal\n");

	if (s3c24xx_register_clock(&clk_mpll) < 0)
		printk(KERN_ERR "failed to register mpll clock\n");

	if (s3c24xx_register_clock(&clk_upll) < 0)
		printk(KERN_ERR "failed to register upll clock\n");

	if (s3c24xx_register_clock(&clk_f) < 0)
		printk(KERN_ERR "failed to register cpu fclk\n");

	if (s3c24xx_register_clock(&clk_h) < 0)
		printk(KERN_ERR "failed to register cpu hclk\n");

	if (s3c24xx_register_clock(&clk_p) < 0)
		printk(KERN_ERR "failed to register cpu pclk\n");

#ifdef CONFIG_S5PV310_FPGA
	s3c_register_clocks(tmp_clocks, ARRAY_SIZE(tmp_clocks));
#endif
	return 0;
}

