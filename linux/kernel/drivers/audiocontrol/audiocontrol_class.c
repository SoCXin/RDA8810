/*
 *  drivers/audiocontrol/audiocontrol_class.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/audiocontrol.h>

struct class *audiocontrol_class;
static atomic_t device_count;

static ssize_t mute_get(struct device *dev, struct device_attribute *attr,
		char *buf)
{

	struct audiocontrol_dev *acdev = (struct audiocontrol_dev *)
		dev_get_drvdata(dev);

	int ret = 0;

	if (acdev->get_mute) {
		ret = acdev->get_mute(acdev);
		if (ret < 0)
			return ret;
	}
	return sprintf(buf, "%d\n", acdev->mute);
}

static ssize_t mute_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	struct audiocontrol_dev *acdev = (struct audiocontrol_dev *)
		dev_get_drvdata(dev);

	int val = 0, ret = 0;
	char *end = NULL;

	if(!buf)
		return -EINVAL;

	val = simple_strtol(buf, &end, 0);

	if(end == buf)
		return -EINVAL;

	if (acdev->set_mute) {
		ret = acdev->set_mute(acdev, val);
	}

	return len;
}

static ssize_t volume_get(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct audiocontrol_dev *acdev = (struct audiocontrol_dev *)
		dev_get_drvdata(dev);

	int ret = 0;

	if (acdev->get_volume) {
		ret = acdev->get_volume(acdev);
		if (ret < 0)
			return ret;
	}
	return sprintf(buf, "%d\n", acdev->volume);
}

static ssize_t volume_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	struct audiocontrol_dev *acdev = (struct audiocontrol_dev *)
		dev_get_drvdata(dev);

	int val = 0, ret = 0;
	char *end = NULL;

	if(!buf)
		return -EINVAL;

	val = simple_strtol(buf, &end, 0);

	if(end == buf)
		return -EINVAL;

	if (acdev->set_volume)
		ret = acdev->set_volume(acdev, val);

	return len;
}

static ssize_t name_get(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct audiocontrol_dev *acdev = (struct audiocontrol_dev *)
		dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", acdev->name);
}

static DEVICE_ATTR(mute, S_IRUGO | S_IWUSR, mute_get, mute_set);
static DEVICE_ATTR(volume, S_IRUGO | S_IWUSR, volume_get, volume_set);
static DEVICE_ATTR(name, S_IRUGO | S_IWUSR, name_get, NULL);

static int create_audiocontrol_class(void)
{
	if (!audiocontrol_class) {
		audiocontrol_class = class_create(THIS_MODULE, "audiocontrol");
		if (IS_ERR(audiocontrol_class))
			return PTR_ERR(audiocontrol_class);
		atomic_set(&device_count, 0);
	}

	return 0;
}

static ssize_t audiocontrol_dev_common_store(struct device *dev, struct device_attribute *attr, const char *buf,size_t count)
{
	pr_info("no support \n");
	return count;
}

int audiocontrol_dev_register(struct audiocontrol_dev *acdev)
{
	int ret;

	if (!audiocontrol_class) {
		ret = create_audiocontrol_class();
		if (ret < 0)
		{
			pr_info("%s fail create aud class :%08x\n ",__func__,ret);
			return ret;
		}
	}
	//dev_attr_volume.store = audiocontrol_dev_common_store ;
	//dev_attr_mute.store = audiocontrol_dev_common_store ;
	dev_attr_name.store = audiocontrol_dev_common_store ;
	acdev->index = atomic_inc_return(&device_count);
	acdev->dev = device_create(audiocontrol_class, NULL,
		MKDEV(0, acdev->index), NULL, acdev->name);
	if (IS_ERR(acdev->dev))
		return PTR_ERR(acdev->dev);

	ret = device_create_file(acdev->dev, &dev_attr_volume);
	if (ret < 0)
		goto err_create_file_1;
	ret = device_create_file(acdev->dev, &dev_attr_mute);
	if (ret < 0)
		goto err_create_file_2;
	ret = device_create_file(acdev->dev, &dev_attr_name);
	if (ret < 0)
		goto err_create_file_3;

	dev_set_drvdata(acdev->dev, acdev);
	return 0;

err_create_file_3:
	device_remove_file(acdev->dev, &dev_attr_volume);
err_create_file_2:
	device_remove_file(acdev->dev, &dev_attr_mute);
err_create_file_1:
	device_destroy(audiocontrol_class, MKDEV(0, acdev->index));
	printk(KERN_ERR "audiocontrol: Failed to register driver %s\n", acdev->name);

	return ret;
}
EXPORT_SYMBOL_GPL(audiocontrol_dev_register);

void audiocontrol_dev_unregister(struct audiocontrol_dev *acdev)
{
	device_remove_file(acdev->dev, &dev_attr_name);
	device_remove_file(acdev->dev, &dev_attr_mute);
	device_remove_file(acdev->dev, &dev_attr_volume);
	device_destroy(audiocontrol_class, MKDEV(0, acdev->index));
	dev_set_drvdata(acdev->dev, NULL);
}
EXPORT_SYMBOL_GPL(audiocontrol_dev_unregister);

static int __init audiocontrol_class_init(void)
{
	return create_audiocontrol_class();
}

static void __exit audiocontrol_class_exit(void)
{
	class_destroy(audiocontrol_class);
}

module_init(audiocontrol_class_init);
module_exit(audiocontrol_class_exit);

MODULE_AUTHOR("yulongwang <yulongwang@android.com>");
MODULE_DESCRIPTION("audiocontrol class driver");
MODULE_LICENSE("GPL");
