/* linux/arch/arm/mach-s5pv310/time.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 (and compatible) HRT support
 * System timer is used for this feature
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/cnt32_to_63.h>
#include <linux/platform_device.h>

#include <asm/smp_twd.h>

#include <mach/regs-systimer.h>
#include <plat/regs-timer.h>
#include <asm/mach/time.h>

enum systimer_type {
	SYS_FRC,
	SYS_TICK,
};

static unsigned long systimer_clk_cnt_per_tick;
static unsigned long pwm_clk_cnt_per_tick;

static struct clk *systimer_clk;
static struct clk *pwm_clk;
static struct clk *tin4;
static struct clk *tdiv4;

/*
 * Clockevent handling.
 */
static void s5pv310_systimer_write(unsigned int value,	void *reg_offset)
{
	unsigned int temp_regs;

	__raw_writel(value, reg_offset);

	if (reg_offset == S5PV310_TCON) {
		while (!(__raw_readl(S5PV310_INT_CSTAT) & 1<<7));

		temp_regs = __raw_readl(S5PV310_INT_CSTAT);
		temp_regs |= 1<<7;
		__raw_writel(temp_regs, S5PV310_INT_CSTAT);

	} else if (reg_offset == S5PV310_TICNTB) {
		while (!(__raw_readl(S5PV310_INT_CSTAT) & 1<<3));

		temp_regs = __raw_readl(S5PV310_INT_CSTAT);
		temp_regs |= 1<<3;
		__raw_writel(temp_regs, S5PV310_INT_CSTAT);

	} else if (reg_offset == S5PV310_FRCNTB) {
		while (!(__raw_readl(S5PV310_INT_CSTAT) & 1<<6));

		temp_regs = __raw_readl(S5PV310_INT_CSTAT);
		temp_regs |= 1<<6;
		__raw_writel(temp_regs, S5PV310_INT_CSTAT);
	}
}

static void s5pv310_systimer_stop(enum systimer_type type)
{
	unsigned long tcon;
	unsigned long tcfg;

	tcon = __raw_readl(S5PV310_TCON);

	switch (type) {
	case SYS_FRC:
		tcon &= ~S5PV310_TCON_FRC_START;
		break;

	case SYS_TICK:
		tcon &= ~(S5PV310_TCON_TICK_START | S5PV310_TCON_TICK_INT_START);
		break;

	default:
		break;
	}

	s5pv310_systimer_write(tcon, S5PV310_TCON);

	tcfg = __raw_readl(S5PV310_TCFG);
	tcfg |= S5PV310_TCFG_TICK_SWRST;
	s5pv310_systimer_write(tcfg, S5PV310_TCFG);
}

static void s5pv310_systimer_start(enum systimer_type type, unsigned long tcnt)
{
	unsigned long tcon;

	s5pv310_systimer_stop(type);

	tcon = __raw_readl(S5PV310_TCON);

	switch (type) {
	case SYS_FRC:
		s5pv310_systimer_write(tcnt, S5PV310_FRCNTB);
		tcon |= S5PV310_TCON_FRC_START;
		break;

	case SYS_TICK:
		while (__raw_readl(S5PV310_TCFG) & S5PV310_TCFG_TICK_SWRST);

		s5pv310_systimer_write(tcnt, S5PV310_TICNTB);

		tcon |= S5PV310_TCON_TICK_INT_START;
		s5pv310_systimer_write(tcon, S5PV310_TCON);

		tcon |= S5PV310_TCON_TICK_START;
		break;

	default:
		break;
	}

	s5pv310_systimer_write(tcon, S5PV310_TCON);
}

static int s5pv310_systimer_set_next_event(unsigned long cycles,
					struct clock_event_device *evt)
{
	s5pv310_systimer_start(SYS_TICK, cycles);
	return 0;
}

static void s5pv310_systimer_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	s5pv310_systimer_stop(SYS_TICK);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		s5pv310_systimer_start(SYS_TICK, systimer_clk_cnt_per_tick);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device sys_tick_event_device = {
	.name		= "sys_tick",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 250,
	.shift		= 20,
	.set_next_event	= s5pv310_systimer_set_next_event,
	.set_mode	= s5pv310_systimer_set_mode,
};

irqreturn_t s5pv310_systimer_event_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt = &sys_tick_event_device;
	unsigned long int_status;

	if (evt->mode != CLOCK_EVT_MODE_PERIODIC)
		s5pv310_systimer_stop(SYS_TICK);

	/* Clear the system tick interrupt */
	int_status = __raw_readl(S5PV310_INT_CSTAT);
	int_status |= S5PV310_INT_TICK_STATUS;
	s5pv310_systimer_write(int_status, S5PV310_INT_CSTAT);

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction s5pv310_systimer_event_irq = {
	.name		= "sys_tick_irq",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL | IRQF_NOBALANCING,
	.handler	= s5pv310_systimer_event_isr,
};

