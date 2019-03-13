#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include <mach/hardware.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <plat/reg_gouda.h>
#include <plat/ap_clk.h>
#include <plat/boot_mode.h>
#include <plat/pm_ddr.h>

#include "rda_gouda.h"
#include "rda_panel.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_FB_RDA_USE_ION
#include <asm/cacheflush.h>
#include "../drivers/staging/android/ion/ion.h"
#include "../drivers/staging/android/ion/ion_priv.h"
extern struct ion_device *rda_IonDev;
#endif

#ifdef CONFIG_PM
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#endif /* CONFIG_PM */

struct rda_fb {
	struct mutex lock;
	wait_queue_head_t wait;
	int mode_user_count;
	enum gouda_work_mode mode;
	int rotation;
	struct fb_info fb;
#ifdef CONFIG_FB_RDA_USE_ION
	struct ion_device *idev;
#endif
	u32 palette[16];

	struct rda_lcd_info *lcd_info;
	unsigned long vid_comp_mem;
	int total_mem_size;

	int isResume;
	struct clk * gouda_clk;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_ops;
#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_PM
	/* Control voltage of lcd */
	struct regulator *lcd_reg;
#endif /* CONFIG_PM */

	dma_addr_t fbpbackaddr;
	char __iomem * screen_base_backup;

	int enabled;

};

static struct platform_device rda_fb_device;
static BLOCKING_NOTIFIER_HEAD(rda_fb_notifier_list);

int rda_fb_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&rda_fb_notifier_list, nb);
}

int rda_fb_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&rda_fb_notifier_list, nb);
}

static inline u32 convert_bitfield(int val, struct fb_bitfield *bf)
{
	unsigned int mask = (1 << bf->length) - 1;

	return (val >> (16 - bf->length) & mask) << bf->offset;
}

static int rda_fb_setcolreg(unsigned int regno, unsigned int red,
			    unsigned int green, unsigned int blue,
			    unsigned int transp, struct fb_info *info)
{
	struct rda_fb *fb = container_of(info, struct rda_fb, fb);

	rda_dbg_gouda("rda_fb_setcolreg: regno = %d, rgbt = %d %d %d %d\n",
		      regno, red, green, blue, transp);

	if (regno < 16) {
		fb->palette[regno] =
		    convert_bitfield(transp, &fb->fb.var.transp)
		    | convert_bitfield(blue, &fb->fb.var.blue)
		    | convert_bitfield(green, &fb->fb.var.green)
		    | convert_bitfield(red, &fb->fb.var.red);
		return 0;
	} else {
		return 1;
	}
}

static int rda_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	rda_dbg_gouda("rda_fb_check_var\n");

	if ((var->rotate & 1) != (info->var.rotate & 1)) {
		if ((var->xres != info->var.yres) ||
		    (var->yres != info->var.xres) ||
		    (var->xres_virtual != info->var.yres) ||
		    (var->yres_virtual > info->var.xres * 2) ||
		    (var->yres_virtual < info->var.xres)) {
			return -EINVAL;
		}
	} else {
		if ((var->xres != info->var.xres) ||
		    (var->yres != info->var.yres) ||
		    (var->xres_virtual != info->var.xres) ||
		    (var->yres_virtual > info->var.yres * 2) ||
		    (var->yres_virtual < info->var.yres)) {
			return -EINVAL;
		}
	}
	if ((var->xoffset != info->var.xoffset) ||
	    (var->bits_per_pixel != info->var.bits_per_pixel) ||
	    (var->grayscale != info->var.grayscale)) {
		return -EINVAL;
	}
	return 0;
}

static int rda_fb_set_par(struct fb_info *info)
{
	struct rda_fb *fb = container_of(info, struct rda_fb, fb);
	struct gouda_rect disp_rect;

	rda_dbg_gouda("rda_fb_set_par\n");

	if (fb->rotation != fb->fb.var.rotate) {
		rda_dbg_gouda("rda_fb_set_par: need to rotate\n");
		info->fix.line_length = info->var.xres * (info->var.bits_per_pixel/8);
		fb->rotation = fb->fb.var.rotate;
	}

	disp_rect.tlX = 0;
	disp_rect.tlY = 0;
	disp_rect.brX = fb->fb.var.xres - 1;
	disp_rect.brY = fb->fb.var.yres - 1;
	if (fb->lcd_info->ops.s_active_win)
		fb->lcd_info->ops.s_active_win(&disp_rect);

	return 0;

}

