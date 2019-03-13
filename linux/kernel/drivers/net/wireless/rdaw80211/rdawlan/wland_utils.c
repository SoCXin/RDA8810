
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

#include <linuxver.h>
#include <linux/kernel.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <linux/debugfs.h>
#include <net/cfg80211.h>

#include <wland_defs.h>
#include <wland_fweh.h>
#include <wland_dev.h>
#include <wland_bus.h>
#include <wland_dbg.h>
#include <wland_utils.h>

void wland_pkt_align(struct sk_buff *p, int len, int align)
{
	uint datalign;

	datalign = (unsigned long) (p->data);
	datalign = roundup(datalign, (align)) - datalign;
	if (datalign)
		skb_pull(p, datalign);
	__skb_trim(p, len);
}

EXPORT_SYMBOL(wland_pkt_align);

struct sk_buff *wland_pkt_buf_get_skb(uint len)
{
	struct sk_buff *skb;

	skb = __dev_alloc_skb(len, GFP_ATOMIC);
	if (skb) {
		skb_put(skb, len);
		skb->priority = 0;
		memset(skb->data, 0, len);
	}
	return skb;
}

EXPORT_SYMBOL(wland_pkt_buf_get_skb);

/* Free the driver packet. Free the tag if present */
void wland_pkt_buf_free_skb(struct sk_buff *skb)
{
	struct sk_buff *nskb;

	if (!skb)
		return;
	WARN_ON(skb->next);

	while (skb) {
		nskb = skb->next;
		skb->next = NULL;
		if (skb->destructor) {
			/*
			 * cannot kfree_skb() on hard IRQ (net/core/skbuff.c) if destructor exists
			 */
			dev_kfree_skb_any(skb);
		} else {
			/*
			 * can free immediately (even in_irq()) if destructor does not exist
			 */
			dev_kfree_skb(skb);
		}
		skb = nskb;
	}
}

EXPORT_SYMBOL(wland_pkt_buf_free_skb);

/*
 * osl multiple-precedence packet queue
 * hi_prec is always >= the number of the highest non-empty precedence
 */
struct sk_buff *wland_pktq_penq(struct pktq *pq, int prec, struct sk_buff *p)
{
	struct sk_buff_head *q;

	if (pktq_full(pq) || pktq_pfull(pq, prec))
		return NULL;

	q = &pq->q[prec].skblist;
	skb_queue_tail(q, p);
	pq->len++;

	if (pq->hi_prec < prec)
		pq->hi_prec = (u8) prec;

	return p;
}

EXPORT_SYMBOL(wland_pktq_penq);

struct sk_buff *wland_pktq_penq_head(struct pktq *pq, int prec,
	struct sk_buff *p)
{
	struct sk_buff_head *q;

	if (pktq_full(pq) || pktq_pfull(pq, prec))
		return NULL;

	q = &pq->q[prec].skblist;
	skb_queue_head(q, p);
	pq->len++;

	if (pq->hi_prec < prec)
		pq->hi_prec = (u8) prec;

	return p;
}

EXPORT_SYMBOL(wland_pktq_penq_head);

struct sk_buff *wland_pktq_pdeq(struct pktq *pq, int prec)
{
	struct sk_buff_head *q = &pq->q[prec].skblist;
	struct sk_buff *p = skb_dequeue(q);

	if (p == NULL)
		return NULL;

	pq->len--;
	return p;
}

EXPORT_SYMBOL(wland_pktq_pdeq);

/*
 * precedence based dequeue with match function. Passing a NULL pointer
 * for the match function parameter is considered to be a wildcard so
 * any packet on the queue is returned. In that case it is no different
 * from wland_pktq_pdeq() above.
 */
struct sk_buff *wland_pktq_pdeq_match(struct pktq *pq, int prec,
	bool(*match_fn) (struct sk_buff * skb, void *arg), void *arg)
{
	struct sk_buff_head *q = &pq->q[prec].skblist;
	struct sk_buff *p, *next;

	skb_queue_walk_safe(q, p, next) {
		if (match_fn == NULL || match_fn(p, arg)) {
			skb_unlink(p, q);
			pq->len--;
			return p;
		}
	}
	return NULL;
}

EXPORT_SYMBOL(wland_pktq_pdeq_match);

struct sk_buff *wland_pktq_pdeq_tail(struct pktq *pq, int prec)
{
	struct sk_buff_head *q = &pq->q[prec].skblist;
	struct sk_buff *p = skb_dequeue_tail(q);

	if (p == NULL)
		return NULL;

	pq->len--;
	return p;
}

EXPORT_SYMBOL(wland_pktq_pdeq_tail);

