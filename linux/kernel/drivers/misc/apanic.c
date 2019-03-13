/* drivers/misc/apanic.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Author: San Mehat <san@android.com>
 *
 * Copyright (C) 2009 Motorola, Inc.
 *
 * Copyright (C) 2015 RDA Micro, Inc.
 * Author: Gary Yu<baolinyu@rdamicro.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/mtd/mtd.h>
#include <linux/rtc.h>
#include <linux/console.h>
#include <linux/notifier.h>
#include <linux/mtd/mtd.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/slab.h>
#include <linux/kmsg_dump.h>
#include <linux/syslog.h>
#include <linux/vmalloc.h>

static unsigned long record_size = 4096*8;
module_param(record_size, ulong, 0400);
MODULE_PARM_DESC(record_size,
		"record size for apanic  pages in bytes");

static char mtddev[80]=CONFIG_APANIC_PLABEL;
module_param_string(mtddev, mtddev, 80, 0400);
MODULE_PARM_DESC(mtddev,
		"name or index number of the MTD device to use");

int has_apanic_dump;

struct panic_header {
	u32 magic;
#define PANIC_MAGIC 0xdeadf00d

	u32 version;
#define PHDR_VERSION   0x01

	u32 console_offset;
	u32 console_length;

	u32 threads_offset;
	u32 threads_length;
};

struct apanic_data {
	struct kmsg_dumper dump;
	struct mtd_info		*mtd;
	struct panic_header	curr;
	void			*bounce;
	struct proc_dir_entry	*apanic_console;
	struct proc_dir_entry	*apanic_threads;
};

static struct apanic_data drv_ctx;
static struct work_struct proc_removal_work;
static DEFINE_MUTEX(drv_mutex);

static unsigned int *apanic_bbt;
static unsigned int apanic_erase_blocks;
static unsigned int apanic_good_blocks;

static void set_bb(unsigned int block, unsigned int *bbt)
{
	unsigned int flag = 1;

	BUG_ON(block >= apanic_erase_blocks);

	flag = flag << (block%32);
	apanic_bbt[block/32] |= flag;
	apanic_good_blocks--;
}

static unsigned int get_bb(unsigned int block, unsigned int *bbt)
{
	unsigned int flag;

	BUG_ON(block >= apanic_erase_blocks);

	flag = 1 << (block%32);
	return apanic_bbt[block/32] & flag;
}

static void alloc_bbt(struct mtd_info *mtd, unsigned int *bbt)
{
	int bbt_size;
	apanic_erase_blocks = (mtd->size)>>(mtd->erasesize_shift);
	bbt_size = (apanic_erase_blocks+32)/32;

	apanic_bbt = kmalloc(bbt_size*4, GFP_KERNEL);
	memset(apanic_bbt, 0, bbt_size*4);
	apanic_good_blocks = apanic_erase_blocks;
}
static void scan_bbt(struct mtd_info *mtd, unsigned int *bbt)
{
	int i;

	for (i = 0; i < apanic_erase_blocks; i++) {
		if (mtd->_block_isbad(mtd, i*mtd->erasesize))
			set_bb(i, apanic_bbt);
	}
}

#define APANIC_INVALID_OFFSET 0xFFFFFFFF

static unsigned int phy_offset(struct mtd_info *mtd, unsigned int offset)
{
	unsigned int logic_block = offset>>(mtd->erasesize_shift);
	unsigned int phy_block;
	unsigned good_block = 0;

	for (phy_block = 0; phy_block < apanic_erase_blocks; phy_block++) {
		if (!get_bb(phy_block, apanic_bbt))
			good_block++;
		if (good_block == (logic_block + 1))
			break;
	}

	if (good_block != (logic_block + 1))
		return APANIC_INVALID_OFFSET;

	return offset + ((phy_block-logic_block)<<mtd->erasesize_shift);
}

static void apanic_erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

static ssize_t
apanic_proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct apanic_data *ctx = &drv_ctx;
	void *dat = PDE_DATA(file_inode(file));
	size_t file_length;
	off_t file_offset;
	unsigned int page_no;
	off_t page_offset;
	int rc;
	size_t len;
	off_t offset = *ppos;

	if (!count)
		return 0;

	mutex_lock(&drv_mutex);

	switch ((int) dat) {
	case 1:	/* apanic_console */
		file_length = ctx->curr.console_length;
		file_offset = ctx->curr.console_offset;
		break;
	case 2:	/* apanic_threads */
		file_length = ctx->curr.threads_length;
		file_offset = ctx->curr.threads_offset;
		break;
	default:
		pr_err("Bad dat (%d)\n", (int) dat);
		mutex_unlock(&drv_mutex);
		return -EINVAL;
	}

	//printk(KERN_ERR "APANIC: file_length:%d, file_offset:%d\n",  file_length, file_offset);
	if ((offset + count) > file_length) {
		mutex_unlock(&drv_mutex);
		return 0;
	}

	/* We only support reading a maximum of a flash page */
	if (count > ctx->mtd->writesize)
		count = ctx->mtd->writesize;

	page_no = (file_offset + offset) / ctx->mtd->writesize;
	page_offset = (file_offset + offset) % ctx->mtd->writesize;

	if (phy_offset(ctx->mtd, (page_no * ctx->mtd->writesize))
		== APANIC_INVALID_OFFSET) {
		pr_err("apanic: reading an invalid address\n");
		mutex_unlock(&drv_mutex);
		return -EINVAL;
	}
	rc = ctx->mtd->_read(ctx->mtd,
		phy_offset(ctx->mtd, (page_no * ctx->mtd->writesize)),
		ctx->mtd->writesize,
		&len, ctx->bounce);

	if (page_offset)
		count -= page_offset;

	if (copy_to_user(buf, ctx->bounce + page_offset,count))
	{
		mutex_unlock(&drv_mutex);
		return -EFAULT;
	}

	*ppos =  offset + count;

	mutex_unlock(&drv_mutex);
	return count;
}

