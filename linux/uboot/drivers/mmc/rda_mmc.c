#include <asm/io.h>
#include "common.h"
#include <errno.h>
#include <mmc.h>
#include <malloc.h>
#include <asm/arch/hardware.h>
#include <asm/arch/reg_mmc.h>
#include <asm/arch/reg_ifc.h>
#include <asm/arch/ifc.h>
#include <asm/arch/rda_sys.h>
#include <mmc/mmcpart.h>
#include <rda/tgt_ap_board_config.h>
#include <asm/arch/spl_board_info.h>

//#define SDMMC_DEBUG

#ifdef CONFIG_RDA_FPGA
#define CONFIG_APB2_CLOCK         26000000
#else
#define CONFIG_APB2_CLOCK         200000000
#endif

#define SECOND                    * CONFIG_SYS_HZ_CLOCK
#define MCD_CMD_TIMEOUT           ( 2 SECOND / 10 )
#define MCD_RESP_TIMEOUT          ( 2 SECOND / 10 )
#define MCD_DATA_TIMEOUT          ( 2 SECOND )
#define MMC_DEV_NUM 2
struct mmc_host {
	int dev_num;
	u32 phyaddr;
	u8  mclk_adj;
	u8  mclk_inv;
	u16  reserved;
};

struct emmc_mfr_mclk_adj_inv {
	u8 mfr_id;
	u8 mclk_adj;
	u8 mclk_inv;
	u8 mclk_inv_kernel;/*Because the u-boot and kernel mode isn't same, so inv set is different sometime*/
};

static const struct emmc_mfr_mclk_adj_inv emmc_mclk_adj_inv[] = {
	{MMC_MFR_TOSHIBA, 0, 0, 1},
	{MMC_MFR_GIGADEVICE, 0, 0, 1},
	{MMC_MFR_SAMSUNG, 0, 0, 1},
	{MMC_MFR_SANDISK, 0, 0, 1},
	{MMC_MFR_HYNIX, 0, 0, 1},
	{MMC_MFR_MICRON, 0, 0, 1},
	{MMC_MFR_MICRON1, 0, 0, 1},
	{0, 0, 0, 0},
};

struct mmc mmc_device_glob[MMC_DEV_NUM];
struct mmc_host mmc_host_dev[MMC_DEV_NUM];
static int rda_ddr_mode = 0;

typedef struct
{
	/// This address in the system memory
	u8* sysMemAddr;
	/// Quantity of data to transfer, in blocks
	u32 blockNum;
	/// Block size
	u32 blockSize;
	HAL_SDMMC_DIRECTION_T direction;
	HAL_IFC_REQUEST_ID_T ifcReq;
	u32 channel;
} HAL_SDMMC_TRANSFER_T;

u64 hal_getticks(void)
{
	return get_ticks();
}

static void hal_send_cmd(struct mmc *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
	u32 configReg = 0;
	struct mmc_host *host = dev->priv;
	u32 ddr_bits = 0;
	HWP_SDMMC_T *  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H)
	/* check ddr mode is enabled */
	if(rda_ddr_mode) {
		ddr_bits = (1<<17) | (1<<23);
	}
#endif

	//hwp_sdmmc->SDMMC_CONFIG = SDMMC_AUTO_FLAG_EN;
	hwp_sdmmc->SDMMC_CONFIG = 0x00000000 | ddr_bits;

	configReg = SDMMC_SDMMC_SENDCMD;
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		configReg |= SDMMC_RSP_EN;
		if (cmd->resp_type & MMC_RSP_136)
			configReg |= SDMMC_RSP_SEL_R2;
		else if (cmd->resp_type & MMC_RSP_CRC)
			configReg |= SDMMC_RSP_SEL_OTHER;
		else
			configReg |= SDMMC_RSP_SEL_R3;
	}

	/* cases for data transfer */
	if (cmd->cmdidx == MMC_CMD_READ_SINGLE_BLOCK) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_READ);
	} else if (cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_READ | SDMMC_S_M_SEL_MULTIPLE);
	} else if (cmd->cmdidx == MMC_CMD_WRITE_SINGLE_BLOCK) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_WRITE);
