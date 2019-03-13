#include <linux/delay.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>

#include <plat/rda_debug.h>
#include <rda_sensor.h>


static struct sensor_callback_ops rda_sensor_cb;

void sensor_power_down(bool pdn, bool acth, int id)
{
	rda_sensor_cb.cb_pdn(pdn, acth, id);
}
void sensor_reset(bool rst, bool acth)
{
	rda_sensor_cb.cb_rst(rst, acth);
}
void sensor_clock(bool out, int mclk)
{
	rda_sensor_cb.cb_clk(out, mclk);
}
void sensor_read(const u16 addr, u8 *data, u8 bits)
{
	rda_sensor_cb.cb_i2c_r(addr, data, bits);
}
void sensor_write(const u16 addr, const u8 data, u8 bits)
{
	rda_sensor_cb.cb_i2c_w(addr, data, bits);
}

void sensor_write_group(struct sensor_reg* reg, u32 size)
{
	int i;
	struct sensor_reg *tmp = NULL;

	for (i = 0; i < size; i++) {
		tmp = reg + i;
		sensor_write(tmp->addr, tmp->data, tmp->bits);
		if (tmp->wait)
			mdelay(tmp->wait);
	}
}

#if defined(GC0308_B) || defined(GC0308_F)
#include "gc0308_config.h"
#endif
#if defined(GC0309_B) || defined(GC0309_F)
#include "gc0309_config.h"
#endif

#if defined(GC0309B_B) || defined(GC0309B_F)
#include "gc0309_config_back.h"
#endif

#if defined(GC0309F_B) || defined(GC0309F_F)
#include "gc0309_config_front.h"
#endif

