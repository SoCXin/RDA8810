/*
 * V4L2 driver for RDA camera host
 *
 * Copyright (C) 2014 Rda electronics, Inc.
 *
 * Contact: Xing Wei <xingwei@rdamicro.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <media/videobuf2-dma-contig.h>

#include <mach/rda_clk_name.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <plat/reg_cam_8810.h>
#include <plat/pm_ddr.h>

#include <rda/tgt_ap_board_config.h>
#include <rda/tgt_ap_clock_config.h>

/* Macros */
#define MAX_BUFFER_NUM		32
#define VID_LIMIT_BYTES		(16 * 1024 * 1024)
#define MAX_SUPPORT_WIDTH	2048
#define MAX_SUPPORT_HEIGHT	2048

#define RDA_CAM_MBUS_PARA	(V4L2_MBUS_MASTER		|\
				V4L2_MBUS_HSYNC_ACTIVE_HIGH	|\
				V4L2_MBUS_HSYNC_ACTIVE_LOW	|\
				V4L2_MBUS_VSYNC_ACTIVE_HIGH	|\
				V4L2_MBUS_VSYNC_ACTIVE_LOW	|\
				V4L2_MBUS_PCLK_SAMPLE_RISING	|\
				V4L2_MBUS_PCLK_SAMPLE_FALLING	|\
				V4L2_MBUS_DATA_ACTIVE_HIGH)

#define RDA_CAM_MBUS_CSI2	V4L2_MBUS_CSI2_LANES		|\
				V4L2_MBUS_CSI2_CONTINUOUS_CLOCK

#define CAM_OUT_MCLK		(_TGT_AP_PLL_BUS_FREQ >> 3)

/* Global Var */
static void __iomem *cam_regs = NULL;
/* Structure */
//static struct tasklet_struct rcam_tasklet;

/* Capture Buffer */
struct cap_buffer {
	struct vb2_buffer vb;
	struct list_head list;
	unsigned int dma_addr;
};

/* RDA camera device */
struct rda_camera_dev {
	struct soc_camera_host soc_host;
	struct soc_camera_device *icd;

	struct list_head cap_buffer_list;
	struct cap_buffer *active;
	struct cap_buffer *next;
	struct vb2_alloc_ctx *alloc_ctx;
	struct delayed_work isr_work;
//	struct v4l2_rect crop_rect;

	int state;
	int sequence;
	wait_queue_head_t vsync_wq;
	spinlock_t lock;

	struct rda_camera_device_data *pdata;

	struct clk *pclk;
	void __iomem *regs;
	unsigned int irq;
};

static const struct soc_mbus_pixelfmt rda_camera_formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_YUYV,
		.name = "Packed YUV422 16 bit",
		.bits_per_sample = 8,
		.packing = SOC_MBUS_PACKING_2X8_PADHI,
		.order = SOC_MBUS_ORDER_LE,
		.layout = SOC_MBUS_LAYOUT_PACKED,
	},
};

/* camera states */
enum {
	CAM_STATE_IDLE = 0,
	CAM_STATE_ONESHOT,
	CAM_STATE_SUCCESS,
};

/* -----------------------------------------------------------------
 * Public functions for sensor
 * -----------------------------------------------------------------*/
void rcam_pdn(bool pdn, bool acth)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)cam_regs;

	if (acth)
		hwp_cam->CTRL &= ~CAMERA_PWDN_POL_INVERT;
	else
		hwp_cam->CTRL |= CAMERA_PWDN_POL_INVERT;

	if (pdn)
		hwp_cam->CMD_SET = CAMERA_PWDN;
	else
		hwp_cam->CMD_CLR = CAMERA_PWDN;
}

void rcam_rst(bool rst, bool acth)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)cam_regs;

	if (acth)
		hwp_cam->CTRL &= ~CAMERA_RESET_POL_INVERT;
	else
		hwp_cam->CTRL |= CAMERA_RESET_POL_INVERT;

	if (rst)
		hwp_cam->CMD_SET = CAMERA_RESET;
	else
		hwp_cam->CMD_CLR = CAMERA_RESET;
}

