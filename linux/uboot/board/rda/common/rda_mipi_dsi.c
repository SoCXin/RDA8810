#include <common.h>
#include <asm/arch/cs_types.h>
#include "rda_lcdc.h"
#include "rda_mipi_dsi.h"

#define COMMANDS_ADDR 0x85000008
#define CMD_QUEUE_BASE 0x180

static u8 cmd_tx_mode = CMD_HS_MODE;

void dsi_set_tx_mode(u8 mode)
{
	cmd_tx_mode = mode;
}

u8 dsi_get_tx_mode(void)
{
	return cmd_tx_mode;
}

static void dsi_dump_cmds(char *buf, int group_num)
{
	int i;
	U64 *data = (U64 *)buf;

	for(i = 0; i < group_num; i++)
		printf("%16llx\n", *data++);
}

static int dsi_get_cmds_num(const struct rda_dsi_cmd *cmds, int cmds_cnt)
{
	int i,j,cmds_num = 0;

	const struct rda_dsi_cmd *cmds_ptr = cmds;

	for (i = 0; i < cmds_cnt; i++) {
		for (j = 0; j < cmds_ptr->dlen; j++)
			cmds_num++;
		cmds_ptr++;
	}
	return cmds_num;
}

void dsi_enable(bool on)
{
	rda_write_dsi_reg(0x04, on ? 0x1 : 0x0);
}

void dsi_op_mode(int mode)
{
	rda_write_dsi_reg(0x14, mode);
}

void dsi_cmd_queue_init(int cmd_cnt)
{
	rda_write_dsi_reg(0x40, cmd_cnt - 1); 	// dsi cmds num,actualy cmd - 1
	rda_write_dsi_reg(0x44, 0x03);			// dsi cmd queue enable
}

/****************************************************************
***************** COMMAND DATA FORMAT IN DDR ********************
*
* LP MODE:	0x| xx | xx |xx|xx|xx|xx|xx|xx| first command useless
*			0x|cmd0|cmd0|00|00|00|00|00|00|
*			0x|cmd1|cmd1|00|00|00|00|00|00|
*	   ...
*
* HS MODE:	0x|cmd0|cmd1|cmd2|00|cmd3|00|00|00|
*			0x|cmd4|cmd5|cmd6|00|cmd7|00|00|00|
*	   ...
*
****************************************************************/

static void dsi_cmds_into_ddr(const struct rda_dsi_cmd *cmd, char **buf)
{
	int i,j;
	u8 pd;
	char *buf_ptr = *buf;
	static int index = 0;
	const char *payload;
	payload = cmd->payload;
	if (cmd_tx_mode == CMD_LP_MODE) {
		for (i = 0; i < cmd->dlen; i++) {
			pd = *payload++;
			*buf_ptr++ = pd;
			*buf_ptr++ = pd;
			printf("cmd	%8x\n", pd);
			for (j = 0; j < 6; j++)
				*buf_ptr++ = 0x00;
		}
	} else {
		j = 0;
		start:
			while (index < 3) {
				if (j < cmd->dlen) {
					*buf_ptr++ = *payload++;
					index++;
					j++;
				} else {
					*buf = buf_ptr;
					return;
				}
			}
			if (index == 3) {
				*buf_ptr++ = 0;
				index++;
			}
			if (index == 4) {
				if (j < cmd->dlen) {
					*buf_ptr++ = *payload++;
					index++;
					j++;
				} else {
					*buf = buf_ptr;
					return;
				}
			}
			while (index < 8) {
				*buf_ptr++ = 0;
				index++;
			}
			index = 0;
			if (j < cmd->dlen)
				goto start;
	}
	*buf = buf_ptr;
}


void dsi_pll_on(bool enable)
{
	if (enable)
		rda_write_dsi_reg(0x00, 0x3);
	else
		rda_write_dsi_reg(0x00, 0x0);
}

static void dsi_set_timing(const struct rda_dsi_phy_ctrl *dsi_phy_db)
{
	int i;
	u32 off,value;

	/* cmd mode */
	for (i = 0; i < ARRAY_SIZE(dsi_phy_db->cmd_timing); i++) {
		off = dsi_phy_db->cmd_timing[i].off;
		value = dsi_phy_db->cmd_timing[i].value;
		rda_write_dsi_reg(off,value);
	}
	/* video mode */
	for (i = 0; i < ARRAY_SIZE(dsi_phy_db->video_timing); i++) {
		off = dsi_phy_db->video_timing[i].off;
		value = dsi_phy_db->video_timing[i].value;
		rda_write_dsi_reg(off, value);
	}
}

