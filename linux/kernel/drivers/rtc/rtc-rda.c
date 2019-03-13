/*
 * rtc-rda.c - A rtc driver for controlling rtc of RDA
 *
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
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <asm/io.h>
#include <asm/ioctls.h>

#include <plat/devices.h>
#include <plat/reg_rtc.h>
#include <plat/md_sys.h>
#include <plat/boot_mode.h>


struct rda_rtc {
	void __iomem *rtc_base;
	struct rtc_device *rtc_dev;

	struct msys_device *rtc_msys;
};

#define RTC_ENABLE_MASK		(RDA_RTC_CMD_ALARM_LOAD | RDA_RTC_CMD_ALARM_ENABLE | RDA_RTC_CMD_ALARM_DISABLE)
#define RTC_DISABLE_MASK		(RDA_RTC_CMD_ALARM_ENABLE | RDA_RTC_CMD_ALARM_DISABLE)


#define __rda_rtc_readl(rtc, field) \
	__raw_readl((rtc)->rtc_base + RDA_RTC_ ## field)
#define __rda_rtc_writel(rtc, field, val) \
	__raw_writel((val), (rtc)->rtc_base + RDA_RTC_ ## field)

static int rda_rtc_readtime(struct device *dev, struct rtc_time *tm);
static int rda_rtc_settime(struct device *dev, struct rtc_time *tm);
static int rda_rtc_readalarm(struct device *dev, struct rtc_wkalrm *alrm);
static int rda_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled);
static int rda_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm);
static int rda_rtc_proc(struct device *dev, struct seq_file *seq);
static int rda_rtc_notify(struct notifier_block *nb, unsigned long mesg, void *data);


/*
 * Read current time and date in RTC
 */
static int rda_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
#ifndef CONFIG_RDA_FPGA
	struct rda_rtc *rtc = dev_get_drvdata(dev);
	u32 status;
	u32 high;
	u32 low;

	status = __rda_rtc_readl(rtc, STA_REG);
	if (status & RDA_RTC_STA_NOT_PROG) {
		tm->tm_sec = 0;
		tm->tm_min = 0;
		tm->tm_hour = 0;
		tm->tm_mday = 1;
		tm->tm_mon = 0;
		/* From 2000/01/01 */
		tm->tm_year = 100;
		tm->tm_wday = 6;

		/*
		 * Force to set a default time to RTC,
		 * otherwise we can't read any time from RTC.
		 */
		rda_rtc_settime(dev, tm);

		return 0;
	}

	/* Read registers */
	high = __rda_rtc_readl(rtc, CUR_LOAD_HIGH_REG);
	low = __rda_rtc_readl(rtc, CUR_LOAD_LOW_REG);

	tm->tm_sec = GET_SEC(low);
	tm->tm_min = GET_MIN(low);
	tm->tm_hour = GET_HOUR(low);
	tm->tm_mday = GET_MDAY(high);
	tm->tm_mon = GET_MON(high);
	tm->tm_year = GET_YEAR(high);
	tm->tm_wday = GET_WDAY(high);

	dev_dbg(dev, "%s: %4d-%02d-%02d %02d:%02d:%02d\n", "readtime",
		2000 + tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	/*
	 * The number of years since 1900 in kernel,
	 * but it is defined since 2000 by HW.
	 */
	tm->tm_year += 100;
	/*
	 * The number of mons' range is from 0 to 11 in kernel,
	 * but it is defined from 1 to 12 by HW.
	 */
	tm->tm_mon -= 1;

	return 0;

#else
	return -EINVAL;
#endif /* CONFIG_RDA_FPGA */
}

/*
 * Set current time and date in RTC
 */
