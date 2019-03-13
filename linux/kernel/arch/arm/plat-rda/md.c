/*
 * md.c - A driver for controlling modem of RDA
 *
 * Copyright (C) 2013 RDA Microelectronics Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/sysrq.h>
#include <linux/tty_flip.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <asm/ioctls.h>
#include <plat/devices.h>

#include <plat/reg_md.h>
#include <plat/rda_md.h>
#include <plat/ipc.h>

//#define line()  printk("[%s %s %d]\n", __FILE__, __func__, __LINE__)
#define line()

#ifndef CONFIG_RDA_FPGA

static int md_initialized;
static struct md_dev *g_md;

#ifdef CONFIG_RDA_RMNET
static struct md_ps_ctrl *ps;
extern u32 *dl_free_buf;
#endif

#define MD_SYS_HDR_SIZE     sizeof(struct md_sys_hdr)

#ifdef CONFIG_RDA_RMNET
#define ALL_RX_MASK     (AT_READY_BIT | SYS_READY_BIT | TRACE_READY_BIT | PS_READY_BIT)
#define ALL_TX_MASK     (AT_EMPTY_BIT | SYS_EMPTY_BIT | TRACE_EMPTY_BIT | PS_EMPTY_BIT)
#else
#define ALL_RX_MASK     (AT_READY_BIT | SYS_READY_BIT | TRACE_READY_BIT )
#define ALL_TX_MASK     (AT_EMPTY_BIT | SYS_EMPTY_BIT | TRACE_EMPTY_BIT )
#endif

#define MD_GET_STATUS(_dev_)     __raw_readl((_dev_)->ctrl_base + RDA_MD_CAUSE_REG)
#define MD_SET_MASK(_dev_, _val_)       __raw_writel((_val_), (_dev_)->ctrl_base + RDA_MD_MASK_SET_REG)
#define MD_CLR_MASK(_dev_, _val_)       __raw_writel((_val_), (_dev_)->ctrl_base + RDA_MD_MASK_CLR_REG)
#define MD_SET_IT(_dev_, _val_)     __raw_writel((_val_), (_dev_)->ctrl_base + RDA_MD_IT_SET_REG)
#define MD_CLR_IT(_dev_, _val_)     __raw_writel((_val_), (_dev_)->ctrl_base + RDA_MD_IT_CLR_REG)
#define MD_GET_IT_CLR(_dev_)    __raw_readl((_dev_)->ctrl_base + RDA_MD_IT_CLR_REG)


/* The it_set register of AP is mapped to it_clr register of BP. */
#define BP_GET_SET_VAL(_dev_)	    __raw_readl((_dev_)->bp_base + RDA_MD_IT_SET_REG)

struct rda_klog_addr {
	unsigned int kbuf_addr;
	unsigned int kbuf_size;

	unsigned int res0;
	unsigned int res1;
};

struct rda_mlog_addr {
	unsigned int mlog_addr;
	unsigned int mlog_size;
	unsigned int mexc_addr;
	unsigned int mexc_size;
};

static struct rda_mlog_addr *gmd_log = NULL;
static struct md_mbx_magic *gmd_mbx_magic = NULL;

static unsigned int gmd_rx;
static unsigned int gmd_tx;

#ifndef CONFIG_RDA_FPGA
static void inline __enable_port(struct md_port *pmd_port)
{
	struct md_dev *pmd = pmd_port->pmd;

    line();
	switch (pmd_port->port_id) {
	case MD_PORT_AT:
    line();
		MD_SET_MASK(pmd, AT_READY_BIT);
		break;

	case MD_PORT_SYS:
    line();
		MD_SET_MASK(pmd, SYS_READY_BIT);
		break;

	case MD_PORT_TRACE:
    line();
		MD_SET_MASK(pmd, TRACE_READY_BIT);
		break;

#ifdef CONFIG_RDA_RMNET
	case MD_PORT_PS:
    line();
		MD_SET_MASK(pmd, PS_READY_BIT);
		break;
#endif

	default:
		break;
	}
}
#else
static void inline __enable_port(struct md_port *pmd_port)
{
    line();
	return;
}
#endif /* CONFIG_RDA_FPGA */

#ifndef CONFIG_RDA_FPGA
static void inline __disable_port(struct md_port *pmd_port)
{
	struct md_dev *pmd = pmd_port->pmd;

    line();
	switch (pmd_port->port_id) {
	case MD_PORT_AT:
		MD_CLR_MASK(pmd, AT_READY_BIT | AT_EMPTY_BIT);
		break;

	case MD_PORT_SYS:
		MD_CLR_MASK(pmd, SYS_READY_BIT | SYS_EMPTY_BIT);
		break;

	case MD_PORT_TRACE:
		MD_CLR_MASK(pmd, TRACE_READY_BIT | TRACE_EMPTY_BIT);
		break;

#ifdef CONFIG_RDA_RMNET
	case MD_PORT_PS:
		MD_CLR_MASK(pmd, PS_READY_BIT | PS_EMPTY_BIT);
		break;
#endif

	default:
		break;
	}
}
#else
static void inline __disable_port(struct md_port *pmd_port)
{
    line();
	return;
}
#endif /* CONFIG_RDA_FPGA */

