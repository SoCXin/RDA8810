/*
 * rda_md.h - A header file of modem of RDA
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

/*
 *   dpram memery layout

========================================== --MD_TRACE_READ_CTRL_OFFSET(0x10)
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_SYS_READ_BUF_OFFSET
        #  MD_SYS_READ_BUF_SIZE(0x200)   #
  SYS   # ------------------------------ # --MD_SYS_WRITE_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_SYS_WRITE_BUF_OFFSET
        #  MD_SYS_WRITE_BUF_SIZE(0x200)  #
========================================== --MD_AT_READ_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_AT_READ_BUF_OFFSET
        #  MD_AT_READ_BUF_SIZE(0x800)    #
  AT    # ------------------------------ # --MD_AT_WRITE_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_AT_WRITE_BUF_OFFSET
        #  MD_AT_WRITE_BUF_SIZE(0x800)   #
========================================== --MD_TRACE_READ_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_TRACE_READ_BUF_OFFSET
        #  MD_TRACE_READ_BUF_SIZE(0x400) #
 TRACE  # ------------------------------ # --MD_TRACE_WRITE_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_TRACE_WRITE_BUF_OFFSET
        #  MD_TRACE_WRITE_BUF_SIZE(0x200)#
========================================== --MD_MISC_OFFSET/MD_MBX_MAGIC_OFFSET
        #  MD_MBX_MAGIC_SIZE(0x10)       #
 MISC   # ------------------------------ # --MD_MLOG_ADDR_OFFSET
        #  MD_MLOG_ADDR_SIZE(0x10)       #
        # ------------------------------ # --MD_KLOG_ADDR_OFFSET
        #  SIZE(0x70)                    #
========================================== --MD_PS_READ_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # -------------------------------# --MD_PS_READ_BUF_OFFSET
        #  MD_PS_READ_BUF_SIZE(0x80)     #
 PS     # ------------------------------ # --MD_PS_WRITE_CTRL_OFFSET
        #  CHAN_CTRL_SIZE(0x10)          #
        # ------------------------------ # --MD_PS_WRITE_BUF_OFFSET
        #  MD_PS_WRITE_BUF_SIZE(0x80)    #
========================================== --
        #      reserve                   #
==========================================
*
*/


#ifndef __RDA_MD_H__
#define __RDA_MD_H__

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>

#if 0
#define RDA_MD_WQ
#endif /* #if 0 */

#define MD_EVENT_DATA 1
#define MD_EVENT_OPEN 2
#define MD_EVENT_CLOSE 3

#define MD_PORT_STREAM_TYPE     1
#define MD_PORT_PACKET_TYPE     2

/* AT command */
#define MD_PORT_AT  0
/* SYSTEM command */
#define MD_PORT_SYS     1
/* Trace communication */
#define MD_PORT_TRACE   2

#ifdef CONFIG_RDA_RMNET
/* PS CtrlMsg communication */
#define MD_PORT_PS		3
#define MD_PORT_MAX     (MD_PORT_PS + 1)
#else
#define MD_PORT_MAX     (MD_PORT_TRACE + 1)
#endif
#define MD_MAX_DEVS      MD_PORT_MAX

#define MD_CHAN_CTRL_SIZE	     0x10

#define MD_SYS_READ_BUF_SIZE     0x200
#define MD_SYS_WRITE_BUF_SIZE    0x200
#define MD_AT_READ_BUF_SIZE      0x800
#define MD_AT_WRITE_BUF_SIZE     0x800
#define MD_TRACE_READ_BUF_SIZE   0x400
#define MD_TRACE_WRITE_BUF_SIZE  0x200
#define MD_MISC_SIZE             0x90
#define MD_MBX_MAGIC_SIZE        0x10
#define MD_MLOG_ADDR_SIZE        0x10
#define MD_PS_READ_BUF_SIZE      0x80
#define MD_PS_WRITE_BUF_SIZE     0x80

#define MD_SYS_READ_CTRL_OFFSET    0x0010
#define MD_SYS_READ_BUF_OFFSET     (MD_SYS_READ_CTRL_OFFSET+MD_CHAN_CTRL_SIZE)
#define MD_SYS_WRITE_CTRL_OFFSET   (MD_SYS_READ_BUF_OFFSET+MD_SYS_READ_BUF_SIZE)
#define MD_SYS_WRITE_BUF_OFFSET    (MD_SYS_WRITE_CTRL_OFFSET+MD_CHAN_CTRL_SIZE)