void rcam_clk(bool out, int freq)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)cam_regs;
	unsigned int val = 0x1;
	int tmp;

	if (out) {
		if (freq == 13)
			val |= 0x1 << 4;
		else if (freq == 26)
			val |= 0x2 << 12;
		else {
			tmp = CAM_OUT_MCLK / freq;
			if (tmp < 2)
				tmp = 2;
			else if (tmp > 17)
				tmp = 17;
			val |= (tmp - 2) << 8;
		}
		hwp_cam->CLK_OUT = val;
	} else {
		hwp_cam->CLK_OUT = 0x3f00;
	}
}

void rcam_config_csi(unsigned int d, unsigned int c,
		unsigned int line, unsigned int flag)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)cam_regs;
	unsigned char d_term_en = (d >> 16) & 0xff;
	unsigned char d_hs_setl = d & 0xff;
	unsigned short c_term_en = c >> 16;
	unsigned short c_hs_setl = c & 0xffff;
	unsigned int frame_line = (line >> 1) & 0x3ff;
	unsigned char ch_sel = flag & 0x1;
	unsigned char avdd = (flag >> 1) & 0x1;

	hwp_cam->CAM_CSI_REG_0 = 0xA0000000 | (frame_line << 8) | d_term_en;
	hwp_cam->CAM_CSI_REG_1 = 0x00020000 | d_hs_setl;
	hwp_cam->CAM_CSI_REG_2 = (c_term_en << 16) | c_hs_setl;
	hwp_cam->CAM_CSI_REG_3 = 0x9E0A0800 | (ch_sel << 20) | (avdd << 11);
	hwp_cam->CAM_CSI_REG_4 = 0xffffffff;
	hwp_cam->CAM_CSI_REG_5 = 0x40dc0200;
	hwp_cam->CAM_CSI_REG_6 = 0x800420ea;
	hwp_cam->CAM_CSI_ENABLE = 1;
}

/* -----------------------------------------------------------------
 * Private functions
 * -----------------------------------------------------------------*/
static void start_dma(struct rda_camera_dev *rcam)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	unsigned int dma_addr = rcam->active->dma_addr;
	unsigned int size = rcam->icd->sizeimage;

	pm_ddr_get(PM_DDR_CAMERA_DMA);
	/* config address & size for next frame */
	hwp_cam->CAM_FRAME_START_ADDR = dma_addr;
	hwp_cam->CAM_FRAME_SIZE = size;

	/* config Camera controller & AXI */
	hwp_cam->CMD_SET = CAMERA_FIFO_RESET;
	hwp_cam->CAM_AXI_CONFIG = CAMERA_AXI_BURST(0xf);//0~15
	/* enable Camera controller & AXI */
	hwp_cam->CTRL |= CAMERA_ENABLE;
	hwp_cam->CAM_AXI_CONFIG |= CAMERA_AXI_START;
}

static void stop_dma(struct rda_camera_dev *rcam)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;

	/* disable Camera controller & AXI */
	hwp_cam->CTRL &= ~CAMERA_ENABLE;
	hwp_cam->CAM_AXI_CONFIG &= ~CAMERA_AXI_START;
	pm_ddr_put(PM_DDR_CAMERA_DMA);
}

static int configure_geometry(struct rda_camera_dev *rcam,
		enum v4l2_mbus_pixelcode code)
{
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		hwp_cam->CTRL |= CAMERA_REORDER_YUYV;
		break;
	case V4L2_MBUS_FMT_YVYU8_2X8:
		hwp_cam->CTRL |= CAMERA_REORDER_YVYU;
		break;
	case V4L2_MBUS_FMT_UYVY8_2X8:
		hwp_cam->CTRL |= CAMERA_REORDER_UYVY;
		break;
	case V4L2_MBUS_FMT_VYUY8_2X8:
		hwp_cam->CTRL |= CAMERA_REORDER_VYUY;
		break;
	default:
		rda_dbg_camera("%s: pixelcode: %x not support\n",
				__func__, code);
		return -EINVAL;
	}
	hwp_cam->CTRL |= CAMERA_DATAFORMAT_YUV422;

	return 0;
}