#define SKIP_ADJ 6
static int rda_fb_pan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct rda_fb *fb = container_of(info, struct rda_fb, fb);
	struct gouda_image src;
	struct gouda_rect src_rect;
	static int skip_cnt = SKIP_ADJ;
	int bytes_per_line;
	int ret;

	rda_dbg_gouda("rda_fb_pan_display, var->yoffset = %d\n", var->yoffset);

	mutex_lock(&fb->lock);

	if (!fb->isResume) {
		mutex_unlock(&fb->lock);
		return 0;
	}

	bytes_per_line = fb->fb.var.xres * (fb->fb.var.bits_per_pixel/8);
	src.buffer = (u16 *) (fb->fb.fix.smem_start
			      + bytes_per_line * var->yoffset);
	src.width = fb->fb.var.xres;
	src.height = fb->fb.var.yres;
	/* src.image_fmt does not care */

	src_rect.tlX = 0;
	src_rect.tlY = 0;
	src_rect.brX = fb->fb.var.xres - 1;
	src_rect.brY = fb->fb.var.yres - 1;

	/* move to set_var */
	//if (fb->lcd_info->ops.s_active_win)
	//      fb->lcd_info->ops.s_active_win(&dst_rect);

	if(fb->screen_base_backup && (rda_get_boot_mode() == BM_NORMAL)) {
		if(skip_cnt == SKIP_ADJ) {
			memcpy(fb->screen_base_backup,fb->fb.screen_base,fb->total_mem_size);
		}
		if(!skip_cnt) {
			struct platform_device *pdev = &rda_fb_device;
			dma_free_writecombine(&pdev->dev, fb->total_mem_size,
				fb->screen_base_backup, fb->fbpbackaddr);
			fb->screen_base_backup = NULL;
		}
		if(fb->screen_base_backup)
			src.buffer = (u16 *) (fb->fbpbackaddr
				+ bytes_per_line * var->yoffset);
		skip_cnt--;
	}

	if (fb->lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DBI)
		rda_gouda_display(&src, &src_rect, 0, 0);
#ifdef CONFIG_FB_RDA_DBI_BLIT_ENABLE
	else {
		struct gouda_vid_blit_strech_var vid_var;
		int bpp = 2;

		/* wait to enable clk */
		ret = rda_gouda_stretch_pre_wait_and_enable_clk();
		if (ret < 0) {
			mutex_unlock(&fb->lock);
			return -EFAULT;
		}

		if (fb->lcd_info->ops.s_active_win)
			fb->lcd_info->ops.s_active_win(&src_rect);

		memset(&vid_var, 0, sizeof(vid_var));
		vid_var.src.bufY = (int)src.buffer;
		vid_var.src.width = src.width;
		vid_var.src.height = src.height;
		vid_var.src.srcOffset = 0;
		vid_var.src.addr_type = PhyAddrType;
		vid_var.src.image_fmt = GOUDA_IMG_FORMAT_RGB565;

		vid_var.src.stride  = src.width * bpp;
		vid_var.dst.pos.tlX = 0;
		vid_var.dst.pos.tlY = 0;
		vid_var.dst.pos.brX = src.width - 1;
		vid_var.dst.pos.brY = src.height - 1;
		vid_var.dst.addr_type = FrmBufIndexType;
#ifdef RDA_FB_FORCE_INVERSE
		vid_var.dst.rot = GOUDA_VID_180_ROTATION;
#endif

		rda_gouda_stretch_blit(&vid_var.src, &vid_var.dst, fb->mode == GOUDA_BLOCKING ? true : false);
	}
#endif
	blocking_notifier_call_chain(&rda_fb_notifier_list, 1, NULL);

	mutex_unlock(&fb->lock);

	rda_dbg_gouda("rda_fb_pan_display, done\n");
	return 0;
}

static int rda_fb_blank(int blank, struct fb_info *info)
{
	struct rda_fb *fb = container_of(info, struct rda_fb, fb);


	if (fb->lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DBI) {
		if (blank == FB_BLANK_POWERDOWN && fb->lcd_info->ops.s_display_off)
			fb->lcd_info->ops.s_display_off();
		else if (blank == FB_BLANK_UNBLANK && fb->lcd_info->ops.s_display_on)
			fb->lcd_info->ops.s_display_on();
		else
		    printk("unknow blank value %d\n", blank);
	}
#ifdef CONFIG_FB_RDA_DBI_BLIT_ENABLE
	else {
		if (fb->gouda_clk) {
			if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
				rda_gouda_stretch_pre_wait_and_enable_clk();
			else
				clk_enable(fb->gouda_clk);
		}


		if (blank == FB_BLANK_POWERDOWN && fb->lcd_info->ops.s_display_off) {
			memset(fb->fb.screen_base, 0, fb->total_mem_size);
			rda_fb_pan_display(&fb->fb.var, &fb->fb);
			mdelay(40);
			fb->lcd_info->ops.s_display_off();
		}
		else if (blank == FB_BLANK_UNBLANK && fb->lcd_info->ops.s_display_on) {
			rda_fb_pan_display(&fb->fb.var, &fb->fb);
			mdelay(40);
			fb->lcd_info->ops.s_display_on();
		}
		else
		    printk("unknow blank value %d\n", blank);

		if (fb->gouda_clk) {
			if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
				rda_gouda_stretch_post_disable_clk();
			else
				clk_disable(fb->gouda_clk);
		}

	}
#endif


	return 0;
}

