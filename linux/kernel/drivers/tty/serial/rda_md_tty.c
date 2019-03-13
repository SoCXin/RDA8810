
/*
 * rda_md_tty.c - A tty driver for commucating with modem of RDA
 *
 * Copyright (C) 2013 RDA Microelectronics Inc.
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
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/wait.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include <plat/rda_md.h>
#include <plat/md_sys.h>
#include <plat/rda_debug.h>

#define RDA_MD_MAJOR     204
#define RDA_MD_MINOR    220

#define MD_AT_TTY	0
#define MD_TRACE_TTY	1
#define MAX_MD_TTYS     2

#define MSYS_TRACE_CMD_TO_DBGHOST	0
#define MSYS_TRACE_CMD_TO_AP	2

struct md_tty_info {
	struct device *pdev;

	struct tty_port port;
	struct md_port *md_port;

	struct msys_device *msys_dev;
};

struct md_tty_port_desc {
	int id;
	const char *name;
};

static ssize_t md_tty_at_tx_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t md_tty_at_rx_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static struct device_attribute md_tty_at_attrs[] = {
	__ATTR(at_tx, S_IRUGO, md_tty_at_tx_show, NULL),
	__ATTR(at_rx, S_IRUGO, md_tty_at_rx_show, NULL),
};

static ssize_t md_tty_show_trace(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t md_tty_store_trace(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static struct device_attribute md_tty_trace_attrs[] = {
	__ATTR(bp_trace, S_IRUGO | S_IWUSR | S_IWGRP, md_tty_show_trace, md_tty_store_trace),
};

static struct md_tty_info md_tty[MAX_MD_TTYS];

static const struct md_tty_port_desc md_default_tty_ports[MAX_MD_TTYS] = {
	{
		.id = 0,
		.name = "rda-at",
	},
	{
		.id = 2,
		.name = "rda-trace",
	},
};

static const struct md_tty_port_desc *md_tty_ports = md_default_tty_ports;
static unsigned int gmd_tty_at_rx;
static unsigned int gmd_tty_at_tx;

static ssize_t md_tty_at_tx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gmd_tty_at_tx);
}

static ssize_t md_tty_at_rx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gmd_tty_at_rx);
}

static ssize_t md_tty_show_trace(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct client_cmd cmd_set;
	struct md_tty_info *info = &md_tty[MD_TRACE_TTY];
	unsigned int status;
	unsigned int ret;

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = info->msys_dev;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_QUERY_TRACE_STATUS;

	cmd_set.pout_data = (void *)&status;
	cmd_set.out_size = sizeof(status);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("<trace-ch> : failed as getting status of trace channel!\n");
		return -EINVAL;
	}

	return sprintf(buf, "%d\n", status);
}

static ssize_t md_tty_store_trace(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int rc;
	int trace;
	struct client_cmd cmd_set;
	struct md_tty_info *info = &md_tty[MD_TRACE_TTY];
	unsigned int sub_cmd;
	unsigned int ret;

	rc = kstrtoint(buf, 0, &trace);
	if (rc) {
		return rc;
	}

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = info->msys_dev;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_ENABLE_TRACE_LOG;

	sub_cmd = (!!trace);
	cmd_set.pdata = (void *)&sub_cmd;
	cmd_set.data_size = sizeof(sub_cmd);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("<trace-ch> : failed as %s log of bp!\n",
				!!trace ? "enable" : "disable");
		return -EINVAL;
	}

	return count;
}

static ssize_t bootlogo_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	char path[50], *b;
	struct client_cmd cmd_set;
	struct md_tty_info *info = &md_tty[MD_TRACE_TTY];
	unsigned int ret;

	memset(path, 0, sizeof(path));
	strncpy(path, buf, sizeof(path));
	b = strim(path);
	pr_info("store bootlogo path %s\n", b);

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = info->msys_dev;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_SET_LOGO_NAME;

	cmd_set.pdata = (void *)b;
	cmd_set.data_size = ALIGN(strlen(b), 4);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("<trace-ch> : failed as setting name of bootlogo!\n");
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(bootlogo, S_IWUSR, NULL, bootlogo_store);
static struct device_attribute *apfact_attrs[] = {
	&dev_attr_bootlogo,
};

static void md_tty_notify(void *priv, unsigned event)
{
	unsigned char *ptr;
	int avail;
	int tty_space;
	int rbytes;
	struct md_tty_info *info = priv;
	struct tty_struct *tty;

	if (event != MD_EVENT_DATA) {
		return;
	}

	tty = tty_port_tty_get(&info->port);
	if (!tty) {
		return;
	}

	rda_dbg_mdcom("%s : tty[%d]\n", __func__, tty->index);

	for (;;) {
		if (test_bit(TTY_THROTTLED, &tty->flags)) {
			pr_info("%s : tty[%d] : TTY_THROTTLED\n", __func__, tty->index);
			break;
		}

		avail = rda_md_read_avail(info->md_port);
		if (avail == 0) {
			break;
		}

		tty_space = tty_prepare_flip_string(&info->port, &ptr, avail);
		if ((rbytes = rda_md_read(info->md_port, ptr, tty_space)) != tty_space) {
		}

		if (info->md_port->port_id == MD_PORT_AT) {
			/* Bytes statics of AT channel */
			gmd_tty_at_rx += rbytes;
		}

		tty_flip_buffer_push(&info->port);
	}

	/* XXX only when writable and necessary */
	tty_wakeup(tty);
	tty_kref_put(tty);
	if (info->md_port->port_id == MD_PORT_AT) {
		/* stop suspend for 500ms */
		pm_wakeup_event(info->pdev, 500);
	}
}

