/*
 * Rda nftl block device access
 *
 * (C) 2010 10
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/device.h>
#include <linux/pagemap.h>

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>

#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/freezer.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>

#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/reboot.h>

#include <linux/kmod.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/scatterlist.h>

#include "rnftl.h"

// use when in old kernel
//#define CONFIG_USE_REQUEST_IF 1

static struct mutex rda_nftl_lock;

#define nftl_notifier_to_blk(l)	container_of(l, struct rda_nftl_blk_t, nb)
#define cache_list_to_node(l)	container_of(l, struct write_cache_node, list)
#define free_list_to_node(l)	container_of(l, struct free_sects_list, list)

static int rda_nftl_write_cache_data(struct rda_nftl_blk_t *rda_nftl_blk, uint8_t cache_flag)
{
	struct rda_nftl_info_t *rda_nftl_info;
	int err = 0, i, j, limit_cache_cnt = NFTL_CACHE_FORCE_WRITE_LEN / 4;
	uint32_t blks_per_sect, current_cache_cnt;
	struct list_head *l, *n;

	current_cache_cnt = rda_nftl_blk->cache_buf_cnt;
	rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	blks_per_sect = rda_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &rda_nftl_blk->cache_list) {
		struct write_cache_node *cache_node = cache_list_to_node(l);

		for (i=0; i<blks_per_sect; i++) {
			if (!cache_node->cache_fill_status[i])
				break;
		}

		if (i < blks_per_sect) {
			err = rda_nftl_info->read_sect(rda_nftl_info, cache_node->vt_sect_addr, rda_nftl_blk->cache_buf);
			if (err)
				return err;

			rda_nftl_blk->cache_sect_addr = -1;
			//rda_nftl_dbg("nftl write cache data blk: %d page: %d \n", (cache_node->vt_sect_addr / rda_nftl_info->pages_per_blk), (cache_node->vt_sect_addr % rda_nftl_info->pages_per_blk));
			for (j=0; j<blks_per_sect; j++) {
				if (!cache_node->cache_fill_status[j])
					memcpy(cache_node->buf + j*512, rda_nftl_blk->cache_buf + j*512, 512);
			}
		}
		if (rda_nftl_blk->cache_sect_addr == cache_node->vt_sect_addr)
			memcpy(rda_nftl_blk->cache_buf, cache_node->buf, 512 * blks_per_sect);

		err = rda_nftl_info->write_sect(rda_nftl_info, cache_node->vt_sect_addr, cache_node->buf);
		if (err)
			return err;

		if (cache_node->bounce_buf_num < 0)
			rda_nftl_free(cache_node->buf);
		else
			rda_nftl_blk->bounce_buf_free[cache_node->bounce_buf_num] = NFTL_BOUNCE_FREE;
		list_del(&cache_node->list);
		rda_nftl_free(cache_node);
		rda_nftl_blk->cache_buf_cnt--;
		if ((cache_flag != CACHE_CLEAR_ALL) && (rda_nftl_blk->cache_buf_cnt <= (current_cache_cnt - limit_cache_cnt)))
			return 0;
	}

	return 0;
}

static int rda_nftl_search_cache_list(struct rda_nftl_blk_t *rda_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num, unsigned char *buf)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	struct write_cache_node *cache_node;
	int err = 0, i, j;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = rda_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &rda_nftl_blk->cache_list) {
		cache_node = cache_list_to_node(l);

		if (cache_node->vt_sect_addr == sect_addr) {
			for (i=blk_pos; i<(blk_pos+blk_num); i++) {
				if (!cache_node->cache_fill_status[i])
					break;
			}
			if (i < (blk_pos + blk_num)) {
				err = rda_nftl_info->read_sect(rda_nftl_info, sect_addr, rda_nftl_blk->cache_buf);
				if (err)
					return err;
				rda_nftl_blk->cache_sect_addr = sect_addr;

				for (j=0; j<blks_per_sect; j++) {
					if (cache_node->cache_fill_status[j])
						memcpy(rda_nftl_blk->cache_buf + j*512, cache_node->buf + j*512, 512);
				}
				memcpy(buf, rda_nftl_blk->cache_buf + blk_pos * 512, blk_num * 512);
			}
			else {
				memcpy(buf, cache_node->buf + blk_pos * 512, blk_num * 512);
			}

			return 0;
		}
	}

	return 1;
}

static int rda_nftl_add_cache_list(struct rda_nftl_blk_t *rda_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num, unsigned char *buf)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	struct write_cache_node *cache_node;
	int err = 0, i, j, need_write = 0;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = rda_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &rda_nftl_blk->cache_list) {
		cache_node = cache_list_to_node(l);

		if (cache_node->vt_sect_addr == sect_addr) {

			for (j=0; j<blks_per_sect; j++) {
				if (!cache_node->cache_fill_status[j])
					break;
			}
			if (j >= blks_per_sect)
				need_write = 1;

			for (i=blk_pos; i<(blk_pos+blk_num); i++)
				cache_node->cache_fill_status[i] = 1;
			memcpy(cache_node->buf + blk_pos * 512, buf, blk_num * 512);

			for (j=0; j<blks_per_sect; j++) {
				if (!cache_node->cache_fill_status[j])
					break;
			}

			if ((j >= blks_per_sect) && (need_write == 1)) {

				if (rda_nftl_blk->cache_sect_addr == cache_node->vt_sect_addr)
					memcpy(rda_nftl_blk->cache_buf, cache_node->buf, 512 * blks_per_sect);

				err = rda_nftl_info->write_sect(rda_nftl_info, sect_addr, cache_node->buf);
				if (err)
					return err;

				if (cache_node->bounce_buf_num < 0)
					rda_nftl_free(cache_node->buf);
				else
					rda_nftl_blk->bounce_buf_free[cache_node->bounce_buf_num] = NFTL_BOUNCE_FREE;
				list_del(&cache_node->list);
				rda_nftl_free(cache_node);
				rda_nftl_blk->cache_buf_cnt--;
			}

			return 0;
		}
	}

	if ((blk_pos == 0) && (blk_num == blks_per_sect))
		return CACHE_LIST_NOT_FOUND;

	if (rda_nftl_blk->cache_buf_cnt >= NFTL_CACHE_FORCE_WRITE_LEN) {
		err = rda_nftl_blk->write_cache_data(rda_nftl_blk, 0);
		if (err) {
			rda_nftl_dbg("nftl cache data full write faile %d err: %d\n", rda_nftl_blk->cache_buf_cnt, err);
			return err;
		}
	}

	cache_node = rda_nftl_malloc(sizeof(struct write_cache_node));
	if (!cache_node)
		return -ENOMEM;
	memset(cache_node->cache_fill_status, 0, MAX_BLKS_PER_SECTOR);

	for (i=0; i<NFTL_CACHE_FORCE_WRITE_LEN; i++) {
		if ((rda_nftl_blk->bounce_buf_free[i] == NFTL_BOUNCE_FREE) && (rda_nftl_blk->bounce_buf != NULL)) {
			cache_node->buf = (rda_nftl_blk->bounce_buf + i*rda_nftl_info->writesize);
			cache_node->bounce_buf_num = i;
			rda_nftl_blk->bounce_buf_free[i] = NFTL_BOUNCE_USED;
			break;
		}
	}

	if (!cache_node->buf) {
		// rda_nftl_dbg("cache buf need malloc %d\n", rda_nftl_blk->cache_buf_cnt);
		cache_node->buf = rda_nftl_malloc(rda_nftl_info->writesize);
		if (!cache_node->buf) 
			return -ENOMEM;
		cache_node->bounce_buf_num = -1;
	}

	cache_node->vt_sect_addr = sect_addr;
	for (i=blk_pos; i<(blk_pos+blk_num); i++)
		cache_node->cache_fill_status[i] = 1;
	memcpy(cache_node->buf + blk_pos * 512, buf, blk_num * 512);
	list_add_tail(&cache_node->list, &rda_nftl_blk->cache_list);
	rda_nftl_blk->cache_buf_cnt++;
	if (rda_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_IDLE) {
		rda_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_READY;
		wake_up_process(rda_nftl_blk->nftl_thread);
	}

#ifdef NFTL_DONT_CACHE_DATA
	if (rda_nftl_blk->cache_buf_cnt >= 1) {
		err = rda_nftl_blk->write_cache_data(rda_nftl_blk, 0);
		if (err) {
			rda_nftl_dbg("nftl cache data full write faile %d err: %d\n", rda_nftl_blk->cache_buf_cnt, err);
			return err;
		}
	}
#endif

	return 0;
}

static int rda_nftl_read_data(struct rda_nftl_blk_t *rda_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	unsigned blks_per_sect, read_sect_addr, read_misalign_num, buf_offset = 0;
	int i, error = 0, status = 0, total_blk_count = 0, read_sect_num;

	blks_per_sect = rda_nftl_info->writesize / 512;
	read_sect_addr = block / blks_per_sect;
	total_blk_count = nblk;
	read_misalign_num = ((blks_per_sect - block % blks_per_sect) > nblk ?  nblk : (blks_per_sect - block % blks_per_sect));

	if (block % blks_per_sect) {
		status = rda_nftl_blk->search_cache_list(rda_nftl_blk, read_sect_addr, block % blks_per_sect, read_misalign_num, buf);
		if (status) {
			if (read_sect_addr != rda_nftl_blk->cache_sect_addr) {
				error = rda_nftl_info->read_sect(rda_nftl_info, read_sect_addr, rda_nftl_blk->cache_buf);
				if (error)
					return error;
				rda_nftl_blk->cache_sect_addr = read_sect_addr;
			}
			memcpy(buf + buf_offset, rda_nftl_blk->cache_buf + (block % blks_per_sect) * 512, read_misalign_num * 512);
		}
		read_sect_addr++;
		total_blk_count -= read_misalign_num;
		buf_offset += read_misalign_num * 512;
	}

	if (total_blk_count >= blks_per_sect) {
		read_sect_num = total_blk_count / blks_per_sect;
		for (i=0; i<read_sect_num; i++) {
			status = rda_nftl_blk->search_cache_list(rda_nftl_blk, read_sect_addr, 0, blks_per_sect, buf + buf_offset);
			if (status) {
				error = rda_nftl_info->read_sect(rda_nftl_info, read_sect_addr, buf + buf_offset);
				if (error)
					return error;
			}
			read_sect_addr += 1;
			total_blk_count -= blks_per_sect;
			buf_offset += rda_nftl_info->writesize;
		}
	}

	if (total_blk_count > 0) {
		status = rda_nftl_blk->search_cache_list(rda_nftl_blk, read_sect_addr, 0, total_blk_count, buf + buf_offset);
		if (status) {
			if (read_sect_addr != rda_nftl_blk->cache_sect_addr) {
				error = rda_nftl_info->read_sect(rda_nftl_info, read_sect_addr, rda_nftl_blk->cache_buf);
				if (error)
					return error;
				rda_nftl_blk->cache_sect_addr = read_sect_addr;
			}	
			memcpy(buf + buf_offset, rda_nftl_blk->cache_buf, total_blk_count * 512);
		}
	}

	return 0;
}

static int rda_nftl_write_data(struct rda_nftl_blk_t *rda_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	unsigned blks_per_sect, write_sect_addr, write_misalign_num, buf_offset = 0;
	int error = 0, status = 0, total_blk_count = 0, write_sect_num, i;

	ktime_get_ts(&rda_nftl_blk->ts_write_start);
	if (rda_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY_DONE)
		rda_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_DONE;
	blks_per_sect = rda_nftl_info->writesize / 512;
	write_sect_addr = block / blks_per_sect;
	total_blk_count = nblk;
	write_misalign_num = ((blks_per_sect - block % blks_per_sect) > nblk ? nblk : (blks_per_sect - block % blks_per_sect));
	//rda_nftl_dbg("nftl write data blk: %d page: %d block: %ld count %d\n", (write_sect_addr / rda_nftl_info->pages_per_blk), (write_sect_addr % rda_nftl_info->pages_per_blk), block, nblk);

	if (block % blks_per_sect) {
		status = rda_nftl_blk->add_cache_list(rda_nftl_blk, write_sect_addr, block % blks_per_sect, write_misalign_num, buf);
		if (status)
			return status;

		write_sect_addr++;
		total_blk_count -= write_misalign_num;
		buf_offset += write_misalign_num * 512;
	}

	if (total_blk_count >= blks_per_sect) {
		write_sect_num = total_blk_count / blks_per_sect;
		for (i=0; i<write_sect_num; i++) {
			status = rda_nftl_blk->add_cache_list(rda_nftl_blk, write_sect_addr, 0, blks_per_sect, buf + buf_offset);
			if ((status) && (status != CACHE_LIST_NOT_FOUND))
				return status;

			if (status == CACHE_LIST_NOT_FOUND) {
				if (rda_nftl_blk->cache_sect_addr == write_sect_addr)
					memcpy(rda_nftl_blk->cache_buf, buf + buf_offset, 512 * blks_per_sect);

				error = rda_nftl_info->write_sect(rda_nftl_info, write_sect_addr, buf + buf_offset);
				if (error)
					return error;
			}
			write_sect_addr += 1;
			total_blk_count -= blks_per_sect;
			buf_offset += rda_nftl_info->writesize;
		}
	}

	if (total_blk_count > 0) {
		status = rda_nftl_blk->add_cache_list(rda_nftl_blk, write_sect_addr, 0, total_blk_count, buf + buf_offset);
		if (status)
			return status;
	}

	return 0;
}

static int rda_nftl_flush(struct mtd_blktrans_dev *dev)
{
	int error = 0;
	struct mtd_info *mtd = dev->mtd;
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)dev;

	mutex_lock(&rda_nftl_lock);
	//rda_nftl_dbg("nftl flush all cache data: %d\n", rda_nftl_blk->cache_buf_cnt);
	if (rda_nftl_blk->cache_buf_cnt > 0)
		error = rda_nftl_blk->write_cache_data(rda_nftl_blk, CACHE_CLEAR_ALL);

	if (mtd->_sync)
		mtd->_sync(mtd);
	mutex_unlock(&rda_nftl_lock);

	return error;
}

static unsigned int rda_nftl_map_sg(struct rda_nftl_blk_t *rda_nftl_blk)
{
	unsigned int sg_len;
	size_t buflen;
	struct scatterlist *sg;
	int i;

	if (!rda_nftl_blk->bounce_buf)
		return blk_rq_map_sg(rda_nftl_blk->queue, rda_nftl_blk->req, rda_nftl_blk->sg);

	sg_len = blk_rq_map_sg(rda_nftl_blk->queue, rda_nftl_blk->req, rda_nftl_blk->bounce_sg);

	rda_nftl_blk->bounce_sg_len = sg_len;

	buflen = 0;
	for_each_sg(rda_nftl_blk->bounce_sg, sg, sg_len, i)
		buflen += sg->length;

	sg_init_one(rda_nftl_blk->sg, rda_nftl_blk->bounce_buf, buflen);

	return sg_len;
}

static int rda_nftl_calculate_sg(struct rda_nftl_blk_t *rda_nftl_blk, size_t buflen, unsigned **buf_addr, unsigned *offset_addr)
{
	struct scatterlist *sgl;
	unsigned int offset = 0, segments = 0, buf_start = 0;
	struct sg_mapping_iter miter;
	unsigned long flags;
	unsigned int nents;
	unsigned int sg_flags = SG_MITER_ATOMIC;

	nents = rda_nftl_blk->bounce_sg_len;
	sgl = rda_nftl_blk->bounce_sg;

	if (rq_data_dir(rda_nftl_blk->req) == WRITE)
		sg_flags |= SG_MITER_FROM_SG;
	else
		sg_flags |= SG_MITER_TO_SG;

	sg_miter_start(&miter, sgl, nents, sg_flags);

	local_irq_save(flags);

	while (offset < buflen) {
		unsigned int len;
		if(!sg_miter_next(&miter))
			break;

		if (!buf_start) {
			segments = 0;
			*(buf_addr + segments) = (unsigned *)miter.addr;
			*(offset_addr + segments) = offset;
			buf_start = 1;
		}
		else {
			if ((unsigned char *)(*(buf_addr + segments)) + (offset - *(offset_addr + segments)) != miter.addr) {
				segments++;
				*(buf_addr + segments) = (unsigned *)miter.addr;
				*(offset_addr + segments) = offset;
			}
		}

		len = min(miter.length, buflen - offset);
		offset += len;
	}
	*(offset_addr + segments + 1) = offset;

	sg_miter_stop(&miter);

	local_irq_restore(flags);

	return segments;
}

/*
 * Alloc bounce buf for read/write numbers of pages in one request
 */