#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H)
		/* when do write, the edge selection is reverse.
		 * it's a workaround. */
		if(rda_ddr_mode) {
			ddr_bits = (1<<17);
		}
#endif
	} else if (cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_WRITE | SDMMC_S_M_SEL_MULTIPLE);
#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H)
		/* when do write, the edge selection is reverse.
		 * it's a workaround. */
		if(rda_ddr_mode) {
			ddr_bits = (1<<17);
		}
#endif
	} else if (cmd->cmdidx == SD_CMD_APP_SEND_SCR) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_READ);
	} else if (cmd->cmdidx == SD_CMD_SWITCH_FUNC && data) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_READ);
	} else if (cmd->cmdidx == MMC_CMD_SEND_EXT_CSD) {
		configReg |= (SDMMC_RD_WT_EN | SDMMC_RD_WT_SEL_READ);
	}

	configReg |= ddr_bits;
#ifdef SDMMC_DEBUG
	printf("  idx_reg = 0x%08x, arg_reg = 0x%08x, config = 0x%08x\n",
		SDMMC_COMMAND(cmd->cmdidx),
		SDMMC_ARGUMENT(cmd->cmdarg),
		configReg);
#endif

	hwp_sdmmc->SDMMC_CMD_INDEX = SDMMC_COMMAND(cmd->cmdidx);
	hwp_sdmmc->SDMMC_CMD_ARG   = SDMMC_ARGUMENT(cmd->cmdarg);
	//hwp_sdmmc->SDMMC_CONFIG    = configReg |SDMMC_AUTO_FLAG_EN;
	hwp_sdmmc->SDMMC_CONFIG    = configReg;
}

static int hal_cmd_done(struct mmc *dev)
{
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

	return (!(hwp_sdmmc->SDMMC_STATUS & SDMMC_NOT_SDMMC_OVER));
}

static int hal_wait_cmd_done(struct mmc *dev)
{
	u64 startTime = hal_getticks();
	u64 time_out;

	time_out = MCD_CMD_TIMEOUT;

	while(hal_getticks() - startTime < time_out && !hal_cmd_done(dev));

	if (!hal_cmd_done(dev))
	{
		printf("cmd waiting timeout\n");
		return TIMEOUT;
	}
	else
	{
		return 0;
	}
}

HAL_SDMMC_OP_STATUS_T hal_get_op_status(struct mmc *dev)
{
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

	return ((HAL_SDMMC_OP_STATUS_T)(u32)hwp_sdmmc->SDMMC_STATUS);
}

static int hal_wait_cmd_resp(struct mmc *dev)
{
	HAL_SDMMC_OP_STATUS_T status = hal_get_op_status(dev);
	u64 startTime = hal_getticks();
	u64 rsp_time_out;

	rsp_time_out = MCD_RESP_TIMEOUT;

	while(hal_getticks() - startTime < rsp_time_out &&status.fields.noResponseReceived){
		status = hal_get_op_status(dev);
	}

	if (status.fields.noResponseReceived){
		printf("  response timeout\n");
		return TIMEOUT;
	}

	if(status.fields.responseCrcError){
		printf("  response CRC error\n");
		return -EILSEQ;
	}

	return 0;
}

static void hal_get_resp(struct mmc *dev, struct mmc_cmd *cmd)
{
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = hwp_sdmmc->SDMMC_RESP_ARG3;
			cmd->response[1] = hwp_sdmmc->SDMMC_RESP_ARG2;
			cmd->response[2] = hwp_sdmmc->SDMMC_RESP_ARG1;
			cmd->response[3] = hwp_sdmmc->SDMMC_RESP_ARG0 << 1;
		}else {
			cmd->response[0] = hwp_sdmmc->SDMMC_RESP_ARG3;
			cmd->response[1] = 0;
			cmd->response[2] = 0;
			cmd->response[3] = 0;
		}
	}
	else {
		cmd->response[0] = 0;
		cmd->response[1] = 0;
		cmd->response[2] = 0;
		cmd->response[3] = 0;
	}
}

