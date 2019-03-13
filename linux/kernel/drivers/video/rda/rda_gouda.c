#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <asm/sizes.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <plat/reg_gouda.h>
#include <plat/ap_clk.h>
#include <linux/notifier.h>
#include <plat/pm_ddr.h>

#include <mach/rda_clk_name.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#include <rda/tgt_ap_clock_config.h>
#include "rda_gouda.h"
#include "rda_panel.h"

#ifndef _TGT_AP_RGB_SCALE_LEVLEL
#define _TGT_AP_RGB_SCALE_LEVLEL 0
#endif

struct rda_gd {
	void __iomem *reg_base;
	int irq;
	spinlock_t lock;
	wait_queue_head_t wait_eof;
	wait_queue_head_t wait_frmover;
	int layerOpenMask;
	bool is_active;
	bool is_frm_over;
	//it is urgly, but have to
	bool bIsDbiPanel;
	struct clk *mclk;

	int bIsFirstFrame;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_ops;
#endif /* CONFIG_HAS_EARLYSUSPEND */

	int enabled;

	bool scale_switch;
	bool scale_flag;
	bool scale_status;
};

static struct rda_gd *rda_gouda = NULL;
static int TECON_ORI_VAL=0;

#if 0
static bool rda_gouda_frame_over(void)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	return (hwp_gouda->gd_eof_irq & GOUDA_FRAME_OVER_IRQ) ? true : false;
}
#endif


static void rda_gouda_irq_enable(int irqMask)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	hwp_gouda->gd_eof_irq_mask |= irqMask;
}


static void rda_gouda_irq_clear(int irqMask)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *)rda_gouda->reg_base;
	hwp_gouda->gd_eof_irq_mask &= ~irqMask;
}

static int rda_gouda_get_irq_status(void)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	return hwp_gouda->gd_eof_irq;
}

static void rda_gouda_clear_irq_status(int statusMask)
{
	//still do not know why eof_irq register can not set bit!
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	hwp_gouda->gd_eof_irq = statusMask;
}

void rda_gouda_wait_done(void)
{
	BUG_ON(rda_gouda == NULL);

	wait_event_timeout(rda_gouda->wait_eof, !rda_gouda->is_active, HZ / 2);

	if (rda_gouda->is_active) {
		pr_err("%s : timeout\n", __func__);
	}

	return;
}

static void rda_gouda_update_buffer(UINT32 addr, bool sync)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	hwp_gouda->dpi_frame0_adrr = addr;
	if (rda_gouda->bIsFirstFrame) {
	    rda_gouda->bIsFirstFrame = 0;
	    hwp_gouda->dpi_config |= 0x1;
	}
	if (sync) {

		//timeout is 2 frames
		rda_gouda->is_frm_over = false;
		wait_event_timeout(rda_gouda->wait_frmover, rda_gouda->is_frm_over, (HZ/2));
	}

}

static void rda_gouda_set_roi(u16 tlX, u16 tlY, u16 brX, u16 brY, int bg_color)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	/* configure ROI in Gouda */
	hwp_gouda->gd_roi_tl_ppos = GOUDA_X0(tlX) | GOUDA_Y0(tlY);
	hwp_gouda->gd_roi_br_ppos = GOUDA_X1(brX) | GOUDA_Y1(brY);
	hwp_gouda->gd_roi_bg_color = bg_color;
}

static int rda_gouda_ovl_disable(int layerMask)
{
	int i = 0;
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	if (layerMask & 0x8)
		hwp_gouda->gd_vl_input_fmt &= ~(GOUDA_ACTIVE);

	layerMask &= 0x7;

	for (i = 0; i < 3; i++) {
		if (layerMask & (1 << i))
			hwp_gouda->overlay_layer[i].gd_ol_input_fmt &= ~(GOUDA_ACTIVE);
	}

	return 0;
}

static int rda_gouda_ovl_enable(int layerMask)
{
	int i;
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	if (layerMask & 0x8)
		hwp_gouda->gd_vl_input_fmt |= (GOUDA_ACTIVE);

	layerMask &= 0x7;

	for (i = 0; i < 3; i++) {
		if (layerMask & (1 << i))
			hwp_gouda->overlay_layer[i].gd_ol_input_fmt |= (GOUDA_ACTIVE);
	}

	return 0;
}

static int rda_gouda_blit(bool wait, int isLast, int dstBuf, int dstOffset)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	hwp_gouda->gd_lcd_mem_address = dstBuf;
	hwp_gouda->gd_lcd_stride_offset = dstOffset;

	//for dbi panel, hwcomposer also need to write to ram
	if (rda_gouda->bIsDbiPanel) {
	    	if (isLast) {
			hwp_gouda->gd_lcd_ctrl =
			GOUDA_LCD_CTRL_CS_MASK(hwp_gouda->gd_lcd_ctrl);

	   	} else {
			hwp_gouda->gd_lcd_ctrl =
			GOUDA_LCD_CTRL_CS_MASK(hwp_gouda->gd_lcd_ctrl) |
			GOUDA_LCD_IN_RAM;
		}
	}

	rda_gouda_clear_irq_status(GOUDA_EOF_IRQ);

	pm_ddr_get(PM_DDR_GOUDA_DMA);
	/* Run GOUDA */
	rda_gouda_irq_enable(GOUDA_EOF_MASK);

	rda_gouda_ovl_enable(rda_gouda->layerOpenMask);
	hwp_gouda->gd_command = GOUDA_START;


	if (wait) {
		rda_gouda_wait_done();

		//need manually un-active video layer, can not depend on interrtupt
		rda_gouda_ovl_close((1 << GOUDA_VID_LAYER_ID3));
	}

	//printk("isLast %d bDisplay %d dstBuf %x\n", isLast, bDisplay, dstBuf);
	//if (isLast && !rda_gouda->bIsDbiPanel)
	//	rda_gouda_update_buffer(dstBuf, false);

	return 0;
}