#ifdef CONFIG_RDA_RMNET
void disable_rmnet_irq(void)
{
    line();
	__disable_port(&g_md->md_ports[MD_PORT_PS]);
}

void enable_rmnet_irq(void)
{
    line();
	__enable_port(&g_md->md_ports[MD_PORT_PS]);
}

u32 ps_dl_read_avail(void)
{
    line();
	return (ps->dl_windex - ps->dl_rindex) & RDA_DL_MASK;
}

u32 ps_dl_read(void)
{
	u32 offset;

    line();
	offset = dl_free_buf[ps->dl_rindex];
	ps->dl_rindex++;
	ps->dl_rindex &= RDA_DL_MASK;

	return offset;
}

void ps_ul_write(void)
{
    line();
	ps->ul_windex++;
	ps->ul_windex &= RDA_UL_MASK;

	/* Enable tx */
	MD_SET_IT(g_md, PS_READY_BIT);
}

u32 ps_ul_idle(void)
{
    line();
	return !(ps->ul_busy);
}
#endif


void disable_md_irq(struct md_port *pmd_port)
{
    line();
	__disable_port(pmd_port);
}

void enable_md_irq(struct md_port *pmd_port)
{
    line();
	__enable_port(pmd_port);
}

void clr_md_irq(struct md_port *pmd_port, int clr_bit)
{
	struct md_dev *pmd = pmd_port->pmd;

    line();
        MD_CLR_IT(pmd, clr_bit);
}

/* provide a pointer and length to readable data in the fifo */
static unsigned __ch_read_buffer(struct md_ch *ch, void **ptr)
{
	unsigned int head = ch->pctrl->head;
	unsigned int tail = ch->pctrl->tail;

    line();
	*ptr = (void *) (ch->pfifo + tail);

	if (tail <= head) {
		return head - tail;
	}

	return ch->fifo_size - tail;
}

/* how many bytes are available for reading */
static int __ch_stream_read_avail(struct md_ch *ch)
{
	int space;
	unsigned long flags;

    line();
	spin_lock_irqsave(&ch->lock, flags);
	space = (ch->pctrl->head - ch->pctrl->tail) & ch->fifo_mask;
	spin_unlock_irqrestore(&ch->lock, flags);

	return space;
}

static int __ch_packet_read_avail(struct md_ch *ch)
{
    line();
	return __ch_stream_read_avail(ch);
}

/* advance the fifo read pointer after data from ch_read_buffer is consumed */
static void __ch_read_done(struct md_ch *ch, unsigned int count)
{
	unsigned long flags;
#if 0
	BUG_ON(count > __ch_stream_read_avail(ch));
#endif /* #if 0 */

    line();
	spin_lock_irqsave(&ch->lock, flags);
	ch->pctrl->tail = (ch->pctrl->tail + count) & ch->fifo_mask;
	spin_unlock_irqrestore(&ch->lock, flags);
}

void rda_md_read_done(struct md_ch *ch, unsigned int count)
{
    line();
	__ch_read_done(ch,count);
}

/**
 *	__ch_read	- read data from channel.
 *	@ch: modem port of sys
 *	@data: a pointer points to a buffer for header
 *     @len: the size of buffer
 *
 *	Return the size that is read.
 */
static int __ch_read(struct md_ch *ch, void *data, int len)
{
	void *ptr;
	unsigned n;
	unsigned char *pbuf = data;
	int orig_len = len;

    line();
	while (len > 0) {
		n = __ch_read_buffer(ch, &ptr);
		if (n == 0) {
			break;
		}

		if (n > len) {
			n = len;
		}

		memcpy(pbuf, ptr, n);

		pbuf += n;
		len -= n;
		__ch_read_done(ch, n);
	}

	return orig_len - len;
}

static int __ch_read_hdr(struct md_ch *ch, void *data, int len)
{
	void *ptr;
	unsigned n;
	unsigned char *pbuf = data;
	int orig_len = len;
	unsigned int head = ch->pctrl->head;
	unsigned int tail = ch->pctrl->tail;

    line();
	while (len > 0) {
		ptr = (void *) (ch->pfifo + tail);

		if (tail <= head) {
			n = head - tail;
		} else {
			n = ch->fifo_size - tail;
		}

		if (n == 0) {
			/* Fifo is empty. */
			break;
		}

		if (n > len) {
			/* There are more than a packet. */
			n = len;
		}

		memcpy(pbuf, ptr, n);

		pbuf += n;
		len -= n;
		tail = (tail + n) & ch->fifo_mask;
	}

	return orig_len - len;
}