static int hal_data_transfer_start(struct mmc *dev,HAL_SDMMC_TRANSFER_T* transfer)
{
	u32 length = 0;
	u32 lengthExp = 0;
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

	length = transfer->blockSize;

	// The block size register
	while (length != 1)
	{
		length >>= 1;
		lengthExp++;
	}

	// Configure amount of data
	hwp_sdmmc->SDMMC_BLOCK_CNT  = SDMMC_SDMMC_BLOCK_CNT(transfer->blockNum);
	hwp_sdmmc->SDMMC_BLOCK_SIZE = SDMMC_SDMMC_BLOCK_SIZE(lengthExp);

	// Configure Bytes reordering
	hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);
	//hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_L_ENDIAN(1);

	switch (transfer->direction){
		case HAL_SDMMC_DIRECTION_READ:
			if (0 == host->dev_num)
				transfer->ifcReq = HAL_IFC_SDMMC_RX;
			else
				transfer->ifcReq = HAL_IFC_SDMMC3_RX;
			break;

		case HAL_SDMMC_DIRECTION_WRITE:
			if (0 == host->dev_num)
				transfer->ifcReq = HAL_IFC_SDMMC_TX;
			else
				transfer->ifcReq = HAL_IFC_SDMMC3_TX;
			break;

		default:
			printf("hal_SdmmcTransfer with wrong direction %d\n", transfer->direction);
		return -EILSEQ;
	}

	transfer->channel = hal_IfcTransferStart(transfer->ifcReq, transfer->sysMemAddr,
					transfer->blockNum*transfer->blockSize,
					HAL_IFC_SIZE_32_MODE_MANUAL);
	if (transfer->channel == HAL_UNKNOWN_CHANNEL){
		printf("hal_IfcTransferStart error\n");
		return -EILSEQ;
	}
	return 0;
}

static void hal_data_transfer_stop(struct mmc *dev, HAL_SDMMC_TRANSFER_T* transfer)
{
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

	// Configure amount of data
	hwp_sdmmc->SDMMC_BLOCK_CNT  = SDMMC_SDMMC_BLOCK_CNT(0);
	hwp_sdmmc->SDMMC_BLOCK_SIZE = SDMMC_SDMMC_BLOCK_SIZE(0);

	//  Put the FIFO in reset state.
	//hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);
	hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);

	hal_IfcChannelFlush(transfer->ifcReq, transfer->channel);
	while(!hal_IfcChannelIsFifoEmpty(transfer->ifcReq, transfer->channel));
	hal_IfcChannelRelease(transfer->ifcReq, transfer->channel);
	transfer->channel = HAL_UNKNOWN_CHANNEL;
}

static int hal_data_transfer_done(struct mmc *dev, HAL_SDMMC_TRANSFER_T* transfer)
{
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);

	// The link is not full duplex. We check both the
	// direction, but only one can be in progress at a time.

	if (transfer->channel != HAL_UNKNOWN_CHANNEL)
	{
		// Operation in progress is a read.
		// The SDMMC module itself can know it has finished
		if ((hwp_sdmmc->SDMMC_INT_STATUS & SDMMC_DAT_OVER_INT)
		 && (hwp_sysIfc->std_ch[transfer->channel].tc == 0))
		{
			// Transfer is over
			hwp_sdmmc->SDMMC_INT_CLEAR = SDMMC_DAT_OVER_CL;
			hal_IfcChannelRelease(transfer->ifcReq, transfer->channel);

			// We finished a read
			transfer->channel = HAL_UNKNOWN_CHANNEL;

			//  Put the FIFO in reset state.
			//hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);
			hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);

			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		printf("unknown channel\n");
		return 1;
	}
}

static int hal_wait_data_transfer_done(struct mmc *dev,
			HAL_SDMMC_TRANSFER_T* transfer)
{
	u64 startTime = hal_getticks();
	u64 tran_time_out = MCD_DATA_TIMEOUT * transfer->blockNum;
#ifdef SDMMC_DEBUG
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);
#endif

	// Wait
	while(!hal_data_transfer_done(dev, transfer)) {
		if (hal_getticks() - startTime > (tran_time_out)) {
			printf("data transfer timeout\n");
#ifdef SDMMC_DEBUG
			printf("SDMMC_STATUS=%#x, INT_STATUS=%#x\n", hwp_sdmmc->SDMMC_STATUS,
					hwp_sdmmc->SDMMC_INT_STATUS);
#endif
			hal_data_transfer_stop(dev, transfer);
			return TIMEOUT;
		}
	}
	return 0;
}

