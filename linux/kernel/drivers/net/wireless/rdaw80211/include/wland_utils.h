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

#ifndef	_WLAND_UTILS_H_
#define	_WLAND_UTILS_H_

#include <linux/skbuff.h>
#include <802.11.h>

#ifndef ABS
#define	ABS(a)			            (((a) < 0) ? -(a) : (a))
#endif /* ABS */

#ifndef MIN
#define	MIN(a, b)		            (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define	MAX(a, b)		            (((a) > (b)) ? (a) : (b))
#endif /* MAX */

/*
 * Spin at most 'us' microseconds while 'exp' is true.
 * Caller should explicitly test 'exp' when this completes
 * and take appropriate error action if 'exp' is still true.
 */
#define SPINWAIT(exp, us) { \
	uint countdown = (us) + 9; \
	while ((exp) && (countdown >= 10)) {\
		udelay(10); \
		countdown -= 10; \
	} \
}

/* osl multi-precedence packet queue */
#define PKTQ_LEN_DEFAULT        128	/* Max 128 packets */
#define PKTQ_MAX_PREC           16	/* Maximum precedence levels */

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	            2048

#ifndef setbit
#ifndef NBBY			        /* the BSD family defines NBBY */
#define	NBBY	8		        /* 8 bits per byte */
#endif				            /* #ifndef NBBY */
#define	setbit(a, i)	        (((u8 *)a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a, i)	        (((u8 *)a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a, i)	            (((const u8 *)a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a, i)	            ((((const u8 *)a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)
#endif				            /* setbit */

#define	NBITS(type)	            (sizeof(type) * 8)
#define NBITVAL(nbits)	        (1 << (nbits))
#define MAXBITVAL(nbits)	    ((1 << (nbits)) - 1)
#define	NBITMASK(nbits)	        MAXBITVAL(nbits)
#define MAXNBVAL(nbyte)	        MAXBITVAL((nbyte) * 8)


/* callback function, taking one arg */
typedef void (*timer_cb_fn_t)(void *);


struct pktq_prec {
	struct sk_buff_head skblist;
	u16                 max;		/* maximum number of queued packets */
};

/* multi-priority pkt queue */
struct pktq {
	u16              num_prec;	/* number of precedences in use */
	u16              hi_prec;	/* rapid dequeue hint (>= highest non-empty prec) */
	u16              max;	    /* total max packets */
	u16              len;	    /* total number of packets */
	/* q array must be last since # of elements can be either PKTQ_MAX_PREC or 1 */
	struct pktq_prec q[PKTQ_MAX_PREC];
};

struct wland_drv_timer{
	struct timer_list   tl;
	timer_cb_fn_t       func;
	void               *data;
	u32                 time_period;
	bool                timer_is_periodic;
	bool                timer_is_canceled;
	u16                 event;
};

/* operations on a specific precedence in packet queue */
static inline int pktq_plen(struct pktq *pq, int prec)
{
	return pq->q[prec].skblist.qlen;
}

static inline int pktq_pavail(struct pktq *pq, int prec)
{
	return pq->q[prec].max - pq->q[prec].skblist.qlen;
}

static inline bool pktq_pfull(struct pktq *pq, int prec)
{
	return pq->q[prec].skblist.qlen >= pq->q[prec].max;
}

static inline bool pktq_pempty(struct pktq *pq, int prec)
{
	return skb_queue_empty(&pq->q[prec].skblist);
}

static inline struct sk_buff *pktq_ppeek(struct pktq *pq, int prec)
{
	return skb_peek(&pq->q[prec].skblist);
}

static inline struct sk_buff *pktq_ppeek_tail(struct pktq *pq, int prec)
{
	return skb_peek_tail(&pq->q[prec].skblist);
}

extern void wland_timer_handler(ulong fcontext);

static inline void wland_init_timer(struct wland_drv_timer *timer, timer_cb_fn_t func, void *data, u16 event)
{
	/* first, setup the timer to trigger the wland_timer_handler proxy */
	init_timer(&timer->tl);
	timer->tl.function       = wland_timer_handler;
	timer->tl.data           = (u32) timer;

	/* then tell the proxy which function to call and what to pass it */
	timer->func              = func;
	timer->data              = data;
	timer->event             = event;
	timer->timer_is_canceled = true;
	timer->timer_is_periodic = false;
}

static inline void wland_add_timer(struct wland_drv_timer *timer, u32 MillisecondPeriod, bool period)
{
	timer->time_period       = MillisecondPeriod;
	timer->timer_is_periodic = period;
	timer->timer_is_canceled = false;
	timer->tl.expires        = jiffies + (MillisecondPeriod * HZ) / 1000;
	
	add_timer(&timer->tl);	
}

