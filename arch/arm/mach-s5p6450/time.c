/* linux/arch/arm/mach-s5p6450/time.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5P6450 (and compatible) HRT support
 * PWM 2/4 is used for this feature
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

#include <asm/smp_twd.h>

#include <mach/map.h>
#include <plat/regs-timer.h>
#include <asm/mach/time.h>

static unsigned long clock_count_per_tick;

static struct clk *tin2;
static struct clk *tin4;
static struct clk *tdiv2;
static struct clk *tdiv4;
static struct clk *timerclk;

static void s5p6450_pwm_stop(unsigned int pwm_id)
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

static void s5p6450_pwm_init(unsigned int pwm_id, unsigned long tcnt)
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

static inline void s5p6450_pwm_start(unsigned int pwm_id, bool periodic)
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

static int s5p6450_pwm_set_next_event(unsigned long cycles,
					struct clock_event_device *evt)
{
	s5p6450_pwm_init(4, cycles);
	s5p6450_pwm_start(4, 0);
	return 0;
}

static void s5p6450_pwm_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	s5p6450_pwm_stop(4);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		s5p6450_pwm_init(4, clock_count_per_tick);
		s5p6450_pwm_start(4, 1);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device pwm_event_device = {
	.name		= "pwm_timer4",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 200,
	.shift		= 32,
	.set_next_event	= s5p6450_pwm_set_next_event,
	.set_mode	= s5p6450_pwm_set_mode,
};

irqreturn_t s5p6450_clock_event_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt = &pwm_event_device;

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction s5p6450_clock_event_irq = {
	.name		= "pwm_timer4_irq",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5p6450_clock_event_isr,
};

static void __init s5p6450_clockevent_init(void)
{
	unsigned long pclk;
	unsigned long clock_rate;
	struct clk *tscaler;

	pclk = clk_get_rate(timerclk);

	/* configure clock tick */
	tscaler = clk_get_parent(tdiv4);

	clk_set_rate(tscaler, pclk / 2);
	clk_set_rate(tdiv4, pclk / 2);
	clk_set_parent(tin4, tdiv4);

	clock_rate = clk_get_rate(tin4);
	clock_count_per_tick = clock_rate / HZ;

	pwm_event_device.mult =
		div_sc(clock_rate, NSEC_PER_SEC, pwm_event_device.shift);
	pwm_event_device.max_delta_ns =
		clockevent_delta2ns(-1, &pwm_event_device);
	pwm_event_device.min_delta_ns =
		clockevent_delta2ns(1, &pwm_event_device);

	pwm_event_device.cpumask = cpumask_of(0);
	clockevents_register_device(&pwm_event_device);

	setup_irq(IRQ_TIMER4, &s5p6450_clock_event_irq);
}

static cycle_t s5p6450_pwm2_read(struct clocksource *cs)
{
	return (cycle_t) ~__raw_readl(S3C_TIMERREG(0x2c));
}


static void s5p6450_clocksource_pwm_start(void)
{
	s5p6450_pwm_init(2, ~0);
	s5p6450_pwm_start(2, 1);
}

struct clocksource pwm_clocksource = {
	.name		= "pwm_timer2",
	.rating		= 250,
	.read		= s5p6450_pwm2_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 20,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
	.resume		= s5p6450_clocksource_pwm_start,
};

static void __init s5p6450_clocksource_init(void)
{
	unsigned long pclk;
	unsigned long clock_rate;

	pclk = clk_get_rate(timerclk);

	clk_set_rate(tdiv2, pclk / 2);
	clk_set_parent(tin2, tdiv2);

	clock_rate = clk_get_rate(tin2);

	pwm_clocksource.mult =
		clocksource_khz2mult(clock_rate/1000, pwm_clocksource.shift);

	if (clocksource_register(&pwm_clocksource))
		panic("%s: can't register clocksource\n", pwm_clocksource.name);

	s5p6450_clocksource_pwm_start();
}

static void __init s5p6450_timer_resources(void)
{
	struct platform_device tmpdev;

	tmpdev.dev.bus = &platform_bus_type;

	timerclk = clk_get(NULL, "timers");
	if (IS_ERR(timerclk))
		panic("failed to get timers clock for system timer");

	clk_enable(timerclk);

	tmpdev.id = 2;
	tin2 = clk_get(&tmpdev.dev, "pwm-tin");
	if (IS_ERR(tin2))
		panic("failed to get pwm-tin2 clock for system timer");

	tdiv2 = clk_get(&tmpdev.dev, "pwm-tdiv");
	if (IS_ERR(tdiv2))
		panic("failed to get pwm-tdiv2 clock for system timer");
	clk_enable(tin2);

	tmpdev.id = 4;
	tin4 = clk_get(&tmpdev.dev, "pwm-tin");
	if (IS_ERR(tin4))
		panic("failed to get pwm-tin4 clock for system timer");

	tdiv4 = clk_get(&tmpdev.dev, "pwm-tdiv");
	if (IS_ERR(tdiv4))
		panic("failed to get pwm-tdiv4 clock for system timer");

	clk_enable(tin4);
}

static void s5p6450_timer_setup(void)
{
	s5p6450_clockevent_init();
	s5p6450_clocksource_init();
}

static void __init s5p6450_timer_init(void)
{
	s5p6450_timer_resources();

	s5p6450_timer_setup();
}

struct sys_timer s5p6450_timer = {
	.init		= s5p6450_timer_init,
};
