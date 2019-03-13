#ifndef __RDA_PANEL_H
#define __RDA_PANEL_H
#include <linux/spi/spi.h>
#include "tgt_ap_panel_setting.h"
#include <plat/reg_spi.h>
#include <plat/rda_display.h>

#define MAX_SPI_READ_BITS	22

#define SPI0_0 "spi0.0"

#define DBG_NO_USE	0
#define DBG_NEED_CHK	1
#define DBG_FIXED	2

struct rda_lcd_ops {
	int (*s_reset_gpio) (void);
	int (*s_open) (void);
	bool (*s_match_id) (void);
	int (*s_active_win) (struct lcd_img_rect *rect);
	int (*s_rotation) (int rotate);
	int (*s_sleep) (void);
	int (*s_wakeup) (void);
	int (*s_close) (void);
	int (*s_display_on)(void);
	int (*s_display_off)(void);
	int (*s_rda_lcd_dbg_w)(void *p,int n);
	void (*s_read_fb)(u8 *buffer);
	int (*s_check_lcd_state)(void *p);
};

struct rda_spi_panel_device {
	struct spi_device *spi;
	struct spi_transfer spi_xfer[4];
	u8 spi_xfer_num;
};

/*
struct rda_panel_id_info{
	char panel_name[16];
	u16 id[4];
};
*/

struct rda_panel_id_param{
	struct rda_lcd_info *lcd_info;
	struct rda_spi_panel_device * panel_dev;
	RDA_SPI_PARAMETERS *lcd_spicfg;
	/*
	 *Panel IC may has dumy read ahead of valid data
	 */
	u8 dumy_bits;
	/*The return bytes per read */
	u8 per_read_bytes;
};

struct rda_panel_driver{
	u8 panel_type;
	struct rda_lcd_info *lcd_driver_info;
	struct spi_driver *rgb_panel_driver;
	struct platform_driver *pltaform_panel_driver;
};

#define PANEL_GET_BITFIELD(v, lo, hi)	\
	(((v) & ((1ULL << ((hi) - (lo) + 1)) - 1) << (lo)) >> (lo))

void panel_mcu_read_id(u32 cmd,struct rda_panel_id_param *id_param,u16 *id_buffer);
void panel_spi_read_id(u32 cmd,struct rda_panel_id_param *id_param,u16 *id_buffer);
void panel_spi_transfer(struct rda_spi_panel_device *panel_dev,
			       const u8 * wbuf, u8 * rbuf, int len);
struct spi_device *panel_find_spidev_by_name(const char * panel_spi_name);

int rda_panel_set_power(bool onoff);
int panel_get_adc_value(u8 channel);

extern int rda_fb_register_panel(struct rda_lcd_info *p_info);
extern int rda_gpadc_check_status(void);
extern int rda_gpadc_open(int channel);
extern int rda_gpadc_close(int channel);
extern int rda_gpadc_get_adc_value(u8 channel);
extern int rda_gpadc_get_calib_adc_value(u8 channel,void *calib_data);

int panel_get_adc_value(u8 channel);

#endif /* __RDA_PANEL_H */
