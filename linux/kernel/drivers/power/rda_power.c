/*
 * drivers/power/rda_power.c
 *
 * Copyright (c) 2013, RDA Microelectronics Inc.
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/jiffies.h>
#include <linux/usb/otg.h>
#include <plat/md_sys.h>
#include <plat/boot_mode.h>
#include <plat/rda_charger.h>

struct rda_battery_info {
	struct switch_dev sdev;
	spinlock_t lock;

	uint32_t capacity;
	uint32_t voltage;
	uint32_t charging;
	uint32_t status;
	int temperature;
	int ac_online;
	int usb_online;
	/* Used for enable/disable charger of modem */
	int charger_switch;

	int charger_current_adj;
	int charger_current;
	int volt_pct_min;
	int volt_pct_max;

	struct wake_lock charger_lock;
#ifndef CONFIG_RDA_FPGA
	struct msys_device *power_msys;
#endif /* CONFIG_RDA_FPGA */
};


enum {
	SUPPLY_BATTERY,
	SUPPLY_AC,
	SUPPLY_USB
};

static struct rda_battery_info *battery_info;


static int rda_ac_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct rda_battery_info *info = battery_info;
	int ret = 0;

	if (!info) {
		return -ENODEV;
	}

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			if (likely(psy->type == POWER_SUPPLY_TYPE_MAINS)) {
				val->intval = info->ac_online ? 1 : 0;
			} else {
				ret = -EINVAL;
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int rda_usb_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct rda_battery_info *info = battery_info;
	int ret = 0;

	if (!info) {
		return -ENODEV;
	}

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = info->usb_online ? 1 : 0;
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int rda_battery_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct rda_battery_info *info = battery_info;
	int ret = 0;

	if (!info) {
		return -ENODEV;
	}

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			/* suppose battery always online */
			if (info->charging) {
				if (info->status == PM_CHARGER_FINISHED)
					val->intval = POWER_SUPPLY_STATUS_FULL;
				else
					val->intval = POWER_SUPPLY_STATUS_CHARGING;
			} else {
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			}
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = 1;
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = (info->capacity > 100) ? 100 : info->capacity;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = info->voltage;
			break;
		case POWER_SUPPLY_PROP_TEMP:
			//val->intval = info->temperature;
			val->intval = 360;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	if(info->status == PM_CHARGER_ERROR_VOLTAGE)
		switch_set_state(&info->sdev,1);
	else
		switch_set_state(&info->sdev,0);

	return ret;
}

static enum power_supply_property rda_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TEMP
};

static enum power_supply_property rda_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property rda_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static inline int usb_connected(void)
{
#ifndef CONFIG_RDA_FPGA
	struct client_cmd cmd_set;
	unsigned int ret = 0;
	unsigned int chrStatus = 0;
	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = battery_info->power_msys;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_GET_CHARGER_STATUS;

	cmd_set.pdata = NULL;
	cmd_set.data_size = 0;
	cmd_set.pout_data = &chrStatus ;
	cmd_set.out_size = sizeof(chrStatus);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		return 0;
	}
	/*0,1 mean unknow,unpluged*/
	if(chrStatus > 1)
		return 1;
	else
		return 0;
#else
	return 1;
#endif
}

static int pre_usb_online = 0xFFFF;
static int pre_ac_online = 0xFFFF;

static int enable_usb_charge(struct rda_battery_info *battery_info)
{
	battery_info->charging = 1;
	return 0;
}

static int enable_ac_charge(struct rda_battery_info *battery_info)
{
	battery_info->charging = 1;
	return 0;
}

static int charge_stop(struct rda_battery_info *battery_info)
{
	battery_info->charging = 0;
	return 0;
}

static char *supply_list[] = {
	"battery",
	"ac",
	"usb"
};

static struct power_supply rda_supply[] = {
	{
		.name = "battery",
		.type	 = POWER_SUPPLY_TYPE_BATTERY,
		.properties = rda_battery_props,
		.num_properties = ARRAY_SIZE(rda_battery_props),
		.get_property = rda_battery_get_property,
	},
	{
		.name = "ac",
		.type	 = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = rda_ac_props,
		.num_properties = ARRAY_SIZE(rda_ac_props),
		.get_property = rda_ac_get_property,
	},
	{
		.name = "usb",
		.type	 = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties =rda_usb_props,
		.num_properties = ARRAY_SIZE(rda_usb_props),
		.get_property = rda_usb_get_property,
	},
};

static struct usb_phy *transceiver;
static struct notifier_block otg_nb;