static int rda_gouda_vid_layer_open(struct gouda_vid_layer_def *def, u32 xpitch,
				    u32 ypitch)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	//BUG_ON(!(rda_gouda->layerOpenMask & (1 << GOUDA_VID_LAYER_ID3)));

	hwp_gouda->gd_vl_extents =
	    GOUDA_MAX_COL(def->width - 1) | GOUDA_MAX_LINE(def->height - 1);

	hwp_gouda->gd_vl_y_src = (u32) def->addr_y;
	hwp_gouda->gd_vl_u_src = (u32) def->addr_u;
	hwp_gouda->gd_vl_v_src = (u32) def->addr_v;

	hwp_gouda->gd_vl_resc_ratio =
	    (GOUDA_XPITCH(xpitch) | GOUDA_YPITCH(ypitch)) | (rda_gouda->bIsDbiPanel ? GOUDA_READ_ACCELERATE : 0);

	hwp_gouda->gd_vl_input_fmt =
	    GOUDA_FORMAT(def->fmt) | GOUDA_STRIDE(def->stride);
	hwp_gouda->gd_vl_tl_ppos =
	    GOUDA_X0(def->pos.tlX) | GOUDA_Y0(def->pos.tlY);
	hwp_gouda->gd_vl_br_ppos =
	    GOUDA_X1(def->pos.brX) | GOUDA_Y1(def->pos.brY);

#if 0
	hwp_gouda->gd_vl_blend_opt =
	    GOUDA_DEPTH(def->depth) |
	    GOUDA_ALPHA(def->alpha) |
	    GOUDA_CHROMA_KEY_COLOR(def->ckey_color) |
	    ((def->ckey_en) ? GOUDA_CHROMA_KEY_ENABLE : 0) |
	    GOUDA_CHROMA_KEY_MASK(def->ckey_mask) |
	    (GOUDA_ROTATION(def->rotation));
#endif

	return 0;
}

static int rda_gouda_osd_layer_open(struct gouda_ovl_buf_var *def)
{

	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	u16 stride;
	u8 wpp;
	int alpha;

	int id = def->ovl_id;

	//BUG_ON(!(rda_gouda->layerOpenMask & (1 << id)));
	if (id > GOUDA_OVL_LAYER_ID2) {
		pr_err("no such osd layer used id %d\r\n", id);
		return -1;
	}

	if ((hwp_gouda->overlay_layer[id].gd_ol_input_fmt & GOUDA_ACTIVE) != 0) {
		pr_err("gouda_osd_layer_open, layer%d used\r\n", id);
		return -1;
	}

	wpp = (def->image_fmt == GOUDA_IMG_FORMAT_RGBA) ? 2 : 1;

	if (def->stride)
		stride = def->stride;
	else
		stride = (def->pos.brX - def->pos.tlX + 1) * wpp;

	// TODO check supported formats
	hwp_gouda->overlay_layer[id].gd_ol_input_fmt =
	    ((def->image_fmt == GOUDA_IMG_FORMAT_RGB565) ? GOUDA_FORMAT(0) :
	     GOUDA_FORMAT(1)) | GOUDA_STRIDE(stride);

	alpha = 255 - def->blend_var.alpha;
	def->blend_var.alpha = alpha;
	hwp_gouda->overlay_layer[id].gd_ol_blend_opt = *((int*)&def->blend_var);

	hwp_gouda->overlay_layer[id].gd_ol_tl_ppos =
	    GOUDA_X0(def->pos.tlX) | GOUDA_Y0(def->pos.tlY);
	hwp_gouda->overlay_layer[id].gd_ol_br_ppos =
	    GOUDA_X1(def->pos.brX) | GOUDA_Y1(def->pos.brY);

	rda_dbg_gouda("osd srcOffset %d\n", def->srcOffset);
	hwp_gouda->overlay_layer[id].gd_ol_rgb_src = (UINT32) (def->buf + def->srcOffset * 2);

	return 0;

}

int rda_gouda_ovl_open(struct gouda_ovl_open_var *layer, bool block)
{
	int layerMask = layer->layerMask;

	BUG_ON(layerMask & ~0xf);

	rda_gouda->layerOpenMask = layerMask;
	rda_dbg_gouda("layerOpenMask %x\n",  rda_gouda->layerOpenMask);
	//do we need to blit here?

	rda_gouda_set_roi(layer->roi.tlX, layer->roi.tlY, layer->roi.brX, layer->roi.brY, 0);
	rda_gouda->is_active = true;

	rda_gouda_blit(block, layer->isLast, layer->dstBuf, layer->dstOffset);

	return 0;
}

int rda_gouda_ovl_close(int layerMask)
{
	BUG_ON(layerMask & ~0xf);

	rda_gouda->layerOpenMask &= ~(layerMask);
	rda_gouda_ovl_disable(layerMask);

	return 0;
}

int rda_gouda_dbi_write_cmd2lcd(u16 addr)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	int err_status = 0;

	if ((hwp_gouda->gd_status & GOUDA_LCD_BUSY) != 0) {
		/* GOUDA LCD is busy */
		err_status = -1;
	} else {
		hwp_gouda->gd_lcd_single_access =
		    GOUDA_START_WRITE | GOUDA_LCD_DATA(addr);
		while ((hwp_gouda->gd_status & GOUDA_LCD_BUSY)) ;
		err_status = 0;
	}

	return err_status;
}