int rda_nftl_init_bounce_buf(struct mtd_blktrans_dev *dev, struct request_queue *rq)
{
	int ret=0, i;
	unsigned int bouncesz;
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)dev;

	rda_nftl_blk->queue = rq;
	bouncesz = (rda_nftl_blk->rda_nftl_info->writesize * NFTL_CACHE_FORCE_WRITE_LEN);
	if(bouncesz < RDA_NFTL_BOUNCE_SIZE)
		bouncesz = RDA_NFTL_BOUNCE_SIZE;

	if (bouncesz >= PAGE_CACHE_SIZE) {
		rda_nftl_blk->bounce_buf = rda_nftl_malloc(bouncesz);
		if (!rda_nftl_blk->bounce_buf) {
			rda_nftl_dbg("not enough memory for bounce buf\n");
			return -1;
		}
	}

	for (i=0; i<NFTL_CACHE_FORCE_WRITE_LEN; i++)
		rda_nftl_blk->bounce_buf_free[i] = NFTL_BOUNCE_FREE;

	queue_flag_test_and_set(QUEUE_FLAG_NONROT, rq);
	blk_queue_bounce_limit(rda_nftl_blk->queue, BLK_BOUNCE_HIGH);
	blk_queue_max_hw_sectors(rda_nftl_blk->queue, bouncesz / 512);
	blk_queue_physical_block_size(rda_nftl_blk->queue, bouncesz);
	blk_queue_max_segments(rda_nftl_blk->queue, bouncesz / PAGE_CACHE_SIZE);
	blk_queue_max_segment_size(rda_nftl_blk->queue, bouncesz);

	rda_nftl_blk->req = NULL;
	
	rda_nftl_blk->sg = rda_nftl_malloc(sizeof(struct scatterlist));
	if (!rda_nftl_blk->sg) {
		ret = -ENOMEM;
		blk_cleanup_queue(rda_nftl_blk->queue);
		return ret;
	}
	sg_init_table(rda_nftl_blk->sg, 1);

	rda_nftl_blk->bounce_sg = rda_nftl_malloc(sizeof(struct scatterlist) * bouncesz / PAGE_CACHE_SIZE);
	if (!rda_nftl_blk->bounce_sg) {
		ret = -ENOMEM;
		kfree(rda_nftl_blk->sg);
		rda_nftl_blk->sg = NULL;
		blk_cleanup_queue(rda_nftl_blk->queue);
		return ret;
	}
	sg_init_table(rda_nftl_blk->bounce_sg, bouncesz / PAGE_CACHE_SIZE);

	return 0;
}

