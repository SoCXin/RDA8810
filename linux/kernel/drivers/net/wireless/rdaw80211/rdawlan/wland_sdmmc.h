
/*
 * Copyright (c) 2014 Rdamicro Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _WLAND_SDMMC_H_
#define _WLAND_SDMMC_H_

#ifdef WLAND_SDIO_SUPPORT

#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/ieee80211.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif /*CONFIG_HAS_WAKELOCK */

#define WLAND_SDIO_NAME	                "wlandfmac_sdio"

#define SDIO_FUNC_MAX_ERR	            10
#define SDIO_FUNC1_BLOCKSIZE		    512

#define SDIOH_READ                      0	/* Read request */
#define SDIOH_WRITE                     1	/* Write request */

/* SDIO Function1 Unique Registers */
#define URSDIO_FUNC1_SPKTLEN_LO         0x00	/* SDIO2AHB Packet Length Register (LSB 8 bits) */
#define URSDIO_FUNC1_SPKTLEN_HI         0x01	/* SDIO2AHB Packet Length Register (MSB 8 bits) */
#define URSDIO_FUNC1_RPKTLEN_LO         0x02	/* AHB2SDIO Packet Length Register (LSB 8 bits) */
#define URSDIO_FUNC1_RPKTLEN_HI         0x03	/* AHB2SDIO Packet Length Register (MSB 8 bits) */
#define URSDIO_FUNC1_REGISTER_MASK      0x04	/* Function1 Mask Register */
#define URSDIO_FUNC1_INT_PENDING        0x05	/* Function1 Interrupt Pending Register */
#define URSDIO_FUNC1_INT_STATUS         0x06	/* Function1 Interrupt Status  Register */
#define URSDIO_FUNC1_FIFO_WR            0x07	/* WR FIFO */
#define URSDIO_FUNC1_FIFO_RD            0x08	/* RD FIFO */
#define URSDIO_FUNC1_INT_TO_DEVICE      0x09	/* Function1 Interrupt to Device */

#define   URSDIO_FUNC1_INT_AHB2SDIO  0x01
#define   URSDIO_FUNC1_INT_ERROR     0x04
#define   URSDIO_FUNC1_INT_SLEEP     0x10
#define   URSDIO_FUNC1_INT_AWAKE     0x20
#define   URSDIO_FUNC1_INT_RXCMPL    0x40
#define   URSDIO_FUNC1_HOST_TX_FLAG  0x80

/* intstatus Mask */
#define I_AHB2SDIO                      BIT0	/* Indicates that data transfer from AHB to SD is
						 * pending Cleared by Host by writing a „1. into
						 * this register location */
#define I_ERROR                         BIT2	/* Indicates that a system error has occurred in
						 * the device and needs to be handled. */
#define I_SLEEP                         BIT4
#define I_AWAKE                         BIT5
#define I_RXCMPL                        BIT6
#define I_HOST_TXFLAG                   BIT7

/* Packet alignment for most efficient SDIO (can change based on platform) */
#define WLAND_SDALIGN	                (1 << 2)

/* watchdog polling interval in ms */
#define WLAND_WD_POLL_MS	            20

#define TXQLEN		                    2048	/* bulk tx queue length */
#define TXHI		                    (TXQLEN - 256)	/* turn on flow control above TXHI */
#define TXLOW		                    (TXHI - 256)	/* turn off flow control below TXLOW */
#define PRIOMASK	                    7
#define TXRETRIES	                    1	/* # of retries for tx frames */
#define WLAND_RXBOUND	                50	/* Default for max rx frames in one scheduling */
#define WLAND_TXBOUND	                20	/* Default for max tx frames in one scheduling */
#define WLAND_TXMINMAX	                1	/* Max tx frames if rx still pending */

#define WLAND_IDLE_INTERVAL	            5

/* clkstate */
#define CLK_NONE	                    0
#define CLK_SDONLY	                    1
#define CLK_PENDING	                    2
#define CLK_AVAIL	                    3

/* Private data for SDIO bus interaction */
struct wland_sdio {
	struct wland_sdio_dev *sdiodev;	/* sdio device handler */
	atomic_t intstatus;	/* Intstatus bits (events) pending */
	uint blocksize;		/* Block size of SDIO transfers */

	struct pktq txq;	/* Queue length used for flow-control */
	struct pktq rxq;
	u8 flowcontrol;		/* per prio flow control bitmask */