void wland_pktq_pflush(struct pktq *pq, int prec, bool dir,
	bool(*fn) (struct sk_buff *, void *), void *arg)
{
	struct sk_buff_head *q;
	struct sk_buff *p, *next;

	q = &pq->q[prec].skblist;

	skb_queue_walk_safe(q, p, next) {
		if (fn == NULL || (*fn) (p, arg)) {
			skb_unlink(p, q);
			wland_pkt_buf_free_skb(p);
			pq->len--;
		}
	}
}

EXPORT_SYMBOL(wland_pktq_pflush);

void wland_pktq_flush(struct pktq *pq, bool dir, bool(*fn) (struct sk_buff *,
		void *), void *arg)
{
	int prec;

	for (prec = 0; prec < pq->num_prec; prec++)
		wland_pktq_pflush(pq, prec, dir, fn, arg);
}

EXPORT_SYMBOL(wland_pktq_flush);

void wland_pktq_init(struct pktq *pq, int num_prec, int max_len)
{
	int prec;

	/*
	 * pq is variable size; only zero out what's requested
	 */
	memset(pq, 0, offsetof(struct pktq,
			q) + (sizeof(struct pktq_prec) * num_prec));

	pq->num_prec = (u16) num_prec;
	pq->max = (u16) max_len;

	for (prec = 0; prec < num_prec; prec++) {
		pq->q[prec].max = pq->max;
		skb_queue_head_init(&pq->q[prec].skblist);
	}
}

EXPORT_SYMBOL(wland_pktq_init);

struct sk_buff *wland_pktq_peek_tail(struct pktq *pq, int *prec_out)
{
	int prec;

	if (pq->len == 0)
		return NULL;

	for (prec = 0; prec < pq->hi_prec; prec++)
		if (!skb_queue_empty(&pq->q[prec].skblist))
			break;

	if (prec_out)
		*prec_out = prec;

	return skb_peek_tail(&pq->q[prec].skblist);
}

EXPORT_SYMBOL(wland_pktq_peek_tail);

/* Return sum of lengths of a specific set of precedences */
int wland_pktq_mlen(struct pktq *pq, uint prec_bmp)
{
	int prec, len = 0;

	for (prec = 0; prec <= pq->hi_prec; prec++)
		if (prec_bmp & (1 << prec))
			len += pq->q[prec].skblist.qlen;

	return len;
}

EXPORT_SYMBOL(wland_pktq_mlen);

/* Priority dequeue from a specific set of precedences */
struct sk_buff *wland_pktq_mdeq(struct pktq *pq)
{
	struct sk_buff_head *q;
	struct sk_buff *p;
	int prec;

	if (pq->len == 0)
		return NULL;

	prec = 0;

	q = &pq->q[prec].skblist;
	p = skb_dequeue(q);
	if (p == NULL)
		return NULL;

	pq->len--;

	return p;
}

EXPORT_SYMBOL(wland_pktq_mdeq);

/* pretty hex print a contiguous buffer */
static void prhex(int level, const char *msg, u8 * buf, uint nbytes)
{
#define CHUNK_SIZE  64
	char line[3 * CHUNK_SIZE + 10], *p;
	int len = sizeof(line), nchar, chunk = CHUNK_SIZE;
	uint i;

	if (msg && (msg[0] != '\0'))
		printk("%s:%s [begin]\n", wland_dbgarea(level), msg);

	p = line;
	for (i = 0; i < nbytes; i++) {
		if (i % chunk == 0) {
			nchar = snprintf(p, len, " %04d: ", i);	/* line prefix */
			p += nchar;
			len -= nchar;
		}

		if (len > 0) {
			nchar = snprintf(p, len, "%02x ", buf[i]);
			p += nchar;
			len -= nchar;
		}

		if (i % chunk == (chunk - 1)) {
			printk("%s:%s\n", wland_dbgarea(level), line);	/* flush line */
			p = line;
			len = sizeof(line);
		}
	}

	/*
	 * flush last partial line
	 */
	if (p != line)
		printk("%s:%s\n", wland_dbgarea(level), line);

	if (msg && (msg[0] != '\0'))
		printk("%s:%s [end]\n", wland_dbgarea(level), msg);
}

void wland_dbg_hex_dump(int level, const void *data, size_t size,
	const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (data && size) {
		va_start(args, fmt);

		vaf.fmt = fmt;
		vaf.va = &args;
#ifndef DEBUG
		printk("[RDAWLAN_DUMP]:%pV", &vaf);
#endif /* DEBUG */
		va_end(args);

#ifdef DEBUG
		prhex(level,
			"=======================Hex Data=======================",
			(u8 *) data, size);
#else /* DEBUG */
		print_hex_dump_bytes("[RDAWLAN_DUMP]", "DATA", data, size);
#endif /* DEBUG */
	}
}