static int md_tty_port_activate(struct tty_port *tport, struct tty_struct *tty)
{
	int ret = 0;
	int md_num;
	struct md_tty_info *info = &md_tty[tty->index];

	md_num = md_tty_ports[tty->index].id;

	rda_dbg_mdcom("%s : tty[%d], md's port[%d]\n", __func__, tty->index, md_num);

	if (info->md_port) {
		/* Nothing to do */
	} else {
		ret = rda_md_open(md_num, &info->md_port, info, md_tty_notify);
	}

	if (!ret) {
		tty->driver_data = info;
	}

	return ret;
}

static void md_tty_port_shutdown(struct tty_port *tport)
{
	struct md_tty_info *info;
	struct tty_struct *tty = tty_port_tty_get(tport);

	rda_dbg_mdcom("%s : tty[%d]\n", __func__, tty->index);

	info = tty->driver_data;
	if (info->md_port) {
		rda_md_close(info->md_port);
		info->md_port = NULL;
	}

	tty->driver_data = NULL;
	tty_kref_put(tty);
}

static int md_tty_open(struct tty_struct *tty, struct file *f)
{
	struct md_tty_info *info = &md_tty[tty->index];
	struct client_cmd cmd_set;
	unsigned int sub_cmd;
	unsigned int ret;

	rda_dbg_mdcom("%s : tty[%d]\n", __func__, tty->index);

	if (tty->index == MD_TRACE_TTY) {
		memset(&cmd_set, 0, sizeof(cmd_set));

		cmd_set.pmsys_dev = info->msys_dev;
		cmd_set.mod_id = SYS_GEN_MOD;
		cmd_set.mesg_id = SYS_GEN_CMD_TRACE;

		sub_cmd = MSYS_TRACE_CMD_TO_AP;
		cmd_set.pdata = (void *)&sub_cmd;
		cmd_set.data_size = sizeof(sub_cmd);

		ret = rda_msys_send_cmd(&cmd_set);
		if (ret > 0) {
			pr_err("<trace-ch> : failed as changing dbghost to ap!\n");
			return -EINVAL;
		}
	}

	return tty_port_open(&info->port, tty, f);
}

static void md_tty_close(struct tty_struct *tty, struct file *f)
{
	struct md_tty_info *info = tty->driver_data;
	struct client_cmd cmd_set;
	unsigned int sub_cmd;
	unsigned int ret;

	rda_dbg_mdcom("%s : tty[%d]\n", __func__, tty->index);

	if (tty->index == MD_TRACE_TTY) {
		memset(&cmd_set, 0, sizeof(cmd_set));

		cmd_set.pmsys_dev = info->msys_dev;
		cmd_set.mod_id = SYS_GEN_MOD;
		cmd_set.mesg_id = SYS_GEN_CMD_TRACE;

		sub_cmd = MSYS_TRACE_CMD_TO_DBGHOST;
		cmd_set.pdata = (void *)&sub_cmd;
		cmd_set.data_size = sizeof(sub_cmd);

		ret = rda_msys_send_cmd(&cmd_set);
		if (ret > 0) {
			pr_err("<trace-ch> : failed as changing ap to dbghost!\n");
		}
	}

	tty_port_close(&info->port, tty, f);
}

static int md_tty_write(struct tty_struct *tty, const unsigned char *buf, int len)
{
	struct md_tty_info *info = tty->driver_data;
	int avail;
	int wbytes;
	int try_num = 3;

	/* Try to check if there are any space in AT channel. */
	while ((avail = rda_md_write_avail(info->md_port)) == 0) {
		--try_num;
		if (try_num == 0) {
			break;
		}
	}

	/*
	 * If no, we must return -EAGAIN instead of zero.
	 * Zero byte will tell TTY core to schedule our process.
	 */
	if (avail == 0) {
		return -EAGAIN;
	}

	if (len > avail) {
		len = avail;
	}

	rda_dbg_mdcom("tty[%d] : len = %d, avail = %d\n", tty->index, len, avail);

	wbytes = rda_md_write(info->md_port, buf, len);
	if ((info->md_port->port_id == MD_PORT_AT) && wbytes >= 0) {
		gmd_tty_at_tx += wbytes;
	}

	return wbytes;
}

