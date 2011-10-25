/* linux/arch/arm/mach-s5pv310/mct.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 (and compatible) HRT support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <mach/map.h>
#include <mach/regs-mct.h>
#include <asm/mach/time.h>

static unsigned long clk_cnt_per_tick;
static unsigned long clk_rate;
static cycle_t time_suspended;
static u32 sched_mult;
static u32 sched_shift;

enum mct_tick_type {
	TICK0,
	TICK1,
};

static void s5pv310_mct_write(unsigned int value, void *addr)
{
	void __iomem *stat_addr;
	u32 mask;
	u32 i;

	__raw_writel(value, addr);

	switch ((u32) addr) {
	case (u32) S5PV310_MCT_G_TCON:
		stat_addr = S5PV310_MCT_G_WSTAT;
		mask = G_WSTAT_TCON_STAT;
		break;
	case (u32) S5PV310_MCT_G_CNT_L:
		stat_addr = S5PV310_MCT_G_CNT_WSTAT;
		mask = G_CNT_L_STAT;
		break;
	case (u32) S5PV310_MCT_G_CNT_U:
		stat_addr = S5PV310_MCT_G_CNT_WSTAT;
		mask = G_CNT_U_STAT;
		break;
	case (u32) S5PV310_MCT_L0_TCON:
		stat_addr = S5PV310_MCT_L0_WSTAT;
		mask = L_TCON_STAT;
		break;
	case (u32) S5PV310_MCT_L1_TCON:
		stat_addr = S5PV310_MCT_L1_WSTAT;
		mask = L_TCON_STAT;
		break;
	case (u32) S5PV310_MCT_L0_TCNTB:
		stat_addr = S5PV310_MCT_L0_WSTAT;
		mask = L_TCNTB_STAT;
		break;
	case (u32) S5PV310_MCT_L1_TCNTB:
		stat_addr = S5PV310_MCT_L1_WSTAT;
		mask = L_TCNTB_STAT;
		break;
	case (u32) S5PV310_MCT_L0_ICNTB:
		stat_addr = S5PV310_MCT_L0_WSTAT;
		mask = L_ICNTB_STAT;
		break;
	case (u32) S5PV310_MCT_L1_ICNTB:
		stat_addr = S5PV310_MCT_L1_WSTAT;
		mask = L_ICNTB_STAT;
		break;
	default:
		return;
	}

	/* Wait maximum 1 ms until written values are applied */
	for (i = 0; i < loops_per_jiffy / 1000 * HZ; i++)
		if (__raw_readl(stat_addr) & mask) {
			__raw_writel(mask, stat_addr);
			return;
		}

	/* Workaround: Try again if fail */
	__raw_writel(value, addr);

	for (i = 0; i < loops_per_jiffy / 1000 * HZ; i++)
		if (__raw_readl(stat_addr) & mask) {
			__raw_writel(mask, stat_addr);
			return;
		}

	panic("mct hangs after writing %d to 0x%x address\n", value, (u32) addr);
}

static void s5pv310_mct_tick_stop(enum mct_tick_type type)
{
	unsigned long reg;
	void __iomem *addr;

	if (type == TICK0)
		addr = S5PV310_MCT_L0_TCON;
	else
		addr = S5PV310_MCT_L1_TCON;

	reg = __raw_readl(addr);
	reg &= ~(L_TCON_INT_START | L_TCON_TIMER_START);
	s5pv310_mct_write(reg, addr);
}

static void s5pv310_mct_tick_start(enum mct_tick_type type,
					unsigned long cycles)
{
	unsigned long reg;
	void __iomem *addr;

	s5pv310_mct_tick_stop(type);

	if (type == TICK0) {
		s5pv310_mct_write(L_UPDATE_ICNTB | cycles, S5PV310_MCT_L0_ICNTB);
		s5pv310_mct_write(L_INT_ENB_CNTIE, S5PV310_MCT_L0_INT_ENB);
		addr = S5PV310_MCT_L0_TCON;
	} else {
		s5pv310_mct_write(L_UPDATE_ICNTB | cycles, S5PV310_MCT_L1_ICNTB);
		s5pv310_mct_write(L_INT_ENB_CNTIE, S5PV310_MCT_L1_INT_ENB);
		addr = S5PV310_MCT_L1_TCON;
	}

	reg = __raw_readl(addr);
	reg |= L_TCON_INT_START | L_TCON_TIMER_START | L_TCON_INTERVAL_MODE;
	s5pv310_mct_write(reg, addr);
}

