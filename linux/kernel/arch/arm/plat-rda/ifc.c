#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <plat/reg_ifc.h>
#include <mach/ifc.h>
#include <plat/cpu.h>
#include <plat/pm_ddr.h>

static HAL_IFC_REQUEST_ID_T ifc_channel_owner[SYS_IFC_STD_CHAN_NB];
static spinlock_t ifc_lock;
static HWP_SYS_IFC_T* hwp_sysIfc = NULL;

static void ifc_channel_sg_release(HAL_IFC_CFG_T *ifc)
{
	unsigned long flags;
	int i;
	int max_index;
	enum ddr_master master_idx;

	BUG_ON(ifc->channel >= SYS_IFC_STD_CHAN_NB);
	BUG_ON(ifc_channel_owner[ifc->channel] != ifc->request);
	BUG_ON(ifc->sg_len < 1 || ifc->sg_len > SYS_IFC_SG_MAX);

	spin_lock_irqsave(&ifc_lock, flags);

	master_idx = PM_DDR_IFC0 + ifc->channel;
	pm_ddr_put(master_idx);
	max_index = ifc->sg_len - 1;
	/* disable this channel */
	hwp_sysIfc->std_ch[ifc->channel].control = (SYS_IFC_REQ_SRC(ifc->request)
						| SYS_IFC_CH_RD_HW_EXCH
						| SYS_IFC_DISABLE);

	for (i = 0; i < ifc->sg_len; i++) {
		hwp_sysIfc->std_ch[ifc->channel].sg_table[max_index - i].tc = 0;
	}
	ifc_channel_owner[ifc->channel] = HAL_IFC_NO_REQWEST;

	spin_unlock_irqrestore(&ifc_lock, flags);
}

static u32 ifc_channel_sg_get_tc(HAL_IFC_CFG_T *ifc)
{
	unsigned long flags;
	u32 tc = 0;
	int i;
	int max_index;
	struct scatterlist *s;

	BUG_ON(ifc->channel >= SYS_IFC_STD_CHAN_NB);
	BUG_ON(ifc_channel_owner[ifc->channel] != ifc->request);
	BUG_ON(ifc->sg_len < 1 || ifc->sg_len > SYS_IFC_SG_MAX);

	spin_lock_irqsave(&ifc_lock, flags);

	max_index = ifc->sg_len - 1;
	for_each_sg(ifc->sg, s, ifc->sg_len, i) {
		tc += hwp_sysIfc->std_ch[ifc->channel].sg_table[max_index - i].tc;
	}
	spin_unlock_irqrestore(&ifc_lock, flags);

	return tc;
}

u8 ifc_transfer_sg_start(HAL_IFC_CFG_T *ifc,
	u32 xfer_size,
	HAL_IFC_MODE_T ifc_mode)
{
	unsigned long flags;
	u8 channel;
	int i;
	int max_index;
	struct scatterlist *s;
	enum ddr_master master_idx;

	/* Sanity checking */
	BUG_ON(!hwp_sysIfc);
	/* Size is less than 32MB. */
	BUG_ON(xfer_size > 0x1FFFFFF);
	BUG_ON(ifc->sg_len < 1 || ifc->sg_len > SYS_IFC_SG_MAX);

	spin_lock_irqsave(&ifc_lock, flags);

	max_index = ifc->sg_len - 1;
	/* alloc channel by hardware */
	channel = SYS_IFC_CH_TO_USE(hwp_sysIfc->get_ch) ;
	if (channel >= SYS_IFC_STD_CHAN_NB) {
		spin_unlock_irqrestore(&ifc_lock, flags);
		return HAL_UNKNOWN_CHANNEL;
	}

	if (ifc->sg_len > 1 && channel >= SYS_IFC_SG_CHAN_NUM) {
		spin_unlock_irqrestore(&ifc_lock, flags);
		pr_err("Invalid sg channel(%d), max = %d\n", channel, SYS_IFC_SG_CHAN_NUM);
		return HAL_UNKNOWN_CHANNEL;
	}

	master_idx = PM_DDR_IFC0 + channel;
	pm_ddr_get(master_idx);
	/* take the channel */
	ifc_channel_owner[channel] = ifc->request;
	for_each_sg(ifc->sg, s, ifc->sg_len, i) {
		hwp_sysIfc->std_ch[channel].sg_table[max_index - i].start_addr = sg_phys(s);
		hwp_sysIfc->std_ch[channel].sg_table[max_index - i].tc = s->length;
	}
	hwp_sysIfc->std_ch[channel].control = (SYS_IFC_REQ_SRC(ifc->request)
			| SYS_IFC_SG_NUM(max_index)
			| ifc_mode
			| SYS_IFC_CH_RD_HW_EXCH
			| SYS_IFC_ENABLE);

	spin_unlock_irqrestore(&ifc_lock, flags);

	return channel;
}

u32 ifc_transfer_sg_get_tc(HAL_IFC_CFG_T *ifc)
{
	BUG_ON(!hwp_sysIfc);

	return ifc_channel_sg_get_tc(ifc);
}

void ifc_transfer_sg_stop(HAL_IFC_CFG_T *ifc)
{
	BUG_ON(!hwp_sysIfc);

	ifc_channel_sg_release(ifc);
}

static void ifc_channel_release(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
	unsigned long flags;

	BUG_ON(channel >= SYS_IFC_STD_CHAN_NB);
	BUG_ON(ifc_channel_owner[channel] != request_id);

	spin_lock_irqsave(&ifc_lock, flags);
	/* disable this channel */
	hwp_sysIfc->std_ch[channel].control = (SYS_IFC_REQ_SRC(request_id)
						| SYS_IFC_CH_RD_HW_EXCH
						| SYS_IFC_DISABLE);
	hwp_sysIfc->std_ch[channel].sg_table[0].tc = 0;
	ifc_channel_owner[channel] = HAL_IFC_NO_REQWEST;
	spin_unlock_irqrestore(&ifc_lock, flags);
}