static void rda_nftl_search_free_list(struct rda_nftl_blk_t *rda_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	struct free_sects_list *free_list;
	int i;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = rda_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &rda_nftl_blk->free_list) {
		free_list = free_list_to_node(l);

		if (free_list->vt_sect_addr == sect_addr) {
			for (i=blk_pos; i<(blk_pos+blk_num); i++) {
				if (free_list->free_blk_status[i])
					free_list->free_blk_status[i] = 0;
			}

			for (i=0; i<blks_per_sect; i++) {
				if (free_list->free_blk_status[i])
					break;;
			}
			if (i >= blks_per_sect) {
				list_del(&free_list->list);
				rda_nftl_free(free_list);
				return;
			}
			return;
		}
	}

	return;
}

static int rda_nftl_add_free_list(struct rda_nftl_blk_t *rda_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	struct free_sects_list *free_list;
	int i;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = rda_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &rda_nftl_blk->free_list) {
		free_list = free_list_to_node(l);

		if (free_list->vt_sect_addr == sect_addr) {
			for (i=blk_pos; i<(blk_pos+blk_num); i++)
				free_list->free_blk_status[i] = 1;

			for (i=0; i<blks_per_sect; i++) {
				if (free_list->free_blk_status[i] == 0)
					break;;
			}
			if (i >= blks_per_sect) {
				list_del(&free_list->list);
				rda_nftl_free(free_list);
				return 0;
			}

			return 1;
		}
	}

	if ((blk_pos == 0) && (blk_num == blks_per_sect))
		return 0;

	free_list = rda_nftl_malloc(sizeof(struct free_sects_list));
	if (!free_list)
		return -ENOMEM;

	free_list->vt_sect_addr = sect_addr;
	for (i=blk_pos; i<(blk_pos+blk_num); i++)
		free_list->free_blk_status[i] = 1;
	list_add_tail(&free_list->list, &rda_nftl_blk->free_list);

	return 1;
}

