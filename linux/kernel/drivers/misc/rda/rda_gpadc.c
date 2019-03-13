/*
 * rda_gpadc.c - Driver for GPADC
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <plat/md_sys.h>
#include <plat/reg_cfg_regs.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define GPADC_CHANNEL_NR_MAX	8
#define CALIB_DATA_SIZE	8

struct rda_gpadc
{
	u8 is_open;
};

static struct rda_gpadc gpadcs[GPADC_CHANNEL_NR_MAX] = {
	[0 ... GPADC_CHANNEL_NR_MAX - 1] = {
		.is_open = 0,
	}
};

static struct msys_device *gpadc_msys = NULL;
static DEFINE_MUTEX(gpadc_mutex);
static int gpadc_init_stat = 0;

static int do_gpadc_init(void);

int rda_gpadc_check_status(void)
{
	if(!gpadc_init_stat){
		if (do_gpadc_init()) {
			printk (KERN_INFO"rda GPADC : do init failed \n");
			return -1;
		}
	}

	return 0;
}

int rda_gpadc_open(u8 channel)
{
	int ret = 0;
	u32 __dat[2];
	struct client_cmd msys_cmd;

	if(channel >= GPADC_CHANNEL_NR_MAX)
		return -1;

	mutex_lock(&gpadc_mutex);

	// then open
	if(!gpadcs[channel].is_open) {
		memset(&msys_cmd, 0, sizeof(msys_cmd));

		__dat[0] = channel;
		__dat[1] = 1;

		msys_cmd.pmsys_dev = gpadc_msys;
		msys_cmd.mod_id = SYS_PM_MOD;
		msys_cmd.mesg_id = SYS_PM_CMD_ENABLE_ADC;
		msys_cmd.pdata = (void *)&__dat;
		msys_cmd.data_size = sizeof(__dat);
		ret = rda_msys_send_cmd(&msys_cmd);

		if(ret == 0) {
			gpadcs[channel].is_open = 1;
		} else {
			pr_err("rda GPADC : SYS_PM_CMD_ENABLE_ADC fail. ret 0x%x\n", ret);
			mutex_unlock(&gpadc_mutex);
			return -1;
		}
	}
	mutex_unlock(&gpadc_mutex);

	return 0;
}

int rda_gpadc_close(u8 channel)
{
	int ret = 0;
	u32 __dat[2];
	struct client_cmd msys_cmd;

	if(channel >= GPADC_CHANNEL_NR_MAX)
		return -1;

	if(!gpadcs[channel].is_open)
		return -1;

	mutex_lock(&gpadc_mutex);

	memset(&msys_cmd, 0, sizeof(msys_cmd));

	__dat[0] = channel;
	__dat[1] = 0;
	msys_cmd.pmsys_dev = gpadc_msys;
	msys_cmd.mod_id = SYS_PM_MOD;
	msys_cmd.mesg_id = SYS_PM_CMD_ENABLE_ADC;
	msys_cmd.pdata = (void *)&__dat;
	msys_cmd.data_size = sizeof(__dat);
	ret = rda_msys_send_cmd(&msys_cmd);

	if(ret < 0) {
		pr_err("rda GPADC : rda_gpadc_close channel %d, err %d \n", channel, ret);
		mutex_unlock(&gpadc_mutex);
		return -1;
	}

	gpadcs[channel].is_open = 0;
	mutex_unlock(&gpadc_mutex);

	return 0;
}

/*
* the adc value is calibrated by modem
* we don't need calibrate it again
*/
int rda_gpadc_get_adc_value(u8 channel)
{
	int ret = 0;
	u32 data = 0;
	u32 adc_value = 0;
	struct client_cmd msys_cmd;

	if(channel >= GPADC_CHANNEL_NR_MAX)
		return -1;

	mutex_lock(&gpadc_mutex);

	data = channel;
	// then open
	if(gpadcs[channel].is_open) {
		memset(&msys_cmd, 0, sizeof(msys_cmd));
		msys_cmd.pmsys_dev = gpadc_msys;
		msys_cmd.mod_id = SYS_PM_MOD;
		msys_cmd.mesg_id = SYS_PM_CMD_ADC_VALUE;
		msys_cmd.pdata = (void *)&data;
		msys_cmd.data_size = sizeof(data);
		msys_cmd.pout_data = (void *)&adc_value;
		msys_cmd.out_size = sizeof(adc_value);

		ret = rda_msys_send_cmd(&msys_cmd);

		if(ret < 0) {
			pr_err("rda GPADC : SYS_PM_CMD_ADC_VALUE fail. ret 0x%x\n", ret);
			mutex_unlock(&gpadc_mutex);
			return -1;
		}
	}

	mutex_unlock(&gpadc_mutex);

	return adc_value;
}

/*
* the calibaration value get from efuse
*/
int rda_gpadc_get_calib_adc_value(u8 channel,void *calib_data)
{
	int ret = 0;
	u32 __dat[2] = { 0 };
	struct client_cmd msys_cmd;

	if(channel >= GPADC_CHANNEL_NR_MAX)
		return -1;

	mutex_lock(&gpadc_mutex);

	// then open
	if(gpadcs[channel].is_open) {
		memset(&msys_cmd, 0, sizeof(msys_cmd));
		msys_cmd.pmsys_dev = gpadc_msys;
		msys_cmd.mod_id = SYS_PM_MOD;
		msys_cmd.mesg_id = SYS_PM_CMD_GET_ADC_CALIB_VALUE;
		msys_cmd.pdata = (void *)&__dat;
		msys_cmd.data_size = sizeof(__dat);
		msys_cmd.pout_data = calib_data;
		msys_cmd.out_size = CALIB_DATA_SIZE;

		ret = rda_msys_send_cmd(&msys_cmd);

		if(ret < 0) {
			pr_err("rda GPADC : SYS_PM_CMD_GET_ADC_CALIB_VALUE fail. ret 0x%x\n", ret);
			mutex_unlock(&gpadc_mutex);
			return -1;
		}
	}

	mutex_unlock(&gpadc_mutex);

	return ret;
}

static int rda_modem_msys_notify(struct notifier_block *nb,
		unsigned long mesg, void *data)
{
	struct client_mesg *pmesg = (struct client_mesg *)data;

	if (pmesg->mod_id != SYS_PM_MOD) {
		return NOTIFY_DONE;
	}

	switch(mesg) {
	default:
		break;
	}

	return 0;
}

static int do_gpadc_init(void)
{
	int ret = 0;

	if(gpadc_init_stat)
		return 0;

	// md sys register
	gpadc_msys = rda_msys_alloc_device();
	if (!gpadc_msys) {
		printk (KERN_INFO"rda GPADC : rda_msys_alloc_device err \n");
		goto err_msys;
	}

	gpadc_msys->module = SYS_PM_MOD;
	gpadc_msys->name = "rda-gpadc";
	gpadc_msys->notifier.notifier_call = rda_modem_msys_notify;
	gpadc_msys->private = (void *)NULL;

	rda_msys_register_device(gpadc_msys);

	gpadc_init_stat = 1;

	printk (KERN_INFO"rda GPADC : initialized \n");
	return ret;

err_msys:
	return -ENOSYS;
}

static int __init dev_init(void)
{
	int ret = 0;

	ret = do_gpadc_init();

	return ret;
}

static void __exit dev_exit(void)
{
	gpadc_init_stat = 0;
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RDA GPADC Driver");
MODULE_AUTHOR("Yulong Wang<yulongwang@rdamicro.com>");