#if defined(GC0310_CSI_B) || defined(GC0310_CSI_F)
#include "gc0310_csi_config.h"
#endif
#if defined(GC0313_CSI_B) || defined(GC0313_CSI_F)
#include "gc0313_csi_config.h"
#endif
#if defined(GC2145_CSI_B) || defined(GC2145_CSI_F)
#include "gc2145_csi_config.h"
#endif
#if defined(GC2355_CSI_B) || defined(GC2355_CSI_F)
#include "gc2355_csi_config.h"
#endif
#if defined(GC0409_CSI_B) || defined(GC0409_CSI_F)
#include "gc0409_csi_config.h"
#endif
#if defined(GC0328_B) || defined(GC0328_F)
#include "gc0328_config.h"
#endif
#if defined(GC0328C_B)|| defined(GC0328C_F)
#include "gc0328c_config.h"
#endif
#if defined(GC0329_B) || defined(GC0329_F)
#include "gc0329_config.h"
#endif
#if defined(GC2035_B) || defined(GC2035_F)
#include "gc2035_config.h"
#elif defined(GC2035_CSI_B) || defined(GC2035_CSI_F)
#include "gc2035_csi_config.h"
#endif
#if defined(GC2155_B) || defined(GC2155_F)
#include "gc2155_config.h"
#endif
#if defined(GC0312_B) || defined(GC0312_F)
#include "gc0312_config.h"
#endif
#if defined(GC2145_B) || defined(GC2145_F)
#include "gc2145_config.h"
#endif
#if defined(RDA2201_CSI_B) || defined(RDA2201_CSI_F)
#include "rda2201_csi_config.h"
#endif
#if defined(RDA2201_B) || defined(RDA2201_F)
#include "rda2201_config.h"
#endif
#if defined(SP0718_B) || defined(SP0718_F)
#include "sp0718_config.h"
#endif
#if defined(SP2508_CSI_B) || defined(SP2508_CSI_F)
#include "sp2508_csi_config.h"
#endif
#if defined(SP0A20_CSI_B) || defined(SP0A20_CSI_F)
#include "sp0a20_csi_config.h"
#endif
#if defined(SP0A09_CSI_B) || defined(SP0A09_CSI_F)
#include "sp0a09_csi_config.h"
#endif
#ifdef RDA_CUSTOMER_DRV_CSB
static LIST_HEAD(rda_sensor_b_list);
static void init_back_sensor_list(struct list_head *head)
{
#ifdef GC0308_B
	INIT_LIST_HEAD(&gc0308_dev.list);
	list_add_tail(&gc0308_dev.list, head);
#endif
#ifdef GC0309_B
	INIT_LIST_HEAD(&gc0309_dev.list);
	list_add_tail(&gc0309_dev.list, head);
#endif

#ifdef GC0309B_B
	INIT_LIST_HEAD(&gc0309b_dev.list);
	list_add_tail(&gc0309b_dev.list, head);
#endif
#ifdef GC0309F_B
	INIT_LIST_HEAD(&gc0309_dev.list);
	list_add_tail(&gc0309_dev.list, head);
#endif

#ifdef GC0310_CSI_B
	INIT_LIST_HEAD(&gc0310_dev.list);
	list_add_tail(&gc0310_dev.list, head);
#endif
#ifdef GC0313_CSI_B
	INIT_LIST_HEAD(&gc0313_dev.list);
	list_add_tail(&gc0313_dev.list, head);
#endif
#ifdef GC2145_CSI_B
	INIT_LIST_HEAD(&gc2145_dev.list);
	list_add_tail(&gc2145_dev.list, head);
#endif
#ifdef GC2355_CSI_B
	INIT_LIST_HEAD(&gc2355_dev.list);
	list_add_tail(&gc2355_dev.list, head);
#endif
#ifdef GC0409_CSI_B
	INIT_LIST_HEAD(&gc0409_dev.list);
	list_add_tail(&gc0409_dev.list, head);
#endif
#ifdef GC0328_B
	INIT_LIST_HEAD(&gc0328_dev.list);
	list_add_tail(&gc0328_dev.list, head);
#endif
#ifdef GC0328C_B
	INIT_LIST_HEAD(&gc0328c_dev.list);
	list_add_tail(&gc0328c_dev.list, head);
#endif
#ifdef GC0329_B
	INIT_LIST_HEAD(&gc0329_dev.list);
	list_add_tail(&gc0329_dev.list, head);
#endif
#ifdef GC0312_B
	INIT_LIST_HEAD(&gc0312_dev.list);
	list_add_tail(&gc0312_dev.list, head);
#endif
#ifdef GC2145_B
	INIT_LIST_HEAD(&gc2145_dev.list);
	list_add_tail(&gc2145_dev.list, head);
#endif
#ifdef GC2035_B
	INIT_LIST_HEAD(&gc2035_dev.list);
	list_add_tail(&gc2035_dev.list, head);
#elif defined(GC2035_CSI_B)
	INIT_LIST_HEAD(&gc2035_dev.list);
	list_add_tail(&gc2035_dev.list, head);
#endif
#ifdef GC2155_B
	INIT_LIST_HEAD(&gc2155_dev.list);
	list_add_tail(&gc2155_dev.list, head);
#endif
#ifdef RDA2201_CSI_B
	INIT_LIST_HEAD(&rda2201_dev.list);
	list_add_tail(&rda2201_dev.list, head);
#endif
#ifdef RDA2201_B
	INIT_LIST_HEAD(&rda2201_dev.list);
	list_add_tail(&rda2201_dev.list, head);
#endif
#ifdef SP0718_B
	INIT_LIST_HEAD(&sp0718_dev.list);
	list_add_tail(&sp0718_dev.list, head);
#endif
#ifdef SP2508_CSI_B
	INIT_LIST_HEAD(&sp2508_dev.list);
	list_add_tail(&sp2508_dev.list, head);
#endif

#ifdef SP0A20_CSI_B
	INIT_LIST_HEAD(&sp0a20_dev.list);
	list_add_tail(&sp0a20_dev.list, head);
#endif
#ifdef SP0A09_CSI_B
	INIT_LIST_HEAD(&sp0a09_dev.list);
	list_add_tail(&sp0a09_dev.list, head);
#endif
}
#endif

#ifdef RDA_CUSTOMER_DRV_CSF
static LIST_HEAD(rda_sensor_f_list);