static void dsi_set_parameter(const struct lcd_panel_info *lcd)
{
	u32 data_lane;
	u32 dsi_bpp;
	data_lane = (cmd_tx_mode == CMD_HS_MODE) ? lcd->mipi_pinfo.data_lane >> 1 : 0;
	rda_write_dsi_reg(0x8,data_lane);
	/*pixel num*/
	rda_write_dsi_reg(0xc,lcd->width);
	/*pixel type*/
	rda_write_dsi_reg(0x10,lcd->mipi_pinfo.dsi_format);

	/*vc,bllp enable*/
	if(lcd->mipi_pinfo.bllp_enable)
		rda_write_dsi_reg(0x18,0x1 << 2);

	rda_write_dsi_reg(0x20,lcd->mipi_pinfo.h_sync_active);
	rda_write_dsi_reg(0x24,lcd->mipi_pinfo.h_back_porch);
	rda_write_dsi_reg(0x28,lcd->mipi_pinfo.h_front_porch);

	dsi_bpp = (lcd->bpp != 32) ? lcd->bpp : 24;
	rda_write_dsi_reg(0x2c,(lcd->width * (dsi_bpp >> 3)+ 6) >> data_lane);

	rda_write_dsi_reg(0x30,lcd->mipi_pinfo.v_sync_active);
	rda_write_dsi_reg(0x34,lcd->mipi_pinfo.v_back_porch);
	rda_write_dsi_reg(0x38,lcd->mipi_pinfo.v_front_porch);
	rda_write_dsi_reg(0x3c,lcd->height - 1);/*vat line */

	/*
	* sync data swap
	*/
	rda_write_dsi_reg(0x104,0x2);
}

void dsi_config(const struct lcd_panel_info *lcd)
{
	const struct rda_dsi_phy_ctrl *dsi_phy_db = lcd->mipi_pinfo.dsi_phy_db;

	rda_write_dsi_reg(0x144, 0x14);		//analog phase
	dsi_pll_on(TRUE);					//pll enable

	dsi_enable(FALSE);					//dsi disable
	dsi_op_mode(DSI_CMD);				//cmd mode
	dsi_set_timing(dsi_phy_db);
	dsi_set_parameter(lcd);
}

void dsi_swrite(u8 cmd)
{

	u32 cmd_header = 0;

	cmd_header |= DSI_HDR_VC(0);
	cmd_header |= DSI_HDR_SHORT_PKT;
	if (cmd_tx_mode == CMD_HS_MODE)
		cmd_header |= DSI_HDR_HS;
	cmd_header |= DSI_HDR_DTYPE(DTYPE_DCS_SWRITE);
	cmd_header |= DSI_HDR_DATA1(cmd);
	cmd_header |= DSI_HDR_DATA2(0);

	dsi_enable(FALSE);
	dsi_op_mode(DSI_CMD);
	dsi_cmd_queue_init(1);
	rda_write_dsi_reg((u32)CMD_QUEUE_BASE, cmd_header);
	dsi_enable(TRUE);
}

int dsi_lwrite(const struct rda_dsi_cmd *cmds, int cmds_cnt)
{
	u32 cmd_header = 0;
	char *buf = (char *)COMMANDS_ADDR;
	u32 offset = 0;
	int cmds_num = 0, group_num = 0;
	int i;

	cmds_num = dsi_get_cmds_num(cmds, cmds_cnt);

	if (cmd_tx_mode == CMD_LP_MODE) {
		cmds_num++;
		group_num = cmds_num;
	} else
		group_num = roundup(cmds_num, 4) / 4;

	memset(buf, 0, group_num * 8);
	if (cmd_tx_mode == CMD_LP_MODE) {
		*buf++ = 0xaa;
		for(i = 0; i < 7; i++)
			*buf++ = 0x00;
	}

	for (i = 0; i < cmds_cnt; i++)
		dsi_cmds_into_ddr(&cmds[i], &buf);

	buf -= group_num * 8;
	if (cmd_tx_mode == CMD_HS_MODE)
		buf += (8 - cmds_num % 4);
	printf("buf addr: %x\n", (u32)&(*buf));

	dsi_dump_cmds(buf, group_num);
	printf("cmds_num = %d\n", cmds_num);

	set_lcdc_for_cmd((u32)&(*buf), group_num);

	if (cmd_tx_mode == CMD_HS_MODE)
		cmd_header |= DSI_HDR_HS;

	dsi_enable(FALSE);
	dsi_op_mode(DSI_CMD);
	dsi_cmd_queue_init(cmds_cnt);
	for (i = 0; i < cmds_cnt; i++) {
		cmd_header |= DSI_HDR_WC(cmds[i].dlen);
		cmd_header |= DSI_HDR_VC(0);
		cmd_header |= DSI_HDR_LONG_PKT;
		cmd_header |= DSI_HDR_DTYPE(DTYPE_DCS_LWRITE);
		rda_write_dsi_reg(CMD_QUEUE_BASE + offset, cmd_header);
		printf("cmd_header: %x   %x\n", cmd_header, offset);

		offset += 4;
		cmd_header = 0;
		if (cmd_tx_mode == CMD_HS_MODE)
			cmd_header |= DSI_HDR_HS;
	}

	dsi_enable(TRUE);

	return 0;
}

