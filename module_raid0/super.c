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


extern ssize_t (*conn_ksys_pwrite64)(unsigned int fd, const char __user *buf, size_t count);
extern ssize_t my_ksys_pwrite64(unsigned int fd, const char __user *buf, size_t count);


static int __init pxt4_init_fs(void)
{
    printk("start raid0 module\n");

    conn_ksys_pwrite64 = &my_ksys_pwrite64;
	return 0;
}

static void __exit pxt4_exit_fs(void)
{
    conn_ksys_pwrite64 = NULL;

    printk("end raid0 module\n");
}

MODULE_AUTHOR("Remy Card, Stephen Tweedie, Andrew Morton, Andreas Dilger, Theodore Ts'o and others");
MODULE_DESCRIPTION("Fourth Extended Filesystem");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: crc32c");
module_init(pxt4_init_fs)
module_exit(pxt4_exit_fs)
