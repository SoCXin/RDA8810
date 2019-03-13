/* ----------------------------------------------------------------------- *
 *
 This file created by albert RDA Inc
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/rtc.h>		/* get the user-level API */
#include <linux/bcd.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/nfs_fs.h>
#include <linux/nfs_fs_sb.h>
#include <linux/nfs_mount.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/tty.h>
#include <linux/syscalls.h>
#include <asm/termbits.h>
#include <linux/serial.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <mach/iomap.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <mach/rda_clk_name.h>
#include <mach/board.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>

#include <rda/tgt_ap_board_config.h>
#include <rda/tgt_ap_gpio_setting.h>
#include "rda_combo.h"
#include <linux/crc16.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>

static struct mutex i2c_rw_lock;
static struct rfkill *wlan_rfkill = NULL;
static struct rfkill *bt_rfkill = NULL;
static struct platform_device *platform_device;
static unsigned short wlan_version = 0;
static struct wake_lock rda_combo_wake_lock;
static struct delayed_work rda_combo_sleep_worker;
struct i2c_client *rda_wifi_core_client = NULL;
struct i2c_client *rda_wifi_rf_client = NULL;
struct i2c_client *rda_bt_core_client = NULL;
struct i2c_client *rda_bt_rf_client = NULL;
struct completion rda_wifi_bt_comp;
struct regulator *combo_reg;

static int bt_host_wake_irq;

static u8 isBigEnded = 0;
static u8 wifi_in_test_mode = 0;
static u8 wifi_role_mode = 0;
static u8 clock_mask_32k = 0;
static u8 clock_mask_26m = 0;
static u8 regulator_mask = 0;
static struct clk *clk32k = NULL;
static struct clk *clk26m = NULL;

/*enable or disable 26m clock regulator */
void enable_26m_regulator(u8 mask)
{
	int ret=0;
	if (regulator_mask & CLOCK_MASK_ALL) {

	} else {
		ret = regulator_enable(combo_reg);
	}
	regulator_mask |= mask;

}

void disable_26m_regulator(u8 mask)
{
	if (regulator_mask & mask) {
		regulator_mask &= ~mask;
		if (regulator_mask & CLOCK_MASK_ALL) {

		} else {
			regulator_disable(combo_reg);
		}
	}
}

void enable_32k_rtc(u8 mask)
{
	if (clock_mask_32k & CLOCK_MASK_ALL) {

	} else {
		clk_prepare_enable(clk32k);
	}
	clock_mask_32k |= mask;
}

void disable_32k_rtc(u8 mask)
{
	if (clock_mask_32k & mask) {
		clock_mask_32k &= ~mask;
		if (clock_mask_32k & CLOCK_MASK_ALL) {

		} else {
			clk_disable_unprepare(clk32k);
		}
	}
}

void enable_26m_rtc(u8 mask)
{
	if (clock_mask_26m & CLOCK_MASK_ALL) {

	} else {
		clk_prepare_enable(clk26m);
	}
	clock_mask_26m |= mask;
}

void disable_26m_rtc(u8 mask)
{
	if (clock_mask_26m & mask) {
		clock_mask_26m &= ~mask;
		if (clock_mask_26m & CLOCK_MASK_ALL) {

		} else {
			clk_disable_unprepare(clk26m);
		}
	}
}

int i2c_write_1_addr_2_data(struct i2c_client *client, const u8 addr,
				const u16 data)
{
	unsigned char DATA[3];
	int ret = 0;
	int retry = 3;

	if (!isBigEnded) {
		DATA[0] = addr;
		DATA[1] = data >> 8;
		DATA[2] = data >> 0;
	} else {
		DATA[0] = addr;
		DATA[1] = data >> 0;
		DATA[2] = data >> 8;
	}

	while (retry--) {
		ret = i2c_master_send(client, (char *)DATA, 3);
		if (ret >= 0) {
			break;
		}
	}

	if (ret < 0) {
		printk(KERN_INFO
			"***i2c_write_1_addr_2_data send:0x%X err:%d bigendia: %d \n",
			addr, ret, isBigEnded);
		return -1;
	} else {
		return 0;
	}

}

int i2c_read_1_addr_2_data(struct i2c_client *client, const u8 addr, u16 * data)
{
	unsigned char DATA[2];
	int ret = 0;
	int retry = 3;

	while (retry--) {
		ret = i2c_master_send(client, (char *)&addr, 1);
		if (ret >= 0) {
			break;
		}
	}

	if (ret < 0) {
		printk(KERN_INFO "***i2c_read_1_addr_2_data send:0x%X err:%d\n",
			addr, ret);
		return -1;
	}

	retry = 3;
	while (retry--) {
		ret = i2c_master_recv(client, DATA, 2);
		if (ret >= 0) {
			break;
		}
	}

	if (ret < 0) {
		printk(KERN_INFO "***i2c_read_1_addr_2_data send:0x%X err:%d\n",
			   addr, ret);
		return -1;
	}

	if (!isBigEnded) {
		*data = (DATA[0] << 8) | DATA[1];
	} else {
		*data = (DATA[1] << 8) | DATA[0];
	}
	return 0;
}

