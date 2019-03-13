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
#include <plat/ap_clk.h>
#include <linux/notifier.h>
#include <plat/pm_ddr.h>

#include <mach/rda_clk_name.h>

#include "rda_gouda.h"

static struct rda_gd *rda_gouda = NULL;
static unsigned char * gouda_reg_base;

#define GOUDA_REG_BASE gouda_reg_base

#define SWAP_INT(a, b) (a ^= b),(b ^= a),(a ^= b)
static void rda_gouda_irq_enable(int irqmask)
{
	GOUDA_OR_WITH_REG(GOUDA_REG_BASE + GD_EOF_IRQ_MASK,irqmask);
}

static void rda_gouda_irq_clear(int irqmask)
{
	GOUDA_AND_WITH_REG(GOUDA_REG_BASE + GD_EOF_IRQ_MASK,~irqmask);
}

static u32 rda_gouda_get_irq_status(void)
{
	return GOUDA_INL(GOUDA_REG_BASE + GD_EOF_IRQ);
}

static void rda_gouda_clear_irq_status(int status_mask)
{
	GOUDA_OUTL(GOUDA_REG_BASE + GD_EOF_IRQ,status_mask);
}

void rda_gouda_wait_done(void)
{
	wait_event_timeout(rda_gouda->wait_eof, !rda_gouda->is_active, HZ / 2);

	if (rda_gouda->is_active) {
		pr_err("%s : timeout\n", __func__);
	}

	return;
}

static void enable_gouda_clk(bool on)
{
	if(on) {
		if(rda_gouda->gouda_clk_on) {
			pr_err("%s : goudav clk already enabled.\n", __func__);
			return;
		}
		rda_dbg_gouda("gouda clk on\n");
		clk_enable(rda_gouda->mclk);
		rda_gouda->gouda_clk_on = true;
	} else {
		if(!rda_gouda->gouda_clk_on) {
			pr_err("%s : gouda clk already disabled.\n", __func__);
			return;
		}
		rda_dbg_gouda("gouda clk off\n");
		clk_disable(rda_gouda->mclk);
		rda_gouda->gouda_clk_on = false;
	}
}

int rda_gouda_enable_clk(bool enable)
{
	long ret;

	if(enable){
		ret = wait_event_timeout(rda_gouda->wait_eof,  !rda_gouda->is_active, HZ / 2);
		if (ret == 0 && rda_gouda->is_active) {
			pr_err("%s : gouda is active.\n", __func__);
			return -ETIMEDOUT;
		}
		enable_gouda_clk(true);
	}else{
		enable_gouda_clk(false);
	}

	return 0;
}

void rda_gouda_reset(void)
{
	apsys_reset_gouda();
}

static void rda_gouda_set_roi(u16 tlx, u16 tly, u16 brx, u16 bry, int bg_color)
{
	u32 value;

	value = GOUDA_X0(tlx) | GOUDA_Y0(tly);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_ROI_TL_PPOS,value);
	value = GOUDA_X1(brx) | GOUDA_Y1(bry);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_ROI_BR_PPOS,value);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_ROI_BG_COLOR,bg_color);
}

static int gouda_ol_layer_set_value(u32 id,u32 offset,u32 value)
{
	switch(id) {
	case GOUDA_OVL_LAYER_ID0:
		GOUDA_OUTL(GOUDA_REG_BASE + 0x40 + offset,value);
		break;
	case GOUDA_OVL_LAYER_ID1:
		GOUDA_OUTL(GOUDA_REG_BASE + 0x54 + offset,value);
		break;
	case GOUDA_OVL_LAYER_ID2:
		GOUDA_OUTL(GOUDA_REG_BASE + 0x68 + offset,value);
		break;
	case GOUDA_OVL_LAYER_ID3:/* OL3/4 reg offset is not continous with OL0/1/2*/
		GOUDA_OUTL(GOUDA_REG_BASE + 0x11C + offset,value);
		break;
	case GOUDA_OVL_LAYER_ID4:
		GOUDA_OUTL(GOUDA_REG_BASE + 0x148 + offset,value);
		break;
	}

	return 0;
}