static int rda_get_phy(struct rda_fb *fb, int fd_from_user, unsigned int *addr, int cacheflg, int addr_type)
{

		if (addr_type == PhyAddrType)
			return 0;
		else if (addr_type == FrmBufIndexType) {
			*addr = fb->fb.fix.smem_start + (fd_from_user) * fb->fb.fix.smem_len/2;
		}
#ifdef CONFIG_FB_RDA_USE_ION
		else {
			struct ion_client *ion_client_rdafb;
			struct ion_handle *ion_handle_rdafb;
			int len;
			int ret = 0;

			rda_dbg_gouda("ion addr fd_from_user %d\n",fd_from_user);

			//ion_client_rdafb = ion_client_create(fb->idev, ION_HEAP_TYPE_SYSTEM_CONTIG, "rda-fb-ion");
			ion_client_rdafb = ion_client_create(fb->idev, ION_HEAP_CARVEOUT_MASK, "rda-fb-ion");
			if (IS_ERR_OR_NULL(ion_client_rdafb)) {
				rda_dbg_gouda("rad-fb ion_client_create error!\n");
				return -1;
			}

			ion_handle_rdafb = ion_import_dma_buf(ion_client_rdafb, fd_from_user);

			if (IS_ERR_OR_NULL(ion_handle_rdafb)) {
				rda_dbg_gouda("rad-fb ion_import_dma_buf error!\n");
				return -1;
			}

			ret = ion_phys(ion_client_rdafb, ion_handle_rdafb, (ion_phys_addr_t *)addr, &len);

			rda_dbg_gouda("ion addr fd_from_user %d addr %x len %d ret %d cacheflg %d\n",fd_from_user, *addr, len, ret, cacheflg);

			if (cacheflg)
				outer_clean_range((phys_addr_t)addr, (phys_addr_t)addr + len);
		}
#endif
		return 0;
}

static unsigned int rda_factory_auto_test(struct rda_fb *fb)
{
	u32 i = 0;
	u32 result = 1;
	u16 color16_r = 0xF800;
	//u16 color16_g = 0x07E0;
	//u16 color16_b = 0x001F;
	u8 buffer[60];
	u32 fbsize = fb->lcd_info->lcd.width * fb->lcd_info->lcd.height * 2;//fill double buffer
	printk("rda_factory_auto_test fbsize=%d bits_per_pixel=%d\n", fbsize, fb->fb.var.bits_per_pixel);

	if (fb->fb.var.bits_per_pixel == 16){
		u16 *fb_buffer =(u16 *)(fb->fb.screen_base);
		for (i = 0; i < fbsize; i++) {
			*fb_buffer++ = color16_r;
		}
	}

	rda_fb_pan_display(&fb->fb.var, &fb->fb);

	if (fb->lcd_info->ops.s_read_fb)
		fb->lcd_info->ops.s_read_fb(buffer);

	if (fb->fb.var.bits_per_pixel == 16){
		for (i = 0; i < 60; i += 2) {
			if ((buffer[i]|((buffer[i+1])<<8)) != color16_r){
				result = 0;
			}
		}
	}

	return result;
}

