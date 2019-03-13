/*
 * rda_pwm.c - RDA PWM driver for controlling backlight.
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/clk.h>

#include <plat/devices.h>
#include <plat/reg_pwm.h>
#include <mach/iomap.h>
#include <mach/rda_clk_name.h>
#include <plat/boot_mode.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#define RDA_FB_TIMEOUT	10000
#define RDA_FB_CHARGER_TIMEOUT	50


extern int rda_fb_register_client(struct notifier_block * nb);
extern int rda_fb_unregister_client(struct notifier_block *nb);

struct rda_pwm {
	struct backlight_device *bldev;
	struct platform_device *pdev;

	void __iomem *base;
	struct clk	*master_clk;
	unsigned long pwm_clk;
	struct rda_bl_pwm_device_data *pdata;

	int gpio_bl_on;
	int gpio_bl_on_valid;
	int pwm_invert;
	int pwt_used;
	int fb_count;
	struct mutex pending_lock;

	unsigned char current_level;
	unsigned char pending_level;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_ops;
#endif /* CONFIG_HAS_EARLYSUSPEND */

	struct notifier_block fb_notifier;

	struct completion fb_done;
	struct work_struct bl_work;
};

static void hal_pwm_sel_level(struct rda_pwm *rda_pwm, unsigned char level)
{
	int val=0;
	HWP_PWM_T*	hwp_apPwm =(HWP_PWM_T*)rda_pwm->base;

	if (level == 0) {
		val |= PWM_PWL1_FORCE_L;
	} else if (level == 0xff) {
		val |= PWM_PWL1_FORCE_H;
	} else {
		val |= (PWM_PWL1_EN_H | PWM_PWL1_THRESHOLD(level));
	}
	val |= PWM_PWL1_CLR_OE;
	hwp_apPwm->PWL1_Config = val;
}

static void hal_pwt_sel_level(struct rda_pwm *rda_pwm, unsigned char level)
{
	HWP_PWM_T*	hwp_apPwm =(HWP_PWM_T*)rda_pwm->base;

	if (level == 0) {
		hwp_apPwm->PWT_Config = ~PWM_PWT_ENABLE;
	} else {
		hwp_apPwm->PWT_Config =(PWM_PWT_PERIOD(256) | PWM_PWT_DUTY(level) | PWM_PWT_ENABLE);
	}
}

static void hal_pwm_output_enable(struct rda_pwm *rda_pwm, int enable)
{
	HWP_PWM_T *hwp_apPwm =(HWP_PWM_T*)rda_pwm->base;
	if (enable) {
		hwp_apPwm->PWL1_Config = PWM_PWL1_CLR_OE;
	} else {
		hwp_apPwm->PWL1_Config = PWM_PWL1_SET_OE;
	}
}

static void hal_pwm_init(struct rda_pwm *rda_pwm)
{
	HWP_PWM_T*	hwp_apPwm =(HWP_PWM_T*)rda_pwm->base;
	unsigned long mclk = clk_get_rate(rda_pwm->master_clk);
	unsigned long pwm_clk = rda_pwm->pwm_clk;
	unsigned long clk_div;

	/*pwm ref clock*/
	clk_div = mclk/pwm_clk-1;
	if (clk_div > 255) {
		clk_div=255;
	}

	hwp_apPwm->Cfg_Clk_PWM = PWM_PWM_DIVIDER(clk_div);
}

static void rda_bl_set_enable(struct rda_pwm *rda_pwm, int enable)
{
	int bl_on = rda_pwm->gpio_bl_on;
	int valid =  rda_pwm->gpio_bl_on_valid;

	if (enable) {
		if (valid) {
			gpio_direction_output(bl_on,1);
		}
	} else {
		if (valid) {
			gpio_direction_output(bl_on,0);
		}
	}
}

static int rda_bl_notifier_cb(struct notifier_block *self, unsigned long event, void *data)
{
	struct rda_pwm *rda_pwm = container_of(self, struct rda_pwm, fb_notifier);

	if (event) {
		mutex_lock(&rda_pwm->pending_lock);

		if (rda_pwm->fb_count < 0) {
			mutex_unlock(&rda_pwm->pending_lock);
			return 0;
		}

		++rda_pwm->fb_count;
		if (rda_pwm->fb_count == 1) {
			msleep(100);

			mutex_unlock(&rda_pwm->pending_lock);
			complete(&rda_pwm->fb_done);

			return 0;
		}
		mutex_unlock(&rda_pwm->pending_lock);
	}else {
		INIT_COMPLETION(rda_pwm->fb_done);
		rda_pwm->fb_count = 0;
	}

	return 0;
}

