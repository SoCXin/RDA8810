/*
 * md_sys.c - A driver for controlling sys channel of modem of RDA
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
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>

#include <plat/md_sys.h>
#include <plat/reg_cfg_regs.h>
#include <plat/cpu.h>

#include <rda/tgt_ap_board_config.h>

#ifndef CONFIG_RDA_FPGA

#if 0
#define MSYS_DUMP_FRAME
#endif /* #if 0 */

#define MSYS_DEBUG_CLIENT	1

#if 0
#include <linux/ktime.h>

#define MSYS_TIME_STATISTICS
#endif /* #if 0 */

#define MODEM_BUILD_TIME_BUF_LEN	(32)
#define MODEM_MOD_VER_BUF_LEN		(32)

#define RDA_CALIB_STATUS_QTY		(4)
#define RDA_RFCALIB_RESULT_QTY		(2)

enum rda_mod_type {
	RDA_MOD_HAL = 0,
	RDA_MOD_AT,
	RDA_MOD_CSW,
	RDA_MOD_STACK,

	RDA_MOD_QTY,
};

#ifndef CONFIG_RDA_FPGA
static const char *gbp_mod_name[RDA_MOD_QTY] = {
	"HAL",
	"AT",
	"CSW",
	"STACK",
};

static const char *gcalib_status[RDA_CALIB_STATUS_QTY] = {
	"Calibrated",
	"Calibrated but CRC is bad",
	"Not calibrated",
	"Unknown",
};

static const char *grfcalib_result[RDA_RFCALIB_RESULT_QTY] = {
	"-",
	"P",
};

static const char *grfcalib_item[] = {
	"GPADC",
	"CRYSTAL",
	"850 AFC",
	"EGSM AFC",
	"DCS AFC",
	"PCS AFC",
	"850 PA PROF",
	"EGSM PA PROF",
	"DCS PA PROF",
	"PCS PA PROF",
	"850 PA OFS",
	"EGSM PA OFS",
	"DCS PA OFS",
	"PCS PA OFS",
	"850 ILOSS",
	"EGSM ILOSS",
	"DCS ILOSS",
	"PCS ILOSS",
	"CABLE TEST",
	"RADIO TEST",
};
#endif /* CONFIG_RDA_FPGA */

struct rda_mod_ver {
	unsigned int revision;
	unsigned int build_date;
};

struct rda_bp_info {
	struct rda_mod_ver ver[RDA_MOD_QTY];
	char build_time[MODEM_BUILD_TIME_BUF_LEN];
};

/*Defined for debugfs*/
struct file_attr {
	char buf[1024];
	struct mutex mutex;
	size_t (*get_string)(char *buf, size_t len);
};

/* Declare a header of notifier */
static BLOCKING_NOTIFIER_HEAD(msys_notifier);

static int gmsys_timeout_dbg = 0;
static unsigned int gmsys_tx = 0;
static unsigned int gmsys_rx = 0;
static struct msys_master *gmsys = NULL;
static struct msys_device *gmsys_dev = NULL;
static struct rda_bp_info gbp_info;
static u32 gcalib_info[3];
static int msys_async_done = 0;

static char gbp_mod_ver[RDA_MOD_QTY][MODEM_MOD_VER_BUF_LEN];

/* Declaration */
static inline void rda_msys_mesg_init(struct msys_message *);
static unsigned int rda_msys_sync_write_timeout(struct msys_message *mesg, int timeout);
static unsigned int rda_msys_sync_write(struct msys_message *);
static void rda_msys_rx_work(struct work_struct *);
static void rda_msys_rx_notify(void *, unsigned int);
static inline int rda_msys_init_rx_queue(struct msys_master *);
static int rda_msys_probe(struct platform_device *);

#ifndef CONFIG_RDA_FPGA
static void rda_msys_complete(void *);
static int rda_msys_pump_mesg(struct msys_message *);
static void rda_msys_fill_version(void);
#endif /* CONFIG_RDA_FPGA */

/* Definition */
#ifndef CONFIG_RDA_FPGA
static void rda_msys_complete(void *arg)
{
	complete(arg);
}
#endif /* CONFIG_RDA_FPGA */

static inline void rda_msys_mesg_init(struct msys_message *mesg)
{
	memset(mesg, 0, sizeof *mesg);
	INIT_LIST_HEAD(&mesg->list);

	return;
}