static int hal_data_read_check_crc(struct mmc *dev)
{
	HAL_SDMMC_OP_STATUS_T operationStatus;

	operationStatus = hal_get_op_status(dev);
	if (operationStatus.fields.dataError != 0) {
	// 0 means no CRC error during transmission
#ifdef SDMMC_DEBUG
		printf("data_read_check_crc fail, status:%08x\n",
				operationStatus.reg);
#endif
		return -EILSEQ;
	} else {
		return 0;
	}
}

static int hal_data_write_check_crc(struct mmc *dev)
{
	HAL_SDMMC_OP_STATUS_T operationStatus;

	operationStatus = hal_get_op_status(dev);
	if (operationStatus.fields.crcStatus != 2) {
	// 0b010 = transmissionRight TODO a macro ?
#ifdef SDMMC_DEBUG
		printf("data_write_check_crc fail, status:%08x\n",
				operationStatus.reg);
#endif
		return -EILSEQ;
	} else {
		return 0;
	}
}

/* send command to the mmc card and wait for results */
static int do_command(struct mmc *dev,
			    struct mmc_cmd *cmd,
			    struct mmc_data *data)
{
	int result;
	//struct mmc_host *host = dev->priv;

#ifdef SDMMC_DEBUG
	printf("do_command, cmdidx = %d, cmdarg = 0x%08x, resp_type = %x\n",
		cmd->cmdidx, cmd->cmdarg, cmd->resp_type);
#endif

	hal_send_cmd(dev, cmd, data);
	result = hal_wait_cmd_done(dev);
	if (result)
		return result;
	if (cmd->resp_type & MMC_RSP_PRESENT)
	{
		result= hal_wait_cmd_resp(dev);
		if (result)
			return result;
	}
	hal_get_resp(dev, cmd);

#ifdef SDMMC_DEBUG
	printf("  response: %08x %08x %08x %08x\n",
		cmd->response[0], cmd->response[1],
		cmd->response[2], cmd->response[3]);
#endif

	/* After CMD2 set RCA to a none zero value. */
	//if (cmd->cmdidx == MMC_CMD_ALL_SEND_CID)
	//	dev->rca = 10;

	/* After CMD3 open drain is switched off and push pull is used. */
	//if (cmd->cmdidx == MMC_CMD_SET_RELATIVE_ADDR) {
	// we don't actually need this
	//}

	return result;
}

static int do_data_transfer(struct mmc *dev,
			    struct mmc_cmd *cmd,
			    struct mmc_data *data)
{
	int result = 0;
	//struct mmc_host *host = dev->priv;
	HAL_SDMMC_TRANSFER_T data_transfer;

#ifdef SDMMC_DEBUG
	printf("do_data_transfer, cmdidx = %d, flag = %d, blks = %d, addr = 0x%08x\n",
		cmd->cmdidx, data->flags, data->blocks,
		(data->flags & MMC_DATA_READ)?(u32)data->dest:(u32)data->src);
#endif

