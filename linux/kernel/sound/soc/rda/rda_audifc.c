/*
 * aud_ifc.c  --  AUDIO IFC interface for the RDA SoC
 *
 * Copyright (C) 2012 RDA Microelectronics (Beijing) Co., Ltd.
 *
 * Contact: Xu Mingliang <mingliangxu@rdamicro.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
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

#include "rda_bb_ifc.h"
#include "rda_audifc.h"

static HWP_BB_IFC_T __iomem *hwp_audIfc;

static struct rda_audifc_ch *audifc_ch[RDA_AUDIFC_QTY];

static int irq[2];

static irqreturn_t rda_audifc_record_irq_handler(int irq, void *channel)
{

	struct rda_audifc_ch *ptr_ch = (struct rda_audifc_ch *)channel;
	u32 reg;

	if(!ptr_ch || ptr_ch->inuse == IFC_NOUSE)
		return IRQ_NONE;

	reg = hwp_audIfc->ch[RDA_AUDIFC_RECORD].status 
			& (BB_IFC_CAUSE_IEF | BB_IFC_CAUSE_IHF | BB_IFC_CAUSE_I4F | BB_IFC_CAUSE_I3_4F);

	hwp_audIfc->ch[RDA_AUDIFC_RECORD].int_clear = reg;

	if(ptr_ch->callback == NULL)
		return IRQ_NONE;

	if (reg & BB_IFC_CAUSE_I4F) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_QUARTER_IRQ,
				 ptr_ch->data);
		return IRQ_HANDLED;
	} else if (reg & BB_IFC_CAUSE_IHF) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_HALF_IRQ, ptr_ch->data);
		return IRQ_HANDLED;
	} else if (reg & BB_IFC_CAUSE_I3_4F) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_THREE_QUARTER_IRQ,
				 ptr_ch->data);
		return IRQ_HANDLED;
	} else if (reg & BB_IFC_CAUSE_IEF) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_END_IRQ, ptr_ch->data);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static irqreturn_t rda_audifc_play_irq_handler(int irq, void *channel)
{

	struct rda_audifc_ch *ptr_ch = (struct rda_audifc_ch *)channel;
	u32 reg;

	if(!ptr_ch || ptr_ch->inuse == IFC_NOUSE)
		return IRQ_NONE;

	reg = hwp_audIfc->ch[RDA_AUDIFC_PLAY].status 
			& (BB_IFC_CAUSE_IEF | BB_IFC_CAUSE_IHF | BB_IFC_CAUSE_I4F | BB_IFC_CAUSE_I3_4F);

	hwp_audIfc->ch[RDA_AUDIFC_PLAY].int_clear = reg;

	if(ptr_ch->callback == NULL)
		return IRQ_NONE;

	if (reg & BB_IFC_CAUSE_I4F) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_QUARTER_IRQ,
				 ptr_ch->data);
		return IRQ_HANDLED;
	} else if (reg & BB_IFC_CAUSE_IHF) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_HALF_IRQ, ptr_ch->data);
		return IRQ_HANDLED;
	} else if (reg & BB_IFC_CAUSE_I3_4F) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_THREE_QUARTER_IRQ,
				 ptr_ch->data);
		return IRQ_HANDLED;
	} else if (reg & BB_IFC_CAUSE_IEF) {
		ptr_ch->callback(ptr_ch->id, RDA_AUDIFC_END_IRQ, ptr_ch->data);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

int rda_set_audifc_params(u8 ch, struct rda_audifc_chan_params *params)
{

	if (hwp_audIfc->ch[ch].status & BB_IFC_ENABLE) {
		printk(KERN_INFO "%s:  audio ifc busy  \n", __func__);
		return RDA_AUDIFC_ERROR;
	}
	// Assert on word alignement
	if (((u32) params->src_addr) % 4 != 0) {
		printk(KERN_INFO "%s: AUDIO IFC transfer start address not aligned: 0x%x", __func__ , (u32)params->src_addr);
		return RDA_AUDIFC_ERROR;
	}
#if 0
	// Size must be a multiple of 32 bytes
	if (((u32) params->xfer_size) % 32 != 0) {
		printk(KERN_INFO "%s: AUDIO IFC transfer size not mult. of 32-bytes: 0x%x", __func__ , (u32)params->xfer_size);
		return RDA_AUDIFC_ERROR;
	}
#endif

	hwp_audIfc->ch[ch].start_addr = (u32) params->src_addr;
	hwp_audIfc->ch[ch].Fifo_Size = params->xfer_size;

	// we need this four ints !!!!
	hwp_audIfc->ch[ch].int_mask = BB_IFC_QUARTER_FIFO |
	    BB_IFC_HALF_FIFO | BB_IFC_THREE_QUARTER_FIFO | BB_IFC_END_FIFO;

	return RDA_AUDIFC_NOERR;
}

audifc_addr_t rda_get_audifc_src_pos(u8 ch)
{
	return (u32) (hwp_audIfc->ch[ch].cur_ahb_addr);
}

audifc_addr_t rda_get_audifc_dst_pos(u8 ch)
{
	return (u32) (hwp_audIfc->ch[ch].cur_ahb_addr);
}

void rda_start_audifc(u8 ch)
{
	enable_irq(irq[ch]);
	hwp_audIfc->ch[ch].control = BB_IFC_ENABLE;
	return;
}

void rda_stop_audifc(u8 ch)
{
	disable_irq_nosync(irq[ch]);
	hwp_audIfc->ch[ch].control = BB_IFC_DISABLE;
	return;
}

void rda_poll_audifc(u8 ch)
{
	return;
}

int rda_request_audifc(int dev_id, const char *dev_name,
		       void (*callback) (int ch, int state, void *data),
		       void *data, int *audifc_ch_out)
{
	/* Clear interrupt flag and disble audifc. */
	if(audifc_ch[dev_id]->inuse == IFC_INUSE)
		rda_stop_audifc(dev_id);

	audifc_ch[dev_id]->id = dev_id;
	audifc_ch[dev_id]->dev_name = dev_name;
	audifc_ch[dev_id]->callback = callback;
	audifc_ch[dev_id]->data = data;
	audifc_ch[dev_id]->inuse = IFC_INUSE;

	*audifc_ch_out = dev_id;

	return 0;
}