static int md_tty_write_room(struct tty_struct *tty)
{
	struct md_tty_info *info = tty->driver_data;

	rda_dbg_mdcom("%s : tty[%d]\n", __func__, tty->index);
	return rda_md_write_avail(info->md_port);
}

static int md_tty_chars_in_buffer(struct tty_struct *tty)
{
	struct md_tty_info *info = tty->driver_data;

	rda_dbg_mdcom("%s : tty[%d]\n", __func__, tty->index);
	return rda_md_read_avail(info->md_port);
}

static void md_tty_unthrottle(struct tty_struct *tty)
{
	struct md_tty_info *info = tty->driver_data;

	pr_info("%s : tty[%d] : UNTHROTLED\n", __func__, tty->index);

	/* Notify tty again. */
	rda_md_kick(info->md_port);
	return;
}

static const struct tty_port_operations md_tty_port_ops = {
	.shutdown = md_tty_port_shutdown,
	.activate = md_tty_port_activate,
};

static int md_tty_install(struct tty_driver *drv,struct tty_struct *tty)
{
	struct md_tty_info *info = &md_tty[tty->index];
	unsigned int ret;

	ret = tty_port_install(&info->port, drv, tty);
	pr_info("%s result %d \n",__func__,ret);
	return ret;
}

static const struct tty_operations md_tty_ops = {
	.open = md_tty_open,
	.close = md_tty_close,
	.write = md_tty_write,
	.write_room = md_tty_write_room,
	.chars_in_buffer = md_tty_chars_in_buffer,
	.unthrottle = md_tty_unthrottle,
	.install = md_tty_install,
};

static struct tty_driver *md_tty_driver;

static int __init md_tty_init(void)
{
	int ret;
	int i;
	int j;

	md_tty_driver = alloc_tty_driver(MAX_MD_TTYS);
	if (md_tty_driver == 0) {
		return -ENOMEM;
	}

	md_tty_driver->driver_name = "md_tty_driver";
	md_tty_driver->name = "modem";
	md_tty_driver->major = RDA_MD_MAJOR;
	md_tty_driver->minor_start = RDA_MD_MINOR;
	md_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	md_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	md_tty_driver->init_termios = tty_std_termios;
	md_tty_driver->init_termios.c_iflag = 0;
	md_tty_driver->init_termios.c_oflag = 0;
	md_tty_driver->init_termios.c_cflag = B921600 | CS8 | CREAD;
	md_tty_driver->init_termios.c_lflag = 0;
	md_tty_driver->flags = TTY_DRIVER_RESET_TERMIOS |
			       TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty_set_operations(md_tty_driver, &md_tty_ops);

	ret = tty_register_driver(md_tty_driver);
	if (ret) {
		return ret;
	}

	for (i = 0; i < MAX_MD_TTYS; i++) {
		tty_port_init(&md_tty[i].port);
		md_tty[i].port.ops = &md_tty_port_ops;

		md_tty[i].pdev = tty_register_device(md_tty_driver, i, 0);
		if (IS_ERR(md_tty[i].pdev)) {
			pr_err("<md_tty> : port%d could not register device with tty!\n", i);
			ret = -ENODEV;
			goto err_register_dev;
		}
	}

	for (i = 0; i < ARRAY_SIZE(md_tty_at_attrs); i++) {
		ret = device_create_file(md_tty[0].pdev, &md_tty_at_attrs[i]);
	}

	for (i = 0; i < ARRAY_SIZE(md_tty_trace_attrs); i++) {
		ret = device_create_file(md_tty[1].pdev, &md_tty_trace_attrs[i]);
	}

	for (i = 0; i < ARRAY_SIZE(apfact_attrs); i++) {
		ret = device_create_file(md_tty[1].pdev, apfact_attrs[i]);
	}

	md_tty[MD_TRACE_TTY].msys_dev = rda_msys_alloc_device();
	md_tty[MD_TRACE_TTY].msys_dev->module = SYS_GEN_MOD;
	md_tty[MD_TRACE_TTY].msys_dev->name = "rda-trace";
	ret = rda_msys_register_device(md_tty[MD_TRACE_TTY].msys_dev);
	if (ret < 0) {
		pr_err("<trace-ch> : could not register with mdcom\n");
		goto msys_register_failed;
	}

	ret = device_init_wakeup(md_tty[MD_AT_TTY].pdev, 1);
	if (ret < 0) {
		pr_err("failed to call device_init_wakeup for AT device\n");
		goto msys_register_failed;
	}
	return ret;

msys_register_failed:
	rda_msys_free_device(md_tty[MD_TRACE_TTY].msys_dev);
err_register_dev:
	for (j = 0; j < i; j++) {
		tty_unregister_device(md_tty_driver, j);
	}

	tty_unregister_driver(md_tty_driver);

	return ret;
}

module_init(md_tty_init);

