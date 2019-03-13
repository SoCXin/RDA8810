#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>

#include "rda_mipi_dsi.h"

static struct dsi_status *dsi_state = NULL;
static char cmdlist_fill_hdr = 0xAA;
static int dsi_get_cmd(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *buf);

static char *rda_dsi_buf_init(struct rda_dsi_buf *rdb)
{
	int off;

	rdb->data = rdb->start;
	off = (int)rdb->data;
	/* 8 byte align */
	off &= 0x07;
	if (off)
		off = 8 - off;
	rdb->data += off;
	rdb->len = 0;
	return rdb->data;
}

static int rda_dsi_buf_release(struct rda_dsi_buf *rdb)
{
	if(rdb->flag == RD_DSI_CMDLIST_HDR){
		if(rdb->start)
			kfree(rdb->start);
	} else if(rdb->flag == RD_DSI_SEND_CMDLIST){
		dma_free_coherent(NULL, rdb->len,(void *)rdb->start,
				  rdb->dma_addr);
	}

	return 0;
}

static int rda_dsi_buf_alloc(struct rda_dsi_buf *rdb, int size)
{

	rdb->start = kzalloc(size, GFP_KERNEL);
	if (rdb->start == NULL) {
		pr_err("%s:%u\n", __func__, __LINE__);
		return -ENOMEM;
	}

	rdb->end = rdb->start + size;
	rdb->size = size;

	if ((int)rdb->start & 0x07)
		pr_err("%s: buf not 8 bytes aligned\n", __func__);

	rdb->data = rdb->start;
	rdb->len = 0;
	return size;
}

static int rda_dsi_get_cmdlist_len(struct dsi_cmd_list *cmdlist,int index)
{
	int i,j,cmds_num = 0;
	int cmds_cnt;
	const struct rda_dsi_cmd *cmds = cmdlist->cmds;

	cmds_cnt = index == CMDLIST_SEND_FLAG ?
		cmdlist->cmds_cnt : index;

	for (i = 0; i < cmds_cnt; i++){
		for(j = 0;j < cmds->dlen;j++)
			cmds_num++;
		cmds++;
	}

	return cmds_num;
}

static int rda_dsi_cmdlist_release(struct rda_dsi_transfer *trans)
{
	int ret = 0;
	struct dsi_cmd_list *cmdlist;

	rda_dbg_lcdc("%s\n",__func__);

	cmdlist = trans->cmdlist;
	rda_dsi_buf_release(cmdlist->header);
	kfree(cmdlist->header);

	rda_dsi_buf_release(cmdlist->payload);
	kfree(cmdlist->payload);

	cmdlist->flags = 0;

	return ret;
}

static void dsi_dump_cmds_in_ddr(struct rda_dsi_buf *buf)
{
#if 0
	int j,i;
	u8 *d;
	unsigned long long *data = (unsigned long long *)buf->start;
#if 0
	for(j = 0;j < (buf->len >> 3);j++){
		printk("%016llx\n",*data++);
	}
#endif
	d = (u8*)data;
	for(j = 0;j < (buf->len >> 3);j++){
		for(i = 0;i<8;i++)
			printk("%02x",*d++);
		printk("\n");
	}

#endif
}

#if 0
static void dsi_dump_cmds(u32 *buf,int size)
{
#if 1
	int j;
	unsigned long long *data = (unsigned long long *)buf;
	for(j = 0;j < size;j++){
		printk("%08llx\n",*data++);
	}
#endif
}
#endif
static void dsi_pack_num_to_ddr(struct rda_dsi_buf *buf)
{
	int i;

	*buf->data++ = cmdlist_fill_hdr;
	buf->len++;
	for(i = 0;i < 7;i++){
		*buf->data++ = 0;
		buf->len++;
	}
}
#if 0
static void dsi_put_cmds_to_ddr(struct rda_dsi_cmd *cmd,struct rda_dsi_buf *buf)
{
	int i,j;
	u8 pd;

	for(i = 0;i < cmd->dlen;i++){
		pd = *buf->data++ = *cmd->payload++;
		buf->len++;
		for(j = 0;j < 7;j++){
			*buf->data++ = 0;
			buf->len++;
		}
		if(!(((buf->len - PAYLOADY_SIZE) >> 3) & 0x1)){
			*(buf->data - (PAYLOADY_SIZE - 1)) = pd;
			*(buf->data - PAYLOADY_SIZE) = 0;
		}
	}
}
#endif