static void handle_vsync(struct work_struct *wk)
{
	struct delayed_work *dwk = to_delayed_work(wk);
	struct rda_camera_dev *p = container_of(dwk,
			struct rda_camera_dev, isr_work);
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)p->regs;
	unsigned int tc = hwp_cam->CAM_TC_COUNT;

	if (!tc)
		schedule_delayed_work(dwk, msecs_to_jiffies(5));
	else if (p->next) {
		hwp_cam->CAM_FRAME_START_ADDR = p->next->dma_addr;
		rda_dbg_camera("%s: tc: %d, next dma_addr: 0x%x\n",
				__func__, tc, p->next->dma_addr);
	} else
		rda_dbg_camera("%s: p->next is NULL\n", __func__);
}
/*
static void handle_vsync(unsigned long pcam)
{
	struct rda_camera_dev *p = (struct rda_camera_dev*)pcam;
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)p->regs;
	unsigned int tc = hwp_cam->CAM_TC_COUNT;

	if (!tc) {
		tasklet_schedule(&rcam_tasklet);
	} else if (p->next) {
		hwp_cam->CAM_FRAME_START_ADDR = p->next->dma_addr;
		rda_dbg_camera("%s: tc: %d, next dma_addr: 0x%x\n",
				__func__, tc, p->next->dma_addr);
	} else
		rda_dbg_camera("%s: p->next is NULL\n", __func__);
}
*/

static irqreturn_t handle_streaming(struct rda_camera_dev *rcam)
{
	struct cap_buffer *buf = rcam->active;
	struct vb2_buffer *vb;
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	unsigned int tc = hwp_cam->CAM_TC_COUNT;

	if (!buf)
		return IRQ_HANDLED;

	if (tc)
		printk(KERN_ERR "%s: frame size=%d, tc=%d\n",
				__func__, rcam->icd->sizeimage, tc);

	list_del_init(&buf->list);
	if (list_empty(&rcam->cap_buffer_list)) {
		rcam->active = NULL;
		rcam->next = NULL;
	} else {
		rcam->active = list_entry(rcam->cap_buffer_list.next,
				struct cap_buffer, list);
		if (list_is_last(&rcam->active->list, &rcam->cap_buffer_list))
			rcam->next = NULL;
		else
			rcam->next = list_entry(rcam->active->list.next,
					struct cap_buffer, list);
	}

	vb = &buf->vb;
	do_gettimeofday(&vb->v4l2_buf.timestamp);
	vb->v4l2_buf.sequence = rcam->sequence++;
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	return IRQ_HANDLED;
}

static irqreturn_t rda_camera_isr(int irq, void *dev)
{
	struct rda_camera_dev *rcam = dev;
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	unsigned int irq_cause = 0;
	unsigned int state = 0;
	unsigned int addr = 0;
	irqreturn_t ret = IRQ_NONE;
	unsigned long flags = 0;

	spin_lock_irqsave(&rcam->lock, flags);
	irq_cause = hwp_cam->IRQ_CAUSE;
	hwp_cam->IRQ_CLEAR |= irq_cause;
	addr = hwp_cam->CAM_FRAME_START_ADDR;
	state = rcam->state;
	if (irq_cause & IRQ_VSYNC_R) {
		if (rcam->next) {
			schedule_delayed_work(&rcam->isr_work, 0);
//			tasklet_schedule(&rcam_tasklet);
			rcam->state = CAM_STATE_SUCCESS;
		} else
			rcam->state = CAM_STATE_ONESHOT;
		if (state == CAM_STATE_IDLE) {
			wake_up_interruptible(&rcam->vsync_wq);
		}
		ret = IRQ_HANDLED;
	} else if (irq_cause & IRQ_OVFL) {
		rcam->state = CAM_STATE_ONESHOT;
		printk(KERN_ERR "%s: overflow!\n", __func__);
		ret = IRQ_HANDLED;
	} else if (irq_cause & IRQ_VSYNC_F) {
		if (!rcam->active) {
			ret = IRQ_HANDLED;
		} else if ((state == CAM_STATE_ONESHOT) ||
				(addr == rcam->active->dma_addr)) {
			stop_dma(rcam);
			ret = handle_streaming(rcam);
			if (rcam->active)
				start_dma(rcam);
			rcam->state = CAM_STATE_ONESHOT;
			cancel_delayed_work(&rcam->isr_work);
		} else if (state == CAM_STATE_SUCCESS) {
			ret = handle_streaming(rcam);
		}
	}

	spin_unlock_irqrestore(&rcam->lock, flags);

	rda_dbg_camera("%s: cause: %x, addr: 0x%x, state: %d, new state %d\n",
			__func__, irq_cause, addr, state, rcam->state);
	return ret;
}

