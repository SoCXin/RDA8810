/*
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
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <asm/io.h>
#include <plat/reg_md_sysctrl.h>
#include <plat/boot_mode.h>
#include <plat/ap_clk.h>

static HWP_SYS_CTRL_T __iomem *hwp_sysCtrlMd = NULL;
void rda_set_boot_mode(const char *cmd)
{
	if(cmd) {
		if(!strncmp(cmd, "fastboot", 8)) {
			hwp_sysCtrlMd->Reset_Cause |= SYS_CTRL_SW_BOOT_MODE(1<<2);
		} else if (!strncmp(cmd, "recovery", 8)) {
			hwp_sysCtrlMd->Reset_Cause |= SYS_CTRL_SW_BOOT_MODE(1<<3);
		} else if (!strncmp(cmd, "calib", 5)) {
			hwp_sysCtrlMd->Reset_Cause |= SYS_CTRL_SW_BOOT_MODE(1<<4);
		} else if (!strncmp(cmd, "pdl2", 4)) {
			hwp_sysCtrlMd->Reset_Cause |= SYS_CTRL_SW_BOOT_MODE(1<<6);
		}
	}
}

static int rda_droid_bm = 0;

int rda_get_boot_mode(void)
{
	return rda_droid_bm;
}
EXPORT_SYMBOL(rda_get_boot_mode);

static ssize_t rda_boot_mode_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char bm_msg[30] = {'\0'};

	switch(rda_droid_bm) {
	case BM_NORMAL:
		strcpy(bm_msg, "normal");
		break;
	case BM_CHARGER:
		strcpy(bm_msg, "charger");
		break;
	case BM_RECOVERY:
		strcpy(bm_msg, "recovery");
		break;
	case BM_FACTORY:
		strcpy(bm_msg, "factory");
		break;
	}

	return sprintf(buf, "boot mode: %s\n", bm_msg);
}

static ssize_t rda_boot_mode_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	unsigned int value;

	if (sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	hwp_sysCtrlMd->Reset_Cause &= ~SYS_CTRL_SW_BOOT_MODE_MASK;
	hwp_sysCtrlMd->Reset_Cause |= SYS_CTRL_SW_BOOT_MODE(value);
	return n;
}

static int __init androidboot_mode_setup(char *options)
{
	if (!strcmp(options, "normal")) {
		rda_droid_bm = BM_NORMAL;
	} else if (!strcmp(options, "charger")) {
		rda_droid_bm = BM_CHARGER;
	} else if (!strcmp(options, "recovery")) {
		rda_droid_bm = BM_RECOVERY;
	} else if (!strcmp(options, "factory")) {
		rda_droid_bm = BM_FACTORY;
	}

	return 0;
}

__setup("androidboot.mode=", androidboot_mode_setup);

static struct kobj_attribute rda_boot_mode_attr =
	__ATTR(boot_mode, 0664, rda_boot_mode_show, rda_boot_mode_store);

static int __init rda_boot_mode_init(void)
{
	int ret = 0;
	hwp_sysCtrlMd = ioremap(RDA_MD_SYSCTRL_PHYS, RDA_MD_SYSCTRL_SIZE);
	if (hwp_sysCtrlMd == NULL) {
		printk(KERN_INFO"Failed to remap md sysctrl register\n");
		return -ENOMEM;
	}

	ret = sysfs_create_file(kernel_kobj, &rda_boot_mode_attr.attr);
	if (ret) {
		printk(KERN_INFO"sysfs_create_file failed\n");
	}

	return ret;
}

module_init(rda_boot_mode_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RDA Boot Mode Driver");
MODULE_AUTHOR("Jia Shuo<shuojia@rdamicro.com>");