static int rda_rtc_settime(struct device *dev, struct rtc_time *tm)
{
#ifndef CONFIG_RDA_FPGA
	struct rda_rtc *rtc = dev_get_drvdata(dev);
	int err;
	u32 cmd;
	u32 high;
	u32 low;

	/* HW doesn't support the years that is less than 2000. */
	if (tm->tm_year + 1900 < 2000) {
		return -EINVAL;
	}

	err = rtc_valid_tm(tm);
	if (err < 0) {
		return err;
	}

	/*
	 * The number of years since 1900 in kernel,
	 * but it is defined since 2000 by HW.
	 * The number of mons' range is from 0 to 11 in kernel,
	 * but it is defined from 1 to 12 by HW.
	 */

	dev_dbg(dev, "%s: %4d-%02d-%02d %02d:%02d:%02d\n", "settime",
		2000 + (tm->tm_year - 100), tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	cmd = __rda_rtc_readl(rtc, CMD_REG);
	if (cmd & RDA_RTC_CMD_CAL_LOAD) {
		return -EBUSY;
	}

	low = (SET_SEC(tm->tm_sec) | SET_MIN(tm->tm_min) | SET_HOUR(tm->tm_hour));
	high = (SET_MDAY(tm->tm_mday) | SET_MON((tm->tm_mon + 1)) |
		SET_YEAR((tm->tm_year - 100)) |SET_WDAY(tm->tm_wday));

	__rda_rtc_writel(rtc, CAL_LOAD_LOW_REG, low);
	__rda_rtc_writel(rtc, CAL_LOAD_HIGH_REG, high);
	__rda_rtc_writel(rtc, CMD_REG, RDA_RTC_CMD_CAL_LOAD);

	/* Check if the cmd has been finished. */
	while ((__rda_rtc_readl(rtc, CMD_REG)) & RDA_RTC_CMD_CAL_LOAD) {
		/* Nothing to duo */
	};

	return 0;

#else
	return -EINVAL;
#endif /* CONFIG_RDA_FPGA */
}

static int rda_rtc_readalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
#ifndef CONFIG_RDA_FPGA
	struct rda_rtc *rtc = dev_get_drvdata(dev);
	struct rtc_time *tm = &alrm->time;
	u32 high;
	u32 low;

	/* Read registers */
	high = __rda_rtc_readl(rtc, ALARM_HIGH_REG);
	low = __rda_rtc_readl(rtc, ALARM_LOW_REG);

	tm->tm_sec = GET_SEC(low);
	tm->tm_min = GET_MIN(low);
	tm->tm_hour = GET_HOUR(low);
	tm->tm_mday = GET_MDAY(high);
	tm->tm_mon = GET_MON(high);
	tm->tm_year = GET_YEAR(high);
	tm->tm_wday = GET_WDAY(high);

	dev_dbg(dev, "%s: %4d-%02d-%02d %02d:%02d:%02d\n", "readalarm",
		2000 + tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	/*
	 * The number of years since 1900 in kernel,
	 * but it is defined since 2000 by HW.
	 */
	tm->tm_year += 100;
	/*
	 * The number of mons' range is from 0 to 11 in kernel,
	 * but it is defined from 1 to 12 by HW.
	 */
	tm->tm_mon -= 1;

	return 0;

#else
	return -EINVAL;
#endif /* CONFIG_RDA_FPGA */
}

static int rda_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct rda_rtc *rtc = dev_get_drvdata(dev);
	volatile u32 cmd;

	if (enabled) {
		/* Enable alarm interrutp */
		while ((cmd = __rda_rtc_readl(rtc, CMD_REG)) & RTC_ENABLE_MASK) {
		}

		__rda_rtc_writel(rtc, CMD_REG, (cmd | RDA_RTC_CMD_ALARM_ENABLE));
	} else {
		/* Disable alarm interrutp */
		while ((cmd = __rda_rtc_readl(rtc, CMD_REG)) & RTC_DISABLE_MASK) {
		}

		__rda_rtc_writel(rtc, CMD_REG, (cmd | RDA_RTC_CMD_ALARM_DISABLE));
	}

	return 0;
}