/* -----------------------------------------------------------------
 * Videobuf operations
 * -----------------------------------------------------------------*/
static int queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
		unsigned int *nbuffers, unsigned int* nplanes,
		unsigned int sizes[], void *alloc_ctxs[])
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	unsigned int size;

	/* May need reset camera host */
	/* TODO: do hardware reset here */
	size = icd->sizeimage;

	if (!*nbuffers || *nbuffers > MAX_BUFFER_NUM)
		*nbuffers = MAX_BUFFER_NUM;

	if (size * *nbuffers > VID_LIMIT_BYTES)
		*nbuffers = VID_LIMIT_BYTES / size;

	*nplanes = 1;
	sizes[0] = size;
	alloc_ctxs[0] = rcam->alloc_ctx;

	rcam->sequence = 0;
	rcam->active = NULL;
	rcam->next = NULL;
	rda_dbg_camera("%s: count=%d, size=%d\n", __func__, *nbuffers, size);

	return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
	struct cap_buffer *buf = container_of(vb, struct cap_buffer, vb);

	INIT_LIST_HEAD(&buf->list);

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vb->vb2_queue);
	struct cap_buffer *buf = container_of(vb, struct cap_buffer, vb);
	unsigned long size;

	size = icd->sizeimage;

	if (vb2_plane_size(vb, 0) < size) {
		rda_dbg_camera("%s: data will not fit into plane(%lu < %lu)\n",
				__func__, vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(&buf->vb, 0, size);

	return 0;
}

static void buffer_cleanup(struct vb2_buffer *vb)
{
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vb->vb2_queue);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	struct cap_buffer *buf = container_of(vb, struct cap_buffer, vb);
	unsigned long flags = 0;

	buf->dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
	spin_lock_irqsave(&rcam->lock, flags);
	list_add_tail(&buf->list, &rcam->cap_buffer_list);

	if (rcam->active == NULL) {
		rcam->active = buf;
		rda_dbg_camera("%s: rcam->active->dma_addr: 0x%x\n",
				__func__, buf->dma_addr);
		if (vb2_is_streaming(vb->vb2_queue)) {
			start_dma(rcam);
		}
	} else if (rcam->next == NULL) {
		rcam->next = buf;
		rda_dbg_camera("%s: rcam->next->dma_addr: 0x%x\n",
				__func__, buf->dma_addr);
	}
	spin_unlock_irqrestore(&rcam->lock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	struct cap_buffer *buf, *node;
	unsigned long flags = 0;
	int ret;

	spin_lock_irqsave(&rcam->lock, flags);
	rcam->state = CAM_STATE_IDLE;
	hwp_cam->IRQ_CLEAR |= IRQ_MASKALL;
	hwp_cam->IRQ_MASK |= IRQ_VSYNC_R | IRQ_VSYNC_F | IRQ_OVFL;
	/* enable camera before wait vsync */
	if (count)
		start_dma(rcam);
	spin_unlock_irqrestore(&rcam->lock, flags);

	rda_dbg_camera("%s: Waiting for VSYNC\n", __func__);
	ret = wait_event_interruptible_timeout(rcam->vsync_wq,
			rcam->state != CAM_STATE_IDLE,
			msecs_to_jiffies(500));
	if (ret == 0) {
		rda_dbg_camera("%s: timed out\n", __func__);
		ret = -ETIMEDOUT;
		goto err;
	} else if (ret == -ERESTARTSYS) {
		rda_dbg_camera("%s: Interrupted by a signal\n", __func__);
		goto err;
	}

	return 0;
err:
	/* Clear & Disable interrupt */
	hwp_cam->IRQ_CLEAR |= IRQ_MASKALL;
	hwp_cam->IRQ_MASK &= ~IRQ_MASKALL;
	stop_dma(rcam);
	rcam->active = NULL;
	rcam->next = NULL;
	list_for_each_entry_safe(buf, node, &rcam->cap_buffer_list, list) {
		list_del_init(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	INIT_LIST_HEAD(&rcam->cap_buffer_list);
	return ret;
}

static int stop_streaming(struct vb2_queue *vq)
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	struct cap_buffer *buf, *node;
	unsigned long flags = 0;

	cancel_delayed_work(&rcam->isr_work);
	flush_scheduled_work();
	spin_lock_irqsave(&rcam->lock, flags);
	/* Clear & Disable interrupt */
	hwp_cam->IRQ_CLEAR |= IRQ_MASKALL;
	hwp_cam->IRQ_MASK &= ~IRQ_MASKALL;
	stop_dma(rcam);
	/* Release all active buffers */
	rcam->active = NULL;
	rcam->next = NULL;
	list_for_each_entry_safe(buf, node, &rcam->cap_buffer_list, list) {
		list_del_init(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	INIT_LIST_HEAD(&rcam->cap_buffer_list);
	spin_unlock_irqrestore(&rcam->lock, flags);

	return 0;
}

static struct vb2_ops rda_video_qops = {
	.queue_setup = queue_setup,
	.buf_init = buffer_init,
	.buf_prepare = buffer_prepare,
	.buf_cleanup = buffer_cleanup,
	.buf_queue = buffer_queue,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
	.wait_prepare = soc_camera_unlock,
	.wait_finish = soc_camera_lock,
};

/* -----------------------------------------------------------------
 * SoC camera operation for the device
 * -----------------------------------------------------------------*/
static int rda_camera_init_videobuf(struct vb2_queue *q,
		struct soc_camera_device *icd)
{
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR;
	q->drv_priv = icd;
	q->buf_struct_size = sizeof(struct cap_buffer);
	q->ops = &rda_video_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	return vb2_queue_init(q);
}

static int rda_camera_try_fmt(struct soc_camera_device *icd,
			      struct v4l2_format *f)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	u32 pixfmt = pix->pixelformat;
	int ret;

	xlate = soc_camera_xlate_by_fourcc(icd, pixfmt);
	if (pixfmt && !xlate) {
		rda_dbg_camera("%s: Format %x not found\n", __func__, pixfmt);
		return -EINVAL;
	}

	/* limit to Atmel ISI hardware capabilities */
	if (pix->height > MAX_SUPPORT_HEIGHT)
		pix->height = MAX_SUPPORT_HEIGHT;
	if (pix->width > MAX_SUPPORT_WIDTH)
		pix->width = MAX_SUPPORT_WIDTH;

	/* limit to sensor capabilities */
	mf.width = pix->width;
	mf.height = pix->height;
	mf.field = pix->field;
	mf.colorspace = pix->colorspace;
	mf.code = xlate->code;

	ret = v4l2_subdev_call(sd, video, try_mbus_fmt, &mf);
	if (ret < 0)
		return ret;

	pix->width = mf.width;
	pix->height = mf.height;
	pix->colorspace = mf.colorspace;

	switch (mf.field) {
	case V4L2_FIELD_ANY:
		pix->field = V4L2_FIELD_NONE;
		break;
	case V4L2_FIELD_NONE:
		break;
	default:
		rda_dbg_camera("%s: Field type %d unsupported.\n",
				__func__, mf.field);
		ret = -EINVAL;
	}

	return ret;
}

static int rda_camera_set_fmt(struct soc_camera_device *icd,
		struct v4l2_format *f)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	int ret;

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		rda_dbg_camera("%s: Format %x not found\n",
				__func__, pix->pixelformat);
		return -EINVAL;
	}

	rda_dbg_camera("%s: Plan to set format %dx%d\n",
			__func__, pix->width, pix->height);

	mf.width = pix->width;
	mf.height = pix->height;
	mf.field = pix->field;
	mf.colorspace = pix->colorspace;
	mf.code = xlate->code;

	ret = v4l2_subdev_call(sd, video, s_mbus_fmt, &mf);
	if (ret < 0 && ret != -ENODEV)
		return ret;

	if (mf.code != xlate->code)
		return -EINVAL;

	ret = configure_geometry(rcam, xlate->code);
	if (ret < 0)
		return ret;

	pix->width = mf.width;
	pix->height = mf.height;
	pix->field = mf.field;
	pix->colorspace = mf.colorspace;
	icd->current_fmt = xlate;

	rda_dbg_camera("%s: Finally set format %dx%d\n",
			__func__, pix->width, pix->height);

	return ret;
}

static int rda_camera_set_bus_param(struct soc_camera_device *icd)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	HWP_CAMERA_T *hwp_cam = (HWP_CAMERA_T*)rcam->regs;
	unsigned int ctrl = hwp_cam->CTRL;
	struct v4l2_mbus_config cfg = {.type = V4L2_MBUS_PARALLEL,};
	unsigned int common_flags = RDA_CAM_MBUS_PARA;
	int ret;

	ret = v4l2_subdev_call(sd, video, g_mbus_config, &cfg);
	if (!ret) {
		if (cfg.type == V4L2_MBUS_CSI2)
			common_flags = RDA_CAM_MBUS_CSI2;
		common_flags = soc_mbus_config_compatible(&cfg,
				common_flags);
		if (!common_flags) {
			rda_dbg_camera("%s: Flags incompatible camera 0x%x, host 0x%x\n",
					__func__, cfg.flags, common_flags);
			return -EINVAL;
		}
	} else if (ret != -ENOIOCTLCMD) {
		return ret;
	}
	rda_dbg_camera("%s: Flags cam: 0x%x common: 0x%x\n",
			__func__, cfg.flags, common_flags);

	if (cfg.type == V4L2_MBUS_PARALLEL) {
		/* Make choises, based on platform preferences */
		if ((common_flags & V4L2_MBUS_HSYNC_ACTIVE_HIGH) &&
				(common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW)) {
			if (rcam->pdata->hsync_act_low)
				common_flags &= ~V4L2_MBUS_HSYNC_ACTIVE_HIGH;
			else
				common_flags &= ~V4L2_MBUS_HSYNC_ACTIVE_LOW;
		}

		if ((common_flags & V4L2_MBUS_VSYNC_ACTIVE_HIGH) &&
				(common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW)) {
			if (rcam->pdata->vsync_act_low)
				common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_HIGH;
			else
				common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_LOW;
		}

		if ((common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING) &&
				(common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
			if (rcam->pdata->pclk_act_falling)
				common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_RISING;
			else
				common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_FALLING;
		}
		/* set bus param for host */
		if (common_flags & V4L2_MBUS_HSYNC_ACTIVE_HIGH)
			ctrl &= ~CAMERA_HREF_POL_INVERT;
		else
			ctrl |= CAMERA_HREF_POL_INVERT;
		if (common_flags & V4L2_MBUS_VSYNC_ACTIVE_HIGH)
			ctrl &= ~CAMERA_VSYNC_POL_INVERT;
		else
			ctrl |= CAMERA_VSYNC_POL_INVERT;
		if (common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING)
			ctrl &= ~CAMERA_PIXCLK_POL_INVERT;
		else
			ctrl |= CAMERA_PIXCLK_POL_INVERT;
		hwp_cam->CTRL = ctrl;
	}

	cfg.flags = common_flags;
	ret = v4l2_subdev_call(sd, video, s_mbus_config, &cfg);
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		rda_dbg_camera("%s: camera s_mbus_config(0x%x) returned %d\n",
				__func__, common_flags, ret);
		return ret;
	}

	return 0;
}

