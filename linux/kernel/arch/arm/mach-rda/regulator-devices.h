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

#ifndef __REGULATOR_DEVICES_H__
#define __REGULATOR_DEVICES_H__


#include <linux/kernel.h>
#include <linux/regulator/machine.h>

#include <mach/regulator.h>

#ifndef CONFIG_RDA_FPGA
/*internal cfg transfer layer: (customer and user ignore it)
  Macors for standart regulator consumer supply
  in custom cfg don't overrid CONSUMERS_XXX
  just overrid REGU_NAMES_XXX*/

#define CONSUMERS_POWER_CAM	REGULATOR_SUPPLY(LDO_CAM, NULL)
#define CONSUMERS_POWER_USB	REGULATOR_SUPPLY(LDO_USB, NULL)
#define CONSUMERS_POWER_SDMMC	REGULATOR_SUPPLY(LDO_SDMMC, NULL)
#define CONSUMERS_POWER_FM		REGULATOR_SUPPLY(LDO_FM, NULL)
#define CONSUMERS_POWER_BT		REGULATOR_SUPPLY(LDO_BT, NULL)
#define CONSUMERS_POWER_LCD	REGULATOR_SUPPLY(LDO_LCD, NULL)
#define CONSUMERS_POWER_CAM_FLASH	REGULATOR_SUPPLY(LDO_CAM_FLASH, NULL)
#define CONSUMERS_POWER_I2C	REGULATOR_SUPPLY(LDO_I2C, NULL)
#define CONSUMERS_POWER_KEYPAD		REGULATOR_SUPPLY(LDO_KEYPAD, NULL)
#define CONSUMERS_POWER_BACKLIGHT	REGULATOR_SUPPLY(LDO_BACKLIGHT, NULL)
#define CONSUMERS_POWER_LEDR	REGULATOR_SUPPLY(LDO_LEDR, NULL)
#define CONSUMERS_POWER_LEDG	REGULATOR_SUPPLY(LDO_LEDG, NULL)
#define CONSUMERS_POWER_LEDB	REGULATOR_SUPPLY(LDO_LEDB, NULL)
#define CONSUMERS_POWER_VIBRATOR	REGULATOR_SUPPLY(LDO_VIBRATOR, NULL)

struct rda_reg_config {
	struct regulator_init_data *init_data;

	u32 pm_id;
	u16 msys_cmd;
	void *table;
	u16 tsize;
};

struct rda_reg_def {
	int def_val;
};

/*export special init func from regulator driver to regulator machine*/
int rda_regulator_do_init(void * driver_data);

#endif /* CONFIG_RDA_FPGA */

#endif /* __REGULATOR_DEVICES_H__ */

