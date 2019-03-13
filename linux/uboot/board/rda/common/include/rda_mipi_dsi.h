#ifndef RDA_MIPI_DSI_H
#define RDA_MIPI_DSI_H
#include "rda_lcdc.h"

struct rda_dsi_cmd{
	int dtype;
	int delay;
	int dlen;
	const char *payload;
};

enum mipi_mode{
	DSI_VIDEO_MODE,
	DSI_CMD_MODE,
};

enum dsi_tx_mode {
	DSI_CMD,
	DSI_PULSE,
	DSI_EVENT,
	DSI_BURST
};

enum dsi_cmd_mode {
	CMD_LP_MODE,
	CMD_HS_MODE
};

enum mipi_pixel_fmt {
	DSI_FMT_RGB565 = 0,
	DSI_FMT_RGB666,
	DSI_FMT_RGB666L,
	DSI_FMT_RGB888
};

#define DSI_HDR_SHORT_PKT	(0 << 5)	/*or 0x1*/
#define DSI_HDR_LONG_PKT	(0x2 << 5)	/*or 0x3*/
#define DSI_HDR_BTA			(1 << 0)
#define DSI_HDR_HS			(1 << 1)

#define DSI_HDR_VC(vc)		(((vc) & 0x03) << 14)
#define DSI_HDR_DTYPE(dtype)(((dtype) & 0x03f) << 8)
#define DSI_HDR_DATA2(data)	(((data) & 0x0ff) << 24)
#define DSI_HDR_DATA1(data)	(((data) & 0x0ff) << 16)
#define DSI_HDR_WC(wc)		(((wc) & 0x0ffff) << 16)

/* dcs read/write */
#define DTYPE_DCS_SWRITE	0x05	/* short write,with 0 parameter  */
#define DTYPE_DCS_LWRITE	0x39	/* long write */

PUBLIC VOID dsi_set_tx_mode(u8 mode);
PUBLIC U8 dsi_get_tx_mode(void);
PUBLIC VOID dsi_enable(bool on);
PUBLIC VOID dsi_pll_on(bool enable);
PUBLIC VOID dsi_op_mode(int mode);
PUBLIC VOID dsi_swrite(u8 cmd);
PUBLIC INT dsi_lwrite(const struct rda_dsi_cmd *cmds, int cmds_cnt);
PUBLIC VOID dsi_config(const struct lcd_panel_info *lcd);

#endif
