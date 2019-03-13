/*
 * comreg0_misc.c - An internal driver for commucating with modem of RDA
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
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <asm/cacheflush.h>

#include <plat/devices.h>
#include <plat/reg_md.h>
#include <plat/rda_md.h>

#define MODEM_CRASH_MAGIC_DEBUG

#define RDA_CR0_STATUS_REG		RDA_MD_CAUSE_REG
#define RDA_CR0_IRQ_ENABLE_REG		RDA_MD_MASK_SET_REG
#define RDA_CR0_IRQ_DISABLE_REG		RDA_MD_MASK_CLR_REG
#define RDA_CR0_CLR_CAUSE_REG		RDA_MD_IT_CLR_REG
#define RDA_CR0_SET_CAUSE_REG		RDA_MD_IT_SET_REG

#define ENABLED_TRIGGERS (DBG_TRIGGER_BIT | MODEM_CRASH_BIT)

struct rda_comreg0_device {
	struct platform_device *pdev;

	void __iomem *base;
#ifdef MODEM_CRASH_MAGIC_DEBUG
	void __iomem *base2;
#endif
	int irq;
	struct sysfs_dirent *sysfs_modem_crash; /* handle for 'modem_crash'
					         * sysfs entry */
	spinlock_t lock;
};

extern struct notifier_block rda_panic_blk;

static irqreturn_t rda_comreg0_interrupt(int irq, void *dev_id)
{
	struct rda_comreg0_device *comreg0 = dev_id;
	volatile unsigned int status = ioread32(comreg0->base + RDA_CR0_STATUS_REG);
	unsigned long flags;

	spin_lock_irqsave(&comreg0->lock, flags);

	if (status & DBG_TRIGGER_BIT) {

		iowrite32(DBG_TRIGGER_BIT, comreg0->base + RDA_CR0_CLR_CAUSE_REG);
#if 0
		/* Unregister our panic reciver. We do not want to see info of background processes. */
		atomic_notifier_chain_unregister(&panic_notifier_list, &rda_panic_blk);
#endif
		/* Sync cache to ram. */
		flush_cache_all();

		spin_unlock_irqrestore(&comreg0->lock, flags);
		/*
		 * Trigger a panic to hung AP.
		 * We can not ensure all traces to be output to console,
		 * because irq handler be called before resume, but we can
		 * sync cache to ram.
		 */
		BUG_ON(1);
	}
	if (status & MODEM_CRASH_BIT) {
		iowrite32(MODEM_CRASH_BIT, comreg0->base + RDA_CR0_CLR_CAUSE_REG);
		if (comreg0->sysfs_modem_crash)
			sysfs_notify_dirent(comreg0->sysfs_modem_crash);
	}
	spin_unlock_irqrestore(&comreg0->lock, flags);

	return IRQ_HANDLED;
}

static ssize_t modem_crash_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct md_mbx_magic magic;
	int cpu;
	const char *s;

	rda_md_get_modem_magic_info(&magic);

	// Magic pattern is:  0x9db09dbX or 0x9db19dbX
	// Where X is:
	//      1:01: wcpu crashed
	//      2:10: xcpu crashed (also 0)
	//      3:11: xcpu and wcpu crashed
	//      4: ... more cpus
	cpu = magic.modem_crashed & 0x0f;
	magic.modem_crashed &= ~0x0001000f;

	if (magic.modem_crashed != MD_MAGIC_MODEM_CRASH_FLAG)
		s = "none";
	else if (cpu == 0 || cpu == 2)
		s = "xcpu";
	else if (cpu == 1)
		s = "wcpu";
	else if (cpu == 3)
		s = "xcpuwcpu";
	else
		s = "unknown";

	return scnprintf(buf, PAGE_SIZE, "%s\n", s);
}

static ssize_t modem_crash_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
	unsigned long n;

	if (kstrtoul(buf, 0, &n))
		return -EINVAL;
	rda_md_set_modem_crash_magic((unsigned int)n);

	return count;
}