static void otgwl_handle_event(unsigned long event)
{
	struct rda_battery_info *info = battery_info;

	switch (event) {
		case USB_EVENT_VBUS:
		case USB_EVENT_ENUMERATED:
			battery_info->usb_online = 1;
			if (battery_info->usb_online != pre_usb_online) {
				enable_usb_charge(battery_info);
				power_supply_changed(&rda_supply[SUPPLY_USB]);
				pre_usb_online = battery_info->usb_online;
			}
			break;

		case USB_EVENT_ID:
		case USB_EVENT_CHARGER:
			wake_lock_timeout(&info->charger_lock, 10 * HZ);
			battery_info->ac_online = 1;
			if (battery_info->ac_online != pre_ac_online) {
				enable_ac_charge(battery_info);
				power_supply_changed(&rda_supply[SUPPLY_AC]);
				pre_ac_online = battery_info->ac_online;
			}
			break;

		case USB_EVENT_NONE:
			battery_info->usb_online = 0;
			battery_info->ac_online = 0;
			if (battery_info->ac_online != pre_ac_online) {
				charge_stop(battery_info);
				power_supply_changed(&rda_supply[SUPPLY_AC]);
				pre_ac_online = battery_info->ac_online;
			} else if (battery_info->usb_online != pre_usb_online) {
				charge_stop(battery_info);
				power_supply_changed(&rda_supply[SUPPLY_USB]);
				pre_usb_online = battery_info->usb_online;
			}
			// pr_info("charger plug out interrupt happen\n");
			wake_lock_timeout(&info->charger_lock, 3 * HZ);
			break;

		default:
			break;
	}
}

static ssize_t rda_battery_sw_state(struct switch_dev *sdev,char *buf)
{
	struct rda_battery_info *data = container_of(sdev,struct rda_battery_info,sdev);

	if(data->status == PM_CHARGER_ERROR_VOLTAGE)
		return sprintf(buf,"1");
	else
		return sprintf(buf,"0");

}
static int otg_handle_notification(struct notifier_block *nb,
		unsigned long event, void *unused)
{
	otgwl_handle_event(event);

	return NOTIFY_OK;
}

static int rda_modem_pm_notify(struct notifier_block *nb, unsigned long mesg, void *data)
{
#ifndef CONFIG_RDA_FPGA
	struct msys_device *pmsys_dev = container_of(nb, struct msys_device, notifier);
	struct rda_battery_info *power_data = (struct rda_battery_info *)(pmsys_dev->private);
	struct client_mesg *pmesg = (struct client_mesg *)data;
	u32 pm_data = 0;
	u32 capacity = 100;
	u32 voltage = 5000000;
	u32 status = 2;
	s32 temperature = 0;

	if (pmesg->mod_id != SYS_PM_MOD) {
		return NOTIFY_DONE;
	}

	switch(mesg) {
		case SYS_PM_MESG_BATT_STATUS:
			pm_data  = *((unsigned int*)&(pmesg->param));
			capacity = (pm_data & 0xFF);
			status = (pm_data >> 8) & 0xFF;
			/* Convert unit to uV. */
			voltage  = ((pm_data & (0xFFFF << 16)) >> 16) * 1000;

			pm_data  = *(((unsigned int*)&(pmesg->param))+1);
			temperature = ((s32)(pm_data & (0xFFFF << 16))) >> 16;

			if (power_data->capacity != capacity ||
					power_data->status != status ||
					power_data->temperature != temperature ||
					power_data->voltage != voltage) {
				power_data->capacity = capacity;
				power_data->status = status;
				power_data->voltage = voltage;
				power_data->temperature = temperature;

				power_supply_changed(&rda_supply[SUPPLY_BATTERY]);
			}
			break;

		default:
			break;
	}
	printk(KERN_DEBUG "%s status:0x%x \n",__func__,status);
#endif /* CONFIG_RDA_FPGA */
	return NOTIFY_OK;
}

int rda_modem_charger_enable(int enable)
{
#ifndef CONFIG_RDA_FPGA
	struct client_cmd cmd_set;
	int value = !!enable;
	unsigned int ret;

	if (!battery_info && !battery_info->power_msys) {
		return -EPERM;
	}

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = battery_info->power_msys;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_ENABLE_CHARGER;

	cmd_set.pdata = (void *)&value;
	cmd_set.data_size = sizeof(value);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		return -EIO;
	}

	battery_info->charger_switch = value;
#endif
	return 0;
}
EXPORT_SYMBOL(rda_modem_charger_enable);