static int __ch_stream_read(struct md_ch *ch, void *data, int len)
{
	int r;

    line();
	if (len < 0 || !data) {
		return -EINVAL;
	}

	r = __ch_read(ch, data, len);

	return r;
}

static int __ch_packet_read(struct md_ch *ch, void *data, int len)
{
	void *pbuf = data;
	struct md_sys_hdr fhdr;
	struct md_sys_hdr *phdr;
	int size = 0;
	int opt_size;
	int r;

    line();
	if (!len && !data) {
		/* Just return a packet's length */
		r = __ch_read_hdr(ch, (void *)&fhdr, sizeof(fhdr));
		if (r < MD_SYS_HDR_SIZE) {
			/* It might that we would read later data, but BP didn't trigger interrupt. */
			return -EAGAIN;
		}

		/* Check if this is a vaild frame. */
		if (fhdr.magic != MD_SYS_MAGIC) {
			return -EINVAL;
		}

		return (r + ALIGN(fhdr.ext_len, 4));
	} else if (len < 0 || !data) {
		/* Sanity checking */
		return -EINVAL;
	}

	/* Sanity checking */
	if (len < MD_SYS_HDR_SIZE) {
		return -EINVAL;
	}

	r = __ch_read(ch, pbuf, sizeof(struct md_sys_hdr));
	if (!r) {
		return r;
	}

	phdr = (struct md_sys_hdr *)pbuf;
	if (phdr->magic != MD_SYS_MAGIC) {
		return -EINVAL;
	}

	size += r;
	opt_size = ALIGN(phdr->ext_len, 4);

	if (opt_size > 0) {
		/* Sanity check if the space is enough to hold data. */
		if (len - size < opt_size) {
			return -EINVAL;
		}

		pbuf += r;
		r = __ch_read(ch, pbuf, opt_size);
		size += r;
	}

	return size;
}

/* how many bytes we are free to write */
static int __ch_stream_write_avail(struct md_ch *ch)
{
	int space;
	unsigned long flags;

    line();
	spin_lock_irqsave(&ch->lock, flags);
	space = ch->fifo_mask - ((ch->pctrl->head - ch->pctrl->tail) & ch->fifo_mask);
	spin_unlock_irqrestore(&ch->lock, flags);

	return space;
}

static int __ch_packet_write_avail(struct md_ch *ch)
{
	int space = __ch_stream_write_avail(ch);

    line();
	return (space > MD_SYS_HDR_SIZE ? (space - MD_SYS_HDR_SIZE) : 0);
}

/* provide a pointer and length to next free space in the fifo */
static int __ch_write_buffer(struct md_ch *ch, void **ptr)
{
	unsigned int head = ch->pctrl->head;
	unsigned int tail = ch->pctrl->tail;

    line();
	*ptr = (void *) (ch->pfifo + head);

	if (head < tail) {
		return tail - head - 1;
	}

	if (tail == 0) {
		return ch->fifo_size - head - 1;
	}

	return ch->fifo_size - head;
}

/* advace the fifo write pointer after freespace
 * from ch_write_buffer is filled
 */
static void __ch_write_done(struct md_ch *ch, unsigned int count)
{
	unsigned long flags;
#if 0
	BUG_ON(count > __ch_stream_write_avail(ch));
#endif /* #if 0 */

    line();
	spin_lock_irqsave(&ch->lock, flags);
	ch->pctrl->head = (ch->pctrl->head + count) & ch->fifo_mask;
	spin_unlock_irqrestore(&ch->lock, flags);
}

static int __ch_stream_write(struct md_ch *ch, const void *data, int len)
{
	void *ptr;
	const unsigned char *buf = data;
	int xfer;
	int orig_len = len;

    line();
	if (len < 0) {
		return -EINVAL;
	}

	while ((xfer = __ch_write_buffer(ch, &ptr)) != 0) {
		if (xfer > len) {
			xfer = len;
		}

		memcpy(ptr, buf, xfer);
		__ch_write_done(ch, xfer);

		len -= xfer;
		buf += xfer;

		if (len == 0) {
			break;
		}
	}

	return orig_len - len;
}

static int __ch_packet_write(struct md_ch *ch, const void *data, int len)
{
    line();
	if (len < 0) {
		return -EINVAL;
	}

	if (__ch_packet_write_avail(ch) < (len - MD_SYS_HDR_SIZE)) {
		return -EBUSY;
	}

	__ch_stream_write(ch, data, len);

	return len;
}

static void do_nothing_notify(void *priv, unsigned int flags)
{
	/* Nothing to do. */
}

int rda_md_check_bp_status(struct md_port *md_port)
{
	struct md_dev *pmd = md_port->pmd;
	/*
	 * Polling comreg0_it_reg, not checking cause register
	 * which is set by interrupt.
	 * */
	volatile unsigned int status = MD_GET_IT_CLR(pmd);

    line();
	/*
	 * Check if BP_SHUTDOWN_BIT is set.
	 * If it is set, it means that BP is power-off.
	 */
	return (status & BP_SHUTDOWN_BIT ? 1 : 0);
}