static int rda_camera_try_bus_param(struct soc_camera_device *icd,
		unsigned char buswidth)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct v4l2_mbus_config cfg = {.type = V4L2_MBUS_PARALLEL,};
	unsigned int common_flags = RDA_CAM_MBUS_PARA;
	int ret;

	ret = v4l2_subdev_call(sd, video, g_mbus_config, &cfg);
	if (!ret) {
		if (cfg.type == V4L2_MBUS_CSI2)
			common_flags = RDA_CAM_MBUS_CSI2;
		common_flags = soc_mbus_config_compatible(&cfg,
				common_flags);
		if (!common_flags) {
			rda_dbg_camera("%s: Flags incompatible camera 0x%x, host 0x%x\n",
					__func__, cfg.flags, common_flags);
			return -EINVAL;
		}
	} else if (ret != -ENOIOCTLCMD) {
		return ret;
	}

	return 0;
}

/* This will be corrected as we get more formats */
static bool rda_camera_packing_supported(const struct soc_mbus_pixelfmt *fmt)
{
	return fmt->packing == SOC_MBUS_PACKING_NONE ||
		(fmt->bits_per_sample == 8 &&
		 fmt->packing == SOC_MBUS_PACKING_2X8_PADHI) ||
		(fmt->bits_per_sample > 8 &&
		 fmt->packing == SOC_MBUS_PACKING_EXTEND16);
}

