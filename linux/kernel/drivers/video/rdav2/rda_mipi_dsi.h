#ifndef RDA_MIPI_DSI_H
#define RDA_MIPI_DSI_H
#include "rda_lcdc.h"

struct rda_dsi_buf {
	u32 *hdr;	/* dsi host header */
	char *start;	/* buffer start addr */
	char *end;	/* buffer end addr */
	int size;	/* size of buffer */
	char *data;	/* buffer */
	dma_addr_t dma_addr;/*for cmdlist*/
	int len;	/* data length */
	int flag;	/*cmdlist or cmd*/
};

struct rda_dsi_cmd{
	int dtype;
	int delay;
	int dlen;
	const char *payload;
};

struct dsi_cmd_list{
	const struct rda_dsi_cmd *cmds;
	int cmds_cnt;
	u32 flags;
	int rxlen;
	struct rda_dsi_buf *payload;/*cmds int ddr*/
	struct rda_dsi_buf *header;/*cmds hdr in dsi cmd reg*/
};

struct dsi_status{
	u32 dsi_tx_reg_offset;
	u32 tx_mode;
	struct mutex dsi_cmd_mutex;
};

struct rda_dsi_transfer {
	struct dsi_cmd_list *cmdlist;/* long pkt */
};

enum mipi_mode{
	DSI_VIDEO_MODE,
	DSI_CMD_MODE,
};

enum dsi_video_mode {
	DSI_CMD,
	DSI_PULSE,
	DSI_EVENT,
	DSI_BURST
};

enum mipi_pixel_fmt {
	DSI_FMT_RGB565 = 0,
	DSI_FMT_RGB666,
	DSI_FMT_RGB666L,
	DSI_FMT_RGB888
};

#define PAYLOADY_SIZE	8
#define CMDLIST_SEND_FLAG 0xFFFF
#define CMDLIST_SEND_MAX_NUM 32
#define CMD_SEND_REG_BASE 0x180

/*
* cmd_config
* bit 0:bta_enable,bit 1:hs_enable_pre,bit 3 :te_enable,bit 5-6:cmd_type,
*/
#define DSI_HOST_HDR_SIZE	4
#define DSI_HDR_SHORT_PKT	(0 << 5) /*or 0x1*/
#define DSI_HDR_LONG_PKT	(0x2 << 5) /*or 0x3*/
#define DSI_HDR_BTA			(1 << 0)
#define DSI_HDR_HS_ENABLE	(1 << 1)

#define DSI_HDR_VC(vc)		(((vc) & 0x03) << 14)
#define DSI_HDR_DTYPE(dtype)	(((dtype) & 0x03f) << 8)
#define DSI_HDR_DATA2(data)	(((data) & 0x0ff) << 24)
#define DSI_HDR_DATA1(data)	(((data) & 0x0ff) << 16)
#define DSI_HDR_WC(wc)		(((wc) & 0x0ffff) << 16)

/* dcs read/write */
#define DTYPE_DCS_SWRITE		0x05	/* short write,with 0 parameter  */
#define DTYPE_DCS_SWRITE1	0x15	/* short write, with 1 parameter */
#define DTYPE_DCS_READ		0x06	/* read */
#define DTYPE_DCS_LWRITE		0x39	/* long write */

/* generic read/write */
#define DTYPE_GEN_SWRITE	0x03	/* short write, with 0 parameter */
#define DTYPE_GEN_SWRITE1	0x13	/* short write, with 1 parameter */
#define DTYPE_GEN_SWRITE2	0x23	/* short write, with 2 parameter */
#define DTYPE_GEN_LWRITE	0x29	/* long write */
#define DTYPE_GEN_READ		0x04	/* long read, with 0 parameter */
#define DTYPE_GEN_READ1		0x14	/* long read, with 1 parameter */
#define DTYPE_GEN_READ2		0x24	/* long read, with 2 parameter */

#define DTYPE_TEAR_ON		0x35	/* set tear on */
#define DTYPE_MAX_PKTSIZE	0x37	/* set max packet size */
#define DTYPE_NULL_PKT		0x09	/* null packet, no data */
#define DTYPE_BLANK_PKT		0x19	/* blankiing packet, no data */

/*
 * dcs response
 */
#define DTYPE_ACK_ERR_RESP	0x02
#define DTYPE_EOT_RESP		0x08    /* end of tx */
#define DTYPE_GEN_READ1_RESP	0x11    /* 1 parameter, short */
#define DTYPE_GEN_READ2_RESP	0x12    /* 2 parameter, short */
#define DTYPE_GEN_LREAD_RESP	0x1a
#define DTYPE_DCS_LREAD_RESP		0x1c
#define DTYPE_DCS_READ1_RESP	0x21    /* 1 parameter, short */
#define DTYPE_DCS_READ2_RESP	0x22    /* 2 parameter, short */

#define DTYPE_PERIPHERAL_OFF		0x22
#define DTYPE_PERIPHERAL_ON		0x32

#define	DSI_LP_MODE		0 << 1
#define	DSI_HS_MODE	1 << 1
#define	DSI_UTLP_MODE	1 << 2

#define RD_DSI_SEND_CMD			0x1
#define RD_DSI_SEND_CMDLIST		0x2
#define RD_DSI_CMDLIST_HDR		0x4

