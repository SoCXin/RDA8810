/*
 * rda_wdt.c
 *
 * Watchdog driver for the RDA88xx serial chips
 *
 * Author: Chen Gang <gangchen@rdamicro.com>
 *
 * 2016 (c) RDAMicro, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Based on rda md driver - md heartbeat. System reset is done by Modem
 */

/* #define DEBUG */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#include <plat/rda_md.h>

#ifdef DEBUG
#define DBG pr_info
#else
#define DBG(...)
#endif

struct rda_wdt_dev {
	struct rda_ap_hb_hdr *base;
	struct device   *dev;
	bool		rda_wdt_users;
	struct mutex	lock;		/* to avoid races with PM */
};

static void rda_wdt_reload(struct rda_wdt_dev *wdev)
{
	struct rda_ap_hb_hdr *base = wdev->base;
	uint32_t ap_cnt;

	ap_cnt = __raw_readl(&(base->ap_cnt));
	ap_cnt++;

	if (ap_cnt < AP_COMM_HEARTBEAT_MIN_VALUE)
		ap_cnt = AP_COMM_HEARTBEAT_MIN_VALUE;

	__raw_writel(ap_cnt, &(base->ap_cnt));

}

static void rda_wdt_enable(struct rda_wdt_dev *wdev)
{
	struct rda_ap_hb_hdr *base = wdev->base;

	__raw_writel(AP_COMM_HEARTBEAT_MIN_VALUE, &(base->ap_cnt));
}

static void rda_wdt_disable(struct rda_wdt_dev *wdev)
{
	struct rda_ap_hb_hdr *base = wdev->base;

	__raw_writel(0, &(base->ap_cnt));
}



static int rda_wdt_start(struct watchdog_device *wdog)
{
	struct rda_wdt_dev *wdev = watchdog_get_drvdata(wdog);

	DBG("%s called\n", __func__);
	mutex_lock(&wdev->lock);
	wdev->rda_wdt_users = true;
	rda_wdt_enable(wdev);
	mutex_unlock(&wdev->lock);

	return 0;
}

static int rda_wdt_stop(struct watchdog_device *wdog)
{
	struct rda_wdt_dev *wdev = watchdog_get_drvdata(wdog);

	DBG("%s called\n", __func__);
	mutex_lock(&wdev->lock);
	rda_wdt_disable(wdev);
	wdev->rda_wdt_users = false;
	mutex_unlock(&wdev->lock);
	return 0;
}

static int rda_wdt_ping(struct watchdog_device *wdog)
{
	struct rda_wdt_dev *wdev = watchdog_get_drvdata(wdog);

	DBG("%s called\n", __func__);
	mutex_lock(&wdev->lock);
	if (wdev->rda_wdt_users)
		rda_wdt_reload(wdev);
	mutex_unlock(&wdev->lock);

	return 0;
}


static const struct watchdog_info rda_wdt_info = {
	.options = WDIOF_KEEPALIVEPING,
	.identity = "RDA Watchdog",
};

static const struct watchdog_ops rda_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= rda_wdt_start,
	.stop		= rda_wdt_stop,
	.ping		= rda_wdt_ping,
};

/* HACK! not right way of use sysfs */
static struct rda_wdt_dev *rda_wd_device;
static ssize_t rda_wdt_ap_cnt_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	uint32_t ap_cnt;

	if (rda_wd_device) {
		ap_cnt = __raw_readl(&(rda_wd_device->base->ap_cnt));
		return sprintf(buf, "addr %p, value 0x%x\n",
				&(rda_wd_device->base->ap_cnt), ap_cnt);
	} else {
		return sprintf(buf, "not initialized\n");
	}
}

static struct kobj_attribute rda_wdt_ap_cnt_attr =
	__ATTR(rda_wdt_ap_cnt, 0444, rda_wdt_ap_cnt_show, NULL);

static int rda_wdt_init_sysfs(void)
{
	int error = 0;
	error = sysfs_create_file(kernel_kobj, &rda_wdt_ap_cnt_attr.attr);
	if (error)
		printk(KERN_ERR "sysfs_create_file rda_wdt_ap_cnt failed: %d\n", error);
	return error;
}

static int rda_wdt_probe(struct platform_device *pdev)
{
	struct watchdog_device *rda_wdt;
	struct rda_wdt_dev *wdev;
	int ret;

	DBG("RDA Watchdog init\n");

	rda_wdt = devm_kzalloc(&pdev->dev, sizeof(*rda_wdt), GFP_KERNEL);
	if (!rda_wdt)
		return -ENOMEM;

	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	wdev->rda_wdt_users	= false;
	wdev->base		= rda_md_get_mapped_addr();
	wdev->dev		= &pdev->dev;
	mutex_init(&wdev->lock);

	rda_wdt->info	      = &rda_wdt_info;
	rda_wdt->ops	      = &rda_wdt_ops;

	watchdog_set_drvdata(rda_wdt, wdev);
	watchdog_set_nowayout(rda_wdt, false);

	platform_set_drvdata(pdev, rda_wdt);

	rda_wdt_disable(wdev);

	ret = watchdog_register_device(rda_wdt);
	if (ret) {
		return ret;
	}

	rda_wd_device = wdev;
	rda_wdt_init_sysfs();

	pr_info("RDA Watchdog init done\n");

	return 0;
}

static void rda_wdt_shutdown(struct platform_device *pdev)
{
	struct watchdog_device *wdog = platform_get_drvdata(pdev);

	DBG("%s called\n", __func__);
	rda_wdt_stop(wdog);
}

static int rda_wdt_remove(struct platform_device *pdev)
{
	struct watchdog_device *wdog = platform_get_drvdata(pdev);

	DBG("%s called\n", __func__);
	watchdog_unregister_device(wdog);

	return 0;
}

#ifdef	CONFIG_PM

/* REVISIT ... not clear this is the best way to handle system suspend; and
 * it's very inappropriate for selective device suspend (e.g. suspending this
 * through sysfs rather than by stopping the watchdog daemon).  Also, this
 * may not play well enough with NOWAYOUT...
 */

static int rda_wdt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct watchdog_device *wdog = platform_get_drvdata(pdev);
	struct rda_wdt_dev *wdev = watchdog_get_drvdata(wdog);

	DBG("%s called\n", __func__);
	mutex_lock(&wdev->lock);
	if (wdev->rda_wdt_users) {
		rda_wdt_disable(wdev);
	}
	mutex_unlock(&wdev->lock);

	return 0;
}

static int rda_wdt_resume(struct platform_device *pdev)
{
	struct watchdog_device *wdog = platform_get_drvdata(pdev);
	struct rda_wdt_dev *wdev = watchdog_get_drvdata(wdog);

	DBG("%s called\n", __func__);
	mutex_lock(&wdev->lock);
	if (wdev->rda_wdt_users) {
		rda_wdt_enable(wdev);
		rda_wdt_reload(wdev);
	}
	mutex_unlock(&wdev->lock);

	return 0;
}

#else
#define	rda_wdt_suspend	NULL
#define	rda_wdt_resume	NULL
#endif

static struct platform_driver rda_wdt_driver = {
	.probe		= rda_wdt_probe,
	.remove		= rda_wdt_remove,
	.shutdown	= rda_wdt_shutdown,
	.suspend	= rda_wdt_suspend,
	.resume		= rda_wdt_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "rda_wdt",
	},
};

module_platform_driver(rda_wdt_driver);

MODULE_AUTHOR("Chen Gang");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rda_wdt");