static void dsi_put_cmds_to_ddr(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *buf)
{
	int i,j,tx_mode;
	const char *payload;
	u8 pd;

	payload = cmd->payload;
	tx_mode = rda_dsi_get_tx_mode();
	if(tx_mode == DSI_LP_MODE){
		for(i = 0;i < cmd->dlen;i++){
			pd = *buf->data++ = *payload++;
			buf->len++;
			for(j = 0;j < 7;j++){
				*buf->data++ = 0;
				buf->len++;
			}
			/*if(!(((buf->len - PAYLOADY_SIZE) >> 3) & 0x1)){*/
				*(buf->data - (PAYLOADY_SIZE - 1)) = pd;
			/*	*(buf->data - PAYLOADY_SIZE) = 0;
			}*/
		}
	}

	if(tx_mode == DSI_HS_MODE){
		/* 32 bits */
		for (i = 0; i < PAYLOADY_SIZE >> 1; i++) {
			pd = *buf->data++ = *payload++;
			buf->len++;
		}
		*buf->data++ = 0;
		buf->len++;

		/* 32 bits */
		pd = *buf->data++ = *payload++;
		buf->len++;
		for (i = 0; i < PAYLOADY_SIZE >> 1; i++) {
			pd = *buf->data++ = 0;
			buf->len++;
		}
	}
}

static void dsi_get_cmdlist_hdr(struct dsi_cmd_list *cmdlist)
{
	int i;
	const struct rda_dsi_cmd *cmds = cmdlist->cmds;
	struct rda_dsi_buf *cmdlist_hdr = cmdlist->header;
	rda_dbg_lcdc("%s\n",__func__);

	for (i = 0; i < cmdlist->cmds_cnt; i++) {
		dsi_get_cmd(cmds,cmdlist_hdr);
		cmdlist_hdr->data += DSI_HOST_HDR_SIZE;
		cmds++;
	}
	/*set start of cmdlist_hdr,the buf just contain hdr bytes,no payload*/
	cmdlist_hdr->data -= cmdlist_hdr->len;
	cmdlist_hdr->hdr = (u32 *)cmdlist_hdr->data;
}

