/*
 * leds-rda.c - A driver for controlling led of RDA
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
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/leds-regulator.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <mach/regulator.h>
#include <plat/devices.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */


#define to_regulator_led(led_cdev) \
	container_of(led_cdev, struct regulator_led, cdev)

struct regulator_led {
	struct led_classdev cdev;

	int keyboard;
	int trigger;

	int bak_brightness;
	int cur_val;

	struct work_struct work;
	int set_val;

	struct mutex mutex;

	struct regulator *vcc;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_ops;
#endif /* CONFIG_HAS_EARLYSUSPEND */

};

static void rda_led_set_value(struct regulator_led *led, enum led_brightness value)
{
	int voltage = (value + 1) * 10000;
	int ret;

	mutex_lock(&led->mutex);

	if (led->cur_val == (int)value) {
		goto out;
	}

	if (led->cdev.max_brightness > 1) {
		dev_dbg(led->cdev.dev, "brightness: %d\n", value);

		ret = regulator_set_voltage(led->vcc, voltage, voltage);
		if (ret != 0) {
			dev_err(led->cdev.dev, "Failed to set brightness %d: %d\n", value, ret);
		} else {
			/*
			 * Backup current brightness,
			 * the member brightness of led_classdev may be modified by sys call,
			 * so we have to save previous value.
			 */
			led->bak_brightness = led->cur_val;
			led->cur_val = (int)value;
			led->cdev.brightness = (int)value;
		}
	}

out:
	mutex_unlock(&led->mutex);
}

static void rda_led_work(struct work_struct *work)
{
	struct regulator_led *led;

	led = container_of(work, struct regulator_led, work);

	rda_led_set_value(led, led->set_val);
}

static void rda_led_brightness_set(struct led_classdev *led_cdev,
			   enum led_brightness value)
{
	struct regulator_led *led = to_regulator_led(led_cdev);

	if (led->cdev.flags & LED_SUSPENDED) {
		return;
	}

	led->set_val = value;
	schedule_work(&led->work);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_led_early_suspend(struct early_suspend *h)
{
	struct regulator_led *rda_led = container_of(h, struct regulator_led, early_ops);

	if (!rda_led) {
		return;
	}

	rda_led_set_value(rda_led, LED_OFF);
}

static void rda_led_late_resume(struct early_suspend *h)
{
	struct regulator_led *rda_led = container_of(h, struct regulator_led, early_ops);

	if (!rda_led) {
		return;
	}

	rda_led_set_value(rda_led, rda_led->bak_brightness);
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int rda_led_probe(struct platform_device *pdev)
{
	struct rda_led_device_data *pdata = pdev->dev.platform_data;
	struct regulator_led *led;
	struct regulator *vcc;
	int ret = 0;

	if (pdata == NULL) {
		dev_err(&pdev->dev, "no platform data\n");
		return -ENODEV;
	}

	if (!pdata->ldo_name) {
		dev_err(&pdev->dev, "invalid name of led\n");
		return -EINVAL;
	}

	vcc = regulator_get_exclusive(&pdev->dev, pdata->ldo_name);
	if (IS_ERR(vcc)) {
		dev_err(&pdev->dev, "Cannot get vcc for %s\n", pdata->led_name);
		return PTR_ERR(vcc);
	}

	led = kzalloc(sizeof(*led), GFP_KERNEL);
	if (led == NULL) {
		ret = -ENOMEM;
		goto err_vcc;
	}

	led->cdev.max_brightness = LED_FULL;
	/* Set a value temporarily */
	led->cdev.brightness = LED_OFF;

	led->cdev.brightness_set = rda_led_brightness_set;
	led->cdev.name = pdata->led_name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;
	led->vcc = vcc;
	led->keyboard = pdata->is_keyboard;
	led->trigger = pdata->trigger;

	mutex_init(&led->mutex);

	if (led->keyboard) {
#ifdef CONFIG_HAS_EARLYSUSPEND
		led->early_ops.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 5;
		led->early_ops.suspend = rda_led_early_suspend;
		led->early_ops.resume = rda_led_late_resume;

		register_early_suspend(&led->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	} else if (led->trigger) {
		led->cdev.default_trigger = pdata->led_name;
	}

	platform_set_drvdata(pdev, led);

	INIT_WORK(&led->work, rda_led_work);

	ret = led_classdev_register(&pdev->dev, &led->cdev);
	if (ret < 0) {
		goto err_led;
	}

	/* Set the default led status */
	rda_led_set_value(led, LED_OFF);

	return 0;

err_led:

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&led->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	kfree(led);
err_vcc:
	regulator_put(vcc);
	return ret;
}

static int rda_led_remove(struct platform_device *pdev)
{
	struct regulator_led *led = platform_get_drvdata(pdev);

	rda_led_set_value(led, LED_OFF);

	if (led->keyboard) {
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&led->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	}

	led_classdev_unregister(&led->cdev);
	cancel_work_sync(&led->work);
	regulator_put(led->vcc);
	kfree(led);
	return 0;
}

static struct platform_driver rda_led_driver = {
	.driver = {
		   .name  = RDA_LEDS_DRV_NAME,
		   .owner = THIS_MODULE,
	},
	.probe  = rda_led_probe,
	.remove = rda_led_remove,
};

static int __init rda_led_init(void)
{
	return platform_driver_probe(&rda_led_driver, rda_led_probe);
}
module_init(rda_led_init);


MODULE_AUTHOR("Tao Lei<leitao@rdamicro.com>");
MODULE_DESCRIPTION("Regulator driven LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:leds-regulator");
