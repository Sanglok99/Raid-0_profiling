#include <linux/init.h>
#include <linux/mm.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/uio.h>
#include <linux/namei.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/falloc.h>
#include <linux/suspend.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/module.h>
#include "blk.h"

#include <linux/calclock.h>

extern int submit_bio_wait(struct bio *bio);
extern bool blkdev_dio_unaligned(struct block_device *bdev, loff_t pos,
                              struct iov_iter *iter);
extern ssize_t __blkdev_direct_IO_async(struct kiocb *iocb,
                                        struct iov_iter *iter,
                                        unsigned int nr_pages);
extern ssize_t __blkdev_direct_IO(struct kiocb *iocb, struct iov_iter *iter,
                unsigned int nr_pages);
extern ssize_t blkdev_buffered_write(struct kiocb *iocb, struct iov_iter *from);
extern int is_hibernate_resume_dev(dev_t dev); 
extern int kiocb_invalidate_pages(struct kiocb *iocb, size_t count);
extern void kiocb_invalidate_post_direct_write(struct kiocb *iocb, size_t count);
extern void dio_warn_stale_pagecache(struct file *filp);
extern void submit_bio(struct bio *bio);
extern unsigned long sysctl_hung_task_timeout_secs;

static blk_opf_t my_dio_bio_write_op(struct kiocb *iocb)
{
        blk_opf_t opf = REQ_OP_WRITE | REQ_SYNC | REQ_IDLE;

        /* avoid the need for a I/O completion work item */
        if (iocb_is_dsync(iocb))
                opf |= REQ_FUA;
        return opf;
}

#define DIO_INLINE_BIO_VECS 4

// static void my_submit_bio_wait_endio(struct bio *bio)
// {
//     complete(bio->bi_private);
// }
// 
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

KTDEF(submit_bio);
KTDEF(wait_for_completion_io);
/**
 * submit_bio_wait - submit a bio, and wait until it completes
 * @bio: The &struct bio which describes the I/O
 *
 * Simple wrapper around submit_bio(). Returns 0 on success, or the error from
 * bio_endio() on failure.
 *
 * WARNING: Unlike to how submit_bio() is usually used, this function does not
 * result in bio reference to be consumed. The caller must drop the reference
 * on his own.
 */
int my_submit_bio_wait(struct bio *bio)
{
    ktime_t stopwatch[2]; // profiling code

    DECLARE_COMPLETION_ONSTACK_MAP(done,
            bio->bi_bdev->bd_disk->lockdep_map);
    unsigned long hang_check;

    bio->bi_private = &done;
    bio->bi_end_io = my_submit_bio_wait_endio;
    bio->bi_opf |= REQ_SYNC;

    ktget(&stopwatch[0]); // profiling code

    submit_bio(bio);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, submit_bio); // profiling code

    /* Prevent hang_check timer from firing at us during very long I/O */
    hang_check = sysctl_hung_task_timeout_secs;
    if (hang_check) {
        while (!my_wait_for_completion_io_timeout(&done,
                    hang_check * (HZ/2)))
            ;
    }
    else {
        ktget(&stopwatch[0]); // profiling code

        wait_for_completion_io(&done);

        ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, wait_for_completion_io); // profiling code
    }

    return blk_status_to_errno(bio->bi_status);
}
EXPORT_SYMBOL(my_submit_bio_wait);

KTDEF(blkdev_dio_unaligned);
KTDEF(bio_iov_iter_get_pages);
KTDEF(task_io_account_write);
KTDEF(submit_bio_wait);
KTDEF(bio_release_pages);
static ssize_t __my_blkdev_direct_IO_simple_internal(struct kiocb *iocb,
                struct iov_iter *iter, unsigned int nr_pages)
{
        struct block_device *bdev = I_BDEV(iocb->ki_filp->f_mapping->host);
        struct bio_vec inline_vecs[DIO_INLINE_BIO_VECS], *vecs;
        loff_t pos = iocb->ki_pos;
        bool should_dirty = false;
        struct bio bio;
        ssize_t ret;

        ktime_t stopwatch[2]; // profiling code
        // ktget(&stopwatch[0]); // profiling code

        if (blkdev_dio_unaligned(bdev, pos, iter))
                return -EINVAL;

        // ktget(&stopwatch[1]); // profiling code
        // ktput(stopwatch, blkdev_dio_unaligned); // profiling code

        if (nr_pages <= DIO_INLINE_BIO_VECS)
                vecs = inline_vecs;
        else {
                vecs = kmalloc_array(nr_pages, sizeof(struct bio_vec),
                                     GFP_KERNEL);
                if (!vecs)
                        return -ENOMEM;
        }

