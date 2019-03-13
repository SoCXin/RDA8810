/*
 * vibrator.c - A vibrator driver of RDA
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
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/timer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>

#include <plat/devices.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>

#include <rda/tgt_ap_board_config.h>

struct rda_vibrator_info {
	struct platform_device *pdev;
	struct timed_output_dev vib_dev;

	struct regulator *vib_reg;

	struct work_struct vib_work;
	struct timer_list vib_timer;

	spinlock_t lock;
	int vib_state;
	unsigned long start_jiffies;
	unsigned long end_jiffies;
};

static void rda_set_vibrator(struct rda_vibrator_info *pvib)
{
#ifndef CONFIG_RDA_FPGA
	int ret = 0;

	if (pvib->vib_state) {
		ret = regulator_set_voltage(pvib->vib_reg, 2800000, 2800000);
	} else {
		ret = regulator_set_voltage(pvib->vib_reg, 1800000, 2800000);
	}

	if (ret < 0) {
		dev_err(&pvib->pdev->dev, "failed as setting : ret = %d\n", ret);
	}
#endif
	return;
}

static int rda_vibrator_pm_init(struct platform_device *pdev, struct rda_vibrator_info *pinfo)
{
#ifndef CONFIG_RDA_FPGA
	int ret;

	if (!pinfo) {
		return -EINVAL;
	}

	pinfo->vib_reg = regulator_get(NULL, LDO_VIBRATOR);
	if (IS_ERR(pinfo->vib_reg)) {
		dev_err(&pdev->dev, "could not find regulator devices\n");
		ret = PTR_ERR(pinfo->vib_reg);
		return ret;
	}

	return regulator_set_voltage(pinfo->vib_reg, 1800000, 2800000);
#else
	return 0;
#endif
}

static void rda_update_vibrator(struct work_struct *work)
{
	struct rda_vibrator_info *pvib = container_of(work, struct rda_vibrator_info, vib_work);

	rda_set_vibrator(pvib);
}

static void rda_vibrator_timer_func(unsigned long data)
{
	struct rda_vibrator_info *pvib = (struct rda_vibrator_info *)data;
	unsigned long flags;

	spin_lock_irqsave(&pvib->lock, flags);
	pvib->vib_state = 0;
	spin_unlock_irqrestore(&pvib->lock, flags);

	schedule_work(&pvib->vib_work);

	return;
}

static void rda_vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct rda_vibrator_info *pvib = container_of(dev, struct rda_vibrator_info, vib_dev);
	unsigned long flags;

	if (value == 0) {
		spin_lock_irqsave(&pvib->lock, flags);
		pvib->vib_state = 0;
		spin_unlock_irqrestore(&pvib->lock, flags);

		del_timer_sync(&pvib->vib_timer);
	} else {
		spin_lock_irqsave(&pvib->lock, flags);
		if (pvib->vib_state == 1) {
			spin_unlock_irqrestore(&pvib->lock, flags);
			return;
		}

		pvib->vib_state = 1;
		spin_unlock_irqrestore(&pvib->lock, flags);

		value = (value > 15000) ? 15000 : value;

		init_timer(&pvib->vib_timer);
		pvib->vib_timer.function = rda_vibrator_timer_func;
		pvib->vib_timer.data = (unsigned long)pvib;
		pvib->vib_timer.expires = jiffies + msecs_to_jiffies(value);

		pvib->start_jiffies = jiffies;
		pvib->end_jiffies = pvib->vib_timer.expires;

		add_timer(&pvib->vib_timer);
	}

	schedule_work(&pvib->vib_work);
}

static int rda_vibrator_get_time(struct timed_output_dev *dev)
{
	struct rda_vibrator_info *pvib = container_of(dev, struct rda_vibrator_info, vib_dev);
	unsigned long cur_jiffies = jiffies;

	if (pvib->vib_state == 1) {
		if (time_in_range(cur_jiffies, pvib->start_jiffies, pvib->end_jiffies)) {
			return jiffies_to_msecs(pvib->end_jiffies - cur_jiffies);
		}
	}

	return 0;
}

static int rda_vibrator_probe(struct platform_device *pdev)
{
	struct rda_vibrator_info *pvib;
	int ret = 0;

	pvib = kzalloc(sizeof(struct rda_vibrator_info), GFP_KERNEL);
	if (!pvib) {
		return -ENOMEM;
	}

	pvib->pdev = pdev;

	pvib->vib_dev.name = "vibrator";
	pvib->vib_dev.get_time = rda_vibrator_get_time;
	pvib->vib_dev.enable = rda_vibrator_enable;

	ret = rda_vibrator_pm_init(pdev, pvib);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed as init : %d\n", ret);
		kfree(pvib);
		return ret;
	}

	INIT_WORK(&pvib->vib_work, rda_update_vibrator);
	spin_lock_init(&pvib->lock);

	pvib->vib_state = 0;

	ret = timed_output_dev_register(&pvib->vib_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "could register with timed-output system\n");
		regulator_put(pvib->vib_reg);
		kfree(pvib);
		return ret;
	}

	platform_set_drvdata(pdev, pvib);

#ifdef _TGT_AP_VIBRATOR_POWER_ON
	rda_vibrator_enable(&pvib->vib_dev, _TGT_AP_VIBRATOR_TIME);
#endif /* _TGT_AP_VIBRATOR_POWER_ON */

	return 0;
}

static int __exit rda_vibrator_remove(struct platform_device *pdev)
{
	struct rda_vibrator_info *pvib = platform_get_drvdata(pdev);

	if (!pvib) {
		return -EINVAL;
	}

	timed_output_dev_unregister(&pvib->vib_dev);

	if (!IS_ERR(pvib->vib_reg)) {
		regulator_put(pvib->vib_reg);
	}

	platform_set_drvdata(pdev, NULL);

	kfree(pvib);

	return 0;
}

#ifdef CONFIG_PM
static int rda_vibrator_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct rda_vibrator_info *pvib = platform_get_drvdata(pdev);

	if (pvib->vib_state == 1) {
		return -EBUSY;
	}

	return 0;
}

static int rda_vibrator_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define rda_vibrator_suspend NULL
#define rda_vibrator_resume NULL
#endif /* CONFIG_PM */

static struct platform_driver rda_vibrator_driver = {
	.driver = {
		.name = RDA_VIBRATOR_DRV_NAME,
	},
	.remove = __exit_p(rda_vibrator_remove),
#ifdef CONFIG_PM
	.suspend = rda_vibrator_suspend,
	.resume = rda_vibrator_resume,
#endif
};

static int __init rda_vibrator_init(void)
{
	return platform_driver_probe(&rda_vibrator_driver, rda_vibrator_probe);
}

module_init(rda_vibrator_init);

MODULE_AUTHOR("Tao Lei <leitao@rdamicro.com>");
MODULE_DESCRIPTION("vibrator driver for RDA");
MODULE_LICENSE("GPL");
