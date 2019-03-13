
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

#define LINUX_PORT

#include <linuxver.h>
#include <linux_osl.h>
#include <wland_utils.h>
#include <linux/delay.h>
#include <linux/fs.h>

#define OS_HANDLE_MAGIC		    0x1234ABCD	/* Magic # to recognize osh */

struct osl_info *osl_attach(void *pdev, uint bustype, bool pkttag)
{
	struct osl_info *osh;

	if (!(osh = kmalloc(sizeof(struct osl_info), GFP_ATOMIC)))
		return osh;

	ASSERT(osh);

	memset(osh, '\0', sizeof(struct osl_info));

	osh->magic = OS_HANDLE_MAGIC;

	atomic_set(&osh->malloced, 0);

	osh->failed = 0;
	osh->pdev = pdev;
	osh->pub.pkttag = pkttag;
	osh->bustype = bustype;
	osh->pub.mmbus = false;

	spin_lock_init(&(osh->pktalloc_lock));

	return osh;
}

void osl_detach(struct osl_info *osh)
{
	if (osh == NULL)
		return;

	ASSERT(osh->magic == OS_HANDLE_MAGIC);
	kfree(osh);
}

static struct sk_buff *osl_alloc_skb(unsigned int len)
{
	return __dev_alloc_skb(len, GFP_ATOMIC);
}

/* Return a new packet. zero out pkttag */
void *osl_pktget(struct osl_info *osh, uint len)
{
	struct sk_buff *skb;
	ulong flags;

	if ((skb = osl_alloc_skb(len))) {
		skb_put(skb, len);
		skb->priority = 0;

		spin_lock_irqsave(&osh->pktalloc_lock, flags);
		osh->pub.pktalloced++;
		spin_unlock_irqrestore(&osh->pktalloc_lock, flags);
	}

	return ((void *) skb);
}

/* Free the driver packet. Free the tag if present */
void osl_pktfree(struct osl_info *osh, void *p, bool send)
{
	struct sk_buff *skb, *nskb;
	ulong flags;

	skb = (struct sk_buff *) p;

	if (send && osh->pub.tx_fn)
		osh->pub.tx_fn(osh->pub.tx_ctx, p, 0);

	/*
	 * perversion: we use skb->next to chain multi-skb packets
	 */
	while (skb) {
		nskb = skb->next;
		skb->next = NULL;

		if (skb->destructor)
			/*
			 * cannot kfree_skb() on hard IRQ (net/core/skbuff.c) if destructor exists
			 */
			dev_kfree_skb_any(skb);
		else
			/*
			 * can free immediately (even in_irq()) if destructor
			 * * does not exist
			 */
			dev_kfree_skb(skb);

		spin_lock_irqsave(&osh->pktalloc_lock, flags);
		osh->pub.pktalloced--;
		spin_unlock_irqrestore(&osh->pktalloc_lock, flags);
		skb = nskb;
	}
}

void *osl_malloc(struct osl_info *osh, uint size)
{
	void *addr;

	/*
	 * only ASSERT if osh is defined
	 */
	if (osh)
		ASSERT(osh->magic == OS_HANDLE_MAGIC);

	if ((addr = kmalloc(size, GFP_ATOMIC)) == NULL) {
		if (osh)
			osh->failed++;
		return (NULL);
	}
	if (osh)
		atomic_add(size, &osh->malloced);

	return (addr);
}

void osl_free(struct osl_info *osh, void *addr, uint size)
{
	if (osh) {
		ASSERT(osh->magic == OS_HANDLE_MAGIC);
		atomic_sub(size, &osh->malloced);
	}
	kfree(addr);
}

uint osl_malloced(struct osl_info *osh)
{
	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));
	return (atomic_read(&osh->malloced));
}

#if defined(BCMASSERT_LOG)
void osl_assert(const char *exp, const char *file, int line)
{
	char tempbuf[256];
	const char *basename;

	basename = strrchr(file, '/');
	/*
	 * skip the '/'
	 */
	if (basename)
		basename++;

	if (!basename)
		basename = file;

	snprintf(tempbuf, 64, "\"%s\": file \"%s\", line %d\n", exp, basename,
		line);

	printk("%s", tempbuf);
}
#endif

void osl_delay(uint usec)
{
	uint d;

	while (usec > 0) {
		d = MIN(usec, 1000);
		udelay(d);
		usec -= d;
	}
}

void *osl_open_image(const char *filename, int open_mode, int mode)
{
	struct file *fp = filp_open(filename, open_mode, mode);

	if (IS_ERR(fp))
		fp = NULL;

	return fp;
}

int osl_get_image_block(char *buf, int len, void *image)
{
	struct file *fp = (struct file *) image;
	int rdlen;

	if (!image)
		return 0;

	rdlen = kernel_read(fp, fp->f_pos, buf, len);
	if (rdlen > 0)
		fp->f_pos += rdlen;

	return rdlen;
}

void osl_close_image(void *image)
{
	if (image)
		filp_close((struct file *) image, NULL);
}