	if (data->flags & MMC_DATA_READ) {
		flush_dcache_range((ulong)data->dest, (ulong)data->dest +
			data->blocks * data->blocksize);

		data_transfer.sysMemAddr = (u8*)data->dest;
		data_transfer.blockNum   = data->blocks;
		data_transfer.blockSize  = data->blocksize;
		data_transfer.direction  = HAL_SDMMC_DIRECTION_READ;

		// Initiate data migration through Ifc.
		result = hal_data_transfer_start(dev, &data_transfer);
		if (result)
			return result;

		result = do_command(dev, cmd, data);
		if (result) {
			hal_data_transfer_stop(dev, &data_transfer);
			return result;
		}

		result = hal_wait_data_transfer_done(dev, &data_transfer);
		if (result)
		{
			hal_data_transfer_stop(dev, &data_transfer);
			return result;
		}

		result = hal_data_read_check_crc(dev);
		if (result)
		{
			printf("data read crc check fail\n");
			return result;
		}
#ifdef SDMMC_DEBUG
		{
			int i;
			for (i=0;i<data->blocksize;i++)
			{
				printf("%02x ", data->dest[i]);
				if ((i+1)%16 == 0)
					printf("\n");
			}
		}
#endif
		invalidate_dcache_range((ulong)data->dest,
			(ulong)data->dest + data->blocks * data->blocksize);
	} else if (data->flags & MMC_DATA_WRITE) {
		flush_dcache_range((ulong)data->src, (ulong)data->src +
			data->blocks * data->blocksize);

		data_transfer.sysMemAddr = (u8*)data->src;
		data_transfer.blockNum   = data->blocks;
		data_transfer.blockSize  = data->blocksize;
		data_transfer.direction  = HAL_SDMMC_DIRECTION_WRITE;

		// Initiate data migration through Ifc.
		result = hal_data_transfer_start(dev, &data_transfer);
		if (result)
			return result;

		result = do_command(dev, cmd, data);
		if (result) {
			hal_data_transfer_stop(dev, &data_transfer);
			return result;
		}

		result = hal_wait_data_transfer_done(dev, &data_transfer);
		if (result)
		{
			hal_data_transfer_stop(dev, &data_transfer);
			return result;
		}

		result = hal_data_write_check_crc(dev);
		if (result)
		{
			printf("data write crc check fail\n");
			return result;
		}
	}

	return result;
}

static int host_request(struct mmc *dev,
			struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	int result;

	if (data)
		result = do_data_transfer(dev, cmd, data);
	else
		result = do_command(dev, cmd, data);

	return result;
}

/* MMC uses open drain drivers in the enumeration phase */
static int mmc_host_reset(struct mmc *dev)
{
	return 0;
}

static void host_set_ios(struct mmc *dev)
{
	struct mmc_host *host = dev->priv;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);
	unsigned long clk_div;
	unsigned long apb2_clock = CONFIG_APB2_CLOCK;

	/* Ramp up the clock rate */
	if (dev->clock) {
		clk_div = apb2_clock / (2 * dev->clock);
		if (apb2_clock % (2 * dev->clock))
			clk_div ++;

		if (clk_div >= 1) {
			clk_div -= 1;
		}
		if (clk_div > 255) {
		/* clock too slow */
			clk_div = 255;
		}

		hwp_sdmmc->SDMMC_TRANS_SPEED = SDMMC_SDMMC_TRANS_SPEED(clk_div);
		if( host->dev_num == 0 ) {
			hwp_sdmmc->SDMMC_MCLK_ADJUST = SDMMC_SDMMC_MCLK_ADJUST(_TGT_AP_SDMMC1_MCLK_ADJ);
#if defined(_TGT_AP_SDMMC1_MCLK_INV) && (_TGT_AP_SDMMC1_MCLK_INV)
			hwp_sdmmc->SDMMC_MCLK_ADJUST |= SDMMC_CLK_INV;
#endif
#ifdef CONFIG_RDA_FPGA
			hwp_sdmmc->SDMMC_MCLK_ADJUST = 0;
#endif
		} else {
			hwp_sdmmc->SDMMC_MCLK_ADJUST = SDMMC_SDMMC_MCLK_ADJUST(host->mclk_adj);
			if(host->mclk_inv)
				hwp_sdmmc->SDMMC_MCLK_ADJUST |= SDMMC_CLK_INV;
		}
#ifdef SDMMC_DEBUG
		printf("set clock to %d, div = %lu\n", dev->clock, clk_div);
#endif
	}

	/* Set the bus width */
	if (dev->bus_width) {
#ifdef SDMMC_DEBUG
		printf("set bus_width to %#x\n", dev->bus_width);
#endif

		switch (dev->bus_width & 0xf) {
		case 1:
			hwp_sdmmc->SDMMC_DATA_WIDTH = 1;
			break;
		case 4:
			hwp_sdmmc->SDMMC_DATA_WIDTH = 4;
			break;
		case 8:
			hwp_sdmmc->SDMMC_DATA_WIDTH = 8;
			break;
		default:
			printf("invalid bus width\n");
			break;
		}

		/* check ddr enable/disable bit from bus_width */
		if(dev->bus_width & 0x80000000) {
			rda_ddr_mode = 1;
#ifdef SDMMC_DEBUG
			printf("mmc ddr mode is enabled.\n");
#endif
		} else {
			rda_ddr_mode = 0;
		}
	}
}