static int rda_camera_get_formats(struct soc_camera_device *icd,
		unsigned int idx,
		struct soc_camera_format_xlate *xlate)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	int formats = 0, ret;
	/* sensor format */
	enum v4l2_mbus_pixelcode code;
	/* soc camera host format */
	const struct soc_mbus_pixelfmt *fmt;

	ret = v4l2_subdev_call(sd, video, enum_mbus_fmt, idx, &code);
	if (ret < 0)
		/* No more formats */
		return 0;

	fmt = soc_mbus_get_fmtdesc(code);
	if (!fmt) {
		rda_dbg_camera("%s: Invalid format code #%u: %d\n",
				__func__, idx, code);
		return 0;
	}

	/* This also checks support for the requested bits-per-sample */
	ret = rda_camera_try_bus_param(icd, fmt->bits_per_sample);
	if (ret < 0) {
		rda_dbg_camera("%s: Fail to try the bus parameters.\n",
				__func__);
		return 0;
	}

	switch (code) {
	case V4L2_MBUS_FMT_UYVY8_2X8:
	case V4L2_MBUS_FMT_VYUY8_2X8:
	case V4L2_MBUS_FMT_YUYV8_2X8:
	case V4L2_MBUS_FMT_YVYU8_2X8:
		formats++;
		if (xlate) {
			xlate->host_fmt = &rda_camera_formats[0];
			xlate->code = code;
			xlate++;
			rda_dbg_camera("%s: Providing format %s using code %d\n",
					__func__, rda_camera_formats[0].name, code);
		}
		break;
	default:
		if (!rda_camera_packing_supported(fmt))
			return 0;
		if (xlate)
			rda_dbg_camera("%s: Providing format %s in pass-through mode\n",
					__func__, fmt->name);
	}

	/* Generic pass-through */
	formats++;
	if (xlate) {
		xlate->host_fmt = fmt;
		xlate->code = code;
		xlate++;
	}

	return formats;
}