static void wlan_read_version_from_chip(void)
{
	int ret;
	u16 project_id = 0, chip_version = 0;

	if (wlan_version != 0 || !rda_wifi_rf_client)
		return;

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);
	if (ret)
		goto err;

	ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x21, &chip_version);
	if (ret)
		goto err;

	ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x20, &project_id);
	if (ret)
		goto err;

	if (project_id == 0x5990) {
		if (chip_version == 0x47)
			wlan_version = WLAN_VERSION_90_D;
		else if (chip_version == 0x44 || chip_version == 0x45)
			wlan_version = WLAN_VERSION_90_E;
	} else if (project_id == 0x5991) {
		if (chip_version == 0x44)
			wlan_version = WLAN_VERSION_91;
		else if (chip_version == 0x45)
			wlan_version = WLAN_VERSION_91_E;
		else if (chip_version == 0x46)
			wlan_version = WLAN_VERSION_91_F;
		else if (chip_version == 0x47)
			wlan_version = WLAN_VERSION_91_G;
	}

	if (wlan_version == 0)
		printk("error, unsupported chip version! project_id:0x%x, chip_version:0x%x\n",
			project_id, chip_version);
	else
		printk("read project_id:%x version:%x wlan_version:%x \n", project_id,
			chip_version, wlan_version);
err:
	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);
	return;

}

#ifdef RDA_COMBO_FROM_FIRMWARE
atomic_t wifi_fw_status = ATOMIC_INIT(0);
atomic_t bt_fw_status = ATOMIC_INIT(0);
struct rda_firmware rda_combo_fw_entry = {
	.status = 0,
	.size = 0,
	.num = 0,
};

static inline void rda_combo_firmware_lock(void)
{
	mutex_lock(&(rda_combo_fw_entry.lock));
}
static inline void rda_combo_firmware_unlock(void)
{
	mutex_unlock(&(rda_combo_fw_entry.lock));
}
static inline int rda_combo_get_firmware_status(void)
{
	return rda_combo_fw_entry.status;
}
static inline void rda_combo_set_firmware_status(int val)
{
	rda_combo_fw_entry.status = val;
}
void rda_release_firmware(void)
{
	if (atomic_read(&wifi_fw_status)==0 && atomic_read(&bt_fw_status)==0) {
		rda_combo_firmware_lock();
		if (rda_combo_get_firmware_status() == 1) {
			vfree(rda_combo_fw_entry.data);
			rda_combo_fw_entry.num = 0;
			rda_combo_fw_entry.size = 0;
			rda_combo_set_firmware_status(0);
			printk("rda_combo: release firmware\n");
		}
		rda_combo_firmware_unlock();
	}
}
static int check_firmware_data(const struct firmware *fw_entry)
{
	return crc16(0, fw_entry->data, fw_entry->size);
}
static struct rda_firmware_data_type* rda_find_data_from_firmware(
		const u8 *fw, const char *data_name, int num_flag, u32 size_flag)
{
	struct rda_firmware_data_type * rda_combo_data_type;
	const u8 *rda_combo_data;
	u32 num = 0;
	u32 size = 0;

	rda_combo_data_type = (struct rda_firmware_data_type*)fw;

	while (1) {
		if (rda_combo_data_type == NULL) {
			printk("rda_combo find data error, fw address error!\n");
			return NULL;
		}

		num ++;
		if (num > num_flag) {
			printk("error: could not find data %s!\n",
				data_name);
			return NULL;
		}

		size += (sizeof(struct rda_firmware_data_type) +
			rda_combo_data_type->size);
		if (size > size_flag) {
			printk("error: could not find data %s!\n",
				data_name);
			return NULL;
		}

		rda_combo_data =
			(u8 *)(rda_combo_data_type + 1);

		if (strcmp(rda_combo_data_type->data_name, data_name) == 0)
			break;

		rda_combo_data_type = (struct rda_firmware_data_type *)
			(rda_combo_data + rda_combo_data_type->size);
	}
	if (crc16(0, rda_combo_data, rda_combo_data_type->size) !=
				rda_combo_data_type->crc) {
		printk("error: data %s crc error\n", data_name);
			return NULL;
	}
	return rda_combo_data_type;

}
static int rda_firmware_copy_data(const struct firmware *fw_entry,
				int chip_version)
{
	struct rda_device_firmware_head *rda_combo_firmware_head;
	struct rda_firmware_data_type * rda_combo_data_type;
	u32 num = 0;
	u32 size = sizeof(struct rda_device_firmware_head);
	int off_set = 0;

	rda_combo_firmware_head = (struct rda_device_firmware_head *)(fw_entry->data);
	if (strcmp(rda_combo_firmware_head->firmware_type,
					RDA_COMBO_FIRMWARE_NAME) != 0) {
		printk("rda_combo error: firmware data error\n");
		return -1;
	}
	if (rda_combo_firmware_head->version != RDA_FIRMWARE_VERSION) {
		printk("firmware data version error. version %d is needed\n",
			RDA_FIRMWARE_VERSION);
		return -1;
	}

	rda_combo_data_type =
		(struct rda_firmware_data_type*)(rda_combo_firmware_head+1);
	while (1) {
		if (rda_combo_data_type == NULL) {
			printk("rda_combo find data error, fw address error!\n");
			return -1;
		}

		num++;
		if (num > rda_combo_firmware_head->data_num)
			break;

		size += (sizeof(struct rda_firmware_data_type) +
			rda_combo_data_type->size);
		if (size > fw_entry->size) {
			printk("rda_combo error, fw size error!\n");
			return -1;
		}

		if (rda_combo_data_type->chip_version == chip_version ||
			rda_combo_data_type->chip_version == -1) {
			rda_combo_fw_entry.size +=
				sizeof(struct rda_firmware_data_type) +
				rda_combo_data_type->size;
			rda_combo_fw_entry.num++;
		}
		rda_combo_data_type = (struct rda_firmware_data_type *)
			((u8 *)(rda_combo_data_type + 1) +
			rda_combo_data_type->size);
	}

	rda_combo_fw_entry.data = (const u8*)vmalloc(rda_combo_fw_entry.size);
	if (rda_combo_fw_entry.data == NULL) {
		printk("rda_combo:vmalloc faild!\n");
		return -1;
	}

	num = 0;
	size = 0;
	rda_combo_data_type =
		(struct rda_firmware_data_type*)(rda_combo_firmware_head+1);
	while (1) {
		if (rda_combo_data_type == NULL) {
			printk("rda_combo find data error, fw address error!\n");
			return -1;
		}

		num++;
		if (num > rda_combo_firmware_head->data_num)
			break;

		size += (sizeof(struct rda_firmware_data_type) +
			rda_combo_data_type->size);
		if (size > fw_entry->size) {
			printk("rda_combo error, fw size error!\n");
			return -1;
		}

		if (rda_combo_data_type->chip_version == chip_version ||
			rda_combo_data_type->chip_version == -1) {
			memcpy((void *)(rda_combo_fw_entry.data+off_set),
				(const void *)rda_combo_data_type,
				rda_combo_data_type->size +
				sizeof(struct rda_firmware_data_type));
			off_set += rda_combo_data_type->size +
				sizeof(struct rda_firmware_data_type);
		}

		rda_combo_data_type = (struct rda_firmware_data_type *)
			((u8 *)(rda_combo_data_type + 1) +
			rda_combo_data_type->size);
	}

	return 0;
}