	bool rxpending;		/* Data frame pending in dongle */
	uint rxbound;		/* Rx frames to read before resched */
	uint txbound;		/* Tx frames to send before resched */
	uint txminmax;

	u8 *rxbuf;		/* Buffer for receiving control packets */
	uint rxblen;		/* Allocated length of rxbuf */
	u8 *rxctl;		/* Aligned pointer into rxbuf */

	uint rxlen;		/* Length of valid data in buffer */
	spinlock_t rxctl_lock;
	struct semaphore txclk_sem;

	bool intr;		/* Use interrupts */
	bool poll;		/* Use polling */
	bool intdis;		/* Interrupts disabled by isr */

	uint pollrate;		/* Ticks between device polls */
	uint polltick;		/* Tick counter */

	uint clkstate;		/* State of sd and backplane clock(s) */
	bool activity;		/* Activity flag for clock down */
	s32 idletime;		/* Control for activity timeout */
	s32 idlecount;		/* Activity timeout counter */

	u8 *ctrl_frame_buf;
	uint ctrl_frame_len;
	bool ctrl_frame_stat;
	bool ctrl_frame_send_success;

	spinlock_t txqlock;
	spinlock_t rxqlock;
	wait_queue_head_t ctrl_wait;
	wait_queue_head_t dcmd_resp_wait;

	struct timer_list timer;
	bool wd_timer_valid;
	uint save_ms;

	struct completion watchdog_wait;
	struct task_struct *watchdog_tsk;

	bool threads_only;
	struct semaphore sdsem;

	struct workqueue_struct *wland_txwq;
	struct work_struct TxWork;
	struct workqueue_struct *wland_rxwq;
	struct work_struct RxWork;

	/*
	 * common part for workqueue and thread
	 */
	atomic_t tx_dpc_tskcnt;	/* flag if need to schdule */
	atomic_t rx_dpc_tskcnt;
	atomic_t ipend;		/* Device interrupt is pending */

	struct work_struct work_hang;

	/*
	 * Wakelocks
	 */
#if defined(CONFIG_HAS_WAKELOCK)
	struct wake_lock wl_wifi;	/* Wifi wakelock */
	struct wake_lock wl_rxwake;	/* Wifi rx wakelock */
	struct wake_lock wl_ctrlwake;	/* Wifi ctrl wakelock */
	struct wake_lock wl_wdwake;	/* Wifi wd wakelock */
#endif				/*(CONFIG_HAS_WAKELOCK)*/

	/*
	 * net_device interface lock, prevent race conditions among net_dev interface
	 * * calls and wifi_on or wifi_off
	 */
	struct mutex dhd_net_if_mutex;
	struct mutex dhd_suspend_mutex;
	spinlock_t wakelock_spinlock;
	s32 wakelock_counter;
	s32 wakelock_wd_counter;
	s32 wakelock_rx_timeout_enable;
	s32 wakelock_ctrl_timeout_enable;

	bool txoff;		/* Transmit flow-controlled */
	struct wland_sdio_count sdcnt;
	u8 tx_hdrlen;		/* sdio bus header length for tx packet */

	bool hang_was_sent;
	int rxcnt_timeout;	/* counter rxcnt timeout to send HANG */
	int txcnt_timeout;	/* counter txcnt timeout to send HANG */

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif				/* CONFIG_HAS_EARLYSUSPEND  && defined(DHD_USE_EARLYSUSPEND) */

#ifdef ARP_OFFLOAD_SUPPORT
	u32 pend_ipaddr;
#endif				/* ARP_OFFLOAD_SUPPORT */

};

struct wland_sdio_dev {
	struct sdio_func *func;
	struct device *dev;
	struct wland_bus *bus_if;
	struct wland_platform_data *pdata;
	struct wland_sdio *bus;
	atomic_t suspend;	/* suspend flag */
	bool card_sleep;

	/*
	 * wait queue list
	 */
	wait_queue_head_t request_byte_wait;
	wait_queue_head_t request_word_wait;
	wait_queue_head_t request_buffer_wait;
};