static inline int s5pv310_tick_set_next_event(enum mct_tick_type type,
					unsigned long cycles)
{
	s5pv310_mct_tick_start(type, cycles);
	return 0;
}

static inline void s5pv310_tick_set_mode(enum mct_tick_type type,
				enum clock_event_mode mode)
{
	s5pv310_mct_tick_stop(type);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		s5pv310_mct_tick_start(type, clk_cnt_per_tick);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static int s5pv310_tick0_set_next_event(unsigned long cycles,
					struct clock_event_device *evt)
{
	return s5pv310_tick_set_next_event(TICK0, cycles);
}

static int s5pv310_tick1_set_next_event(unsigned long cycles,
					struct clock_event_device *evt)
{
	return s5pv310_tick_set_next_event(TICK1, cycles);
}

static void s5pv310_tick0_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	s5pv310_tick_set_mode(TICK0, mode);
}

static void s5pv310_tick1_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	s5pv310_tick_set_mode(TICK1, mode);
}

static struct clock_event_device mct_tick0_device = {
	.name		= "mct-tick0",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 450,
	.set_next_event	= s5pv310_tick0_set_next_event,
	.set_mode	= s5pv310_tick0_set_mode,
};

static struct clock_event_device mct_tick1_device = {
	.name		= "mct-tick1",
	.features       = CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 450,
	.set_next_event	= s5pv310_tick1_set_next_event,
	.set_mode	= s5pv310_tick1_set_mode,
};

irqreturn_t s5pv310_mct0_event_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt;

	/* Clear the mct tick interrupt */
	evt = &mct_tick0_device;
	if (evt->mode != CLOCK_EVT_MODE_PERIODIC)
		s5pv310_mct_tick_stop(TICK0);

	evt->event_handler(evt);

	s5pv310_mct_write(0x1, S5PV310_MCT_L0_INT_CSTAT);
	/* Limitation for MCT Timer */
	s5pv310_mct_write(0x1, S5PV310_MCT_L0_INT_CSTAT);

	return IRQ_HANDLED;
}

irqreturn_t s5pv310_mct1_event_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt;

	/* Clear the mct tick interrupt */
	evt = &mct_tick1_device;
	if (evt->mode != CLOCK_EVT_MODE_PERIODIC)
		s5pv310_mct_tick_stop(TICK1);

	evt->event_handler(evt);

	s5pv310_mct_write(0x1, S5PV310_MCT_L1_INT_CSTAT);
	/* Limitation for MCT Timer */
	s5pv310_mct_write(0x1, S5PV310_MCT_L1_INT_CSTAT);

	return IRQ_HANDLED;
}

static struct irqaction mct_tick0_event_irq = {
	.name		= "mct_tick0_irq",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL | IRQF_NOBALANCING,
	.handler	= s5pv310_mct0_event_isr,
};

static struct irqaction mct_tick1_event_irq = {
	.name		= "mct_tick1_irq",
	.flags		= IRQF_TIMER | IRQF_NOBALANCING,
	.handler	= s5pv310_mct1_event_isr,
};

static void s5pv310_mct_clockevent_init(struct clock_event_device *dev)
{
	clockevents_calc_mult_shift(dev, clk_rate / 2, 5);
	dev->max_delta_ns =
		clockevent_delta2ns(0x7fffffff, dev);
	dev->min_delta_ns =
		clockevent_delta2ns(0xf, dev);
}

static void __init s5pv310_clockevent0_init(void)
{
	clk_cnt_per_tick = clk_rate / 2	/ HZ;

	s5pv310_mct_write(0x1, S5PV310_MCT_L0_TCNTB);
	s5pv310_mct_clockevent_init(&mct_tick0_device);
	mct_tick0_device.cpumask = cpumask_of(0);
	clockevents_register_device(&mct_tick0_device);

	setup_irq(IRQ_MCT_L0, &mct_tick0_event_irq);
	setup_irq(IRQ_MCT_L1, &mct_tick1_event_irq);
}