int rda_dsi_cmdlist_init(struct rda_dsi_transfer *trans)
{
	int ret = 0,cmds_num,i,tx_mode;
	dma_addr_t pld_dma_addr;
	const struct rda_dsi_cmd *cmds;
	struct rda_dsi_buf *payload;
	struct dsi_cmd_list *cmdlist;

	rda_dbg_lcdc("%s\n",__func__);
	tx_mode = rda_dsi_get_tx_mode();
	cmdlist = trans->cmdlist;
	cmdlist->payload = kzalloc(sizeof(struct rda_dsi_buf),GFP_KERNEL);
	cmdlist->header = kzalloc(sizeof(struct rda_dsi_buf),GFP_KERNEL);

	if(!cmdlist->payload){
		pr_err("%s : no memory\n", __func__);
		goto error1;
	}
	if(!cmdlist->header){
		pr_err("%s : no memory\n", __func__);
		goto error2;
	}

	/*
	* we put the cmds in ddr and send as video data,for each 8 bytes,we
	* only use the low 1 byte as the payload to send to peripheral,also we need a
	* useless hdr 0xaa before send the first 8*byte in ddr,
	* so alloc (cmds_num + 1) * 8 bytes
	*/
	cmds_num = rda_dsi_get_cmdlist_len(cmdlist,CMDLIST_SEND_FLAG);
	cmdlist->payload->flag = RD_DSI_SEND_CMDLIST;

	if(tx_mode == DSI_LP_MODE){
		cmds_num += 1;
	}

	/*dma alloc*/
	cmdlist->payload->start = dma_alloc_coherent(NULL, cmds_num * PAYLOADY_SIZE,
					&pld_dma_addr, GFP_KERNEL);

	if(!cmdlist->payload->start){
		pr_err("%s: dma alloc nomem\n", __func__);
		goto error2;
	}

	if ((int)cmdlist->payload->start & 0x07){
		pr_err("%s: buf not 8 bytes aligned\n", __func__);
		return -EFAULT;
	}

	cmdlist->payload->dma_addr = pld_dma_addr;
	cmdlist->payload->end = cmdlist->payload->start + cmds_num * PAYLOADY_SIZE;
	cmdlist->payload->size = cmds_num * PAYLOADY_SIZE;
	cmdlist->payload->data = cmdlist->payload->start;
	cmdlist->payload->len = 0;

	/*cmd list arry size*/
	rda_dsi_buf_alloc(cmdlist->header,cmdlist->cmds_cnt * DSI_HOST_HDR_SIZE);
	rda_dsi_buf_init(cmdlist->header);
	cmdlist->header->flag = RD_DSI_CMDLIST_HDR;/*only use hdr,payload already put into ddr*/

	payload = cmdlist->payload;
	cmds = cmdlist->cmds;

	if(tx_mode == DSI_LP_MODE){
		dsi_pack_num_to_ddr(payload);
	}

	for (i = 0; i < cmdlist->cmds_cnt; i++) {
		if(cmds->dtype == DTYPE_GEN_LWRITE
			|| cmds->dtype == DTYPE_DCS_LWRITE){
			dsi_put_cmds_to_ddr(cmds, payload);
		}
		cmds++;
	}

	dsi_dump_cmds_in_ddr(payload);
	dsi_get_cmdlist_hdr(cmdlist);
	cmdlist->flags = CMDLIST_INITED;

	return ret;

error2:
	kfree(cmdlist->header);
error1:
	kfree(cmdlist->payload);

	return -ENOMEM;
}

/*
* Data Identifier Byte
*------------------------
* B7-B6 | B5         ---       B0
*------------------------
*    VC    |               DT
*------------------------
*/

/*
 * mipi dsi gerneric long write
 */
static int rda_dsi_generic_lwrite(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	char *pld;
	u32 *hp;
	int i;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data;
	pld = db->data + DSI_HOST_HDR_SIZE;

	/* fill up payload */
	if (cmd->payload) {
		for (i = 0; i < cmd->dlen; i++)
			*pld++ = cmd->payload[i];
		db->len += cmd->dlen;
	}

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp = DSI_HDR_WC(cmd->dlen);
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_LONG_PKT;
	*hp |= DSI_HDR_DTYPE(DTYPE_GEN_LWRITE);
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	db->data -= DSI_HOST_HDR_SIZE;
	db->len += DSI_HOST_HDR_SIZE;

	return 0;
}

static int rda_dsi_generic_swrite(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	u32 *hp;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data;

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_SHORT_PKT;
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	if(cmd->dlen > 2){
		pr_err("%s: short-write len  should < 2 \n", __func__);
		return -EINVAL;
	}

	if(cmd->dtype == DTYPE_GEN_SWRITE1){
		*hp |= DSI_HDR_DTYPE(DTYPE_GEN_SWRITE1);
		*hp |= DSI_HDR_DATA1(cmd->payload[0]);
		*hp |= DSI_HDR_DATA2(0);
	}else if(cmd->dtype == DTYPE_GEN_SWRITE2){
		*hp |= DSI_HDR_DTYPE(DTYPE_GEN_SWRITE2);
		*hp |= DSI_HDR_DATA1(cmd->payload[0]);
		*hp |= DSI_HDR_DATA2(cmd->payload[1]);
	}else{
		pr_err("%s: Error cmd type! \n",__func__);
	}
	return 0;
}

/*
 * mipi dsi dcs long write
 */