#define MD_AT_READ_CTRL_OFFSET     (MD_SYS_WRITE_BUF_OFFSET+MD_SYS_WRITE_BUF_SIZE)
#define MD_AT_READ_BUF_OFFSET      (MD_AT_READ_CTRL_OFFSET+MD_CHAN_CTRL_SIZE)
#define MD_AT_WRITE_CTRL_OFFSET    (MD_AT_READ_BUF_OFFSET+MD_AT_READ_BUF_SIZE)
#define MD_AT_WRITE_BUF_OFFSET     (MD_AT_WRITE_CTRL_OFFSET+MD_CHAN_CTRL_SIZE)

#define MD_TRACE_READ_CTRL_OFFSET  (MD_AT_WRITE_BUF_OFFSET+MD_AT_WRITE_BUF_SIZE)
#define MD_TRACE_READ_BUF_OFFSET   (MD_TRACE_READ_CTRL_OFFSET+MD_CHAN_CTRL_SIZE)
#define MD_TRACE_WRITE_CTRL_OFFSET (MD_TRACE_READ_BUF_OFFSET+MD_TRACE_READ_BUF_SIZE)
#define MD_TRACE_WRITE_BUF_OFFSET  (MD_TRACE_WRITE_CTRL_OFFSET+MD_CHAN_CTRL_SIZE)

#define MD_MISC_OFFSET          (MD_TRACE_WRITE_BUF_OFFSET+MD_TRACE_WRITE_BUF_SIZE)
#define MD_MBX_MAGIC_OFFSET     MD_MISC_OFFSET
#define MD_MLOG_ADDR_OFFSET     (MD_MBX_MAGIC_OFFSET+MD_MBX_MAGIC_SIZE)
#define MD_KLOG_ADDR_OFFSET		(MD_MLOG_ADDR_OFFSET+MD_MLOG_ADDR_SIZE)

#define MD_PS_CTRL_OFFSET	(MD_MISC_OFFSET+MD_MISC_SIZE)

#define MD_KLOG_PRINTK_OFFSET	0x0
#define MD_KLOG_MAIN_OFFSET		0x10
#define MD_KLOG_EVENTS_OFFSET	0x20
#define MD_KLOG_RADIO_OFFSET	0x30
#define MD_KLOG_SYSTEM_OFFSET	0x40

/*
 * MAGIC NUMBER
 */
#define MD_MAGIC_SYSTEM_STARTED_FLAG	0x057a67ed
#define MD_MAGIC_MODEM_CRASH_FLAG	0x9db09db0
#define MD_MAGIC_FACT_UPD_CMD_FLAG	0xfac40c3d
#define MD_MAGIC_FACT_UPD_TYPE_FLAG	0x496efa00
#define MD_MAGIC_FACT_UPD_TYPE_FLAG_MASK	0xFFFFFF00
#define MD_MAGIC_FACT_UPD_TYPE_CALIB	0x1
#define MD_MAGIC_FACT_UPD_TYPE_FACT	0x2
#define MD_MAGIC_FACT_UPD_TYPE_AP_FACT	0x4


#define MD_SYS_MAGIC    0xA8B1
#define MD_SYS_PACKET_MAX   144

#define MD_ADDRESS_TO_AP(x)	(((x) & 0x0FFFFFFF) | 0x10000000)
#define MD_ADDRESS_VALID(x)	(((x) >= 0x81C00000 && (x) < 0x81C18000) || \
		((x) >= 0xA1C00000 && (x) < 0xA1C18000) || \
		((x) >= 0x82000000 && (x) < 0x84000000) || \
		((x) >= 0xA2000000 && (x) < 0xA4000000))

#define AP_CPU_PLL	(1 << 0)
#define AP_BUS_PLL	(1 << 1)
#define AP_DDR_PLL	(1 << 2)
#define AP_DDR_PLL_VPUBUG_SET (1<<4)
#define AP_DDR_PLL_VPUBUG_RESTORE (1<<5)
#define	MD_PLL_REG_OFFS 	(0x228)

#define AP_COMM_HEARTBEAT_MIN_VALUE          0x80000000
/* Structure of heart beat */
struct rda_ap_hb_hdr {
	/* Modem heartbeat counter */
	volatile unsigned int bp_cnt;	/* 0x00000000 */
	/* AP heartbeat counter */
	volatile unsigned int ap_cnt; /* 0x00000004 */

	unsigned int reset_cause;
	unsigned int version;
};

struct md_sys_hdr {
	unsigned short magic;
	unsigned short mod_id;
	unsigned int req_id;
	unsigned int ret_val;
	unsigned short msg_id;
	unsigned short ext_len;
};