void rda_md_notify_bp(struct md_port *md_port)
{
	struct md_dev *pmd = md_port->pmd;

    line();
	MD_SET_IT(pmd, (1<<4));
}

int rda_md_open(int index, struct md_port** port, void *priv,
		void (*notify)(void *, unsigned))
{
	struct md_port *md_port = NULL;

    line();
	if (md_initialized == 0) {
		return -ENODEV;
	}
    line();

	/* Sys port isn't used by application. */
	if (index >= MD_PORT_MAX) {
		pr_err("rda_md: Invalid parameters\n");
		return -ENODEV;
	}
    line();

	md_port = &g_md->md_ports[index];
	/* Prevent to open more times. */
	if (atomic_read(&md_port->opened) >= 1) {
    line();
		return -EPERM;
	} else {
    line();
		atomic_inc(&md_port->opened);
	}

    line();
	if (notify == NULL) {
    line();
		notify = do_nothing_notify;
	}
    line();

	md_port->notify = notify;
	md_port->priv = priv;

	*port = md_port;

	__enable_port(md_port);

	return 0;
}

int rda_md_close(struct md_port *md_port)
{
    line();
	if (md_port == NULL) {
		return -ENODEV;
	}

	/* Sys port isn't closed by application. */
	if (md_port->port_id == MD_PORT_SYS) {
		return -EPERM;
	}

	if (atomic_read(&md_port->opened) != 1) {
		return -EPERM;
	}

	atomic_dec(&md_port->opened);

	__disable_port(md_port);

	md_port->notify = do_nothing_notify;

	return 0;
}

int rda_md_read(struct md_port *md_port, void *data, int len)
{
	int rbytes = md_port->read(&md_port->rx, data, len);

    line();
	gmd_rx += rbytes;

	return rbytes;
}

int rda_md_write(struct md_port *md_port, const void *data, int len)
{
	struct md_dev *pmd = md_port->pmd;
	int wbytes;
	unsigned int mask;

    line();
	wbytes = md_port->write(&md_port->tx, data, len);
    line();

	if (wbytes > 0) {
    line();
		gmd_tx += wbytes;

		switch (md_port->port_id) {
		case MD_PORT_AT:
    line();
			mask = AT_READY_BIT;
			break;

		case MD_PORT_SYS:
    line();
			mask = SYS_READY_BIT;
			break;

		case MD_PORT_TRACE:
    line();
			mask = TRACE_READY_BIT;
			break;

		default:
			return -EINVAL;
		}

    line();
		/* Enable tx */
		MD_SET_IT(pmd, mask);
	}

    line();
	return wbytes;
}

int rda_md_read_avail(struct md_port *md_port)
{
    line();
	return md_port->read_avail(&md_port->rx);
}

int rda_md_write_avail(struct md_port *md_port)
{
    line();
	return md_port->write_avail(&md_port->tx);
}

unsigned int rda_md_query_ver(struct md_port *md_port)
{
	struct md_dev *pmd;

    line();
	if (!md_port) {
    line();
		return 0;
	}

    line();
	pmd = (struct md_dev *)md_port->pmd;
    line();

	return pmd->bb_version;
}

static void rda_mdcom_port_rx(struct md_port *port)
{
    line();
	if (port->notify) {
		port->notify(port->priv, MD_EVENT_DATA);
	}
}

void rda_md_kick(struct md_port *md_port)
{
	unsigned long flags;

    line();
	spin_lock_irqsave(&md_port->notify_lock, flags);
	rda_mdcom_port_rx(md_port);
	spin_unlock_irqrestore(&md_port->notify_lock, flags);
}

static void __iomem *md_pll_reg;
void rda_md_plls_shutdown(int plls)
{
	int value = 0;

    line();
	if (plls & AP_CPU_PLL)
		value |= AP_CPU_PLL;
	if (plls & AP_BUS_PLL)
		value |= AP_BUS_PLL;
	if (plls & AP_DDR_PLL)
		value |= AP_DDR_PLL;
	if (plls & AP_DDR_PLL_VPUBUG_SET)
		value |= AP_DDR_PLL_VPUBUG_SET;
	if (plls & AP_DDR_PLL_VPUBUG_RESTORE)
		value |= AP_DDR_PLL_VPUBUG_RESTORE;

	writel(value, md_pll_reg);
}
int rda_md_plls_read(void)
{
    line();
	return readl(md_pll_reg);
}