static u32 gouda_ol_layer_get_value(u32 id,u32 offset)
{
	switch(id) {
	case GOUDA_OVL_LAYER_ID0:
		return GOUDA_INL(GOUDA_REG_BASE + 0x40 + offset);
	case GOUDA_OVL_LAYER_ID1:
		return GOUDA_INL(GOUDA_REG_BASE + 0x54 + offset);
	case GOUDA_OVL_LAYER_ID2:
		return GOUDA_INL(GOUDA_REG_BASE + 0x68 + offset);
	case GOUDA_OVL_LAYER_ID3:/* OL3/4 reg offset is not continous with OL0/1/2*/
		return GOUDA_INL(GOUDA_REG_BASE + 0x11C + offset);
	case GOUDA_OVL_LAYER_ID4:
		return GOUDA_INL(GOUDA_REG_BASE + 0x148 + offset);
	}

	return 0;
}

static int rda_gouda_layer_disable(int mask)
{
	int i = 0;
	u32 value;

	if (mask & GOUDA_VL_LAYER_BITMAP){
		GOUDA_AND_WITH_REG(GOUDA_REG_BASE + GD_VL_INPUT_FMT,
			~(GOUDA_VL_LAYER_ACTIVE));
	}

	mask &= GOUDA_OL_LAYER_BITMAP;
	for (i = 0; i < GOUDA_OL_LAYER_NUM; i++) {
		if (mask & (1 << i)){
			value = gouda_ol_layer_get_value(i,GD_OL_INPUT_FMT);
			value &= ~(GOUDA_OL_LAYER_ACTIVE);
			gouda_ol_layer_set_value(i,GD_OL_INPUT_FMT,value);
		}
	}

	return 0;
}

static int rda_gouda_layer_enable(int mask)
{
	int i;
	u32 value;

	if (mask & GOUDA_VL_LAYER_BITMAP){
		GOUDA_OR_WITH_REG(GOUDA_REG_BASE + GD_VL_INPUT_FMT,GOUDA_VL_LAYER_ACTIVE);
	}

	mask &= GOUDA_OL_LAYER_BITMAP;
	for (i = 0; i < GOUDA_OL_LAYER_NUM; i++) {
		if (mask & (1 << i)){
			value = gouda_ol_layer_get_value(i,GD_OL_INPUT_FMT);
			value |= (GOUDA_OL_LAYER_ACTIVE);
			gouda_ol_layer_set_value(i,GD_OL_INPUT_FMT,value);
		}
	}

	return 0;
}

static int rda_gouda_blit(int dst_buf, int dst_offset)
{
	GOUDA_OUTL(GOUDA_REG_BASE + GD_LCD_MEM_ADDRESS,dst_buf);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_LCD_STRIDE_OFFSET,dst_offset);

	/* fixed to 3 for rda8850e */
	GOUDA_OR_WITH_REG(GOUDA_REG_BASE + GD_LCD_CTRL,LCD_IN_RAM);

	rda_gouda_clear_irq_status(GOUDA_EOF_CAUSE);
	pm_ddr_get(PM_DDR_GOUDA_DMA);

	/* Run GOUDA */
	rda_gouda_irq_enable(GOUDA_EOF_MASK);
	rda_gouda_layer_enable(rda_gouda->layer_open_mask);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_COMMAND,GOUDA_START);

	rda_gouda_wait_done();

	//need manually un-active video layer, can not depend on interrtupt
	rda_gouda_ovl_close((1 << GOUDA_VID_LAYER_ID));
	rda_dbg_gouda("dst_buf %x\n",dst_buf);

	return 0;
}

static int rda_gouda_vid_layer_open(struct gouda_vid_layer_def *def, u32 xpitch,
				    u32 ypitch)
{
	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_EXTENTS,
		GOUDA_VL_MAX_COL(def->width - 1) | GOUDA_VL_MAX_LINE(def->height - 1));

	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_Y_SRC,(u32) def->addr_y);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_U_SRC,(u32) def->addr_u);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_V_SRC,(u32) def->addr_v);
	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_RESC_RATIO,
		(GOUDA_VL_XPITCH(xpitch) | GOUDA_VL_YPITCH(ypitch)) | GOUDA_VL_PREFETCH_ENABLE);

	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_INPUT_FMT,
		GOUDA_VL_FORMAT(def->input_fmt) | GOUDA_VL_STRIDE(def->stride) |
		(def->input_fmt == GOUDA_IMG_FORMAT_RGBA ? (GOUDA_VL_FORMAT_ARGB | GOUDA_VL_RGB_SWAP) : 0));

	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_TL_PPOS,
		GOUDA_X0(def->rect.tlx) | GOUDA_Y0(def->rect.tly));

	GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_BR_PPOS,
		GOUDA_X1(def->rect.brx) | GOUDA_Y1(def->rect.bry));

	return 0;
}

