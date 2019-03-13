/*
    RDA gouda support header.

    Copyright (C) 2012  Huaping Wu <huapingwu@rdamicro.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _RDA_FB_H
#define _RDA_FB_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/fb.h>

struct rda_fb {
	struct fb_info fb;

	struct mutex lock;
	struct mutex gouda_lock;
#ifdef CONFIG_FB_RDA_USE_HWC
	struct mutex sync_lock;
#endif
	wait_queue_head_t wait;

	int total_mem_size;
	int rotation;
	u32 palette[16];
	u32 fb_addr;
	struct rda_lcd_info *lcd_info;

	int enabled;

#ifdef CONFIG_FB_RDA_USE_HWC
	/* hw buf sync */
	bool enable_sync;
	int acq_fence_cnt;
	int timeline_value;
	struct sync_fence *acq_fence[RDA_MAX_FENCE_FD];
	struct sw_sync_timeline *timeline;
#endif

#ifdef CONFIG_FB_RDA_USE_ION
	struct ion_client * ion_client;
#endif
};

#endif
