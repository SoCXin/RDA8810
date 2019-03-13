 /*
  * rda-usb-vbus.c - simple GPIO VBUS sensing driver for B peripheral devices
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
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/usb.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <plat/md_sys.h>

#include <linux/usb/gadget.h>
#include <linux/usb/gpio_vbus.h>
#include <linux/usb/otg.h>
#include <linux/regulator/consumer.h>
#include <plat/ispi.h>
#include <plat/boot_mode.h>
#include <plat/rda_charger.h>
#include <mach/regulator.h>
#include <plat/reg_sysctrl_8850e.h>
#include <plat/cpu.h>
/*
 * A simple GPIO VBUS sensing driver for B peripheral only devices
 * with internal transceivers. It can control a D+ pullup GPIO and
 * a regulator to limit the current drawn from VBUS.
 *
 * Needs to be loaded before the UDC driver that will use it.
 */
struct gpio_vbus_data {
	struct usb_phy phy;
	struct device *dev;
	//struct regulator *usb_ldo;
	int vbus_draw_enabled;
	unsigned mA;
	struct delayed_work work;
	struct delayed_work cable_work;
	struct delayed_work otg_switch;
	struct msys_device *vbus_msys;
	unsigned charger_status;
	int otg_detect;
};

enum {
	USB_CABLE,
	AC_CABLE
};

struct mutex mutex;
static struct regulator *usb_ldo;
static struct wake_lock vbus_lock;
static struct workqueue_struct *usb_otg_wq = NULL;
static struct clk *clk; /* 'usb' clock, enable/disable it when the cable is plugged in/out */

extern int android_usb_ready(void);
extern int musb_g_addressed(struct usb_gadget *gadget);
extern int musb_disconnect_gadget(struct usb_gadget *gadget);
extern int rda_modem_charger_enable(int enable);
extern void rda_start_host(struct usb_bus *host);
extern void rda_stop_host(struct usb_bus *host);
extern void rda_usbid_set(int value);
extern void show_devctl(struct usb_bus *host);
extern int charger_status_notify_is_ignore(void);

#ifdef CONFIG_MACH_RDA8810
void rda_vbus_release(void)
{
	mutex_lock(&mutex);
	if (regulator_is_enabled(usb_ldo))
		regulator_disable(usb_ldo);
	mutex_unlock(&mutex);
	wake_unlock(&vbus_lock);
}

void rda_vbus_acquire(void)
{
	wake_lock(&vbus_lock);
	mutex_lock(&mutex);
	if (regulator_is_enabled(usb_ldo) == 0) {
		if (regulator_enable(usb_ldo))
			pr_err("Failed to enable usb ldo\n");
	}
	mutex_unlock(&mutex);
}
#else
static int regulator_enabled = 0;
static int clk_enabled = 0;
void rda_regulator_enable(void)
{
	u16 metal_id = rda_get_soc_metal_id();
	if (metal_id < 3) {
		ispi_reg_write(0x82,0x020b);
		ispi_reg_write(0x81,0x7248);
	}
	ispi_reg_write(0x83,0x72e3);
	ispi_reg_write(0x89,0x7100);
	regulator_enabled = 1;
}

void rda_regulator_disable(void)
{
	u16 metal_id = rda_get_soc_metal_id();


	if (metal_id >= 2)
		ispi_reg_write(0x83,0x62c2);
	else
		ispi_reg_write(0x83,0x62c3);
	ispi_reg_write(0x89,0x2101);
	if (metal_id < 3) {
		ispi_reg_write(0x82,0x024b);
		ispi_reg_write(0x81,0x0000);
	}
	regulator_enabled = 0;
}

int rda_regulator_is_enabled(void)
{
	return regulator_enabled;
}

int rda_clk_is_enabled(void)
{
	return clk_enabled;
}
void rda_vbus_release(void)
{
	mutex_lock(&mutex);
	if (clk && (rda_clk_is_enabled())) {
		clk_disable_unprepare(clk);
		clk_enabled = 0;
	}
	if (rda_regulator_is_enabled())
		rda_regulator_disable();
	mutex_unlock(&mutex);
	wake_unlock(&vbus_lock);
}

