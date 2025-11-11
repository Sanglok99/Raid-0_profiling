#include "../../block/blk.h"

#include <linux/calclock.h>

extern inline long __sched do_wait_for_common(struct completion *x, long (*action)(long), long timeout, int state);

KTDEF(might_sleep);
KTDEF(complete_acquire);
KTDEF(raw_spin_lock_irq);
KTDEF(complete_release);
static inline long __sched
__my_wait_for_common_internal(struct completion *x,
          long (*action)(long), long timeout, int state)
{
    ktime_t stopwatch[2]; // profiling code
    ktget(&stopwatch[0]); // profiling code
 
    might_sleep();

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, might_sleep); // profiling code

    ktget(&stopwatch[0]); // profiling code

    complete_acquire(x);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, complete_acquire); // profiling code

    ktget(&stopwatch[0]); // profiling code

    raw_spin_lock_irq(&x->wait.lock);
    timeout = do_wait_for_common(x, action, timeout, state);
    raw_spin_unlock_irq(&x->wait.lock);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, raw_spin_lock_irq); // profiling code

    ktget(&stopwatch[0]); // profiling code
 
    complete_release(x);
    
    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, complete_release); // profiling code
 
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

    ret = __my_wait_for_common_internal(x, io_schedule_timeout, timeout, state);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, __wait_for_common);

    return ret;
}

static long __sched
my_wait_for_common_io(struct completion *x, long timeout, int state)
{
    return __my_wait_for_common(x, io_schedule_timeout, timeout, state);
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