#ifdef CONFIG_USE_REQUEST_IF
static int do_nftltrans_request(struct mtd_blktrans_ops *tr,
			       struct mtd_blktrans_dev *dev,
			       struct request *req)
{
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)dev;
	int ret = 0, segments, i;
	unsigned long block, nblk, blk_addr, blk_cnt;
	unsigned short max_segm = queue_max_segments(rda_nftl_blk->queue);
	unsigned *buf_addr[max_segm+1];
	unsigned offset_addr[max_segm+1];
	size_t buflen;
	char *buf;

	memset((unsigned char *)buf_addr, 0, (max_segm+1)*4);
	memset((unsigned char *)offset_addr, 0, (max_segm+1)*4);
	rda_nftl_blk->req = req;
	block = blk_rq_pos(req) << 9 >> tr->blkshift;
	nblk = blk_rq_sectors(req);
	buflen = (nblk << tr->blkshift);

	if (!blk_fs_request(req))
		return -EIO;

	if (blk_rq_pos(req) + blk_rq_cur_sectors(req) >
	    get_capacity(req->rq_disk))
		return -EIO;

	if (blk_discard_rq(req))
		return tr->discard(dev, block, nblk);

	rda_nftl_blk->bounce_sg_len = rda_nftl_map_sg(rda_nftl_blk);
	segments = rda_nftl_calculate_sg(rda_nftl_blk, buflen, buf_addr, offset_addr);
	if (offset_addr[segments+1] != (nblk << tr->blkshift))
		return -EIO;

	mutex_lock(&rda_nftl_lock);
	switch(rq_data_dir(req)) {
		case READ:
			for(i=0; i<(segments+1); i++) {
				blk_addr = (block + (offset_addr[i] >> tr->blkshift));
				blk_cnt = ((offset_addr[i+1] - offset_addr[i]) >> tr->blkshift);
				buf = (char *)buf_addr[i];
				if (rda_nftl_blk->read_data(rda_nftl_blk, blk_addr, blk_cnt, buf)) {
					ret = -EIO;
					break;
				}
			}
			bio_flush_dcache_pages(rda_nftl_blk->req->bio);
			break;

		case WRITE:
			bio_flush_dcache_pages(rda_nftl_blk->req->bio);
			for(i=0; i<(segments+1); i++) {
				blk_addr = (block + (offset_addr[i] >> tr->blkshift));
				blk_cnt = ((offset_addr[i+1] - offset_addr[i]) >> tr->blkshift);
				buf = (char *)buf_addr[i];
				if (rda_nftl_blk->write_data(rda_nftl_blk, blk_addr, blk_cnt, buf)) {
					ret = -EIO;
					break;
				}
			}
			break;

		default:
			rda_nftl_dbg(KERN_NOTICE "Unknown request %u\n", rq_data_dir(req));
			break;
	}

	mutex_unlock(&rda_nftl_lock);

	return ret;
}
#endif

