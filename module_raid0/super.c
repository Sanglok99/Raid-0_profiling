// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/pxt4/super.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 */

#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/parser.h>
#include <linux/buffer_head.h>
#include <linux/exportfs.h>
#include <linux/vfs.h>
#include <linux/random.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/quotaops.h>
#include <linux/seq_file.h>
#include <linux/ctype.h>
#include <linux/log2.h>
#include <linux/crc16.h>
#include <linux/dax.h>
#include <linux/uaccess.h>
#include <linux/iversion.h>
#include <linux/unicode.h>
#include <linux/part_stat.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/fsnotify.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>

#include <linux/calclock.h>

// extern ssize_t (*conn_ksys_pwrite64)(unsigned int fd, const char __user *buf, size_t count);
extern ssize_t (*conn_blkdev_write_iter)(struct kiocb *iocb, struct iov_iter *from);
// extern ssize_t my_ksys_pwrite64(unsigned int fd, const char __user *buf, size_t count);
extern ssize_t my_blkdev_write_iter(struct kiocb *iocb, struct iov_iter *from);

KTDEC(vfs_write);
KTDEC(new_sync_write);
KTDEC(init_sync_kiocb);
KTDEC(iov_iter_ubuf);
KTDEC(call_write_iter);

KTDEC(blkdev_write_iter);
KTDEC(file_update_time);
KTDEC(blkdev_direct_write);
KTDEC(direct_write_fallback);
KTDEC(blkdev_buffered_write);
KTDEC(generic_write_sync);
KTDEC(iov_iter_reexpand);

KTDEC(iov_iter_count);
KTDEC(kiocb_invalidate_pages);
KTDEC(blkdev_direct_IO);
KTDEC(kiocb_invalidate_post_direct_write);
KTDEC(iov_iter_revert);

KTDEC(__blkdev_direct_IO_simple);
KTDEC(__blkdev_direct_IO_async);
KTDEC(__blkdev_direct_IO);

KTDEC(blkdev_dio_unaligned);
KTDEC(bio_iov_iter_get_pages);
KTDEC(task_io_account_write);
KTDEC(submit_bio_wait);
KTDEC(bio_release_pages);

KTDEC(submit_bio);
KTDEC(wait_for_completion_io);
KTDEC(__wait_for_common);

KTDEC(might_sleep);
KTDEC(complete_acquire);
KTDEC(raw_spin_lock_irq);
KTDEC(complete_release);

static int __init pxt4_init_fs(void)
{
    printk("start raid0 module\n");

    // conn_ksys_pwrite64 = &my_ksys_pwrite64;
	conn_blkdev_write_iter = &my_blkdev_write_iter;

	return 0;
}

static void __exit pxt4_exit_fs(void)
{
    // conn_ksys_pwrite64 = NULL;
	conn_blkdev_write_iter = NULL;

	// ktprint(0, vfs_write);
	// ktprint(1, new_sync_write);
	// ktprint(2, init_sync_kiocb);
	// ktprint(2, iov_iter_ubuf);
	// ktprint(2, call_write_iter);
	
    ktprint(0, blkdev_write_iter);
	// ktprint(1, file_update_time);
	ktprint(1, blkdev_direct_write);
	// ktprint(1, direct_write_fallback);
	// ktprint(1, blkdev_buffered_write);
	// ktprint(1, generic_write_sync);
	// ktprint(1, iov_iter_reexpand);
	
	// ktprint(2, iov_iter_count);
	// ktprint(2, kiocb_invalidate_pages);
	ktprint(2, blkdev_direct_IO);
	// ktprint(2, kiocb_invalidate_post_direct_write);
	// ktprint(2, iov_iter_revert);

	ktprint(3, __blkdev_direct_IO_simple);
	// ktprint(3, __blkdev_direct_IO_async);
	// ktprint(3, __blkdev_direct_IO);
    
    // ktprint(4, blkdev_dio_unaligned);
	// ktprint(4, bio_iov_iter_get_pages);
    // ktprint(4, task_io_account_write);
    ktprint(4, submit_bio_wait);
    // ktprint(4, bio_release_pages);

    ktprint(5, submit_bio);
    ktprint(5, wait_for_completion_io);
    ktprint(5, __wait_for_common);

    ktprint(6, might_sleep);
    ktprint(6, complete_acquire);
    ktprint(6, raw_spin_lock_irq);
    ktprint(6, complete_release);

    printk("end raid0 module\n");
}

MODULE_AUTHOR("Remy Card, Stephen Tweedie, Andrew Morton, Andreas Dilger, Theodore Ts'o and others");
MODULE_DESCRIPTION("Fourth Extended Filesystem");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: crc32c");
module_init(pxt4_init_fs)
module_exit(pxt4_exit_fs)