static void mtd_panic_erase(void)
{
	struct apanic_data *ctx = &drv_ctx;
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	int rc, i;
	init_waitqueue_head(&wait_q);
	erase.mtd = ctx->mtd;
	erase.callback = apanic_erase_callback;
	erase.len = ctx->mtd->erasesize;
	erase.priv = (u_long)&wait_q;
	for (i = 0; i < ctx->mtd->size; i += ctx->mtd->erasesize) {
		erase.addr = i;
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		if (get_bb(erase.addr>>ctx->mtd->erasesize_shift, apanic_bbt)) {
			printk(KERN_WARNING
			       "apanic: Skipping erase of bad "
			       "block @%llx\n", erase.addr);
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			continue;
		}

		rc = ctx->mtd->_erase(ctx->mtd, &erase);
		if (rc) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk(KERN_ERR
			       "apanic: Erase of 0x%llx, 0x%llx failed\n",
			       (unsigned long long) erase.addr,
			       (unsigned long long) erase.len);
			if (rc == -EIO) {
				if (ctx->mtd->_block_markbad(ctx->mtd,
							    erase.addr)) {
					printk(KERN_ERR
					       "apanic: Err marking blk bad\n");
					goto out;
				}
				printk(KERN_INFO
				       "apanic: Marked a bad block"
				       " @%llx\n", erase.addr);
				set_bb(erase.addr>>ctx->mtd->erasesize_shift,
					apanic_bbt);
				continue;
			}
			goto out;
		}
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}
	printk(KERN_DEBUG "apanic: %s partition erased\n",
	       mtddev);
out:
	return;
}

static void apanic_remove_proc_work(struct work_struct *work)
{
	struct apanic_data *ctx = &drv_ctx;

	mutex_lock(&drv_mutex);
	mtd_panic_erase();
	memset(&ctx->curr, 0, sizeof(struct panic_header));
	if (ctx->apanic_console) {
		remove_proc_entry("apanic_console", NULL);
		ctx->apanic_console = NULL;
	}
	if (ctx->apanic_threads) {
		remove_proc_entry("apanic_threads", NULL);
		ctx->apanic_threads = NULL;
	}
	mutex_unlock(&drv_mutex);
}

static ssize_t apanic_proc_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *ppos)
{
	schedule_work(&proc_removal_work);
	return count;
}

static int apanic_proc_show(struct seq_file *m, void *v)
{
	return 0;
}

static int apanic_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, apanic_proc_show, PDE_DATA(inode));
}