static int rda_gouda_osd_layer_open(struct gouda_ovl_layer_var *ol)
{
	u8 wpp;
	u8 alpha;
	u8 id = ol->ovl_id;
	u16 stride;
	u32 blend_var;
	u32 value = 0;

	if (id > GOUDA_OVL_LAYER_ID4) {
		pr_err("no such osd layer used id %d\r\n", id);
		return -1;
	}

	if ((gouda_ol_layer_get_value(id,GD_OL_INPUT_FMT) & GOUDA_OL_LAYER_ACTIVE) != 0) {
		pr_err("gouda_osd_layer_open, layer%d used\r\n", id);
		return -1;
	}

	wpp = (ol->input_fmt == GOUDA_IMG_FORMAT_RGBA) ? 2 : 1;

	if (ol->buf_var.stride)
		stride = ol->buf_var.stride;
	else
		stride = (ol->ovl_rect.brx - ol->ovl_rect.tlx + 1) * wpp;

	// TODO check supported format
	switch(ol->input_fmt) {
	case GOUDA_IMG_FORMAT_RGB565:
		value = GOUDA_OL_FORMAT_RGB565;
		break;
	case GOUDA_IMG_FORMAT_RGBA:
		value = (GOUDA_OL_FORMAT_ARGB8888 | GOUDA_OL_RGB_SWA);
		break;
	case GOUDA_IMG_FORMAT_UYVY:
		break;
	case GOUDA_IMG_FORMAT_YUYV:
		break;
	case GOUDA_IMG_FORMAT_IYUV:
		break;
	default:
		value = (GOUDA_OL_FORMAT_ARGB8888 | GOUDA_OL_RGB_SWA);
		rda_dbg_gouda("ol layer do not supported input format\n");
		break;
	}

	gouda_ol_layer_set_value(id,GD_OL_INPUT_FMT,value | GOUDA_OL_STRIDE(stride)
		 | GOUDA_OL_PREFETCH_ENABLE);

	alpha = 255 - ol->blend_var.alpha;
	ol->blend_var.alpha = alpha;

	blend_var = GOUDA_OL_CHROMA_KEY_EN(ol->blend_var.chroma_key_en) |
		GOUDA_OL_ALPHA(ol->blend_var.alpha);

	gouda_ol_layer_set_value(id,GD_OL_BLEND_OPT,blend_var);
	gouda_ol_layer_set_value(id,GD_OL_TL_PPOS,
		GOUDA_X0(ol->ovl_rect.tlx) | GOUDA_Y0(ol->ovl_rect.tly));

	gouda_ol_layer_set_value(id,GD_OL_BR_PPOS,
		GOUDA_X1(ol->ovl_rect.brx) | GOUDA_Y1(ol->ovl_rect.bry));

	gouda_ol_layer_set_value(id,GD_OL_RGB_SRC,(u32) (ol->buf_var.buf + ol->buf_var.src_offset * wpp));
	rda_dbg_gouda("ion ol buf 0x%x\n",ol->buf_var.buf);
	rda_dbg_gouda("osd src_offset %d\n", ol->buf_var.src_offset);

	return 0;
}