static int rda_fb_ioctl(struct fb_info *info, unsigned int cmd,
			unsigned long arg)
{
	struct gouda_vid_blit_strech_var vid_var;
	struct gouda_ovl_composition_var ovl_var;
	struct gouda_ovl_buf_var buf_var;
	struct gouda_ovl_open_var layer_var;
	struct gouda_compose_var com;
	struct rda_fb *fb;
	int mode;
	int ret = 0;
	int osdId = 0;
	int reg;
	int layerMask;

	fb = container_of(info, struct rda_fb, fb);

	switch (cmd) {
	case FB_COMPOSE_LAYERS:
		if (copy_from_user
		    (&com, (void __user *)arg, sizeof(com))) {
			return -EFAULT;
		} else {
			layerMask = com.ctl_var.layerMask;
			if (!layerMask) {
			     pr_err("no layer need to compose!\n");
			     return -EFAULT;
			}

			for (; layerMask & (1 << osdId); osdId++){
				if (rda_get_phy(fb, com.osd_var[osdId].buf, &com.osd_var[osdId].buf, com.osd_var[osdId].cacheflag, com.osd_var[osdId].addr_type))
					return -EFAULT;
				rda_gouda_osd_display(&com.osd_var[osdId]);
			}

			if (layerMask & (1 << GOUDA_VID_LAYER_ID3)) {
				if (rda_get_phy(fb, com.vid_var.src.bufY,
					&com.vid_var.src.bufY,
					com.vid_var.src.cacheflag,
					com.vid_var.src.addr_type)) {
					return -EFAULT;
				}
				rda_gouda_vid_display(&com.vid_var.src, &com.vid_var.dst);
			}

			if (rda_get_phy(fb, com.ctl_var.dstBuf, &com.ctl_var.dstBuf, 0, com.ctl_var.addr_type)) {
				return -EFAULT;
			}

			/* Enable clock of gouda. */
			ret = rda_gouda_stretch_pre_wait_and_enable_clk();
			if (ret < 0) {
				return -EFAULT;
			}
			rda_gouda_ovl_open(&com.ctl_var, true);
		}
		break;
	case FB_SET_VID_BUF:
		if (copy_from_user
		    (&vid_var, (void __user *)arg, sizeof(vid_var))) {
			return -EFAULT;
		} else {

			rda_dbg_gouda
			    ("src: buffer Y %x U %x V %x, stride %d width %d height %d format %d\n",
			     vid_var.src.bufY, vid_var.src.bufU,
			     vid_var.src.bufV, vid_var.src.stride,
			     vid_var.src.width, vid_var.src.height,
			     vid_var.src.image_fmt);
			rda_dbg_gouda("dst: buffer %x\n", vid_var.dst.buf);
			rda_dbg_gouda("dst pos: tlX %d tlY %d brX %d brY %d\n",
				      vid_var.dst.pos.tlX, vid_var.dst.pos.tlY,
				      vid_var.dst.pos.brX, vid_var.dst.pos.brY);

			mutex_lock(&fb->lock);

			//need to initial lcd here
			if (info->fbops->fb_set_par) {
				ret = info->fbops->fb_set_par(info);

				if (ret) {
					rda_dbg_gouda(KERN_WARNING "detected "
						      "fb_set_par error, "
						      "error code: %d\n", ret);
					mutex_unlock(&fb->lock);
					return ret;
				}
			}

			if (rda_get_phy(fb, vid_var.src.bufY, &vid_var.src.bufY, vid_var.src.cacheflag, vid_var.src.addr_type)) {
				mutex_unlock(&fb->lock);
				return -EFAULT;
			}

			rda_gouda_vid_display(&vid_var.src, &vid_var.dst);

			mutex_unlock(&fb->lock);
		}
		break;
	case FB_STRETCH_BLIT:
		if (copy_from_user
		    (&vid_var, (void __user *)arg, sizeof(vid_var))) {
			return -EFAULT;
		} else {
			mutex_lock(&fb->lock);

			ret = rda_gouda_stretch_pre_wait_and_enable_clk();
			if (ret < 0) {
				mutex_unlock(&fb->lock);
				return -EFAULT;
			}

			//need to initial lcd here
			if (info->fbops->fb_set_par) {
				ret = info->fbops->fb_set_par(info);
				if (ret) {
					rda_dbg_gouda(KERN_WARNING "detected "
						      "fb_set_par error, "
						      "error code: %d\n", ret);
					mutex_unlock(&fb->lock);
					return ret;
				}
			}

			if (rda_get_phy(fb, vid_var.src.bufY, &vid_var.src.bufY, vid_var.src.cacheflag, vid_var.src.addr_type)) {
				mutex_unlock(&fb->lock);
				return -EFAULT;
			}
			if (rda_get_phy(fb, vid_var.dst.buf, &vid_var.dst.buf, 0, vid_var.dst.addr_type)) {
				mutex_unlock(&fb->lock);
				return -EFAULT;
			}

			reg = rda_gouda_save_outfmt();

			rda_gouda_stretch_blit(&vid_var.src, &vid_var.dst, true);

			rda_gouda_restore_outfmt(reg);

			mutex_unlock(&fb->lock);
		}
		break;
	case FB_SET_OVL_VAR:
		if (copy_from_user
		    (&ovl_var, (void __user *)arg, sizeof(ovl_var))) {
			return -EFAULT;
		} else {
			rda_gouda_set_ovl_var(&ovl_var.ovl_blend_var,
					      ovl_var.ovl_id);
		}
		break;
	case FB_GET_OVL_VAR:
		if (copy_from_user
		    (&ovl_var, (void __user *)arg, sizeof(ovl_var))) {
			return -EFAULT;
		} else {
			rda_gouda_get_ovl_var(&ovl_var.ovl_blend_var,
					      ovl_var.ovl_id);
		}

		if (copy_to_user((void __user *)arg, &ovl_var, sizeof(ovl_var)))
			return -EFAULT;
		break;
	case FB_SET_OSD_BUF:
		if (copy_from_user
		    (&buf_var, (void __user *)arg, sizeof(buf_var))) {
			return -EFAULT;
		} else {
			if (rda_get_phy(fb, buf_var.buf, &buf_var.buf, buf_var.cacheflag, buf_var.addr_type))
					return -EFAULT;
			rda_gouda_osd_display(&buf_var);
		}
		break;
	case FB_OPEN_LAYER:
		if (copy_from_user
		    (&layer_var, (void __user *)arg, sizeof(layer_var))) {
			return -EFAULT;
		} else {
			if (rda_get_phy(fb, layer_var.dstBuf, &layer_var.dstBuf, 0, layer_var.addr_type))
				return -EFAULT;
			rda_gouda_ovl_open(&layer_var, true);
		}
		break;
	case FB_CLOSE_LAYER:
		if (copy_from_user
		    (&layerMask, (void __user *)arg, sizeof(unsigned int))) {
			return -EFAULT;
		} else {
			rda_gouda_ovl_close(layerMask);
		}
		break;

	case FB_SET_MODE:
		mutex_lock(&fb->lock);
		if (copy_from_user
		    (&mode, (void __user *)arg, sizeof(unsigned int))) {
			mutex_unlock(&fb->lock);
			return -EFAULT;
		} else {
			fb->mode_user_count++;
			if (fb->mode_user_count > 0)
				fb->mode |= mode;
		}
		mutex_unlock(&fb->lock);
		break;
	case FB_CLR_MODE:
		mutex_lock(&fb->lock);
		if (copy_from_user
		    (&mode, (void __user *)arg, sizeof(unsigned int))) {
			mutex_unlock(&fb->lock);
			return -EFAULT;
		} else {
			fb->mode_user_count--;
			if (fb->mode_user_count == 0)
				fb->mode &= ~mode;
		}
		mutex_unlock(&fb->lock);
		break;
	case FB_SET_TE:
		mutex_lock(&fb->lock);
		if (copy_from_user
		    (&mode, (void __user *)arg, sizeof(unsigned int))) {
			mutex_unlock(&fb->lock);
			return -EFAULT;
		} else {
		    printk("set TE: %d\n", mode);
		    rda_gouda_configure_te(&fb->lcd_info->lcd, mode);
		}
		mutex_unlock(&fb->lock);
		break;
	case FB_FACTORY_AUTO_TEST:
	{
		unsigned int result = 0;
		result = rda_factory_auto_test(fb);
		return copy_to_user((void __user *)arg, &result, sizeof(result)) ? -EFAULT : 0;
	}
	default:
		pr_err("bad ioctl cmd: %d\n", cmd);
		return -EINVAL;
	}
	return 0;
}