static void s5pv310_clockevent1_init(void)
{
	s5pv310_mct_write(0x1, S5PV310_MCT_L1_TCNTB);
	s5pv310_mct_clockevent_init(&mct_tick1_device);
	mct_tick1_device.cpumask = cpumask_of(1);
	clockevents_register_device(&mct_tick1_device);


#ifdef CONFIG_USE_EXT_GIC
	irq_set_affinity(IRQ_MCT_L1, cpumask_of(1));
#else
	irq_set_affinity(IRQ_MCT1, cpumask_of(1));
#endif
}

#ifdef CONFIG_LOCAL_TIMERS
/*
 * Setup the local clock events for a CPU.
 */
void __cpuinit local_timer_setup(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();

	/* For cpu 1 */
	if (cpu == 1)
		s5pv310_clockevent1_init();
}

int local_timer_ack(void)
{
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
void local_timer_stop(void)
{
	unsigned int cpu = smp_processor_id();
	/* For cpu 1 */
	if (cpu == 1) {
		s5pv310_mct_tick_stop(TICK1);

		s5pv310_mct_write(0x1, S5PV310_MCT_L1_INT_CSTAT);
		/* Limitation for MCT Timer */
		s5pv310_mct_write(0x1, S5PV310_MCT_L1_INT_CSTAT);
	}
}
#endif

#endif

static void s5pv310_mct_frc_start(u32 hi, u32 lo)
{
	u32 reg;

	s5pv310_mct_write(lo, S5PV310_MCT_G_CNT_L);
	s5pv310_mct_write(hi, S5PV310_MCT_G_CNT_U);

	reg = __raw_readl(S5PV310_MCT_G_TCON);
	reg |= G_TCON_START;
	s5pv310_mct_write(reg, S5PV310_MCT_G_TCON);
}

/*
 * Clocksource handling
 */
static cycle_t s5pv310_frc_read(struct clocksource *cs)
{
	unsigned int lo, hi;
	u32 hi2 = __raw_readl(S5PV310_MCT_G_CNT_U);

	do {
		hi = hi2;
		lo = __raw_readl(S5PV310_MCT_G_CNT_L);
		hi2 = __raw_readl(S5PV310_MCT_G_CNT_U);
	} while (hi != hi2);

	return ((cycle_t)hi << 32) | lo;
}

static void s5pv310_frc_suspend(struct clocksource *cs)
{
	time_suspended = s5pv310_frc_read(cs);
};

static void s5pv310_frc_resume(struct clocksource *cs)
{
	s5pv310_mct_write(0x1, S5PV310_MCT_L0_TCNTB);
	s5pv310_mct_frc_start((u32)(time_suspended >> 32), (u32)time_suspended);
};

struct clocksource mct_frc = {
	.name		= "mct-frc",
	.rating		= 400,
	.read		= s5pv310_frc_read,
	.mask		= CLOCKSOURCE_MASK(64),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
	.suspend	= s5pv310_frc_suspend,
	.resume		= s5pv310_frc_resume,
};

static void __init s5pv310_clocksource_init(void)
{
	s5pv310_mct_frc_start(0, 0);

	if (clocksource_register_hz(&mct_frc, clk_rate))
		panic("%s: can't register clocksource\n", mct_frc.name);

	sched_shift = 16;
	sched_mult = clocksource_khz2mult(clk_rate / 1000, sched_shift);
}

unsigned long long sched_clock(void)
{
	struct clocksource *cs = &mct_frc;

	return clocksource_cyc2ns(cs->read(NULL), sched_mult, sched_shift);
}

static void __init s5pv310_timer_resources(void)
{
	struct clk *mct_clk;
	mct_clk = clk_get(NULL, "xtal");

	clk_rate = clk_get_rate(mct_clk);
}

static void __init s5pv310_timer_init(void)
{
	s5pv310_timer_resources();
	s5pv310_clockevent0_init();
	s5pv310_clocksource_init();
}

struct sys_timer s5pv310_timer = {
	.init		= s5pv310_timer_init,
};