EXPORT_SYMBOL(rda_gouda_dbi_write_cmd2lcd);

int rda_gouda_dbi_write_data2lcd(u16 data)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	int err_status = 0;

	if ((hwp_gouda->gd_status & GOUDA_LCD_BUSY) != 0) {
		/* GOUDA LCD is busy */
		err_status = -1;
	} else {
		hwp_gouda->gd_lcd_single_access =
		    GOUDA_START_WRITE | GOUDA_TYPE | GOUDA_LCD_DATA(data);
		while ((hwp_gouda->gd_status & GOUDA_LCD_BUSY)) ;
		err_status = 0;
	}

	return err_status;
}

EXPORT_SYMBOL(rda_gouda_dbi_write_data2lcd);

int rda_gouda_dbi_read_data(u8 * data)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	int err_status = 0;

	if ((hwp_gouda->gd_status & GOUDA_LCD_BUSY) != 0) {
		/* GOUDA LCD is busy */
		err_status = -1;
	} else {
		/* Start to read */
		hwp_gouda->gd_lcd_single_access = GOUDA_START_READ | GOUDA_TYPE;
		while ((hwp_gouda->gd_status & GOUDA_LCD_BUSY)) ;
		*data = hwp_gouda->gd_lcd_single_access & 0xff;
		err_status = 0;
	}

	return err_status;
}

EXPORT_SYMBOL(rda_gouda_dbi_read_data);

int rda_gouda_dbi_read_data16(u16 * data)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	int err_status = 0;

	if ((hwp_gouda->gd_status & GOUDA_LCD_BUSY) != 0) {
		/* GOUDA LCD is busy */
		err_status = -1;
	} else {
		/* Start to read */
		hwp_gouda->gd_lcd_single_access = GOUDA_START_READ | GOUDA_TYPE;
		while ((hwp_gouda->gd_status & GOUDA_LCD_BUSY)) ;
		*data = hwp_gouda->gd_lcd_single_access & 0xffff;
		err_status = 0;
	}

	return err_status;
}

EXPORT_SYMBOL(rda_gouda_dbi_read_data16);

void rda_gouda_display(struct gouda_image *src,
		       struct gouda_rect *src_rect, u16 x, u16 y)
{
#ifdef CONFIG_FB_RDA_VSYNC_ENABLE
		rda_gouda_update_buffer((UINT32)src->buffer, true);
#else
		rda_gouda_update_buffer((UINT32)src->buffer, true);
#endif
	return;
}

void rda_gouda_osd_display(struct gouda_ovl_buf_var *def)
{
	//int ret;
	/* open the layer */
	rda_gouda_osd_layer_open(def);
	/* gouda is doing everything */
#if 0
	ret = rda_gouda_blit(1);
	if (ret) {
		pr_err("hal_gouda_blit_roi fail\n");
	}
#endif
}

void rda_gouda_set_ovl_var(struct gouda_ovl_blend *var, int id)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	u32 value = *((u32 *) var);
	if (id == GOUDA_VID_LAYER_ID3) {
		hwp_gouda->gd_vl_blend_opt = value;
	} else {
		hwp_gouda->overlay_layer[id].gd_ol_blend_opt = value;
	}
}

void rda_gouda_get_ovl_var(struct gouda_ovl_blend *var, int id)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	u32 value;
	if (id == GOUDA_VID_LAYER_ID3) {
		value = hwp_gouda->gd_vl_blend_opt;
	} else {
		value = hwp_gouda->overlay_layer[id].gd_ol_blend_opt;
	}

	*((u32 *) var) = value;
}

#define SWAP_INT(a, b)				\
	{  temp = a; a = b; b = temp; }

void rda_gouda_vid_display(struct gouda_input *src,
					struct gouda_output *dst)
{
	//int ret;
	struct gouda_vid_layer_def gouda_vid_def = { 0, };
	struct gouda_ovl_blend var;
	int out_width, out_height, in_width, in_height;
	int offsetY, offsetU, offsetV, stride, rotation;
	int temp = 0;
	int offset = src->srcOffset;
	u8 bpp;
	u32 xpitch, ypitch;

	rda_dbg_gouda("vid srcOffset %d\n", src->srcOffset);
	bpp = (src->image_fmt == GOUDA_IMG_FORMAT_IYUV) ? 1 : 2;
	stride = gouda_vid_def.stride =
	    src->stride ? src->stride : src->width * bpp;

	if (src->image_fmt != GOUDA_IMG_FORMAT_RGB565 &&
	    src->image_fmt != GOUDA_IMG_FORMAT_UYVY &&
	    src->image_fmt != GOUDA_IMG_FORMAT_YUYV &&
	    src->image_fmt != GOUDA_IMG_FORMAT_IYUV) {
		pr_err("source input format error fmt %d\n",
			   src->image_fmt);
	}

	if (dst->pos.tlX > dst->pos.brX || dst->pos.tlY > dst->pos.brY)
		pr_err("bad output roi tlx %d tlY %d brX %d brY %d\n",
			   dst->pos.tlX, dst->pos.tlY, dst->pos.brX,
			   dst->pos.brY);


	gouda_vid_def.fmt = src->image_fmt;

	rotation = dst->rot;
	rda_gouda_get_ovl_var(&var, GOUDA_VID_LAYER_ID3);
	var.rotation = rotation;
	rda_gouda_set_ovl_var(&var, GOUDA_VID_LAYER_ID3);
	/* stride calc in open() */
	gouda_vid_def.width = src->width;
	gouda_vid_def.height = src->height;