static int rda_prepare_data_from_firmware(struct i2c_client* client)
{
	int ret;
	u32 version = rda_wlan_version();
	const struct firmware *fw_entry;
	struct device * device = &(client->dev);
	ret = request_firmware(&fw_entry,
		RDA_COMBO_FIRMWARE_NAME,  device);
	if (ret) {
		printk("Request firmware: request firmware failed\n");
		return ret;
	}
	if (check_firmware_data(fw_entry)) {
		printk("firmware data crc check error\n");
		ret = -1;
		goto out;
	}

	switch (version) {
	case WLAN_VERSION_90_D:
	case WLAN_VERSION_90_E:
		ret = rda_firmware_copy_data(fw_entry, WLAN_VERSION_90_D);
		break;
	case WLAN_VERSION_91:
		ret = rda_firmware_copy_data(fw_entry, WLAN_VERSION_91);
		break;
	case WLAN_VERSION_91_E:
		ret = rda_firmware_copy_data(fw_entry, WLAN_VERSION_91_E);
		break;
	case WLAN_VERSION_91_F:
		ret = rda_firmware_copy_data(fw_entry, WLAN_VERSION_91_F);
		break;
	case WLAN_VERSION_91_G:
		ret = rda_firmware_copy_data(fw_entry, WLAN_VERSION_91_G);
		break;
	default:
		printk("rda_combo: error chip version!\n");
		ret = -1;
		goto out;
	}
	if (ret == 0)
		printk("rda_combo: prepare data from firmware successfully!\n");
	else {
		printk("rda_combo: prepare data from firmware error!\n");
		ret = -1;
		goto out;
	}
out:
	release_firmware(fw_entry);
	return ret;

}
int rda_write_data_to_rf_from_firmware(struct i2c_client* client,
						char *data_name)
{
	int ret = 0;
	struct rda_firmware_data_type * rda_combo_data_type;
	const u8 *rda_combo_data;

	rda_combo_firmware_lock();
	if (rda_combo_get_firmware_status() == 0) {
		ret = rda_prepare_data_from_firmware(client);
		if (ret < 0) {
			printk("rda_comb error: prepare array_data error\n");
			goto out;
		}
		rda_combo_set_firmware_status(1);
	}

	rda_combo_data_type = rda_find_data_from_firmware(
		rda_combo_fw_entry.data, data_name,
		rda_combo_fw_entry.num, rda_combo_fw_entry.size);
	if (rda_combo_data_type == NULL) {
		printk("rda_write_data_to_rf_from_firmware failed\n");
		ret = -1;
		goto out;
	}

	rda_combo_data = (const u8 *)(rda_combo_data_type+1);

	ret = rda_i2c_write_data_to_rf(client,
		(const u16 (*)[2])rda_combo_data, rda_combo_data_type->size / 4);

out:
	rda_combo_firmware_unlock();
	return ret;
}
#endif

int rda_i2c_write_data_to_rf(struct i2c_client *client, const u16(*data)[2],
			 u32 count)
{
	int ret = 0;
	u32 i = 0;

	for (i = 0; i < count; i++) {
		if (data[i][0] == I2C_DELAY_FLAG) {
			msleep(data[i][1]);
			continue;
		}
		ret = i2c_write_1_addr_2_data(client, data[i][0], data[i][1]);
		if (ret < 0)
			break;
	}
	return ret;
}