static int rda_dsi_dcs_lwrite(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	char *pld;
	u32 *hp;
	int i;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data; /*get hdr*/
	db->data += DSI_HOST_HDR_SIZE; /*reserve hdr*/
	pld = db->data;

	/* fill up payload */
	if (cmd->payload && !(db->flag & RD_DSI_CMDLIST_HDR)) {
		for (i = 0; i < cmd->dlen; i++)
			*pld++ = cmd->payload[i];
		db->len += cmd->dlen;
	}

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp = DSI_HDR_WC(cmd->dlen);
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_LONG_PKT;
	*hp |= DSI_HDR_DTYPE(DTYPE_DCS_LWRITE);
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	db->data -= DSI_HOST_HDR_SIZE;
	db->len += DSI_HOST_HDR_SIZE;

	return db->len;
}

static int rda_dsi_dcs_swrite(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	u32 *hp;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data;

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_SHORT_PKT;
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	if(cmd->dlen > 2){
		pr_err("%s: short-write len  should < 2 \n", __func__);
		return -EINVAL;
	}

	if(cmd->dtype == DTYPE_DCS_SWRITE){
		*hp |= DSI_HDR_DTYPE(DTYPE_DCS_SWRITE);
		*hp |= DSI_HDR_DATA1(cmd->payload[0]);
		*hp |= DSI_HDR_DATA2(0);
	}
	return 0;
}

static int rda_dsi_dcs_swrite1(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	u32 *hp;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data;

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_SHORT_PKT;
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	if(cmd->dlen > 2){
		pr_err("%s: short-write len  should < 2 \n", __func__);
		return -EINVAL;
	}

	if(cmd->dtype == DTYPE_DCS_SWRITE1){
		*hp |= DSI_HDR_DTYPE(DTYPE_DCS_SWRITE1);
		*hp |= DSI_HDR_DATA1(cmd->payload[0]);
		*hp |= DSI_HDR_DATA2(cmd->payload[1]);
	}
	return 0;
}

static int rda_dsi_set_max_packet(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	u32 *hp;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data;

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_SHORT_PKT;
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	if(cmd->dlen > 2){
		pr_err("%s: short-write len  should < 2 \n", __func__);
		return -EINVAL;
	}

	if(cmd->dtype == DTYPE_MAX_PKTSIZE){
		*hp |= DSI_HDR_DTYPE(DTYPE_MAX_PKTSIZE);
		*hp |= DSI_HDR_DATA1(cmd->payload[0]);
		*hp |= DSI_HDR_DATA2(cmd->payload[1]);
	}
	return 0;
}

static int rda_dsi_dcs_read(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *db)
{
	u32 *hp;
	int tx_mode;

	tx_mode = rda_dsi_get_tx_mode();
	db->hdr = (u32 *)db->data;

	/* fill up header */
	hp = db->hdr;
	*hp = 0;
	*hp |= DSI_HDR_VC(0);
	*hp |= DSI_HDR_BTA;
	if(tx_mode == DSI_HS_MODE)
		*hp |= DSI_HS_MODE;

	if(cmd->dlen > 2){
		pr_err("%s: short-write len  should < 2 \n", __func__);
		return -EINVAL;
	}

	if(cmd->dtype == DTYPE_DCS_READ){
		*hp |= DSI_HDR_DTYPE(DTYPE_DCS_READ);
		*hp |= DSI_HDR_DATA1(cmd->payload[0]);
		*hp |= DSI_HDR_DATA2(0);
	}
	return 0;
}

static int dsi_get_cmd(const struct rda_dsi_cmd *cmd,struct rda_dsi_buf *buf)
{
	int ret = 0;
	switch(cmd->dtype){
	case DTYPE_DCS_SWRITE:
		ret = rda_dsi_dcs_swrite(cmd,buf);
		break;
	case DTYPE_DCS_SWRITE1:
		ret = rda_dsi_dcs_swrite1(cmd,buf);
		break;
	case DTYPE_DCS_LWRITE:
		ret = rda_dsi_dcs_lwrite(cmd,buf);
		break;
	case DTYPE_GEN_SWRITE:
		ret = rda_dsi_generic_swrite(cmd,buf);
		break;
	case DTYPE_GEN_LWRITE:
		ret = rda_dsi_generic_lwrite(cmd,buf);
		break;
	case DTYPE_MAX_PKTSIZE:
		ret = rda_dsi_set_max_packet(cmd,buf);
		break;
	case DTYPE_DCS_READ:
		ret = rda_dsi_dcs_read(cmd,buf);
		break;
	default:
		break;
	}
	return ret;
}