        if (iov_iter_rw(iter) == READ) {
                bio_init(&bio, bdev, vecs, nr_pages, REQ_OP_READ);
                if (user_backed_iter(iter))
                        should_dirty = true;
        } else {
                bio_init(&bio, bdev, vecs, nr_pages, my_dio_bio_write_op(iocb));
        }
        bio.bi_iter.bi_sector = pos >> SECTOR_SHIFT;
        bio.bi_ioprio = iocb->ki_ioprio;

        // ktget(&stopwatch[0]); // profiling code

        ret = bio_iov_iter_get_pages(&bio, iter);

	    // ktget(&stopwatch[1]); // profiling code
	    // ktput(stopwatch, bio_iov_iter_get_pages); // profiling code

        if (unlikely(ret))
                goto out;
        ret = bio.bi_iter.bi_size;

        if (iov_iter_rw(iter) == WRITE) {
        		// ktget(&stopwatch[0]); // profiling code

                task_io_account_write(ret);

			    // ktget(&stopwatch[1]); // profiling code
			    // ktput(stopwatch, task_io_account_write); // profiling code
		}

        if (iocb->ki_flags & IOCB_NOWAIT)
                bio.bi_opf |= REQ_NOWAIT;

		
        ktget(&stopwatch[0]); // profiling code

        my_submit_bio_wait(&bio);

	    ktget(&stopwatch[1]); // profiling code
	    ktput(stopwatch, submit_bio_wait); // profiling code

        // ktget(&stopwatch[0]); // profiling code

        bio_release_pages(&bio, should_dirty);

	    // ktget(&stopwatch[1]); // profiling code
	    // ktput(stopwatch, bio_release_pages); // profiling code

        if (unlikely(bio.bi_status))
                ret = blk_status_to_errno(bio.bi_status);

out:
        if (vecs != inline_vecs)
                kfree(vecs);

        bio_uninit(&bio);

        return ret;
}

KTDEF(__blkdev_direct_IO_simple);
static ssize_t __my_blkdev_direct_IO_simple(struct kiocb *iocb,
                struct iov_iter *iter, unsigned int nr_pages) {
	ssize_t ret;

    ktime_t stopwatch[2]; // profiling code
    ktget(&stopwatch[0]); // profiling code

	ret = __my_blkdev_direct_IO_simple_internal(iocb, iter, nr_pages);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, __blkdev_direct_IO_simple); // profiling code

	return ret;
}

KTDEF(__blkdev_direct_IO_async);
ssize_t __my_blkdev_direct_IO_async(struct kiocb *iocb,
                    struct iov_iter *iter,
                    unsigned int nr_pages)
{
	ssize_t ret;

    ktime_t stopwatch[2]; // profiling code
    ktget(&stopwatch[0]); // profiling code

	ret = __blkdev_direct_IO_async(iocb, iter, nr_pages);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, __blkdev_direct_IO_async); // profiling code

	return ret; 
}

KTDEF(__blkdev_direct_IO);
ssize_t __my_blkdev_direct_IO(struct kiocb *iocb, struct iov_iter *iter,
        unsigned int nr_pages)
{
	ssize_t ret;

    ktime_t stopwatch[2]; // profiling code
    ktget(&stopwatch[0]); // profiling code

	ret = __blkdev_direct_IO(iocb, iter, bio_max_segs(nr_pages));

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, __blkdev_direct_IO); // profiling code

	return ret;	
}

static ssize_t my_blkdev_direct_IO(struct kiocb *iocb, struct iov_iter *iter)
{
        unsigned int nr_pages;

        if (!iov_iter_count(iter))
                return 0;

        nr_pages = bio_iov_vecs_to_alloc(iter, BIO_MAX_VECS + 1);
        if (likely(nr_pages <= BIO_MAX_VECS)) {
                if (is_sync_kiocb(iocb)) {
                        return __my_blkdev_direct_IO_simple(iocb, iter, nr_pages);
                }
                return __my_blkdev_direct_IO_async(iocb, iter, nr_pages);
        }
        return __my_blkdev_direct_IO(iocb, iter, bio_max_segs(nr_pages));
}

