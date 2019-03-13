/*
 * drivers/gpu/tegra/tegra_ion.c
 *
 * Copyright (C) 2011 Google, Inc.
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
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>

#include "../ion.h"
#include "../ion_priv.h"

struct ion_device *rda_IonDev;

struct ion_mapper *rda_user_mapper;
int num_heaps;
struct ion_heap **heaps;

#define RDA_ION_CUSTOM_IOC_GET_PHYS     0

struct rda_ion_get_phys_data {
	ion_phys_addr_t addr;
	size_t len;
};

static long rda_ion_custom_ioctl(struct ion_client *client,
	unsigned int cmd, unsigned long arg,
	void *ptr)
{
	switch (cmd) {
	case RDA_ION_CUSTOM_IOC_GET_PHYS:
	{
		struct ion_handle *handle = (struct ion_handle *)arg;
		struct rda_ion_get_phys_data data;
		struct ion_custom_data *pcustom = (struct ion_custom_data *)ptr;
		int ret;

		ret = ion_phys(client, handle, &data.addr, &data.len);
		if (ret) {
			pr_err("%s: get phys failed\n", __func__);
			return ret;
		}

		pcustom->cmd = data.addr;
		pcustom->arg = data.len;

		break;
	}

	default:
		return -ENOTTY;
	}

	return 0;
}

int rda_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i;

	num_heaps = pdata->nr;

	heaps = kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);

	rda_IonDev = ion_device_create(rda_ion_custom_ioctl);
	if (IS_ERR_OR_NULL(rda_IonDev)) {
		kfree(heaps);
		return PTR_ERR(rda_IonDev);
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];

		heaps[i] = ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(heaps[i])) {
			err = PTR_ERR(heaps[i]);
			goto err;
		}
		ion_device_add_heap(rda_IonDev, heaps[i]);
	}
	platform_set_drvdata(pdev, rda_IonDev);

	dev_info(&pdev->dev, "%d heaps created\n",
		num_heaps);

	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (heaps[i])
			ion_heap_destroy(heaps[i]);
	}
	kfree(heaps);
	return err;
}

int rda_ion_remove(struct platform_device *pdev)
{
	struct ion_device *idev = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(idev);
	for (i = 0; i < num_heaps; i++)
		ion_heap_destroy(heaps[i]);
	kfree(heaps);
	rda_IonDev = NULL;
	return 0;
}

static struct platform_driver rda_ion_driver = {
	.probe = rda_ion_probe,
	.remove = rda_ion_remove,
	.driver = { .name = RDA_ION_DRV_NAME }
};

static int __init rda_ion_init(void)
{
	return platform_driver_register(&rda_ion_driver);
}

static void __exit rda_ion_exit(void)
{
	platform_driver_unregister(&rda_ion_driver);
}

module_init(rda_ion_init);
module_exit(rda_ion_exit);

EXPORT_SYMBOL(rda_IonDev);