#ifdef RDA_MD_WQ
static void rda_md_at_work(struct work_struct *work)
{
	struct md_dev *pmd = container_of(work, struct md_dev, at_work);
	struct md_port *pmd_at = &pmd->md_ports[MD_PORT_AT];
	unsigned long flags;

    line();
	spin_lock_irqsave(&pmd_at->notify_lock, flags);
	rda_mdcom_port_rx(pmd_at);
	spin_unlock_irqrestore(&pmd_at->notify_lock, flags);

	return;
}

static void rda_md_trace_work(struct work_struct *work)
{
	struct md_dev *pmd = container_of(work, struct md_dev, trace_work);
	struct md_port *pmd_trace = &pmd->md_ports[MD_PORT_TRACE];
	unsigned long flags;

    line();
	spin_lock_irqsave(&pmd_trace->notify_lock, flags);
	rda_mdcom_port_rx(pmd_trace);
	spin_unlock_irqrestore(&pmd_trace->notify_lock, flags);

	return;
}
#else
static void rda_md_tasklet_at(unsigned long data)
{
	struct md_dev *pmd = (struct md_dev *)data;
	struct md_port *pmd_at = &pmd->md_ports[MD_PORT_AT];
	unsigned long flags;

    line();
	spin_lock_irqsave(&pmd_at->notify_lock, flags);
	rda_mdcom_port_rx(pmd_at);
	spin_unlock_irqrestore(&pmd_at->notify_lock, flags);

	return;
}

static void rda_md_tasklet_trace(unsigned long data)
{
	struct md_dev *pmd = (struct md_dev *)data;
	struct md_port *pmd_trace = &pmd->md_ports[MD_PORT_TRACE];
	unsigned long flags;

    line();
	spin_lock_irqsave(&pmd_trace->notify_lock, flags);
	rda_mdcom_port_rx(pmd_trace);
	spin_unlock_irqrestore(&pmd_trace->notify_lock, flags);

	return;
}
#endif /* RDA_MD_WQ */

static irqreturn_t rda_md_interrupt(int irq, void *md_dev)
{
	struct md_dev *pmd = (struct md_dev *)md_dev;
	struct md_port *port = NULL;
	volatile unsigned int irq_status = MD_GET_STATUS(pmd);
	unsigned long flags;
	volatile unsigned int value;

    line();
#if 0
	pr_info("status = 0x%08x\n", irq_status);
#endif /* #if 0 */

	/* Disable current task be preempted. */
	spin_lock_irqsave(&pmd->irq_lock, flags);
    line();

	if (irq_status & SYS_READY_BIT) {
    line();
		port = &pmd->md_ports[MD_PORT_SYS];
		if (port->notify) {
    line();
			port->notify(port->priv, MD_EVENT_DATA);
		}

    line();
		MD_CLR_IT(pmd, SYS_READY_BIT);
		/* Read itself for waiting for completion of clear. */
		MD_GET_IT_CLR(pmd);
    line();
		value = BP_GET_SET_VAL(pmd);
    line();

		if (value & SYS_READY_BIT) {
    line();
			pr_warn("rda_md: may be a sync error on sys channel!\n");
		}
	}
    line();

	if (irq_status & AT_READY_BIT) {
#ifdef RDA_MD_WQ
    line();
		queue_work(pmd->at_wq, &pmd->at_work);
    line();
#else
		tasklet_schedule(&pmd->at_tasklet);
#endif /* RDA_MD_WQ */

    line();
		MD_CLR_IT(pmd, AT_READY_BIT);
		/* Read itself for waiting for completion of clear. */
    line();
		MD_GET_IT_CLR(pmd);

    line();
		value = BP_GET_SET_VAL(pmd);
    line();
		if (value & AT_READY_BIT) {
    line();
			pr_warn("rda_md: may be a sync error on at channel!\n");
		}
	}

#ifdef CONFIG_RDA_RMNET
	if (irq_status & PS_READY_BIT) {
    line();
		//pr_info("\nPS_READY_BIT\n");
		port = &pmd->md_ports[MD_PORT_PS];
    line();
		if (port->notify) {
    line();
			port->notify(port->priv, MD_EVENT_DATA);
		}

    line();
		MD_CLR_IT(pmd, PS_READY_BIT);
		/* Read itself for waiting for completion of clear. */
    line();
		MD_GET_IT_CLR(pmd);
    line();
		value = BP_GET_SET_VAL(pmd);

    line();
		if (value & PS_READY_BIT) {
			pr_warn("rda_md: may be a sync error on ps channel!\n");
		}
	}
#endif

    line();
	if (irq_status & TRACE_READY_BIT) {
#ifdef RDA_MD_WQ
    line();
		queue_work(pmd->trace_wq, &pmd->trace_work);
#else
    line();
		tasklet_schedule(&pmd->trace_tasklet);
#endif /* RDA_MD_WQ */

    line();
		MD_CLR_IT(pmd, TRACE_READY_BIT);
		/* Read itself for waiting for completion of clear. */
    line();
		MD_GET_IT_CLR(pmd);

		value = BP_GET_SET_VAL(pmd);
		if (value & TRACE_READY_BIT) {
			pr_warn("rda_md: may be a sync error on trace channel!\n");
		}
	}

    line();
	spin_unlock_irqrestore(&pmd->irq_lock, flags);
    line();

	return IRQ_HANDLED;
}