u32 rda_wlan_version(void)
{
	if(wlan_version == 0)
		wlan_read_version_from_chip();
	return wlan_version;
}

static int rda_wifi_rf_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	int result = 0;

	rda_wifi_rf_client = client;
	printk("rda_wifi_rf_probe \n");
	return result;
}

static int rda_wifi_rf_remove(struct i2c_client *client)
{
	return 0;
}

static int rda_wifi_rf_detect(struct i2c_client *client,
				struct i2c_board_info *info)
{
	strcpy(info->type, RDA_WIFI_RF_I2C_DEVNAME);
	return 0;
}

static const struct i2c_device_id wifi_rf_i2c_id[] ={
	{RDA_WIFI_RF_I2C_DEVNAME, RDA_I2C_CHANNEL},
	{}
};

static struct i2c_driver rda_wifi_rf_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = rda_wifi_rf_probe,
	.remove = rda_wifi_rf_remove,
	.detect = rda_wifi_rf_detect,
	.driver.name = RDA_WIFI_RF_I2C_DEVNAME,
	.id_table = wifi_rf_i2c_id,
};

static int rda_wifi_core_detect(struct i2c_client *client,
				struct i2c_board_info *info)
{
	strcpy(info->type, RDA_WIFI_CORE_I2C_DEVNAME);
	return 0;
}

static int rda_wifi_core_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int result = 0;

	rda_wifi_core_client = client;
	printk("rda_wifi_core_probe \n");
	return result;
}

static int rda_wifi_core_remove(struct i2c_client *client)
{
	return 0;
}

int rda_wifi_power_off(void)
{
	if (wlan_version == WLAN_VERSION_90_D
		|| wlan_version == WLAN_VERSION_90_E)
		return rda_5990_wifi_power_off();
	else if (wlan_version == WLAN_VERSION_91)
		return rda_5991_wifi_power_off();
	else if (wlan_version == WLAN_VERSION_91_E)
		return rda_5991e_wifi_power_off();
	else if (wlan_version == WLAN_VERSION_91_F)
		return rda_5991f_wifi_power_off();
	else if (wlan_version == WLAN_VERSION_91_G)
		return rda_5991g_wifi_power_off();
	return -1;
}

int rda_wifi_power_on(void)
{
	if (wlan_version == WLAN_VERSION_90_D
			|| wlan_version == WLAN_VERSION_90_E) {
		return rda_5990_wifi_power_on();
	} else if (wlan_version == WLAN_VERSION_91)
		return rda_5991_wifi_power_on();
	else if (wlan_version == WLAN_VERSION_91_E)
		return rda_5991e_wifi_power_on();
	else if (wlan_version == WLAN_VERSION_91_F)
		return rda_5991f_wifi_power_on();
	else if (wlan_version == WLAN_VERSION_91_G)
		return rda_5991g_wifi_power_on();
	return -1;
}

static void rda_wifi_shutdown(struct i2c_client *client)
{
	printk("rda_wifi_shutdown \n");
#ifdef RDA_COMBO_FROM_FIRMWARE
	if(atomic_read(&wifi_fw_status))
#endif
		rda_wifi_power_off();
}

static const struct i2c_device_id wifi_core_i2c_id[] = {
	{RDA_WIFI_CORE_I2C_DEVNAME, RDA_I2C_CHANNEL},
	{}
};
static struct i2c_driver rda_wifi_core_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = rda_wifi_core_probe,
	.remove = rda_wifi_core_remove,
	.detect = rda_wifi_core_detect,
	.shutdown = rda_wifi_shutdown,
	.driver.name = RDA_WIFI_CORE_I2C_DEVNAME,
	.id_table = wifi_core_i2c_id,
};

static int rda_bt_rf_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	int result = 0;

	rda_bt_rf_client = client;
	printk("rda_bt_rf_probe \n");
	return result;
}

static int rda_bt_rf_remove(struct i2c_client *client)
{
	rda_bt_rf_client = NULL;
	return 0;
}

static int rda_bt_rf_detect(struct i2c_client *client,
				struct i2c_board_info *info)
{
	strcpy(info->type, RDA_BT_RF_I2C_DEVNAME);
	return 0;
}

static const struct i2c_device_id bt_rf_i2c_id[] = {
	{RDA_BT_RF_I2C_DEVNAME, RDA_I2C_CHANNEL},
	{}
};
static struct i2c_driver rda_bt_rf_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = rda_bt_rf_probe,
	.remove = rda_bt_rf_remove,
	.detect = rda_bt_rf_detect,
	.driver.name = RDA_BT_RF_I2C_DEVNAME,
	.id_table = bt_rf_i2c_id,
};

static int rda_bt_core_detect(struct i2c_client *client,
				  struct i2c_board_info *info)
{
	strcpy(info->type, RDA_BT_CORE_I2C_DEVNAME);
	return 0;
}

void rda_combo_set_wake_lock(void);

#ifdef CONFIG_BLUEZ_SUPPORT
extern void hci_bt_wakeup_host(void);
#endif