static struct fb_ops rda_fb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = rda_fb_check_var,
	.fb_set_par = rda_fb_set_par,
	.fb_setcolreg = rda_fb_setcolreg,
	.fb_pan_display = rda_fb_pan_display,
	.fb_blank = rda_fb_blank,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_ioctl = rda_fb_ioctl,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_fb_early_suspend(struct early_suspend *h)
{
	struct rda_fb *fb = container_of(h, struct rda_fb, early_ops);

	mutex_lock(&fb->lock);

	if (fb->gouda_clk) {
		if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
			rda_gouda_stretch_pre_wait_and_enable_clk();
		else
			clk_enable(fb->gouda_clk);
	}

	if (fb->lcd_info->ops.s_sleep) {
		fb->lcd_info->ops.s_sleep();
	}

	if (fb->gouda_clk) {
		if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
			rda_gouda_stretch_post_disable_clk();
		else
			clk_disable(fb->gouda_clk);
	}

	rda_gouda_close_lcd(&fb->lcd_info->lcd);

#ifdef CONFIG_PM
	regulator_disable(fb->lcd_reg);
#endif
	blocking_notifier_call_chain(&rda_fb_notifier_list, 0, NULL);

	fb->isResume = 0;

	mutex_unlock(&fb->lock);
}

static void rda_fb_late_resume(struct early_suspend *h)
{
	struct rda_fb *fb = container_of(h, struct rda_fb, early_ops);

	mutex_lock(&fb->lock);

#ifdef CONFIG_PM
	regulator_enable(fb->lcd_reg);
#endif

	if (fb->gouda_clk) {
		if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
			rda_gouda_stretch_pre_wait_and_enable_clk();
		else
			clk_enable(fb->gouda_clk);
	}

	apsys_reset_gouda();

	rda_gouda_open_lcd(&fb->lcd_info->lcd, fb->vid_comp_mem);

	if (fb->lcd_info->ops.s_wakeup) {
		fb->lcd_info->ops.s_wakeup();
	}

	if (fb->gouda_clk && fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
		rda_gouda_stretch_post_disable_clk();

	fb->isResume = 1;

	mutex_unlock(&fb->lock);
}
#else
static int rda_fb_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rda_fb *fb = platform_get_drvdata(pdev);

	mutex_lock(&fb->lock);

	if (fb->gouda_clk) {
		if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
			rda_gouda_stretch_pre_wait_and_enable_clk();
		else
			clk_enable(fb->gouda_clk);
	}


	if (fb->lcd_info->ops.s_sleep) {
		fb->lcd_info->ops.s_sleep();
	}

	if (fb->gouda_clk) {
		if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
			rda_gouda_stretch_post_disable_clk();
		else
			clk_disable(fb->gouda_clk);
	}

	rda_gouda_close_lcd(&fb->lcd_info->lcd);

#ifdef CONFIG_PM
	regulator_disable(fb->lcd_reg);
#endif
	blocking_notifier_call_chain(&rda_fb_notifier_list, 0, NULL);

	fb->isResume = 0;

	if (fb->lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DBI)
		pm_ddr_put(PM_DDR_FB_DMA);

	mutex_unlock(&fb->lock);

	return 0;
}