static int inline rda_md_init_prev(struct md_dev *pmd)
{
    line();
	/* Disable all interrupts */
	MD_CLR_MASK(pmd, IRQ1_MASK_CLR_ALL);

	spin_lock_init(&pmd->irq_lock);

	return 0;
}

void *rda_md_get_mapped_addr(void)
{
    line();
	return (void *)(g_md->mem_base);
}

static void rda_md_init_post(struct md_dev *pmd)
{
	struct rda_ap_hb_hdr *pheart = (struct rda_ap_hb_hdr *)pmd->mem_base;

    line();
	pmd->bb_version = pheart->version;
	pr_info("rda_md: rda mdcom interface ver = 0x%08x\n", pmd->bb_version);
	printk("rda_md: rda mdcom interface ver = 0x%08x\n", pmd->bb_version);

	return;
}

static int inline rda_md_init_port(struct md_port *pmd_port, struct platform_device *pdev)
{
	struct md_dev *pmd = (struct md_dev *)pmd_port->pmd;

    line();
	switch (pmd_port->port_id) {
	case MD_PORT_AT:
		pmd_port->type = MD_PORT_STREAM_TYPE;

		pmd_port->rx.pctrl = (struct md_ch_ctrl *)(pmd->mem_base + MD_AT_READ_CTRL_OFFSET);
		pmd_port->rx.pfifo = (void *)(pmd->mem_base + MD_AT_READ_BUF_OFFSET);
		pmd_port->rx.fifo_size = MD_AT_READ_BUF_SIZE;
		pmd_port->rx.fifo_mask = MD_AT_READ_BUF_SIZE - 1;

		pmd_port->tx.pctrl = (struct md_ch_ctrl *)(pmd->mem_base + MD_AT_WRITE_CTRL_OFFSET);
		pmd_port->tx.pfifo = (void *)(pmd->mem_base + MD_AT_WRITE_BUF_OFFSET);
		pmd_port->tx.fifo_size = MD_AT_WRITE_BUF_SIZE;
		pmd_port->tx.fifo_mask = MD_AT_WRITE_BUF_SIZE - 1;
		break;

	case MD_PORT_SYS:
		pmd_port->type = MD_PORT_PACKET_TYPE;

		pmd_port->rx.pctrl = (struct md_ch_ctrl *)(pmd->mem_base + MD_SYS_READ_CTRL_OFFSET);
		pmd_port->rx.pfifo = (void *)(pmd->mem_base + MD_SYS_READ_BUF_OFFSET);
		pmd_port->rx.fifo_size = MD_SYS_READ_BUF_SIZE;
		pmd_port->rx.fifo_mask = MD_SYS_READ_BUF_SIZE - 1;

		pmd_port->tx.pctrl = (struct md_ch_ctrl *)(pmd->mem_base + MD_SYS_WRITE_CTRL_OFFSET);
		pmd_port->tx.pfifo = (void *)(pmd->mem_base + MD_SYS_WRITE_BUF_OFFSET);
		pmd_port->tx.fifo_size = MD_SYS_WRITE_BUF_SIZE;
		pmd_port->tx.fifo_mask = MD_SYS_WRITE_BUF_SIZE - 1;
		break;

	case MD_PORT_TRACE:
		pmd_port->type = MD_PORT_STREAM_TYPE;

		pmd_port->rx.pctrl = (struct md_ch_ctrl *)(pmd->mem_base + MD_TRACE_READ_CTRL_OFFSET);
		pmd_port->rx.pfifo = (void *)(pmd->mem_base + MD_TRACE_READ_BUF_OFFSET);
		pmd_port->rx.fifo_size = MD_TRACE_READ_BUF_SIZE;
		pmd_port->rx.fifo_mask = MD_TRACE_READ_BUF_SIZE - 1;

		pmd_port->tx.pctrl = (struct md_ch_ctrl *)(pmd->mem_base + MD_TRACE_WRITE_CTRL_OFFSET);
		pmd_port->tx.pfifo = (void *)(pmd->mem_base + MD_TRACE_WRITE_BUF_OFFSET);
		pmd_port->tx.fifo_size = MD_TRACE_WRITE_BUF_SIZE;
		pmd_port->tx.fifo_mask = MD_TRACE_WRITE_BUF_SIZE - 1;
		break;

#ifdef CONFIG_RDA_RMNET
	case MD_PORT_PS:
		ps = (struct md_ps_ctrl *)(pmd->mem_base + MD_PS_CTRL_OFFSET);
		memset(pmd->mem_base + MD_PS_CTRL_OFFSET, 0, sizeof(struct md_ps_ctrl));
		break;
#endif

	default:
		dev_err(&pdev->dev, "Invalid index of port\n");
		return -EINVAL;
	}

	spin_lock_init(&pmd_port->rx.lock);
	spin_lock_init(&pmd_port->tx.lock);
	spin_lock_init(&pmd_port->notify_lock);

	pmd_port->rx.port = (void *)pmd_port;
	pmd_port->tx.port = (void *)pmd_port;

	if (pmd_port->type == MD_PORT_STREAM_TYPE) {
		pmd_port->read = __ch_stream_read;
		pmd_port->write = __ch_stream_write;
		pmd_port->read_avail = __ch_stream_read_avail;
		pmd_port->write_avail = __ch_stream_write_avail;
	} else if (pmd_port->type == MD_PORT_PACKET_TYPE) {
		pmd_port->read = __ch_packet_read;
		pmd_port->write = __ch_packet_write;
		pmd_port->read_avail = __ch_packet_read_avail;
		pmd_port->write_avail = __ch_packet_write_avail;
	} else {
		/* Nothing to do */
	}

	return 0;
}

