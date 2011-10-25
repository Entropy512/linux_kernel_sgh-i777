/*
 * kernel/sched_monitor.c
 *
 * get the Dynamic CPU hotplug flag
 *
 * Copyright(C) 2010, samsung Electronics.co.Ltd Jiseong.oh
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifdef CONFIG_SMP
unsigned int d_hotplug_lock;
unsigned int force_hotplug;

static int is_rt_task_run(struct task_struct *p)
{
    struct rq *rq = task_rq(p);

    return (rq->rt.rt_nr_running?1:0);
}

static void update_hotplug_monitor(struct sched_domain *sd, unsigned int on_load_balance)
{
	unsigned int cpu, run_task_prio[2];
    	struct task_struct *p;

	d_hotplug_lock = 0;
	
	if(0 < on_load_balance)
	    	d_hotplug_lock = 1;
	else if(idle_cpu(0))
	    	force_hotplug = 1;
	
	for_each_online_cpu(cpu) {

	        p = cpu_curr(cpu);

		if(cpu && is_rt_task_run(p))
		    d_hotplug_lock = 1;

		if(p->policy == SCHED_FIFO || p->policy == SCHED_RR)
		    run_task_prio[cpu] = MAX_RT_PRIO-1 - p->rt_priority;
		else
		    run_task_prio[cpu] = p->static_prio;
	}
	
	if(run_task_prio[0] > run_task_prio[1]){
	    	d_hotplug_lock = 1;
	}
		
	
}
#endif