void rda_vbus_acquire(void)
{
	wake_lock(&vbus_lock);
	mutex_lock(&mutex);
	if (rda_regulator_is_enabled() == 0)
		rda_regulator_enable();
	if (clk && (rda_clk_is_enabled()) == 0) {
		clk_prepare_enable(clk);
		clk_enabled = 1;
	}
	mutex_unlock(&mutex);
}
#endif /* CONFIG_MACH_RDA8810 */

static int notify_md_charger_type(struct msys_device *msys_dev, int type)
{
	struct client_cmd cmd_set;
	unsigned int cmd;
	unsigned int ret;
	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = msys_dev;
	cmd_set.mod_id = SYS_PM_MOD;
	cmd_set.mesg_id = SYS_PM_CMD_SET_CHARGER_TYPE;

	if (type == USB_CABLE)
		cmd = 1;
	else
		cmd = 0;
	cmd_set.pdata = (void *)&cmd;
	cmd_set.data_size = sizeof(cmd);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("<trace-ch> : notify modem charger type fail!\n");
		return -EINVAL;
	}
	return 0;
}

static void cable_type_work(struct work_struct *work)
{
	int type;
	struct gpio_vbus_data *gpio_vbus =
		container_of(work, struct gpio_vbus_data, cable_work.work);
	struct usb_gadget *gadget = gpio_vbus->phy.otg->gadget;


	if (!gadget) {
		pr_warning("the musb phy isn't configured\n");
		return;
	}

	if (musb_g_addressed(gadget)) {
		type = USB_CABLE;
	} else {
		type = AC_CABLE;
	}

	//check the cable type according to wether t here is data packet on the cable.
	if (type == USB_CABLE) {
		int status;
		status = USB_EVENT_NONE;
		gpio_vbus->phy.state = OTG_STATE_B_PERIPHERAL;
		gpio_vbus->phy.last_event = status;

		atomic_notifier_call_chain(&gpio_vbus->phy.notifier,
					   status, gadget);

		status = USB_EVENT_VBUS;
		gpio_vbus->phy.last_event = status;

		notify_md_charger_type(gpio_vbus->vbus_msys, USB_CABLE);
		atomic_notifier_call_chain(&gpio_vbus->phy.notifier,
					   status, gadget);
                if (rda_get_boot_mode() == BM_CHARGER) {
                        wake_lock_timeout(&vbus_lock, HZ * 5);
                }
	} else {
		notify_md_charger_type(gpio_vbus->vbus_msys, AC_CABLE);
		rda_vbus_release();
	}
}

static int is_vbus_powered(struct gpio_vbus_data *pvbus)
{
	if (pvbus->charger_status == PM_CHARGER_CHARGING ||
	    pvbus->charger_status == PM_CHARGER_FINISHED ||
	    pvbus->charger_status == PM_CHARGER_CONNECTED) {
		return 1;
	}

	return 0;
}

static void gpio_vbus_work(struct work_struct *work)
{
	struct gpio_vbus_data *gpio_vbus =
	    container_of(work, struct gpio_vbus_data, work.work);
	int status;
	struct usb_gadget *gadget;

	if (!gpio_vbus->phy.otg->gadget)
		return;

	gadget = gpio_vbus->phy.otg->gadget;
	/* Peripheral controllers which manage the pullup themselves won't have
	 * gpio_pullup configured here.  If it's configured here, we'll do what
	 * isp1301_omap::b_peripheral() does and enable the pullup here... although
	 * that may complicate usb_gadget_{,dis}connect() support.
	 */
	if (is_vbus_powered(gpio_vbus)) {
		int delay;

		rda_vbus_acquire();
		status = USB_EVENT_CHARGER;
		gpio_vbus->phy.state = OTG_STATE_B_PERIPHERAL;
		gpio_vbus->phy.last_event = status;

		pr_info("usb cable connect... \n");

		atomic_notifier_call_chain(&gpio_vbus->phy.notifier,
					   status, gpio_vbus->phy.otg->gadget);
		/* if android usb function don't ready, the usb will not work,
		 * so delay a rather long time.
		 */
		if (android_usb_ready())
			delay = 20 * HZ;
		else
			delay = 60 * HZ;
		schedule_delayed_work(&gpio_vbus->cable_work, delay);
	} else {
		if (delayed_work_pending(&gpio_vbus->cable_work))
			cancel_delayed_work_sync(&gpio_vbus->cable_work);
		//usb_gadget_vbus_disconnect(gpio_vbus->phy.otg->gadget);
		status = USB_EVENT_NONE;
		pr_info("usb cable disconnect... \n");
		musb_disconnect_gadget(gadget);
		gpio_vbus->phy.state = OTG_STATE_B_IDLE;
		gpio_vbus->phy.last_event = status;

		atomic_notifier_call_chain(&gpio_vbus->phy.notifier,
					   status, gpio_vbus->phy.otg->gadget);
		rda_vbus_release();
	}
}

