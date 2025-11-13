// SPDX-License-Identifier: GPL-2.0-only
/*
 *  kernel/sched/core.c
 *
 *  Core kernel scheduler code and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 */
// #include <linux/highmem.h>
// #include <linux/hrtimer_api.h>
// #include <linux/ktime_api.h>
// #include <linux/sched/signal.h>
// #include <linux/syscalls_api.h>
// #include <linux/debug_locks.h>
// #include <linux/prefetch.h>
// #include <linux/capability.h>
// #include <linux/pgtable_api.h>
// #include <linux/wait_bit.h>
// #include <linux/jiffies.h>
// #include <linux/spinlock_api.h>
// #include <linux/cpumask_api.h>
// #include <linux/lockdep_api.h>
// #include <linux/hardirq.h>
// #include <linux/softirq.h>
// #include <linux/refcount_api.h>
// #include <linux/topology.h>
// #include <linux/sched/clock.h>
// #include <linux/sched/cond_resched.h>
// #include <linux/sched/cputime.h>
// #include <linux/sched/debug.h>
// #include <linux/sched/hotplug.h>
// #include <linux/sched/init.h>
// #include <linux/sched/isolation.h>
// #include <linux/sched/loadavg.h>
// #include <linux/sched/mm.h>
// #include <linux/sched/nohz.h>
// #include <linux/sched/rseq_api.h>
// #include <linux/sched/rt.h>
// 
// #include <linux/blkdev.h>
// #include <linux/context_tracking.h>
// #include <linux/cpuset.h>
// #include <linux/delayacct.h>
// #include <linux/init_task.h>
// #include <linux/interrupt.h>
// #include <linux/ioprio.h>
// #include <linux/kallsyms.h>
// #include <linux/kcov.h>
// #include <linux/kprobes.h>
// #include <linux/llist_api.h>
// #include <linux/mmu_context.h>
// #include <linux/mmzone.h>
// #include <linux/mutex_api.h>
// #include <linux/nmi.h>
// #include <linux/nospec.h>
// #include <linux/perf_event_api.h>
// #include <linux/profile.h>
// #include <linux/psi.h>
// #include <linux/rcuwait_api.h>
// #include <linux/sched/wake_q.h>
// #include <linux/scs.h>
// #include <linux/slab.h>
// #include <linux/syscalls.h>
// #include <linux/vtime.h>
// #include <linux/wait_api.h>
// #include <linux/workqueue_api.h>
// 
// #ifdef CONFIG_PREEMPT_DYNAMIC
// # ifdef CONFIG_GENERIC_ENTRY
// #  include <linux/entry-common.h>
// # endif
// #endif
// 
// #include <uapi/linux/sched/types.h>
// 
// #include <asm/irq_regs.h>
// #include <asm/switch_to.h>
// // #include <asm/tlb.h> // comment out due to compilation issue
// 
// #define CREATE_TRACE_POINTS
// #include <linux/sched/rseq_api.h>
// #include <trace/events/sched.h>
// #include <trace/events/ipi.h>
// #undef CREATE_TRACE_POINTS
//  
// #include "sched.h"
// #include "stats.h"
// #include "autogroup.h"
// 
// #include "autogroup.h"
// #include "pelt.h"
// #include "smp.h"
// #include "stats.h"
// 
// #include "../workqueue_internal.h"
// #include "../../io_uring/io-wq.h"
// #include "../smpboot.h"
// 
// #include <linux/calclock.h> // profiling code
// 
// extern signed long __sched schedule_timeout(signed long timeout);
// extern int io_schedule_prepare(void);
// 
// void io_schedule_finish(int token)
// {
// 	current->in_iowait = token;
// }
// 
// KTDEF(io_schedule_timeout);
// long __sched my_io_schedule_timeout(long timeout)
// {
// 	int token;
// 	long ret;
// 
//     ktime_t stopwatch[2]; // profiling code
//     ktget(&stopwatch[0]); // profiling code
// 
// 	token = io_schedule_prepare();
// 	ret = schedule_timeout(timeout);
// 	io_schedule_finish(token);
// 
//     ktget(&stopwatch[1]); // profiling code
//     ktput(stopwatch, io_schedule_timeout); // profiling code
//                          
// 	return ret;
// }
// EXPORT_SYMBOL(my_io_schedule_timeout);