int rda_gouda_ovl_blend(struct gouda_layer_blend_var *layer)
{
	rda_gouda->layer_open_mask = layer->layer_mask;
	rda_dbg_gouda("layer_open_mask %x\n",  rda_gouda->layer_open_mask);

	//rda_dbg_gouda("%s layer->dst_buf 0x%x layer->roi.tlx,%d layer->roi.bry %x\n",__func__,layer->dst_buf,layer->roi.tlx,layer->roi.bry);
	//rda_dbg_gouda("%s layer->roi.tly,%d layer->roi.brx %x\n",__func__,layer->roi.tly,layer->roi.brx);
	rda_gouda_set_roi(layer->blend_roi.tlx, layer->blend_roi.tly, layer->blend_roi.brx, layer->blend_roi.bry, 0);
	rda_gouda->is_active = true;

	rda_gouda_blit(layer->buf_var.buf, layer->buf_var.dst_offset);

	return 0;
}

int rda_gouda_ovl_close(int layer_mask)
{
	rda_gouda->layer_open_mask &= ~(layer_mask);
	rda_gouda_layer_disable(layer_mask);

	return 0;
}

void rda_gouda_set_osd_layer(struct gouda_ovl_layer_var *ol)
{
	rda_gouda_osd_layer_open(ol);
}

static inline u32 get_blend_opt_var(struct gouda_blend_var *var,int id)
{
	u32 value = 0;

	if(id == GOUDA_VID_LAYER_ID){
		value = GOUDA_VL_CHROMA_KEY_EN(var->chroma_key_en) | GOUDA_VL_ALPHA(var->alpha) |
			GOUDA_VL_DEPTH(var->rotation);
	} else {
		value = GOUDA_OL_CHROMA_KEY_EN(var->chroma_key_en) | GOUDA_OL_ALPHA(var->alpha);
	}

	return value;
}

static inline void set_blend_opt_var(struct gouda_blend_var *blend_var,u32 value,int id)
{
	if(id == GOUDA_VID_LAYER_ID){
		blend_var->chroma_key_en = GOUDA_VL_CHROMA_KEY_EN_V(value);
		blend_var->alpha = GOUDA_VL_ALPHA_V(value);
		blend_var->depth = GOUDA_VL_DEPTH_V(value);
		blend_var->rotation = GOUDA_VL_ROTATION_V(value);
	} else {
		blend_var->chroma_key_en = GOUDA_OL_CHROMA_KEY_EN_V(value);
		blend_var->alpha = GOUDA_OL_ALPHA_V(value);
	}
}

void rda_gouda_set_ovl_var(struct gouda_blend_var *var, int id)
{
	u32 value = get_blend_opt_var(var,id);

	if (id == GOUDA_VID_LAYER_ID) {
		GOUDA_OUTL(GOUDA_REG_BASE + GD_VL_BLEND_OPT,value);
		GOUDA_OR_WITH_REG(GOUDA_REG_BASE + DCT_SHIFT_UV_REG1,
			ACCELERATE_VIDOE_LAYAER_FLAG);
	} else {
		gouda_ol_layer_set_value(id,GD_OL_BLEND_OPT,value);
		GOUDA_AND_WITH_REG(GOUDA_REG_BASE + DCT_SHIFT_UV_REG1,
			(~ACCELERATE_VIDOE_LAYAER_FLAG));
	}
}

void rda_gouda_get_ovl_var(struct gouda_blend_var *var, int id)
{
	u32 value;
	if (id == GOUDA_VID_LAYER_ID) {
		value = GOUDA_INL(GOUDA_REG_BASE + GD_VL_BLEND_OPT);
	} else {
		value = gouda_ol_layer_get_value(id,GD_OL_BLEND_OPT);
	}

	set_blend_opt_var(var,value,id);
}

void rda_gouda_set_vid_layer(struct gouda_vl_input *src,
					struct gouda_vl_output *dst)
{
	struct gouda_vid_layer_def gouda_vid_def = {0};
	struct gouda_blend_var blend_var;
	int out_width, out_height, in_width, in_height;
	int offset_y, offset_u, offset_v, stride, rotation;
	int offset = src->buf_var.src_offset;
	u8 bpp;
	u32 xpitch, ypitch;

	rda_dbg_gouda("vid src_offset %d\n", src->buf_var.src_offset);
	bpp = (src->input_fmt == GOUDA_IMG_FORMAT_IYUV) ? 1 :
		(src->input_fmt == GOUDA_IMG_FORMAT_RGB565) ? 2 : 4;/* 4 == ARGB8888 */

	stride = gouda_vid_def.stride =
	    src->buf_var.stride ? : src->width * bpp;