static void rda_dsi_cmd_queue_enable(int cmd_cnt)
{
	rda_write_dsi_reg(0x40,cmd_cnt - 1);/*dsi cmds num,actualy cmd - 1*/
	rda_write_dsi_reg(0x44,0x03);/*dsi cmd queue enable*/
}

static inline void mipi_dsi_tx(u32 *payload,u32 cnt)
{
	int i;
	u32 base_offset = CMD_SEND_REG_BASE;

	for(i = 0;i < cnt;i++){
		rda_write_dsi_reg(base_offset,*payload);
		base_offset += 4;
		payload++;
	}

#if RDA8850E_WORKAROUND
	if(cnt > 1)
		dsi_state->dsi_tx_reg_offset = base_offset - 4;
	else
		dsi_state->dsi_tx_reg_offset = CMD_SEND_REG_BASE;
#endif

}

void rda_dsi_op_mode_config(int mode)
{
	rda_write_dsi_reg(0x14,mode);
}

void rda_dsi_and_lprx_enable(bool on)
{
	rda_write_dsi_reg(0x04,on ? 0x3 : 0);
}

void rda_dsi_cmd_bta_sw_trigger(void)
{
	rda_dbg_lcdc("%s\n",__func__);
	rda_dsi_enable(0);
	mipi_dsi_switch_lp_mode();
	rda_dsi_and_lprx_enable(1);
}

void mipi_dsi_switch_lp_mode(void)
{
	rda_reset_lcdc_for_dsi_tx();
/*
	rda_set_lcdc_for_dsi_cmdlist_tx(0,0);
	rda_write_dsi_reg(0x04,0);//dsi tx cmd mode
	rda_write_dsi_reg(0x14,0);//dsi tx cmd mode
*/
}

void put_cmdlist_for_lcdc_tx(struct dsi_cmd_list *cmdlist,int index)
{
	u32 *hdrlist;
	u32 cmd_cnt;
	struct rda_dsi_buf *hdr = cmdlist->header;

	rda_dsi_enable(0);
	rda_dsi_op_mode_config(DSI_CMD);

	cmd_cnt = (index == CMDLIST_SEND_FLAG) ? cmdlist->cmds_cnt : 1;
	rda_dsi_cmd_queue_enable(cmd_cnt);

	hdrlist = (u32 *)(hdr->start);
	if(index == CMDLIST_SEND_FLAG){
		mipi_dsi_tx(hdrlist,cmdlist->cmds_cnt);
	} else {
		mipi_dsi_tx(hdrlist + index,1);
	}


	if(cmdlist->flags == DSI_NEED_RX)
		rda_dsi_and_lprx_enable(1);
	else
		rda_dsi_enable(1);
}

static int rda_dsi_cmdlist_tx(struct dsi_cmd_list *cmdlist,int index)
{
	int total_cmd_num,tx_mode;
	u32 payload_addr;
	struct rda_dsi_buf *payload = cmdlist->payload;

	rda_dbg_lcdc("%s\n",__func__);

	tx_mode = rda_dsi_get_tx_mode();
	if(!(cmdlist->flags & CMDLIST_LCDC_DSI_INITED)){
		/*0xaa + cmdlist,so cmd_num + 1*/
		total_cmd_num = rda_dsi_get_cmdlist_len(cmdlist,CMDLIST_SEND_FLAG);
		if(tx_mode == DSI_LP_MODE)
			total_cmd_num += 1;
		payload_addr = (u32)payload->dma_addr;

		rda_set_lcdc_for_dsi_cmdlist_tx((u32)payload_addr,total_cmd_num);
		cmdlist->flags |= CMDLIST_LCDC_DSI_INITED;
	}

	put_cmdlist_for_lcdc_tx(cmdlist,index);

	return 0;
}