static void ifc_channel_flush(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
	unsigned long flags;

	BUG_ON(channel >= SYS_IFC_STD_CHAN_NB);
	BUG_ON(ifc_channel_owner[channel] != request_id);

	spin_lock_irqsave(&ifc_lock, flags);
	/* If fifo not empty, flush it. */
	if (!(hwp_sysIfc->std_ch[channel].status & SYS_IFC_FIFO_EMPTY))
		hwp_sysIfc->std_ch[channel].control |= SYS_IFC_FLUSH;
	spin_unlock_irqrestore(&ifc_lock, flags);
}

static int ifc_channel_is_fifo_empty(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
	unsigned long flags;
	int fifo_empty;

	BUG_ON(channel >= SYS_IFC_STD_CHAN_NB);
	BUG_ON(ifc_channel_owner[channel] != request_id);

	spin_lock_irqsave(&ifc_lock, flags);
	fifo_empty = (hwp_sysIfc->std_ch[channel].status & SYS_IFC_FIFO_EMPTY);
	spin_unlock_irqrestore(&ifc_lock, flags);

	return fifo_empty;
}

/**
 *	ifc_channel_get_tc -get counter of transfer of the specifical channel.
 *	@request_id: 	the source of request.
 *	@channel: 	the number of channel.
 *
 *	Note: this function is only for the channel which does not support scatter/gather feature.
 *
 */
static int ifc_channel_get_tc(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
	unsigned long flags;
	u32 tc;

	BUG_ON(channel >= SYS_IFC_STD_CHAN_NB);
	BUG_ON(ifc_channel_owner[channel] != request_id);

	spin_lock_irqsave(&ifc_lock, flags);
	tc = hwp_sysIfc->std_ch[channel].sg_table[0].tc;
	spin_unlock_irqrestore(&ifc_lock, flags);

	return tc;
}

u8 ifc_transfer_start(HAL_IFC_REQUEST_ID_T request_id, u8* mem_addr,
	u32 xfer_size, HAL_IFC_MODE_T ifc_mode)
{
	unsigned long flags;
	u8 channel;
	enum ddr_master master_idx;

	BUG_ON(!hwp_sysIfc);
	BUG_ON(!xfer_size);
	BUG_ON(xfer_size > 0x7FFFFF);

	if (rda_soc_is_older_metal10()) {
		/* To see if the address is aligned with 16Bytes and cross boundary. */
		if (!IS_ALIGNED((u32)mem_addr, 16) &&
			(((u32)mem_addr >> PAGE_SHIFT) !=
			(((u32)mem_addr + xfer_size - 1) >> PAGE_SHIFT))) {
			pr_err("IFC cross page boundary: mem_addr = %p, size = %d\n",
				mem_addr, xfer_size);
			BUG_ON(1);
		}
	}

	spin_lock_irqsave(&ifc_lock, flags);
	/* alloc channel by hardware */
	channel = SYS_IFC_CH_TO_USE(hwp_sysIfc->get_ch) ;
	if (channel >= SYS_IFC_STD_CHAN_NB) {
		spin_unlock_irqrestore(&ifc_lock, flags);
		return HAL_UNKNOWN_CHANNEL;
	}

	master_idx = PM_DDR_IFC0 + channel;
	pm_ddr_get(master_idx);
	/* take the channel */
	ifc_channel_owner[channel] = request_id;
	hwp_sysIfc->std_ch[channel].sg_table[0].start_addr = (u32)mem_addr;
	hwp_sysIfc->std_ch[channel].sg_table[0].tc = xfer_size;
	hwp_sysIfc->std_ch[channel].control = (SYS_IFC_REQ_SRC(request_id)
						   | ifc_mode
						   | SYS_IFC_CH_RD_HW_EXCH
						   | SYS_IFC_ENABLE);
	spin_unlock_irqrestore(&ifc_lock, flags);
	return channel;
}

u32 ifc_transfer_get_tc(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
	BUG_ON(!hwp_sysIfc);
	return ifc_channel_get_tc(request_id, channel);
}

void ifc_transfer_flush(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
#define IFC_FLUSH_TIMEOUT_MS 1000
	unsigned long timeout;

	BUG_ON(!hwp_sysIfc);

	ifc_channel_flush(request_id, channel);

	timeout = jiffies + msecs_to_jiffies(IFC_FLUSH_TIMEOUT_MS);
	while(!ifc_channel_is_fifo_empty(request_id, channel)
				&& time_before(jiffies, timeout));

	/* timeout? */
	if(!ifc_channel_is_fifo_empty(request_id, channel))
		BUG_ON(1);
}

void ifc_transfer_stop(HAL_IFC_REQUEST_ID_T request_id, u8 channel)
{
	enum ddr_master master_idx;
	BUG_ON(!hwp_sysIfc);

	master_idx = PM_DDR_IFC0 + channel;
	pm_ddr_put(master_idx);
	ifc_channel_release(request_id, channel);
}

static int __init ifc_init(void)
{
	u8 channel;

	spin_lock_init(&ifc_lock);

	/* Initialize the channel table with unknown requests. */
	for (channel = 0; channel < SYS_IFC_STD_CHAN_NB; channel++) {
		ifc_channel_owner[channel] = HAL_IFC_NO_REQWEST;
	}

	hwp_sysIfc = ((HWP_SYS_IFC_T*)IO_ADDRESS(RDA_IFC_BASE));
	return 0;
}

subsys_initcall(ifc_init);