static inline uint wland_get_align_size(struct wland_sdio *bus, uint count)
{
#ifdef WLAND_RDAPLATFORM_SUPPORT
	uint block = 1, block_size = 512, base_len = count;

	if (count <= block_size) {
		if (count < 3) {
			count = 4;
		} else if (count & (count - 1)) {
			do {
				block <<= 1;
			} while ((count >>= 1));
			count = block;
		}
	} else if (count <= (3 * block_size)) {
		if (count % block_size) {
			block = (count / block_size + 1) * block_size;
			count = block;
		}
	} else {
		count = (3 * block_size);
		WLAND_ERR("count of SDIO read/write is overflow!\n");
	}
	WLAND_DBG(SDIO, TRACE, "2^nByteAllignSize:%d, 4ByteAlignSize:%d\n",
		count, base_len);
#endif /*WLAND_RDAPLATFORM_SUPPORT */
	return count;
}

/* Register/deregister interrupt handler. */
extern int wland_sdio_intr_register(struct wland_sdio_dev *sdiodev);
extern int wland_sdio_intr_unregister(struct wland_sdio_dev *sdiodev);

extern void wland_sdio_wd_timer(struct wland_sdio *bus, uint wdtick);
extern int wland_sdio_clkctl(struct wland_sdio *bus, uint target);
extern void wland_pm_resume_wait(struct wland_sdio_dev *sdiodev,
	wait_queue_head_t * wq);
extern bool wland_pm_resume_error(struct wland_sdio_dev *sdiodev);

/* attach, return handler on success, NULL if failed. */
extern int wland_sdioh_attach(struct wland_sdio_dev *sdiodev);
extern void wland_sdioh_detach(struct wland_sdio_dev *sdiodev);

/* read or write one byte using cmd52  */
extern int sdioh_request_byte(struct wland_sdio_dev *sdiodev, uint rw,
	uint addr, u8 * byte);

/* read or write 2/4 bytes using cmd52 */
extern int sdioh_request_word(struct wland_sdio_dev *sdiodev, uint rw,
	uint addr, u32 * word, uint nbyte);

/* read or write bytes using cmd53     */
extern int sdioh_request_bytes(struct wland_sdio_dev *sdiodev, uint rw,
	uint addr, u8 * byte, uint nbyte);

/* read or write skb buffer */
extern int wland_sdio_recv_pkt(struct wland_sdio *bus, struct sk_buff *skbbuf,
	uint size);
extern int wland_sdio_send_pkt(struct wland_sdio *bus, struct sk_buff *skbbuf,
	uint count);

/* wland sdio probe or realse */
extern void *wland_sdio_probe(struct osl_info *osh,
	struct wland_sdio_dev *sdiodev);
extern void wland_sdio_release(struct wland_sdio *bus);

/* sdio interface */
extern void wland_sdio_exit(void);
extern void wland_sdio_register(void);

/* linux osl */
extern void dhd_os_sdlock(struct wland_sdio *bus);
extern void dhd_os_sdunlock(struct wland_sdio *bus);
extern void dhd_os_sdlock_txq(struct wland_sdio *bus, unsigned long *flags);
extern void dhd_os_sdunlock_txq(struct wland_sdio *bus, unsigned long *flags);
extern void dhd_os_sdlock_rxq(struct wland_sdio *bus, unsigned long *flags);
extern void dhd_os_sdunlock_rxq(struct wland_sdio *bus, unsigned long *flags);
extern int dhd_os_ioctl_resp_wait(struct wland_sdio *bus, uint * condition,
	bool * pending);
extern void dhd_os_ioctl_resp_wake(struct wland_sdio *bus);
extern void dhd_os_wait_for_event(struct wland_sdio *bus, bool * lockvar);
extern void dhd_os_wait_event_wakeup(struct wland_sdio *bus);

extern int dhd_os_wake_lock(struct wland_sdio *bus);
extern int dhd_os_wake_lock_timeout(struct wland_sdio *bus);
extern int dhd_os_wake_unlock(struct wland_sdio *bus);
extern int dhd_os_check_wakelock(struct wland_sdio *bus);
extern int dhd_os_wd_wake_lock(struct wland_sdio *bus);
extern int dhd_os_wd_wake_unlock(struct wland_sdio *bus);

extern ulong dhd_os_spin_lock(struct wland_sdio *bus);
extern void dhd_os_spin_unlock(struct wland_sdio *bus, ulong flags);
extern void rda_wland_shutdown(struct device *dev);
#endif /* WLAND_SDIO_SUPPORT */

#endif /*_WLAND_SDMMC_H_*/