/* Called with .host_lock held */
static int rda_camera_add_device(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;
	int ret;

	if (rcam->icd)
		return -EBUSY;

	ret = clk_enable(rcam->pclk);
	if (ret)
		return ret;

	rcam->icd = icd;
	rda_dbg_camera("%s: Camera driver attached to camera %d\n",
			__func__, icd->devnum);
	return 0;
}

/* Called with .host_lock held */
static void rda_camera_remove_device(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct rda_camera_dev *rcam = ici->priv;

	BUG_ON(icd != rcam->icd);

	clk_disable(rcam->pclk);
	rcam->icd = NULL;

	rda_dbg_camera("%s: Camera driver detached from camera %d\n",
			__func__, icd->devnum);
}

static unsigned int rda_camera_poll(struct file *file, poll_table *pt)
{
	struct soc_camera_device *icd = file->private_data;

	return vb2_poll(&icd->vb2_vidq, file, pt);
}

static int rda_camera_querycap(struct soc_camera_host *ici,
		struct v4l2_capability *cap)
{
	strcpy(cap->driver, RDA_CAMERA_DRV_NAME);
	strcpy(cap->card, "RDA Camera Sensor Interface");
	cap->capabilities = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING);
	return 0;
}

static struct soc_camera_host_ops rda_soc_camera_host_ops = {
	.owner		= THIS_MODULE,
	.add		= rda_camera_add_device,
	.remove		= rda_camera_remove_device,
	.set_fmt	= rda_camera_set_fmt,
	.try_fmt	= rda_camera_try_fmt,
	.get_formats	= rda_camera_get_formats,
	.init_videobuf2	= rda_camera_init_videobuf,
	.poll		= rda_camera_poll,
	.querycap	= rda_camera_querycap,
	.set_bus_param	= rda_camera_set_bus_param,
};

static int rda_camera_remove(struct platform_device *pdev)
{
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
	struct rda_camera_dev *rcam = container_of(soc_host,
			struct rda_camera_dev, soc_host);

	free_irq(rcam->irq, rcam);
	soc_camera_host_unregister(soc_host);
	vb2_dma_contig_cleanup_ctx(rcam->alloc_ctx);
	cam_regs = NULL;
	iounmap(rcam->regs);
	clk_unprepare(rcam->pclk);
	clk_put(rcam->pclk);
	kfree(rcam);

	return 0;
}