static ssize_t magic_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct md_mbx_magic magic;

	rda_md_get_modem_magic_info(&magic);
	return scnprintf(buf, PAGE_SIZE, "0x%08x 0x%08x 0x%08x 0x%08x\n",
		magic.sys_started,
		magic.modem_crashed,
		magic.fact_update_cmd,
		magic.fact_update_type);
}

#ifdef MODEM_CRASH_MAGIC_DEBUG
struct rda_comreg0_device *hackme = NULL;
static ssize_t magic_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
	ssize_t ret;

	ret = modem_crash_store(dev, attr, buf, count);
	if (ret > 0) {
		iowrite32(MODEM_CRASH_BIT+8, hackme->base2 + RDA_CR0_SET_CAUSE_REG);
	}
	return ret;
}
#endif

static struct device_attribute rda_mcd_attrs[] = {
	__ATTR(modem_crash, S_IRUGO | S_IWUSR, modem_crash_show, modem_crash_store),
#ifdef MODEM_CRASH_MAGIC_DEBUG
	__ATTR(magic, S_IRUGO | S_IWUSR, magic_show, magic_store),
#else
	__ATTR_RO(magic),
#endif
};


static int rda_comreg0_probe(struct platform_device *pdev)
{
	struct rda_comreg0_device * comreg0;
	struct resource *mem;
	int irq;
	int ret = 0;
	int i;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (mem == NULL || irq < 0) {
		return -ENXIO;
	}

	comreg0 = kzalloc(sizeof(struct rda_comreg0_device), GFP_KERNEL);
	if (!comreg0) {
		return -ENOMEM;
	}

	comreg0->pdev = pdev;
	comreg0->irq = irq;
#ifdef MODEM_CRASH_MAGIC_DEBUG
	hackme = comreg0;
	comreg0->base2 = ioremap(0x11A0A000, 256);
	if (!comreg0->base2) {
		dev_err(&pdev->dev, "ioremap fail\n");
		ret = -ENOMEM;
		goto ioremap_err;
	}
#endif
	comreg0->base = ioremap(mem->start, resource_size(mem));
	if (!comreg0->base) {
		dev_err(&pdev->dev, "ioremap fail\n");
		ret = -ENOMEM;
		goto ioremap_err;
	}

	for (i = 0; i < ARRAY_SIZE(rda_mcd_attrs); i++) {
		ret = device_create_file(&pdev->dev, &rda_mcd_attrs[i]);
	}
	comreg0->sysfs_modem_crash = sysfs_get_dirent(pdev->dev.kobj.sd, NULL,
	                                              "modem_crash");
	if (!comreg0->sysfs_modem_crash)
		dev_err(&pdev->dev, "sysfs_get_dirent() fail\n");

	spin_lock_init(&comreg0->lock);

	/* We disable the specify irq before request an irq. */
	iowrite32(ENABLED_TRIGGERS, comreg0->base + RDA_CR0_IRQ_DISABLE_REG);

	ret = request_irq(irq, rda_comreg0_interrupt,
			  IRQF_NO_SUSPEND,
			  "rda_comreg0_irq",
			  (void *)comreg0);
	if (ret < 0) {
		dev_err(&pdev->dev, "request irq fail\n");
		goto irq_err;
	}

	/* Enable IRQ */
	iowrite32(ENABLED_TRIGGERS, comreg0->base + RDA_CR0_IRQ_ENABLE_REG);

	dev_info(&pdev->dev, "initialized\n");

	return ret;

irq_err:

	iounmap(comreg0->base);

ioremap_err:

	if (comreg0) {
		kfree(comreg0);
	}

	return ret;
}

static struct platform_driver rda_comreg0_driver = {
	.driver = {
		.name = RDA_COMREG0_DRV_NAME,
	},
};

static int __init rda_comreg0_init(void)
{
	return platform_driver_probe(&rda_comreg0_driver, rda_comreg0_probe);
}
arch_initcall(rda_comreg0_init);

MODULE_AUTHOR("Tao Lei <leitao@rdamicro.com>");
MODULE_DESCRIPTION("RDA ComReg0 driver");
MODULE_LICENSE("GPL");