	in_width = src->width;
	in_height = src->height;
	out_width = dst->pos.brX - dst->pos.tlX + 1;
	out_height = dst->pos.brY - dst->pos.tlY + 1;

	gouda_vid_def.pos.tlX = dst->pos.tlX;
	gouda_vid_def.pos.tlY = dst->pos.tlY;
	gouda_vid_def.pos.brX = dst->pos.brX;
	gouda_vid_def.pos.brY = dst->pos.brY;

	offsetY = offsetU = offsetV = 0;
	if (rotation == GOUDA_VID_NO_ROTATION) {
		offsetY = offsetU = offsetV = 0;
	} else if (rotation == GOUDA_VID_90_ROTATION) {
		offsetY = stride * (src->height - 1);
		offsetU = offsetV = stride / 2 * (src->height / 2 - 1);
		SWAP_INT(in_width, in_height);

	} else if (rotation == GOUDA_VID_180_ROTATION) {
		offsetY = stride * (src->height - 1);
		offsetU = offsetV = stride / 2 * (src->height / 2 - 1);
	} else if (rotation == GOUDA_VID_270_ROTATION) {
		offsetY = stride - ((src->image_fmt ==
				GOUDA_IMG_FORMAT_IYUV) ? 8 : 16);
		offsetU = offsetV = offsetY / 2;
		SWAP_INT(in_width, in_height);
	}

	gouda_vid_def.addr_y = (u32 *) (src->bufY + offsetY + offset);
	gouda_vid_def.addr_u = (u32 *) (src->bufU + offsetU + offset);
	gouda_vid_def.addr_v = (u32 *) (src->bufV + offsetV + offset);

	xpitch = (in_width << 8) / out_width;
	ypitch = (in_height << 8) / out_height;
	rda_dbg_gouda("xpitch %d ypitch %d  buffer %p stride %d\n",
		xpitch, ypitch, gouda_vid_def.addr_y, stride);
	/* open the layer */
	rda_gouda_vid_layer_open(&gouda_vid_def, xpitch, ypitch);
}
EXPORT_SYMBOL(rda_gouda_vid_display);

int rda_gouda_stretch_pre_wait_and_enable_clk(void)
{
	long ret;

	if (rda_gouda->bIsDbiPanel) {
		ret = wait_event_timeout(rda_gouda->wait_eof,  !rda_gouda->is_active, HZ / 2);
		if (ret == 0 && rda_gouda->is_active) {
			//pr_err("%s : gouda is active.\n", __func__);
			return -ETIMEDOUT;
		}

		if (rda_gouda->mclk) {
			clk_enable(rda_gouda->mclk);
		}
	}

	return 0;
}
EXPORT_SYMBOL(rda_gouda_stretch_pre_wait_and_enable_clk);

void rda_gouda_stretch_post_disable_clk(void)
{
	if (rda_gouda->bIsDbiPanel && rda_gouda->mclk) {
		clk_disable(rda_gouda->mclk);
	}
}
EXPORT_SYMBOL(rda_gouda_stretch_post_disable_clk);

void rda_gouda_stretch_blit(struct gouda_input *src,
					struct gouda_output *dst, bool block)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	struct gouda_ovl_open_var var;

	if (!rda_gouda->enabled) {
	    pr_err("%s disable!\n", __func__);
	    return;
	}

	if ((hwp_gouda->gd_status & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
		wait_event_timeout(rda_gouda->wait_eof,	!rda_gouda->is_active, HZ / 2);
		if ((hwp_gouda->gd_status & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
			pr_err("%s: gouda busy! status = 0x%x, block = %d\n",
				__func__, hwp_gouda->gd_status, block);
			return;
		}
	}

	rda_gouda_vid_display(src, dst);

	memset(&var, 0, sizeof(var));
	var.layerMask = (1 << GOUDA_VID_LAYER_ID3);
	var.dstBuf = dst->buf;
	var.addr_type = dst->addr_type;
	//hw composer does not need last flag to store temporary image, the
	//isLast flag is used for mcu panel to differ memory and lcd
	var.isLast = dst->addr_type == FrmBufIndexType;
	memcpy(&var.roi, &dst->pos, sizeof(struct gouda_rect));

	rda_gouda_ovl_open(&var, block);

}


EXPORT_SYMBOL(rda_gouda_stretch_blit);

void rda_gouda_pre_enable_lcd(struct gouda_lcd *lcd,int onoff)
{
	struct clk *pre_clk = NULL;
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	pre_clk = clk_get(NULL, RDA_CLK_GOUDA);
	if (!pre_clk) {
		pr_err("failed to get gouda clk controller !\n");
		return;
	}

	if(onoff){
		clk_enable(pre_clk);
		if (lcd->lcd_interface == GOUDA_LCD_IF_DBI) {
			hwp_gouda->gd_lcd_ctrl = lcd->lcd_cfg.reg[0];
			hwp_gouda->TECON = lcd->lcd_cfg.reg[1];
			TECON_ORI_VAL= lcd->lcd_cfg.reg[1];
			hwp_gouda->TECON2 = lcd->lcd_cfg.reg[2];
			hwp_gouda->gd_lcd_timing = lcd->lcd_timing.reg;
			hwp_gouda->gd_spilcd_config = 0;
		}
	}else{
		clk_disable(pre_clk);
	}
}
EXPORT_SYMBOL(rda_gouda_pre_enable_lcd);

/*
te_mode:
0:close
1:open
-1:reset to default
.sheen
*/
void rda_gouda_configure_te(struct gouda_lcd *lcd,int te_mode)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	if (lcd->lcd_interface == GOUDA_LCD_IF_DBI) {
	    if(te_mode== 0)
		hwp_gouda->TECON = TECON_ORI_VAL & 0xFFFFFFFE;
	    else if(te_mode== 1)
		hwp_gouda->TECON = TECON_ORI_VAL | 0x1;
	    if(te_mode== -1)
		hwp_gouda->TECON = TECON_ORI_VAL;
	} 
}
EXPORT_SYMBOL(rda_gouda_configure_te);


