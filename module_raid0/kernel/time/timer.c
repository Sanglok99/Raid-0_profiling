// SPDX-License-Identifier: GPL-2.0
/*
 *  Kernel internal timers
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  1997-01-28  Modified by Finn Arne Gangstad to make timers scale better.
 *
 *  1997-09-10  Updated NTP code according to technical memorandum Jan '96
 *              "A Kernel Model for Precision Timekeeping" by Dave Mills
 *  1998-12-24  Fixed a xtime SMP race (we need the xtime_lock rw spinlock to
 *              serialize accesses to xtime/lost_ticks).
 *                              Copyright (C) 1998  Andrea Arcangeli
 *  1999-03-10  Improved NTP compatibility by Ulrich Windl
 *  2002-05-31	Move sys_sysinfo here and make its locking sane, Robert Love
 *  2000-10-05  Implemented scalable SMP per-CPU timer handling.
 *                              Copyright (C) 2000, 2001, 2002  Ingo Molnar
 *              Designed by David S. Miller, Alexey Kuznetsov and Ingo Molnar
 */

#include <linux/kernel_stat.h>
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/pid_namespace.h>
#include <linux/notifier.h>
#include <linux/thread_info.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/posix-timers.h>
#include <linux/cpu.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/tick.h>
#include <linux/kallsyms.h>
#include <linux/irq_work.h>
#include <linux/sched/signal.h>
#include <linux/sched/sysctl.h>
#include <linux/sched/nohz.h>
#include <linux/sched/debug.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/random.h>
#include <linux/sysctl.h>

#include <linux/uaccess.h>
#include <asm/unistd.h>
#include <asm/div64.h>
#include <asm/timex.h>
#include <asm/io.h>

#include "tick-internal.h"

#define CREATE_TRACE_POINTS
#include <trace/events/timer.h>

#include <linux/calclock.h>

extern int wake_up_process(struct task_struct *p);
extern inline int __mod_timer(struct timer_list *timer, unsigned long expires, unsigned int options);

#define MOD_TIMER_PENDING_ONLY		0x01
#define MOD_TIMER_REDUCE		0x02
#define MOD_TIMER_NOTPENDING		0x04

/*
 * Since schedule_timeout()'s timer is defined on the stack, it must store
 * the target task on the stack as well.
 */
struct process_timer {
	struct timer_list timer;
	struct task_struct *task;
};

static void process_timeout(struct timer_list *t)
{
	struct process_timer *timeout = from_timer(timeout, t, timer);

	wake_up_process(timeout->task);
}

KTDEF(my_schedule_timeout);
KTDEF(first_schedule);
KTDEF(timer_setup_on_stack);
KTDEF(__mod_timer);
KTDEF(second_schedule);
KTDEF(del_timer_sync);
KTDEF(destroy_timer_on_stack);
/**
 * schedule_timeout - sleep until timeout
 * @timeout: timeout value in jiffies
 *
 * Make the current task sleep until @timeout jiffies have elapsed.
 * The function behavior depends on the current task state
 * (see also set_current_state() description):
 *
 * %TASK_RUNNING - the scheduler is called, but the task does not sleep
 * at all. That happens because sched_submit_work() does nothing for
 * tasks in %TASK_RUNNING state.
 *
 * %TASK_UNINTERRUPTIBLE - at least @timeout jiffies are guaranteed to
 * pass before the routine returns unless the current task is explicitly
 * woken up, (e.g. by wake_up_process()).
 *
 * %TASK_INTERRUPTIBLE - the routine may return early if a signal is
 * delivered to the current task or the current task is explicitly woken
 * up.
 *
 * The current task state is guaranteed to be %TASK_RUNNING when this
 * routine returns.
 *
 * Specifying a @timeout value of %MAX_SCHEDULE_TIMEOUT will schedule
 * the CPU away without a bound on the timeout. In this case the return
 * value will be %MAX_SCHEDULE_TIMEOUT.
 *
 * Returns 0 when the timer has expired otherwise the remaining time in
 * jiffies will be returned. In all cases the return value is guaranteed
 * to be non-negative.
 */
signed long __sched my_schedule_timeout(signed long timeout)
{
	struct process_timer timer;
	unsigned long expire;

    ktime_t stopwatch[2]; // profiling code
    ktime_t stopwatch_0[2]; // profiling code
    ktget(&stopwatch_0[0]); // profiling code

	switch (timeout)
	{
	case MAX_SCHEDULE_TIMEOUT:
		/*
		 * These two special cases are useful to be comfortable
		 * in the caller. Nothing more. We could take
		 * MAX_SCHEDULE_TIMEOUT from one of the negative value
		 * but I' d like to return a valid offset (>=0) to allow
		 * the caller to do everything it want with the retval.
		 */

        ktget(&stopwatch[0]); // profiling code

		schedule();
        
        ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, first_schedule); // profiling code

		goto out;
	default:
		/*
		 * Another bit of PARANOID. Note that the retval will be
		 * 0 since no piece of kernel is supposed to do a check
		 * for a negative retval of schedule_timeout() (since it
		 * should never happens anyway). You just have the printk()
		 * that will tell you if something is gone wrong and where.
		 */
		if (timeout < 0) {
			printk(KERN_ERR "schedule_timeout: wrong timeout "
				"value %lx\n", timeout);
			dump_stack();
			__set_current_state(TASK_RUNNING);
			goto out;
		}
	}

	expire = timeout + jiffies;

	timer.task = current;
    
    ktget(&stopwatch[0]); // profiling code
	
    timer_setup_on_stack(&timer.timer, process_timeout, 0);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, timer_setup_on_stack); // profiling code

    ktget(&stopwatch[0]); // profiling code
                        
	__mod_timer(&timer.timer, expire, MOD_TIMER_NOTPENDING);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, __mod_timer); // profiling code

    ktget(&stopwatch[0]); // profiling code

	schedule();

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, second_schedule); // profiling code

    ktget(&stopwatch[0]); // profiling code

	del_timer_sync(&timer.timer);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, del_timer_sync); // profiling code
                        
    ktget(&stopwatch[0]); // profiling code

	/* Remove the timer from the object tracker */
	destroy_timer_on_stack(&timer.timer);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, destroy_timer_on_stack); // profiling code

	timeout = expire - jiffies;

 out:
    ktget(&stopwatch_0[1]); // profiling code
    ktput(stopwatch_0, my_schedule_timeout); // profiling code

	return timeout < 0 ? 0 : timeout;
}
EXPORT_SYMBOL(my_schedule_timeout);


