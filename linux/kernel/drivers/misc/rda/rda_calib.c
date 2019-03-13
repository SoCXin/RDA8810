/*
 * rda_calib.c - Driver for calib
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
#include <linux/miscdevice.h>
#include <plat/md_sys.h>
#include <plat/reg_cfg_regs.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include "rda_calib.h"
#include "calib_m.h"

#define DEBUG 1

#define CHECK_ITF(itf) \
	(itf >=0 && itf < CALIB_AUDIO_ITF_QTY)

#define CALIBDATA_DEV_NAME  "rdacalib"
#define CALIBDATA_SYS_CLASS_NAME  "rdacalib"
/* 0 -- dynamic allocation; xxx -- static allocation */
#define CALIBDATA_DEV_MAJOR	0
#define CALIBDATA_DEV_MINOR	0
static int calibdata_dev_major = CALIBDATA_DEV_MAJOR;
static int calibdata_dev_minor = CALIBDATA_DEV_MINOR;

static struct calibdata *g_calibdata = NULL;

struct calibdata {
	unsigned int buf_addr;
	unsigned int buf_size;
	CALIB_AUDIO_ITF_T *calibp;
	CALIB_AUDIO_IIR_PARAM_T *iirParam;

	struct class *sys_class;
	struct msys_device *msys_dev;

	struct cdev cdev;
	dev_t cdev_t;
};

static char *dev_names[CALIB_AUDIO_ITF_QTY] = {
	"receiver",
	"headset",
	"speaker",
	"bt",
	"calibdata",
	"tv",
};

static long calibdata_ops_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg);
static int calibdata_ops_open(struct inode *inode, struct file *filp);
static int calibdata_ops_release(struct inode *inode, struct file *filp);

//Add for Audio Calibration -----start-----
static int get_calib_data(unsigned int cmd, unsigned long itf,
		void *pout, struct calibdata *calibdata);
//Add for Audio Calibration -----end -----

static struct file_operations calibdata_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = calibdata_ops_ioctl,
	.open = calibdata_ops_open,
	.release = calibdata_ops_release,
};

DEFINE_SEMAPHORE(calibdata_ops_mutex);

static int name_to_index(const char *name)
{
	int i = 0;
	if (name == NULL)
		return -1;

	for (i = 0; i < CALIB_AUDIO_ITF_QTY; ++i) {
		if (name[0] == dev_names[i][0])
			return i;
	}

	return -1;
}

static ssize_t audioGains_get(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	const char *name = dev_name(dev);
	printk(KERN_INFO "rda calibdata: dev name - %s, index - %d\n",
	       (char *)name, name_to_index(name));

	return 0;
}

static ssize_t audioGains_set(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t len)
{
	printk(KERN_INFO "rda calibdata: %s - not implemented\n", __func__);
	return len;
}

static DEVICE_ATTR(audioGains, S_IRUGO | S_IWUSR, audioGains_get,
		   audioGains_set);

static struct class *create_calibdata_sysclass(void)
{
	struct class *class = NULL;
	class = class_create(THIS_MODULE, CALIBDATA_SYS_CLASS_NAME);
	if (IS_ERR(class))
		return NULL;	/* PTR_ERR(class) */

	return class;
}

//Add for Audio Calibration -----start-----
static int get_calib_data(unsigned int cmd, unsigned long itf,
		void *pout, struct calibdata *calibdata)
{
	u32 *p = NULL;
	u32 md_buf[2] = { 0 };
	u32 input_itf = itf;
	struct client_cmd calibdata_cmd;
	int ret = 0;

	printk(KERN_INFO"get calib data cmd=%x, itf=%ld\n",cmd,itf);

	memset(&calibdata_cmd, 0, sizeof(calibdata_cmd));
	calibdata_cmd.pmsys_dev = calibdata->msys_dev;
	calibdata_cmd.mod_id = SYS_AUDIO_MOD;
	calibdata_cmd.mesg_id = cmd;
	calibdata_cmd.pdata = (void*)&input_itf;
	calibdata_cmd.data_size = sizeof(input_itf);
	calibdata_cmd.pout_data = (void *)&md_buf;
	calibdata_cmd.out_size = sizeof(md_buf);
	ret = rda_msys_send_cmd(&calibdata_cmd);
	if(ret > 0) {
		printk(KERN_ERR "rda calibdata: send get audio calib cmd fail.\n");
		ret = -EIO;
		return ret;
	}

	// we got md buf, we need to convert and iomap
	calibdata->buf_size = md_buf[1];
	calibdata->buf_addr = MD_ADDRESS_TO_AP(md_buf[0]);
	printk(KERN_INFO"rda calibdata: get audio calib data addr 0x%x, size 0x%x\n",
			calibdata->buf_addr, calibdata->buf_size);

	if(calibdata->buf_addr <= 0) {
		printk(KERN_ERR "rda calibdata: wrong address of buffer\n");
		ret = -EIO;
		return ret;
	}

	// remap
	p = ioremap(calibdata->buf_addr, calibdata->buf_size);
	if(p <= 0) {
		printk(KERN_ERR "rda calibdata: ioremap error\n");
		ret = -EIO;
		return ret;
	}
	memcpy(pout, p, calibdata->buf_size);

	iounmap(p);
	return ret;
}
//Add for Audio Calibration -----end -----