void rda_gouda_configure_timing(struct gouda_lcd *lcd)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	if (lcd->lcd_interface == GOUDA_LCD_IF_DBI) {
		hwp_gouda->gd_lcd_timing = lcd->lcd_timing.reg;
	} else {

		printk("info: to do add spi reconfiguration timing here\n");
	}
}

EXPORT_SYMBOL(rda_gouda_configure_timing);

static int scale_level[] = {
	100,// 0
	150,// 1
	160,// 2
	200,// 3
};

void rda_gouda_scale_clk(struct rda_gd *gouda,struct rda_lcd_info *lcd_info)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	unsigned int def_vth,def_hdth;
	if(!gouda->scale_switch)
		return;

	def_vth =
		lcd_info->lcd.lcd_timing.rgb.v_back_porch +
		lcd_info->lcd.lcd_timing.rgb.v_front_porch - 1 +
		lcd_info->lcd.lcd_timing.rgb.height;
	def_hdth =
		lcd_info->lcd.lcd_timing.rgb.width +
		lcd_info->lcd.lcd_timing.rgb.h_back_porch +
		lcd_info->lcd.lcd_timing.rgb.h_front_porch - 1;

	if(gouda->scale_flag){
		/*
		def_vth = (def_vth * scale_level[2])/100;
		hwp_gouda->dpi_time1 &= ~0x7ff;
		hwp_gouda->dpi_time1 |=
			VSYNC_INCLUDE_HSYNC_TH_HIGH(def_vth > 0x7ff ? 0x7ff : def_vth);
		*/
		def_hdth = (def_hdth * scale_level[_TGT_AP_RGB_SCALE_LEVLEL])/100;
		hwp_gouda->dpi_time2 &= ~0x7ff;
		hwp_gouda->dpi_time2 |=
			HSYNC_INCLUDE_DOTCLK_TH_HIGH(def_hdth > 0x7ff ? 0x7ff : def_hdth);
	}else{
		/*
		hwp_gouda->dpi_time1 &= ~0x7ff;
		hwp_gouda->dpi_time1 |=
			VSYNC_INCLUDE_HSYNC_TH_HIGH(def_vth);
		*/
		hwp_gouda->dpi_time2 &= ~0x7ff;
		hwp_gouda->dpi_time2 |=
			HSYNC_INCLUDE_DOTCLK_TH_HIGH(def_hdth);
	}

	gouda->scale_status = false;
}

/*
void rda_gouda_auto_clk(struct rda_gd *gouda)
{
	//if(gouda->scale_status)
		//rda_gouda_scale_clk(gouda);
}
*/
int rda_gouda_save_outfmt(void)
{
     int reg;
     HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
     reg = hwp_gouda->gd_lcd_ctrl;
     hwp_gouda->gd_lcd_ctrl = (reg & ~0x70) | (GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565 << 4);
     return reg;
}

void rda_gouda_restore_outfmt(int reg)
{
     HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
     hwp_gouda->gd_lcd_ctrl = reg;
}