static int rda_nftl_write_sect(struct mtd_blktrans_dev *dev, unsigned long block, char *buf)
{
	int ret = 0;
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)dev;
	mutex_lock(&rda_nftl_lock);
	if (rda_nftl_blk->write_data(rda_nftl_blk, block, 1, buf)) {
		ret = -EIO;
	}
	mutex_unlock(&rda_nftl_lock);
	return ret;
}

static int rda_nftl_read_sect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	int ret = 0;
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)dev;
	mutex_lock(&rda_nftl_lock);
	if (rda_nftl_blk->read_data(rda_nftl_blk, block, 1, buf)) {
		ret = -EIO;
	}
	mutex_unlock(&rda_nftl_lock);
	return ret;
}

static int rda_nftl_thread(void *arg)
{
	struct rda_nftl_blk_t *rda_nftl_blk = arg;
	unsigned long period = NFTL_MAX_SCHEDULE_TIMEOUT / 10;
	struct timespec ts_nftl_current;

	while (!kthread_should_stop()) {
		struct rda_nftl_info_t *rda_nftl_info;
		struct rda_nftl_wl_t *rda_nftl_wl;

		mutex_lock(&rda_nftl_lock);
		rda_nftl_info = rda_nftl_blk->rda_nftl_info;
		rda_nftl_wl = rda_nftl_info->rda_nftl_wl;

		if ((rda_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY) 
				|| (rda_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY_DONE)
				|| (rda_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_DONE)) {

			rda_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_READY_DONE;
			mutex_unlock(&rda_nftl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(period);

			mutex_lock(&rda_nftl_lock);
			if ((rda_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY_DONE)
					&& (rda_nftl_blk->cache_buf_cnt > 0)) {

				ktime_get_ts(&ts_nftl_current);
				if ((ts_nftl_current.tv_sec - rda_nftl_blk->ts_write_start.tv_sec) >= NFTL_FLUSH_DATA_TIME) {
					rda_nftl_dbg("nftl writed cache data: %d %d %d\n", rda_nftl_blk->cache_buf_cnt, rda_nftl_wl->used_root.count, rda_nftl_wl->free_root.count);
					if (rda_nftl_blk->cache_buf_cnt >= (NFTL_CACHE_FORCE_WRITE_LEN / 2))
						rda_nftl_blk->write_cache_data(rda_nftl_blk, 0);
					else
						rda_nftl_blk->write_cache_data(rda_nftl_blk, CACHE_CLEAR_ALL);
				}

				if (rda_nftl_blk->cache_buf_cnt > 0) {
					mutex_unlock(&rda_nftl_lock);
					continue;
				}
				else {
					if ((rda_nftl_wl->free_root.count < (rda_nftl_info->fillfactor / 2)) && (!rda_nftl_wl->erased_root.count))
						rda_nftl_wl->gc_need_flag = 1;
					rda_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_IDLE;
				}
			}
			else {
				rda_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_READY_DONE;
				mutex_unlock(&rda_nftl_lock);
				continue;
			}
		}

		if (!rda_nftl_info->isinitialised) {

			mutex_unlock(&rda_nftl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(10);

			mutex_lock(&rda_nftl_lock);
			rda_nftl_info->creat_structure(rda_nftl_info);
			mutex_unlock(&rda_nftl_lock);
			continue;
		}

		if (!rda_nftl_wl->gc_need_flag) {
			mutex_unlock(&rda_nftl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			continue;
		}

		rda_nftl_wl->garbage_collect(rda_nftl_wl, DO_COPY_PAGE);
		if (rda_nftl_wl->free_root.count >= (rda_nftl_info->fillfactor / 2)) {
			rda_nftl_dbg("nftl initilize garbage completely: %d \n", rda_nftl_wl->free_root.count);
			rda_nftl_wl->gc_need_flag = 0;
		}
		mutex_unlock(&rda_nftl_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(period);
	}

	return 0;
}

static int rda_nftl_reboot_notifier(struct notifier_block *nb, unsigned long priority, void * arg)
{
	int error = 0;
	struct rda_nftl_blk_t *rda_nftl_blk = nftl_notifier_to_blk(nb);

	mutex_lock(&rda_nftl_lock);
	//rda_nftl_dbg("nftl reboot flush cache data: %d\n", rda_nftl_blk->cache_buf_cnt);
	if (rda_nftl_blk->cache_buf_cnt > 0)
		error = rda_nftl_blk->write_cache_data(rda_nftl_blk, CACHE_CLEAR_ALL);
	mutex_unlock(&rda_nftl_lock);

	return error;
}

static void rda_nftl_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct rda_nftl_blk_t *rda_nftl_blk;

	if (mtd->type != MTD_NANDFLASH)
		return;

	if (memcmp(mtd->name, "fat", 3)) {
		printk(KERN_INFO"rftl : %s : part name is %s, not 'fat', ignore\n", __func__, mtd->name);
		return;
	}

	rda_nftl_blk = rda_nftl_malloc(sizeof(struct rda_nftl_blk_t));
	if (!rda_nftl_blk)
		return;

	rda_nftl_blk->cache_sect_addr = -1;
	rda_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_IDLE;
	rda_nftl_blk->cache_buf_cnt = 0;
	rda_nftl_blk->mbd.mtd = mtd;
	rda_nftl_blk->mbd.devnum = mtd->index;
	rda_nftl_blk->mbd.tr = tr;
	rda_nftl_blk->nb.notifier_call = rda_nftl_reboot_notifier;

	register_reboot_notifier(&rda_nftl_blk->nb);
	INIT_LIST_HEAD(&rda_nftl_blk->cache_list);
	INIT_LIST_HEAD(&rda_nftl_blk->free_list);
	rda_nftl_blk->read_data = rda_nftl_read_data;
	rda_nftl_blk->write_data = rda_nftl_write_data;
	rda_nftl_blk->write_cache_data = rda_nftl_write_cache_data;
	rda_nftl_blk->search_cache_list = rda_nftl_search_cache_list;
	rda_nftl_blk->add_cache_list = rda_nftl_add_cache_list;
	rda_nftl_blk->cache_buf = rda_nftl_malloc(mtd->writesize);
	if (!rda_nftl_blk->cache_buf)
		return;

	if (rda_nftl_initialize(rda_nftl_blk))
		return;

	rda_nftl_blk->nftl_thread = kthread_run(rda_nftl_thread, rda_nftl_blk, "%sd", "rda_nftl");
	if (IS_ERR(rda_nftl_blk->nftl_thread))
		return;

	if (!(mtd->flags & MTD_WRITEABLE))
		rda_nftl_blk->mbd.readonly = 0;

#ifdef CONFIG_USE_REQUEST_IF
	if (rda_nftl_init_bounce_buf(&rda_nftl_blk->mbd, tr->blkcore_priv->rq))
		return;
#endif

	if (add_mtd_blktrans_dev(&rda_nftl_blk->mbd))
		rda_nftl_dbg("nftl add blk disk dev failed\n");

	return;
}

static int rda_nftl_open(struct mtd_blktrans_dev *mbd)
{
	return 0;
}

static int rda_nftl_release(struct mtd_blktrans_dev *mbd)
{
	int error = 0;
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)mbd;

	mutex_lock(&rda_nftl_lock);
	rda_nftl_dbg("nftl release flush cache data: %d\n", rda_nftl_blk->cache_buf_cnt);
	if (rda_nftl_blk->cache_buf_cnt > 0)
		error = rda_nftl_blk->write_cache_data(rda_nftl_blk, CACHE_CLEAR_ALL);
	mutex_unlock(&rda_nftl_lock);

	return error;
}

static void rda_nftl_blk_release(struct rda_nftl_blk_t *rda_nftl_blk)
{
	struct rda_nftl_info_t *rda_nftl_info = rda_nftl_blk->rda_nftl_info;
	rda_nftl_info_release(rda_nftl_info);
	rda_nftl_free(rda_nftl_info);
	if (rda_nftl_blk->cache_buf)
		rda_nftl_free(rda_nftl_blk->cache_buf);
	if (rda_nftl_blk->bounce_buf)
		rda_nftl_free(rda_nftl_blk->bounce_buf);
}

static void rda_nftl_remove_dev(struct mtd_blktrans_dev *dev)
{
	struct rda_nftl_blk_t *rda_nftl_blk = (void *)dev;

	unregister_reboot_notifier(&rda_nftl_blk->nb);
	del_mtd_blktrans_dev(dev);
	rda_nftl_free(dev);
	rda_nftl_blk_release(rda_nftl_blk);
	rda_nftl_free(rda_nftl_blk);
}

static struct mtd_blktrans_ops rda_nftl_tr = {
	.name		= "rnftl",
	.major		= RDA_NFTL_MAJOR,
	.part_bits	= 2,
	.blksize 	= 512,
	.open		= rda_nftl_open,
	.release	= rda_nftl_release,
#ifdef CONFIG_USE_REQUEST_IF
	.do_blktrans_request = do_nftltrans_request,
#else
	.readsect  = rda_nftl_read_sect,
	.writesect = rda_nftl_write_sect,
#endif
	.flush		= rda_nftl_flush,
	.add_mtd	= rda_nftl_add_mtd,
	.remove_dev	= rda_nftl_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init init_rda_nftl(void)
{
	mutex_init(&rda_nftl_lock);
	return register_mtd_blktrans(&rda_nftl_tr);
}

static void __exit cleanup_rda_nftl(void)
{
	deregister_mtd_blktrans(&rda_nftl_tr);
}

module_init(init_rda_nftl);
module_exit(cleanup_rda_nftl);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rda nftl block interface");
