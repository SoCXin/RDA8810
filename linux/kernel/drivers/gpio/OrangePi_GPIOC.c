/*
 * Dirver for GOPIC on OrangePi 2G-IOT
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h> 
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/audiocontrol.h>
#include <linux/gpio.h>
#include <plat/md_sys.h>
#include <linux/leds.h>

#include <rda/tgt_ap_board_config.h>

#define CLASS_NAME     "OrangePi_2G-IOT-GPIOC"
#define GPIO_NR         1

char *GPIO_name[GPIO_NR] = { "PC27" };

struct msys_device *bp_gpioc_msys = NULL;

struct OrangePi_gpio_classdev {
	const char       *name;
	struct mutex     class_mutex;
	int gpio;
	int (*OrangePi_gpio_data_set)(struct OrangePi_gpio_classdev *, int);
	int (*OrangePi_gpio_data_get)(struct OrangePi_gpio_classdev *);
	struct device    *dev;
	struct list_head node;
};

struct OrangePi_gpio_pd {
	char name[16];
	char link[16];
};

static struct class *OrangePi_gpio_class;
static struct OrangePi_gpio_pd *OrangePi_pdata[256];
static struct platform_device *OrangePi_gpio_dev[256];

/* rda_gpioc_data driver data */
struct rda_gpioc_data {
	struct msys_device *gpioc_msys;
};

typedef struct
{
	u8 id;
	u8 value;
	u8 default_value1;
	u8 default_value2;
} rda_gpioc_op;

#ifdef _TGT_AP_LED_RED_FLASH
#define LED_CAM_FLASH	"red-flash"
#elif defined(_TGT_AP_LED_GREEN_FLASH)
#define LED_CAM_FLASH	"green-flash"
#elif defined(_TGT_AP_LED_BLUE_FLASH)
#define LED_CAM_FLASH	"blue-flash"
#endif

#ifdef LED_CAM_FLASH
DEFINE_LED_TRIGGER(rda_sensor_led);
#endif


static int rda_modem_gpioc_notify(struct notifier_block *nb, unsigned long mesg, void *data)
{
	struct msys_device *pmsys_dev = container_of(nb, struct msys_device, notifier);
	struct client_mesg *pmesg = (struct client_mesg *)data;


	if (pmesg->mod_id != SYS_GEN_MOD) {
		return NOTIFY_DONE;
	}

	if (mesg != SYS_GEN_MESG_RTC_TRIGGER) {
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

int rda_gpioc_operation(rda_gpioc_op *gpioc_op)
{
	int enable;
	int ret, value;
	u8 data[sizeof(rda_gpioc_op)] = { 0 };
	struct client_cmd gpioc_cmd;

	value = sizeof(rda_gpioc_op);

	memcpy(data, gpioc_op, sizeof(rda_gpioc_op));
	memset(&gpioc_cmd, 0, sizeof(gpioc_cmd));
	gpioc_cmd.pmsys_dev = bp_gpioc_msys;
	gpioc_cmd.mod_id = SYS_GPIO_MOD;
	gpioc_cmd.mesg_id = SYS_GPIO_CMD_OPERATION;
	gpioc_cmd.pdata = (void *)&data;
	gpioc_cmd.data_size = sizeof(data);
	ret = rda_msys_send_cmd(&gpioc_cmd);

	return ret;
}

static ssize_t rdabp_gpio_open_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int value;
	rda_gpioc_op gpioc_op;

    gpioc_op.id = 27;
	gpioc_op.value = 1;
	gpioc_op.default_value1 = 0;
	gpioc_op.default_value2 = 0;
	rda_gpioc_operation(&gpioc_op);
	
	return 1;
}

static ssize_t rdabp_gpio_close_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	return count;
}


static ssize_t rdabp_gpio_set_io_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;
	rda_gpioc_op gpioc_op;

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;
    gpioc_op.id = 8;
	gpioc_op.value = 1;
	gpioc_op.default_value1 = 0;
	gpioc_op.default_value2 = 0;
	rda_gpioc_operation(&gpioc_op);
	
	return 1;
}


static ssize_t rdabp_gpio_get_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	return count;
}


static ssize_t rdabp_gpio_set_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;

	return count;
}

static ssize_t rdabp_gpio_enable_irq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	printk(KERN_DEBUG "%s, buf: %s\n", __func__, buf);

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;


	return count;
}

static DEVICE_ATTR(gpio_open, 0777,
		NULL, rdabp_gpio_open_store);
static DEVICE_ATTR(gpio_close,0777,
		NULL, rdabp_gpio_close_store);
static DEVICE_ATTR(gpio_set_io, 0777,
		NULL,rdabp_gpio_set_io_store);
static DEVICE_ATTR(gpio_get_value, 0777,
		NULL,rdabp_gpio_get_value_store);