KTDEF(iov_iter_count);
KTDEF(kiocb_invalidate_pages);
KTDEF(blkdev_direct_IO);
KTDEF(kiocb_invalidate_post_direct_write);
KTDEF(iov_iter_revert);
static ssize_t
my_blkdev_direct_write(struct kiocb *iocb, struct iov_iter *from)
{
    ktime_t stopwatch[2]; // profiling code
    // ktget(&stopwatch[0]); // profiling code

    size_t count = iov_iter_count(from);

    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, iov_iter_count); // profiling code

    ssize_t written;

    // ktget(&stopwatch[0]); // profiling code

    written = kiocb_invalidate_pages(iocb, count);

    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, kiocb_invalidate_pages); // profiling code

    if (written) {
        if (written == -EBUSY) {
            return 0;
		}
        return written;
    }

    ktget(&stopwatch[0]); // profiling code

    written = my_blkdev_direct_IO(iocb, from);

    ktget(&stopwatch[1]); // profiling code
    ktput(stopwatch, blkdev_direct_IO); // profiling code

    if (written > 0) {
    	// ktget(&stopwatch[0]); // profiling code

        kiocb_invalidate_post_direct_write(iocb, count);

	    // ktget(&stopwatch[1]); // profiling code
    	// ktput(stopwatch, kiocb_invalidate_post_direct_write); // profiling code
        
		iocb->ki_pos += written;
        count -= written;
    }
    if (written != -EIOCBQUEUED) {
    	// ktget(&stopwatch[0]); // profiling code
        
		iov_iter_revert(from, count - iov_iter_count(from));

	    // ktget(&stopwatch[1]); // profiling code
	    // ktput(stopwatch, iov_iter_revert); // profiling code	
	}
    return written;
}

KTDEF(blkdev_write_iter);
KTDEF(file_update_time);
KTDEF(blkdev_direct_write);
KTDEF(direct_write_fallback);
KTDEF(blkdev_buffered_write);
KTDEF(generic_write_sync);
KTDEF(iov_iter_reexpand);
ssize_t my_blkdev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct file *file = iocb->ki_filp;
	struct block_device *bdev = I_BDEV(file->f_mapping->host);
	struct inode *bd_inode = bdev->bd_inode;
	loff_t size = bdev_nr_bytes(bdev);
	size_t shorted = 0;
	ssize_t ret;

    ktime_t stopwatch_0[2]; // profiling code
    ktget(&stopwatch_0[0]); // profiling code

	if (bdev_read_only(bdev))
		return -EPERM;

	if (IS_SWAPFILE(bd_inode) && !is_hibernate_resume_dev(bd_inode->i_rdev))
		return -ETXTBSY;

	if (!iov_iter_count(from))
		return 0;

	if (iocb->ki_pos >= size)
		return -ENOSPC;

	if ((iocb->ki_flags & (IOCB_NOWAIT | IOCB_DIRECT)) == IOCB_NOWAIT)
		return -EOPNOTSUPP;

	size -= iocb->ki_pos;
	if (iov_iter_count(from) > size) {
		shorted = iov_iter_count(from) - size;
		iov_iter_truncate(from, size);
	}

    ktime_t stopwatch[2]; // profiling code
    // ktget(&stopwatch[0]); // profiling code

	ret = file_update_time(file);

    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, file_update_time); // profiling code

	if (ret)
		return ret;

	if (iocb->ki_flags & IOCB_DIRECT) {

        ktget(&stopwatch[0]); // profiling code

		ret = my_blkdev_direct_write(iocb, from);

        ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, blkdev_direct_write); // profiling code

		if (ret >= 0 && iov_iter_count(from)){
        	// ktget(&stopwatch[0]); // profiling code

			ret = direct_write_fallback(iocb, from, ret, blkdev_buffered_write(iocb, from));

		    // ktget(&stopwatch[1]); // profiling code
        	// ktput(stopwatch, direct_write_fallback); // profiling code
		}
	} else {
        // ktget(&stopwatch[0]); // profiling code

		ret = blkdev_buffered_write(iocb, from);
		
		// ktget(&stopwatch[1]); // profiling code
        // ktput(stopwatch, blkdev_buffered_write); // profiling code
	}

	if (ret > 0) {
        // ktget(&stopwatch[0]); // profiling code

		ret = generic_write_sync(iocb, ret);

	    // ktget(&stopwatch[1]); // profiling code
        // ktput(stopwatch, generic_write_sync); // profiling code
	}

    // ktget(&stopwatch[0]); // profiling code
	
	iov_iter_reexpand(from, iov_iter_count(from) + shorted);

    // ktget(&stopwatch[1]); // profiling code
    // ktput(stopwatch, iov_iter_reexpand); // profiling code

    ktget(&stopwatch_0[1]); // profiling code
    ktput(stopwatch_0, blkdev_write_iter); // profiling code

	return ret;
}