EXPORT_SYMBOL(wland_dbg_hex_dump);

/* This function extracts the beacon period field from the beacon or probe   */

/* response frame.                                                           */
u16 get_beacon_period(u8 * data)
{
	u16 bcn_per = 0;

	bcn_per = data[0];
	bcn_per |= (data[1] << 8);

	return bcn_per;
}

EXPORT_SYMBOL(get_beacon_period);

/* This function extracts the 'frame type' bits from the MAC header of the   */

/* input frame.                                                              */

/* Returns the value in the LSB of the returned value.                       */
u8 get_type(u8 * header)
{
	return ((u8) (header[0] & 0x0C));
}

EXPORT_SYMBOL(get_type);

/* This function extracts the 'frame type and sub type' bits from the MAC    */

/* header of the input frame.                                                */

/* Returns the value in the LSB of the returned value.                       */
u8 get_sub_type(u8 * header)
{
	return ((u8) (header[0] & 0xFC));
}

EXPORT_SYMBOL(get_sub_type);

/* This function extracts the 'to ds' bit from the MAC header of the input   */

/* frame.                                                                    */

/* Returns the value in the LSB of the returned value.                       */
u8 get_to_ds(u8 * header)
{
	return (header[1] & 0x01);
}

EXPORT_SYMBOL(get_to_ds);

/* This function extracts the 'from ds' bit from the MAC header of the input */

/* frame.                                                                    */

/* Returns the value in the LSB of the returned value.                       */
u8 get_from_ds(u8 * header)
{
	return ((header[1] & 0x02) >> 1);
}

EXPORT_SYMBOL(get_from_ds);

/* This function extracts the MAC Address in 'address1' field of the MAC     */

/* header and updates the MAC Address in the allocated 'addr' variable.      */
void get_address1(u8 * pu8msa, u8 * addr)
{
	memcpy(addr, pu8msa + 4, 6);
}

EXPORT_SYMBOL(get_address1);

/* This function extracts the MAC Address in 'address2' field of the MAC     */

/* header and updates the MAC Address in the allocated 'addr' variable.      */
void get_address2(u8 * pu8msa, u8 * addr)
{
	memcpy(addr, pu8msa + 10, 6);
}

EXPORT_SYMBOL(get_address2);

/* This function extracts the MAC Address in 'address3' field of the MAC     */

/* header and updates the MAC Address in the allocated 'addr' variable.      */
void get_address3(u8 * pu8msa, u8 * addr)
{
	memcpy(addr, pu8msa + 16, 6);
}

EXPORT_SYMBOL(get_address3);

/* This function extracts the BSSID from the incoming WLAN packet based on   */

/* the 'from ds' bit, and updates the MAC Address in the allocated 'addr'    */

/* variable.                                                                 */
void get_BSSID(u8 * data, u8 * bssid)
{
	if (get_from_ds(data) == 1)
		get_address2(data, bssid);
	else if (get_to_ds(data) == 1)
		get_address1(data, bssid);
	else
		get_address3(data, bssid);
}

EXPORT_SYMBOL(get_BSSID);

/* This function extracts the SSID from a beacon/probe response frame        */
void get_ssid(u8 * data, u8 * ssid, u8 * p_ssid_len)
{
	u8 len = 0;
	u8 i = 0;
	u8 j = 0;

	len = data[MAC_HDR_LEN + TIME_STAMP_LEN + BEACON_INTERVAL_LEN +
		CAP_INFO_LEN + 1];
	j = MAC_HDR_LEN + TIME_STAMP_LEN + BEACON_INTERVAL_LEN + CAP_INFO_LEN +
		2;

	/*
	 * If the SSID length field is set wrongly to a value greater than the
	 */
	/*
	 * allowed maximum SSID length limit, reset the length to 0
	 */
	if (len >= MAX_SSID_LEN)
		len = 0;

	for (i = 0; i < len; i++, j++)
		ssid[i] = data[j];

	ssid[len] = '\0';

	*p_ssid_len = len;
}

EXPORT_SYMBOL(get_ssid);

/* This function extracts the capability info field from the beacon or probe */

/* response frame.                                                           */
u16 get_cap_info(u8 * data)
{
	u16 cap_info = 0;
	u16 index = MAC_HDR_LEN;
	u8 st = BEACON;

	st = get_sub_type(data);

	/*
	 * Location of the Capability field is different for Beacon and
	 */
	/*
	 * Association frames.
	 */
	if ((st == BEACON) || (st == PROBE_RSP))
		index += TIME_STAMP_LEN + BEACON_INTERVAL_LEN;

	cap_info = data[index];
	cap_info |= (data[index + 1] << 8);

	return cap_info;
}

