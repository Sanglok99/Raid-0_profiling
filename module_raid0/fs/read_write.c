#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/sched/xacct.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/fsnotify.h>
#include <linux/security.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
#include <linux/compat.h>
#include <linux/mount.h>
#include <linux/fs.h>
#include "internal.h"

#include <linux/uaccess.h>
#include <asm/unistd.h>







ssize_t new_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos);

atomic_t tmp_1;
ssize_t my_vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;
	if (!(file->f_mode & FMODE_CAN_WRITE))
		return -EINVAL;
	if (unlikely(!access_ok(buf, count)))
		return -EFAULT;

	ret = rw_verify_area(WRITE, file, pos, count);
	if (ret)
		return ret;
	if (count > MAX_RW_COUNT)
		count =  MAX_RW_COUNT;
	file_start_write(file);
	if (file->f_op->write) {
        if (atomic_inc_return(&tmp_1) < 50)
            printk("[%s] 1 write=%pS\n", __func__, file->f_op->write);
		ret = file->f_op->write(file, buf, count, pos);
    }
	else if (file->f_op->write_iter) {
        if (atomic_inc_return(&tmp_1) < 50)
            printk("[%s] 2 write_iter=%pS\n", __func__,
                    file->f_op->write_iter);

		ret = new_sync_write(file, buf, count, pos);
    }
	else
		ret = -EINVAL;
	if (ret > 0) {
		fsnotify_modify(file);
		add_wchar(current, ret);
	}
	inc_syscw(current);
	file_end_write(file);
	return ret;
}

ssize_t my_ksys_pwrite64(unsigned int fd, const char __user *buf,
		      size_t count, loff_t pos)
{
	struct fd f;
	ssize_t ret = -EBADF;

	if (pos < 0)
		return -EINVAL;

	f = fdget(fd);

	if (f.file) {
		ret = -ESPIPE;
		if (f.file->f_mode & FMODE_PWRITE)  
			//ret = vfs_write(f.file, buf, count, &pos);
			ret = my_vfs_write(f.file, buf, count, &pos);
		fdput(f);
	}

	return ret;
}