/* Enable/Disable charger current adjust dynamic */
int rda_modem_charger_current_adj(int value)
{
#ifndef CONFIG_RDA_FPGA
	struct client_cmd cmd_set;
	unsigned int ret;

	if (!battery_info && !battery_info->power_msys) {
		return -EPERM;
	}

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = battery_info->power_msys;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_EN_CHARGER_CURRENT;

	cmd_set.pdata = (void *)&value;
	cmd_set.data_size = sizeof(value);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		return -EIO;
	}

	battery_info->charger_current_adj = value;
#endif

	return 0;
}

int rda_modem_set_charger_current(int value)
{
#ifndef CONFIG_RDA_FPGA
	struct client_cmd cmd_set;
	unsigned int ret;

	if (!battery_info && !battery_info->power_msys) {
		return -EPERM;
	}

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = battery_info->power_msys;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_SET_CHARGER_CURRENT;

	cmd_set.pdata = (void *)&value;
	cmd_set.data_size = sizeof(value);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		return -EIO;
	}

	battery_info->charger_current = value;
#endif

	return 0;
}

int rda_modem_set_volt_pct_min(int value)
{
#ifndef CONFIG_RDA_FPGA
	struct client_cmd cmd_set;
	unsigned int ret;

	if (!battery_info && !battery_info->power_msys) {
		return -EPERM;
	}

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = battery_info->power_msys;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_SET_VOLT_PCT_MIN;

	cmd_set.pdata = (void *)&value;
	cmd_set.data_size = sizeof(value);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		return -EIO;
	}

	battery_info->volt_pct_min = value;
#endif

	return 0;
}

int rda_modem_set_volt_pct_max(int value)
{
#ifndef CONFIG_RDA_FPGA
	struct client_cmd cmd_set;
	unsigned int ret;

	if (!battery_info && !battery_info->power_msys) {
		return -EPERM;
	}

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = battery_info->power_msys;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_SET_VOLT_PCT_MAX;

	cmd_set.pdata = (void *)&value;
	cmd_set.data_size = sizeof(value);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		return -EIO;
	}

	battery_info->volt_pct_max = value;
#endif

	return 0;
}


static int ignore_charger_status_notify = 0;

int charger_status_notify_is_ignore(void)
{
	return ignore_charger_status_notify;
}

static ssize_t rda_charger_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", battery_info->charger_switch);
}

static ssize_t rda_charger_enable_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int enable;

	if (sscanf(buf, "%u", &enable) != 1)
		return -EINVAL;

	ignore_charger_status_notify = 1;

	if (enable == 1) {
		rda_modem_charger_enable(1);
	} else if (enable == 0) {
		rda_modem_charger_enable(0);
	}

	return count;
}

static ssize_t rda_charger_current_adj_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	return sprintf(buf, "%u\n", battery_info->charger_current_adj);
}

static ssize_t rda_charger_current_adj_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	rda_modem_charger_current_adj(value);

	return count;
}

static ssize_t rda_charger_set_current_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	return sprintf(buf, "%u\n", battery_info->charger_current);
}

static ssize_t rda_charger_set_current_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	rda_modem_set_charger_current(value);

	return count;
}

static ssize_t rda_volt_pct_min_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	return sprintf(buf, "%u\n", battery_info->volt_pct_min);
}

static ssize_t rda_volt_pct_min_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	rda_modem_set_volt_pct_min(value);

	return count;
}

static ssize_t rda_volt_pct_max_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	return sprintf(buf, "%u\n", battery_info->volt_pct_max);
}

static ssize_t rda_volt_pct_max_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	rda_modem_set_volt_pct_max(value);

	return count;
}

static DEVICE_ATTR(charger_enable, S_IRUGO | S_IWUSR,
		rda_charger_enable_show, rda_charger_enable_store);
static DEVICE_ATTR(charger_current_adj, S_IRUGO | S_IWUSR,
		rda_charger_current_adj_show, rda_charger_current_adj_store);
static DEVICE_ATTR(charger_current, S_IRUGO | S_IWUSR,
		rda_charger_set_current_show, rda_charger_set_current_store);
static DEVICE_ATTR(volt_pct_min, S_IRUGO | S_IWUSR,
		rda_volt_pct_min_show, rda_volt_pct_min_store);
static DEVICE_ATTR(volt_pct_max, S_IRUGO | S_IWUSR,
		rda_volt_pct_max_show, rda_volt_pct_max_store);