static int rda_fb_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rda_fb *fb = platform_get_drvdata(pdev);
#ifdef CONFIG_PM
	int ret = 0;
#endif

	mutex_lock(&fb->lock);

	if (fb->lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DBI)
		pm_ddr_get(PM_DDR_FB_DMA);
#ifdef CONFIG_PM
	ret = regulator_enable(fb->lcd_reg);
#endif

	if (fb->gouda_clk) {
		if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
			rda_gouda_stretch_pre_wait_and_enable_clk();
		else
			clk_enable(fb->gouda_clk);
	}

	apsys_reset_gouda();

	rda_gouda_open_lcd(&fb->lcd_info->lcd, fb->vid_comp_mem);

	if (fb->lcd_info->ops.s_wakeup) {
		fb->lcd_info->ops.s_wakeup();
	}

	if (fb->gouda_clk && fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI)
		rda_gouda_stretch_post_disable_clk();

	fb->isResume = 1;


	mutex_unlock(&fb->lock);

	return 0;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static ssize_t rda_fb_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_fb *fb = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", fb->enabled);
}

static ssize_t rda_fb_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_fb *fb = dev_get_drvdata(dev);
	int ret;
	int set;

	ret = kstrtoint(buf, 0, &set);
	if (ret < 0) {
		return ret;
	}

	set = !!set;

	if (fb->enabled == set) {
		return count;
	}

	if (set) {
		ret = rda_fb_resume(dev);
	} else {
		ret = rda_fb_suspend(dev);
	}

	fb->enabled = set;

	return count;
}

static ssize_t rda_fb_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)

{
	struct rda_fb *fb = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", fb->lcd_info->name);
}

static DEVICE_ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_fb_enable_show, rda_fb_enable_store);

static DEVICE_ATTR(name, S_IRUGO, rda_fb_name_show, NULL);

static struct attribute *rda_fb_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group rda_fb_attr_group = {
	.attrs = rda_fb_attrs,
};

static int rda_fb_probe(struct platform_device *pdev)
{
	int ret;
	struct rda_fb *fb;
	size_t framesize, dma_map_size, size, vid_comp_buf_size;
	uint32_t width, height;
	dma_addr_t fbpaddr;

	if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "lcd info not present\n");
		return -ENODEV;
	}

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (fb == NULL) {
		ret = -ENOMEM;
		goto err_fb_alloc_failed;
	}

#ifdef CONFIG_PM
	fb->lcd_reg = regulator_get(NULL, LDO_LCD);
	if (IS_ERR(fb->lcd_reg)) {
		dev_err(&pdev->dev, "rda-fb not find lcd regulator devices\n");
		ret = -EINVAL;
		goto out_free_dev;
	}

	/*
	 * Have to call enable,
	 * otherwise we cann't call disable action in suspend.
	 */
	ret = regulator_enable(fb->lcd_reg);
	if (ret < 0) {
		dev_err(&pdev->dev, "rda-fb lcd could not be enabled!\n");
	}
#endif /* CONFIG_PM */

#ifdef CONFIG_FB_RDA_USE_ION
	fb->idev = rda_IonDev;