static long calibdata_ops_ioctl(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct calibdata *calibdata = container_of(filp->f_dentry->d_inode->i_cdev,
			 	struct calibdata, cdev);

	switch (cmd) {
	case CALIBDATA_IOCTL_GET_MIC_GAINS:
		{
			struct get_mic_gains_param param;
			CALIB_AUDIO_IN_GAINS_T *in_gains;
			if (copy_from_user(&param, (void *)arg, sizeof(param))) {
				return -EFAULT;
			}

			if (!CHECK_ITF(param.itf)) {
				pr_err("rda calibdata: GET_MIC_GAINS - itf wrong.");
				return -EFAULT;
			}

			if (down_interruptible(&calibdata_ops_mutex))
				return -EFAULT;

			in_gains = &calibdata->calibp[param.itf].audioGains.inGains;
			param.ana = in_gains->ana;
			param.adc = in_gains->adc;
			param.alg = in_gains->alg;

#ifdef DEBUG
			pr_info("rda calibdata: GET_MIC_GAINS - ana - %d, adc - %d, alg - %d.\n",
				param.ana, param.adc, param.alg);
#endif
			up(&calibdata_ops_mutex);

			if (copy_to_user((void *)arg, &param, sizeof(param)))
				return -EFAULT;
			break;
		}
	case CALIBDATA_IOCTL_GET_IIR_PARAM:
		{
			struct get_iir_param_param param;
			printk("rda calibdata: CALIBDATA_IOCTL_GET_IIR_PARAM\n");
			if(copy_from_user(&param, (void*)arg, sizeof(param))) {
				return -EFAULT;
			}

			if(!CHECK_ITF(param.itf)) {
				pr_err("rda calibdata: GET_IIR_PARAM - itf wrong.");
				return -EFAULT;
			}

			if (down_interruptible(&calibdata_ops_mutex))
				return -EFAULT;

			memcpy(&param.gain, (*(calibdata->iirParam))[param.itf].gain, sizeof(param.gain));
			memcpy(&param.qual, (*(calibdata->iirParam))[param.itf].qual, sizeof(param.qual));
			memcpy(&param.freq, (*(calibdata->iirParam))[param.itf].freq, sizeof(param.freq));

			up(&calibdata_ops_mutex);
#ifdef DEBUG
			pr_info("rda calibdata: GET_IIR_PARAM -freq: %d %d %d %d %d %d %d %d %d %d\n",
					param.freq[0], param.freq[1], param.freq[2], param.freq[3], param.freq[4],
					param.freq[5], param.freq[6], param.freq[7], param.freq[8], param.freq[9]);
			pr_info("rda calibdata: GET_IIR_PARAM -gain: %d %d %d %d %d %d %d %d %d %d\n",
					param.gain[0], param.gain[1], param.gain[2], param.gain[3], param.gain[4],
					param.gain[5], param.gain[6], param.gain[7], param.gain[8], param.gain[9]);
			pr_info("rda calibdata: GET_IIR_PARAM -qual: %d %d %d %d %d %d %d %d %d %d\n",
					param.qual[0], param.qual[1], param.qual[2], param.qual[3], param.qual[4],
					param.qual[5], param.qual[6], param.qual[7], param.qual[8], param.qual[9]);
#endif

			if (copy_to_user((void*)arg, &param, sizeof(param)))
				return -EFAULT;
			break;
		}
	//Add for Audio Calibration -----start-----
	case CALIBDATA_IOCTL_GET_EQ_PARAM:
		{
			struct eq_data eq;
			int ret;

			if (copy_from_user(&eq, (void *)arg, sizeof(eq))) {
				return -EFAULT;
			}
			printk(KERN_INFO"CALIBDATA_IOCTL_GET_EQ_PARAM itf = %d\n", eq.itf);

			if (down_interruptible(&calibdata_ops_mutex))
				return -EFAULT;

			ret = get_calib_data(SYS_AUDIO_CMD_AUD_GET_AUD_EQ_CALIB, eq.itf,
					&eq.eq_param, calibdata);
			if (ret < 0) {
				up(&calibdata_ops_mutex);
				printk(KERN_ERR"failed to get EQ calib data");
				return -EFAULT;
			}

			up(&calibdata_ops_mutex);

#ifdef DEBUG
			pr_info("rda eq calibdata: flag - %d\n", eq.eq_param.eq_en);
			pr_info("rda eq calibdata: band0 - 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					eq.eq_param.band[0].num[0], eq.eq_param.band[0].num[1],
					eq.eq_param.band[0].num[2], eq.eq_param.band[0].den[0],
					eq.eq_param.band[0].den[1], eq.eq_param.band[0].den[2]);
			pr_info("rda eq calibdata: band6 - 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					eq.eq_param.band[6].num[0], eq.eq_param.band[6].num[1],
					eq.eq_param.band[6].num[2], eq.eq_param.band[6].den[0],
					eq.eq_param.band[6].den[1], eq.eq_param.band[6].den[2]);
#endif
			if (copy_to_user((void*)arg, &eq, sizeof(eq)))
				return -EFAULT;

			break;
		}
	case CALIBDATA_IOCTL_GET_DRC_PARAM:
		{
			struct drc_data drc;
			int ret;

			if (copy_from_user(&drc, (void *)arg, sizeof(drc))) {
				return -EFAULT;
			}
			printk(KERN_INFO"CALIBDATA_IOCTL_GET_DRC_PARAM itf = %d\n", drc.itf);

			if (down_interruptible(&calibdata_ops_mutex))
				return -EFAULT;

			ret = get_calib_data(SYS_AUDIO_CMD_AUD_GET_AUD_DRC_CALIB, drc.itf,
					&drc.drc_calib_param, calibdata);
			if (ret < 0) {
				up(&calibdata_ops_mutex);
				printk(KERN_ERR"failed to get DRC calib data");
				return -EFAULT;
			}

			up(&calibdata_ops_mutex);
			if (copy_to_user((void *)arg, &drc, sizeof(drc))) {
				printk(KERN_INFO"DRC PARAM ERR\n");
				return -EFAULT;
			}
			printk(KERN_INFO"rda calibration: CALIBDATA_IOCTL_GET_DRC_PARAM\n");

			break;
		}
	case CALIBDATA_IOCTL_GET_INGAINS:
		{
			struct get_mic_gains_param ingains;
			CALIB_AUDIO_IN_GAINS_T in_param;
			int ret;
			if (copy_from_user(&ingains, (void *)arg, sizeof(ingains))) {
				return -EFAULT;
			}
			printk(KERN_INFO"CALIBDATA_IOCTL_GET_INGAINS itf = %d\n", ingains.itf);

			if (down_interruptible(&calibdata_ops_mutex))
				return -EFAULT;

			ret = get_calib_data(SYS_AUDIO_CMD_AUD_GET_INGAINS_RECORD_CALIB, ingains.itf,
					&in_param, calibdata);
			up(&calibdata_ops_mutex);
			if (ret < 0) {
				printk(KERN_ERR"failed to get ingains calib data");
				return -EFAULT;
			}
			if (copy_to_user((void *)arg, &ingains, sizeof(ingains))) {
				printk(KERN_INFO"IN GAINS PARAM ERR\n");
				return -EFAULT;
			}
			printk(KERN_INFO"rda calibration: CALIBDATA_IOCTL_GET_INGAINS,%d,%d,%d\n",
					in_param.ana, in_param.adc, in_param.alg);
			break;
		}
	//Add for Audio Calibration -----end -----
	default:
			break;
	}

	return ret;
}