#ifndef CONFIG_RDA_FPGA
static unsigned int rda_msys_sync_write_timeout(struct msys_message *mesg, int timeout)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int ret;
	unsigned int status = 0;
	struct msys_master *pmsys = NULL;
	int try_count = 0;

	mesg->complete = rda_msys_complete;
	mesg->context = &done;
	mesg->tx_len = 0;
	mesg->rx_len = 0;
	mesg->status = -1;

	if (mesg->pmsys_dev == NULL) {
		status = SYS_CMD_FAIL;
	} else {
		pmsys = mesg->pmsys_dev->pmaster;

		if (rda_md_check_bp_status(pmsys->port)) {
			pr_warn("rda_msys: %s : bp is power-off!\n",
				mesg->pmsys_dev->name ? mesg->pmsys_dev->name : "null");
			while (1) {
				/* Enter a loop until AP is power-off. */
			}
		}
		/*
		 * Put message to pending list and waiting for response from bp.
		 * Note: We have to invoke it at first, because response from bp may
		 * be returned quickly, otherwise we would hung with complete.
		 */
		spin_lock(&pmsys->pending_lock);
		list_add_tail(&mesg->list, &pmsys->pending_list);
		spin_unlock(&pmsys->pending_lock);

		/*
		 * If it returns -EBUSY, it means that we cann't write data to sys channel.
		 * So, we can try write data again.
		 */
		do {
			mutex_lock(&pmsys->tx_mutex);
			ret = rda_msys_pump_mesg(mesg);
			mutex_unlock(&pmsys->tx_mutex);

			if (ret == -EBUSY) {
				try_count++;
				if (try_count == 10) {
					pr_err("rda_msys : %s : no available space!\n",
						mesg->pmsys_dev->name ? mesg->pmsys_dev->name : "null");
					/* Notify bp to dump context. */
					rda_md_notify_bp(pmsys->port);
					BUG_ON(1);
				}
			}
		} while (ret == -EBUSY);


		if (ret < 0) {
			/* Remove item from list as failure. */
			spin_lock(&pmsys->pending_lock);
			list_del(&mesg->list);
			spin_unlock(&pmsys->pending_lock);

			status = SYS_CMD_FAIL;
		}
	}

	if (status == 0) {
		ret = wait_for_completion_timeout(&done,
				msecs_to_jiffies(timeout));
		if (!ret) {
			/* Remove item from list as timeout. */
			spin_lock(&pmsys->pending_lock);
			list_del(&mesg->list);
			spin_unlock(&pmsys->pending_lock);
			/* Timeout as waiting */
			if (rda_md_check_bp_status(pmsys->port)) {
				pr_warn("rda_msys: %s : timeout as bp is power-off!\n",
					mesg->pmsys_dev->name ? mesg->pmsys_dev->name : "null");
				while (1) {
					/* Enter a loop until AP is power-off. */
				}
			} else {
				pr_warn("rda_msys: %s : timeout as sending a message\n",
					mesg->pmsys_dev->name ? mesg->pmsys_dev->name : "null");
			}

			if (gmsys_timeout_dbg) {
				/* Notify bp there is a timeout issue. */
				rda_md_notify_bp(pmsys->port);
				BUG_ON(1);
			}

			status = EPERM;
		} else {
			/*
			 * If successful, we don't delete mesg from list.
			 * It has been removed by rx worker.
			 */
			status = mesg->status;
		}
	}

	return status;
}

static unsigned int rda_msys_sync_write(struct msys_message *mesg)
{
	/* 500ms */
	return rda_msys_sync_write_timeout(mesg, 500);
}

#else

static unsigned int rda_msys_sync_write_timeout(struct msys_message *mesg, int timeout)
{
	return SYS_CMD_FAIL;
}

static unsigned int rda_msys_sync_write(struct msys_message *mesg)
{
	return SYS_CMD_FAIL;
}
#endif /* CONFIG_RDA_FPGA */

unsigned int rda_msys_send_cmd_timeout(struct client_cmd *pcmd, int timeout)
{
	struct msys_message sys_mesg;

	rda_msys_mesg_init(&sys_mesg);

	if (!IS_ALIGNED(pcmd->data_size, 4)) {
		WARN(1, "data size must be aligned to 4\n");
		return SYS_CMD_FAIL;
	}
	sys_mesg.pmsys_dev = pcmd->pmsys_dev;
	sys_mesg.mod_id = pcmd->mod_id;
	sys_mesg.mesg_id = pcmd->mesg_id;
	sys_mesg.pdata = pcmd->pdata;
	sys_mesg.data_size = pcmd->data_size;
	/* Init pointer and size of out direction. */
	sys_mesg.pbp_data = pcmd->pout_data;
	sys_mesg.bp_data_size = pcmd->out_size;

	return rda_msys_sync_write_timeout(&sys_mesg, timeout);
}
EXPORT_SYMBOL_GPL(rda_msys_send_cmd_timeout);

unsigned int rda_msys_send_cmd(struct client_cmd *pcmd)
{
	struct msys_message sys_mesg;

	rda_msys_mesg_init(&sys_mesg);

	if (!IS_ALIGNED(pcmd->data_size, 4)) {
		WARN(1, "data size must be aligned to 4\n");
		return SYS_CMD_FAIL;
	}
	sys_mesg.pmsys_dev = pcmd->pmsys_dev;
	sys_mesg.mod_id = pcmd->mod_id;
	sys_mesg.mesg_id = pcmd->mesg_id;
	sys_mesg.pdata = pcmd->pdata;
	sys_mesg.data_size = pcmd->data_size;
	/* Init pointer and size of out direction. */
	sys_mesg.pbp_data = pcmd->pout_data;
	sys_mesg.bp_data_size = pcmd->out_size;

	return rda_msys_sync_write(&sys_mesg);
}
EXPORT_SYMBOL_GPL(rda_msys_send_cmd);