#define CMDLIST_INITED	0x1
#define CMDLIST_LCDC_DSI_INITED	0x2
#define	DSI_NEED_RX		0x4

/*
* how to set the cmd dsi timing
* phy lp tx rate
* T = (1000000000l/dsi_speed);,dsi_speed 50Mhz,UI 2.5ns
* dsi phy 400MHz,byte clk 50MHz@20ns,T = 20ns
* Tlpx ,transmitted length of any lower-power state period, min value 50ns
* Tlpx = (t_bta_lpx_reg+1)*T= (phy_lp_tx_rate_reg+1)*T
* we choose Tlpx 60ns
* ratio Tlpx Tlpx(master)/Tlpx(slave) between (2/3 < r <  3/2)
* phy_lp_tx_rate_reg = ((60 + T) / T) - 1;
* write_dsi_reg(0x64,phy_lp_tx_rate_reg);
* write_dsi_reg(0x134,phy_lp_tx_rate_reg);
* write_dsi_reg(0x138,0x8);(3*phy_lp_tx_rate_reg + 2)
* write_dsi_reg(0x14c,0x20);(3*phy_lp_tx_rate_reg + 2)
* write_dsi_reg(0x13c,0x5);(2*phy_lp_tx_rate_reg + 1)
* write_dsi_reg(0x13c,0x8);
* bit 4:11,LPDT Entry Code,escape mode to LPDT see mipi phy spec table 8
* write_dsi_reg(0x170,0x087a);
*/

/*
* how to set video dsi timing
*
* bllp,  > clk_state_ad_num + (t_lp_01+1)+(t_lp_00+1)+ (t_zero_reg + 1)+ (t_sync_reg + 1)
* write_dsi_reg(0x6c,0x80);
*
*lp 01 timing,(t_lp_01_reg + 1)*T
* write_dsi_reg(0x10c,0x4);
*
* lp 00 timing,(t_lp_00_reg + 1)*T
* write_dsi_reg(0x108,0x3);
*
* THS-PREPARE in [40ns+4UI, 85ns+6UI]
* THS-ZERO : THS-PREPARE + THS-ZERO > 145ns + 10UI
* write_dsi_reg(0x118,0x9);
*
* timing_sync,t_sync_reg
* write_dsi_reg(0x11c,0x0);
*
* EOT timing number
* Teot transmitted time interval from the the Ths-trail or Tclk-trail
*  ,to the start of the LP11 state following a HS burst  < 105ns + n*12*ui
* write_dsi_reg(0x120,0x6);
*
* clk_lp_01timing
* write_dsi_reg(0x124,0x3);
*
* clk_lp_01timing
* write_dsi_reg(0x128,0x4);
*
* time that the clock shall be driven from clk_lp_11 by the transmitter prior to
* any associated data lane beginning the transition from LP11 to LP01 mode
* the time include clk_lp_11,clk_lp_00,clk_hd_zero_num and partial time of
* HS clock,=Tlpx + Tclk-prepare + Tclk-zero + Tclk-pre, Tclk-pre >= 8UI
* write_dsi_reg(0x80,0x1f);
*
* Tclk-zero
* time that the transmitter drives the clock lane HS-0 line state immediately
* before data lane starting the HS transmission.
* Tclk-zero + Tclk-prepare > 300ns
* if dsi phy 400Mhz,byte clk 50Mhz unit = 20ns,0x14*20 = 400ns
* write_dsi_reg(0x84,0x14);
*
* Transmitted time interval from the start of Ths-trail or Tclk-trail,
* to the start od the LP 11 state following a HS burst. < 105ns + n*12*UI
* T_CLK_EOT is actually TCLK_TRAIL + TCLK_POST
* TCLK_TRAIL >=60ns  <= tEOT(105+12UI)
* write_dsi_reg(0x130,0x14);
*
* Tclk-post,time that transmitter continues to send HS clock after the
* last associated Data lane hastransitioned to LP mode, > 60ns + 52*UI
* write_dsi_reg(0x150,0x10);
*
* bit 4-11,escape mode entry value,enter LPDT, 1110 0001
* bit 1,3 need set 1 just in rda8850e
* write_dsi_reg(0x170,0x087a);
*/
void mipi_dsi_switch_lp_mode(void);
void rda_dsi_set_tx_mode(int mode);
int rda_dsi_get_tx_mode(void);
void rda_dsi_op_mode_config(int mode);

int rda_mipi_dsi_init(struct mipi_panel_info *mipi_pinfo);
void rda_mipi_dsi_deinit(struct mipi_panel_info *mipi_pinfo);
void rda_dsi_cmd_queue_clear(int cmds_cnt);
void rda_dsi_cmd_bta_sw_trigger(void);
int rda_dsi_cmd_send(const struct rda_dsi_cmd *cmd);
int rda_dsi_read_data(u8 *data, u32 len);
int rda_dsi_cmdlist_send_lpkt(struct rda_dsi_transfer *trans);
int rda_dsi_cmdlist_send(struct rda_dsi_transfer *trans);
int rda_dsi_spkt_cmds_read(const struct rda_dsi_cmd *cmds,int cmds_cnt);
#endif