struct mmc *alloc_mmc_struct(unsigned int dev_num)
{
	struct mmc_host *host = NULL;
	struct mmc *mmc_device = NULL;

	if (dev_num >=  MMC_DEV_NUM)
		return NULL;

	host = &mmc_host_dev[dev_num];
	if (!host)
		return NULL;

	host->dev_num = dev_num;
	if (0 == dev_num)
		host->phyaddr =  RDA_SDMMC1_BASE;
	else
		host->phyaddr =  RDA_SDMMC3_BASE;

	mmc_device = &mmc_device_glob[dev_num];
	if (!mmc_device)
		goto err;

	mmc_device->priv = host;
	return mmc_device;

err:
	return NULL;
}

#define VOLTAGE_WINDOW_MMC 0x00FF8080

/*
 * mmc_host_init - initialize the mmc controller.
 * Set initial clock and power for mmc slot.
 * Initialize mmc struct and register with mmc framework.
 */
static int rda_mmc_host_init(struct mmc *dev)
{
	struct mmc_host *host = dev->priv;
	unsigned long clk_div;
	HWP_SDMMC_T*  hwp_sdmmc = (HWP_SDMMC_T*)(host->phyaddr);
	struct spl_emmc_info *info = get_bd_spl_emmc_info();

	// clk_div = 0;		// for APB2 = 48M, 48/2
	//clk_div = 1;		// for APB2 = 120M, 120/4
	clk_div = 0x21;		// for APB2 = 240M, 240/16

	host->reserved = 0;

	// We don't use interrupts.
	hwp_sdmmc->SDMMC_INT_MASK = 0x0;
	hwp_sdmmc->SDMMC_TRANS_SPEED = SDMMC_SDMMC_TRANS_SPEED(clk_div);
	if (0 == host->dev_num){
		hwp_sdmmc->SDMMC_MCLK_ADJUST = SDMMC_SDMMC_MCLK_ADJUST(_TGT_AP_SDMMC1_MCLK_ADJ);
#if defined(_TGT_AP_SDMMC1_MCLK_INV) && (_TGT_AP_SDMMC1_MCLK_INV)
		hwp_sdmmc->SDMMC_MCLK_ADJUST |= SDMMC_CLK_INV;
#endif
#ifdef CONFIG_RDA_FPGA
		hwp_sdmmc->SDMMC_MCLK_ADJUST = 0;
#endif
		dev->host_caps = MMC_MODE_4BIT | MMC_MODE_HS | MMC_MODE_HC;
	} else if (1 == host->dev_num){
		host->mclk_adj = 0;
		host->mclk_inv = 0;
		if (info->manufacturer_id){
			int i;

			for(i = 0; emmc_mclk_adj_inv[i].mfr_id != 0; i++ )
				if (info->manufacturer_id == emmc_mclk_adj_inv[i].mfr_id ){
					host->mclk_adj = emmc_mclk_adj_inv[i].mclk_adj;
					host->mclk_inv = emmc_mclk_adj_inv[i].mclk_inv;
					break;
				}

			if (emmc_mclk_adj_inv[i].mfr_id == 0)
				printf("Cannot find the emmc corresponding mclk adj and inv.Now use default zero. Please add it \n");
		} else
			printf("Spl init emmc first, use default zero mclk adj and inv.\n");
		hwp_sdmmc->SDMMC_MCLK_ADJUST = SDMMC_SDMMC_MCLK_ADJUST(host->mclk_adj);
		if(host->mclk_inv)
			hwp_sdmmc->SDMMC_MCLK_ADJUST |= SDMMC_CLK_INV;

#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H)
		dev->host_caps = MMC_MODE_8BIT | MMC_MODE_HS | MMC_MODE_HC | MMC_MODE_DDR_52MHz;
#else
		dev->host_caps = MMC_MODE_8BIT | MMC_MODE_HS | MMC_MODE_HC;
#endif
	} else{
		printf("invalid mmc device number \n");
	}

	sprintf(dev->name, "MMC");
	dev->clock = 400000;
	dev->send_cmd = host_request;
	dev->set_ios = host_set_ios;
	dev->init = mmc_host_reset;
	dev->voltages = VOLTAGE_WINDOW_MMC;
	dev->f_min = 400000;
	if( host->dev_num == 0 ) {
		dev->f_max = _TGT_AP_SDMMC1_MAX_FREQ;
	} else {
		dev->f_max = _TGT_AP_SDMMC3_MAX_FREQ;
	}

#ifdef CONFIG_RDA_FPGA
	printf("mmc host %d init, speed=0x%x, adjust = 0x%x \n",
		host->dev_num,  hwp_sdmmc->SDMMC_TRANS_SPEED,hwp_sdmmc->SDMMC_MCLK_ADJUST);
#endif
	return 0;
}
int mmc_set_host_mclk_adj_inv(struct mmc *mmc, u8 adj, u8 inv)
{
	struct mmc_host *host = mmc->priv;

	if (!host)
		return -1;

	host->mclk_adj = adj;
	host->mclk_inv = inv;

	return 0;
}