static irqreturn_t rda_bt_host_wake_eirq_handler(int irq, void *dev_id)
{
#ifdef CONFIG_BLUEZ_SUPPORT
	hci_bt_wakeup_host();
#endif
	rda_combo_set_wake_lock();
	return IRQ_HANDLED;
}

static int rda_bt_core_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	int result = 0;
	rda_bt_core_client = client;
	printk("rda_bt_core_probe\n");
	return result;
}

static int rda_bt_core_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id bt_core_i2c_id[] ={
	{RDA_BT_CORE_I2C_DEVNAME, RDA_I2C_CHANNEL},
	{}
};

static struct i2c_driver rda_bt_core_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = rda_bt_core_probe,
	.remove = rda_bt_core_remove,
	.detect = rda_bt_core_detect,
	.driver.name = RDA_BT_CORE_I2C_DEVNAME,
	.id_table = bt_core_i2c_id,
};

#ifdef CONFIG_BT_RANDADDR
extern void bt_get_random_address(char *buf);
#endif

static int rda_combo_i2c_ops(unsigned long arg)
{
	int ret = 0, argc = 0;
	u8 cmd[256], *argv[5], *pos, rw = 0, addr = 0, pageup = 0;
	struct i2c_client * i2Client = NULL;
	u16  data = 0;
	void __user *argp = (void __user *)arg;

	if(copy_from_user(cmd, argp, 256))
		return -EFAULT;
	else {
		pos = cmd;
		while (*pos != '\0') {
			if (*pos == '\n') {
				*pos = '\0';
				break;
			}
			pos++;
		}
		argc = 0;
		pos = cmd;
		for (;;) {
			while (*pos == ' ')
				pos++;
			if (*pos == '\0')
				break;
			argv[argc] = pos;
			argc++;
			if (argc == 5)
				break;
			if (*pos == '"') {
				char *pos2 = strrchr(pos, '"');
				if (pos2)
					pos = pos2 + 1;
			}
			while (*pos != '\0' && *pos != ' ')
				pos++;
			if (*pos == ' ')
				*pos++ = '\0';
		}
	}

	if (!memcmp(argv[1], "bt", 2)) {
		i2Client = rda_bt_rf_client;
	} else
		i2Client = rda_wifi_rf_client;

	if (kstrtou8(argv[3], 0, &addr))
		return -EINVAL;

	if (*(argv[2]) == 'r') {
		rw = 0;
	} else {
		rw = 1;
		if (kstrtou16(argv[4], 0, &data))
			return -EINVAL;
	}

	if (addr >= 0x80) {
		i2c_write_1_addr_2_data(i2Client, 0x3F, 0x0001);
		addr -= 0x80;
		pageup = 1;
	}

	if (*(argv[2]) == 'r') {
		int read_data = 0;
		i2c_read_1_addr_2_data(i2Client, addr, &data);
		read_data = (int)data;

		if (copy_to_user(argp, &read_data, sizeof(int)))
			ret = -EFAULT;
	} else
		i2c_write_1_addr_2_data(i2Client, addr, data);

	if (pageup == 1)
		i2c_write_1_addr_2_data(i2Client, 0x3F, 0x0000);

	printk("wlan: %s %s %s %s :0x%x \n",
		argv[0], argv[1], argv[2], argv[3], data);
	return ret;
}


static long rda_combo_pw_ioctl(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case RDA_WLAN_COMBO_VERSION:
		{
			u32 version = rda_wlan_version();
			if (copy_to_user(argp, &version, sizeof(version)))
				ret = -EFAULT;
		}
		break;
	case RDA_WIFI_POWER_SET_TEST_MODE_IOCTL:
		wifi_in_test_mode = 1;
		printk("****set rda wifi in test mode \n");
		break;
	case RDA_WIFI_POWER_CANCEL_TEST_MODE_IOCTL:
		wifi_in_test_mode = 0;
		printk("****set rda wifi in normal mode \n");
		break;
	case RDA_WIFI_STA_MODE_IOCTL:
		wifi_role_mode = 0;
		printk("****set rda wifi in sta mode \n");
		break;
	case RDA_WIFI_SOFTAP_MODE_IOCTL:
		wifi_role_mode = 1;
		printk("****set rda wifi in softap mode \n");
		break;
	case RDA_WIFI_P2P_MODE_IOCTL:
		wifi_role_mode |= 0xA0;
		printk("****set rda wifi in p2p mode \n");
		break;
	case RDA_BT_GET_ADDRESS_IOCTL:
		{
			u8 bt_addr[6] = { 0 };
#ifdef CONFIG_BT_RANDADDR
			bt_get_random_address(bt_addr);
#endif
			printk(KERN_INFO
				"rdabt address[0x%x]:[0x%x]:[0x%x]:[0x%x]:[0x%x]:[0x%x].\n",
				bt_addr[0], bt_addr[1], bt_addr[2], bt_addr[3],
				bt_addr[4], bt_addr[5]);

			if (copy_to_user(argp, &bt_addr[0], sizeof(bt_addr))) {
				ret = -EFAULT;
			}
		}
		break;
	case RDA_COMBO_I2C_OPS:
		ret = rda_combo_i2c_ops(arg);
		break;
	default:
		if (wlan_version == WLAN_VERSION_90_D
			|| wlan_version == WLAN_VERSION_90_E) {
			ret = rda_5990_pw_ioctl(file, cmd, arg);
		} else if (wlan_version == WLAN_VERSION_91)
			ret = rda_5991_pw_ioctl(file, cmd, arg);
		else if (wlan_version == WLAN_VERSION_91_E)
			ret = rda_5991e_pw_ioctl(file, cmd, arg);
		else if (wlan_version == WLAN_VERSION_91_F)
			ret = rda_5991f_pw_ioctl(file, cmd, arg);
		else if (wlan_version == WLAN_VERSION_91_G)
			ret = rda_5991g_pw_ioctl(file, cmd, arg);
		else
			ret = -1;
		break;
	}
	return ret;
}