#endif

	mutex_init(&fb->lock);
	init_waitqueue_head(&fb->wait);
	platform_set_drvdata(pdev, fb);

	fb->lcd_info = (struct rda_lcd_info *)pdev->dev.platform_data;

	width = fb->lcd_info->lcd.width;
	height = fb->lcd_info->lcd.height;


	if (fb->lcd_info->lcd.lcd_cfg.rgb.pix_fmt == RDA_FMT_RGB565) {
		fb->fb.var.bits_per_pixel = 16;
	} else if (fb->lcd_info->lcd.lcd_cfg.rgb.pix_fmt == RDA_FMT_RGB888) {
		fb->fb.var.bits_per_pixel = 24;
	} else {
		fb->fb.var.bits_per_pixel = 32;
	}

	fb->mode = (rda_get_boot_mode() == BM_NORMAL)? GOUDA_NONBLOCKING : GOUDA_BLOCKING;
	fb->isResume = 1;

	fb->gouda_clk = clk_get(&pdev->dev, RDA_CLK_GOUDA);
	if (!fb->gouda_clk) {
		printk("failed to get gouda clk controller, but do not treat as error\n");
	}

	//if (fb->gouda_clk && fb->lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DBI)
	if (fb->lcd_info->lcd.lcd_interface != GOUDA_LCD_IF_DBI) {
		pm_ddr_get(PM_DDR_FB_DMA);
	}


	fb->fb.fbops = &rda_fb_ops;
	fb->fb.flags = FBINFO_FLAG_DEFAULT;
	fb->fb.pseudo_palette = fb->palette;
	//strncpy(fb->fb.fix.id, clcd_name, sizeof(fb->fb.fix.id));
	fb->fb.fix.type = FB_TYPE_PACKED_PIXELS;
	fb->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	fb->fb.fix.line_length = width * (fb->fb.var.bits_per_pixel/8);
	fb->fb.fix.accel = FB_ACCEL_NONE;
	fb->fb.fix.ypanstep = 1;

	fb->fb.var.xres = width;
	fb->fb.var.yres = height;
	fb->fb.var.xres_virtual = width;
	fb->fb.var.yres_virtual = height * 2;
	fb->fb.var.activate = FB_ACTIVATE_NOW;
	fb->fb.var.height = -1;
	fb->fb.var.width = -1;
	fb->fb.var.pixclock = 10000;

	if (fb->fb.var.bits_per_pixel == 16) {
		fb->fb.var.red.offset = 11;
		fb->fb.var.red.length = 5;
		fb->fb.var.green.offset = 5;
		fb->fb.var.green.length = 6;
		fb->fb.var.blue.offset = 0;
		fb->fb.var.blue.length = 5;
		fb->fb.var.transp.offset = 0;
		fb->fb.var.transp.length = 0;
	} else if (fb->fb.var.bits_per_pixel == 24){
		fb->fb.var.red.offset = 0;
		fb->fb.var.red.length = 8;
		fb->fb.var.green.offset = 8;
		fb->fb.var.green.length = 8;
		fb->fb.var.blue.offset = 16;
		fb->fb.var.blue.length = 8;
		fb->fb.var.transp.offset = 0;
		fb->fb.var.transp.length = 0;
	} else {
		//to mach vivante BGRA format
		fb->fb.var.red.offset = 16;
		fb->fb.var.red.length = 8;
		fb->fb.var.green.offset = 8;
		fb->fb.var.green.length = 8;
		fb->fb.var.blue.offset = 0;
		fb->fb.var.blue.length = 8;
		fb->fb.var.transp.offset = 24;
		fb->fb.var.transp.length = 8;
	}

	framesize = width * height * 2 * (fb->fb.var.bits_per_pixel/8);
	if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DPI) {
		/*
		 * FB driver needs 3 buffers with DPI interface.
		 * The last buffer is filled with black color that is used as doing resume,
		 * because screen has to blank with black color. */
		size = width * height * (fb->fb.var.bits_per_pixel/8);
	} else {
		size = 0;
	}

	vid_comp_buf_size = PAGE_ALIGN(size);

	dma_map_size = PAGE_ALIGN(framesize + PAGE_SIZE);
	fb->total_mem_size = dma_map_size + vid_comp_buf_size;
	fb->fb.screen_base =
			dma_alloc_writecombine(&pdev->dev,
			fb->total_mem_size,
			&fbpaddr, GFP_KERNEL);
	if (fb->fb.screen_base == 0) {
		ret = -ENOMEM;
		goto err_alloc_screen_base_failed;
	}
	fb->fb.fix.smem_start = fbpaddr;
	fb->fb.fix.smem_len = framesize;

	if (rda_get_boot_mode() == BM_NORMAL){
		fb->screen_base_backup =
				dma_alloc_writecombine(&pdev->dev,
				fb->total_mem_size,
				&fb->fbpbackaddr, GFP_KERNEL);
	}

	rda_dbg_gouda("buffer0 %x buffer1 %x\n", fbpaddr, fbpaddr + size);

	if (fb->lcd_info->lcd.lcd_interface == GOUDA_LCD_IF_DPI) {
		fb->vid_comp_mem = (unsigned long)(fbpaddr + dma_map_size);
		memset(fb->fb.screen_base + dma_map_size, 0, vid_comp_buf_size);
	} else {
		fb->vid_comp_mem = 0;
	}

	ret = fb_set_var(&fb->fb, &fb->fb.var);

	if (ret)
		goto err_fb_set_var_failed;

	rda_fb_set_par(&fb->fb);

	rda_gouda_open_lcd(&fb->lcd_info->lcd, fb->vid_comp_mem);

	if (fb->lcd_info->ops.s_init_gpio)
		fb->lcd_info->ops.s_init_gpio();

	if (fb->lcd_info->ops.s_open)
		fb->lcd_info->ops.s_open();

