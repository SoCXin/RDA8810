#ifndef __RDA_PANEL_H
#define __RDA_PANEL_H
#include <linux/spi/spi.h>
#include <rda/tgt_ap_panel_setting.h>
#include "rda_gouda.h"
#include <plat/reg_spi.h>

#ifdef TGT_FB_IS_32BITS
#define PIX_FMT	RDA_FMT_XRGB
#else
#define PIX_FMT	RDA_FMT_RGB565
#endif

#ifdef TGT_LCD_DATA_IS_24BITS
#define LCD_DATA_WIDTH RGB_IS_24BIT
#else
#define LCD_DATA_WIDTH RGB_IS_16BIT
#endif

//Just hack for target WLDS301P3
#ifdef TGT_LCD_FORCE_INVERSE
#define RDA_FB_FORCE_INVERSE
#endif

#ifdef TGT_LCD_NT35310_MCU_HSD
#define NT35310_MCU_HSD_INIT_CODE
#endif

#define MAX_SPI_READ_BITS	22

#define SPI0_0 "spi0.0"

struct rda_lcd_ops {
	int (*s_init_gpio) (void);
	int (*s_open) (void);
	int (*s_readid) (void);
	int (*s_active_win) (struct gouda_rect *rect);
	int (*s_rotation) (int rotate);
	int (*s_sleep) (void);
	int (*s_wakeup) (void);
	int (*s_close) (void);
	int (*s_display_on)(void);
	int (*s_display_off)(void);
	int (*s_rda_lcd_dbg_w)(void *p,int n);
	void (*s_read_fb)(u8 *buffer);
};

struct rda_lcd_info {
	struct rda_lcd_ops ops;
	struct gouda_lcd lcd;
	char name[32];
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

#define LCD_MCU_CMD(Cmd)		\
		{while(rda_gouda_dbi_write_cmd2lcd(Cmd) != 0);}
#define LCD_MCU_DATA(Data)	\
		{while(rda_gouda_dbi_write_data2lcd(Data) != 0);}
#define LCD_MCU_READ(pData)		\
		{while(rda_gouda_dbi_read_data(pData) != 0);}
#define LCD_MCU_READ16(pData)		\
		{while(rda_gouda_dbi_read_data16(pData) != 0);}

extern int rda_fb_probe_panel(struct rda_lcd_info *p_info, void *p_driver);
extern unsigned int read_rxtxbuffer(u16 * receive_buffer,u8 framesize,u8 total_bits,u8 byte_num);
extern struct spi_device *panel_find_spidev_by_name(const char * panel_spi_name);
extern void panel_mcu_read_id(u32 cmd,struct rda_panel_id_param *id_param,u16 *id_buffer);
extern void panel_spi_read_id(u32 cmd,struct rda_panel_id_param *id_param,u16 *id_buffer);
extern void panel_spi_transfer(struct rda_spi_panel_device *panel_dev,
			       const u8 * wbuf, u8 * rbuf, int len);

extern int rda_fb_register_panel(struct rda_lcd_info *p_info);
#endif /* __RDA_PANEL_H */