static void __init systimer_clockevent_init(void)
{
	unsigned long clock_rate;

	clock_rate = clk_get_rate(systimer_clk);

	systimer_clk_cnt_per_tick = clock_rate / HZ;

	sys_tick_event_device.mult =
		div_sc(clock_rate, NSEC_PER_SEC, sys_tick_event_device.shift);
	sys_tick_event_device.max_delta_ns =
		clockevent_delta2ns(-1, &sys_tick_event_device);
	sys_tick_event_device.min_delta_ns =
		clockevent_delta2ns(0x1, &sys_tick_event_device);

	sys_tick_event_device.cpumask = cpumask_of(0);
	clockevents_register_device(&sys_tick_event_device);

	s5pv310_systimer_write(S5PV310_INT_TICK_EN | S5PV310_INT_EN, S5PV310_INT_CSTAT);
	setup_irq(IRQ_SYSTEM_TIMER, &s5pv310_systimer_event_irq);
}

static void s5pv310_pwm_stop(unsigned int pwm_id)
{
	unsigned long tcon;

	tcon = __raw_readl(S3C2410_TCON);

	switch (pwm_id) {
	case 2:
		tcon &= ~S3C2410_TCON_T2START;
		break;
	case 4:
		tcon &= ~S3C2410_TCON_T4START;
		break;
	default:
		break;
	}
	__raw_writel(tcon, S3C2410_TCON);
}

static void s5pv310_pwm_init(unsigned int pwm_id, unsigned long tcnt)
{
	unsigned long tcon;

	tcon = __raw_readl(S3C2410_TCON);

	/* timers reload after counting zero, so reduce the count by 1 */
	tcnt--;

	/* ensure timer is stopped... */
	switch (pwm_id) {
	case 2:
		tcon &= ~(0xf<<12);
		tcon |= S3C2410_TCON_T2MANUALUPD;

		__raw_writel(tcnt, S3C2410_TCNTB(2));
		__raw_writel(tcnt, S3C2410_TCMPB(2));
		__raw_writel(tcon, S3C2410_TCON);

		break;
	case 4:
		tcon &= ~(7<<20);
		tcon |= S3C2410_TCON_T4MANUALUPD;

		__raw_writel(tcnt, S3C2410_TCNTB(4));
		__raw_writel(tcnt, S3C2410_TCMPB(4));
		__raw_writel(tcon, S3C2410_TCON);

		break;
	default:
		break;
	}
}

static inline void s5pv310_pwm_start(unsigned int pwm_id, bool periodic)
{
	unsigned long tcon;

	tcon  = __raw_readl(S3C2410_TCON);

	switch (pwm_id) {
	case 2:
		tcon |= S3C2410_TCON_T2START;
		tcon &= ~S3C2410_TCON_T2MANUALUPD;

		if (periodic)
			tcon |= S3C2410_TCON_T2RELOAD;
		else
			tcon &= ~S3C2410_TCON_T2RELOAD;
		break;
	case 4:
		tcon |= S3C2410_TCON_T4START;
		tcon &= ~S3C2410_TCON_T4MANUALUPD;

		if (periodic)
			tcon |= S3C2410_TCON_T4RELOAD;
		else
			tcon &= ~S3C2410_TCON_T4RELOAD;
		break;
	default:
		break;
	}
	__raw_writel(tcon, S3C2410_TCON);
}

static int s5pv310_pwm_set_next_event(unsigned long cycles,
					struct clock_event_device *evt)
{
	s5pv310_pwm_init(4, cycles);
	s5pv310_pwm_start(4, 0);
	return 0;
}

static void s5pv310_pwm_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	s5pv310_pwm_stop(4);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		s5pv310_pwm_init(4, pwm_clk_cnt_per_tick);
		s5pv310_pwm_start(4, 1);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device pwm_event_device = {
	.name		= "pwm_timer4",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 250,
	.shift		= 20,
	.set_next_event	= s5pv310_pwm_set_next_event,
	.set_mode	= s5pv310_pwm_set_mode,
};

irqreturn_t s5pv310_pwm_event_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt = &pwm_event_device;

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static void __init pwm_clockevent_init(void)
{
	unsigned long pclk;
	unsigned long clock_rate;
	struct clk *tscaler;

	unsigned int stat = __raw_readl(S3C64XX_TINT_CSTAT) & 0x1f;

	pclk = clk_get_rate(pwm_clk);

	/* configure clock tick */
	tscaler = clk_get_parent(tdiv4);

	clk_set_rate(tscaler, pclk / 2);
	clk_set_rate(tdiv4, pclk / 2);
	clk_set_parent(tin4, tdiv4);

	clock_rate = clk_get_rate(tin4);

	pwm_clk_cnt_per_tick = clock_rate / HZ;

	pwm_event_device.mult =
		div_sc(clock_rate, NSEC_PER_SEC, pwm_event_device.shift);
	pwm_event_device.max_delta_ns =
		clockevent_delta2ns(-1, &pwm_event_device);
	pwm_event_device.min_delta_ns =
		clockevent_delta2ns(1, &pwm_event_device);

	pwm_event_device.cpumask = cpumask_of(1);

	stat |= 1 << 9;
	__raw_writel(stat, S3C64XX_TINT_CSTAT);
}

