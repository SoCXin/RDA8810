/***************************************************************
*
* Copyright(C) RDA Micro Company.,2012
* All Rights Reserved. Confidential
*
****************************************************************
*
* Project: RDA
* File Name: arch/arm/plat-rda/dma.c
*
* Author: Jason Tao
* Creation Date: 2012-10-15
*
*****************************************************************
*
* Implementation of functions of DMA
*
*****************************************************************
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <plat/devices.h>
#include <plat/dma.h>

static void __iomem *dma_base;
#define DMA_CHAN_BASE(i) (dma_base + i*0x100)

DEFINE_SPINLOCK(rda_dma_spinlock);
static struct rda_dma_device_data *dma_device;

static inline void __dma_write(u32 val, u32 offset, u8 ch)
{
	__raw_writel(val, DMA_CHAN_BASE(ch) + offset);
}

static inline u32 __dma_read(u32 offset, u8 ch)
{
	u32 val;

	val = __raw_readl(DMA_CHAN_BASE(ch) + offset);
	return val;
}

static irqreturn_t rda_dma_irq_handler(int irq, void *device)
{
	struct rda_dma_device_data *dma_device =
		   (struct rda_dma_device_data *)device;
	u32 reg, i;
	irqreturn_t ret = IRQ_NONE;

	for (i = 0; i < dma_device->ch_num; i++) {
		struct rda_dma_ch *ptr_ch;

		ptr_ch = &dma_device->chan[i];
		reg = __dma_read(RDA_DMA_STATUS_REG, ptr_ch->id);
		if (reg & RDA_DMA_STA_TRANS_DONE) {
			rda_stop_dma(ptr_ch->id);
			if (ptr_ch->callback)
				ptr_ch->callback(ptr_ch->id, ptr_ch->data);
			ret = IRQ_HANDLED;
		}
	}

	return ret;
}
#ifdef RDA_SCATTER_GATHER_DMA
void rda_dma_map_list_params(struct rda_dma_chan_list_params *params,
		enum dma_data_direction dir)
{
	int i;
	dma_addr_t addr;

	for( i = 0; i < params->nr_dma_lists; i++) {
		if (dir == DMA_FROM_DEVICE)
			addr = params->addrs[i].dst_addr;
		else
			addr = params->addrs[i].src_addr;
		dma_sync_single_for_device(NULL, addr,
				params->addrs[i].xfer_size, dir);
	}
}
EXPORT_SYMBOL(rda_dma_map_list_params);

void rda_dma_unmap_list_params(struct rda_dma_chan_list_params *params,
		enum dma_data_direction dir)
{
	int i;
	dma_addr_t addr;

	for( i = 0; i < params->nr_dma_lists; i++) {
		if (dir == DMA_FROM_DEVICE)
			addr = params->addrs[i].dst_addr;
		else
			addr = params->addrs[i].src_addr;
		dma_sync_single_for_cpu(NULL, addr,
				params->addrs[i].xfer_size, dir);
	}
}

EXPORT_SYMBOL(rda_dma_unmap_list_params);
static int rda_dma_set_list_addr(u8 ch, u32 mode, u32 index, struct rda_dma_trans_addr *addr)
{
	dma_addr_t src;
	dma_addr_t dst;
	u32 size;

	src = addr->src_addr;
	dst = addr->dst_addr;
	size = addr->xfer_size;
	/* Sanity checking */
	BUG_ON(!IS_ALIGNED(src, 8));
	BUG_ON(!IS_ALIGNED(dst, 8));

#if 0
	printk(KERN_INFO
	       "[rda_dma] : src = 0x%08x, dst = 0x%08x, size = 0x%x\n", src,
	       dst, size);
#endif /* #if 0 */

	switch (mode) {
	case RDA_DMA_FW_MODE:
		/*
		 * DMA transits data by a word(4 bytes).
		 * The mask is only for NANDFC that supports no more than 2048 words(8192 bytes),
		 * but our address's unit is byte. So we have to mask 0x1FFF.
		 */
		dst &= 0x1FFF;
		break;

	case RDA_DMA_FR_MODE:
		src &= 0x1FFF;
		break;

	case RDA_DMA_NOR_MODE:
		/* Nothing to do */
		break;

	default:
		printk(KERN_ERR "[rda dma] : Invalid mode of dma!\r\n");
		return -EINVAL;
	}
	/* Fill src address */
	__dma_write(src, DMA_LLI1_SRC_REG + index * 12, ch);
	/* Fill dst address */
	__dma_write(dst, DMA_LLI1_DST_REG  + index * 12, ch);
	/* Fill size */
	__dma_write(size, DMA_LLI1_SIZE_REG + index * 12, ch);

	return 0;
}