static const struct file_operations apanic_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= apanic_proc_open,
	.read		= apanic_proc_read,
	.llseek		= seq_lseek,
	.release		= single_release,
	.write		= apanic_proc_write,
};

static int in_panic = 0;

static int apanic_writeflashpage(struct mtd_info *mtd, loff_t to,
				 const u_char *buf)
{
	int rc;
	size_t wlen;
	int panic = in_interrupt() | in_atomic();

	if (panic && !mtd->_panic_write) {
		printk(KERN_EMERG "%s: No panic_write available\n", __func__);
		return 0;
	} else if (!panic && !mtd->_write) {
		printk(KERN_EMERG "%s: No write available\n", __func__);
		return 0;
	}

	to = phy_offset(mtd, to);
	if (to == APANIC_INVALID_OFFSET) {
		printk(KERN_EMERG "apanic: write to invalid address\n");
		return 0;
	}

	if (panic)
		rc = mtd->_panic_write(mtd, to, mtd->writesize, &wlen, buf);
	else
		rc = mtd->_write(mtd, to, mtd->writesize, &wlen, buf);

	if (rc) {
		printk(KERN_EMERG
		       "%s: Error writing data to flash (%d)\n",
		       __func__, rc);
		return rc;
	}

	return wlen;
}

static int apanic_write_console(struct mtd_info *mtd, unsigned int off, struct kmsg_dumper *dumper, bool rewind)
{
	struct apanic_data *ctx = &drv_ctx;
	int saved_oip;
	int idx = 0;
	int rc, rc2;
	bool ret;
	int page_no, page_offset;
	char *buffer;
	int i = 0;

	saved_oip = oops_in_progress;
	oops_in_progress = 1;
	idx = 0;

	if (rewind) {
		kmsg_dump_rewind(dumper);
		printk(KERN_EMERG "apanic: rewind kmsg dumper\n");
	}

	ret = kmsg_dump_get_buffer(dumper, true, ctx->bounce,
			     record_size, &rc);
	if (!ret) {
		printk(KERN_EMERG "apanic: end of kmsg\n");
		return 0;
	}

	if (rc <= 0) {
		printk(KERN_EMERG "apanic: read 0 bytes kmsg\n");
		return 0;
	}

	page_no = rc / mtd->writesize;
	page_offset = rc % mtd->writesize;
	i = 0;
	buffer = NULL;

	printk(KERN_EMERG "apanic: ======page_no:%d, page_offset:%d\n", page_no,page_offset );
	while (page_no) {
		buffer = (char*)ctx->bounce + i *mtd->writesize;
		rc2 = apanic_writeflashpage(mtd, off, buffer);
		if (rc2 <= 0) {
			printk(KERN_EMERG
			     "apanic: Flash write failed (%d)\n", rc2);
			return idx;
		}

		page_no --;
		i++;
		idx += rc2;
		off += rc2;
	}

	if (page_offset)  {
		buffer = (char*)ctx->bounce + i *mtd->writesize;
		memset (buffer+page_offset , 0, mtd->writesize - page_offset);
		rc2 = apanic_writeflashpage(mtd, off, buffer);
		if (rc2 <= 0) {
			printk(KERN_EMERG
			     "apanic: Flash write failed (%d)\n", rc2);
			return idx;
		}

		idx += rc2;
		off += rc2;
	}

	oops_in_progress = saved_oip;
	printk(KERN_EMERG "apanic: Flash success (%d)\n", idx);

	return idx;
}

static void apanic_do_dump(struct kmsg_dumper *dumper,
					enum kmsg_dump_reason reason)

{
	struct apanic_data *ctx = &drv_ctx;
	struct panic_header *hdr = (struct panic_header *) ctx->bounce;
	int console_offset = 0;
	int console_len = 0;
	int threads_offset = 0;
	int threads_len = 0;
	int rc;
	struct timespec now;
	struct timespec uptime;
	struct rtc_time rtc_timestamp;
	//struct console *con;

	if (in_panic)
		return ;
	in_panic = 1;
#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	add_preempt_count(PREEMPT_ACTIVE);
#endif
	touch_softlockup_watchdog();

	if (!ctx->mtd)
		goto out;

	if (ctx->curr.magic) {
		printk(KERN_EMERG "Crash partition in use!\n");
		goto out;
	}

