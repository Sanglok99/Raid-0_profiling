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

#include <linux/calclock.h>


KTDEFEX(init_sync_kiocb);
KTDEFEX(iov_iter_ubuf);
KTDEFEX(call_write_iter);
static ssize_t my_new_sync_write_internal(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
        struct kiocb kiocb;
        struct iov_iter iter;
        ssize_t ret;

	ktime_t stopwatch[2]; // profiling code
	ktget(&stopwatch[0]); // profiling code
        
	init_sync_kiocb(&kiocb, filp);

        ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, init_sync_kiocb); // profiling code

        kiocb.ki_pos = (ppos ? *ppos : 0);

        ktget(&stopwatch[0]); // profiling code

        iov_iter_ubuf(&iter, ITER_SOURCE, (void __user *)buf, len);

        ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, iov_iter_ubuf); // profiling code

	ktget(&stopwatch[0]); // profiling code

        ret = call_write_iter(filp, &kiocb, &iter);

        ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, call_write_iter); // profiling code

        BUG_ON(ret == -EIOCBQUEUED);
        if (ret > 0 && ppos)
                *ppos = kiocb.ki_pos;

        return ret;
}

KTDEF(new_sync_write);
static ssize_t my_new_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos){
        ssize_t ret;

        ktime_t stopwatch[2]; // profiling code
        ktget(&stopwatch[0]); // profiling code
	
	ret = my_new_sync_write_internal(filp, buf, len, ppos);
	
	ktget(&stopwatch[1]); // profiling code
        ktput(stopwatch, new_sync_write); // profiling code

	return ret;
}

KTDEF(vfs_write);
atomic_t tmp_1;
ssize_t my_vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	ktime_t stopwatch[2]; // profiling code
	ktget(&stopwatch[0]); // profiling code

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
            		printk("[%s] 2 write_iter=%pS\n", __func__, file->f_op->write_iter);

		ret = my_new_sync_write(file, buf, count, pos);
    	}
	else
		ret = -EINVAL;
	if (ret > 0) {
		fsnotify_modify(file);
		add_wchar(current, ret);
	}
	inc_syscw(current);
	
file_end_write(file);

	ktget(&stopwatch[1]); // profiling code
	ktput(stopwatch, vfs_write); // profiling code

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