static inline void wland_mod_timer(struct wland_drv_timer *timer, u32 MillisecondPeriod)
{
	timer->time_period       = MillisecondPeriod;
	timer->timer_is_periodic = false;
	timer->timer_is_canceled = false;
	
	mod_timer(&timer->tl, jiffies + (MillisecondPeriod * HZ) / 1000);
}

static inline void wland_del_timer(struct wland_drv_timer *timer)
{
	if(!timer->timer_is_canceled)
	{
		del_timer(&timer->tl);
		timer->timer_is_canceled = true;
	}
}

static inline void wland_sched_timeout(u32 millisec)
{ 
	ulong timeout = 0, expires = 0;
	expires = jiffies + msecs_to_jiffies(millisec);
	timeout = millisec;

	while(timeout)
	{
		timeout = schedule_timeout(timeout);
		
		if(time_after(jiffies, expires))
			break;
	}
}

static inline int pktq_avail(struct pktq *pq)
{
	return (int)(pq->max - pq->len);
}

static inline bool pktq_full(struct pktq *pq)
{
	return pq->len >= pq->max;
}

static inline bool pktq_empty(struct pktq *pq)
{
	return pq->len == 0;
}

/*
 * bitfield macros using masking and shift
 *
 * remark: the mask parameter should be a shifted mask.
 */
static inline void brcmu_maskset32(u32 *var, u32 mask, u8 shift, u32 value)
{
	value = (value << shift) & mask;
	*var  = (*var & ~mask) | value;
}

static inline u32 brcmu_maskget32(u32 var, u32 mask, u8 shift)
{
	return (var & mask) >> shift;
}

static inline void brcmu_maskset16(u16 *var, u16 mask, u8 shift, u16 value)
{
	value = (value << shift) & mask;
	*var = (*var & ~mask) | value;
}

static inline u16 brcmu_maskget16(u16 var, u16 mask, u8 shift)
{
	return (var & mask) >> shift;
}

#ifdef DEBUG
extern void wland_dbg_hex_dump(int level, const void *data, size_t size, const char *fmt, ...);
#else
static inline void wland_dbg_hex_dump(int level,const void *data, size_t size, const char *fmt, ...)
{
}
#endif

extern struct sk_buff *wland_pktq_penq(struct pktq *pq, int prec,struct sk_buff *p);
extern struct sk_buff *wland_pktq_penq_head(struct pktq *pq, int prec, struct sk_buff *p);
extern struct sk_buff *wland_pktq_pdeq(struct pktq *pq, int prec);
extern struct sk_buff *wland_pktq_pdeq_tail(struct pktq *pq, int prec);
extern struct sk_buff *wland_pktq_pdeq_match(struct pktq *pq, int prec, bool (*match_fn)(struct sk_buff *p, void *arg), void *arg);

/* packet primitives */
extern struct sk_buff *wland_pkt_buf_get_skb(uint len);
extern void wland_pkt_buf_free_skb(struct sk_buff *skb);

/* Empty the queue at particular precedence level */
/* callback function fn(pkt, arg) returns true if pkt belongs to if */
extern void wland_pktq_pflush(struct pktq *pq, int prec, bool dir, bool (*fn)(struct sk_buff *, void *), void *arg);

/* operations on a set of precedences in packet queue */
extern int  wland_pktq_mlen(struct pktq *pq, uint prec_bmp);
extern struct sk_buff *wland_pktq_mdeq(struct pktq *pq);

extern void wland_pktq_init(struct pktq *pq, int num_prec, int max_len);
/* prec_out may be NULL if caller is not interested in return value */
extern struct sk_buff *wland_pktq_peek_tail(struct pktq *pq, int *prec_out);
extern void wland_pktq_flush(struct pktq *pq, bool dir,	bool (*fn)(struct sk_buff *, void *), void *arg);

extern void get_ssid(u8*data, u8*ssid, u8*p_ssid_len);
extern u16  get_cap_info(u8*data);
extern void get_ssid(u8* data, u8* ssid, u8* p_ssid_len);
extern void get_BSSID(u8* data, u8* bssid);
extern u8   get_current_channel(u8 *pu8msa, u16 u16RxLen);
extern u8  *get_data_rate(u8 *pu8msa, u16 u16RxLen, u8 type, u8 *rate_size);
extern u8  *get_tim_elm(u8* pu8msa, u16 u16RxLen, u16 u16TagParamOffset);
extern u16  get_beacon_period(u8* data);
extern u8   num_2_char(u8 num);
extern void num_2_str(u8 num, u8* str);
#endif /* _WLAND_UTILS_H_ */

