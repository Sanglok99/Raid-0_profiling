#include "../../block/blk.h"

#include <linux/calclock.h>

extern inline long __sched do_wait_for_common(struct completion *x, long (*action)(long), long timeout, int state);
extern signed long __sched my_io_schedule_timeout(signed long timeout);

extern signed long __sched schedule_timeout(signed long timeout);
extern int io_schedule_prepare(void);

void io_schedule_finish(int token)
{
	current->in_iowait = token;
}

KTDEF(io_schedule_timeout);
long __sched my_io_schedule_timeout(long timeout)
{
	int token;
	long ret;

    ktime_t stopwatch[2]; // profiling code
    ktget(&stopwatch[0]); // profiling code

	token = io_schedule_prepare();
	ret = schedule_timeout(timeout);
	io_schedule_finish(token);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, io_schedule_timeout); // profiling code
                         
	return ret;
}
EXPORT_SYMBOL(my_io_schedule_timeout);

void __my_prepare_to_swait(struct swait_queue_head *q, struct swait_queue *wait)
{
	wait->task = current;
	if (list_empty(&wait->task_list))
		list_add_tail(&wait->task_list, &q->task_list);
}

void __my_finish_swait(struct swait_queue_head *q, struct swait_queue *wait)
{
	__set_current_state(TASK_RUNNING);
	if (!list_empty(&wait->task_list))
		list_del_init(&wait->task_list);
}

KTDEF(__prepare_to_swait);
KTDEF(__set_current_state);
KTDEF(__finish_swait);
KTDEF(action);
static inline long __sched
my_do_wait_for_common(struct completion *x,
		   long (*action)(long), long timeout, int state)
{
    ktime_t stopwatch[2]; // profiling code

	if (!x->done) {
		DECLARE_SWAITQUEUE(wait);

		do {
			if (signal_pending_state(state, current)) {
				timeout = -ERESTARTSYS;
				break;
			}
            
            // ktget(&stopwatch[0]); // profiling code

			__my_prepare_to_swait(&x->wait, &wait);

            // ktget(&stopwatch[1]); // profiling code
            // ktput(stopwatch, __prepare_to_swait); // profiling code

            // ktget(&stopwatch[0]); // profiling code

			__set_current_state(state);

            // ktget(&stopwatch[1]); // profiling code
            // ktput(stopwatch, __set_current_state); // profiling code

			raw_spin_unlock_irq(&x->wait.lock);

            ktget(&stopwatch[1]); // profiling code
                                 
			timeout = action(timeout);

            ktget(&stopwatch[1]); // profiling code
            ktput(stopwatch, action); // profiling code

			raw_spin_lock_irq(&x->wait.lock);
		} while (!x->done && timeout);

        // ktget(&stopwatch[0]); // profiling code
                          
		__my_finish_swait(&x->wait, &wait);

        // ktget(&stopwatch[1]); // profiling code
        // ktput(stopwatch, __finish_swait); // profiling code

		if (!x->done)
			return timeout;
	}
	if (x->done != UINT_MAX)
		x->done--;
	return timeout ?: 1;
}

KTDEF(might_sleep);
KTDEF(complete_acquire);
KTDEF(raw_spin_lock_irq);
KTDEF(complete_release);
static inline long __sched
__my_wait_for_common_internal(struct completion *x,
          long (*action)(long), long timeout, int state)
{
    ktime_t stopwatch[2]; // profiling code
    // ktget(&stopwatch[0]); // profiling code
 
    might_sleep();

    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, might_sleep); // profiling code

    // ktget(&stopwatch[0]); // profiling code

    complete_acquire(x);

    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, complete_acquire); // profiling code

    ktget(&stopwatch[0]); // profiling code

    raw_spin_lock_irq(&x->wait.lock);
    timeout = my_do_wait_for_common(x, action, timeout, state);
    raw_spin_unlock_irq(&x->wait.lock);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, raw_spin_lock_irq); // profiling code

    // ktget(&stopwatch[0]); // profiling code
 
    complete_release(x);
    
    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, complete_release); // profiling code
 
    return timeout;
}

KTDEF(__wait_for_common);
static inline long __sched
__my_wait_for_common(struct completion *x,
          long (*action)(long), long timeout, int state)
{
    long ret;

    ktime_t stopwatch[2]; // profiling code
    ktget(&stopwatch[0]); // profiling code

    ret = __my_wait_for_common_internal(x, action, timeout, state);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, __wait_for_common);

    return ret;
}

static long __sched
my_wait_for_common_io(struct completion *x, long timeout, int state)
{
    return __my_wait_for_common(x, my_io_schedule_timeout, timeout, state);
}

/**
 * wait_for_completion_io_timeout: - waits for completion of a task (w/timeout)
 * @x:  holds the state of this particular completion
 * @timeout:  timeout value in jiffies
 *
 * This waits for either a completion of a specific task to be signaled or for a
 * specified timeout to expire. The timeout is in jiffies. It is not
 * interruptible. The caller is accounted as waiting for IO (which traditionally
 * means blkio only).
 *
 * Return: 0 if timed out, and positive (at least 1, or number of jiffies left
 * till timeout) if completed.
 */
unsigned long __sched
my_wait_for_completion_io_timeout(struct completion *x, unsigned long timeout)
{
    return my_wait_for_common_io(x, timeout, TASK_UNINTERRUPTIBLE);
}
