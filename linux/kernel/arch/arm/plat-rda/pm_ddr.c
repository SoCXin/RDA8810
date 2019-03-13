/*
 * arch/arm/plat-rda/pm_ddr.c
 *
 * CPU idle driver for RDAmicro CPUs
 *
 * Copyright (c) 2014 RDA Micro, Inc.
 * Author: yingchunli<yingchunli@rdamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>
#include <plat/pm_ddr.h>

static atomic64_t pm_ddr_map = ATOMIC64_INIT(0);
static spinlock_t pm_ddr_lock;
static int master_ref_cnt[PM_DDR_MASTER_MAX];

static inline bool ddr_master_is_valid(int master)
{
	return master >= 0 && master < PM_DDR_MASTER_MAX;
}

#if 0
int pm_ddr_enable(int master)
{
	unsigned long flags;

	if(!ddr_master_is_valid(master))
		return -1;

	spin_lock_irqsave(&pm_ddr_lock, flags);
	pm_ddr_unmask |= 1 << master;
	spin_unlock_irqrestore(&pm_ddr_lock, flags);
	return 0;
}
EXPORT_SYMBOL(pm_ddr_enable);
#endif

int pm_ddr_get(enum ddr_master master)
{
	unsigned long flags;
	s64 value;

	if (!ddr_master_is_valid(master))
		return -1;

	spin_lock_irqsave(&pm_ddr_lock, flags);
	if (master_ref_cnt[master] == 0) {
		value = atomic64_read(&pm_ddr_map);
		value |= 1 << master;
		atomic64_set(&pm_ddr_map, value);
	} else {
		printk("master %d already hold ddr\n", master);
	}
	master_ref_cnt[master]++;
	spin_unlock_irqrestore(&pm_ddr_lock, flags);
	return 0;
}
EXPORT_SYMBOL(pm_ddr_get);

int pm_ddr_put(enum ddr_master master)
{
	unsigned long flags;
	s64 value;

	if (!ddr_master_is_valid(master))
		return -1;

	spin_lock_irqsave(&pm_ddr_lock, flags);
	if (master_ref_cnt[master] == 0) {
		spin_unlock_irqrestore(&pm_ddr_lock, flags);
		return -1;
	}
	if (--master_ref_cnt[master] > 0) {
		spin_unlock_irqrestore(&pm_ddr_lock, flags);
		return -1;
	}
	value = atomic64_read(&pm_ddr_map);
	value &= ~(1 << master);
	atomic64_set(&pm_ddr_map, value);
	spin_unlock_irqrestore(&pm_ddr_lock, flags);
	return 0;
}
EXPORT_SYMBOL(pm_ddr_put);

int pm_ddr_idle_status(void)
{
	if (atomic64_read(&pm_ddr_map) == 0)
		return 1;
	else
		return 0;
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
static const char *pm_ddr_info[PM_DDR_MASTER_MAX] = {
	[PM_DDR_CPU] = "cpu",
	[PM_DDR_VOC] = "voc",
	[PM_DDR_IFC0] = "ifc0",
	[PM_DDR_IFC1] = "ifc1",
	[PM_DDR_IFC2] = "ifc2",
	[PM_DDR_IFC3] = "ifc3",
	[PM_DDR_IFC4] = "ifc4",
	[PM_DDR_IFC5] = "ifc5",
	[PM_DDR_IFC6] = "ifc6",
	[PM_DDR_IFC7] = "ifc7",
	[PM_DDR_USB_DMA0] = "usb dma0",
	[PM_DDR_USB_DMA1] = "usb dma1",
	[PM_DDR_USB_DMA2] = "usb dma2",
	[PM_DDR_USB_DMA3] = "usb dma3",
	[PM_DDR_USB_DMA4] = "usb dma4",
	[PM_DDR_USB_DMA5] = "usb dma5",
	[PM_DDR_USB_DMA6] = "usb dma6",
	[PM_DDR_USB_DMA7] = "usb dma7",
	[PM_DDR_SD_DMA0] = "sd dma0",
	[PM_DDR_SD_DMA1] = "sd dma1",
	[PM_DDR_VPU_DMA] = "vpu",
	[PM_DDR_GPU_DMA] = "gpu",
	[PM_DDR_GOUDA_DMA] = "gouda",
	[PM_DDR_FB_DMA] = "fb",
	[PM_DDR_CAMERA_DMA] = "camera",
	[PM_DDR_AUDIO_IFC_PLAYBACK] = "audio playback",
	[PM_DDR_AUDIO_IFC_CAPTURE] = "audio capture",
};

static int pm_ddr_debug_show(struct seq_file *s, void *data)
{
	s64 ddr_masters = atomic64_read(&pm_ddr_map);
	int i;

	seq_printf(s, "ddr master        \t\tstatus\n");
	for (i = 0; i < PM_DDR_MASTER_MAX; i++) {
		int active = (ddr_masters & (1 << i)) ? 1 : 0;
		seq_printf(s, "%s\t\t %s\n", pm_ddr_info[i],
				active ? "active" : "inactive");
	}

	return 0;
}

static int pm_ddr_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, pm_ddr_debug_show, NULL);
}

static const struct file_operations pm_ddr_debug_fops = {
	.open = pm_ddr_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int __init pm_ddr_init(void)
{
	struct dentry *d;

	if (PM_DDR_MASTER_MAX > 64) {
		pr_err("too many ddr masters\n");
		return -ENOMEM;
	}
	spin_lock_init(&pm_ddr_lock);
#ifdef CONFIG_DEBUG_FS
	d = debugfs_create_file("ddr_master", S_IRUGO, NULL, NULL,
			&pm_ddr_debug_fops);
	if (!d) {
		pr_err("fail to create debug file for ddr master\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

core_initcall(pm_ddr_init);

#include <rda/tgt_ap_board_config.h>
 /*   if you met VPU bug, please enable _TGT_AP_VPU_DDR_DYM_EN
 *    tgt_ap_board_config.h of your target
 * */