#if 0
int rda_dsi_read_data(u8 *data)
{
	u32 rxcmd,payload;
	u32 i,base,readcounta,readcountb;
	u8 len = 0;

#if RDA8850E_WORKAROUND
	/*
	*  only rda8850e need read dsi reg according to tx reg offset
	*/
	rda_read_dsi_reg(dsi_state->dsi_tx_reg_offset + 0x80);
	rxcmd = rda_read_dsi_reg(dsi_state->dsi_tx_reg_offset + 0x80);
	rda_dbg_lcdc("%s rxcmd 0x%x \n",__func__,rxcmd);

#else
	rxcmd = rda_read_dsi_reg(0x200);
#endif

	switch(rxcmd & 0xff){
	case DTYPE_ACK_ERR_RESP:
		data[0] = rxcmd & 0xff;
		pr_err("%s: rx ACK_ERR_PACLAGE(0x%x)\n", __func__,rxcmd);
		break;
	case DTYPE_GEN_READ1_RESP:
	case DTYPE_DCS_READ1_RESP:
		len = 1;
		data[0] = (rxcmd >> 8) & 0xff;
		data[1] = (rxcmd) & 0xff;
		break;
	case DTYPE_GEN_READ2_RESP:
	case DTYPE_DCS_READ2_RESP:
		len = 2;
		data[0] = (rxcmd >> 8) & 0xff;
		data[1] = (rxcmd >> 16) & 0xff;
		break;
	case DTYPE_GEN_LREAD_RESP:
	case DTYPE_DCS_LREAD_RESP:
		len = ((rxcmd >> 8) & 0xff) | ((rxcmd >> 16) & 0xff);
		readcounta = len / 4;
		readcountb = len % 4;

#if RDA8850E_WORKAROUND
		/*dumy read only for original rda8850e soc,this read should be remove later!!! */
		payload = rda_read_dsi_reg(0x300);
#endif
		rda_dbg_lcdc("%s payload 0x%x \n",__func__,payload);

		for(i = 0;i < readcounta * 4;){
			payload = rda_read_dsi_reg(0x300);
			rda_dbg_lcdc("%s payload 0x%x \n",__func__,payload);
			data[i] = payload & 0xff;
			data[i + 1] = (payload >> 8) & 0xff;
			data[i + 2] = (payload >> 16) & 0xff;
			data[i + 3] = (payload >> 24) & 0xff;
			i += 4;
		}

		if(readcountb){
			base = readcounta * 4;
			payload = rda_read_dsi_reg(0x300);
			rda_dbg_lcdc("%s payload 0x%x \n",__func__,payload);
			for(i = base;i < base + readcountb;i++){
				data[i] = (payload >> (i * 8)) & 0xff;
			}
		}
		break;
	default:
		pr_err("%s: rx ACK_ERR_PACLAGE(0x%x)\n", __func__,rxcmd);
		break;
	}

	return len;
}
#endif
int rda_dsi_read_data(u8 *data, u32 len)
{
	u32 rxcmd, payload;
	u32 i, counta, countb;
	u32 get_len = 0;

	rxcmd = rda_read_dsi_reg(dsi_state->dsi_tx_reg_offset + 0x80);
	rda_dbg_lcdc("%s rxcmd 0x%x \n", __func__, rxcmd);

	switch (rxcmd & 0xff) {
	case DTYPE_ACK_ERR_RESP:
		data[0] = rxcmd & 0xff;
		pr_err("%s: rx ACK_ERR_PACLAGE(0x%x)\n", __func__, rxcmd);
		break;
	case DTYPE_GEN_READ1_RESP:
	case DTYPE_DCS_READ1_RESP:
		get_len = 1;
		data[0] = (rxcmd >> 8) & 0xff;
		data[1] = (rxcmd) & 0xff;
		break;
	case DTYPE_GEN_READ2_RESP:
	case DTYPE_DCS_READ2_RESP:
		get_len = 2;
		data[0] = (rxcmd >> 8) & 0xff;
		data[1] = (rxcmd >> 16) & 0xff;
		break;
	case DTYPE_GEN_LREAD_RESP:
	case DTYPE_DCS_LREAD_RESP:
		get_len = (rxcmd >> 8) & 0xffff;
		if (get_len > len) {
			pr_err("%s:got data length over the buffer length", __func__);
			return -1;
		}
		counta = rounddown(len, 4);
		countb = len % 4;

		for (i = 0;i < counta; i += 4) {
			payload = rda_read_dsi_reg(0x300);
			memcpy(&data[i], &payload, 4);
		}

		if (countb) {
			payload = rda_read_dsi_reg(0x300);
			for (i = 0; i < countb; i++) {
				data[counta + i] = (payload >> (i * 8)) & 0xff;
			}
		}
		break;
	default:
		pr_err("%s: rx ACK_ERR_PACLAGE(0x%x)\n", __func__, rxcmd);
		break;
	}

	return len;
}