static int rda_combo_major;
static struct class *rda_combo_class = NULL;
struct device *rda_combo_device;
static const struct file_operations rda_combo_operations = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = rda_combo_pw_ioctl,
	.release = NULL
};

void rda_combo_sleep_worker_task(struct work_struct *work)
{
	printk("---rda_combo_sleep_worker_task end \n");
	wake_unlock(&rda_combo_wake_lock);
}

void rda_combo_set_wake_lock(void)
{
	wake_lock(&rda_combo_wake_lock);
	cancel_delayed_work(&rda_combo_sleep_worker);
	schedule_delayed_work(&rda_combo_sleep_worker, 6 * HZ);
}

static struct platform_driver platform_driver = {
	.driver = {
		   .name = "rda_combo_rfkill_device",
		   .owner = THIS_MODULE,
		}
};

static int wlan_rfkill_set(void *data, bool blocked)
{
	printk("wlan_rfkill_set %d \n", blocked);
	if (blocked) {
		return rda_wifi_power_off();
	} else {
		return rda_wifi_power_on();
	}
}

static int rda_bt_power_off(void)
{
	if (wlan_version == WLAN_VERSION_90_D
		|| wlan_version == WLAN_VERSION_90_E) {
		return rda_5990_bt_power_off();
	} else if (wlan_version == WLAN_VERSION_91)
		return rda_5991_bt_power_off();
	else if (wlan_version == WLAN_VERSION_91_E)
		return rda_5991e_bt_power_off();
	else if (wlan_version == WLAN_VERSION_91_F)
		return rda_5991f_bt_power_off();
	else if (wlan_version == WLAN_VERSION_91_G)
		return rda_5991g_bt_power_off();
	return -1;
}

static int rda_bt_power_on(void)
{
	if (wlan_version == WLAN_VERSION_90_D
		|| wlan_version == WLAN_VERSION_90_E) {
		return rda_5990_bt_power_on();
	} else if (wlan_version == WLAN_VERSION_91)
		return rda_5991_bt_power_on();
	else if (wlan_version == WLAN_VERSION_91_E)
		return rda_5991e_bt_power_on();
	else if (wlan_version == WLAN_VERSION_91_F)
		return rda_5991f_bt_power_on();
	else if (wlan_version == WLAN_VERSION_91_G)
		return rda_5991g_bt_power_on();
	return -1;
}

static const struct rfkill_ops wlan_rfkill_ops = {
	.set_block = wlan_rfkill_set,
};

static int bt_rfkill_set(void *data, bool blocked)
{
	printk("bt_rfkill_set %d \n", blocked);
	if (blocked) {
		return rda_bt_power_off();
	} else {
		return rda_bt_power_on();
	}
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set,
};

#ifdef _TGT_AP_HAVE_GPS
static struct rfkill *gps_rfkill = NULL;
static int rda_gps_power_off(int gpio, int value)
{
	int ret;
	ret = gpio_request(gpio, "gps-on");
	if (ret) {
		printk("Fail to request gpio :%d\n", gpio);
		return -1;
	}
	gpio_direction_output(gpio, value);
	gpio_set_value(gpio, value);
	disable_32k_rtc(CLOCK_GPS);
	gpio_free(gpio);
	printk("Disable GPS GPIO and 32K\n");
	return ret;
}

static int rda_gps_power_on(int gpio, int value)
{
	int ret;
	ret = gpio_request(gpio, "gps-on");
	if (ret) {
		printk("Fail to request gpio :%d\n", gpio);
		return -1;
	}
	gpio_direction_output(gpio, value);
	gpio_set_value(gpio, value);
	enable_32k_rtc(CLOCK_GPS);
	gpio_free(gpio);
	printk("Enable GPS GPIO and 32K\n");
	return ret;
}

static int gps_rfkill_set(void *data, bool blocked)
{
	printk("gps_rfkill_set %d \n", blocked);
	if (blocked) {
		rda_gps_power_off(_TGT_AP_GPIO_GPS_ENABLE, 0);
	} else {
		rda_gps_power_on(_TGT_AP_GPIO_GPS_ENABLE, 1);
	}
	return 0;
}