struct msys_device *rda_msys_alloc_device(void)
{
	struct msys_device *msys_dev = NULL;

	msys_dev = kzalloc(sizeof *msys_dev, GFP_KERNEL);
	if (!msys_dev) {
		pr_err("rda_msys: can not alloc msys_device\n");
		return NULL;
	}

	msys_dev->pmaster = gmsys;

	return msys_dev;
}
EXPORT_SYMBOL_GPL(rda_msys_alloc_device);

void rda_msys_free_device(struct msys_device *pmsys_dev)
{
	/* Sanity checking */
	if (pmsys_dev) {
		kfree(pmsys_dev);
	}
}
EXPORT_SYMBOL_GPL(rda_msys_free_device);

int rda_msys_register_device(struct msys_device *pmsys_dev)
{
	struct msys_master *pmaster = NULL;
	struct msys_device *pdev = NULL;
	int ret = 0;

	/* Sanity checking */
	if (!pmsys_dev) {
		return -EPERM;
	}

	pmaster = pmsys_dev->pmaster;
	if (!pmaster) {
		pr_err("rda_msys: Invalid sub-device of msys!\n");
		return -EINVAL;
	}

	spin_lock(&pmaster->client_lock);
	/* Go throught the list to find if the device has been registered before. */
	list_for_each_entry(pdev, &pmaster->client_list, dev_entry) {
		if (&pdev->dev_entry == &pmsys_dev->dev_entry) {
			spin_unlock(&pmaster->client_lock);
			return 0;
		}
	}

	list_add_tail(&pmsys_dev->dev_entry, &pmaster->client_list);

	spin_unlock(&pmaster->client_lock);

	if (pmsys_dev->notifier.notifier_call) {
		/* If nb is not NULL, we have to register device with notifier chain. */
		ret = blocking_notifier_chain_register(&msys_notifier, &pmsys_dev->notifier);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(rda_msys_register_device);

int rda_msys_unregister_device(struct msys_device *pmsys_dev)
{
	struct msys_master *pmaster = NULL;
	struct msys_device *pdev = NULL;
	struct msys_device *temp_dev = NULL;
	int ret = 0;

	/* Sanity checking */
	if (!pmsys_dev) {
		return -EPERM;
	}

	if (pmsys_dev->notifier.notifier_call) {
		ret = blocking_notifier_chain_unregister(&msys_notifier, &pmsys_dev->notifier);
		if (ret < 0) {
			pr_err("rda_msys: Failure as unregister notifier!\n");
			return ret;
		}
	}

	pmaster = pmsys_dev->pmaster;
	if (!pmaster) {
		pr_err("rda_msys: Invalid sub-device of msys!\n");
		return -EINVAL;
	}

	spin_lock(&pmaster->client_lock);
	/* Go throught the list to find if the device has been registered before. */
	list_for_each_entry_safe(pdev, temp_dev, &pmaster->client_list, dev_entry) {
		if (&pdev->dev_entry == &pmsys_dev->dev_entry) {
			list_del(&pdev->dev_entry);
			break;
		}
	}
	spin_unlock(&pmaster->client_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(rda_msys_unregister_device);

#ifndef CONFIG_RDA_FPGA
static void rda_msys_get_version(char *buf, size_t len,
		struct rda_mod_ver *ver, const char *name)
{
	u32 rev = ver->revision;

	memset(buf, len, 0);

	if (rev & 0x80000000) {
		/* It is a svn's revision*/
		/* 0xFFFFFFFF stands for an invalid revision of svn */
		if (rev != 0xFFFFFFFF)
			rev &= 0x7FFFFFFF;
		snprintf(buf, len, "%6s R(%d)-%d", name, rev, ver->build_date);
	} else {
		/* It is a git's revision*/
		snprintf(buf, len, "%6s G(%X)-%d", name, rev, ver->build_date);
	}

	buf[len - 1] = '\0';
}

static void rda_msys_fill_version(void)
{
	int i;

	gbp_info.build_time[MODEM_BUILD_TIME_BUF_LEN - 1] = '\0';

	for (i = 0; i < RDA_MOD_QTY; i++)
		rda_msys_get_version(gbp_mod_ver[i], sizeof(gbp_mod_ver[i]),
				&gbp_info.ver[i], gbp_mod_name[i]);
}
#endif /* CONFIG_RDA_FPGA */

#ifdef CONFIG_PROC_FS
/*====================================================================*/
/* Support for /proc/modem_version */

static struct proc_dir_entry *proc_md;

static int md_proc_show(struct seq_file *m, void *v)
{
	int i;
	u16 prod_id = rda_get_soc_prod_id();
	u16 metal_id = rda_get_soc_metal_id();

	seq_printf(m, "Chip %04X Metal %02d\n", prod_id, metal_id);
	seq_printf(m, "------------------\n");
	for (i = 0; i < RDA_MOD_QTY; i++)
		seq_printf(m, "%s\n", gbp_mod_ver[i]);
	seq_printf(m, "Built on %s\n", gbp_info.build_time);
	seq_printf(m, "------------------\n");
	seq_printf(m, "   RF: %s\n",
			gcalib_status[gcalib_info[0]]);
	seq_printf(m, "Audio: %s\n",
			gcalib_status[gcalib_info[1]]);

	return 0;
}

static int md_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, md_proc_show, NULL);
}

static const struct file_operations md_proc_ops = {
	.open = md_proc_open,
	.read	= seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*====================================================================*/
/* Support for /proc/modem_rfcalib */

static struct proc_dir_entry *proc_rfcalib;

static int rfcalib_proc_show(struct seq_file *m, void *v)
{
	int i;
	int result;

	for (i = 0; i < ARRAY_SIZE(grfcalib_item); i++) {
		/* No gpadc calib */
		if (i == 0)
			continue;
		/* Show calib result only if calibrated by offical tool */
		if ((gcalib_info[0] == 0 || gcalib_info[0] == 1) &&
				(gcalib_info[2] & 0x80000000)) {
			/* GSM850 PA profile is the same as EGSM's, and
			 * PCS PA profile is the same as DCS's */
			if (i == 6)
				result = (gcalib_info[2] & (1 << 7)) ? 1 : 0;
			else if (i == 9)
				result = (gcalib_info[2] & (1 << 8)) ? 1 : 0;
			else
				result = (gcalib_info[2] & (1 << i)) ? 1 : 0;
		} else {
			result = 0;
		}
		seq_printf(m, "%-14s:%s\n",
				grfcalib_item[i],
				grfcalib_result[result]);
	}

	return 0;
}

static int rfcalib_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rfcalib_proc_show, NULL);
}

static const struct file_operations rfcalib_proc_ops = {
	.open = rfcalib_proc_open,
	.read	= seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif /* CONFIG_PROC_FS */

#ifndef CONFIG_RDA_FPGA
static int rda_msys_async_msg_handler(struct msys_master *pmsys, struct msys_frame_slot *pslot)
{
	struct client_mesg *cmesg = &pmsys->cli_msg;
	struct md_sys_hdr_ext *phdr = pslot->phdr;
	int notify_ret = 0;
	int panic_flag;
#ifdef MSYS_TIME_STATISTICS
	ktime_t tm_pre;
	ktime_t tm_cur;
	ktime_t tm_delta;
#endif /* MSYS_TIME_STATISTICS */

	if (phdr->mod_id == SYS_GEN_MOD &&
		phdr->msg_id == SYS_GEN_MESG_UNKNOWN) {
		/* Output the header */
		pr_err("rda_msys: Modem Interface Version(0x%08x), Header :\n",
			pmsys->version);
		pr_err("rda_msys: mod_id = 0x%04x, msg_id = 0x%04x, "
			"req_id = 0x%08x, ret_val = 0x%08x, ext_len = 0x%x\n",
			phdr->mod_id, phdr->msg_id, phdr->req_id,
			phdr->ret_val, phdr->ext_len);

		// Allow to run modem which doesn't support calib info cmd
		if (phdr->ret_val ==
				((SYS_CALIB_CMD_CALIB_STATUS << 16) |
				 SYS_CALIB_MOD))
			panic_flag = 0;
		else
			panic_flag = 1;

		if (panic_flag)
			panic("rda_msys: Received unknown Command!\n");
		else
			pr_err("rda_msys: Received unknown command!\n");
	}

	if (phdr->msg_id == SYS_GEN_MESG_VER &&
		phdr->mod_id == SYS_GEN_MOD) {
		pslot->using = 0;
		/* Nothing to do, return immediately. */
		return 0;
	}

	memset(cmesg, 0, sizeof(*cmesg));

	cmesg->mod_id = phdr->mod_id;
	cmesg->mesg_id = phdr->msg_id;
	cmesg->param_size = phdr->ext_len;
	if (phdr->ext_len) {
		memcpy((void *)&cmesg->param, (void *)phdr->ext_data, phdr->ext_len);
	}

#ifdef MSYS_TIME_STATISTICS
	tm_pre = ktime_get();
#endif /* MSYS_TIME_STATISTICS */

	/* It is a notify from bp. */
	notify_ret = blocking_notifier_call_chain(
		&msys_notifier,
		phdr->msg_id,
		(void *)cmesg);

	pslot->using = 0;

#ifdef MSYS_TIME_STATISTICS
	tm_cur = ktime_get();
	tm_delta = ktime_sub(tm_cur, tm_pre);
	pr_info("rda_msys : mod = 0x%x, msg = 0x%x, time = %lld us\n",
		cmesg->mod_id, cmesg->mesg_id, ktime_to_us(tm_delta));
#endif /* MSYS_TIME_STATISTICS */

	return notify_ret;
}
#endif /* CONFIG_RDA_FPGA */

#define MD_SYS_HDR_SIZE	sizeof(struct md_sys_hdr)

#ifndef CONFIG_RDA_FPGA

static int rda_msys_find_slot(struct msys_master *pmsys)
{
	int i;

	for (i = 0; i < MSYS_MAX_SLOTS; i++) {
		if (!pmsys->fr_slot[i].using) {
			pmsys->fr_slot[i].using = 1;
			return i;
		}
	}

	return -EINVAL;
}

static void rda_msys_rx_work(struct work_struct *work)
{
	struct msys_master *pmsys = container_of(work, struct msys_master, rx_work);
	struct md_port *port = pmsys->port;
	int ret = 0;
	int rsize = 0;
	int slot;
	void *pmsg = NULL;
	struct md_sys_hdr_ext *phdr_ext = NULL;
	struct msys_message *pending_msg = NULL;
	struct msys_message *temp_msg = NULL;
	struct msys_frame_slot *async_slot = NULL;
	struct msys_frame_slot *temp_slot = NULL;

#ifdef MSYS_DUMP_FRAME
	pr_info("rda_msys: rx work, port[%d]\n", port->port_id);
#endif /* MSYS_DUMP_FRAME */

	if (msys_async_done && !list_empty(&pmsys->rx_pending_list)) {
		list_for_each_entry_safe(async_slot, temp_slot, &pmsys->rx_pending_list, list) {
			list_del(&async_slot->list);
			rda_msys_async_msg_handler(pmsys,  async_slot);
		}
		/*
		 * After we processed all async mesg which is pending, we just return.
		 * It doesn't lost messages send by BP, because work_queue always is triggered by queue_work.
		 */
		return;
	}

	/* Get all frames from sys channel. */
	while ((ret = rda_md_read_avail(port)) > 0) {
		rsize = rda_md_read(port, NULL, 0);
		if (rsize < 0) {
			break;
		}

		slot = rda_msys_find_slot(pmsys);
		if (slot < 0) {
			pr_err("rda_msys: rx work, no slot\n");
			break;
		}

		pmsg = (void *)pmsys->fr_slot[slot].phdr;
		rsize = rda_md_read(port, pmsg, rsize);
		if (rsize < 0) {
			pr_err("rda_msys: rx work, sys exception\n");
			kfree(pmsg);
			break;
		}

		gmsys_rx += rsize;

		phdr_ext = (struct md_sys_hdr_ext *)pmsg;

#ifdef MSYS_DUMP_FRAME
		pr_info("rda_msys: rsize = 0x%x, magic = 0x%04x\n",
		       rsize, phdr_ext->magic);
		pr_info("rda_msys: mod = 0x%04x, msg = 0x%04x, ext = 0x%x\n",
			phdr_ext->mod_id, phdr_ext->msg_id, phdr_ext->ext_len);
		pr_info("rda_msys: req = 0x%08x, ret = 0x%08x\n",
			phdr_ext->req_id, phdr_ext->ret_val);
#endif /* MSYS_DUMP_FRAME */

		/* Process synchronised messages */
		if (phdr_ext->msg_id & SYS_REPLY_FLAG) {
			int msg_done;

			msg_done = 0;
			/* It is a reply message from bp. */
			spin_lock(&pmsys->pending_lock);
			/* Go throught the list to find out the pending message. */
			list_for_each_entry_safe(pending_msg, temp_msg, &pmsys->pending_list, list) {
				/* Check if it is the message we want to reply. */
				if (((unsigned int)pending_msg->pmsys_dev) == phdr_ext->req_id &&
					pending_msg->mesg_id == (phdr_ext->msg_id & SYS_MESG_MASK)) {

					/* Remove the specified item from list */
					list_del_init(&pending_msg->list);

					/* Hold the return value */
					pending_msg->rx_len = rsize;
					pending_msg->status = phdr_ext->ret_val;

#ifdef MSYS_DEBUG_CLIENT
					//pr_info("<rda_msys>: pid = %d, %s: ext_len = %d\n",
					//	current->pid, (pending_msg->pmsys_dev->name ? pending_msg->pmsys_dev->name : "null"), phdr_ext->ret_val);
#endif /* MSYS_DEBUG_CLIENT */
					if (phdr_ext->ext_len) {
						/* Bp wants to return some parameters to AP. */
						if (pending_msg->pbp_data &&
							pending_msg->bp_data_size >= phdr_ext->ext_len) {
							memcpy(pending_msg->pbp_data, phdr_ext->ext_data, phdr_ext->ext_len);
						} else {
							pending_msg->status = ENOMEM;
							pr_err("rda_msys: no enough space for param of bp. bp=%d, ext=%d\n",
									pending_msg->bp_data_size, phdr_ext->ext_len);
						}
					}

					/* Wake up the sender */
					if (pending_msg->complete) {
						pending_msg->complete(pending_msg->context);
					}
#ifdef MSYS_DEBUG_CLIENT
					pr_info("<rda_msys>: end of pending msg\n");
#endif /* MSYS_DEBUG_CLIENT */
					msg_done = 1;
					break;
				}
			}
			spin_unlock(&pmsys->pending_lock);

			if (!msg_done) {
				pr_warn("<rda_msys> : no owner : mod = 0x%04x, msg = 0x%04x, req = 0x%08x, ret = 0x%08x\n",
					phdr_ext->mod_id, phdr_ext->msg_id, phdr_ext->req_id, phdr_ext->ret_val);
			}

			pmsys->fr_slot[slot].using = 0;
		}
		else {
			if (msys_async_done) {
				/*
				 * If async_done flag has been set,
				 * it means that the receiver of messages have been registered with mdsys.
				 */
				rda_msys_async_msg_handler(pmsys, &pmsys->fr_slot[slot]);
			} else {
				list_add_tail(&pmsys->fr_slot[slot].list, &pmsys->rx_pending_list);
			}
		}
	}

	return;
}
#else
static void rda_msys_rx_work(struct work_struct *work)
{
	return;
}
#endif /* CONFIG_RDA_FPGA */

#ifndef CONFIG_RDA_FPGA
static void rda_msys_rx_notify(void *priv, unsigned int event)
{
	struct msys_master *pmsys = (struct msys_master *)priv;
	int ret = 0;

	ret = queue_work(pmsys->rx_wq, &pmsys->rx_work);
	if (!ret) {
		/*
		 * Work was already on a queue. The previous work is processing.
		 * We don't worry about this case. There is a while loop will handle the later one
		 * after previous one is done.
		 */
		pr_warn("rda_msys: channel is busy!\n");
	}

	return;
}
#else
static void rda_msys_rx_notify(void *priv, unsigned int event)
{
	return;
}
#endif /* CONFIG_RDA_FPGA */

#ifndef CONFIG_RDA_FPGA

static int rda_msys_pump_mesg(struct msys_message *mesg)
{
	struct msys_master *pmsys = mesg->pmsys_dev->pmaster;
	struct md_sys_hdr *phdr;
	struct md_sys_hdr_ext *phdr_ext;
	struct msys_message *pmesg = mesg;
	const void *psrc;
	int pkt_size;
	char rda_msys_frame[MD_SYS_PACKET_MAX];
	int ret;

	/* Make a packet. */
	phdr = (struct md_sys_hdr *)rda_msys_frame;
	phdr->magic = MD_SYS_MAGIC;
	phdr->mod_id = pmesg->mod_id;
	phdr->msg_id = pmesg->mesg_id;
	phdr->ret_val = 0;
	phdr->req_id = (unsigned int)pmesg->pmsys_dev;
	phdr->ext_len = pmesg->data_size;

#ifdef MSYS_DUMP_CMD
	pr_info("sys_msys: magic = 0x%04x, mod = 0x%04x,"
		" msg = 0x%04x, req = 0x%08x, ext = 0x%x\n",
		phdr->magic, phdr->mod_id, phdr->msg_id,
		phdr->req_id, phdr->ext_len);
#endif /* MSYS_DUMP_CMD */

	pkt_size = sizeof(struct md_sys_hdr);

	if (phdr->ext_len > 0 && phdr->ext_len <= (MD_SYS_PACKET_MAX - MD_SYS_HDR_SIZE)) {
		psrc = pmesg->pdata;
		phdr_ext = (struct md_sys_hdr_ext *)rda_msys_frame;

		memcpy((void *)phdr_ext->ext_data, psrc, phdr_ext->ext_len);
		pkt_size += phdr_ext->ext_len;
	} else if (phdr->ext_len > (MD_SYS_PACKET_MAX - MD_SYS_HDR_SIZE)) {
		pr_err("rda_msys : too larger frame\n");
		BUG_ON(1);
	}

	ret = rda_md_write(pmsys->port, (void *)&rda_msys_frame[0], pkt_size);
	if (ret > 0) {
		pmesg->tx_len = ret;
		gmsys_tx += ret;
	}

#ifdef MSYS_DUMP_CMD
	pr_info("sys_msys: write size = 0x%x\n", pmesg->tx_len);
#endif /* MSYS_DUMP_CMD */

	return ret;
}
#endif /* CONFIG_RDA_FPGA */

static inline int rda_msys_init_rx_queue(struct msys_master *pmsys)
{
	/* Init work of rx and create workqueue of rx. */
	INIT_WORK(&pmsys->rx_work, rda_msys_rx_work);

	pmsys->rx_wq = create_singlethread_workqueue(dev_name(&pmsys->dev));
	if (!pmsys->rx_wq) {
		return -EBUSY;
	}

	return 0;
}

static inline int rda_msys_init_slot(struct msys_master *pmsys)
{
	int i;
	int blk_size = (2 * PAGE_SIZE) / MSYS_MAX_SLOTS;

	pmsys->pslot_data = kzalloc(2 * PAGE_SIZE, GFP_KERNEL);
	if (!pmsys->pslot_data) {
		pr_err("rda_msys : could not allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < MSYS_MAX_SLOTS; i++) {
		pmsys->fr_slot[i].phdr = (struct md_sys_hdr_ext *)(pmsys->pslot_data + i * blk_size);
		pmsys->fr_slot[i].using = 0;
	}

	return 0;
}

static void rda_msys_free_slot(struct msys_master *pmsys)
{
	if (pmsys->pslot_data) {
		kfree(pmsys->pslot_data);
	}
}

static ssize_t rda_mdsys_show_timeout_dbg(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gmsys_timeout_dbg);
}

static ssize_t rda_mdsys_store_timeout_dbg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;

	rc = kstrtoint(buf, 0, &gmsys_timeout_dbg);
	if (rc) {
		return rc;
	}

	return count;
}

static ssize_t rda_mdsys_tx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gmsys_tx);
}

