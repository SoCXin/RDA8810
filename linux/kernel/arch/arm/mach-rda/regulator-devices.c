/*
 * Copyright (C) 2013-2014 RDA Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <plat/md_sys.h>

#include <mach/regulator.h>
#include "regulator-devices.h"


#ifndef CONFIG_RDA_FPGA

static struct regulator_consumer_supply cam_consumers[] = {
	CONSUMERS_POWER_CAM,
};

static struct rda_reg_def cam_default = {
	.def_val = 2800000,
};

static struct regulator_init_data cam_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(cam_consumers),
	.consumer_supplies = cam_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &cam_default,
};

static int cam_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config cam_config = {
	.init_data = &cam_data,
	.pm_id = 1,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)cam_vtable,
	.tsize = ARRAY_SIZE(cam_vtable),
};

static struct regulator_consumer_supply usb_consumers[] = {
	CONSUMERS_POWER_USB,
};

static struct rda_reg_def usb_default = {
	.def_val = 0,
};

static struct regulator_init_data usb_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(usb_consumers),
	.consumer_supplies = usb_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &usb_default,
};

static int usb_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config usb_config = {
	.init_data = &usb_data,
	.pm_id = 6,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)usb_vtable,
	.tsize = ARRAY_SIZE(usb_vtable),
};

static struct regulator_consumer_supply sdmmc_consumers[] = {
	CONSUMERS_POWER_SDMMC,
};

static struct rda_reg_def sdmmc_default = {
	.def_val = 2800000,
};

static struct regulator_init_data sdmmc_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(sdmmc_consumers),
	.consumer_supplies = sdmmc_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &sdmmc_default,
};

static int sdmmc_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config sdmmc_config = {
	.init_data = &sdmmc_data,
	.pm_id = 7,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)sdmmc_vtable,
	.tsize = ARRAY_SIZE(sdmmc_vtable),
};

static struct regulator_consumer_supply fm_consumers[] = {
	CONSUMERS_POWER_FM,
};

static struct rda_reg_def fm_default = {
	.def_val = 1800000,
};

static struct regulator_init_data fm_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(fm_consumers),
	.consumer_supplies = fm_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &fm_default,
};

static int fm_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config fm_config = {
	.init_data = &fm_data,
	.pm_id = 8,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)fm_vtable,
	.tsize = ARRAY_SIZE(fm_vtable),
};

static struct regulator_consumer_supply bt_consumers[] = {
	CONSUMERS_POWER_BT,
};

static struct rda_reg_def bt_default = {
	.def_val = 1800000,
};

static struct regulator_init_data bt_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(bt_consumers),
	.consumer_supplies = bt_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &bt_default,
};

static int bt_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config bt_config = {
	.init_data = &bt_data,
	.pm_id = 10,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)bt_vtable,
	.tsize = ARRAY_SIZE(bt_vtable),
};

static struct regulator_consumer_supply cam_flash_consumers[] = {
	CONSUMERS_POWER_CAM_FLASH,
};

static struct rda_reg_def cam_flash_default = {
	.def_val = 1800000,
};

static struct regulator_init_data cam_flash_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(cam_flash_consumers),
	.consumer_supplies = cam_flash_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &cam_flash_default,
};

static int cam_flash_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config cam_flash_config = {
	.init_data = &cam_flash_data,
	.pm_id = 11,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)cam_flash_vtable,
	.tsize = ARRAY_SIZE(cam_flash_vtable),
};

static struct regulator_consumer_supply lcd_consumers[] = {
	CONSUMERS_POWER_LCD
};

static struct rda_reg_def lcd_default = {
	.def_val = 2800000,
};

static struct regulator_init_data lcd_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(lcd_consumers),
	.consumer_supplies = lcd_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &lcd_default,
};

static int lcd_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config lcd_config = {
	.init_data = &lcd_data,
	.pm_id = 12,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)lcd_vtable,
	.tsize = ARRAY_SIZE(lcd_vtable),
};

static struct regulator_consumer_supply i2c_consumers[] = {
	CONSUMERS_POWER_I2C,
};

static struct rda_reg_def i2c_default = {
	.def_val = 2800000,
};

static struct regulator_init_data i2c_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(i2c_consumers),
	.consumer_supplies = i2c_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &i2c_default,
};

static int i2c_vtable[] = {
	1800000, 2800000
};

static struct rda_reg_config i2c_config = {
	.init_data = &i2c_data,
	.pm_id = 13,
	.msys_cmd = SYS_PM_CMD_EN,
	.table = (void *)i2c_vtable,
	.tsize = ARRAY_SIZE(i2c_vtable),
};

static struct regulator_consumer_supply keypad_consumers[] = {
	CONSUMERS_POWER_KEYPAD,
};

static struct rda_reg_def keypad_default = {
	.def_val = 1800000,
};

static struct regulator_init_data keypad_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(keypad_consumers),
	.consumer_supplies = keypad_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &keypad_default,
};

static int keypad_vtable[] = {
	1800000, 2800000,
};

static struct rda_reg_config keypad_config = {
	.init_data = &keypad_data,
	.pm_id = 1,
	.msys_cmd = SYS_PM_CMD_SET_LEVEL,
	.table = (void *)keypad_vtable,
	.tsize = ARRAY_SIZE(keypad_vtable),
};

static struct regulator_consumer_supply backlight_consumers[] = {
	CONSUMERS_POWER_BACKLIGHT,
};

static struct rda_reg_def backlight_default = {
	.def_val = 10000,
};

static struct regulator_init_data backlight_data = {
	.constraints = {
		.min_uV = 10000,
		.max_uV = 2560000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(backlight_consumers),
	.consumer_supplies = backlight_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &backlight_default,
};

static int backlight_table[POWER_BL_NUM] = {0};

static struct rda_reg_config backlight_config = {
	.init_data = &backlight_data,
	.pm_id = 2,
	.msys_cmd = SYS_PM_CMD_SET_LEVEL,
	.table = (void *)backlight_table,
	.tsize = ARRAY_SIZE(backlight_table),
};

static struct regulator_consumer_supply ledr_consumers[] = {
	CONSUMERS_POWER_LEDR,
};

static struct rda_reg_def ledr_default = {
	.def_val = 10000,
};

static struct regulator_init_data ledr_data = {
	.constraints = {
		.min_uV = 10000,
		.max_uV = 2560000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(ledr_consumers),
	.consumer_supplies = ledr_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &ledr_default,
};

static int ledr_vtable[POWER_LED_NUM] = {0};

static struct rda_reg_config ledr_config = {
	.init_data = &ledr_data,
	.pm_id = 4,
	.msys_cmd = SYS_PM_CMD_SET_LEVEL,
	.table = (void *)ledr_vtable,
	.tsize = ARRAY_SIZE(ledr_vtable),
};

static struct regulator_consumer_supply ledg_consumers[] = {
	CONSUMERS_POWER_LEDG,
};

static struct rda_reg_def ledg_default = {
	.def_val = 10000,
};

static struct regulator_init_data ledg_data = {
	.constraints = {
		.min_uV = 10000,
		.max_uV = 2560000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(ledg_consumers),
	.consumer_supplies = ledg_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &ledg_default,
};

static int ledg_vtable[POWER_LED_NUM] = {0};

static struct rda_reg_config ledg_config = {
	.init_data = &ledg_data,
	.pm_id = 5,
	.msys_cmd = SYS_PM_CMD_SET_LEVEL,
	.table = (void *)ledg_vtable,
	.tsize = ARRAY_SIZE(ledg_vtable),
};

static struct regulator_consumer_supply ledb_consumers[] = {
	CONSUMERS_POWER_LEDB
};

static struct rda_reg_def ledb_default = {
	.def_val = 10000,
};

static struct regulator_init_data ledb_data = {
	.constraints = {
		.min_uV = 10000,
		.max_uV = 2560000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(ledb_consumers),
	.consumer_supplies = ledb_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &ledb_default,
};

static int ledb_vtable[POWER_LED_NUM] = {0};

static struct rda_reg_config ledb_config = {
	.init_data = &ledb_data,
	.pm_id = 6,
	.msys_cmd = SYS_PM_CMD_SET_LEVEL,
	.table = (void *)ledb_vtable,
	.tsize = ARRAY_SIZE(ledb_vtable),
};

static struct regulator_consumer_supply vibrator_consumers[] = {
	CONSUMERS_POWER_VIBRATOR,
};

static struct rda_reg_def vibrator_default = {
	.def_val = 1800000,
};

static struct regulator_init_data vibrator_data = {
	.constraints = {
		.min_uV = 1800000,
		.max_uV = 2800000,
		.valid_modes_mask = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE,
	},

	.num_consumer_supplies = ARRAY_SIZE(vibrator_consumers),
	.consumer_supplies = vibrator_consumers,

	.regulator_init = rda_regulator_do_init,
	.driver_data = &vibrator_default,
};

static int vibrator_vtable[] = {
	1800000, 2800000,
};

static struct rda_reg_config vibrator_config = {
	.init_data = &vibrator_data,
	.pm_id = 8,
	.msys_cmd = SYS_PM_CMD_SET_LEVEL,
	.table = (void *)vibrator_vtable,
	.tsize = ARRAY_SIZE(vibrator_vtable),
};


#define REGULATOR_DEV(_id_, _data_) \
	{ \
		.name = "regulator-rda", \
		.id = (_id_), \
		.dev = { \
			.platform_data = &(_data_), \
		}, \
	}

#define REGULATOR_DEBUG_DEV(_id_, _name_) \
	{ \
		.name = "reg-virt-consumer", \
		.id = (_id_), \
		.dev = { \
			.platform_data = (_name_), \
		}, \
	}

static struct platform_device regulator_devices[] = {

	REGULATOR_DEV(rda_ldo_cam,	cam_config),
	REGULATOR_DEV(rda_ldo_usb,	usb_config),
	REGULATOR_DEV(rda_ldo_sdmmc,	sdmmc_config),
	REGULATOR_DEV(rda_ldo_fm,	fm_config),
	REGULATOR_DEV(rda_ldo_bt,	bt_config),
	REGULATOR_DEV(rda_ldo_lcd,	lcd_config),
	REGULATOR_DEV(rda_ldo_cam_flash,	cam_flash_config),
	REGULATOR_DEV(rda_ldo_i2c,	i2c_config),
	REGULATOR_DEV(rda_ldo_keypad,	keypad_config),
	REGULATOR_DEV(rda_ldo_backlight,	backlight_config),
	REGULATOR_DEV(rda_ldo_ledr,	ledr_config),
	REGULATOR_DEV(rda_ldo_ledg,	ledg_config),
	REGULATOR_DEV(rda_ldo_ledb,	ledb_config),
	REGULATOR_DEV(rda_ldo_vibrator,	vibrator_config),
};

void __init regulator_add_devices(void)
{
	int i = 0;
	int j;
	struct rda_reg_config *pconfig;
	int *table;

	for (i = 0 ; i < ARRAY_SIZE(regulator_devices); i++) {
		if (regulator_devices[i].id == rda_ldo_backlight ||
			regulator_devices[i].id == rda_ldo_ledr ||
			regulator_devices[i].id == rda_ldo_ledg ||
			regulator_devices[i].id == rda_ldo_ledb) {
			pconfig = (struct rda_reg_config *)regulator_devices[i].dev.platform_data;
			table = (int *)pconfig->table;

			for (j = 0; j < POWER_BL_NUM; j++) {
				/* Init a set of pseudo-value, from 10000 mV to 2560000 mV. */
				table[j] = (j + 1) * 10000;
			}
		}

		platform_device_register(&(regulator_devices[i]));
	}
}
#else

void __init regulator_add_devices(void)
{
	return;
}

#endif /* CONFIG_RDA_FPGA */