static int rda_bl_set_intensity(struct backlight_device *bd)
{
	struct rda_pwm *rda_pwm = bl_get_data(bd);
	int intensity = bd->props.brightness;
	unsigned char level;
	unsigned long fb_timeout = 0;
	unsigned long timeout_ret = 0;

	if (intensity < 0) {
		level = rda_pwm->pdata->min;
	} else {
		if (rda_pwm->pdata->min + intensity > rda_pwm->pdata->max) {
			level = rda_pwm->pdata->max;
		} else {
			level = rda_pwm->pdata->min + intensity;
		}

		if (bd->props.state & BL_CORE_FBBLANK) {
			level = rda_pwm->pdata->min;
		}
	}

	//brightness intensity value
	rda_pwm->current_level = level;

	mutex_lock(&rda_pwm->pending_lock);
	if (rda_pwm->pwm_invert) {
		level = rda_pwm->pdata->max - level;
	}

	/* BL in suspend state, return*/
	if (rda_pwm->fb_count == 0) {
		int boot_mode = rda_get_boot_mode();

		mutex_unlock(&rda_pwm->pending_lock);
		if (boot_mode == BM_CHARGER) {
			schedule_work(&rda_pwm->bl_work);
			return 0;
		}

		fb_timeout = RDA_FB_TIMEOUT;
		timeout_ret = wait_for_completion_timeout(&rda_pwm->fb_done, msecs_to_jiffies(fb_timeout));
		if (timeout_ret == 0) {
			pr_err("<rda-pwm> : fb doesn't transit frame, timeout(%ld ms)\n", fb_timeout);
			return 0;
		}
		mutex_lock(&rda_pwm->pending_lock);
	} else if (rda_pwm->fb_count < 0) {
		mutex_unlock(&rda_pwm->pending_lock);
		return 0;
	}
	mutex_unlock(&rda_pwm->pending_lock);

	pr_debug("%s : write val = %d\n", __func__, level);

	if (rda_pwm->pwt_used) {
		hal_pwt_sel_level(rda_pwm, level);
	} else {
		hal_pwm_sel_level(rda_pwm, level);
	}

	if (rda_pwm->current_level > rda_pwm->pdata->min) {
		rda_bl_set_enable(rda_pwm, 1);
	} else {
		rda_bl_set_enable(rda_pwm, 0);
	}

	return 0;
}

static int rda_bl_get_intensity(struct backlight_device *bd)
{
	struct rda_pwm *rda_pwm = bl_get_data(bd);
	int intensity;

	intensity = rda_pwm->current_level - rda_pwm->pdata->min;

	pr_debug("%s : read intensity = %d\n", __func__, intensity);

	return intensity;
}

static void rda_bl_work(struct work_struct *work)
{
	struct rda_pwm *rda_pwm = container_of(work, struct rda_pwm, bl_work);

	rda_bl_set_intensity(rda_pwm->bldev);
}