static void gpio_otg_switch_work(struct work_struct *work)
{
	int id_value;
	struct usb_bus *host;
	struct gpio_vbus_data *gpio_vbus =
	    container_of(work, struct gpio_vbus_data, otg_switch.work);

	if (!gpio_vbus->phy.otg->host)
		return;
	id_value = gpio_get_value(gpio_vbus->otg_detect);
	host = gpio_vbus->phy.otg->host;
	if (id_value) {
		printk("disable vbus and enable charger\n");
		rda_stop_host(host);
		msleep(1000);
		rda_modem_charger_enable(1);
		if (regulator_is_enabled(usb_ldo))
			regulator_disable(usb_ldo);
	} else {
		printk("enable vbus and disable charge\n");
		if (regulator_enable(usb_ldo))
			pr_err("Failed to enable usb ldo\n");
		rda_modem_charger_enable(0);
		rda_start_host(host);
	}
}

int otg_usb_id_state(struct usb_phy *phy)
{
	struct gpio_vbus_data *gpio_vbus;
	int id_value;

	gpio_vbus = container_of(phy, struct gpio_vbus_data, phy);
	id_value = gpio_get_value(gpio_vbus->otg_detect);

	return id_value;
}
/* VBUS change IRQ handler */
static irqreturn_t gpio_otg_switch_irq(int irq, void *data)
{
	struct platform_device *pdev = data;
	struct gpio_vbus_data *gpio_vbus = platform_get_drvdata(pdev);
	unsigned gpio;
	unsigned delay;

	if (!gpio_vbus) {
		pr_info("gpio vbus isn't initialized\n");
		return IRQ_HANDLED;
	}
	gpio = gpio_vbus->otg_detect;
	if (gpio_get_value(gpio)) {
		wake_lock_timeout(&vbus_lock, 20 * HZ);
		delay = 10;
		rda_usbid_set(1);
		irq_set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);
	} else {
		wake_lock(&vbus_lock);
		/*
		 * if wakeup the system,must delay a long time for AP
		 * vcore calm;
		 */
		delay = 100;
		rda_usbid_set(0);
		irq_set_irq_type(irq, IRQ_TYPE_LEVEL_HIGH);
	}

	queue_delayed_work(usb_otg_wq, &gpio_vbus->otg_switch, delay);

	return IRQ_HANDLED;
}

/* OTG transceiver interface */

/* bind/unbind the peripheral controller */
static int gpio_vbus_set_peripheral(struct usb_otg *otg,
				    struct usb_gadget *gadget)
{

	struct gpio_vbus_data *gpio_vbus;
	struct gpio_vbus_mach_info *pdata;
	struct platform_device *pdev;
	gpio_vbus = container_of(otg->phy, struct gpio_vbus_data, phy);
	pdev = to_platform_device(gpio_vbus->dev);
	pdata = gpio_vbus->dev->platform_data;

	if (!gadget) {
		dev_dbg(&pdev->dev, "unregistering gadget '%s'\n",
			otg->gadget->name);

		usb_gadget_vbus_disconnect(otg->gadget);
		otg->phy->state = OTG_STATE_UNDEFINED;

		otg->gadget = NULL;
		return 0;
	}

	otg->gadget = gadget;
	dev_info(&pdev->dev, "registered gadget '%s'\n", gadget->name);

	return 0;
}