/*
 * MMC write function
 */
int mmc_read(struct mmc *mmc, u64 from, uchar *buf, int size)
{
	unsigned long cnt;
	unsigned int start_block, block_num, block_size;
	int blksz_shift;

	if (!mmc) {
#ifdef SDMMC_DEBUG
		printf("mmc_read: mmc device not found!!\n");
#endif
		return -1;
	}

	block_size = mmc->read_bl_len;
	blksz_shift = mmc->block_dev.log2blksz;
	start_block = from >> blksz_shift;
	block_num = (size + block_size - 1) >> blksz_shift;

	/* Read the header too to avoid extra memcpy */
	cnt = mmc->block_dev.block_read(mmc->block_dev.dev,
			start_block,
			block_num, (void *)buf);
	if (cnt != block_num) {
#ifdef SDMMC_DEBUG
		printf("mmc blk read error!\n");
#endif
		return -1;
	}

	return size;
}


int mmc_write(struct mmc *mmc, u64 to, uchar *buf, int size)
{
	unsigned long cnt;
	unsigned int start_block, block_num, block_size;
	int blksz_shift;

	if (!mmc) {
#ifdef SDMMC_DEBUG
		printf("mmc_write: mmc device not found!!\n");
#endif
		return -1;
	}

	blksz_shift = mmc->block_dev.log2blksz;
	block_size = mmc->write_bl_len;
	start_block = to >> blksz_shift;
	block_num = (size + block_size - 1) >> blksz_shift;

	cnt = mmc->block_dev.block_write(mmc->block_dev.dev,
			start_block,
			block_num, (void *)buf);
	if (cnt != block_num) {
#ifdef SDMMC_DEBUG
		printf("mmc blk write error!\n");
#endif
		return -1;
	}

	return size;
}

static void setup_bb_cfg_sdmmc(void)
{
#ifdef CONFIG_SDMMC_BOOT
    unsigned long temp;

    /* pinmux for sdmmc0 */
    temp = readl(0x11a09008);
    temp &= ~(0x3F<<9);
    writel(temp, 0x11a09008);
#endif
}

#define RDA_MMC_MAX_BLOCKS    (1024)
int rda_mmc_init(void)
{
	int error,i;
	struct mmc *dev;

#ifdef SDMMC_DEBUG
	printf("MMC: rda_mmc_init\n");
#endif
	setup_bb_cfg_sdmmc();

	for(i = 0; i < MMC_DEV_NUM; i++){
		dev = alloc_mmc_struct(i);
		if (!dev)
			return -1;

		error = rda_mmc_host_init(dev);
		if (error) {
			printf("rda_mmc_host_init %d, error = %d\n",
				dev->block_dev.dev, error);
			return -1;
		}

		dev->b_max = RDA_MMC_MAX_BLOCKS;
		mmc_register(dev);
		printf("MMC: registered mmc interface %d\n",
			dev->block_dev.dev);
	}

	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int err = -1;

	err = rda_mmc_init();
	if (err)
		return err;

#if defined(CONFIG_PARTITIONS) && !defined(CONFIG_SPL_BUILD)
	if (rda_media_get() == MEDIA_MMC) {
		err = mmc_parts_init();
		if (err)
			return err;
	}
#endif
	return err;
}