EXPORT_SYMBOL(get_cap_info);

/* This function extracts the capability info field from the Association */

/* response frame.                                                           		 */
u16 get_assoc_resp_cap_info(u8 * data)
{
	u16 cap_info = 0;

	cap_info = data[0];
	cap_info |= (data[1] << 8);

	return cap_info;
}

EXPORT_SYMBOL(get_assoc_resp_cap_info);

/* This funcion extracts the association status code from the incoming       */

/* association response frame and returns association status code            */
u16 get_asoc_status(u8 * data)
{
	u16 asoc_status = 0;

	asoc_status = data[3];
	asoc_status = (asoc_status << 8) | data[2];

	return asoc_status;
}

EXPORT_SYMBOL(get_asoc_status);

/* This function extracts association ID from the incoming association       */

/* response frame							                                     */
u16 get_asoc_id(u8 * data)
{
	u16 asoc_id = 0;

	asoc_id = data[4];
	asoc_id |= (data[5] << 8);

	return asoc_id;
}

EXPORT_SYMBOL(get_asoc_id);

u8 *get_tim_elm(u8 * pu8msa, u16 u16RxLen, u16 u16TagParamOffset)
{
	u16 u16index = u16TagParamOffset;

    /*************************************************************************/
	/*
	 * Beacon Frame - Frame Body
	 */
	/*
	 * ---------------------------------------------------------------------
	 */
	/*
	 * |Timestamp |BeaconInt |CapInfo |SSID |SupRates |DSParSet |TIM elm   |
	 */
	/*
	 * ---------------------------------------------------------------------
	 */
	/*
	 * |8         |2         |2       |2-34 |3-10     |3        |4-256     |
	 */
	/*
	 * ---------------------------------------------------------------------
	 */
	/*
	 */

    /*************************************************************************/

	/*
	 * Search for the TIM Element Field and return if the element is found
	 */
	while (u16index < (u16RxLen - FCS_LEN)) {
		if (pu8msa[u16index] == ITIM) {
			return (&pu8msa[u16index]);
		} else {
			u16index += (IE_HDR_LEN + pu8msa[u16index + 1]);
		}
	}

	return NULL;
}

EXPORT_SYMBOL(get_tim_elm);

u8 get_current_channel(u8 * pu8msa, u16 u16RxLen)
{
	u16 index =
		MAC_HDR_LEN + TIME_STAMP_LEN + BEACON_INTERVAL_LEN +
		CAP_INFO_LEN;

	while (index < (u16RxLen - FCS_LEN)) {
		if (pu8msa[index] == IDSPARMS)
			return (pu8msa[index + 2]);
		else
			/*
			 * Increment index by length information and header
			 */
			index += pu8msa[index + 1] + IE_HDR_LEN;
	}

	/*
	 * Return current channel information from the MIB, if beacon/probe
	 */
	/*
	 * response frame does not contain the DS parameter set IE
	 */
	return 0;		/* no MIB here */
}

EXPORT_SYMBOL(get_current_channel);

u8 *get_data_rate(u8 * pu8msa, u16 u16RxLen, u8 type, u8 * rate_size)
{
	u16 index =
		MAC_HDR_LEN + TIME_STAMP_LEN + BEACON_INTERVAL_LEN +
		CAP_INFO_LEN;

	while (index < (u16RxLen - FCS_LEN)) {
		if (pu8msa[index] == type) {
			if (rate_size)
				*rate_size = pu8msa[index + 1];
			return (&pu8msa[index + 2]);
		} else {
			/*
			 * Increment index by length information and header
			 */
			index += pu8msa[index + 1] + IE_HDR_LEN;
		}
	}

	/*
	 * Return current channel information from the MIB, if beacon/probe
	 */
	/*
	 * response frame does not contain the DS parameter set IE
	 */
	return NULL;		/* no MIB here */
}

EXPORT_SYMBOL(get_data_rate);

u8 num_2_char(u8 num)
{
	if (num >= 0 && num <= 9) {
		return '0' + num;
	} else {
		return 'a' + (num - 0x0a);
	}
}

EXPORT_SYMBOL(num_2_char);

void num_2_str(u8 num, u8 * str)
{
	*str = num_2_char((num >> 4) & 0x0f);
	*(str + 1) = num_2_char(num & 0x0f);
}

EXPORT_SYMBOL(num_2_str);