static DEVICE_ATTR(gpio_set_value,  0777,
		NULL,rdabp_gpio_set_value_store);
static DEVICE_ATTR(gpio_enable_irq, 0777,
		NULL,rdabp_gpio_enable_irq_store);


/*
 * Probe GPIOC on OrangePi 2G-IOT
 */
static int rda_gpioc_platform_probe(struct platform_device *pdev)
{
	struct rda_gpioc_data *gpioc_data = NULL;
	int ret = 0;

	gpioc_data = devm_kzalloc(&pdev->dev, sizeof(struct rda_gpioc_data), GFP_KERNEL);

	if (gpioc_data == NULL) {
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, gpioc_data);

	// ap <---> modem gpioc
	gpioc_data->gpioc_msys = rda_msys_alloc_device();
	if (!gpioc_data->gpioc_msys) {
		ret = -ENOMEM;
	}

	gpioc_data->gpioc_msys->module = SYS_GPIO_MOD;
	gpioc_data->gpioc_msys->name = "rda-gpioc";
	gpioc_data->gpioc_msys->notifier.notifier_call = rda_modem_gpioc_notify;

	rda_msys_register_device(gpioc_data->gpioc_msys);
	bp_gpioc_msys = gpioc_data->gpioc_msys;

	device_create_file(&pdev->dev, &dev_attr_gpio_open);
	device_create_file(&pdev->dev, &dev_attr_gpio_close);
	device_create_file(&pdev->dev, &dev_attr_gpio_set_io);
	device_create_file(&pdev->dev, &dev_attr_gpio_get_value);
	device_create_file(&pdev->dev, &dev_attr_gpio_set_value);
	device_create_file(&pdev->dev, &dev_attr_gpio_enable_irq);

	#ifdef LED_CAM_FLASH
	led_trigger_register_simple(LED_CAM_FLASH, &rda_sensor_led);
	mdelay(5);
	led_trigger_event(rda_sensor_led, LED_HALF);
	#endif

	return ret;
}

static int __exit rda_gpioc_platform_remove(struct platform_device *pdev)
{
	struct rda_gpioc_data *gpioc_data = platform_get_drvdata(pdev);

	rda_msys_unregister_device(gpioc_data->gpioc_msys);
	rda_msys_free_device(gpioc_data->gpioc_msys);

	platform_set_drvdata(pdev, NULL);

	#ifdef LED_CAM_FLASH
	led_trigger_unregister_simple(rda_sensor_led);
	#endif

	return 0;
}

static struct platform_driver rda_gpioc_driver = {
	.driver = {
		.name = "OrangePi_2G-IOT-gpioc",
		.owner = THIS_MODULE,
	},

	.probe = rda_gpioc_platform_probe,
	.remove = __exit_p(rda_gpioc_platform_remove),
};

static int __init rda_gpioc_modinit(void)
{
	int i, j;

	/* Cread debug dir: /sys/class/CLASS_NAME */
	OrangePi_gpio_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(OrangePi_gpio_class))
		return PTR_ERR(OrangePi_gpio_class);
	
	for (i = 0; i < GPIO_NR; i++) {
		OrangePi_pdata[i] = kzalloc(sizeof(struct OrangePi_gpio_pd), GFP_KERNEL);
		if (!OrangePi_pdata[i]) {
			printk(KERN_ERR "No free memory on pdata\n");
			goto faild;
		}

		OrangePi_gpio_dev[i] = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
		if (!OrangePi_gpio_dev[i]) {
			printk(KERN_ERR "No free memory on dev\n");
			goto faild_memory;
		}

		sprintf(OrangePi_pdata[i]->name, "%s", GPIO_name[i]);

		OrangePi_gpio_dev[i]->name = CLASS_NAME;
		OrangePi_gpio_dev[i]->id   = i;
		OrangePi_gpio_dev[i]->dev.platform_data = OrangePi_pdata[i];

		if (platform_device_register(OrangePi_gpio_dev[i])) {
			printk(KERN_ERR "%s platform device register fail\n", OrangePi_pdata[i]->name);
			goto faild_memory2;
		}

	}

	return platform_driver_register(&rda_gpioc_driver);

faild_memory2:
	j = i;
	while (i--)
		kfree(OrangePi_gpio_dev[i]);
	i = j;
faild_memory:
	while (i--)
		kfree(OrangePi_pdata[i]);
faild:
	return -1;
}

static void __exit rda_gpioc_modexit(void)
{
	platform_driver_unregister(&rda_gpioc_driver);
}

module_init(rda_gpioc_modinit);
module_exit(rda_gpioc_modexit);

MODULE_AUTHOR("Buddy.Zhang<buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("OrangePi GPIOC driver");
MODULE_LICENSE("GPL");