void rda_gouda_open_lcd(struct gouda_lcd *lcd, unsigned int addr)
{
	struct gouda_ovl_blend var = { 0 };
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	clk_enable(rda_gouda->mclk);
	if (lcd->lcd_interface == GOUDA_LCD_IF_DBI) {
		hwp_gouda->gd_lcd_ctrl = lcd->lcd_cfg.reg[0];
		hwp_gouda->TECON = lcd->lcd_cfg.reg[1];
		TECON_ORI_VAL= lcd->lcd_cfg.reg[1];
		hwp_gouda->TECON2 = lcd->lcd_cfg.reg[2];
		hwp_gouda->gd_lcd_timing = lcd->lcd_timing.reg;
		hwp_gouda->gd_spilcd_config = 0;
		//for DBI, lcd display need blit!
		rda_gouda->bIsDbiPanel = true;
	} else if (lcd->lcd_interface == GOUDA_LCD_IF_DPI) {
		u32 val = 0;
		unsigned long gd_clk = clk_get_rate(rda_gouda->mclk);
		int bpp;
		if (lcd->lcd_cfg.rgb.pix_fmt == RDA_FMT_RGB565)
			bpp = 2;
		else if (lcd->lcd_cfg.rgb.pix_fmt == RDA_FMT_RGB888)
			bpp = 3;
		else
			bpp = 4;

		rda_gouda->bIsDbiPanel = false;
		rda_gouda_irq_enable(GOUDA_FRAME_OVER_MASK);
		/* Calculate a divider used by dpi lcd. */
		if (!lcd->lcd_timing.rgb.clk_divider &&
			lcd->lcd_timing.rgb.lcd_freq) {
			val = gd_clk / lcd->lcd_timing.rgb.lcd_freq;

			lcd->lcd_timing.rgb.clk_divider = (val > 255 ? 255 : val);
		}

		val = lcd->lcd_timing.rgb.clk_divider & 0xff;

		if (lcd->lcd_timing.rgb.data_pol)
			val |= (1 << 11);
		if (lcd->lcd_timing.rgb.v_pol)
			val |= (1 << 10);
		if (lcd->lcd_timing.rgb.h_pol)
			val |= (1 << 9);
		if (lcd->lcd_timing.rgb.dot_clk_pol)
			val |= (1 << 8);
		hwp_gouda->dpi_pol = val;
		hwp_gouda->dpi_time0 =
			BACK_PORCH_END_VSYNC_TIMER(
			lcd->lcd_timing.rgb.v_low +
			lcd->lcd_timing.rgb.v_back_porch) |
			FRONT_PORCH_STATRT_TIMER(
			lcd->lcd_timing.rgb.v_low +
			lcd->lcd_timing.rgb.v_back_porch +
			lcd->lcd_timing.rgb.height);
		hwp_gouda->dpi_time1 =
			VSYNC_INCLUDE_HSYNC_TH_LOW(
			lcd->lcd_timing.rgb.v_low) |
			VSYNC_INCLUDE_HSYNC_TH_HIGH(
			lcd->lcd_timing.rgb.v_low +
			lcd->lcd_timing.rgb.v_back_porch +
			lcd->lcd_timing.rgb.v_front_porch - 1 +
			lcd->lcd_timing.rgb.height);
		hwp_gouda->dpi_time2 =
			HSYNC_INCLUDE_DOTCLK_TH_LOW(
			lcd->lcd_timing.rgb.h_low) |
			HSYNC_INCLUDE_DOTCLK_TH_HIGH(
			lcd->lcd_timing.rgb.h_low +
			lcd->lcd_timing.rgb.width +
			lcd->lcd_timing.rgb.h_back_porch +
			lcd->lcd_timing.rgb.h_front_porch - 1);
		hwp_gouda->dpi_time3 =
			RGB_DATA_ENABLE_START_TIMER(
			lcd->lcd_timing.rgb.h_low +
			lcd->lcd_timing.rgb.h_back_porch) |
			RGB_DATA_ENABLE_END_TIMER(
			lcd->lcd_timing.rgb.h_low +
			lcd->lcd_timing.rgb.width +
			lcd->lcd_timing.rgb.h_back_porch);
		hwp_gouda->dpi_size =
			VERTICAL_PIX_NUM(lcd->lcd_timing.rgb.height) |
			HORIZONTAL_PIX_NUM(lcd->lcd_timing.rgb.width);
		hwp_gouda->dpi_frame0_con =
			((bpp * lcd->lcd_timing.rgb.width) << 16) | 0x1;

		hwp_gouda->gd_lcd_mem_address = addr;
		hwp_gouda->dpi_frame0_adrr = (UINT32)addr;
		hwp_gouda->gd_lcd_ctrl = GOUDA_LCD_IN_RAM;
		GOUDA_LCD_CTRL_FMT_SET(hwp_gouda->gd_lcd_ctrl, GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565);


		hwp_gouda->dct_shift_uv_reg1 =
			HSYNC_INCLUDE_DOTCLK_TH_HIGH(
			lcd->lcd_timing.rgb.width +
			lcd->lcd_timing.rgb.h_back_porch +
			lcd->lcd_timing.rgb.h_front_porch - 1);

		/* workaround for dislocation problem */
		//hwp_gouda->dpi_fifo_ctrl = 2;

		//metal1 fix for underflow
		hwp_gouda->dct_shift_uv_reg1 |= DPI_WAIT_UNDERFLOW;

		//u07-u08 auto-clk
		hwp_gouda->dct_shift_uv_reg1 |= DPI_CLK_AUTO;

		//U06 fix to request 2 outstanding
		hwp_gouda->dct_shift_uv_reg1 |= DPI_REQUEST_2_OUTSTANDING;

		//metal2 fix for ARGB
		hwp_gouda->dct_shift_uv_reg1 |= (0x7 << 27);

		//metal0 hack to accelerate video scalar, it is only valid for
		//rda8810, later chip will have new design
#define RDA8810_ONLY_WORKAROUND
#ifdef RDA8810_ONLY_WORKAROUND
		hwp_gouda->dct_shift_uv_reg1 |= ACCELERATE_VIDOE_LAYAER_FLAG;
#endif
		hwp_gouda->dpi_config =
			(lcd->lcd_cfg.rgb.rgb_format << 12)
			| (lcd->lcd_cfg.rgb.frame1 << 1)
			| (lcd->lcd_cfg.rgb.frame2 << 2)
			| (lcd->lcd_cfg.rgb.pix_fmt << 4);
#if 0
		printk(KERN_INFO "dpi_pol %x\n", hwp_gouda->dpi_pol);
		printk(KERN_INFO "dpi_time0 %x\n", hwp_gouda->dpi_time0);
		printk(KERN_INFO "dpi_time1 %x\n", hwp_gouda->dpi_time1);
		printk(KERN_INFO "dpi_time2 %x\n", hwp_gouda->dpi_time2);
		printk(KERN_INFO "dpi_time3 %x\n", hwp_gouda->dpi_time3);
		printk(KERN_INFO "dpi_size %x\n", hwp_gouda->dpi_size);
		printk(KERN_INFO "dpi_frame0_con %x\n", hwp_gouda->dpi_frame0_con);
		printk(KERN_INFO "dpi_config %x\n", hwp_gouda->dpi_config);
#endif
	}

	rda_gouda_set_roi(0, 0, lcd->width - 1, lcd->height - 1, 0);

	var.alpha = 0xff;
	//let framebuffer connect to osd0, videolayer behind
	var.depth = GOUDA_VID_LAYER_BEHIND_ALL;
	rda_gouda_set_ovl_var(&var, GOUDA_VID_LAYER_ID3);
	//set osd0 alpha to 0xff by default
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID0);
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID1);
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID2);
	/* need this delay to stable */

}