static int rda_camera_probe(struct platform_device *pdev)
{
	unsigned int irq;
	struct rda_camera_dev *rcam;
	struct clk *pclk;
	struct resource *regs;
	int ret;
	struct device *dev = &pdev->dev;
	struct soc_camera_host *soc_host;
	struct rda_camera_device_data *pdata;

	pdata = dev->platform_data;
	if (!pdata) {
		printk(KERN_ERR "%s: No platform data available\n", __func__);
		return -EINVAL;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs)
		return -ENXIO;

	pclk = clk_get(&pdev->dev, RDA_CLK_CAMERA);
	if (IS_ERR(pclk))
		return PTR_ERR(pclk);
	ret = clk_prepare(pclk);
	if (ret)
		goto err_clk_prepare;

	rcam = kzalloc(sizeof(struct rda_camera_dev), GFP_KERNEL);
	if (!rcam) {
		ret = -ENOMEM;
		printk(KERN_ERR "%s: Can't allocate interface!\n", __func__);
		goto err_alloc_rcam;
	}

	rcam->pclk = pclk;
	rcam->pdata = pdata;
	rcam->active = NULL;
	rcam->next = NULL;
	spin_lock_init(&rcam->lock);
	INIT_DELAYED_WORK(&rcam->isr_work, handle_vsync);
//	tasklet_init(&rcam_tasklet, handle_vsync, (unsigned long)rcam);
	init_waitqueue_head(&rcam->vsync_wq);
	INIT_LIST_HEAD(&rcam->cap_buffer_list);

	rcam->alloc_ctx = vb2_dma_contig_init_ctx(&pdev->dev);
	if (IS_ERR(rcam->alloc_ctx)) {
		ret = PTR_ERR(rcam->alloc_ctx);
		goto err_alloc_ctx;
	}

	rcam->regs = ioremap(regs->start, resource_size(regs));
	if (!rcam->regs) {
		ret = -ENOMEM;
		goto err_ioremap;
	}
	cam_regs = rcam->regs;

	irq = platform_get_irq(pdev, 0);
	if (IS_ERR_VALUE(irq)) {
		ret = irq;
		goto err_req_irq;
	}
	ret = request_irq(irq, rda_camera_isr, IRQF_SHARED, pdev->name, rcam);
	if (ret) {
		printk(KERN_ERR "%s: Unable to request irq %d\n",
				__func__, irq);
		goto err_req_irq;
	}
	rcam->irq = irq;

	soc_host = &rcam->soc_host;
	soc_host->drv_name = RDA_CAMERA_DRV_NAME;
	soc_host->ops = &rda_soc_camera_host_ops;
	soc_host->priv = rcam;
	soc_host->v4l2_dev.dev = &pdev->dev;
	soc_host->nr = pdev->id;

	ret = soc_camera_host_register(soc_host);
	if (ret) {
		printk(KERN_ERR "%s: Unable to register soc camera host\n",
				__func__);
		goto err_register_soc_camera_host;
	}
	return 0;

err_register_soc_camera_host:
	free_irq(rcam->irq, rcam);
err_req_irq:
	cam_regs = NULL;
	iounmap(rcam->regs);
err_ioremap:
	vb2_dma_contig_cleanup_ctx(rcam->alloc_ctx);
err_alloc_ctx:
//	tasklet_kill(&rcam_tasklet);
	kfree(rcam);
err_alloc_rcam:
	clk_unprepare(pclk);
err_clk_prepare:
	clk_put(pclk);

	return ret;
}

static struct platform_driver rda_camera_driver = {
	.probe = rda_camera_probe,
	.remove = rda_camera_remove,
	.driver = {
		.name = RDA_CAMERA_DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init rda_camera_init_module(void)
{
	return platform_driver_probe(&rda_camera_driver, &rda_camera_probe);
}

static void __exit rda_camera_exit(void)
{
	platform_driver_unregister(&rda_camera_driver);
}

module_init(rda_camera_init_module);
module_exit(rda_camera_exit);

MODULE_AUTHOR("Wei Xing <xingwei@rdamicro.com>");
MODULE_DESCRIPTION("The V4L2 driver for RDA camera");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("video");

