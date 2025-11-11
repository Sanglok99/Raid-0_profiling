// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2001 Jens Axboe <axboe@kernel.dk>
 */
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/uio.h>
#include <linux/iocontext.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/mempool.h>
#include <linux/workqueue.h>
#include <linux/cgroup.h>
#include <linux/highmem.h>
#include <linux/sched/sysctl.h>
#include <linux/blk-crypto.h>
#include <linux/xarray.h>

#include <trace/events/block.h>
#include "blk.h"
#include "blk-rq-qos.h"
#include "blk-cgroup.h"

#define ALLOC_CACHE_THRESHOLD   16
#define ALLOC_CACHE_MAX     256

extern void dio_warn_stale_pagecache(struct file *filp);
extern void submit_bio(struct bio *bio);
extern unsigned long sysctl_hung_task_timeout_secs;

void my_submit_bio_wait_endio(struct bio *bio)
{
    complete(bio->bi_private);
}

// static inline long __sched
// __my_wait_for_common(struct completion *x,
//           long (*action)(long), long timeout, int state)
// {
//     might_sleep();
// 
//     complete_acquire(x);
// 
//     raw_spin_lock_irq(&x->wait.lock);
//     timeout = do_wait_for_common(x, action, timeout, state);
//     raw_spin_unlock_irq(&x->wait.lock);
// 
//     complete_release(x);
// 
//     return timeout;
// }
// 
// static long __sched
// my_wait_for_common_io(struct completion *x, long timeout, int state)
// {
//     return __my_wait_for_common(x, io_schedule_timeout, timeout, state);
// }
// 
// /**
//  * wait_for_completion_io_timeout: - waits for completion of a task (w/timeout)
//  * @x:  holds the state of this particular completion
//  * @timeout:  timeout value in jiffies
//  *
//  * This waits for either a completion of a specific task to be signaled or for a
//  * specified timeout to expire. The timeout is in jiffies. It is not
//  * interruptible. The caller is accounted as waiting for IO (which traditionally
//  * means blkio only).
//  *
//  * Return: 0 if timed out, and positive (at least 1, or number of jiffies left
//  * till timeout) if completed.
//  */
// unsigned long __sched
// my_wait_for_completion_io_timeout(struct completion *x, unsigned long timeout)
// {
//     return my_wait_for_common_io(x, timeout, TASK_UNINTERRUPTIBLE);
// }
//  
// KTDEF(submit_bio);
// KTDEF(wait_for_completion_io);
// /**
//  * submit_bio_wait - submit a bio, and wait until it completes
//  * @bio: The &struct bio which describes the I/O
//  *
//  * Simple wrapper around submit_bio(). Returns 0 on success, or the error from
//  * bio_endio() on failure.
//  *
//  * WARNING: Unlike to how submit_bio() is usually used, this function does not
//  * result in bio reference to be consumed. The caller must drop the reference
//  * on his own.
//  */
// int my_submit_bio_wait(struct bio *bio)
// {
//     ktime_t stopwatch[2]; // profiling code
// 
//     DECLARE_COMPLETION_ONSTACK_MAP(done,
//             bio->bi_bdev->bd_disk->lockdep_map);
//     unsigned long hang_check;
// 
//     bio->bi_private = &done;
//     bio->bi_end_io = my_submit_bio_wait_endio;
//     bio->bi_opf |= REQ_SYNC;
// 
//     ktget(&stopwatch[0]); // profiling code
// 
//     submit_bio(bio);
// 
//     ktget(&stopwatch[1]); // profiling code
//     ktput(stopwatch, submit_bio); // profiling code
// 
//     /* Prevent hang_check timer from firing at us during very long I/O */
//     hang_check = sysctl_hung_task_timeout_secs;
//     if (hang_check) {
//         while (!my_wait_for_completion_io_timeout(&done,
//                     hang_check * (HZ/2)))
//             ;
//     }
//     else {
//         ktget(&stopwatch[0]); // profiling code
// 
//         wait_for_completion_io(&done);
// 
//         ktget(&stopwatch[1]); // profiling code
//         ktput(stopwatch, wait_for_completion_io); // profiling code
//     }
// 
//     return blk_status_to_errno(bio->bi_status);
// }
// EXPORT_SYMBOL(my_submit_bio_wait);