struct md_sys_hdr_ext {
	unsigned short magic;
	unsigned short mod_id;
	unsigned int req_id;
	unsigned int ret_val;
	unsigned short msg_id;
	unsigned short ext_len;
	unsigned char ext_data[];
};

struct md_ch_ctrl {
	/* Offset of read pointer */
	unsigned int tail;
	/* Offset of write pointer */
	unsigned int head;
	/* Reserved0 */
	unsigned int res0;
	/* Reserved1 */
	unsigned int res1;
} __attribute__(( aligned(4), packed ));

struct md_ch {
	/* The address of 16-byte buffer head */
	volatile struct md_ch_ctrl *pctrl;
	/* The start address of modem to AP buffer */
	void *pfifo;
	/* The size of modem to AP buffer */
	unsigned int fifo_size;
	unsigned int fifo_mask;

	spinlock_t lock;

	void *port;
};

struct md_mbx_magic {
	unsigned int sys_started;
	unsigned int modem_crashed;
	unsigned int fact_update_cmd;
	unsigned int fact_update_type;
};

struct md_port {
	unsigned int port_id;
	unsigned int type;
	atomic_t opened;

	/* Modem to AP channel */
	struct md_ch rx;
	/* AP to Modem channel */
	struct md_ch tx;

	spinlock_t notify_lock;
	void (*notify)(void *priv, unsigned int flags);

	int (*read)(struct md_ch *ch, void *data, int len);
	int (*write)(struct md_ch *ch, const void *data, int len);
	int (*read_avail)(struct md_ch *ch);
	int (*write_avail)(struct md_ch *ch);

	void (*update_state)(struct md_ch *ch);
	unsigned int last_state;
	void (*notify_other_cpu)(void);

	void *priv;
	void *pmd;
};

struct md_dev {
	struct md_port md_ports[MD_PORT_MAX];

	void __iomem *ctrl_base;
	void __iomem *mem_base;
	/* Version of baseband */
	unsigned int bb_version;

	spinlock_t irq_lock;
	int irq;

	unsigned int rx_status;
	unsigned int tx_statux;

#ifdef RDA_MD_WQ
	struct workqueue_struct *at_wq;
	struct work_struct at_work;

	struct workqueue_struct *trace_wq;
	struct work_struct trace_work;
#else
	/* For AT port */
	struct tasklet_struct at_tasklet;
	/* For Trace port */
	struct tasklet_struct trace_tasklet;
#endif /* RDA_MD_WQ */

	void __iomem *bp_base;

	void *pdata;
};

#ifdef CONFIG_RDA_RMNET
struct md_ps_ctrl {
	volatile u32 dl_rindex;
	volatile u32 dl_windex;
	volatile u32 ul_rindex;
	volatile u32 ul_windex;
	volatile u32 ul_busy; /* true: modem ul busy, false: modem ul idle */
} __attribute__(( aligned(4), packed ));
#endif

int rda_md_open(int index, struct md_port** port, void *priv, void (*notify)(void *, unsigned));

int rda_md_close(struct md_port *md_port);

int rda_md_read(struct md_port *md_port, void *data, int len);

int rda_md_write(struct md_port *md_port, const void *data, int len);

int rda_md_read_avail(struct md_port *md_port);

int rda_md_write_avail(struct md_port *md_port);

unsigned int rda_md_query_ver(struct md_port *md_port);

void rda_md_notify_bp(struct md_port *md_port);

void rda_md_kick(struct md_port *md_port);

void rda_md_get_exception_info(u32 *addr, u32 *len);

void rda_md_get_modem_magic_info(struct md_mbx_magic *magic);

void rda_md_set_modem_crash_magic(unsigned int magic);

void rda_md_plls_shutdown(int plls);

int rda_md_plls_read(void);

int rda_md_check_bp_status(struct md_port *md_port);

#ifdef CONFIG_RDA_RMNET
void disable_rmnet_irq(void);
void enable_rmnet_irq(void);
void ps_ul_write(void);
u32 ps_dl_read_avail(void);
u32 ps_dl_read(void);
u32 ps_ul_idle(void);
#endif

void *rda_md_get_mapped_addr(void);
void disable_md_irq(struct md_port *pmd_port);
void enable_md_irq(struct md_port *pmd_port);
void rda_md_read_done(struct md_ch *ch, unsigned int count);
void clr_md_irq(struct md_port *pmd_port, int clr_bit);
#endif /* __RDA_MDCOM_H__ */