#ifdef _TGT_AP_VPU_DDR_DYM_EN
#include <plat/ap_clk.h>
#include <linux/delay.h>
#include <rad/tgt_ap_clock_config.h>
#include <plat/rda_debug.h>

static atomic_t ddr_freq_users = ATOMIC_INIT(0);
static volatile int vpu_bug_ddr_freq_set_done = false;

static void vpu_bug_ddr_freq_poll(void)
{
	while(vpu_bug_ddr_freq_set_done != true)
	       msleep(5);
	/* printk(KERN_INFO"%s done\n", __func__); */
}
extern int rda_gpu_idle_status(void);
void vpu_bug_do_ddr_freq_adjust(bool is_request)
{
	/*
	*  stop the cpu and ask modem to change ddr freq
	*  this must be done
	* */

	unsigned long irq_flags;
	unsigned int i;

	i = 0;
	local_save_flags(irq_flags);
	while(1) {
		if (!is_request && atomic_read(&ddr_freq_users)) {
			/* release is ongoing, but somebody else does
			 * request while this thread is sleeping
			 * */
			local_irq_restore(irq_flags);
			return;
		}
		i++;
		if (!pm_ddr_idle_status() || !rda_gpu_idle_status()) {
			local_irq_restore(irq_flags);
			if ((i & 0xF) == 0)
				printk(KERN_INFO"try to poll ddr %d times\n", i);
			msleep(1);
			local_save_flags(irq_flags);
		} else {
			break;
		}
	}

	while (1) {

		i = apsys_request_for_vpu_bug(is_request);
		if (i == 0) {
			if (is_request)
				vpu_bug_ddr_freq_set_done = true;
			else
				vpu_bug_ddr_freq_set_done = false;
			local_irq_restore(irq_flags);
			printk(KERN_INFO"%s to change ddr timing done\n",
				       is_request?"request":"restore");
			return;
		}
	}
}

/**
 * lower DDR freq, this should be called by
 * 1. VPU driver when vpu is open
 * 2. any driver that holds ddr long enough, such as camera
 * 3. can only be called in process context as it may sleep
 */
void vpu_bug_ddr_freq_adjust(void)
{

	if (1 == atomic_add_return(1, &ddr_freq_users))
		vpu_bug_do_ddr_freq_adjust(1);
	else
		vpu_bug_ddr_freq_poll();
}
/*
 * restore the DDR freq, this should be called by anydriver that
 * call vpu_bug_ddr_freq_adjust
 */
void vpu_bug_ddr_freq_adjust_restore(void)
{
	if (atomic_dec_and_test(&ddr_freq_users))
		vpu_bug_do_ddr_freq_adjust(0);
}

#else
void vpu_bug_ddr_freq_adjust(void) {}
void vpu_bug_ddr_freq_adjust_restore(void) {}
#endif

EXPORT_SYMBOL_GPL(vpu_bug_ddr_freq_adjust);
EXPORT_SYMBOL_GPL(vpu_bug_ddr_freq_adjust_restore);