static ssize_t rda_md_tx_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    line();
	return sprintf(buf, "%d\n", gmd_tx);
}

static ssize_t rda_md_rx_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    line();
	return sprintf(buf, "%d\n", gmd_rx);
}

static struct device_attribute rda_md_attrs[] = {
	__ATTR(md_tx, S_IRUGO, rda_md_tx_show, NULL),
	__ATTR(md_rx, S_IRUGO, rda_md_rx_show, NULL),
};

extern void rda_get_log_buf_info(unsigned int *addr_ptr, unsigned int *len_ptr);
extern void rda_logger_get_info(unsigned int *addr_ptr, unsigned int *len_ptr, int index);

static int rda_md_probe(struct platform_device *pdev)
{
	struct md_port *md_port;
	struct resource *data_mem;
	struct md_dev *pmd = NULL;
	struct rda_klog_addr *pklog = NULL;
	int i;
	int ret;

    line();
	pmd = kzalloc(sizeof(struct md_dev), GFP_KERNEL);
	if (!pmd) {
		dev_err(&pdev->dev, "malloc memory fail\n");
		return -ENOMEM;
	}

    line();
	/* Get resources from our definitions. */
	data_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!data_mem) {
		dev_err(&pdev->dev, "no data mem resource\n");
		ret = -EINVAL;
		goto err_get_resource;
	}

    line();
	pmd->irq = platform_get_irq(pdev, 0);
	if (pmd->irq < 0) {
		dev_err(&pdev->dev, "no irq resource\n");
		ret = pmd->irq;
		goto err_get_resource;
	}

    line();
	/* Using static io mapping instead of dynamic allocated. */
	pmd->ctrl_base = (void *)RDA_COMREGS_BASE;
	/* I/O remap */
	pmd->mem_base = ioremap(data_mem->start, resource_size(data_mem));
	if (!pmd->mem_base) {
		dev_err(&pdev->dev, "remap data mem fail!\n");
		ret = -ENOMEM;
		goto err_io_remap;
	}

    line();
	md_pll_reg = pmd->mem_base + MD_PLL_REG_OFFS;
	pmd->bp_base = ioremap(RDA_MODEM_COM_PHYS, RDA_MODEM_COM_SIZE);
	if (!pmd->bp_base) {
		dev_err(&pdev->dev, "remap bp mem fail!\n");
		ret = -ENOMEM;
		goto err_io_remap;
	}

    line();
	for (i = 0; i < MD_PORT_MAX; i++) {
		md_port = &pmd->md_ports[i];
		md_port->port_id = i;
		/* Save a pointer points to its parent. */
		md_port->pmd = (void *)pmd;

		rda_md_init_port(md_port, pdev);
	}
    line();

	rda_md_init_prev(pmd);

#ifdef RDA_MD_WQ
	INIT_WORK(&pmd->at_work, rda_md_at_work);
	INIT_WORK(&pmd->trace_work, rda_md_trace_work);

	pmd->at_wq = create_singlethread_workqueue("at-wq");
	pmd->trace_wq = create_singlethread_workqueue("trace-wq");
#else
	tasklet_init(&pmd->at_tasklet, rda_md_tasklet_at, (unsigned long)pmd);
	tasklet_init(&pmd->trace_tasklet, rda_md_tasklet_trace, (unsigned long)pmd);