int rda_set_dma_list_params(u8 ch, struct rda_dma_chan_list_params *params)
{
	dma_addr_t src;
	dma_addr_t dst;
	u32 size;
	u32 mode;
	int i, j;

	if (!params) {
		printk(KERN_ERR "rda dma : Invalid parameter!\n");
		return -EINVAL;
	}

	BUG_ON(params->nr_dma_lists > RDA_SCATTER_GATHER_DMA_LIST);
	__dma_write(params->nr_dma_lists-1, DMA_LLI_CNT_REG, ch);
	src = params->addrs[0].src_addr;
	dst = params->addrs[0].dst_addr;
	size = params->addrs[0].xfer_size;
	/* Sanity checking */
	BUG_ON(!IS_ALIGNED(src, 8));
	BUG_ON(!IS_ALIGNED(dst, 8));

	mode = (params->dma_mode & RDA_DMA_MODE_MASK);

#if 0
	printk(KERN_INFO
	       "[rda_dma] : src = 0x%08x, dst = 0x%08x, size = 0x%x\n", src,
	       dst, size);
#endif /* #if 0 */

	switch (mode) {
	case RDA_DMA_FW_MODE:
		/*
		 * DMA transits data by a word(4 bytes).
		 * The mask is only for NANDFC that supports no more than 2048 words(8192 bytes),
		 * but our address's unit is byte. So we have to mask 0x1FFF.
		 */
		dst &= 0x1FFF;
		break;

	case RDA_DMA_FR_MODE:
		src &= 0x1FFF;
		break;

	case RDA_DMA_NOR_MODE:
		/* Nothing to do */
		break;

	default:
		printk(KERN_ERR "[rda dma] : Invalid mode of dma!\r\n");
		return -EINVAL;
	}
	/* Fill src address */
	__dma_write(src, RDA_DMA_SRC_REG, ch);
	/* Fill dst address */
	__dma_write(dst, RDA_DMA_DST_REG, ch);
	/* Fill size */
	__dma_write(size, RDA_DMA_XFER_SIZE_REG, ch);

	/* If 4 buffers are given, DMA hardware will use buffers by
	** oder 0-3-2-1 
	*/
	j = 1;
	for (i = params->nr_dma_lists-1; i >= 0 ; i--) {
		rda_dma_set_list_addr(ch, mode & RDA_DMA_MODE_MASK,
				j, &params->addrs[i+1]);
		j++;
	}

	if (params->enable_int) {
		/* Set interrupt flag */
		mode |= RDA_DMA_CTL_INT_MASK;
	}
	/* Set ctl flag */
	__dma_write(mode, RDA_DMA_CTL_REG, ch);

	return 0;
}

EXPORT_SYMBOL(rda_set_dma_list_params);
#endif

int rda_set_dma_params(u8 ch, struct rda_dma_chan_params *params)
{
	dma_addr_t src;
	dma_addr_t dst;
	u32 size;
	u32 mode;

	if (!params) {
		printk(KERN_ERR "rda dma : Invalid parameter!\n");
		return -EINVAL;
	}

	src = params->src_addr;
	dst = params->dst_addr;
	size = params->xfer_size;
	/* Sanity checking */
#ifdef CONFIG_ARCH_RDA8810
	BUG_ON(!IS_ALIGNED(src, 8));
	BUG_ON(!IS_ALIGNED(dst, 8));
#endif

	mode = (params->dma_mode & RDA_DMA_MODE_MASK);

#if 0
	printk(KERN_INFO
	       "[rda_dma] : src = 0x%08x, dst = 0x%08x, size = 0x%x\n", src,
	       dst, size);
#endif /* #if 0 */

	switch (mode) {
	case RDA_DMA_FW_MODE:
		/*
		 * DMA transits data by a word(4 bytes).
		 * The mask is only for NANDFC that supports no more than 2048 words(8192 bytes),
		 * but our address's unit is byte. So we have to mask 0x1FFF.
		 */
		dst &= 0x1FFF;
		break;

	case RDA_DMA_FR_MODE:
		src &= 0x1FFF;
		break;

	case RDA_DMA_NOR_MODE:
		/* Nothing to do */
		break;

	default:
		printk(KERN_ERR "[rda dma] : Invalid mode of dma!\r\n");
		return -EINVAL;
	}
#ifdef RDA_SCATTER_GATHER_DMA
	__dma_write(0, DMA_LLI_CNT_REG, ch);
#endif
	/* Fill src address */
	__dma_write(src, RDA_DMA_SRC_REG, ch);
	/* Fill dst address */
	__dma_write(dst, RDA_DMA_DST_REG, ch);
	/* Fill size */
	__dma_write(size, RDA_DMA_XFER_SIZE_REG, ch);

	if (params->enable_int) {
		/* Set interrupt flag */
		mode |= RDA_DMA_CTL_INT_MASK;
	}
	/* Set ctl flag */
	__dma_write(mode, RDA_DMA_CTL_REG, ch);

	return 0;
}


EXPORT_SYMBOL(rda_set_dma_params);

void rda_start_dma(u8 ch)
{
	u32 reg = 0;

	reg = __dma_read(RDA_DMA_CTL_REG, ch);

	reg |= RDA_DMA_CTL_EN;
	__dma_write(reg, RDA_DMA_CTL_REG, ch);

	return;
}

EXPORT_SYMBOL(rda_start_dma);