void rda_gouda_close_lcd(struct gouda_lcd *lcd)
{
	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	if (lcd->lcd_interface == GOUDA_LCD_IF_DBI) {
		hwp_gouda->gd_spilcd_config = 0;
		hwp_gouda->gd_lcd_ctrl = 0;
	} else if (lcd->lcd_interface == GOUDA_LCD_IF_DPI) {
	}

}

void rda_gouda_flash_fifo(void)
{

	HWP_GOUDA_T *hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	volatile unsigned int dpi_fifo = hwp_gouda->dpi_fifo_ctrl;

	/* Clear fifo bit */
	while (!dpi_fifo & 0x1) {
		dpi_fifo |= 0x1;
		hwp_gouda->dpi_fifo_ctrl = dpi_fifo;
		dpi_fifo = hwp_gouda->dpi_fifo_ctrl;
	}

	mdelay(10);
	while (dpi_fifo & 0x1) {
		dpi_fifo &= 0xFFFFFFFE;
		hwp_gouda->dpi_fifo_ctrl = dpi_fifo;
		dpi_fifo = hwp_gouda->dpi_fifo_ctrl;
	}

	return;
}

static irqreturn_t rda_gouda_interrupt(int irq, void *dev_id)
{
	unsigned long irq_flags;
	struct rda_gd *gd = dev_id;
	uint32_t status;

	spin_lock_irqsave(&gd->lock, irq_flags);
	status = rda_gouda_get_irq_status();

	rda_dbg_gouda("rda_gd_interrupt, eof irq %x\n", status);
	if (status & GOUDA_FRAME_OVER_IRQ) {
		rda_dbg_gouda("rda_gd_interrupt, frameover irq\n");
		rda_gouda->is_frm_over = true;
		//rda_gouda_irq_clear(GOUDA_FRAME_OVER_MASK);
		rda_gouda_clear_irq_status(GOUDA_FRAME_OVER_IRQ);
		wake_up(&gd->wait_frmover);
	}

	if (status & GOUDA_EOF_IRQ) {
		rda_gouda_irq_clear(GOUDA_EOF_MASK);
		//close all layer
		rda_gouda_ovl_disable(gd->layerOpenMask);
		gd->layerOpenMask = 0;
		gd->is_active = false;

		if(gd->bIsDbiPanel && gd->mclk)
			clk_disable(gd->mclk);

		wake_up(&gd->wait_eof);
		pm_ddr_put(PM_DDR_GOUDA_DMA);
		rda_dbg_gouda("rda_gd_interrupt, eof irq\n");
	}

	spin_unlock_irqrestore(&gd->lock, irq_flags);

	return status ? IRQ_HANDLED : IRQ_NONE;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_gouda_early_suspend(struct early_suspend *h)
{
	HWP_GOUDA_T *hwp_gouda = hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;

	/* Clear bit0 */
	hwp_gouda->dpi_frame0_con &= 0xFFFFFFFE;
	hwp_gouda->dpi_config &= 0xFFFFFFFE;
}

static void rda_gouda_late_resume(struct early_suspend *h)
{
	//apsys_reset_gouda();
	rda_gouda->bIsFirstFrame = 1;
}
#else
static int rda_gouda_suspend(struct device *dev)
{
	HWP_GOUDA_T *hwp_gouda = hwp_gouda = (HWP_GOUDA_T *) rda_gouda->reg_base;
	printk("gouda suspend in\n");

	if ((hwp_gouda->gd_status & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
		wait_event_timeout(rda_gouda->wait_eof,
			!rda_gouda->is_active, HZ / 2);
		if ((hwp_gouda->gd_status & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
			pr_err("rda_gouda_suspend: gouda busy\n");
			return -1;
		}
	}

	if(!rda_gouda->is_active){//write wrong memory addr maybe.sheen
	    /* Clear bit0 */
	    hwp_gouda->dpi_frame0_con &= 0xFFFFFFFE;
	    hwp_gouda->dpi_config &= 0xFFFFFFFE;
	}
	printk("gouda suspend out\n");

	return 0;
}

static int rda_gouda_resume(struct device *dev)
{
	//apsys_reset_gouda();
	rda_gouda->bIsFirstFrame = 1;
	return 0;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static ssize_t rda_gouda_switch_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", gouda->scale_switch);
}

static ssize_t rda_gouda_switch_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);
	struct rda_lcd_info *lcd_info;
	struct platform_device *rda_fb_device;
	//unsigned long irq_flags;
	ssize_t ret;
	int set;

	rda_fb_device = (struct platform_device*)rda_fb_get_device();
	lcd_info = (struct rda_lcd_info *)rda_fb_device->dev.platform_data;

	ret = kstrtoint(buf, 0, &set);
	if (ret < 0) {
		return ret;
	}

	if(!set){
		/*restore gou rgb pclk div*/
		//spin_lock_irqsave(&gouda->lock, irq_flags);
		gouda->scale_flag = false;
		rda_gouda_scale_clk(gouda,lcd_info);
		//spin_unlock_irqrestore(&gouda->lock, irq_flags);
	}

	gouda->scale_switch= set;
	return count;
}

static DEVICE_ATTR(switch, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_gouda_switch_show, rda_gouda_switch_store);

static ssize_t rda_gouda_scale_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", gouda->scale_flag);
}

