 /*
  * Copyright (C) 2013-2014 RDA Microelectronics Inc.
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <mach/iomap.h>
#include <plat/reg_modem_xcpu.h>
#include <plat/rda_md.h>

#ifndef CONFIG_RDA_FPGA

struct str_file_attr {
	char buf[200];
	struct mutex mutex;
	size_t (*get_string)(char *buf, size_t len);
};

static HWP_XCPU_T *hwp_bcpu =
	((HWP_XCPU_T *)IO_ADDRESS(RDA_MODEM_BCPU_BASE));
static HWP_XCPU_T *hwp_xcpu =
	((HWP_XCPU_T *)IO_ADDRESS(RDA_MODEM_XCPU_BASE));

static size_t get_diag_string(char *buf, size_t len, int xcpu)
{
	size_t size;
	HWP_XCPU_T *cpu;
	char name;

	if (xcpu) {
		cpu = hwp_xcpu;
		name = 'X';
	} else {
		cpu = hwp_bcpu;
		name = 'B';
	}

	size = scnprintf(buf, len,
			"Modem %cCPU info:\n"
			"----------------\n"
			"Cause = 0x%08x\n"
			"Status = 0x%08x\n"
			"BadAddr = 0x%08x\n"
			"EPC = 0x%08x\n"
			"RA = 0x%08x\n"
			"PC = 0x%08x\n"
			"----------------\n",
			name,
			cpu->cp0_Cause,
			cpu->cp0_Status,
			cpu->cp0_BadVAddr,
			cpu->cp0_EPC,
			cpu->Regfile_RA,
			cpu->rf0_addr);

	return size;
}

static size_t get_xcpu_diag_string(char *buf, size_t len)
{
	return get_diag_string(buf, len, 1);
}

static size_t get_bcpu_diag_string(char *buf, size_t len)
{
	return get_diag_string(buf, len, 0);
}

static size_t get_exception_string(char *buf, size_t len)
{
	size_t size;
	u32 addr, count;
	char __iomem *exception;

	rda_md_get_exception_info(&addr, &count);
	if (addr == 0 || count == 0) {
		size = 0;
	} else {
		size = len;
		if (size > count)
			size = count;
		exception = ioremap(addr, size);
		if (exception) {
			size = strnlen(exception, size);
			memcpy(buf, exception, size);
			BUG_ON(len < 2);
			if (size > len - 2)
				size = len - 2;
			buf[size] = '\n';
			buf[size + 1] = '\0';
		} else {
			pr_err("get_exception_string: ioremap failed\n");
			size = 0;
		}
	}

	return size;
}

static ssize_t str_read_file(struct file *file, char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct str_file_attr *attr;
	size_t size;
	ssize_t ret;

	/* pr_info("str_read_file: count=%d, pos=%lld\n", count, *ppos); */

	attr = file->private_data;

        ret = mutex_lock_interruptible(&attr->mutex);
        if (ret)
                return ret;

	if (*ppos)	/* continued read */
		size = strlen(attr->buf);
	else		/* first read */
		size = attr->get_string(attr->buf, sizeof(attr->buf));

	ret = simple_read_from_buffer(buf, count, ppos, attr->buf, size);

        mutex_unlock(&attr->mutex);
        return ret;
}

#if 0
static ssize_t str_write_file(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	return count;
}
#endif

static int str_file_open(struct inode *inode, struct file *file)
{
	struct str_file_attr *attr;

	attr = kmalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr)
		return -ENOMEM;

	attr->buf[0] = '\0';
        mutex_init(&attr->mutex);
	attr->get_string = inode->i_private;

	file->private_data = attr;

        return nonseekable_open(inode, file);
}

static int str_file_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static const struct file_operations fops_str = {
	.read =		str_read_file,
	.write =	NULL,
	.open =		str_file_open,
	.release = 	str_file_release,
	/* .llseek =	noop_llseek, */
	/* .llseek =	generic_file_llseek, */
	.llseek =	NULL,
};

struct dentry *debugfs_create_string(const char *name, struct dentry *parent,
		size_t (*get_string)(char *buf, size_t len))
{
	return debugfs_create_file(name, S_IRUGO, parent, get_string,
			&fops_str);
}

static int __init modem_diag_init(void)
{
	struct dentry *rootdir = NULL;
	struct dentry *d = NULL;
	struct dentry *db = NULL;
	struct dentry *e = NULL;

	rootdir = debugfs_create_dir("modem", NULL);
	if (!rootdir)
		return -ENOMEM;

	d = debugfs_create_string("diag", rootdir, &get_xcpu_diag_string);
	if (!d)
		goto out;

	db = debugfs_create_string("diagb", rootdir, &get_bcpu_diag_string);
	if (!db)
		goto out;

	e = debugfs_create_string("exception_info", rootdir,
			&get_exception_string);
	if (!e)
		goto out;

out:
	return 0;
}
late_initcall(modem_diag_init);

#endif /* CONFIG_RDA_FPGA */