#endif /* RDA_MD_WQ */

    line();
	for (i = 0; i < ARRAY_SIZE(rda_md_attrs); i++) {
    line();
		ret = device_create_file(&pdev->dev, &rda_md_attrs[i]);
	}

	ret = request_irq(pmd->irq, rda_md_interrupt,
		IRQF_NO_SUSPEND | IRQF_ONESHOT, "rda-md", (void *)pmd);
    line();
	if (ret < 0) {
    line();
		dev_err(&pdev->dev, "request irq fail\n");
		goto err_request_irq;
	}

    line();
	pmd->pdata = (void *)pdev;
	/* Set flag of initialization. */
	md_initialized = 1;
	g_md = pmd;

    line();
	rda_md_init_post(pmd);
    line();

	gmd_log = (struct rda_mlog_addr *)(pmd->mem_base + MD_MLOG_ADDR_OFFSET);
	gmd_mbx_magic = (struct md_mbx_magic *)(pmd->mem_base + MD_MBX_MAGIC_OFFSET);
    line();

	/*
	 * Acquire buffer address and length of logs,
	 * e.g printk of kernel, main, events, radio, and system of android.
	 */
	pklog = (struct rda_klog_addr *)(pmd->mem_base + MD_KLOG_ADDR_OFFSET + MD_KLOG_PRINTK_OFFSET);
	rda_get_log_buf_info(&pklog->kbuf_addr, &pklog->kbuf_size);
    line();

	pklog = (struct rda_klog_addr *)(pmd->mem_base + MD_KLOG_ADDR_OFFSET + MD_KLOG_MAIN_OFFSET);
	//rda_logger_get_info(&pklog->kbuf_addr, &pklog->kbuf_size, 0);
    line();

	pklog = (struct rda_klog_addr *)(pmd->mem_base + MD_KLOG_ADDR_OFFSET + MD_KLOG_EVENTS_OFFSET);
	//rda_logger_get_info(&pklog->kbuf_addr, &pklog->kbuf_size, 1);

	pklog = (struct rda_klog_addr *)(pmd->mem_base + MD_KLOG_ADDR_OFFSET + MD_KLOG_RADIO_OFFSET);
	//rda_logger_get_info(&pklog->kbuf_addr, &pklog->kbuf_size, 2);

	pklog = (struct rda_klog_addr *)(pmd->mem_base + MD_KLOG_ADDR_OFFSET + MD_KLOG_SYSTEM_OFFSET);
	//rda_logger_get_info(&pklog->kbuf_addr, &pklog->kbuf_size, 3);
    line();

	pr_info("rda_md: mdcom initialized\n");

    line();
	return 0;

err_request_irq:
err_io_remap:

    line();
	if (pmd->bp_base) {
    line();
		iounmap(pmd->bp_base);
	}

    line();
	if (pmd->mem_base) {
    line();
		iounmap(pmd->mem_base);
	}

    line();
	if (pmd->ctrl_base) {
		pmd->ctrl_base = NULL;
	}

err_get_resource:

    line();
	if (pmd) {
		kfree(pmd);
	}

	return ret;
}

static struct platform_driver rda_md_driver = {
	.probe = rda_md_probe,
	.driver = {
		.name = RDA_MD_DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init rda_md_init(void)
{
    line();
	return platform_driver_register(&rda_md_driver);
}
arch_initcall(rda_md_init);

void rda_md_get_exception_info(u32 *addr, u32 *len)
{
    line();
	if (gmd_log && MD_ADDRESS_VALID(gmd_log->mexc_addr)) {
		*addr = MD_ADDRESS_TO_AP(gmd_log->mexc_addr);
		*len = gmd_log->mexc_size;
	} else {
		*addr = 0;
		*len = 0;
	}
}
EXPORT_SYMBOL(rda_md_get_exception_info);

void rda_md_get_modem_magic_info(struct md_mbx_magic *magic)
{
    line();
	if (gmd_mbx_magic) {
		*magic = *gmd_mbx_magic;
	}
	else {
		memset(magic, 0, sizeof(*magic));
	}
}
EXPORT_SYMBOL(rda_md_get_modem_magic_info);

void rda_md_set_modem_crash_magic(unsigned int magic)
{
    line();
	if (gmd_mbx_magic) {
		gmd_mbx_magic->modem_crashed = magic;
	}
}
EXPORT_SYMBOL(rda_md_set_modem_crash_magic);

MODULE_AUTHOR("Jason Tao<leitao@rdamicro.com>");
MODULE_DESCRIPTION("RDA virtual tty driver");
MODULE_LICENSE("GPL");

#else /* defined CONFIG_RDA_FPGA  */

int rda_md_read_avail(struct md_port *md_port)
{
    line();
	return 0;
}

int rda_md_write_avail(struct md_port *md_port)
{
    line();
	return 0;
}

int rda_md_read(struct md_port *md_port, void *data, int len)
{
    line();
	return 0;
}

int rda_md_write(struct md_port *md_port, const void *data, int len)
{
    line();
	return 0;
}

int rda_md_open(int index, struct md_port** port, void *priv,
		void (*notify)(void *, unsigned))
{
    line();
	return 0;
}
int rda_md_plls_read(void)
{
    line();
	return 0;
}

#endif /* CONFIG_RDA_FPGA */