static ssize_t rda_mdsys_rx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gmsys_rx);
}

static struct device_attribute rda_mdsys_attributes[] = {
	__ATTR(timeout_dbg, S_IWUSR | S_IWGRP | S_IRUGO, rda_mdsys_show_timeout_dbg, rda_mdsys_store_timeout_dbg),
	__ATTR(mdsys_tx, S_IRUGO, rda_mdsys_tx_show, NULL),
	__ATTR(mdsys_rx, S_IRUGO, rda_mdsys_rx_show, NULL),
};

static int rda_msys_probe(struct platform_device *pdev)
{
	int ret;
	int attr_idx;
	struct msys_master *pmsys = NULL;
#ifndef CONFIG_RDA_FPGA
	int i;
	unsigned int sys_ret = 0;
	struct client_cmd cmd_set;
#endif /* CONFIG_RDA_FPGA */

	pmsys = kzalloc(sizeof(struct msys_master), GFP_KERNEL);
	if (!pmsys) {
		pr_err("rda_msys: Allocate memory failed!\n");
		BUG();
		return -ENOMEM;
	}

	gmsys = pmsys;
	pmsys->dev = pdev->dev;

	spin_lock_init(&pmsys->client_lock);
	INIT_LIST_HEAD(&pmsys->client_list);

	pmsys->running = false;

	ret = rda_msys_init_slot(pmsys);
	if (ret < 0) {
		goto err_handle_slot;
	}

	/* Init work of rx and create workqueue of rx. */
	ret = rda_msys_init_rx_queue(pmsys);
	if (ret < 0) {
		goto err_handle_rx;
	}

	mutex_init(&pmsys->tx_mutex);

	spin_lock_init(&pmsys->pending_lock);
	INIT_LIST_HEAD(&pmsys->pending_list);
	INIT_LIST_HEAD(&pmsys->rx_pending_list);

	pmsys->running = true;

	/* Open sys channel and attach rx's callback function. */
	ret = rda_md_open(MD_PORT_SYS, &pmsys->port, pmsys, rda_msys_rx_notify);
	if (ret < 0) {
		pr_err("rda_msys: can not get modem's port!\n");
		goto err_handle_port;
	}

	for (attr_idx = 0; attr_idx < ARRAY_SIZE(rda_mdsys_attributes); attr_idx++) {
		device_create_file(&pdev->dev, &rda_mdsys_attributes[attr_idx]);
	}

	pmsys->version = rda_md_query_ver(pmsys->port);

	memset(&gbp_info, sizeof(gbp_info), 0);
	gmsys_dev = rda_msys_alloc_device();
	if (!gmsys_dev) {
		pr_err("rda_msys: can not allocate msys device!\n");
		ret = -ENOMEM;
		goto err_handle_msys;
	}

#ifndef CONFIG_RDA_FPGA
	rda_msys_register_device(gmsys_dev);

	cmd_set.pmsys_dev = gmsys_dev;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_BP_INFO;
	cmd_set.pdata = NULL;
	cmd_set.data_size = 0;
	cmd_set.pout_data = &gbp_info;
	cmd_set.out_size = sizeof(gbp_info);

	sys_ret = rda_msys_send_cmd(&cmd_set);
	if (sys_ret) {
		pr_err("rda_msys: can not get info of bp!\n");
		ret = -EINVAL;
		goto err_handle_cmd;
	}

	gcalib_info[0] = gcalib_info[1] = RDA_CALIB_STATUS_QTY - 1;

	cmd_set.pmsys_dev = gmsys_dev;
	cmd_set.mod_id = SYS_CALIB_MOD;
	cmd_set.mesg_id = SYS_CALIB_CMD_CALIB_STATUS;
	cmd_set.pdata = NULL;
	cmd_set.data_size = 0;
	cmd_set.pout_data = gcalib_info;
	cmd_set.out_size = sizeof(gcalib_info);

	sys_ret = rda_msys_send_cmd(&cmd_set);
	if (sys_ret) {
		pr_err("rda_msys: can not get info of calibration!\n");
	}
	if (gcalib_info[0] >= RDA_CALIB_STATUS_QTY) {
		pr_err("rda_msys: Invalid RF calib status: %d\n",
				gcalib_info[0]);
		gcalib_info[0] = RDA_CALIB_STATUS_QTY - 1;
	}
	if (gcalib_info[1] >= RDA_CALIB_STATUS_QTY) {
		pr_err("rda_msys: Invalid audio calib status: %d\n",
				gcalib_info[1]);
		gcalib_info[1] = RDA_CALIB_STATUS_QTY - 1;
	}

	rda_msys_unregister_device(gmsys_dev);

	rda_msys_fill_version();

	for (i = 0; i < RDA_MOD_QTY; i++)
		pr_info("%s\n", gbp_mod_ver[i]);
	pr_info("Built on %s\n", gbp_info.build_time);
	pr_info("------------------\n");
	pr_info("RF   : %s\n",
			gcalib_status[gcalib_info[0]]);
	pr_info("Audio: %s\n",
			gcalib_status[gcalib_info[1]]);
#endif /* CONFIG_RDA_FPGA */

	pr_info("rda_msys: msys initialized\n");

	return ret;

#ifndef CONFIG_RDA_FPGA
err_handle_cmd:
	rda_msys_unregister_device(gmsys_dev);
#endif /* CONFIG_RDA_FPGA */

err_handle_msys:
err_handle_port:
	pmsys->running = false;
	destroy_workqueue(pmsys->rx_wq);

err_handle_rx:
	rda_msys_free_slot(pmsys);

err_handle_slot:
	if (pmsys) {
		kfree(pmsys);
	}

	gmsys = NULL;
	/* any error here should halt the system */
	BUG();
	return ret;
}