	/*
	 * Add timestamp to displays current UTC time and uptime (in seconds).
	 */
	now = current_kernel_time();
	rtc_time_to_tm((unsigned long)now.tv_sec, &rtc_timestamp);
	do_posix_clock_monotonic_gettime(&uptime);
	bust_spinlocks(1);
	printk(KERN_EMERG "Timestamp = %lu.%03lu\n",
			(unsigned long)now.tv_sec,
			(unsigned long)(now.tv_nsec / 1000000));
	printk(KERN_EMERG "Kernel Core Dump Time = "
			"%02d-%02d %02d:%02d:%lu.%03lu, "
			"Uptime = %lu.%03lu seconds\n",
			rtc_timestamp.tm_mon + 1, rtc_timestamp.tm_mday,
			rtc_timestamp.tm_hour, rtc_timestamp.tm_min,
			(unsigned long)rtc_timestamp.tm_sec,
			(unsigned long)(now.tv_nsec / 1000000),
			(unsigned long)uptime.tv_sec,
			(unsigned long)(uptime.tv_nsec/USEC_PER_SEC));
	printk(KERN_EMERG "========RDA Kernel Trace Log End========\n");
	bust_spinlocks(0);

	console_offset = ctx->mtd->writesize;

	/*
	 * Write out the console
	 */
	console_len = apanic_write_console(ctx->mtd, console_offset,dumper, false);
	if (console_len < 0) {
		printk(KERN_EMERG "Error writing console to panic log! (%d)\n",
		       console_len);
		console_len = 0;
	}


	/*
	 * Write out all threads
	 */
	threads_offset = ALIGN(console_offset + console_len,
			       ctx->mtd->writesize);
	if (!threads_offset)
		threads_offset = ctx->mtd->writesize;

#ifdef CONFIG_ANDROID_RAM_CONSOLE
	//ram_console_enable_console(0);
#endif

	do_syslog(SYSLOG_ACTION_CLEAR, NULL, 0, SYSLOG_FROM_READER);

	show_state_filter(0);
	threads_len = apanic_write_console(ctx->mtd, threads_offset, dumper,true);
	if (threads_len < 0) {
		printk(KERN_EMERG "Error writing threads to panic log! (%d)\n",
		       threads_len);
		threads_len = 0;
	}

	/*
	 * Finally write the panic header
	 */
	memset(ctx->bounce, 0, PAGE_SIZE);
	hdr->magic = PANIC_MAGIC;
	hdr->version = PHDR_VERSION;

	hdr->console_offset = console_offset;
	hdr->console_length = console_len;

	hdr->threads_offset = threads_offset;
	hdr->threads_length = threads_len;

	rc = apanic_writeflashpage(ctx->mtd, 0, ctx->bounce);
	if (rc <= 0) {
		printk(KERN_EMERG "apanic: Header write failed (%d)\n",
		       rc);
		goto out;
	}

	printk(KERN_EMERG "========RDA kernel panic dump sucessfully written to flash========\n");

 out:
#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
#endif
	in_panic = 0;
	return;
}