static int gpio_vbus_set_host(struct usb_otg *otg, struct usb_bus *host)
{

	struct gpio_vbus_data *gpio_vbus;
	struct platform_device *pdev;

	gpio_vbus = container_of(otg->phy, struct gpio_vbus_data, phy);
	pdev = to_platform_device(gpio_vbus->dev);
	if (!host) {
		dev_info(&pdev->dev, "unregistering host '%s'\n",
			 otg->host->bus_name);

		otg->host = NULL;
		return 0;
	}

	otg->host = host;
	dev_info(&pdev->dev, "registered host '%s'\n", host->bus_name);

	return 0;
}

static int rda_modem_charger_notify(struct notifier_block *nb,
				    unsigned long mesg, void *data)
{

	struct msys_device *pmsys_dev =
	    container_of(nb, struct msys_device, notifier);
	struct gpio_vbus_data *vbus_data =
	    (struct gpio_vbus_data *)(pmsys_dev->private);
	struct client_mesg *pmesg = (struct client_mesg *)data;
	u32 charger_status = 0;
	u32 delay;

	if (pmesg->mod_id != SYS_PM_MOD) {
		return NOTIFY_DONE;
	}

	switch (mesg) {
	case SYS_PM_MESG_BATT_STATUS:
		charger_status = (*((unsigned int *)&pmesg->param) >> 8) & 0xFF;
		if ((vbus_data->charger_status != charger_status) &&
			((charger_status == PM_CHARGER_CONNECTED) ||
			(charger_status == PM_CHARGER_DISCONNECTED))) {
			if (!charger_status_notify_is_ignore()) {
				vbus_data->charger_status = charger_status;
				delay = vbus_data->phy.otg->gadget ? 0 : 4*HZ;
				schedule_delayed_work(&vbus_data->work,delay);
			}
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static void gpio_otg_switch_irq_enable(unsigned long irq)
{
	enable_irq(irq);
}

static int gpio_vbus_phy_init(struct usb_phy* x)
{
	pr_info("%s \n", __func__);
	return 0;
}
/* platform driver interface */


static int __init gpio_vbus_probe(struct platform_device *pdev)
{
	struct gpio_vbus_mach_info *pdata = pdev->dev.platform_data;
	struct gpio_vbus_data *gpio_vbus;
	int err = 0;
	int otg_detect;
	pr_info("%s \n",__func__);
	if (!pdata) {
		return -EINVAL;
	}

	gpio_vbus = kzalloc(sizeof(struct gpio_vbus_data), GFP_KERNEL);
	if (!gpio_vbus)
		return -ENOMEM;

	gpio_vbus->phy.otg = kzalloc(sizeof(struct usb_otg), GFP_KERNEL);
	if (!gpio_vbus->phy.otg) {
		kfree(gpio_vbus);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, gpio_vbus);

	gpio_vbus->dev = &pdev->dev;
	gpio_vbus->phy.label = "gpio-vbus";
	gpio_vbus->phy.state = OTG_STATE_UNDEFINED;

	gpio_vbus->phy.otg->phy = &gpio_vbus->phy;
	gpio_vbus->phy.otg->set_peripheral = gpio_vbus_set_peripheral;

	gpio_vbus->phy.type = USB_PHY_TYPE_UNDEFINED;
	gpio_vbus->phy.init = gpio_vbus_phy_init;
	gpio_vbus->phy.dev = gpio_vbus->dev;
	// ap <---> modem pm
	gpio_vbus->vbus_msys = rda_msys_alloc_device();
	if (!gpio_vbus->vbus_msys) {
		goto err_msys;
	}

	gpio_vbus->vbus_msys->module = SYS_PM_MOD;
	gpio_vbus->vbus_msys->name = "gpio-vbus";
	gpio_vbus->vbus_msys->notifier.notifier_call = rda_modem_charger_notify;
	gpio_vbus->vbus_msys->private = (void *)gpio_vbus;

	clk = clk_get(&pdev->dev, "usb");

	mutex_init(&mutex);
	rda_msys_register_device(gpio_vbus->vbus_msys);

	ATOMIC_INIT_NOTIFIER_HEAD(&gpio_vbus->phy.notifier);

	INIT_DELAYED_WORK(&gpio_vbus->work, gpio_vbus_work);

	INIT_DELAYED_WORK(&gpio_vbus->cable_work, cable_type_work);

	/* only active when a gadget is registered */
	err = usb_add_phy(&gpio_vbus->phy,USB_PHY_TYPE_USB2);
	pr_info("%s add phy ret %d \n",__func__ , err);
	if (err) {
		dev_err(&pdev->dev, "can't register transceiver, err: %d\n",
			err);
		goto err_otg;
	}

	usb_ldo = regulator_get(gpio_vbus->dev, LDO_USB);
	if (IS_ERR(usb_ldo)) {
		dev_err(&pdev->dev, "can't get usb ldo\n");
		goto err_otg;
	}
	otg_detect = pdata->gpio_vbus;
	if(!gpio_is_valid(otg_detect)){
		return 0;
	}

	err = gpio_request(otg_detect, "otg_detect");
	if (!err) {
		int irq;
		static struct timer_list timer;

		gpio_direction_input(otg_detect);

		gpio_vbus->otg_detect = otg_detect;
		usb_otg_wq = create_singlethread_workqueue("usb_otg_wq");
		if (!usb_otg_wq) {
			pr_info("Fail to alloc usb otg workqueue\n");
			err = -ENOMEM;
			goto err_ldo;
		}
		INIT_DELAYED_WORK(&gpio_vbus->otg_switch, gpio_otg_switch_work);
		gpio_vbus->phy.otg->set_host = gpio_vbus_set_host;
		irq = gpio_to_irq(otg_detect);
		set_irq_flags(irq, IRQF_VALID | IRQF_NOAUTOEN);
		err = request_irq(irq, gpio_otg_switch_irq, IRQF_TRIGGER_LOW
				  | IRQF_NO_SUSPEND, "usb_otg_switch", pdev);
		if (err) {
			dev_err(&pdev->dev, "can't request irq %i, err: %d\n",
				irq, err);
			goto err_irq;
		}

		/*
		   modem sys module load is very late, if we call the mdsys api
		   before that, we will die. enable irq later
		 */
		init_timer(&timer);
		timer.expires = jiffies + 500;
		timer.function = gpio_otg_switch_irq_enable;
		timer.data = irq;
		add_timer(&timer);
	}

	return 0;

err_irq:
	destroy_workqueue(usb_otg_wq);
	gpio_free(otg_detect);
err_ldo:
	regulator_put(usb_ldo);
err_otg:
//	usb_set_phy_dev(NULL);
err_msys:
	platform_set_drvdata(pdev, NULL);
	kfree(gpio_vbus->phy.otg);
	kfree(gpio_vbus);
	return err;
}


static int __exit gpio_vbus_remove(struct platform_device *pdev)
{
	struct gpio_vbus_data *gpio_vbus = platform_get_drvdata(pdev);
	int otg_detect = gpio_vbus->otg_detect;

	if (clk) {
		clk_disable(clk);
		clk_unprepare(clk);
		clk_put(clk);
	}

	usb_remove_phy(&gpio_vbus->phy);

	rda_msys_unregister_device(gpio_vbus->vbus_msys);
	rda_msys_free_device(gpio_vbus->vbus_msys);

	free_irq(gpio_to_irq(otg_detect), &pdev->dev);
	gpio_free(otg_detect);

	platform_set_drvdata(pdev, NULL);
	kfree(gpio_vbus->phy.otg);
	kfree(gpio_vbus);

	return 0;
}

/* NOTE:  the gpio-vbus device may *NOT* be hotplugged */

MODULE_ALIAS("platform:gpio-vbus");

static struct platform_driver gpio_vbus_driver = {
	.driver = {
		   .name = "gpio-vbus",
		   .owner = THIS_MODULE,
		   },
	.remove = __exit_p(gpio_vbus_remove),
};

static int __init gpio_vbus_init(void)
{
	wake_lock_init(&vbus_lock, WAKE_LOCK_SUSPEND, "vbus wakelock");

	return platform_driver_probe(&gpio_vbus_driver, gpio_vbus_probe);
}

module_init(gpio_vbus_init);

static void __exit gpio_vbus_exit(void)
{
	platform_driver_unregister(&gpio_vbus_driver);
}

module_exit(gpio_vbus_exit);

MODULE_DESCRIPTION("simple GPIO controlled OTG transceiver driver");
MODULE_AUTHOR("RDA Microelectronics");
MODULE_LICENSE("GPL");