static ssize_t rda_gouda_scale_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);
	struct rda_lcd_info *lcd_info;
	struct platform_device *rda_fb_device;
	int set,ret;

	rda_fb_device = (struct platform_device*)rda_fb_get_device();
	lcd_info = (struct rda_lcd_info *)rda_fb_device->dev.platform_data;

	if(lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DPI || _TGT_AP_RGB_SCALE_LEVLEL == 0)
		goto out;

	ret = kstrtoint(buf, 0, &set);
	if (ret < 0) {
		return ret;
	}
	rda_dbg_gouda("rda_gouda_scale_store in scale %s\n",set ? "on" : "off");
	gouda->scale_flag = set;
	gouda->scale_status = true;

	if(gouda->scale_switch){
		rda_gouda_scale_clk(gouda,lcd_info);
	}

out:
	return count;
}

static DEVICE_ATTR(scale, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_gouda_scale_show, rda_gouda_scale_store);

static ssize_t rda_gouda_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", gouda->enabled);
}

static ssize_t rda_gouda_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);
	int ret;
	int set;

	printk("rda_gouda_enable_store in\n");
	ret = kstrtoint(buf, 0, &set);
	if (ret < 0) {
		return ret;
	}

	set = !!set;

	if (gouda->enabled == set) {
		return count;
	}

	gouda->enabled = set;
	if (set) {
		ret = rda_gouda_resume(dev);
	} else {
		ret = rda_gouda_suspend(dev);
	}

	//gouda->enabled = set;

	printk("rda_gouda_enable_store out\n");
	return count;
}

static DEVICE_ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_gouda_enable_show, rda_gouda_enable_store);

static struct attribute *rda_gouda_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_scale.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group rda_gouda_attr_group = {
	.attrs = rda_gouda_attrs,
};

static int rda_gouda_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *mem;
	struct rda_gd *gd;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	gd = kzalloc(sizeof(*gd), GFP_KERNEL);
	if (gd == NULL) {
		ret = -ENOMEM;
		goto err_gd_alloc_failed;
	}
	rda_gouda = gd;

	gd->is_active = false;
	gd->is_frm_over = true;
	gd->bIsFirstFrame = 1;

	spin_lock_init(&gd->lock);
	init_waitqueue_head(&gd->wait_eof);
	init_waitqueue_head(&gd->wait_frmover);
	platform_set_drvdata(pdev, gd);

	gd->reg_base = ioremap(mem->start, resource_size(mem));
	if (!gd->reg_base) {
		dev_err(&pdev->dev, "ioremap fail\n");
		ret = -ENOMEM;
		goto err_iomap_fail;
	}


	gd->irq = platform_get_irq(pdev, 0);
	if (gd->irq < 0) {
		dev_err(&pdev->dev, "fail to get irq\n");
		ret = -ENODEV;
		goto err_request_irq_failed;
	}
	ret = request_irq(gd->irq, rda_gouda_interrupt,
			  IRQF_SHARED, pdev->name, gd);
	if (ret)
		goto err_request_irq_failed;

	gd->mclk = clk_get(NULL, RDA_CLK_GOUDA);
	if (IS_ERR(gd->mclk)) {
		ret = PTR_ERR(gd->mclk);
		goto err_clk_failed;
	}

	clk_prepare(gd->mclk);
	//rda_gouda_irq_enable(GOUDA_EOF_MASK);

#ifdef CONFIG_HAS_EARLYSUSPEND
	gd->early_ops.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 3;
	gd->early_ops.suspend = rda_gouda_early_suspend;
	gd->early_ops.resume = rda_gouda_late_resume;

	register_early_suspend(&gd->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	gd->enabled = 1;
	ret = sysfs_create_group(&pdev->dev.kobj, &rda_gouda_attr_group);
	if (ret < 0) {
		goto err_sysfs_create_group;
	}

	rda_dbg_gouda("rda_gd_probe, reg_base = 0x%08x, irq = %d\n",
		      mem->start, gd->irq);

	return 0;

err_sysfs_create_group:
	clk_unprepare(gd->mclk);
	clk_put(gd->mclk);

err_clk_failed:
	free_irq(gd->irq, gd);

err_request_irq_failed:
	iounmap(gd->reg_base);
err_iomap_fail:
	kfree(gd);
err_gd_alloc_failed:
	return ret;
}

static int rda_gouda_remove(struct platform_device *pdev)
{
	struct rda_gd *gd = platform_get_drvdata(pdev);
	printk("gouda remove\n");

	if (!gd) {
		return -EINVAL;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&gd->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	sysfs_remove_group(&pdev->dev.kobj, &rda_gouda_attr_group);

	clk_unprepare(gd->mclk);
	clk_put(gd->mclk);

	free_irq(gd->irq, gd);

	kfree(gd);
	rda_gouda = NULL;

	return 0;
}

static struct platform_driver rda_gouda_driver = {
	.probe = rda_gouda_probe,
	.remove = rda_gouda_remove,
	.driver = {
		   .name = RDA_GOUDA_DRV_NAME,
	}
};

static int __init rda_gouda_init_module(void)
{
	return platform_driver_register(&rda_gouda_driver);
}

static void __exit rda_gouda_exit_module(void)
{
	platform_driver_unregister(&rda_gouda_driver);
}

module_init(rda_gouda_init_module);
module_exit(rda_gouda_exit_module);

MODULE_AUTHOR("Huaping Wu<huapingwu@rdamicro.com>");
MODULE_DESCRIPTION("The lcd controller driver for RDA Linux");
