#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include <mach/hardware.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <plat/rda_display.h>
#include <plat/ap_clk.h>
#include <plat/boot_mode.h>
#include <plat/pm_ddr.h>

#ifdef CONFIG_FB_RDA_USE_HWC
#include <linux/file.h>
#include <linux/syscalls.h>
#include "sw_sync.h"
#endif

#include "rda_gouda.h"
#include "rda_lcdc.h"
#include "rda_panel.h"
#include "rda_fb.h"

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

	rda_dbg_lcdc("rda_fb_setcolreg: regno = %d, rgbt = %d %d %d %d\n",
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
	rda_dbg_lcdc("rda_fb_check_var\n");

	if ((var->rotate & 1) != (info->var.rotate & 1)) {
		if ((var->xres != info->var.yres) ||
		    (var->yres != info->var.xres) ||
		    (var->xres_virtual != info->var.yres) ||
		    (var->yres_virtual > info->var.xres * NUM_FRAMEBUFFERS) ||
		    (var->yres_virtual < info->var.xres)) {
			return -EINVAL;
		}
	} else {
		if ((var->xres != info->var.xres) ||
		    (var->yres != info->var.yres) ||
		    (var->xres_virtual != info->var.xres) ||
		    (var->yres_virtual > info->var.yres * NUM_FRAMEBUFFERS) ||
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
	struct lcd_img_rect disp_rect;

	rda_dbg_lcdc("rda_fb_set_par\n");

	if (fb->rotation != fb->fb.var.rotate) {
		rda_dbg_lcdc("rda_fb_set_par: need to rotate\n");
		info->fix.line_length = info->var.xres * (info->var.bits_per_pixel >> 3);
		fb->rotation = fb->fb.var.rotate;
	}

	disp_rect.tlx = 0;
	disp_rect.tly = 0;
	disp_rect.brx = fb->fb.var.xres - 1;
	disp_rect.bry = fb->fb.var.yres - 1;
	if (fb->lcd_info->ops.s_active_win)
		fb->lcd_info->ops.s_active_win(&disp_rect);

	return 0;

}

static int rda_fb_pan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct rda_fb *rdafb = container_of(info, struct rda_fb, fb);
	struct rda_lcd_info *lcd_info = rdafb->lcd_info;
	struct lcd_img_rect img_rect;
	int bytes_per_line;
	//int lcd_state;

	rda_dbg_lcdc("rda_fb_pan_display, var->yoffset = %d\n", var->yoffset);

	mutex_lock(&rdafb->lock);
	bytes_per_line = rdafb->fb.var.xres * (rdafb->fb.var.bits_per_pixel >> 3);
	rdafb->fb_addr = (rdafb->fb.fix.smem_start
			      + bytes_per_line * var->yoffset);

	img_rect.tlx = 0;
	img_rect.tly = 0;
	img_rect.brx = rdafb->fb.var.xres - 1;
	img_rect.bry = rdafb->fb.var.yres - 1;

	if (lcd_info->ops.s_active_win)
		lcd_info->ops.s_active_win(&img_rect);
#if 0
	if(lcd_info->lcd.mipi_pinfo.debug_mode == DBG_NEED_CHK
		&& lcd_info->ops.s_check_lcd_state){
		again:
		/* rda_lcdc_display(rdafb); */
		lcd_state = lcd_info->ops.s_check_lcd_state((void*)lcd_info);
		rda_lcdc_enable_mipi(rdafb);
		if(lcd_state){
			goto again;
		}
	}
#endif
	rda_lcdc_display(rdafb);

#ifdef CONFIG_FB_RDA_USE_HWC
	//if(rdafb->enable_sync){
		sw_sync_timeline_inc(rdafb->timeline, 1);
	//}
#endif
	blocking_notifier_call_chain(&rda_fb_notifier_list, 1, NULL);
	mutex_unlock(&rdafb->lock);

	return 0;
}

static int rda_fb_blank(int blank, struct fb_info *info)
{
	struct rda_fb *fb = container_of(info, struct rda_fb, fb);

	switch(blank) {
	case FB_BLANK_UNBLANK:
		if(fb->lcd_info->ops.s_display_on)
			fb->lcd_info->ops.s_display_on();
		break;
	case FB_BLANK_POWERDOWN:
	default:
		if(fb->lcd_info->ops.s_display_off)
			fb->lcd_info->ops.s_display_off();
		break;
	}

	return 0;
}

#if RDA_HW_ON_DEBUG
static  void save_ion_bin(char *addr,int count)
{
	mm_segment_t old_fs;
	struct file *file_ion=NULL;
	static int index = 0;
	char file_name[40];

	printk("%s line %d\n",__func__,__LINE__);
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file_name[39] = '\0';
	sprintf(file_name,"/sdcard/rgb_ion_%d.raw",index);
	printk("file name %s\n",file_name);
	file_ion = filp_open(file_name, O_WRONLY|O_CREAT, 0644);

	if (!IS_ERR(file_ion)) {
		printk("%s file open ok addr %p len %d\n",__func__,addr,count);
        file_ion->f_op->write(file_ion, addr, count, &file_ion->f_pos);
        set_fs(old_fs);
		index++;
        filp_close(file_ion, NULL);
    } else {
		printk("%s file open error \n",__func__);
	}

}
#endif

static int rda_get_phy(struct rda_fb *fb, struct gouda_layer_buf *phy_var)
{
	if (phy_var->addr_type == PHY_ADDR_TYPE) {
		return 0;
	} else if (phy_var->addr_type == FB_INDEX_TYPE) {
		phy_var->buf = fb->fb.fix.smem_start +
			(phy_var->fbbuf.fb_index) * (fb->fb.fix.smem_len / (NUM_FRAMEBUFFERS));
	} else if (phy_var->addr_type == ION_TYPE) {
#ifdef CONFIG_FB_RDA_USE_ION
		struct ion_handle *ion_handle_rdafb;
#if RDA_HW_ON_DEBUG
		void *ionaddr;
#endif
		int len;
		int ret = 0;

		if(!fb->ion_client)
			fb->ion_client = ion_client_create(rda_IonDev,"rda-fb-ion");

		if (IS_ERR_OR_NULL(fb->ion_client)) {
			rda_dbg_gouda("rad-fb ion_client_create error!\n");
			return -1;
		}

		ion_handle_rdafb = ion_import_dma_buf(fb->ion_client, phy_var->ionbuf.ion_fd);

		if (IS_ERR_OR_NULL(ion_handle_rdafb)) {
			rda_dbg_gouda("rad-fb ion_import_dma_buf error!\n");
			return EINVAL;
		}

		ret = ion_phys(fb->ion_client, ion_handle_rdafb, (ion_phys_addr_t *)&phy_var->buf, &len);
#if RDA_HW_ON_DEBUG
		ionaddr = ion_map_kernel(fb->ion_client,ion_handle_rdafb);
		save_ion_bin((char *)ionaddr,len);
#endif

		ion_free(fb->ion_client, ion_handle_rdafb);
#endif
	}
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
	u32 fbsize = fb->lcd_info->lcd.width * fb->lcd_info->lcd.height * NUM_FRAMEBUFFERS;//fill double buffer
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

#ifdef CONFIG_FB_RDA_USE_HWC
/*
int rda_fb_signal_timeline(struct rda_fb *fb)
{
	mutex_lock(&fb->sync_lock);
	if (fb->timeline) {
		sw_sync_timeline_inc(fb->timeline, 1);
		fb->timeline_value++;
		//printk("%s timeline value %d\n",__func__,fb->timeline_value);
	}
	mutex_unlock(&fb->sync_lock);
	return 0;
}
*/

static int rda_fb_wait_fence(struct rda_fb *fb)
{
	int i, ret = 0;

	/* buf sync */
	for (i = 0; i < fb->acq_fence_cnt; i++) {
		ret = sync_fence_wait(fb->acq_fence[i], 1000);
		sync_fence_put(fb->acq_fence[i]);
		if (ret < 0) {
			pr_err("%s: sync_fence_wait failed! ret = %x\n",
				__func__, ret);
			break;
		}
	}
	fb->acq_fence_cnt = 0;
	return ret;
}

static int rda_handle_hw_buf_sync(struct rda_fb *fb,
						struct gouda_buf_sync *buf_sync)
{
	int i, fence_cnt = 0, ret = 0,fd;
	int acq_fence_fd[RDA_MAX_FENCE_FD];
	struct sync_fence *fence;
	struct sync_fence *rel_fence;
	struct sync_pt *pt;
	struct sw_sync_pt *sw_pt;

	rda_dbg_gouda("%s \n",__func__);
	fb->enable_sync = true;
	if ((buf_sync->acq_fence_cnt > RDA_MAX_FENCE_FD) ||
		(fb->timeline == NULL))
		return -EINVAL;

	if (!fb->enabled)
		return -EPERM;

	if (buf_sync->acq_fence_cnt)
		ret = copy_from_user(acq_fence_fd, buf_sync->acq_fence_fd,
				buf_sync->acq_fence_cnt * sizeof(int));
	if (ret) {
		pr_err("%s:copy_from_user failed", __func__);
		return ret;
	}

	mutex_lock(&fb->sync_lock);
	for (i = 0; i < buf_sync->acq_fence_cnt; i++) {
		fence = sync_fence_fdget(acq_fence_fd[i]);
		if (fence == NULL) {
			pr_err("%s: null fence! i=%d fd=%d\n", __func__, i,
				acq_fence_fd[i]);
			ret = -EINVAL;
			break;
		}
		fb->acq_fence[i] = fence;
	}
	fb->acq_fence_cnt = fence_cnt = i;
	if (ret)
		goto buf_sync_err_1;

	rda_fb_wait_fence(fb);
	fb->timeline_value++;
	pt = sw_sync_pt_create(fb->timeline, fb->timeline_value + 1);
	if (pt == NULL) {
		pr_err("%s: cannot create sync point", __func__);
		ret = -ENOMEM;
		goto buf_sync_err_1;
	}
	sw_pt = (struct sw_sync_pt *)pt;
	rel_fence = sync_fence_create("rda-fence",pt);
	if (rel_fence == NULL) {
		sync_pt_free(pt);
		pr_err("%s: cannot create fence", __func__);
		ret = -ENOMEM;
		goto buf_sync_err_1;
	}

	fd = get_unused_fd();
	if (fd < 0) {
		pr_err("%s: get_unused_fd_flags failed", __func__);
		ret  = -EIO;
		goto buf_sync_err_2;
	}

	sync_fence_install(rel_fence, fd);
	ret = copy_to_user(buf_sync->rel_fence_fd,
		&fd, sizeof(int));
	if (ret) {
		pr_err("%s:copy_to_user failed", __func__);
		goto buf_sync_err_3;
	}
	mutex_unlock(&fb->sync_lock);

	return ret;
buf_sync_err_3:
	put_unused_fd(fd);
buf_sync_err_2:
	sync_fence_put(rel_fence);
	rel_fence = NULL;
	fd = 0;
buf_sync_err_1:
	for (i = 0; i < fence_cnt; i++)
		sync_fence_put(fb->acq_fence[i]);
	fb->acq_fence_cnt = 0;
	mutex_unlock(&fb->sync_lock);
	return ret;
}
#endif

static int rda_fb_ioctl(struct fb_info *info, unsigned int cmd,
			unsigned long arg)
{
	struct gouda_ovl_layer_var ovl_var;
	struct gouda_vid_layer_var vid_var;
	struct gouda_layer_blend_var bl_var;
	struct gouda_compose_var com;
#ifdef CONFIG_FB_RDA_USE_HWC
	int enable = 0;
	struct gouda_buf_sync buf_sync;
#endif
	struct rda_fb *fb;
	int i;
	int layer_mask;
#ifdef CONFIG_FB_RDA_USE_HWC
	int ret;
#endif

	fb = container_of(info, struct rda_fb, fb);

	switch (cmd) {
	case FB_COMPOSE_LAYERS:
		if (copy_from_user(&com, (void __user *)arg, sizeof(com))) {
			return -EFAULT;
		}

		layer_mask = com.ctl_var.layer_mask;
		if (!layer_mask) {
		     pr_err("No layer need to compose!\n");
		     return -EFAULT;
		}

		/* overlay layers */
		for (i = 0; i < GOUDA_VID_LAYER_ID; i++) {
			if(layer_mask & (1 << i)){
				if (rda_get_phy(fb, &com.ovl_var[i].buf_var))
					return -EFAULT;
				rda_gouda_set_osd_layer(&com.ovl_var[i]);
			}
		}

		/* video layer */
		if (layer_mask & GOUDA_VL_LAYER_BITMAP) {
			if (rda_get_phy(fb, &com.vid_var.src.buf_var)) {
				return -EFAULT;
			}
			com.vid_var.src.buf_y = com.vid_var.src.buf_var.buf;/* vid src */
			rda_gouda_set_vid_layer(&com.vid_var.src, &com.vid_var.dst);
		}

		/* blend all layers */
		if (rda_get_phy(fb, &com.ctl_var.buf_var)) {
			return -EFAULT;
		}

		rda_gouda_enable_clk(true);
		rda_gouda_ovl_blend(&com.ctl_var);
		rda_gouda_enable_clk(false);
		break;
	case FB_SET_VID_BUF:
		if(copy_from_user(&vid_var, (void __user *)arg, sizeof(vid_var))) {
			return -EFAULT;
		}
		rda_dbg_gouda
		    ("src: buffer Y %x U %x V %x, stride %d width %d height %d format %d addr_type %d\n",
		     vid_var.src.buf_y, vid_var.src.buf_u,
		     vid_var.src.buf_v, vid_var.src.buf_var.stride,
		     vid_var.src.width, vid_var.src.height,
		     vid_var.src.input_fmt,vid_var.src.buf_var.addr_type);
		rda_dbg_gouda("dst: buffer %x\n", vid_var.dst.buf_var.buf);
		rda_dbg_gouda("dst pos: tlx %d tly %d brx %d bry %d\n",
			      vid_var.dst.vl_rect.tlx, vid_var.dst.vl_rect.tly,
			      vid_var.dst.vl_rect.brx, vid_var.dst.vl_rect.bry);

		mutex_lock(&fb->gouda_lock);

		if (rda_get_phy(fb, &vid_var.src.buf_var)) {
			mutex_unlock(&fb->gouda_lock);
			return -EFAULT;
		}

		vid_var.src.buf_y = vid_var.src.buf_var.buf;/* vid src */
		rda_gouda_set_vid_layer(&vid_var.src, &vid_var.dst);

		mutex_unlock(&fb->gouda_lock);
		break;
	case FB_SET_OSD_BUF:
		if (copy_from_user (&ovl_var, (void __user *)arg, sizeof(ovl_var))) {
			return -EFAULT;
		}

		mutex_lock(&fb->gouda_lock);
		if (rda_get_phy(fb, &ovl_var.buf_var)) {
			mutex_unlock(&fb->gouda_lock);
			return -EFAULT;
		}

		rda_gouda_set_osd_layer(&ovl_var);
		mutex_unlock(&fb->gouda_lock);
		break;
	case FB_BLEND_LAYER:
		if (copy_from_user(&bl_var, (void __user *)arg, sizeof(bl_var))) {
			return -EFAULT;
		}

		mutex_lock(&fb->gouda_lock);
		if (rda_get_phy(fb, &bl_var.buf_var)){
			mutex_unlock(&fb->gouda_lock);
			return -EFAULT;
		}

		rda_gouda_enable_clk(true);
		rda_gouda_ovl_blend(&bl_var);
		rda_gouda_enable_clk(false);
		mutex_unlock(&fb->gouda_lock);
		break;
	case FB_CLOSE_LAYER:
		if (copy_from_user
		    (&layer_mask, (void __user *)arg, sizeof(unsigned int))) {
			return -EFAULT;
		} else {
			rda_gouda_ovl_close(layer_mask);
		}
		break;
#ifdef CONFIG_FB_RDA_USE_HWC
	case FB_HW_SYNC_EN:
		if (copy_from_user(&enable, (void __user *)arg, sizeof(enable))) {
			return -EFAULT;
		}

		rda_lcdc_set_vsync(enable);
		break;
	case FB_HW_SYNC:
		if (copy_from_user(&buf_sync, (void __user *)arg, sizeof(buf_sync))) {
			return -EFAULT;
		}

		ret = rda_handle_hw_buf_sync(fb, &buf_sync);

		if (!ret)
			ret = copy_to_user((void __user *)arg, &buf_sync, sizeof(buf_sync));
		break;
#endif
	case FB_STRETCH_BLIT:
		if (copy_from_user(&vid_var, (void __user *)arg, sizeof(vid_var))) {
			return -EFAULT;
		}

		mutex_lock(&fb->gouda_lock);
		rda_gouda_enable_clk(true);

		if (rda_get_phy(fb, &vid_var.src.buf_var)) {
			mutex_unlock(&fb->gouda_lock);
			return -EFAULT;
		}
		if (rda_get_phy(fb, &vid_var.dst.buf_var)) {
			mutex_unlock(&fb->gouda_lock);
			return -EFAULT;
		}

		vid_var.src.buf_y = vid_var.src.buf_var.buf;/* vid src */
		rda_gouda_stretch_blit(&vid_var.src, &vid_var.dst);
		rda_gouda_enable_clk(false);

		mutex_unlock(&fb->gouda_lock);
		break;
	case FB_FACTORY_AUTO_TEST:
	{
		unsigned int result = 0;
		result = rda_factory_auto_test(fb);
		return copy_to_user((void __user *)arg, &result, sizeof(result)) ? -EFAULT : 0;
	}
	default:
		;//pr_err("bad ioctl cmd: %d\n", cmd);
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

static int rda_fb_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rda_fb *fb = platform_get_drvdata(pdev);

	mutex_lock(&fb->lock);

	enable_lcdc_clk(true);

	if (fb->lcd_info->ops.s_sleep) {
		fb->lcd_info->ops.s_sleep();
	}

	enable_lcdc_clk(false);
	rda_lcdc_close_lcd(&fb->lcd_info->lcd);

	//rda_panel_set_power(false);
	blocking_notifier_call_chain(&rda_fb_notifier_list, 0, NULL);

	if (fb->lcd_info->lcd.lcd_interface != LCD_IF_DBI)
		pm_ddr_put(PM_DDR_FB_DMA);

	mutex_unlock(&fb->lock);

	return 0;
}

static int rda_fb_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rda_fb *fb = platform_get_drvdata(pdev);

	mutex_lock(&fb->lock);

	if (fb->lcd_info->lcd.lcd_interface != LCD_IF_DBI){
		pm_ddr_get(PM_DDR_FB_DMA);
	}

	enable_lcdc_clk(true);
	rda_gouda_reset();
	rda_lcdc_reset();

	rda_lcdc_open_lcd(fb);

	rda_panel_set_power(true);

	if (fb->lcd_info->ops.s_wakeup) {
		fb->lcd_info->ops.s_wakeup();
	}

	if (fb->lcd_info->lcd.lcd_interface == LCD_IF_DSI)
		rda_lcdc_enable_mipi(fb);

#ifdef CONFIG_FB_RDA_USE_HWC
	fb->enable_sync = false;
#endif
	mutex_unlock(&fb->lock);

	return 0;
}

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
	size_t framesize, dma_map_size;
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

	mutex_init(&fb->lock);
	mutex_init(&fb->gouda_lock);

	init_waitqueue_head(&fb->wait);
	platform_set_drvdata(pdev, fb);

	fb->lcd_info = (struct rda_lcd_info *)pdev->dev.platform_data;

	width = fb->lcd_info->lcd.width;
	height = fb->lcd_info->lcd.height;

	fb->fb.var.bits_per_pixel = fb->lcd_info->lcd.bpp;

	if (fb->lcd_info->lcd.lcd_interface != LCD_IF_DBI) {
		pm_ddr_get(PM_DDR_FB_DMA);
	}

	fb->fb.fbops = &rda_fb_ops;
	fb->fb.flags = FBINFO_FLAG_DEFAULT;
	fb->fb.pseudo_palette = fb->palette;
	//strncpy(fb->fb.fix.id, clcd_name, sizeof(fb->fb.fix.id));
	fb->fb.fix.type = FB_TYPE_PACKED_PIXELS;
	fb->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	fb->fb.fix.line_length = width * (fb->fb.var.bits_per_pixel >> 3);
	fb->fb.fix.accel = FB_ACCEL_NONE;
	fb->fb.fix.ypanstep = 1;

	fb->fb.var.xres = width;
	fb->fb.var.yres = height;
	fb->fb.var.xres_virtual = width;
	fb->fb.var.yres_virtual = height * NUM_FRAMEBUFFERS;
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
	} else if (fb->fb.var.bits_per_pixel == 32) {
		fb->fb.var.red.offset = 24;
		fb->fb.var.red.length = 8;
		fb->fb.var.green.offset = 16;
		fb->fb.var.green.length = 8;
		fb->fb.var.blue.offset = 8;
		fb->fb.var.blue.length = 8;
		fb->fb.var.transp.offset = 0;
		fb->fb.var.transp.length = 8;
	}

	framesize = width * height * NUM_FRAMEBUFFERS * (fb->fb.var.bits_per_pixel >> 3);
	dma_map_size = PAGE_ALIGN(framesize + PAGE_SIZE);
	fb->total_mem_size = dma_map_size;
	fb->fb.screen_base =
			dma_alloc_writecombine(&pdev->dev,
			fb->total_mem_size,
			&fbpaddr, GFP_KERNEL);
	if (fb->fb.screen_base == 0) {
		ret = -ENOMEM;
		goto err_alloc_screen_base_failed;
	}

	memset(fb->fb.screen_base, 0x0, framesize);

	fb->fb.fix.smem_start = fbpaddr;
	fb->fb.fix.smem_len = framesize;

	rda_dbg_lcdc("buffer0 %x\n", fbpaddr);


	ret = fb_set_var(&fb->fb, &fb->fb.var);

	if (ret)
		goto err_fb_set_var_failed;

	rda_fb_set_par(&fb->fb);

	fb->fb_addr = fbpaddr;
	rda_lcdc_open_lcd(fb);

	if (fb->lcd_info->ops.s_reset_gpio)
		fb->lcd_info->ops.s_reset_gpio();

	if (fb->lcd_info->ops.s_open)
		fb->lcd_info->ops.s_open();

	if(fb->lcd_info->lcd.lcd_interface == LCD_IF_DSI)
		rda_lcdc_enable_mipi(fb);

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

#ifdef CONFIG_FB_RDA_USE_HWC
	if (fb->timeline == NULL) {
		fb->timeline = sw_sync_timeline_create("rda-timeline");
		if (fb->timeline == NULL) {
			pr_err("%s: cannot create time line", __func__);
			return -ENOMEM;
		} else {
			fb->timeline_value = 0;
		}
	}
	fb->enable_sync = false;
	mutex_init(&fb->sync_lock);
#endif

#ifdef CONFIG_FB_RDA_USE_ION
	fb->ion_client = NULL;
#endif
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
err_alloc_screen_base_failed:
	kfree(fb);
err_fb_alloc_failed:
	return ret;
}

static int rda_fb_remove(struct platform_device *pdev)
{
	size_t framesize;
	struct rda_fb *fb = platform_get_drvdata(pdev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&fb->early_ops);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	sysfs_remove_group(&pdev->dev.kobj, &rda_fb_attr_group);

	rda_lcdc_close_lcd(&fb->lcd_info->lcd);

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