void rda_free_audifc(u8 ch)
{
	audifc_ch[ch]->dev_name = NULL;
	audifc_ch[ch]->callback = NULL;
	audifc_ch[ch]->data = NULL;
	audifc_ch[ch]->inuse = IFC_NOUSE;

	return;
}

static int rda_audifc_probe(struct platform_device *pdev)
{
	struct rda_audifc_device_data *audifc_device;
	struct resource *mem;
	//int irq[RDA_AUDIFC_QTY];
	int ret;

	audifc_device = pdev->dev.platform_data;
	if (!audifc_device) {
		dev_err(&pdev->dev,
			"%s: rda audifc initialized without platform data\n",
			__func__);
		return -EINVAL;
	}

	audifc_ch[RDA_AUDIFC_RECORD] = &audifc_device->chan[RDA_AUDIFC_RECORD];
	audifc_ch[RDA_AUDIFC_RECORD]->id = RDA_AUDIFC_RECORD;
	audifc_ch[RDA_AUDIFC_RECORD]->dev_name = NULL;
	audifc_ch[RDA_AUDIFC_RECORD]->callback = NULL;
	audifc_ch[RDA_AUDIFC_RECORD]->data = NULL;

	audifc_ch[RDA_AUDIFC_PLAY] = &audifc_device->chan[RDA_AUDIFC_PLAY];
	audifc_ch[RDA_AUDIFC_PLAY]->id = RDA_AUDIFC_PLAY;
	audifc_ch[RDA_AUDIFC_PLAY]->dev_name = NULL;
	audifc_ch[RDA_AUDIFC_PLAY]->callback = NULL;
	audifc_ch[RDA_AUDIFC_PLAY]->data = NULL;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "%s: no mem resource\n", __func__);
		return -EINVAL;
	}

	irq[RDA_AUDIFC_RECORD] = platform_get_irq(pdev, RDA_AUDIFC_RECORD);
	if (irq[RDA_AUDIFC_RECORD] < 0) {
		return irq[RDA_AUDIFC_RECORD];
	}


	irq[RDA_AUDIFC_PLAY] = platform_get_irq(pdev, RDA_AUDIFC_PLAY);
	if (irq[RDA_AUDIFC_PLAY] < 0) {
		return irq[RDA_AUDIFC_PLAY];
	}


	hwp_audIfc = ioremap(mem->start, resource_size(mem));
	if (!hwp_audIfc) {
		dev_err(&pdev->dev, "%s: ioremap fail\n", __func__);
		return -ENOMEM;
	}
	/*
	 * The flag, IRQF_TRIGGER_RISING, is not for device.
	 * It's only for calling mask_irq function to enable interrupt.
	 */
	ret = request_irq(irq[RDA_AUDIFC_RECORD], rda_audifc_record_irq_handler,
			  0, "rda-audifc",
			  (void *)audifc_ch[RDA_AUDIFC_RECORD]);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: request record irq fail\n", __func__);
		iounmap(hwp_audIfc);
		return ret;
	}
	disable_irq(irq[RDA_AUDIFC_RECORD]);

	ret = request_irq(irq[RDA_AUDIFC_PLAY], rda_audifc_play_irq_handler,
			  0, "rda-audifc", (void *)audifc_ch[RDA_AUDIFC_PLAY]);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: request play irq fail\n", __func__);
		iounmap(hwp_audIfc);
		return ret;
	}
	disable_irq(irq[RDA_AUDIFC_PLAY]);
	printk(KERN_INFO"audio ifc : initialized .\n");

	return 0;
}