static int rda_battery_probe(struct platform_device *pdev)
{
	int ret = -ENODEV;
	struct rda_battery_info *data;
	int i;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, data);
	battery_info = data;

	/* Dafault data */
	battery_info->temperature = 0;
	battery_info->capacity = 100;
	/* Unit is uV. */
	battery_info->voltage = 5000 * 1000;

	battery_info->charger_current_adj = 0;
	battery_info->charger_current = 200;
	battery_info->volt_pct_min = 30;
	battery_info->volt_pct_max = 80;

	spin_lock_init(&data->lock);

	for (i = 0; i < ARRAY_SIZE(rda_supply); i++) {
		ret = power_supply_register(&pdev->dev, &rda_supply[i]);
		if (ret) {
			pr_err("Failed to register power supply\n");
			while (i--)
				power_supply_unregister(&rda_supply[i]);
			kfree(data);
			return ret;
		}
	}

	data->charging = 0;

	dev_dbg(&pdev->dev, "charger present: %d capacity %d\n",
			usb_connected(), data->capacity);

	wake_lock_init(&data->charger_lock, WAKE_LOCK_SUSPEND, "rda_charger_lock");

	data->usb_online = 0;
	data->ac_online = 0;

	transceiver = usb_get_phy(USB_PHY_TYPE_USB2);
	if (!IS_ERR_OR_NULL(transceiver)) {
		otg_nb.notifier_call = otg_handle_notification;
		ret = usb_register_notifier(transceiver, &otg_nb);
		if (ret) {
			dev_warn(&pdev->dev, "failure to register otg notifier\n");
		}

		otgwl_handle_event(transceiver->last_event);
	} else {
		dev_warn(&pdev->dev, "failed to get usb transceiver\n");
	}

#ifndef CONFIG_RDA_FPGA
	// ap <---> modem pm
	data->power_msys = rda_msys_alloc_device();
	if (!data->power_msys) {
		ret = -EINVAL;
		goto err;
	}

	data->sdev.name	= "invalid_charger";
	data->sdev.print_state = rda_battery_sw_state;
	if(switch_dev_register(&data->sdev) < 0)
	{
		pr_err("%s switch dev unable register!\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	data->power_msys->module = SYS_PM_MOD;
	data->power_msys->name = "rda-power";
	data->power_msys->notifier.notifier_call = rda_modem_pm_notify;
	data->power_msys->private = (void *)data;

	rda_msys_register_device(data->power_msys);
#endif
	data->ac_online = usb_connected();

	device_create_file(&pdev->dev, &dev_attr_charger_enable);
	device_create_file(&pdev->dev, &dev_attr_charger_current_adj);
	device_create_file(&pdev->dev, &dev_attr_charger_current);
	device_create_file(&pdev->dev, &dev_attr_volt_pct_min);
	device_create_file(&pdev->dev, &dev_attr_volt_pct_max);

	return 0;

err:
	power_supply_unregister(&rda_supply[SUPPLY_USB]);
	power_supply_unregister(&rda_supply[SUPPLY_AC]);
	power_supply_unregister(&rda_supply[SUPPLY_BATTERY]);

	kfree(data);
	battery_info = NULL;
	return ret;
}

static int rda_battery_remove(struct platform_device *pdev)
{
	struct rda_battery_info *data = platform_get_drvdata(pdev);

	power_supply_unregister(&rda_supply[SUPPLY_USB]);
	power_supply_unregister(&rda_supply[SUPPLY_AC]);
	power_supply_unregister(&rda_supply[SUPPLY_BATTERY]);

#ifndef CONFIG_RDA_FPGA
	switch_dev_unregister(&data->sdev);
	rda_msys_unregister_device(data->power_msys);
	rda_msys_free_device(data->power_msys);
#endif

	kfree(data);
	battery_info = NULL;
	return 0;
}
#if defined (CONFIG_PM)
static int rda_battery_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

/* any smbus transaction will wake up pad */
static int rda_battery_resume(struct platform_device *pdev)
{
	return 0;
}


static void rda_battery_shutdown(struct platform_device *pdev)
{
}
#endif

static struct platform_driver rda_battery_driver = {
	.probe	= rda_battery_probe,
	.remove 	= rda_battery_remove,
#if defined (CONFIG_PM)
	.suspend	= rda_battery_suspend,
	.resume 	= rda_battery_resume,
	.shutdown = rda_battery_shutdown,
#endif
	.driver = {
		.name = "rda-power",
	},
};
static int __init rda_battery_init(void)
{
	return platform_driver_register(&rda_battery_driver);
}
module_init(rda_battery_init);

static void __exit rda_battery_exit(void)
{
	platform_driver_unregister(&rda_battery_driver);
}
module_exit(rda_battery_exit);

MODULE_AUTHOR("RDA Microelectronics");
MODULE_DESCRIPTION("rda power monitor driver");
MODULE_LICENSE("GPL");