#ifdef CONFIG_HAS_EARLYSUSPEND
	fb->early_ops.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	fb->early_ops.suspend = rda_fb_early_suspend;
	fb->early_ops.resume = rda_fb_late_resume;

	register_early_suspend(&fb->early_ops);

#endif /* CONFIG_HAS_EARLYSUSPEND */

	pm_set_vt_switch(0);

	ret = register_framebuffer(&fb->fb);
	if (ret)
		goto err_register_framebuffer_failed;

	fb->enabled = 1;
	ret = sysfs_create_group(&pdev->dev.kobj, &rda_fb_attr_group);
	if (ret < 0) {
		goto err_sysfs_create_group;
	}

	dev_info(&pdev->dev,
		 "init done, %d x %d = %d, at %p, phys 0x%08x\n",
		 width, height, dma_map_size,
		 fb->fb.screen_base, (u32)fbpaddr);

	return 0;

err_sysfs_create_group:
	unregister_framebuffer(&fb->fb);

err_register_framebuffer_failed:
err_fb_set_var_failed:
	dma_free_writecombine(&pdev->dev, fb->total_mem_size,
			fb->fb.screen_base, fb->fb.fix.smem_start);

#ifdef CONFIG_PM
out_free_dev:
	if (!IS_ERR(fb->lcd_reg)) {
		regulator_disable(fb->lcd_reg);
		regulator_put(fb->lcd_reg);
	}
#endif /* CONFIG_PM */
err_alloc_screen_base_failed:
	kfree(fb);
err_fb_alloc_failed:
	return ret;
}

static int rda_fb_remove(struct platform_device *pdev)
{
	size_t framesize;
	struct rda_fb *fb = platform_get_drvdata(pdev);

#ifdef CONFIG_FB_RDA_USE_ION
	fb->idev = NULL;
#endif

#ifdef CONFIG_PM
	if (!IS_ERR(fb->lcd_reg)) {
		regulator_put(fb->lcd_reg);
	}
#endif /* CONFIG_PM */

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&fb->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	sysfs_remove_group(&pdev->dev.kobj, &rda_fb_attr_group);

	if (fb->gouda_clk)
		clk_put(fb->gouda_clk);

	rda_gouda_close_lcd(&fb->lcd_info->lcd);

	framesize = fb->fb.var.xres_virtual * fb->fb.var.yres_virtual * (fb->fb.var.bits_per_pixel/8);

	unregister_framebuffer(&fb->fb);
	dma_free_writecombine(&pdev->dev, fb->total_mem_size,
			fb->fb.screen_base, fb->fb.fix.smem_start);

	kfree(fb);
	return 0;
}

static struct platform_driver rda_fb_driver = {
	.probe = rda_fb_probe,
	.remove = rda_fb_remove,
	.driver = {
		   .name = RDA_FB_DRV_NAME,
	}
};

static int __init rda_fb_init(void)
{
	return platform_driver_register(&rda_fb_driver);
}

static void __exit rda_fb_exit(void)
{
	platform_driver_unregister(&rda_fb_driver);
}

module_init(rda_fb_init);
module_exit(rda_fb_exit);

static void rda_fb_release(struct device *dev)
{
}

static u64 rda_fb_dmamask = DMA_BIT_MASK(32);

/* dummy device fb */
static struct platform_device rda_fb_device = {
	.name = RDA_FB_DRV_NAME,
	.id = -1,
	.dev = {
		.dma_mask = &rda_fb_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.release = rda_fb_release,
		},
};

int rda_fb_register_panel(struct rda_lcd_info *p_lcd_info)
{
	struct platform_device *pdev = &rda_fb_device;
	int ret;

	BUG_ON(!p_lcd_info);

	pdev->dev.platform_data = (void *)p_lcd_info;

	ret = platform_device_register(&rda_fb_device);
	if (ret) {
		dev_err(&pdev->dev, "can't register rda_fb device\n");
		return ret;
	}

	return 0;
}

EXPORT_SYMBOL(rda_fb_register_panel);

void * rda_fb_get_device(void)
{
	return &rda_fb_device;
}