int rda_dsi_send_lpkt(struct dsi_cmd_list *cmdlist,int index)
{
	int ret = 0;
	rda_dbg_lcdc("%s\n",__func__);

	mutex_lock(&dsi_state->dsi_cmd_mutex);
	if(!(cmdlist->flags & CMDLIST_INITED)){
		mutex_unlock(&dsi_state->dsi_cmd_mutex);
		pr_err("%s cmdlist not init\n",__func__);
		return ret;
	}

	ret = rda_dsi_cmdlist_tx(cmdlist,index);
	ret = rda_mipi_dsi_txrx_status();
	if(ret){
		pr_err("%s send cmdlist error\n",__func__);
	}
	mutex_unlock(&dsi_state->dsi_cmd_mutex);

	return ret;

}

int rda_dsi_cmdlist_send_lpkt(struct rda_dsi_transfer *trans)
{
	int i;
	int ret = 0;
	struct dsi_cmd_list *cmdlist;

	rda_dsi_cmdlist_init(trans);
	cmdlist = trans->cmdlist;

	for(i = 0;i < cmdlist->cmds_cnt;i++) {
		ret = rda_dsi_send_lpkt(cmdlist,i);
		if(ret){
			pr_err("%s error\n",__func__);
		}
		//mdelay(10);
		if(cmdlist->cmds[i].delay){
			msleep(cmdlist->cmds[i].delay);
		}
	}

	rda_dsi_cmdlist_release(trans);

	return ret;
}

int rda_dsi_cmdlist_send(struct rda_dsi_transfer *trans)
{
	int ret = 0;
	struct dsi_cmd_list *cmdlist;
	rda_dbg_lcdc("%s\n",__func__);

	cmdlist = trans->cmdlist;
	if(cmdlist->cmds_cnt > CMDLIST_SEND_MAX_NUM){
		pr_err("%s cmdlist'cmd bumbers exceed the max value of dsi reg(32)\n",__func__);
		return -1;
	}

	mutex_lock(&dsi_state->dsi_cmd_mutex);
	ret = rda_dsi_cmdlist_init(trans);
	if(ret){
		mutex_unlock(&dsi_state->dsi_cmd_mutex);
		pr_err("%s cmdlist init error\n",__func__);
		return ret;
	}

	ret = rda_dsi_cmdlist_tx(cmdlist,CMDLIST_SEND_FLAG);
	ret = rda_mipi_dsi_txrx_status();
	if(ret){
		pr_err("%s send cmdlist error\n",__func__);
	}
	rda_dsi_cmdlist_release(trans);

	mutex_unlock(&dsi_state->dsi_cmd_mutex);

	return ret;
}