static void init_front_sensor_list(struct list_head *head)
{
#ifdef GC0308_F
	INIT_LIST_HEAD(&gc0308_dev.list);
	list_add_tail(&gc0308_dev.list, head);
#endif
#ifdef GC0309_F
	INIT_LIST_HEAD(&gc0309_dev.list);
	list_add_tail(&gc0309_dev.list, head);
#endif
#ifdef GC0309B_F
	INIT_LIST_HEAD(&gc0309b_dev.list);
	list_add_tail(&gc0309b_dev.list, head);
#endif
#ifdef GC0309F_F
	INIT_LIST_HEAD(&gc0309_dev.list);
	list_add_tail(&gc0309_dev.list, head);
#endif
#ifdef GC0310_CSI_F
	INIT_LIST_HEAD(&gc0310_dev.list);
	list_add_tail(&gc0310_dev.list, head);
#endif
#ifdef GC0313_CSI_F
	INIT_LIST_HEAD(&gc0313_dev.list);
	list_add_tail(&gc0313_dev.list, head);
#endif
#ifdef GC2145_CSI_F
	INIT_LIST_HEAD(&gc2145_dev.list);
	list_add_tail(&gc2145_dev.list, head);
#endif
#ifdef GC2355_CSI_F
	INIT_LIST_HEAD(&gc2355_dev.list);
	list_add_tail(&gc2355_dev.list, head);
#endif
#ifdef GC0409_CSI_F
	INIT_LIST_HEAD(&gc0409_dev.list);
	list_add_tail(&gc0409_dev.list, head);
#endif
#ifdef GC0328_F
	INIT_LIST_HEAD(&gc0328_dev.list);
	list_add_tail(&gc0328_dev.list, head);
#endif
#ifdef GC0328C_F
        INIT_LIST_HEAD(&gc0328c_dev.list);
        list_add_tail(&gc0328c_dev.list, head);
#endif
#ifdef GC0329_F
	INIT_LIST_HEAD(&gc0329_dev.list);
	list_add_tail(&gc0329_dev.list, head);
#endif
#ifdef GC0312_F
	INIT_LIST_HEAD(&gc0312_dev.list);
	list_add_tail(&gc0312_dev.list, head);
#endif
#ifdef GC2145_F
	INIT_LIST_HEAD(&gc2145_dev.list);
	list_add_tail(&gc2145_dev.list, head);
#endif
#ifdef GC2035_F
	INIT_LIST_HEAD(&gc2035_dev.list);
	list_add_tail(&gc2035_dev.list, head);
#elif defined(GC2035_CSI_F)
	INIT_LIST_HEAD(&gc2035_dev.list);
	list_add_tail(&gc2035_dev.list, head);
#endif
#ifdef GC2155_F
	INIT_LIST_HEAD(&gc2155_dev.list);
	list_add_tail(&gc2155_dev.list, head);
#endif
#ifdef RDA2201_CSI_F
	INIT_LIST_HEAD(&rda2201_dev.list);
	list_add_tail(&rda2201_dev.list, head);
#endif
#ifdef RDA2201_F
	INIT_LIST_HEAD(&rda2201_dev.list);
	list_add_tail(&rda2201_dev.list, head);
#endif
#ifdef SP0718_F
	INIT_LIST_HEAD(&sp0718_dev.list);
	list_add_tail(&sp0718_dev.list, head);
#endif
#ifdef SP2508_CSI_F
	INIT_LIST_HEAD(&sp2508_dev.list);
	list_add_tail(&sp2508_dev.list, head);
#endif
#ifdef SP0A20_CSI_F
	INIT_LIST_HEAD(&sp0a20_dev.list);
	list_add_tail(&sp0a20_dev.list, head);
#endif
#ifdef SP0A09_CSI_F
	INIT_LIST_HEAD(&sp0a09_dev.list);
	list_add_tail(&sp0a09_dev.list, head);
#endif
}
#endif


static int __init cam_sensor_subsys_init(void)
{
	int ret;
	int init_done = -1;

#ifdef RDA_CUSTOMER_DRV_CSB
	init_back_sensor_list(&rda_sensor_b_list);
	ret = rda_sensor_adapt(&rda_sensor_cb,
			&rda_sensor_b_list, 0);
	if (ret < 0) {
		rda_dbg_camera("%s: back sensor adapt failed\n", __func__);
	} else {
		init_done = 0;
	}
#endif
#ifdef RDA_CUSTOMER_DRV_CSF
	init_front_sensor_list(&rda_sensor_f_list);
	ret = rda_sensor_adapt(&rda_sensor_cb,
			&rda_sensor_f_list, 1);
	if (ret < 0) {
		rda_dbg_camera("%s: front sensor adapt failed\n", __func__);
	} else {
		init_done = 0;
	}
#endif

	return init_done;
}

subsys_initcall(cam_sensor_subsys_init);

MODULE_DESCRIPTION("The sensor driver for RDA Linux");
MODULE_LICENSE("GPL");