static void __init s5pv310_clockevent_init(void)
{
	systimer_clockevent_init();
	pwm_clockevent_init();
}

#ifdef CONFIG_LOCAL_TIMERS

static struct irqaction s5pv310_pwm_event_irq = {
	.name		= "pwm_timer4_irq",
	.flags		= IRQF_TIMER | IRQF_NOBALANCING,
	.handler	= s5pv310_pwm_event_isr,
};

/*
 *  * Setup the local clock events for a CPU.
 *   */
void __cpuinit local_timer_setup(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();
	unsigned int stat;

	/* For cpu 1 */
	if (cpu == 1) {
		clockevents_register_device(&pwm_event_device);
		irq_set_affinity(IRQ_SPI(22), cpumask_of(1));
		setup_irq(IRQ_TIMER4, &s5pv310_pwm_event_irq);

		stat = __raw_readl(S3C64XX_TINT_CSTAT) & 0x1f;
		stat = 1 << 4;
		__raw_writel(stat, S3C64XX_TINT_CSTAT);
	}
}

int local_timer_ack(void)
{
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
void local_timer_stop(void)
{
	unsigned int stat = __raw_readl(S3C64XX_TINT_CSTAT) & 0x1f;

	s5pv310_pwm_stop(4);

	stat &= ~(1 << 4);
	stat |= 1 << 9;

	__raw_writel(stat, S3C64XX_TINT_CSTAT);
}
#endif

#endif

/*
 * Clocksource handling
 */
static cycle_t s5pv310_frc_read(struct clocksource *cs)
{
	return (cycle_t) (0xffffffff - __raw_readl(S5PV310_FRCNTO));
}

struct clocksource sys_frc_clksrc = {
	.name		= "sys_frc",
	.rating		= 250,
	.read		= s5pv310_frc_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS ,
};

static void __init s5pv310_clocksource_init(void)
{
	unsigned long clock_rate;

	clock_rate = clk_get_rate(systimer_clk);

	s5pv310_systimer_start(SYS_FRC, 0xffffffff);

	if (clocksource_register_hz(&sys_frc_clksrc, clock_rate))
		panic("%s: can't register clocksource\n", sys_frc_clksrc.name);
}

static void __init s5pv310_timer_resources(void)
{
	unsigned long tcfg;
	struct platform_device tmpdev;

	tcfg = __raw_readl(S5PV310_TCFG);
	tcfg &= ~S5PV310_TCFG_CLKBASE_MASK;
	tcfg |= S5PV310_TCFG_CLKBASE_SYS_MAIN;
	s5pv310_systimer_write(tcfg, S5PV310_TCFG);

	/* get system timer source clock and enable */
	systimer_clk = clk_get(NULL, "ext_xtal");

	if (IS_ERR(systimer_clk))
		panic("failed to get ext_xtal clock for system timer");

	clk_enable(systimer_clk);

	/* get pwm timer4 source clock and enable */
	pwm_clk = clk_get(NULL, "timers");
	if (IS_ERR(pwm_clk))
		panic("failed to get timers clock for pwm timer");

	clk_enable(pwm_clk);

	tmpdev.dev.bus = &platform_bus_type;
	tmpdev.id = 4;

	tin4 = clk_get(&tmpdev.dev, "pwm-tin");
	if (IS_ERR(tin4))
		panic("failed to get pwm-tin4 clock for system timer");

	tdiv4 = clk_get(&tmpdev.dev, "pwm-tdiv");
	if (IS_ERR(tdiv4))
		panic("failed to get pwm-tdiv4 clock for system timer");

	clk_enable(tin4);
}

/*
 * S5PV310's sched_clock implementation.
 * (Inspired by ORION implementation)
 */
#define TCLK2NS_SCALE_FACTOR 8

static unsigned long tclk2ns_scale;

unsigned long long sched_clock(void)
{
	unsigned long long v;

	if (tclk2ns_scale == 0)
		return 0;

	v = cnt32_to_63(0xffffffff - __raw_readl(S5PV310_FRCNTO));

	return (v * tclk2ns_scale) >> TCLK2NS_SCALE_FACTOR;
}

static void __init s5pv310_setup_sched_clock(void)
{
	unsigned long long v;
	unsigned long clock_rate;

	clock_rate = clk_get_rate(systimer_clk);

	v = NSEC_PER_SEC;
	v <<= TCLK2NS_SCALE_FACTOR;
	v += clock_rate/2;
	do_div(v, clock_rate);
	/*
	 * We want an even value to automatically clear the top bit
	 * returned by cnt32_to_63() without an additional run time
	 * instruction. So if the LSB is 1 then round it up.
	 */
	if (v & 1)
		v++;
	tclk2ns_scale = v;
}

static void __init s5pv310_timer_init(void)
{
	s5pv310_timer_resources();

	s5pv310_clockevent_init();
	s5pv310_clocksource_init();

	s5pv310_setup_sched_clock();
}

struct sys_timer s5pv310_timer = {
	.init		= s5pv310_timer_init,
};