static const struct rfkill_ops gps_rfkill_ops = {
	.set_block = gps_rfkill_set,
};
#endif
int rda_combo_power_ctrl_init(void)
{
	int ret = 0;

	printk("rda_combo_power_ctrl_init begin\n");
	if (i2c_add_driver(&rda_wifi_core_driver)) {
		printk("rda_wifi_core_driver failed!\n");
		ret = -ENODEV;
		return ret;
	}

	if (i2c_add_driver(&rda_wifi_rf_driver)) {
		printk("rda_wifi_rf_driver failed!\n");
		ret = -ENODEV;
		return ret;
	}

	if (i2c_add_driver(&rda_bt_core_driver)) {
		printk("rda_bt_core_driver failed!\n");
		ret = -ENODEV;
		return ret;
	}

	if (i2c_add_driver(&rda_bt_rf_driver)) {
		printk("rda_bt_rf_driver failed!\n");
		ret = -ENODEV;
		return ret;
	}
#ifdef RDA_COMBO_FROM_FIRMWARE
	mutex_init(&(rda_combo_fw_entry.lock));
#endif
	mutex_init(&i2c_rw_lock);
	INIT_DELAYED_WORK(&rda_combo_sleep_worker,rda_combo_sleep_worker_task);
	wake_lock_init(&rda_combo_wake_lock, WAKE_LOCK_SUSPEND,
				"RDA_sleep_worker_wake_lock");
	rda_combo_major =
		register_chrdev(0, "rdacombo_power_ctrl", &rda_combo_operations);
	if (rda_combo_major < 0) {
		printk(KERN_INFO "register rdacombo_power_ctrl failed!!! \n");
		ret = rda_combo_major;
		goto fail;
	}

	rda_combo_class = class_create(THIS_MODULE, "rda_combo");
	if (IS_ERR(rda_combo_class)) {
		printk(KERN_INFO "create rda_combo_class failed!!! \n");
		ret = PTR_ERR(rda_combo_class);
		goto fail;
	}

	rda_combo_device= device_create(rda_combo_class, NULL, MKDEV(rda_combo_major, 0),
			NULL, "rdacombo");
	if (IS_ERR(rda_combo_device)) {
		printk(KERN_INFO "create rda_combo_device failed!!! \n");
		ret = -ENODEV;
		goto fail;
	}

	combo_reg = regulator_get(NULL, LDO_BT);
	if (IS_ERR(combo_reg)) {
		printk(KERN_INFO "could not find regulator devices\n");
		ret = PTR_ERR(combo_reg);
		goto fail;
	}

	{
		unsigned char *temp = NULL;
		unsigned short testData = 0xffee;
		temp = (unsigned char *)&testData;
		if (*temp == 0xee)
			isBigEnded = 0;
		else
			isBigEnded = 1;
	}

	ret = platform_driver_register(&platform_driver);
	if (ret)
		goto fail;

	platform_device = platform_device_alloc("rda_combo_rfkill_device", -1);
	if (!platform_device) {
		ret = -ENOMEM;
	} else
		ret = platform_device_add(platform_device);

	if (ret)
		goto fail_platform_device;


	wlan_rfkill =
		rfkill_alloc("rda_wlan_rk", &platform_device->dev, RFKILL_TYPE_WWAN,
			 &wlan_rfkill_ops, NULL);
	if (wlan_rfkill) {
		rfkill_init_sw_state(wlan_rfkill, true);
		ret = rfkill_register(wlan_rfkill);
		if (ret < 0)
			goto fail_rfkill_register;
	} else
		printk("rda_wlan_rk failed\n");

	bt_rfkill =
		rfkill_alloc("rda_bt_rk", &platform_device->dev,
			RFKILL_TYPE_BLUETOOTH, &bt_rfkill_ops, NULL);
	if (bt_rfkill) {
		rfkill_init_sw_state(bt_rfkill, true);
		ret = rfkill_register(bt_rfkill);
		if (ret < 0)
			goto fail_rfkill_register;
	} else
		printk("rda_bt_rk failed\n");

#ifdef _TGT_AP_HAVE_GPS
	gps_rfkill =
		rfkill_alloc("rda_gps_rk", &platform_device->dev,
			RFKILL_TYPE_GPS, &gps_rfkill_ops, NULL);
	if (gps_rfkill) {
		rfkill_init_sw_state(gps_rfkill, true);
		ret = rfkill_register(gps_rfkill);
		if (ret < 0)
			goto fail_rfkill_register;
	} else
		printk("rda_gps_rk failed\n");
#endif
	ret = gpio_request(GPIO_BT_HOST_WAKE, "rda_bt_host_wake");
	if (ret) {
		/* this is not fatal */
		printk("Fail to request GPIO for rda_bt_host_wake\n");
		ret = 0;
	} else {
		bt_host_wake_irq = gpio_to_irq(GPIO_BT_HOST_WAKE);
		if (bt_host_wake_irq < 0) {
			ret = -1;
			goto fail_platform_device;
		}

		ret = request_irq(bt_host_wake_irq,
				rda_bt_host_wake_eirq_handler,
				IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
				"rda_bt_host_wake_irq", NULL);
		if (ret)
			goto fail_platform_device;
	}

	clk32k = clk_get(NULL, RDA_CLK_OUT);
	clk26m = clk_get(NULL, RDA_CLK_AUX);


	wlan_read_version_from_chip();
	if (wlan_version == 0) {
		ret = -1;
		goto fail_wland_veriosn;
	}

	init_completion(&rda_wifi_bt_comp);
	complete(&rda_wifi_bt_comp);

	printk("rda_combo_power_ctrl_init end\n");
	return 0;

fail_wland_veriosn:
	disable_32k_rtc(CLOCK_MASK_ALL);
	disable_26m_rtc(CLOCK_MASK_ALL);
	clk_put(clk32k);
	clk_put(clk26m);
	free_irq(bt_host_wake_irq, NULL);
fail_rfkill_register:
fail_platform_device:
	if (wlan_rfkill) {
		rfkill_unregister(wlan_rfkill);
		rfkill_destroy(wlan_rfkill);
	}
	if (bt_rfkill) {
		rfkill_unregister(bt_rfkill);
		rfkill_destroy(bt_rfkill);
	}
#ifdef _TGT_AP_HAVE_GPS
	if (gps_rfkill) {
		rfkill_unregister(gps_rfkill);
		rfkill_destroy(gps_rfkill);
	}
#endif
	if (platform_device)
		platform_device_unregister(platform_device);
	platform_driver_unregister(&platform_driver);

fail:
	if (!IS_ERR(combo_reg)) {
		disable_26m_regulator(CLOCK_MASK_ALL);
		regulator_put(combo_reg);
	}
	if (!IS_ERR(rda_combo_device))
		device_destroy(rda_combo_class, MKDEV(rda_combo_major, 0));
	if (rda_combo_class)
		class_destroy(rda_combo_class);
	if (rda_combo_major >= 0)
		unregister_chrdev(rda_combo_major, "rdacombo_power_ctrl");
	cancel_delayed_work_sync(&rda_combo_sleep_worker);
	wake_lock_destroy(&rda_combo_wake_lock);
	i2c_del_driver(&rda_bt_rf_driver);
	i2c_del_driver(&rda_bt_core_driver);
	i2c_del_driver(&rda_wifi_rf_driver);
	i2c_del_driver(&rda_wifi_core_driver);
	return ret;
}

