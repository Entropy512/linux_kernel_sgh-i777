/* linux/arch/arm/mach-s5pv310/platsmp.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Based on linux/arch/arm/mach-vexpress/platsmp.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/sched.h>

#include <asm/cacheflush.h>
#include <asm/localtimer.h>
#include <asm/smp_scu.h>
#include <asm/unified.h>
#include <asm/mmu_context.h>

#include <plat/s5pv310.h>

#include <mach/hardware.h>
#include <mach/regs-clock.h>

#define SCU_CTRL		0x00
#define   IC_STANDBY_EN		(1 << 6)
#define   SCU_STANDBY_EN	(1 << 5)

extern void s5pv310_secondary_startup(void);
extern inline void platform_do_lowpower(unsigned int cpu);

#define CPU1_BOOT_REG (s5pv310_subrev() == 0 ? S5P_VA_SYSRAM : S5P_INFORM5)

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 */
volatile int __cpuinitdata pen_release = -1;

static void __iomem *scu_base_addr(void)
{
	return (void __iomem *)(S5P_VA_SCU);
}

static DEFINE_SPINLOCK(boot_lock);

#ifdef CONFIG_VFP
static void vfp_enable(void *unused)
{
	u32 access = get_copro_access();

	/*
	 * Enable full access to VFP (cp10 and cp11)
	 */
	set_copro_access(access | CPACC_FULL(10) | CPACC_FULL(11));
}
#endif

void __cpuinit platform_secondary_init(unsigned int cpu)
{
#ifdef CONFIG_VFP
	unsigned int cpu_arch = cpu_architecture();

	if (cpu_arch >= CPU_ARCH_ARMv6)
		vfp_enable(NULL);
#endif

	trace_hardirqs_off();

	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_cpu_init(0, gic_cpu_base_addr);

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	pen_release = -1;
	smp_wmb();

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	pen_release = cpu;
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));

	if (!(__raw_readl(S5P_ARM_CORE1_STATUS) & S5P_CORE_LOCAL_PWR_EN)) {
		__raw_writel(S5P_CORE_LOCAL_PWR_EN,
			     S5P_ARM_CORE1_CONFIGURATION);

		timeout = 10;

		/* wait max 10 ms until cpu1 is on */
		while ((__raw_readl(S5P_ARM_CORE1_STATUS)
			& S5P_CORE_LOCAL_PWR_EN) != S5P_CORE_LOCAL_PWR_EN) {
			if (timeout-- == 0)
				break;

			mdelay(1);
		}

		if (timeout == 0) {
			printk(KERN_ERR "cpu1 power enable failed");
			spin_unlock(&boot_lock);

			return -ETIMEDOUT;
		}
	}

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system ram, and branch to
	 * the address found there.
	 */
	smp_cross_call(cpumask_of(cpu));

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();

		if (!__raw_readl(CPU1_BOOT_REG)) {
			__raw_writel(BSYM(virt_to_phys(s5pv310_secondary_startup)),
				     CPU1_BOOT_REG);
			smp_cross_call(cpumask_of(cpu));
		}

		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
{
	void __iomem *scu_base = scu_base_addr();
	unsigned int i, ncores;

	ncores = scu_base ? scu_get_core_count(scu_base) : 1;

	/* sanity check */
	if (ncores == 0) {
		printk(KERN_ERR
		       "s5pv310: strange CM count of 0? Default to 1\n");

		ncores = 1;
	}

	if (ncores > NR_CPUS) {
		printk(KERN_WARNING
		       "s5pv310: no. of cores (%d) greater than configured "
		       "maximum of %d - clipping\n",
		       ncores, NR_CPUS);
		ncores = NR_CPUS;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int ncores = num_possible_cpus();
	unsigned int cpu = smp_processor_id();
	int i;

	init_new_context(current, &init_mm);
	smp_store_cpu_info(cpu);

	/*
	 * are we trying to boot more cores than exist?
	 */
	if (max_cpus > ncores)
		max_cpus = ncores;

	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	for (i = 0; i < ncores; i++)
		set_cpu_present(i, true);

	/*
	 * Initialise the SCU if there are more than one CPU and let
	 * them know where to start.
	 */
	if (ncores > 1) {
		for (i = max_cpus; i < ncores; i++)
			platform_do_lowpower(i);

		/*
		 * Enable the local timer or broadcast device for the
		 * boot CPU, but only if we have more than one CPU.
		 */
		percpu_timer_setup();

		i = __raw_readl(scu_base_addr() + SCU_CTRL);
		i |= SCU_STANDBY_EN;
		__raw_writel(i, scu_base_addr() + SCU_CTRL);

		scu_enable(scu_base_addr());

		if (max_cpus > 1) {
			/*
			 * Write the address of secondary startup into the
			 * system-wide flags register. The boot monitor waits
			 * until it receives a soft interrupt, and then the
			 * secondary CPU branches to this address.
			 */
			__raw_writel(BSYM(virt_to_phys(s5pv310_secondary_startup)),
				     CPU1_BOOT_REG);
		}
	}
}