static int rda_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
#ifndef CONFIG_RDA_FPGA
	struct rda_rtc *rtc = dev_get_drvdata(dev);
	struct rtc_time *tm = &alrm->time;
	int err;
	volatile u32 cmd;
	u32 high;
	u32 low;

	/* HW doesn't support the years that is less than 2000. */
	if (tm->tm_year + 1900 < 2000) {
		return -EINVAL;
	}

	err = rtc_valid_tm(tm);
	if (err < 0) {
		return err;
	}

	/* Disable alarm interrutp firstly. */
	rda_rtc_alarm_irq_enable(dev, 0);

	/*
	 * The number of years since 1900 in kernel,
	 * but it is defined since 2000 by HW.
	 * The number of mons' range is from 0 to 11 in kernel,
	 * but it is defined from 1 to 12 by HW.
	 */

	cmd = __rda_rtc_readl(rtc, CMD_REG);
	if (cmd & RDA_RTC_CMD_ALARM_LOAD) {
		return -EBUSY;
	}

	low = (SET_SEC(tm->tm_sec) | SET_MIN(tm->tm_min) | SET_HOUR(tm->tm_hour));
	high = (SET_MDAY(tm->tm_mday) | SET_MON((tm->tm_mon + 1)) | SET_YEAR((tm->tm_year - 100)));

	__rda_rtc_writel(rtc, ALARM_LOW_REG, low);
	__rda_rtc_writel(rtc, ALARM_HIGH_REG, high);
	__rda_rtc_writel(rtc, CMD_REG, RDA_RTC_CMD_ALARM_LOAD);

	if (alrm->enabled) {
		rda_rtc_alarm_irq_enable(dev, 1);
	}

	dev_dbg(dev, "%s: %4d-%02d-%02d %02d:%02d:%02d\n", "setalarm",
		2000 + (tm->tm_year - 100), tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;

#else
	return -EINVAL;
#endif /* CONFIG_RDA_FPGA */
}

/*
 * Provide additional RTC information in /proc/driver/rtc
 */
static int rda_rtc_proc(struct device *dev, struct seq_file *seq)
{
	struct rda_rtc *rtc = dev_get_drvdata(dev);
	u32 status = __rda_rtc_readl(rtc, STA_REG);

	seq_printf(seq, "alarm enable\t: %s\n", (status & RDA_RTC_STA_ALARM_ENABLE) ? "yes" : "no");

	return 0;
}

/*
 * IRQ handler for the RTC
 */
static int rda_rtc_notify(struct notifier_block *nb, unsigned long mesg, void *data)
{
	struct msys_device *pmsys_dev = container_of(nb, struct msys_device, notifier);
	struct rda_rtc *prtc = (struct rda_rtc *)pmsys_dev->private;
	struct rtc_device *prtc_dev = prtc->rtc_dev;
	struct client_mesg *pmesg = (struct client_mesg *)data;
	unsigned long events = 0;

	if (pmesg->mod_id != SYS_GEN_MOD) {
		return NOTIFY_DONE;
	}

	if (mesg != SYS_GEN_MESG_RTC_TRIGGER) {
		return NOTIFY_DONE;
	}

	dev_dbg(prtc_dev->dev.parent, "%s : mesg = 0x%lx\n", __func__, mesg);

	if (rda_get_boot_mode() == BM_CHARGER) {
		machine_restart(NULL);
	}

	events |= (RTC_AF | RTC_IRQF);
	rtc_update_irq(prtc_dev, 1, events);

	return NOTIFY_OK;
}

static const struct rtc_class_ops rda_rtc_ops = {
	.read_time = rda_rtc_readtime,
	.set_time = rda_rtc_settime,
	.read_alarm = rda_rtc_readalarm,
	.set_alarm = rda_rtc_setalarm,
	.proc = rda_rtc_proc,
	.alarm_irq_enable = rda_rtc_alarm_irq_enable,
};

/*
 * Initialize and install RTC driver
 */