	if (dst->vl_rect.tlx > dst->vl_rect.brx || dst->vl_rect.tly > dst->vl_rect.bry)
		pr_err("bad output roi tlx %d tly %d brx %d bry %d\n",
			   dst->vl_rect.tlx, dst->vl_rect.tly, dst->vl_rect.brx,
			   dst->vl_rect.bry);

	gouda_vid_def.input_fmt = src->input_fmt;
	gouda_vid_def.output_fmt = dst->output_fmt;
	if(dst->output_fmt == GOUDA_OUTPUT_FORMAT_RGB565){
		GOUDA_AND_WITH_REG(GOUDA_REG_BASE + GD_LCD_CTRL,GOUDA_OUTPUT_FORMAT_MASK);
		GOUDA_OR_WITH_REG(GOUDA_REG_BASE + GD_LCD_CTRL,GOUDA_OUTPUT_FORMAT_16_BIT_RGB565);
	}else if(dst->output_fmt == GOUDA_OUTPUT_FORMAT_RGB888){
		GOUDA_AND_WITH_REG(GOUDA_REG_BASE + GD_LCD_CTRL,GOUDA_OUTPUT_FORMAT_MASK);
		GOUDA_OR_WITH_REG(GOUDA_REG_BASE + GD_LCD_CTRL,GOUDA_OUTPUT_FORMAT_24_BIT_RGB888);
	}
	rotation = dst->vl_rot;
	rda_gouda_get_ovl_var(&blend_var, GOUDA_VID_LAYER_ID);
	blend_var.rotation |= GOUDA_VL_ROTATION(rotation);
	rda_gouda_set_ovl_var(&blend_var, GOUDA_VID_LAYER_ID);
	/* stride calc in open() */
	gouda_vid_def.width = src->width;
	gouda_vid_def.height = src->height;

	in_width = src->width;
	in_height = src->height;
	out_width = dst->vl_rect.brx - dst->vl_rect.tlx + 1;
	out_height = dst->vl_rect.bry - dst->vl_rect.tly + 1;

	gouda_vid_def.rect.tlx = dst->vl_rect.tlx;
	gouda_vid_def.rect.tly = dst->vl_rect.tly;
	gouda_vid_def.rect.brx = dst->vl_rect.brx;
	gouda_vid_def.rect.bry = dst->vl_rect.bry;

	offset_y = offset_u = offset_v = 0;
	if (rotation == GOUDA_VID_NO_ROTATION) {
		offset_y = offset_u = offset_v = 0;
	} else if (rotation == GOUDA_VID_90_ROTATION) {
		offset_y = stride * (src->height - 1);
		offset_u = offset_v = stride / 2 * (src->height / 2 - 1);
		SWAP_INT(in_width, in_height);
	} else if (rotation == GOUDA_VID_180_ROTATION) {
		offset_y = stride * (src->height - 1);
		offset_u = offset_v = stride / 2 * (src->height / 2 - 1);
	} else if (rotation == GOUDA_VID_270_ROTATION) {
		offset_y = stride - ((src->input_fmt ==
				GOUDA_IMG_FORMAT_IYUV) ? 8 : 16);
		offset_u = offset_v = offset_y / 2;
		SWAP_INT(in_width, in_height);
	}

	gouda_vid_def.addr_y = (u32 *) (src->buf_y + offset_y + offset);
	gouda_vid_def.addr_u = (u32 *) (src->buf_u + offset_u + offset);
	gouda_vid_def.addr_v = (u32 *) (src->buf_v + offset_v + offset);

	xpitch = (in_width << 8) / out_width;
	ypitch = (in_height << 8) / out_height;
	rda_dbg_gouda("xpitch %d ypitch %d  buffer %p stride %d\n",
		xpitch, ypitch, gouda_vid_def.addr_y, stride);
	/* open the layer */
	rda_gouda_vid_layer_open(&gouda_vid_def, xpitch, ypitch);
}

void rda_gouda_stretch_blit(struct gouda_vl_input *src,
					struct gouda_vl_output *dst)
{
	struct gouda_layer_blend_var var;

	if (!rda_gouda->enabled) {
	    pr_err("%s disable!\n", __func__);
	    return;
	}

