
/*
 * Rda nftl ops
 *
 * (C) 2010 10
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
//#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/leds.h>

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <linux/mtd/blktrans.h>
#include <linux/mutex.h>

#include "rnftl.h"

static int rda_ops_read_page(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, 
								unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len)
{
	struct mtd_info *mtd = rda_nftl_info->mtd;
	//struct mtd_oob_ops rda_oob_ops;
	loff_t from;
	size_t len, retlen;
	int ret;

	struct mtd_oob_ops *rda_oob_ops;
	rda_oob_ops = rda_nftl_malloc(sizeof(struct mtd_oob_ops));
	
	if(rda_oob_ops== NULL){
		printk("%s,%d malloc failed\n",__func__,__LINE__);
		return -ENOMEM;;
	}
	
	from = mtd->erasesize;
	from *= blk_addr;
	from += page_addr * mtd->writesize;

	len = mtd->writesize;
	rda_oob_ops->mode = MTD_OPS_AUTO_OOB;
	rda_oob_ops->len = mtd->writesize;
	rda_oob_ops->ooblen = oob_len;
	rda_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
	rda_oob_ops->datbuf = data_buf;
	rda_oob_ops->oobbuf = nftl_oob_buf;

	if (nftl_oob_buf)
		//ret = mtd->read_oob(mtd, from, rda_oob_ops);
		ret = mtd_read_oob(mtd, from, rda_oob_ops);
	else
		//ret = mtd->read(mtd, from, len, &retlen, data_buf);
		ret = mtd_read(mtd, from, len, &retlen, data_buf);

	if (ret == -EUCLEAN) {
		//if (mtd->ecc_stats.corrected >= 10)
			//do read err 
		ret = 0;
	}
	if (ret)
		rda_nftl_dbg("rda_ops_read_page failed: %llx %d %d\n", from, blk_addr, page_addr);
	
	rda_nftl_free(rda_oob_ops);
	return ret;
}

static int rda_ops_write_page(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, 
								unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len)
{
	struct mtd_info *mtd = rda_nftl_info->mtd;
	//struct mtd_oob_ops rda_oob_ops;
	loff_t from;
	size_t len, retlen;
	int ret;

	struct mtd_oob_ops *rda_oob_ops;
	rda_oob_ops = rda_nftl_malloc(sizeof(struct mtd_oob_ops));
	
	if(rda_oob_ops== NULL){
		printk("%s,%d malloc failed\n",__func__,__LINE__);
		return -ENOMEM;;
	}
	
	from = mtd->erasesize;
	from *= blk_addr;
	from += page_addr * mtd->writesize;

	len = mtd->writesize;
	rda_oob_ops->mode = MTD_OPS_AUTO_OOB;
	rda_oob_ops->len = mtd->writesize;
	rda_oob_ops->ooblen = oob_len;
	rda_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
	rda_oob_ops->datbuf = data_buf;
	rda_oob_ops->oobbuf = nftl_oob_buf;

	if (nftl_oob_buf)
		//ret = mtd->write_oob(mtd, from, rda_oob_ops);
		ret = mtd_write_oob(mtd, from, rda_oob_ops);
	else
		//ret = mtd->write(mtd, from, len, &retlen, data_buf);
		ret = mtd_write(mtd, from, len, &retlen, data_buf);
	rda_nftl_free(rda_oob_ops);
	return ret;
}

static int rda_ops_read_page_oob(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr, 
										addr_page_t page_addr, unsigned char *nftl_oob_buf, int oob_len)
{
	struct mtd_info *mtd = rda_nftl_info->mtd;
//	struct mtd_oob_ops rda_oob_ops;
//	struct mtd_ecc_stats stats;
	loff_t from;
	int ret;
	
	struct mtd_oob_ops *rda_oob_ops;
	rda_oob_ops = rda_nftl_malloc(sizeof(struct mtd_oob_ops));
	
	if(rda_oob_ops== NULL){
		printk("%s,%d malloc failed\n",__func__,__LINE__);
		return -ENOMEM;;
	}
	//stats = mtd->ecc_stats;
	from = mtd->erasesize;
	from *= blk_addr;
	from += page_addr * mtd->writesize;

	rda_oob_ops->mode = MTD_OPS_AUTO_OOB;
	rda_oob_ops->len = 0;
	rda_oob_ops->ooblen = oob_len;
	rda_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
	rda_oob_ops->datbuf = NULL;
	rda_oob_ops->oobbuf = nftl_oob_buf;

	//ret = mtd->read_oob(mtd, from, rda_oob_ops);
	ret = mtd_read_oob(mtd, from, rda_oob_ops);

	if (ret == -EUCLEAN) {
		//if (mtd->ecc_stats.corrected >= 10)
			//do read err 
		ret = 0;
	}
	rda_nftl_free(rda_oob_ops);
	return ret;
}

static int rda_ops_blk_isbad(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr)
{
	struct mtd_info *mtd = rda_nftl_info->mtd;
	loff_t from;

	from = mtd->erasesize;
	from *= blk_addr;

	//return mtd->block_isbad(mtd, from);
	return mtd_block_isbad(mtd, from);
}

static int rda_ops_blk_mark_bad(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr)
{
	struct mtd_info *mtd = rda_nftl_info->mtd;
	loff_t from;

	from = mtd->erasesize;
	from *= blk_addr;

	//return mtd->block_markbad(mtd, from);
	return mtd_block_markbad(mtd, from);
}

static int rda_ops_erase_block(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr)
{
	struct mtd_info *mtd = rda_nftl_info->mtd;
	struct erase_info rda_nftl_erase_info;

	memset(&rda_nftl_erase_info, 0, sizeof(struct erase_info));
	rda_nftl_erase_info.mtd = mtd;
	rda_nftl_erase_info.addr = mtd->erasesize;
	rda_nftl_erase_info.addr *= blk_addr;
	rda_nftl_erase_info.len = mtd->erasesize;

	//return mtd->erase(mtd, &rda_nftl_erase_info);
	return mtd_erase(mtd, &rda_nftl_erase_info);
}

void rda_nftl_ops_init(struct rda_nftl_info_t *rda_nftl_info)
{
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_malloc(sizeof(struct rda_nftl_ops_t));
	if (!rda_nftl_ops)
		return;

	rda_nftl_info->rda_nftl_ops = rda_nftl_ops;
	rda_nftl_ops->read_page = rda_ops_read_page;
	rda_nftl_ops->write_page = rda_ops_write_page;
	rda_nftl_ops->read_page_oob = rda_ops_read_page_oob;
	rda_nftl_ops->blk_isbad = rda_ops_blk_isbad;
	rda_nftl_ops->blk_mark_bad = rda_ops_blk_mark_bad;
	rda_nftl_ops->erase_block = rda_ops_erase_block;

	return;
}

