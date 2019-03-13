
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

#ifndef _WLAND_BUS_H_
#define _WLAND_BUS_H_

#define BUS_WAKE(bus) \
	do { \
		(bus)->idlecount = 0; \
	} while (0)

#define WAKE_TX_WORK(bus) \
	do { \
	    atomic_inc(&(bus)->tx_dpc_tskcnt); \
		queue_work((bus)->wland_txwq, &(bus)->TxWork); \
	} while (0)

#define WAKE_RX_WORK(bus) \
	do { \
	    atomic_inc(&(bus)->rx_dpc_tskcnt); \
		queue_work((bus)->wland_rxwq, &(bus)->RxWork); \
	} while (0)

/* The level of bus communication with the chip */
enum wland_bus_state {
	WLAND_BUS_DOWN,		/* Not ready for frame transfers    */
	WLAND_BUS_LOAD,		/* Download access only (CPU reset)
				 * But this state not used in RDA WIFI chip
				 */
	WLAND_BUS_DATA		/* Ready for frame transfers        */
};

/*
 * struct wland_bus_ops - bus callback operations.
 *
 * @init    : prepare for communication with dongle.
 * @stop    : clear pending frames, disable data flow.
 * @txdata  : send a data frame to the dongle. When the data
 *	has been transferred, the common driver must be
 *	notified using wland_txcomplete(). The common
 *	driver calls this function with interrupts disabled.
 * @txctl   : transmit a control request message to dongle.
 * @rxctl   : receive a control response message from dongle.
 * @gettxq  : obtain a reference of bus transmit queue (optional).
 *
 * This structure provides an abstract interface towards the
 * bus specific driver. For control messages to common driver
 * will assure there is only one active transaction. Unless
 * indicated otherwise these callbacks are mandatory.
 */
struct wland_bus_ops {
	int (*init) (struct device * dev);
	void (*stop) (struct device * dev);
	int (*txdata) (struct device * dev, struct sk_buff * skb);
	int (*txctl) (struct device * dev, u8 * msg, uint len);
	int (*rxctl) (struct device * dev, u8 * msg, uint len);
	struct pktq *(*gettxq) (struct device * dev);
};

/*
 * struct wland_bus - interface structure between common and bus layer
 *
 * @bus_priv:   pointer to private bus device.
 * @dev:        device pointer of bus device.
 * @drvr:       public driver information.
 * @state:      operational state of the bus interface.
 * @dstats:     dongle-based statistical data.
 * @chip:       device identifier of the dongle chip.
 */
struct wland_bus {
	union {
		struct wland_sdio_dev *sdio;
		struct wland_usb_dev *usb;
	} bus_priv;
	struct device *dev;
	struct osl_info *osh;	/* OSL handle */
	struct wland_private *drvr;
	struct wland_bus_ops *ops;
	enum wland_bus_state state;
	u32 chip;
};

/* callback wrappers:txrx data to sdio or usb device interface */
static inline int wland_bus_init(struct wland_bus *bus)
{
	return bus->ops->init(bus->dev);
}

static inline void wland_bus_stop(struct wland_bus *bus)
{
	bus->ops->stop(bus->dev);
}

static inline int wland_bus_txdata(struct wland_bus *bus, struct sk_buff *skb)
{
	return bus->ops->txdata(bus->dev, skb);
}

static inline int wland_bus_txctl(struct wland_bus *bus, u8 * msg, uint len)
{
	return bus->ops->txctl(bus->dev, msg, len);
}

static inline int wland_bus_rxctl(struct wland_bus *bus, u8 * msg, uint len)
{
	return bus->ops->rxctl(bus->dev, msg, len);
}

static inline struct pktq *wland_bus_gettxq(struct wland_bus *bus)
{
	if (!bus->ops->gettxq)
		return ERR_PTR(-ENOENT);

	return bus->ops->gettxq(bus->dev);
}

/* Receive frame for delivery to OS.  Callee disposes of rxp. */
extern void wland_rx_frames(struct device *dev, struct sk_buff *skb);

/* Indication from bus module regarding presence/insertion of dongle. */
extern int wland_bus_attach(uint bus_hdrlen, struct device *dev);

/* Indication from bus module regarding removal/absence of dongle */
extern void wland_bus_detach(struct device *dev);

/* Indication from bus module to change flow-control state */
extern void wland_txflowcontrol(struct device *dev, bool state);

/* Notify the bus has transferred the tx packet to firmware */
extern void wland_txcomplete(struct device *dev, struct sk_buff *txp,
	bool success);

/* Start Bus */
extern int wland_bus_start(struct device *dev);

/* Active Bus */
extern int wland_bus_active(struct device *dev);

#endif /* _WLAND_BUS_H_ */