static void mtd_panic_notify_add(struct mtd_info *mtd)
{
	struct apanic_data *ctx = &drv_ctx;
	struct panic_header *hdr = ctx->bounce;
	size_t len;
	int rc;
	int    proc_entry_created = 0;

	if (strcmp(mtd->name, mtddev))
		return;

	ctx->mtd = mtd;

	alloc_bbt(mtd, apanic_bbt);
	scan_bbt(mtd, apanic_bbt);

	if (apanic_good_blocks == 0) {
		printk(KERN_ERR "apanic: no any good blocks?!\n");
		goto out_err;
	}

	rc = mtd->_read(mtd, phy_offset(mtd, 0), mtd->writesize,
			&len, ctx->bounce);
	if (rc && rc == -EBADMSG) {
		printk(KERN_WARNING
		       "apanic: Bad ECC on block 0 (ignored)\n");
	} else if (rc && rc != -EUCLEAN) {
		printk(KERN_ERR "apanic: Error reading block 0 (%d)\n", rc);
		goto out_err;
	}

	if (len != mtd->writesize) {
		printk(KERN_ERR "apanic: Bad read size (%d)\n", rc);
		goto out_err;
	}

	printk(KERN_INFO "apanic: Bound to mtd partition '%s'\n", mtd->name);

	if (hdr->magic != PANIC_MAGIC) {
		printk(KERN_INFO "apanic: No panic data available\n");
		mtd_panic_erase();
		return;
	}

	if (hdr->version != PHDR_VERSION) {
		printk(KERN_INFO "apanic: Version mismatch (%d != %d)\n",
		       hdr->version, PHDR_VERSION);
		mtd_panic_erase();
		return;
	}

	memcpy(&ctx->curr, hdr, sizeof(struct panic_header));

	printk(KERN_INFO "apanic: c(%u, %u) \n", hdr->console_offset, hdr->console_length);

	if (hdr->console_length) {
		ctx->apanic_console = proc_create_data("apanic_console",
						      S_IFREG | S_IRUGO, NULL, &apanic_proc_fops, (void *) 1);
		if (!ctx->apanic_console)
			printk(KERN_ERR "%s: failed creating procfile\n",
			       __func__);
		else {
			proc_entry_created = 1;
			has_apanic_dump = 1;
		}
	}

	if (hdr->threads_length) {
		ctx->apanic_threads = proc_create_data("apanic_threads",
						       S_IFREG | S_IRUGO, NULL,&apanic_proc_fops, (void *) 2);
		if (!ctx->apanic_threads)
			printk(KERN_ERR "%s: failed creating procfile\n",
			       __func__);
		else {
			proc_entry_created = 1;
		}
	}

	if (!proc_entry_created)
		mtd_panic_erase();

	return;
out_err:
	ctx->mtd = NULL;
}

static void mtd_panic_notify_remove(struct mtd_info *mtd)
{
	struct apanic_data *ctx = &drv_ctx;

	if (mtd == ctx->mtd) {
		ctx->mtd = NULL;
		printk(KERN_INFO "apanic: Unbound from %s\n", mtd->name);
	}
}

#if defined(CONFIG_MTD)
static struct mtd_notifier mtd_panic_notifier = {
	.add	= mtd_panic_notify_add,
	.remove	= mtd_panic_notify_remove,
};
#endif

static int panic_dbg_get(void *data, u64 *val)
{
	struct apanic_data *ctx = &drv_ctx;

	apanic_do_dump(&ctx->dump,KMSG_DUMP_PANIC);
	return 0;
}

static int panic_dbg_set(void *data, u64 val)
{
	BUG();
	return -1;
}

DEFINE_SIMPLE_ATTRIBUTE(panic_dbg_fops, panic_dbg_get, panic_dbg_set, "%llu\n");

static int __init apanic_init(void)
{
	int err;
	struct apanic_data *ctx = &drv_ctx;

#if defined(CONFIG_MTD)
	register_mtd_user(&mtd_panic_notifier);
#endif

	debugfs_create_file("apanic", 0644, NULL, NULL, &panic_dbg_fops);
	memset(&drv_ctx, 0, sizeof(drv_ctx));
	INIT_WORK(&proc_removal_work, apanic_remove_proc_work);

	ctx->dump.max_reason = KMSG_DUMP_PANIC;
	ctx->dump.dump = apanic_do_dump;
	err = kmsg_dump_register(&ctx->dump);
	if (err) {
		printk(KERN_ERR "apanic: registering kmsg dumper failed, error %d\n", err);
		return -EINVAL;
	}

	ctx->bounce = vmalloc(record_size);
	if (!ctx->bounce) {
		printk(KERN_ERR "mtdoops: failed to allocate buffer workspace\n");
		kmsg_dump_unregister(&ctx->dump);
		return -ENOMEM;
	}
	memset(ctx->bounce, 0, record_size);

	printk(KERN_INFO "Android kernel panic handler initialized (bind=%s)\n",
	       mtddev);

	return 0;
}

static void __exit apanic_exit(void)
{
	struct apanic_data *ctx = &drv_ctx;

	vfree(ctx->bounce);
	kmsg_dump_unregister(&ctx->dump);

#if defined(CONFIG_MTD)
	unregister_mtd_user(&mtd_panic_notifier);
#endif
}

module_init(apanic_init);
module_exit(apanic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RDA");
MODULE_DESCRIPTION("Panic console logger/driver");