static struct platform_driver rda_msys_driver = {
	.probe = rda_msys_probe,
	.driver = {
		.name = RDA_MSYS_DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init rda_msys_init(void)
{
	return platform_driver_register(&rda_msys_driver);
}
arch_initcall(rda_msys_init);

static size_t get_info_users(char *buf, size_t len)
{
	unsigned long size = 0;
	struct msys_device *pdev = NULL;

	spin_lock(&gmsys->client_lock);
	list_for_each_entry(pdev, &gmsys->client_list, dev_entry) {
		if (pdev->name) {
			size += scnprintf(buf+size, len-size, "%s\n", pdev->name);
		}
	}
	spin_unlock(&gmsys->client_lock);

	return size;
}

static ssize_t file_read(struct file *file, char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct file_attr *attr;
	size_t size;
	ssize_t ret;

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

static int file_open(struct inode *inode, struct file *file)
{
	struct file_attr *attr;

	attr = kmalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr)
		return -ENOMEM;

	attr->buf[0] = '\0';
	mutex_init(&attr->mutex);
	attr->get_string = inode->i_private;
	file->private_data = attr;

	return nonseekable_open(inode, file);
}

static int file_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static const struct file_operations fops_msys = {
	.read =		file_read,
	.write =	NULL,
	.open =		file_open,
	.release = 	file_release,
	.llseek =	NULL,
};

static int __init msys_dbinfo_init(void)
{
	struct dentry *rootdir = NULL;
	struct dentry *msys = NULL;

	rootdir = debugfs_create_dir("msys", NULL);
	if (!rootdir)
		return -ENOMEM;

	msys = debugfs_create_file("users", S_IRUGO, rootdir, &get_info_users, &fops_msys);
	if (!msys)
		return -ENOMEM;

	return 0;
}

/**
 * Notify rx worker thread to process all pending messages send by BP.
 */
static int __init rda_msys_late_init(void)
{
	int ret = 0;

	msys_async_done = 1;

	do {
		ret = queue_work(gmsys->rx_wq, &gmsys->rx_work);
		if (ret > 0) {
			/*
			 * To check if the work item is queued.
			 * If not, we will continue to append it, until it is done.
			 */
			break;
		}
	} while (1);

	ret = msys_dbinfo_init();
	if (ret)
		pr_err("rda_msys: debugfs init failed!\n");

#ifdef CONFIG_PROC_FS
	proc_md = proc_create("modem_version", 0, NULL, &md_proc_ops);
	proc_rfcalib = proc_create("modem_rfcalib", 0, NULL, &rfcalib_proc_ops);
#endif

	return 0;
}
late_initcall(rda_msys_late_init);

MODULE_AUTHOR("Jason Tao<leitao@rdamicro.com>");
MODULE_DESCRIPTION("RDA sys channel of modem driver");
MODULE_LICENSE("GPL");

#else /* CONFIG_FPGA_RDA defined */

unsigned int rda_msys_send_cmd_timeout(struct client_cmd *pcmd, int timeout)
{
	return 0;
}
EXPORT_SYMBOL_GPL(rda_msys_send_cmd_timeout);

unsigned int rda_msys_send_cmd(struct client_cmd *pcmd)
{
	return 0;
}
EXPORT_SYMBOL_GPL(rda_msys_send_cmd);

struct msys_device *rda_msys_alloc_device(void)
{
	return NULL;
}
EXPORT_SYMBOL_GPL(rda_msys_alloc_device);

int rda_msys_register_device(struct msys_device *pmsys_dev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(rda_msys_register_device);

int rda_msys_unregister_device(struct msys_device *pmsys_dev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(rda_msys_unregister_device);

void rda_msys_free_device(struct msys_device *pmsys_dev)
{

}
EXPORT_SYMBOL_GPL(rda_msys_free_device);

#endif /* CONFIG_FPGA_RDA */
