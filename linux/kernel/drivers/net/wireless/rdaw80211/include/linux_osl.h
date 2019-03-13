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

#ifndef _LINUX_OSL_H_
#define _LINUX_OSL_H_

/*
 * BINOSL selects the slightly slower function-call-based binary compatible osl.
 * Macros expand to calls to functions defined in linux_osl.c .
 */
#include <linuxver.h>           /* use current 2.4.x calling conventions */
#include <linux/kernel.h>       /* for vsn/printf's */
#include <linux/string.h>       /* for mem*, str* */


typedef void (*pktfree_fn)(void *ctx, void *pkt, unsigned int status);


/* OSL initialization */
extern struct osl_info *osl_attach(void *pdev, uint bustype, bool pkttag);
extern void osl_detach(struct osl_info *osh);

/* ASSERT */
#if defined(BCMASSERT_LOG)
#define ASSERT(exp) \
	  do { if (!(exp)) osl_assert(#exp, __FILE__, __LINE__); } while (0)
extern void osl_assert(const char *exp, const char *file, int line);
#else
	#ifdef __GNUC__
		#define GCC_VERSION \
			(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
		#if GCC_VERSION > 30100
			#define ASSERT(exp)	do {} while (0)
		#else
			/* ASSERT could cause segmentation fault on GCC3.1, use empty instead */
			#define ASSERT(exp)
		#endif /* GCC_VERSION > 30100 */
	#endif /* __GNUC__ */
#endif 


/* Pkttag flag should be part of public information */
struct osl_pubinfo{
	bool                pkttag;
	uint                pktalloced; 	/* Number of allocated packet buffers */
	bool                mmbus;		    /* Bus supports memory-mapped register accesses */
	pktfree_fn          tx_fn;          /* Callback function for osl_pktfree */
	void               *tx_ctx;		    /* Context to the callback function */
};

struct osl_info {
	struct osl_pubinfo   pub;
	uint                 magic;
	void                *pdev;
	atomic_t             malloced;
	uint                 failed;
	uint                 bustype;
	spinlock_t           pktalloc_lock;
};

/* microsecond delay */
extern void  osl_delay(uint usec);
extern void *osl_malloc(struct osl_info *osh, uint size);
extern void  osl_free(struct osl_info *osh, void *addr, uint size);
extern uint  osl_malloced(struct osl_info *osh);


/* packet primitives */
#define	PKTDATA(osh, skb)		    (((struct sk_buff*)(skb))->data)
#define	PKTLEN(osh, skb)		    (((struct sk_buff*)(skb))->len)
#define PKTHEADROOM(osh, skb)		(((struct sk_buff*)(skb))->data-(((struct sk_buff*)(skb))->head))
#define	PKTNEXT(osh, skb)		    (((struct sk_buff*)(skb))->next)
#define	PKTSETNEXT(osh, skb, x)		(((struct sk_buff*)(skb))->next = (struct sk_buff*)(x))
#define	PKTSETLEN(osh, skb, len)	__skb_trim((struct sk_buff*)(skb), (len))
#define	PKTPUSH(osh, skb, bytes)	skb_push((struct sk_buff*)(skb), (bytes))
#define	PKTPULL(osh, skb, bytes)	skb_pull((struct sk_buff*)(skb), (bytes))

#define	PKTLINK(skb)			    (((struct sk_buff*)(skb))->prev)
#define	PKTSETLINK(skb, x)		    (((struct sk_buff*)(skb))->prev = (struct sk_buff*)(x))
#define	PKTPRIO(skb)			    (((struct sk_buff*)(skb))->priority)
#define	PKTSETPRIO(skb, x)		    (((struct sk_buff*)(skb))->priority = (x))
#define PKTSUMNEEDED(skb)		    (((struct sk_buff*)(skb))->ip_summed == CHECKSUM_HW)
#define PKTSETSUMGOOD(skb, x)		(((struct sk_buff*)(skb))->ip_summed = ((x) ? CHECKSUM_UNNECESSARY : CHECKSUM_NONE))

extern void *osl_open_image(const char* filename, int open_mode, int mode);
extern int   osl_get_image_block(char * buf, int len, void * image);
extern void  osl_close_image(void * image);

extern void  osl_pktfree(struct osl_info *osh, void *skb, bool send);
extern void *osl_pktget(struct osl_info *osh, uint len);

#endif	/* _LINUX_OSL_H_ */