void rda_stop_dma(u8 ch)
{
	u32 reg = RDA_DMA_CTL_INT_CLE;

	__dma_write(reg, RDA_DMA_CTL_REG, ch);
	return;
}

EXPORT_SYMBOL(rda_stop_dma);

void rda_poll_dma(u8 ch)
{
	u32 reg = 0;

	while (!((reg = __dma_read(RDA_DMA_STATUS_REG, ch)) & RDA_DMA_STA_TRANS_DONE)) {
		cpu_relax();
	}

	return;
}

EXPORT_SYMBOL(rda_poll_dma);

int rda_request_dma(int dev_id, const char *dev_name,
    void (*callback) (u8 ch, void *data), void *data,
    u8 * dma_ch_out)
{
	struct rda_dma_ch *dma_ch;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&rda_dma_spinlock, flags);
	for (i = 0; i< dma_device->ch_num; i++) {
		dma_ch = &dma_device->chan[i];

		if (((1<<i) & dma_device->ch_inuse_bitmask))
			continue;
		dma_device->ch_inuse_bitmask |= 1<<i;
		if (dma_ch->callback == NULL) {
			/*find an unused channel*/
			rda_stop_dma(i);
			dma_ch->dev_name = dev_name;
			dma_ch->callback = callback;
			dma_ch->data = data;
			*dma_ch_out = i;
			break;
		}
	}
	spin_unlock_irqrestore(&rda_dma_spinlock, flags);

	if (i == dma_device->ch_num) {
		BUG_ON(1); //upper layer doesn't check return value
		return -1;
	}
	return 0;
}

EXPORT_SYMBOL(rda_request_dma);

void rda_free_dma(u8 ch)
{
	struct rda_dma_ch *dma_ch;
	unsigned long flags;

	BUG_ON(ch >= dma_device->ch_num);
	spin_lock_irqsave(&rda_dma_spinlock, flags);
	dma_device->ch_inuse_bitmask &= ~(1<<ch);
	dma_ch = &dma_device->chan[ch];
	dma_ch->dev_name = NULL;
	dma_ch->callback = NULL;
	dma_ch->data = NULL;

	spin_unlock_irqrestore(&rda_dma_spinlock, flags);
	return;
}

EXPORT_SYMBOL(rda_free_dma);

static int rda_dma_probe(struct platform_device *pdev)
{
	struct resource *mem;
	struct rda_dma_ch *dma_ch;
	int irq;
	int ret;
	int i;

	dma_device = pdev->dev.platform_data;
	if (!dma_device) {
		dev_err(&pdev->dev, "%s: rda dma initialized without platform data\n", __func__);
		return -EINVAL;
	}
	dma_device->ch_inuse_bitmask = 0;

	for (i = 0; i < dma_device->ch_num; i++) {
		dma_ch = &dma_device->chan[i];
		dma_ch->id = i;
		dma_ch->dev_name = NULL;
		dma_ch->callback = NULL;
		dma_ch->data = NULL;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "%s: no mem resource\n", __func__);
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		return irq;
	}

	dma_base = ioremap(mem->start, resource_size(mem));
	if (!dma_base) {
		dev_err(&pdev->dev, "%s: ioremap fail\n", __func__);
		return -ENOMEM;
	}

	/*
	 * The flag, IRQF_TRIGGER_RISING, is not for device.
	 * It's only for calling mask_irq function to enable interrupt.
	 */
	ret = request_irq(irq, rda_dma_irq_handler,
			  IRQF_DISABLED | IRQF_ONESHOT, "rda_dma",
			  (void *)dma_device);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: request irq fail\n", __func__);
		iounmap(dma_base);
		return ret;
	}

	printk(KERN_INFO "rda dma is initialized!\n");

	return 0;
}

static int rda_dma_remove(struct platform_device *pdev)
{
	struct rda_dma_device_data *dma_device = pdev->dev.platform_data;
	struct rda_dma_ch *chan;
	struct resource *io;
	int irq;
	int i;

	iounmap(dma_base);
	dma_base = NULL;

	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(io->start, resource_size(io));

	irq = platform_get_irq(pdev, 0);
	free_irq(irq, (void *)dma_device);

	for (i = 0; i < dma_device->ch_num; i++) {
		chan = &dma_device->chan[i];
		chan->id = i;
		chan->dev_name = NULL;
		chan->callback = NULL;
		chan->data = NULL;
	}
	return 0;
}

static struct platform_driver rda_dma_driver = {
	.probe = rda_dma_probe,
	.remove = rda_dma_remove,
	.driver = {
		   .name = "rda-dma"},
};

static int __init rda_dma_init(void)
{
	return platform_driver_register(&rda_dma_driver);
}

module_init(rda_dma_init);

static void __exit rda_dma_exit(void)
{
	platform_driver_unregister(&rda_dma_driver);
}

module_exit(rda_dma_exit);

MODULE_DESCRIPTION("RDA DMA DRIVER");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: rda-dma");
MODULE_AUTHOR("RDA Micro");