static const struct backlight_ops rda_bl_ops = {
	.get_brightness = rda_bl_get_intensity,
	.update_status  = rda_bl_set_intensity,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_bl_early_suspend(struct early_suspend *h);
static void rda_bl_late_resume(struct early_suspend *h);
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int rda_pwm_probe(struct platform_device *pdev)
{
	struct backlight_properties props;
	struct backlight_device *bldev;
	struct rda_pwm *rda_pwm;
	struct resource *res;
	struct rda_bl_pwm_device_data *pdata;
	int retval;

	pdata = (struct rda_bl_pwm_device_data *)pdev->dev.platform_data;
	if (!pdata) {
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		return -ENXIO;
	}

	rda_pwm = kzalloc(sizeof(struct rda_pwm), GFP_KERNEL);
	if (!rda_pwm) {
		return -ENOMEM;
	}

	rda_pwm->base =ioremap(res->start, resource_size(res));
	if (!rda_pwm->base) {
		printk(KERN_ERR "ioremap fail\n");
		retval = -ENOMEM;
		goto err_free_mem;
	}

	rda_pwm->pdata = pdata;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = pdata->max - pdata->min;

	bldev = backlight_device_register(RDA_BL_DRV_NAME,
		&pdev->dev,
		rda_pwm,
		&rda_bl_ops,
		&props);
	if (IS_ERR(bldev)) {
		retval = PTR_ERR(bldev);
		goto err_free_mem;
	}

	init_completion(&rda_pwm->fb_done);
	INIT_WORK(&rda_pwm->bl_work, rda_bl_work);

	rda_pwm->bldev = bldev;

#ifdef CONFIG_HAS_EARLYSUSPEND
	rda_pwm->early_ops.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	rda_pwm->early_ops.suspend = rda_bl_early_suspend;
	rda_pwm->early_ops.resume = rda_bl_late_resume;
	register_early_suspend(&rda_pwm->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	rda_pwm->fb_notifier.notifier_call = rda_bl_notifier_cb;
	rda_fb_register_client(&rda_pwm->fb_notifier);

	rda_pwm->gpio_bl_on = pdata->gpio_bl_on;
	rda_pwm->gpio_bl_on_valid = pdata->gpio_bl_on_valid;
	rda_pwm->pwm_invert = pdata->pwm_invert;
	rda_pwm->pwm_clk = pdata->pwm_clk;
	rda_pwm->pwt_used = pdata->pwt_used;

	mutex_init(&rda_pwm->pending_lock);

	rda_pwm->fb_count = 0;
	if (rda_pwm->pwm_invert) {
		rda_pwm->pending_level = pdata->max;
	} else {
		rda_pwm->pending_level = 0;
	}

	rda_pwm->master_clk = clk_get(NULL, RDA_CLK_APB1);
	if (!rda_pwm->master_clk) {
		printk(KERN_ERR "no handler of clock\n");
		retval = -EINVAL;
		goto err_bl_register;
	}

	if (rda_pwm->gpio_bl_on_valid) {
		gpio_request(rda_pwm->gpio_bl_on, "LCD_BL_ON");
	}

	platform_set_drvdata(pdev, rda_pwm);

	/* make pwm output*/
	hal_pwm_init(rda_pwm);

	/* Power up the backlight by default at middle intesity. */
	bldev->props.power = FB_BLANK_UNBLANK;

	/* A pulse longer than 200ns is necessary to enable pwm ic,
	but current pwm signal can't satisfy this condition, so we
	need set brightness to the highest level firstly */
	bldev->props.brightness = bldev->props.max_brightness;
	schedule_work(&rda_pwm->bl_work);
	printk(KERN_INFO "rda pwm backlight is done\n");
	return 0;

err_bl_register:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&rda_pwm->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	backlight_device_unregister(rda_pwm->bldev);
err_free_mem:
	if (rda_pwm) {
		kfree(rda_pwm);
	}
	return retval;
}

static int __exit rda_pwm_remove(struct platform_device *pdev)
{
	struct rda_pwm *rda_pwm = platform_get_drvdata(pdev);

	if (!rda_pwm) {
		return -EINVAL;
	}

	if (rda_pwm->gpio_bl_on_valid) {
		gpio_free(rda_pwm->gpio_bl_on);
	}

	clk_put(rda_pwm->master_clk);
	rda_fb_unregister_client(&rda_pwm->fb_notifier);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&rda_pwm->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	backlight_device_unregister(rda_pwm->bldev);
	platform_set_drvdata(pdev, NULL);
	kfree(rda_pwm);

	return 0;
}

static void rda_pwm_shutdown(struct platform_device *pdev)
{
	struct rda_pwm *rda_pwm = platform_get_drvdata(pdev);

	if (rda_pwm->pwt_used) {
		hal_pwt_sel_level(rda_pwm, 0);
	} else {
		hal_pwm_output_enable(rda_pwm, 0);
	}
	rda_bl_set_enable(rda_pwm, 0);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_bl_early_suspend(struct early_suspend *h)
{
	struct rda_pwm *rda_pwm = container_of(h, struct rda_pwm, early_ops);

	mutex_lock(&rda_pwm->pending_lock);
	rda_pwm->fb_count = -1;
	rda_pwm->current_level = 0;
	mutex_unlock(&rda_pwm->pending_lock);

	if (rda_pwm->pwt_used) {
		hal_pwt_sel_level(rda_pwm, 0);
	} else {
		hal_pwm_output_enable(rda_pwm, 0);
	}
	rda_bl_set_enable(rda_pwm, 0);
}

static void rda_bl_late_resume(struct early_suspend *h)
{
	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static struct platform_driver rda_pwm_driver = {
	.driver = {
		.name = RDA_PWM_DRV_NAME,
	},
	.probe = rda_pwm_probe,
	.remove = rda_pwm_remove,
	.shutdown = rda_pwm_shutdown,
};

static int __init rda_pwm_init(void)
{
	return platform_driver_register(&rda_pwm_driver);
}

static void __exit rda_pwm_exit(void)
{
	platform_driver_unregister(&rda_pwm_driver);
}

module_init(rda_pwm_init);
module_exit(rda_pwm_exit);

MODULE_AUTHOR("RDA Microelectronics Inc");
MODULE_DESCRIPTION("RDA PWM Backlight Driver");
MODULE_LICENSE("GPL");