void rda_combo_power_ctrl_exit(void)
{
	i2c_del_driver(&rda_wifi_core_driver);
	i2c_del_driver(&rda_wifi_rf_driver);
	i2c_del_driver(&rda_bt_core_driver);
	i2c_del_driver(&rda_bt_rf_driver);
	unregister_chrdev(rda_combo_major, "rdacombo_power_ctrl");
	if (rda_combo_class)
		class_destroy(rda_combo_class);

	cancel_delayed_work_sync(&rda_combo_sleep_worker);
	wake_lock_destroy(&rda_combo_wake_lock);
	disable_32k_rtc(CLOCK_MASK_ALL);
	disable_26m_rtc(CLOCK_MASK_ALL);
	clk_put(clk32k);
	clk_put(clk26m);
	if (wlan_rfkill) {
		rfkill_unregister(wlan_rfkill);
		rfkill_destroy(wlan_rfkill);
	}

	if (bt_rfkill) {
		rfkill_unregister(bt_rfkill);
		rfkill_destroy(bt_rfkill);
	}

	free_irq(bt_host_wake_irq, NULL);

	if (platform_device) {
		platform_device_unregister(platform_device);
		platform_driver_unregister(&platform_driver);
	}

	if (!IS_ERR(combo_reg)) {
		disable_26m_regulator(CLOCK_MASK_ALL);
			regulator_put(combo_reg);
	}
}

u8 check_test_mode(void)
{
	return wifi_in_test_mode;
}

u8 check_role_mode(void)
{
	return wifi_role_mode;
}

void rda_combo_i2c_lock(void)
{
	mutex_lock(&i2c_rw_lock);
}

void rda_combo_i2c_unlock(void)
{
	mutex_unlock(&i2c_rw_lock);
}

int rda_fm_power_on(void)
{
	if (wlan_version == WLAN_VERSION_90_D
		|| wlan_version == WLAN_VERSION_90_E) {
		return rda_5990_fm_power_on();
	} else if (wlan_version == WLAN_VERSION_91)
		return rda_5991_fm_power_on();
	else if (wlan_version == WLAN_VERSION_91_E)
		return rda_5991e_fm_power_on();
	else if (wlan_version == WLAN_VERSION_91_F)
		return rda_5991f_fm_power_on();
	else if (wlan_version == WLAN_VERSION_91_G)
		return rda_5991g_fm_power_on();
	return -1;
}

int rda_fm_power_off(void)
{
	if (wlan_version == WLAN_VERSION_90_D
		|| wlan_version == WLAN_VERSION_90_E) {
		return rda_5990_fm_power_off();
	} else if (wlan_version == WLAN_VERSION_91)
		return rda_5991_fm_power_off();
	else if (wlan_version == WLAN_VERSION_91_E)
		return rda_5991e_fm_power_off();
	else if (wlan_version == WLAN_VERSION_91_F)
		return rda_5991f_fm_power_off();
	else if (wlan_version == WLAN_VERSION_91_G)
		return rda_5991g_fm_power_off();
	return -1;

}

EXPORT_SYMBOL(rda_wlan_version);
EXPORT_SYMBOL(check_test_mode);
EXPORT_SYMBOL(check_role_mode);
EXPORT_SYMBOL(rda_combo_set_wake_lock);
EXPORT_SYMBOL(rda_wifi_power_off);
EXPORT_SYMBOL(rda_wifi_power_on);
EXPORT_SYMBOL(rda_fm_power_on);
EXPORT_SYMBOL(rda_fm_power_off);
late_initcall(rda_combo_power_ctrl_init);
module_exit(rda_combo_power_ctrl_exit);
MODULE_LICENSE("GPL");