static int calibdata_ops_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int calibdata_ops_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int calibdata_setup_cdev(struct calibdata *calibdata)
{
	int err;

	if (calibdata_dev_major) {
		calibdata->cdev_t =
		    MKDEV(calibdata_dev_major, calibdata_dev_minor);
		err = register_chrdev_region(calibdata->cdev_t, 1,
					     CALIBDATA_DEV_NAME);
		if (err) {
			printk(KERN_ERR
			       "rda calibdata: register chrdev region fail.\n");
			return -1;
		}
	} else {
		err = alloc_chrdev_region(&calibdata->cdev_t, 0, 1,
					  CALIBDATA_DEV_NAME);
		if (err) {
			printk(KERN_ERR
			       "rda calibdata: alloc chrdev region fail.\n");
			return -1;
		}
	}

	cdev_init(&calibdata->cdev, &calibdata_ops);

	calibdata->cdev.owner = THIS_MODULE;
	calibdata->cdev.ops = &calibdata_ops;

	err = cdev_add(&calibdata->cdev, calibdata->cdev_t, 1);
	if (err) {
		printk(KERN_ERR "rda calibdata: add cdev fail.\n");
		return -1;
	}

	return 0;
}

static int audio_calibdata_init(void)
{
	u32 *p = NULL;
	int ret = 0, i = 0;
	u32 md_buf[2] = { 0 };
	struct device *dev = NULL;
	struct client_cmd calibdata_cmd;
	struct calibdata *calibdata = NULL;

	/* 1. alloc struct calibdata */
	calibdata = kzalloc(sizeof(struct calibdata), GFP_KERNEL);
	if (!calibdata) {
		printk(KERN_ERR "rda calibdata: alloc calibdata struct fail\n");
		ret = -ENOMEM;
		goto err_alloc_calibdata;
	}

	/* 2. ap <---> modem msys */
	calibdata->msys_dev = rda_msys_alloc_device();
	if (!calibdata->msys_dev) {
		printk(KERN_ERR "rda calibdata: alloc msys device fail\n");
		ret = -ENOMEM;
		goto err_alloc_msys;
	}
	calibdata->msys_dev->module = SYS_AUDIO_MOD;
	calibdata->msys_dev->name = "calibdata";
	calibdata->msys_dev->notifier.notifier_call = NULL;
	calibdata->msys_dev->private = NULL;
	rda_msys_register_device(calibdata->msys_dev);

	// get calib buffer
	memset(&calibdata_cmd, 0, sizeof(calibdata_cmd));
	calibdata_cmd.pmsys_dev = calibdata->msys_dev;
	calibdata_cmd.mod_id = SYS_AUDIO_MOD;
	calibdata_cmd.mesg_id = SYS_AUDIO_CMD_AUD_GET_AUD_CALIB;
	calibdata_cmd.pout_data = (void *)&md_buf;
	calibdata_cmd.out_size = sizeof(md_buf);
	ret = rda_msys_send_cmd(&calibdata_cmd);
	if (ret > 0) {
		printk(KERN_ERR
		       "rda calibdata: send get audio calib cmd fail.\n");
		ret = -EIO;
		goto err_send_msys_cmd_fail;
	}
	/* we got md buf, we need to convert and iomap */
	calibdata->buf_size = md_buf[1];
	calibdata->buf_addr = MD_ADDRESS_TO_AP(md_buf[0]);
	printk(KERN_INFO
	       "rda calibdata: get audio calib addr 0x%x, size 0x%x\n",
	       calibdata->buf_addr, calibdata->buf_size);

	if (calibdata->buf_addr <= 0) {
		printk(KERN_ERR "rda calibdata: wrong address of buffer\n");
		ret = -EIO;
		goto err_addr_wrong;
	}

	calibdata->calibp = kzalloc(calibdata->buf_size, GFP_KERNEL);
	if (calibdata->calibp <= 0) {
		printk(KERN_ERR "rda calibdata: alloc buffer error\n");
		ret = -ENOMEM;
		goto err_alloc_buffer;
	}
	/* remap */
	p = ioremap(calibdata->buf_addr, calibdata->buf_size);
	if (p <= 0) {
		printk(KERN_ERR "rda calibdata: ioremap error\n");
		ret = -EIO;
		goto err_remap;
	}
	memcpy(calibdata->calibp, p, calibdata->buf_size);
#ifdef DEBUG
	printk(KERN_INFO "audioGains.inGains.ana-%d\n",
	       calibdata->calibp[1].audioGains.inGains.ana);
	printk(KERN_INFO "audioGains.inGains.adc-%d\n",
	       calibdata->calibp[1].audioGains.inGains.adc);
	printk(KERN_INFO "audioGains.inGains.alg-%d\n",
	       calibdata->calibp[1].audioGains.inGains.alg);
#endif
	iounmap(p);

	// get calib iir buffer
	memset(&calibdata_cmd, 0, sizeof(calibdata_cmd));
	calibdata_cmd.pmsys_dev = calibdata->msys_dev;
	calibdata_cmd.mod_id = SYS_AUDIO_MOD;
	calibdata_cmd.mesg_id = SYS_AUDIO_CMD_AUD_GET_AUD_IIR_CALIB;
	calibdata_cmd.pout_data = (void *)&md_buf;
	calibdata_cmd.out_size = sizeof(md_buf);
	ret = rda_msys_send_cmd(&calibdata_cmd);
	if(ret > 0) {
		printk(KERN_ERR "rda calibdata: send get audio iir calib cmd fail.\n");
		ret = -EIO;
		goto err_send_msys_cmd_fail;
	}

	// we got md buf, we need to convert and iomap
	calibdata->buf_size = md_buf[1];
	calibdata->buf_addr = MD_ADDRESS_TO_AP(md_buf[0]);
	printk(KERN_INFO"rda calibdata: get audio calib iir addr 0x%x, size 0x%x\n",
			calibdata->buf_addr, calibdata->buf_size);

	if(calibdata->buf_addr <= 0) {
		printk(KERN_ERR "rda calibdata: wrong address of buffer\n");
		ret = -EIO;
		goto err_addr_wrong;
	}

	calibdata->iirParam = kzalloc(calibdata->buf_size, GFP_KERNEL);
	if(calibdata->iirParam <= 0) {
		printk(KERN_ERR "rda calibdata: alloc buffer error\n");
		ret = -ENOMEM;
		goto err_alloc_buffer;
	}

	// remap
	p = ioremap(calibdata->buf_addr, calibdata->buf_size);
	if(p <= 0) {
		printk(KERN_ERR "rda calibdata: ioremap error\n");
		ret = -EIO;
		goto err_remap;
	}
	memcpy(calibdata->iirParam, p, calibdata->buf_size);
#ifdef DEBUG
	printk(KERN_INFO"iirParam.freq-%d\n", calibdata->iirParam[1]->freq[0]);
	printk(KERN_INFO"iirParam.gain-%d\n", calibdata->iirParam[1]->gain[0]);
	printk(KERN_INFO"iirParam.qual-%d\n", calibdata->iirParam[1]->qual[0]);
#endif
	iounmap(p);

	/* 3.create sys fs interface */
	calibdata->sys_class = create_calibdata_sysclass();
	if (calibdata->sys_class < 0) {
		printk(KERN_ERR "rda calibdata: alloc calibdata struct fail\n");
		goto err_create_sys_class_fail;
	}
	for (i = 0; i < CALIB_AUDIO_ITF_QTY; ++i) {
		dev = device_create(calibdata->sys_class, NULL,
				    MKDEV(0, i), NULL, dev_names[i]);
		if (IS_ERR(dev))
			return PTR_ERR(dev);

		ret = device_create_file(dev, &dev_attr_audioGains);
		if (ret < 0) {
			printk(KERN_ERR
			       "rda calibdata: create calib all device file fail\n");
			goto err_create_file;
		}
	}

	/* 4. setup char dev */
	if (calibdata_setup_cdev(calibdata) != 0) {
		printk(KERN_ERR "rda calibdata: create char device fail\n");
		ret = -EIO;
		goto err_create_char_dev;
	}

	dev =
	    device_create(calibdata->sys_class, NULL, calibdata->cdev_t, NULL,
			  CALIBDATA_DEV_NAME);

	if (IS_ERR(dev)) {
		printk(KERN_ERR "rda calibdata: create cdev device fail\n");
		goto err_create_cdev_device;
	}

	g_calibdata = calibdata;
	printk(KERN_INFO "rda calibdata: initialized.\n");
	return 0;

err_create_cdev_device:
err_create_char_dev:
err_create_file:
err_create_sys_class_fail:
err_remap:
	kfree(calibdata->calibp);
err_alloc_buffer:
err_addr_wrong:
err_send_msys_cmd_fail:
	rda_msys_unregister_device(calibdata->msys_dev);
	rda_msys_free_device(calibdata->msys_dev);
	calibdata->msys_dev = NULL;
err_alloc_msys:
err_alloc_calibdata:

	return ret;
}

static int __init calibdata_class_init(void)
{
	audio_calibdata_init();
	return 0;
}

static void __exit calibdata_class_exit(void)
{
	if (!g_calibdata)
		return;

	rda_msys_unregister_device(g_calibdata->msys_dev);
	rda_msys_free_device(g_calibdata->msys_dev);

	class_destroy(g_calibdata->sys_class);
}

module_init(calibdata_class_init);
module_exit(calibdata_class_exit);

MODULE_AUTHOR("yulongwang <yulongwang@android.com>");
MODULE_DESCRIPTION("rda calib driver");
MODULE_LICENSE("GPL");