static int rda_rtc_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct rda_rtc *rtc;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res ) {
		dev_err(&pdev->dev, "need  ressources\n");
		return -ENODEV;
	}

	rtc = kzalloc(sizeof *rtc, GFP_KERNEL);
	if (!rtc) {
		return -ENOMEM;
	}

	/* platform setup code should have handled this; sigh */
	if (!device_can_wakeup(&pdev->dev)) {
		device_init_wakeup(&pdev->dev, 1);
	}

	rtc->rtc_msys = rda_msys_alloc_device();
	if (!rtc->rtc_msys) {
		ret = -ENOMEM;
		goto err_handle_msys;
	}

	rtc->rtc_msys->module = SYS_GEN_MOD;
	rtc->rtc_msys->name = RDA_RTC_DRV_NAME;
	/* Init callback of notify. */
	rtc->rtc_msys->notifier.notifier_call = rda_rtc_notify;
	rtc->rtc_msys->private = (void *)rtc;

	rda_msys_register_device(rtc->rtc_msys);

	platform_set_drvdata(pdev, rtc);
	rtc->rtc_base = ioremap(res->start, resource_size(res));
	if (!rtc->rtc_base) {
		dev_err(&pdev->dev, "failed to map registers, aborting.\n");
		ret = -ENOMEM;
		goto err_handle_ioremap;
	}

	rtc->rtc_dev = rtc_device_register(pdev->name, &pdev->dev, &rda_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc_dev)) {
		ret = PTR_ERR(rtc->rtc_dev);
		goto err_handle_register;
	}

	return 0;

err_handle_register:

	iounmap(rtc->rtc_base);

err_handle_ioremap:

	rda_msys_unregister_device(rtc->rtc_msys);
	rda_msys_free_device(rtc->rtc_msys);

	platform_set_drvdata(pdev, NULL);

err_handle_msys:

	if (rtc) {
		kfree(rtc);
	}

	return ret;
}

/*
 * Disable and remove the RTC driver
 */
static int rda_rtc_remove(struct platform_device *pdev)
{
	struct rda_rtc *rtc = platform_get_drvdata(pdev);

	rda_msys_unregister_device(rtc->rtc_msys);
	rda_msys_free_device(rtc->rtc_msys);

	rtc_device_unregister(rtc->rtc_dev);

	if (rtc->rtc_base) {
		iounmap(rtc->rtc_base);
	}

	platform_set_drvdata(pdev, NULL);

	if (rtc) {
		kfree(rtc);
	}

	return 0;
}

static void rda_rtc_shutdown(struct platform_device *pdev)
{
	return;
}

#ifdef CONFIG_PM

/* RTC Power management control */

static int rda_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int rda_rtc_resume(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	if (rda_get_boot_mode() == BM_CHARGER) {
		/*
		 * In charger mode, alarmtimer.c can not reset rtc,
		 * so we do not disable alarm in this case.
		 */
		return 0;
	}

	/* If alarms were left, we turn them off. */
	rda_rtc_alarm_irq_enable(dev, 0);

	return 0;
}
#else
#define rda_rtc_suspend	NULL
#define rda_rtc_resume		NULL
#endif

static struct platform_driver rda_rtc_driver = {
	.probe = rda_rtc_probe,
	.remove = rda_rtc_remove,
	.shutdown = rda_rtc_shutdown,
	.suspend = rda_rtc_suspend,
	.resume = rda_rtc_resume,
	.driver = {
		.name = RDA_RTC_DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init rda_rtc_init(void)
{
	return platform_driver_register(&rda_rtc_driver);
}
module_init(rda_rtc_init);

static void __exit rda_rtc_exit(void)
{
	platform_driver_unregister(&rda_rtc_driver);
}
module_exit(rda_rtc_exit);


MODULE_AUTHOR("Tao Lei");
MODULE_DESCRIPTION("RTC driver for RDA");
MODULE_LICENSE("GPL");