int rda_dsi_cmd_send(const struct rda_dsi_cmd *cmd)
{
	int ret = 0;
	struct rda_dsi_buf *cmd_hdr;
	rda_dbg_lcdc("%s\n",__func__);

	mutex_lock(&dsi_state->dsi_cmd_mutex);

	cmd_hdr= kzalloc(sizeof(struct rda_dsi_buf),GFP_KERNEL);
	if(!cmd_hdr){
		pr_err("%s : no memory\n", __func__);
		mutex_unlock(&dsi_state->dsi_cmd_mutex);
		return -ENOMEM;
	}

	rda_dsi_buf_alloc(cmd_hdr,DSI_HOST_HDR_SIZE);
	rda_dsi_buf_init(cmd_hdr);

	dsi_get_cmd(cmd,cmd_hdr);

	rda_dsi_enable(0);
	rda_dsi_op_mode_config(DSI_CMD);
	rda_dsi_cmd_queue_enable(1);
	mipi_dsi_tx(cmd_hdr->hdr,1);

	if(cmd->dtype == DTYPE_DCS_READ)
		rda_dsi_and_lprx_enable(1);
	else
		rda_dsi_enable(1);

	ret = rda_mipi_dsi_txrx_status();
	if(ret){
		pr_err("%s send cmd error\n",__func__);
	}

	if(cmd->delay)
		mdelay(cmd->delay);

	/*clear */
	rda_dsi_buf_release(cmd_hdr);
	kfree(cmd_hdr);

	mutex_unlock(&dsi_state->dsi_cmd_mutex);

	return ret;
}

/*send multi short cmd pkt*/
int rda_dsi_spkt_cmds_read(const struct rda_dsi_cmd *cmds,int cmds_cnt)
{
	int ret = 0,i;
	const struct rda_dsi_cmd * pcmds = cmds;
	struct rda_dsi_buf *cmds_hdr;
	rda_dbg_lcdc("%s\n",__func__);

	mutex_lock(&dsi_state->dsi_cmd_mutex);

	cmds_hdr= kzalloc(sizeof(struct rda_dsi_buf),GFP_KERNEL);
	if(!cmds_hdr){
		pr_err("%s : no memory\n", __func__);
		mutex_unlock(&dsi_state->dsi_cmd_mutex);
		return -ENOMEM;
	}

	rda_dsi_buf_alloc(cmds_hdr,cmds_cnt*DSI_HOST_HDR_SIZE);
	rda_dsi_buf_init(cmds_hdr);

	rda_dsi_and_lprx_enable(0);
	rda_dsi_op_mode_config(DSI_CMD);
	rda_dsi_cmd_queue_enable(cmds_cnt);

	for (i = 0; i < cmds_cnt; i++) {
		dsi_get_cmd(pcmds,cmds_hdr);
		cmds_hdr->data += DSI_HOST_HDR_SIZE;
		pcmds++;
	}

	cmds_hdr->hdr = (u32 *)cmds_hdr->start;

	mipi_dsi_tx(cmds_hdr->hdr,cmds_cnt);

	rda_dsi_and_lprx_enable(1);
	ret = rda_mipi_dsi_txrx_status();
	if(ret){
		pr_err("%s send cmd error\n",__func__);
	}

	rda_dsi_buf_release(cmds_hdr);
	kfree(cmds_hdr);

	mutex_unlock(&dsi_state->dsi_cmd_mutex);

	return ret;
}

void rda_dsi_set_tx_mode(int mode)
{
	switch(mode){
	case DSI_LP_MODE:
	case DSI_HS_MODE:
		dsi_state->tx_mode = mode;
		break;
	default:
		dsi_state->tx_mode = 0;
		pr_err("dsi invalid tx mode\n");
		break;
	}
}

int rda_dsi_get_tx_mode(void)
{
	return dsi_state->tx_mode;
}

int rda_mipi_dsi_init(struct mipi_panel_info *mipi_pinfo)
{
	if (!dsi_state) {
		dsi_state = kzalloc(sizeof(struct dsi_status),GFP_KERNEL);
		if (!dsi_state) {
			pr_err("%s : no memory for dsi_state\n", __func__);
			return -ENOMEM;
		}
		dsi_state->tx_mode = DSI_LP_MODE;
		mutex_init(&dsi_state->dsi_cmd_mutex);
	}
	return 0;
}

void rda_mipi_dsi_deinit(struct mipi_panel_info *mipi_pinfo)
{
	if(dsi_state){
		mutex_destroy(&dsi_state->dsi_cmd_mutex);
		kfree(dsi_state);
		dsi_state = NULL;
	}
}