	if ((GOUDA_INL(GOUDA_REG_BASE + GD_STATUS) & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
		wait_event_timeout(rda_gouda->wait_eof,	!rda_gouda->is_active, HZ / 2);
		if ((GOUDA_INL(GOUDA_REG_BASE + GD_STATUS) & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
			pr_err("%s: gouda busy! status = 0x%x\n",
				__func__, GOUDA_INL(GOUDA_REG_BASE + GD_STATUS));
			return;
		}
	}

	rda_gouda_set_vid_layer(src, dst);
	memset(&var, 0, sizeof(var));
	var.layer_mask = GOUDA_VL_LAYER_BITMAP;
	var.buf_var.buf = dst->buf_var.buf;
	var.buf_var.addr_type = dst->buf_var.addr_type;
	memcpy(&var.blend_roi, &dst->vl_rect, sizeof(struct gouda_rect));

	rda_gouda_ovl_blend(&var);
}

static int rda_gouda_layer_init(void)
{
	struct gouda_blend_var var = { 0 };

	var.alpha = 0xff;
	//let framebuffer connect to osd0, videolayer behind
	var.depth = GOUDA_VID_LAYER_BEHIND_ALL;
	rda_gouda_set_ovl_var(&var, GOUDA_VID_LAYER_ID);
	//set osd0 alpha to 0xff by default
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID0);
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID1);
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID2);
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID3);
	rda_gouda_set_ovl_var(&var, GOUDA_OVL_LAYER_ID4);

	return 0;
}

static irqreturn_t rda_gouda_interrupt(int irq, void *dev_id)
{
	unsigned long irq_flags;
	struct rda_gd *gd = dev_id;
	uint32_t status;

	spin_lock_irqsave(&gd->lock, irq_flags);
	status = rda_gouda_get_irq_status();

	rda_dbg_gouda("rda_gd_interrupt, eof irq %x\n", status);

	if (status & GOUDA_EOF_CAUSE) {
		rda_gouda_irq_clear(GOUDA_EOF_MASK);
		//close all layer
		rda_gouda_layer_disable(gd->layer_open_mask);
		gd->layer_open_mask = 0;
		gd->is_active = false;

		wake_up(&gd->wait_eof);
		pm_ddr_put(PM_DDR_GOUDA_DMA);
		rda_dbg_gouda("rda_gd_interrupt, eof irq\n");
	}

	spin_unlock_irqrestore(&gd->lock, irq_flags);

	return IRQ_HANDLED;
}

static ssize_t rda_gouda_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_gd *gouda = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", gouda->enabled);
}

static int rda_gouda_suspend(struct device *dev)
{
	rda_dbg_gouda("gouda suspend in\n");

	if ((GOUDA_INL(GOUDA_REG_BASE + GD_STATUS) & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
		wait_event_timeout(rda_gouda->wait_eof,
			!rda_gouda->is_active, HZ / 2);
		if ((GOUDA_INL(GOUDA_REG_BASE + GD_STATUS) & (GOUDA_IA_BUSY | GOUDA_LCD_BUSY)) != 0) {
			pr_err("rda_gouda_suspend: gouda busy\n");
			return -1;
		}
	}

	rda_dbg_gouda("gouda suspend out\n");

	return 0;
}

static int rda_gouda_resume(struct device *dev)
{
	return 0;
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

	spin_lock_init(&gd->lock);
	init_waitqueue_head(&gd->wait_eof);
	platform_set_drvdata(pdev, gd);

	gd->reg_base = ioremap(mem->start, resource_size(mem));
	if (!gd->reg_base) {
		dev_err(&pdev->dev, "ioremap fail\n");
		ret = -ENOMEM;
		goto err_iomap_fail;
	}

	gouda_reg_base = gd->reg_base;

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

	gd->enabled = 1;
	ret = sysfs_create_group(&pdev->dev.kobj, &rda_gouda_attr_group);
	if (ret < 0) {
		goto err_sysfs_create_group;
	}
	rda_gouda_layer_init();

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

MODULE_AUTHOR("RDA rdamicro.com");
MODULE_DESCRIPTION("The GOUDA controller driver for RDA Linux");