static int rda_audifc_remove(struct platform_device *pdev)
{
	struct resource *io;
	//int irq[2];

	iounmap(hwp_audIfc);
	hwp_audIfc = NULL;

	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(io->start, resource_size(io));

	//free record irq
	irq[RDA_AUDIFC_RECORD] = platform_get_irq(pdev, RDA_AUDIFC_RECORD);
	free_irq(irq[RDA_AUDIFC_RECORD], (void *)audifc_ch[RDA_AUDIFC_RECORD]);

	if (audifc_ch[RDA_AUDIFC_RECORD] != NULL) {
		audifc_ch[RDA_AUDIFC_RECORD]->id = RDA_AUDIFC_RECORD;
		audifc_ch[RDA_AUDIFC_RECORD]->dev_name = NULL;
		audifc_ch[RDA_AUDIFC_RECORD]->callback = NULL;
		audifc_ch[RDA_AUDIFC_RECORD]->data = NULL;
		audifc_ch[RDA_AUDIFC_RECORD]->inuse = IFC_NOUSE;
		audifc_ch[RDA_AUDIFC_RECORD] = NULL;

	}
	//free record irq
	irq[RDA_AUDIFC_PLAY] = platform_get_irq(pdev, RDA_AUDIFC_PLAY);
	free_irq(irq[RDA_AUDIFC_PLAY], (void *)audifc_ch[RDA_AUDIFC_PLAY]);

	if (audifc_ch[RDA_AUDIFC_PLAY] != NULL) {

		audifc_ch[RDA_AUDIFC_PLAY]->id = RDA_AUDIFC_PLAY;
		audifc_ch[RDA_AUDIFC_PLAY]->dev_name = NULL;
		audifc_ch[RDA_AUDIFC_PLAY]->callback = NULL;
		audifc_ch[RDA_AUDIFC_PLAY]->data = NULL;
		audifc_ch[RDA_AUDIFC_PLAY]->inuse = IFC_NOUSE;
		audifc_ch[RDA_AUDIFC_PLAY] = NULL;
	}

	return 0;
}

static struct platform_driver rda_audifc_driver = {
	.probe = rda_audifc_probe,
	.remove = rda_audifc_remove,
	.driver = {
		   .name = "rda-audifc"},
};

static int __init rda_audifc_init(void)
{
	return platform_driver_register(&rda_audifc_driver);
}

module_init(rda_audifc_init);

static void __exit rda_audifc_exit(void)
{
	platform_driver_unregister(&rda_audifc_driver);
}

module_exit(rda_audifc_exit);

MODULE_DESCRIPTION("AUDIO IFC for RDA FPGA PCM");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_ALIAS("platform: rda-audifc");
MODULE_LICENSE("GPL");
