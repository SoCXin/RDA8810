/*
 * drivers/mtd/nand/nand_util.c
 *
 * Copyright (C) 2006 by Weiss-Electronic GmbH.
 * All rights reserved.
 *
 * @author:	Guido Classen <clagix@gmail.com>
 * @descr:	NAND Flash support
 * @references: borrowed heavily from Linux mtd-utils code:
 *		flash_eraseall.c by Arcom Control System Ltd
 *		nandwrite.c by Steven J. Hill (sjhill@realitydiluted.com)
 *			       and Thomas Gleixner (tglx@linutronix.de)
 *
 * Copyright (C) 2008 Nokia Corporation: drop_ffs() function by
 * Artem Bityutskiy <dedekind1@gmail.com> from mtd-utils
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Copyright 2010 Freescale Semiconductor
 * The portions of this file whose copyright is held by Freescale and which
 * are not considered a derived work of GPL v2-only code may be distributed
 * and/or modified under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <common.h>
#include <command.h>
#include <watchdog.h>
#include <malloc.h>
#include <div64.h>

#include <asm/errno.h>
#include <linux/mtd/mtd.h>
#include <nand.h>
#include <jffs2/jffs2.h>
#include <mmc/sparse.h>

typedef struct erase_info erase_info_t;
typedef struct mtd_info mtd_info_t;

/* support only for native endian JFFS2 */
#define cpu_to_je16(x) (x)
#define cpu_to_je32(x) (x)

extern void udc_irq(void);
/**
 * nand_erase_opts: - erase NAND flash with support for various options
 *		      (jffs2 formating)
 *
 * @param meminfo	NAND device to erase
 * @param opts		options,  @see struct nand_erase_options
 * @return		0 in case of success
 *
 * This code is ported from flash_eraseall.c from Linux mtd utils by
 * Arcom Control System Ltd.
 */
int nand_erase_opts(nand_info_t * meminfo, const nand_erase_options_t * opts)
{
	struct jffs2_unknown_node cleanmarker;
	erase_info_t erase;
	unsigned long erase_length, erased_length;	/* in blocks */
	int bbtest = 1;
	int result;
	int percent_complete = -1;
	const char *mtd_device = meminfo->name;
	struct mtd_oob_ops oob_opts;
	struct nand_chip *chip = meminfo->priv;

	if (chip->page_shift == 0) {
		if (mtd_mod_by_ws(opts->offset, meminfo) != 0){
			printf("Attempt to erase non page aligned data\n");
			return -1;
		}
	} else {
		if ((opts->offset & (meminfo->writesize - 1)) != 0) {
			printf("Attempt to erase non page aligned data\n");
			return -1;
		}
	}

	memset(&erase, 0, sizeof(erase));
	memset(&oob_opts, 0, sizeof(oob_opts));

	erase.mtd = meminfo;
	erase.len = meminfo->erasesize;
	erase.addr = opts->offset;
	erase_length = lldiv(opts->length + meminfo->erasesize - 1,
			     meminfo->erasesize);

	cleanmarker.magic = cpu_to_je16(JFFS2_MAGIC_BITMASK);
	cleanmarker.nodetype = cpu_to_je16(JFFS2_NODETYPE_CLEANMARKER);
	cleanmarker.totlen = cpu_to_je32(8);

	/* scrub option allows to erase badblock. To prevent internal
	 * check from erase() method, set block check method to dummy
	 * and disable bad block table while erasing.
	 */
	if (opts->scrub) {
		erase.scrub = opts->scrub;
		/*
		 * We don't need the bad block table anymore...
		 * after scrub, there are no bad blocks left!
		 */
		if (chip->bbt) {
			kfree(chip->bbt);
		}
		chip->bbt = NULL;
	}

	for (erased_length = 0;
	     erased_length < erase_length; erase.addr += meminfo->erasesize) {

		WATCHDOG_RESET();
		udc_irq();

		if (!opts->scrub && bbtest) {
			int ret = meminfo->block_isbad(meminfo, erase.addr);
			if (ret > 0) {
				if (!opts->quiet)
					printf("\rSkipping bad block at  "
					       "0x%08llx                 "
					       "                         \n",
					       erase.addr);

				if (!opts->spread)
					erased_length++;

				continue;

			} else if (ret < 0) {
				printf("\n%s: MTD get bad block failed: %d\n",
				       mtd_device, ret);
				return -1;
			}
		}

		erased_length++;

		result = meminfo->erase(meminfo, &erase);
		if (result != 0) {
			printf("\n%s: MTD Erase failure: %d\n",
			       mtd_device, result);
			continue;
		}

		/* format for JFFS2 ? */
		if (opts->jffs2 && chip->ecc.layout->oobavail >= 8) {
			chip->ops.ooblen = 8;
			chip->ops.datbuf = NULL;
			chip->ops.oobbuf = (uint8_t *) & cleanmarker;
			chip->ops.ooboffs = 0;
			chip->ops.mode = MTD_OOB_AUTO;

			result = meminfo->write_oob(meminfo,
						    erase.addr, &chip->ops);
			if (result != 0) {
				printf("\n%s: MTD writeoob failure: %d\n",
				       mtd_device, result);
				continue;
			}
		}

		if (!opts->quiet) {
			unsigned long long n = erased_length * 100ULL;
			int percent;

			do_div(n, erase_length);
			percent = (int)n;

			/* output progress message only at whole percent
			 * steps to reduce the number of messages printed
			 * on (slow) serial consoles
			 */
			if (percent != percent_complete) {
				percent_complete = percent;

				printf("\rErasing at 0x%llx -- %3d%% complete.\n",
				       erase.addr, percent);

				if (opts->jffs2 && result == 0)
					printf
					    (" Cleanmarker written at 0x%llx.",
					     erase.addr);
			}
		}
	}
	if (!opts->quiet)
		printf("\n");

	if (opts->scrub)
		chip->scan_bbt(meminfo);

	return 0;
}

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK

/******************************************************************************
 * Support for locking / unlocking operations of some NAND devices
 *****************************************************************************/

#define NAND_CMD_LOCK		0x2a
#define NAND_CMD_LOCK_TIGHT	0x2c
#define NAND_CMD_UNLOCK1	0x23
#define NAND_CMD_UNLOCK2	0x24
#define NAND_CMD_LOCK_STATUS	0x7a

/**
 * nand_lock: Set all pages of NAND flash chip to the LOCK or LOCK-TIGHT
 *	      state
 *
 * @param mtd		nand mtd instance
 * @param tight		bring device in lock tight mode
 *
 * @return		0 on success, -1 in case of error
 *
 * The lock / lock-tight command only applies to the whole chip. To get some
 * parts of the chip lock and others unlocked use the following sequence:
 *
 * - Lock all pages of the chip using nand_lock(mtd, 0) (or the lockpre pin)
 * - Call nand_unlock() once for each consecutive area to be unlocked
 * - If desired: Bring the chip to the lock-tight state using nand_lock(mtd, 1)
 *
 *   If the device is in lock-tight state software can't change the
 *   current active lock/unlock state of all pages. nand_lock() / nand_unlock()
 *   calls will fail. It is only posible to leave lock-tight state by
 *   an hardware signal (low pulse on _WP pin) or by power down.
 */
int nand_lock(struct mtd_info *mtd, int tight)
{
	int ret = 0;
	int status;
	struct nand_chip *chip = mtd->priv;

	/* select the NAND device */
	chip->select_chip(mtd, 0);

	chip->cmdfunc(mtd,
		      (tight ? NAND_CMD_LOCK_TIGHT : NAND_CMD_LOCK), -1, -1);

	/* call wait ready function */
	status = chip->waitfunc(mtd, chip);

	/* see if device thinks it succeeded */
	if (status & 0x01) {
		ret = -1;
	}

	/* de-select the NAND device */
	chip->select_chip(mtd, -1);
	return ret;
}

/**
 * nand_get_lock_status: - query current lock state from one page of NAND
 *			   flash
 *
 * @param mtd		nand mtd instance
 * @param offset	page address to query (muss be page aligned!)
 *
 * @return		-1 in case of error
 *			>0 lock status:
 *			  bitfield with the following combinations:
 *			  NAND_LOCK_STATUS_TIGHT: page in tight state
 *			  NAND_LOCK_STATUS_LOCK:  page locked
 *			  NAND_LOCK_STATUS_UNLOCK: page unlocked
 *
 */
int nand_get_lock_status(struct mtd_info *mtd, loff_t offset)
{
	int ret = 0;
	int chipnr;
	int page;
	struct nand_chip *chip = mtd->priv;

	/* select the NAND device */
	if (chip->chip_shift == 0)
		chipnr = mtd_div_by_cs(offset, mtd);
	else
		chipnr = (int)(offset >> chip->chip_shift);

	chip->select_chip(mtd, chipnr);

	if (chip->page_shift == 0) {
		if (mtd_mod_by_ws(offset, mtd) != 0){
			printf("nand_get_lock_status: "
		       	"Start address must be beginning of " "nand page!\n");
			ret = -1;
			goto out;
		}

		/* check the Lock Status */
		page = mtd_div_by_ws(offset, mtd);
	} else {
		if ((offset & (mtd->writesize - 1)) != 0) {
			printf("nand_get_lock_status: "
		       	"Start address must be beginning of " "nand page!\n");
			ret = -1;
			goto out;
		}

		/* check the Lock Status */
		page = (int)(offset >> chip->page_shift);
	}

	chip->cmdfunc(mtd, NAND_CMD_LOCK_STATUS, -1, page & chip->pagemask);

	ret = chip->read_byte(mtd) & (NAND_LOCK_STATUS_TIGHT
				      | NAND_LOCK_STATUS_LOCK
				      | NAND_LOCK_STATUS_UNLOCK);

out:
	/* de-select the NAND device */
	chip->select_chip(mtd, -1);
	return ret;
}

/**
 * nand_unlock: - Unlock area of NAND pages
 *		  only one consecutive area can be unlocked at one time!
 *
 * @param mtd		nand mtd instance
 * @param start		start byte address
 * @param length	number of bytes to unlock (must be a multiple of
 *			page size nand->writesize)
 *
 * @return		0 on success, -1 in case of error
 */
int nand_unlock(struct mtd_info *mtd, ulong start, ulong length)
{
	int ret = 0;
	int chipnr;
	int status;
	int page;
	struct nand_chip *chip = mtd->priv;
	printf("nand_unlock: start: %08x, length: %d!\n",
	       (int)start, (int)length);

	/* select the NAND device */
	if (chip->chip_shift == 0)
		chipnr = mtd_div_by_cs(start, mtd);
	else
		chipnr = (int)(start >> chip->chip_shift);

	chip->select_chip(mtd, chipnr);

	/* check the WP bit */
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	if (!(chip->read_byte(mtd) & NAND_STATUS_WP)) {
		printf("nand_unlock: Device is write protected!\n");
		ret = -1;
		goto out;
	}

	if (chip->phy_erase_shift == 0) {
		if (mtd_mod_by_eb(start, mtd) != 0) {
			printf("nand_unlock: Start address must be beginning of "
		       	"nand block!\n");
			ret = -1;
			goto out;
		}

		if (length == 0 || mtd_mod_by_eb(length, mtd) != 0) {
			printf("nand_unlock: Length must be a multiple of nand block "
		       	"size %08x!\n", mtd->erasesize);
			ret = -1;
			goto out;
		}
	} else {
		if ((start & (mtd->erasesize - 1)) != 0) {
			printf("nand_unlock: Start address must be beginning of "
		       	"nand block!\n");
			ret = -1;
			goto out;
		}

		if (length == 0 || (length & (mtd->erasesize - 1)) != 0) {
			printf("nand_unlock: Length must be a multiple of nand block "
		       	"size %08x!\n", mtd->erasesize);
			ret = -1;
			goto out;
		}
	}

	/*
	 * Set length so that the last address is set to the
	 * starting address of the last block
	 */
	length -= mtd->erasesize;

	/* submit address of first page to unlock */
	if (chip->page_shift == 0)
		page = mtd_div_by_ws(start, mtd);
	else
		page = (int)(start >> chip->page_shift);

	chip->cmdfunc(mtd, NAND_CMD_UNLOCK1, -1, page & chip->pagemask);

	/* submit ADDRESS of LAST page to unlock */
	if (chip->page_shift == 0)
		page += mtd_div_by_ws(length, mtd);
	else
		page += (int)(length >> chip->page_shift);

	chip->cmdfunc(mtd, NAND_CMD_UNLOCK2, -1, page & chip->pagemask);

	/* call wait ready function */
	status = chip->waitfunc(mtd, chip);
	/* see if device thinks it succeeded */
	if (status & 0x01) {
		/* there was an error */
		ret = -1;
		goto out;
	}

out:
	/* de-select the NAND device */
	chip->select_chip(mtd, -1);
	return ret;
}
#endif

/**
 * check_skip_len
 *
 * Check if there are any bad blocks, and whether length including bad
 * blocks fits into device
 *
 * @param nand		NAND device
 * @param offset	offset in flash
 * @param length	image length
 * @param max_limit	maximun access offset in flash
 * @return 0 if the image fits and there are no bad blocks
 *         1 if the image fits, but there are bad blocks
 *        -1 if the image does not fit
 */
static int check_skip_len(nand_info_t * nand, loff_t offset, size_t length,
			  loff_t max_limit)
{
	size_t len_excl_bad = 0;
	int ret = 0;
	struct nand_chip *chip = nand->priv;

	if (max_limit == 0)
		max_limit = nand->size;

	while (len_excl_bad < length) {
		size_t block_len, block_off;
		loff_t block_start;

		if (offset >= max_limit)
			return -1;

		if (chip->phys_erase_shift == 0) {
			block_start = mtd_div_by_eb(offset, nand) * nand->erasesize;
			block_off = mtd_mod_by_eb(offset, nand);
		} else {
			block_start = offset & ~(loff_t) (nand->erasesize - 1);
			block_off = offset & (nand->erasesize - 1);
		}

		block_len = nand->erasesize - block_off;

		if (!nand_block_isbad(nand, block_start))
			len_excl_bad += block_len;
		else
			ret = 1;

		offset += block_len;
	}

	return ret;
}

#ifdef CONFIG_CMD_NAND_TRIMFFS
static size_t drop_ffs(const nand_info_t * nand, const u_char * buf,
		       const size_t * len)
{
	size_t i, l = *len;

	for (i = l - 1; i >= 0; i--)
		if (buf[i] != 0xFF)
			break;

	/* The resulting length must be aligned to the minimum flash I/O size */
	l = i + 1;
	l = (l + nand->writesize - 1) / nand->writesize;
	l *= nand->writesize;

	/*
	 * since the input length may be unaligned, prevent access past the end
	 * of the buffer
	 */
	return min(l, *len);
}
#endif

/**
 * nand_write_skip_bad_new:
 *
 * Write image to NAND flash.
 * Blocks that are marked bad are skipped and the is written to the next
 * block instead as long as the image is short enough to fit even after
 * skipping the bad blocks.
 *
 * @param nand  	NAND device
 * @param offset	offset in flash
 * @param length	buffer length
 * @param max_limit	maximun access offset in flash
 * @param buffer	buffer to read from
 * @param flags		flags modifying the behaviour of the write to NAND
 * @skip_blocks		the blocks has been skiped in this write
 * @return		0 in case of success
 */
int nand_write_skip_bad_new(nand_info_t * nand, loff_t offset, size_t * length,
			    loff_t max_limit, u_char * buffer, int flags,
			    u32 * skip_blocks)
{
	int rval = 0, blocksize;
	size_t left_to_write = *length;
	u_char *p_buffer = buffer;
	int need_skip;
	struct nand_chip *chip = nand->priv;

	*skip_blocks = 0;
#ifdef CONFIG_CMD_NAND_YAFFS
	if (flags & WITH_YAFFS_OOB) {
		if (flags & ~WITH_YAFFS_OOB)
			return -EINVAL;

		int pages;
		pages = nand->erasesize / nand->writesize;
		blocksize = (pages * nand->oobsize) + nand->erasesize;
		if (*length % (nand->writesize + nand->oobsize)) {
			printf("Attempt to write incomplete page"
			       " in yaffs mode\n");
			return -EINVAL;
		}
	} else
#endif
	{
		blocksize = nand->erasesize;
	}

	/*
	 * nand_write() handles unaligned, partial page writes.
	 *
	 * We allow length to be unaligned, for convenience in
	 * using the $filesize variable.
	 *
	 * However, starting at an unaligned offset makes the
	 * semantics of bad block skipping ambiguous (really,
	 * you should only start a block skipping access at a
	 * partition boundary).  So don't try to handle that.
	 */
	if (chip->page_shift == 0) {
		if (mtd_mod_by_ws(offset, nand) != 0) {
			printf("Attempt to write non page aligned data\n");
			*length = 0;
			return -EINVAL;
		}
	} else {
		if ((offset & (nand->writesize - 1)) != 0) {
			printf("Attempt to write non page aligned data\n");
			*length = 0;
			return -EINVAL;
		}
	}

	need_skip = check_skip_len(nand, offset, *length, max_limit);
	if (need_skip < 0) {
		printf("Attempt to write outside the flash area\n");
		*length = 0;
		return -EINVAL;
	}

	if (!need_skip && !(flags & WITH_DROP_FFS) && !(flags & WITH_YAFFS_OOB)) {
		rval = nand_write(nand, offset, length, buffer);
		if (rval == 0)
			return 0;

		*length = 0;
		printf("NAND write to offset %llx failed %d\n", offset, rval);
		return rval;
	}

	while (left_to_write > 0) {
		size_t write_size, truncated_write_size, block_offset;

		WATCHDOG_RESET();

		if (chip->phys_erase_shift == 0) {
			block_offset = mtd_mod_by_eb(offset, nand);

			if (nand_block_isbad(nand, mtd_div_by_eb(offset, nand) * nand->erasesize)) {
				printf("Skip bad block 0x%08llx\n",
					(loff_t)mtd_div_by_eb(offset, nand) * nand->erasesize);
				offset += nand->erasesize - block_offset;
				*skip_blocks += 1;
				continue;
			}
		}else {
			block_offset = offset & (nand->erasesize - 1);

			if (nand_block_isbad(nand, offset & ~(nand->erasesize - 1))) {
				printf("Skip bad block 0x%08llx\n", offset & ~(nand->erasesize - 1));
				offset += nand->erasesize - block_offset;
				*skip_blocks += 1;
				continue;
			}
		}

		if (left_to_write < (blocksize - block_offset))
			write_size = left_to_write;
		else
			write_size = blocksize - block_offset;

#ifdef CONFIG_CMD_NAND_YAFFS
		if (flags & WITH_YAFFS_OOB) {
			int page, pages;
			size_t pagesize = nand->writesize;
			size_t pagesize_oob = pagesize + nand->oobsize;
			struct mtd_oob_ops ops;

			ops.len = pagesize;
			ops.ooblen = nand->oobsize;
			ops.mode = MTD_OOB_AUTO;
			ops.ooboffs = 0;

			pages = write_size / pagesize_oob;
			for (page = 0; page < pages; page++) {
				WATCHDOG_RESET();

				ops.datbuf = p_buffer;
				ops.oobbuf = ops.datbuf + pagesize;

				rval = nand->write_oob(nand, offset, &ops);
				if (rval != 0)
					break;

				offset += pagesize;
				p_buffer += pagesize_oob;
			}
		} else
#endif
		{
			truncated_write_size = write_size;
#ifdef CONFIG_CMD_NAND_TRIMFFS
			if (flags & WITH_DROP_FFS)
				truncated_write_size = drop_ffs(nand, p_buffer,
								&write_size);
#endif

			rval = nand_write(nand, offset, &truncated_write_size,
					  p_buffer);
			offset += write_size;
			p_buffer += write_size;
		}

		if (rval != 0) {
			printf("NAND write to offset %llx failed %d\n",
			       offset, rval);
			*length -= left_to_write;
			return rval;
		}

		left_to_write -= write_size;
	}

	return 0;
}

#define SPARSE_HEADER_MAJOR_VER 1
#define SECTOR_SIZE     512
struct rnftl_oobinfo_t {
	int16_t sect;
	int16_t ec;		//00, Bad
	int16_t vtblk;		//-1 , free,-2~-10, Snapshot
	unsigned timestamp:15;
	unsigned status_page:1;
	int8_t maigic[4];
	uint8_t reserved[2];
};

static int sparse_parse(unsigned char *buffer, sparse_header_t ** header,
			loff_t max_limit)
{
	sparse_header_t *pheader;

	pheader = (sparse_header_t *) buffer;
	if (pheader->magic != SPARSE_HEADER_MAGIC) {
		printf("sparse: bad magic.\n");
		return -1;
	}

	if (((u64) pheader->total_blks * pheader->blk_sz) > max_limit) {
		printf("sparse: section size %08llx MB limit: exceeded\n",
		       max_limit / (1024 * 1024));
		return -1;
	}

	if ((pheader->major_version != SPARSE_HEADER_MAJOR_VER) ||
	    (pheader->file_hdr_sz != sizeof(sparse_header_t)) ||
	    (pheader->chunk_hdr_sz != sizeof(chunk_header_t))) {
		printf("sparse: incompatible format\n");
		return -1;
	}
	*header = pheader;
	return 0;
}

static int trunk_parse(unsigned char *buffer, sparse_header_t * sparse_header,
		       chunk_header_t ** header)
{
	chunk_header_t *pheader;

	pheader = (chunk_header_t *) buffer;
	if (pheader->chunk_type != CHUNK_TYPE_RAW &&
	    pheader->chunk_type != CHUNK_TYPE_DONT_CARE) {
		printf("chunk: unknow chunk type:0x%x.\n", pheader->chunk_type);
		return -1;
	}

	if (pheader->chunk_sz == 0) {
		printf("chunk: chunk_sz = 0.\n");
		return -1;
	}

	if (pheader->chunk_type == CHUNK_TYPE_RAW &&
	    pheader->total_sz !=
	    (u64) pheader->chunk_sz * sparse_header->blk_sz +
	    sizeof(chunk_header_t)) {
		printf
		    ("chunk: total_sz error.total_size = 0x%x,chunk_sz = 0x%x.\n",
		     pheader->total_sz, pheader->chunk_sz);
		return -1;
	}

	if (pheader->chunk_type == CHUNK_TYPE_DONT_CARE &&
	    pheader->total_sz != sizeof(chunk_header_t)) {
		printf
		    ("chunk: total_sz error.total_size = 0x%x,chunk_sz = 0x%x.\n",
		     pheader->total_sz, pheader->chunk_sz);
		return -1;
	}
	*header = pheader;
	return 0;
}

static int nand_write_nftl(nand_info_t * nand, loff_t offset,
			   unsigned char *buffer, int16_t vt_block,
			   int16_t vt_sect, uint16_t timestamp)
{
	int rval;
	struct mtd_oob_ops ops;
	struct rnftl_oobinfo_t nftl_oob;

	memset(&ops, 0, sizeof(ops));
	memset(&nftl_oob, 0, sizeof(nftl_oob));
	// ops
	ops.len = nand->writesize;
	ops.ooblen = sizeof(nftl_oob);
	ops.mode = MTD_OOB_AUTO;
	ops.ooboffs = 2;
	ops.oobbuf = (uint8_t *) & nftl_oob;
	ops.datbuf = buffer;
	//nftl_oob
	nftl_oob.ec = 0;
	nftl_oob.status_page = 1;
	memcpy(&(nftl_oob.maigic), "rda", 3);
	nftl_oob.sect = vt_sect;
	nftl_oob.vtblk = vt_block;
	nftl_oob.timestamp = timestamp;

	rval = nand->write_oob(nand, offset, &ops);
	if (rval != 0) {
		printf("write_oob fail, rval = %d.\n", rval);
		return -1;
	}
	return 0;
}

int nand_write_unsparse(nand_info_t * nand, loff_t offset, size_t * length,
			loff_t max_limit, unsigned char *buffer, int flags,
			u32 * skip_blocks)
{
	int rval = 0;
	u32 trunk_nr;
	u64 chunk_len = 0;
	u64 outlen = 0;
	loff_t base_offset;
	uint8_t *page_buffer = NULL;
	int16_t page_len = 0;
	int16_t page_per_block;
	int16_t vt_block = 0;
	int16_t vt_sect = 0;
	uint16_t timestamp = 0;
	sparse_header_t *sparse_h;
	struct nand_chip *chip = nand->priv;

	printf("nand_write_unsparse begin.\n");
	printf("offset:0x%08llx.\n", offset);
	printf("max_limit:0x%08llx.\n", max_limit);
	printf("flags:0x%x.\n", *length);
	printf("flags:0x%x.\n", flags);
	printf("skip_blocks:0x%x.\n", *skip_blocks);
	base_offset = offset;
	page_per_block = (nand->erasesize / nand->writesize);
	rval = sparse_parse(buffer, &sparse_h, max_limit);
	if (rval) {
		printf("nand_unsprase: sparse_parse fail.\n");
		return -1;
	}
	buffer += sizeof(sparse_header_t);
	(*length) -= sizeof(sparse_header_t);
	page_buffer = malloc(nand->writesize);
	for (trunk_nr = 0; trunk_nr < sparse_h->total_chunks; trunk_nr++) {
		chunk_header_t *chunk_h;

		rval = trunk_parse(buffer, sparse_h, &chunk_h);
		if (rval) {
			printf("nand_unsprase: trunk_parse fail.\n");
			free(page_buffer);
			return -1;
		}
		/*
		   printf("trunk:0x%x,0x%x,0x%x.\n",
		   trunk_nr,chunk_h->chunk_type,chunk_h->chunk_sz);
		 */
		buffer += sizeof(chunk_header_t);
		(*length) -= sizeof(chunk_header_t);
		chunk_len = (u64) chunk_h->chunk_sz * (u64) sparse_h->blk_sz;
		while (chunk_len > 0) {
			uint8_t *data_buff = page_buffer;
			if (chunk_h->chunk_type == CHUNK_TYPE_RAW) {
				memcpy(data_buff + page_len, buffer,
				       SECTOR_SIZE);
				buffer += SECTOR_SIZE;
				(*length) -= SECTOR_SIZE;
				page_len += SECTOR_SIZE;
				chunk_len -= SECTOR_SIZE;

			} else if (chunk_h->chunk_type == CHUNK_TYPE_DONT_CARE) {
				if(chunk_len >= (nand->writesize - page_len))
					chunk_len -= (nand->writesize - page_len);
				else
					chunk_len = 0;
				if (page_len) {
					printf("write spare data.page_len = 0x%x.\n",
					     page_len);
					/* memset(data_buff + page_len, 0,nand->writesize - page_len); */
				} else {
					/* memset(data_buff, 0,nand->writesize); */
					data_buff = NULL;
				}
				page_len = nand->writesize;
			} else {
				printf("nand_unsprase:"
					"Unknow Trunk type: 0x%x.\n",
					chunk_h->chunk_type);
				break;
			}
			if (page_len == nand->writesize) {
				/* skip bad block. */
				do {
					size_t block_offset;
					if (chip->phys_erase_shift == 0) {
						block_offset = mtd_mod_by_eb(offset, nand);
						if (nand_block_isbad(nand, mtd_div_by_eb(offset, nand) * nand->erasesize)) {
							printf("nand_unsprase:Skip bad block 0x%08llx.\n",
									(loff_t)mtd_div_by_eb(offset, nand) * nand->erasesize);
							offset += nand->erasesize -block_offset;
							(*skip_blocks)++;
							continue;
						} else {
							break;
						}
					} else {
						block_offset = offset & (nand->erasesize - 1);
						if (nand_block_isbad(nand,offset & ~(nand->erasesize - 1))) {
							printf("nand_unsprase:Skip bad block 0x%08llx.\n",
									offset & ~(nand->erasesize -1));
							offset += nand->erasesize -block_offset;
							(*skip_blocks)++;
							continue;
						} else {
							break;
						}
					}
				} while (1);

				if (offset + nand->writesize >
				    base_offset + max_limit) {
					printf("nand_unsprase:"
					       "exceed.section max_limit:0x%08llx,offset:0x%08llx.\n",
					       max_limit,
					       offset + nand->writesize -
					       base_offset);
					free(page_buffer);
					return -1;
				}
				rval = nand_write_nftl(nand, offset, data_buff,
						       vt_block, vt_sect,
						       timestamp);
				if (rval) {
					printf("nand_unsprase: nand_write_nftl fail.\n");
					free(page_buffer);
					return -1;
				}
				page_len = 0;
				if (vt_sect + 1 == page_per_block) {
					vt_block++;
					vt_sect = 0;
					timestamp = timestamp == 0x7fff ? 0 : timestamp + 1;
					if ((vt_block % 8) == 0
					    && vt_block >= 8) {
						printf("write block:0x%x - 0x%x.\n",
						     vt_block - 8, vt_block - 1);
					}
				} else {
					vt_sect++;
				}
				offset += nand->writesize;
				outlen += nand->writesize;
			}
		}
	}
	if (page_len > 0) {
		printf("nand_unsprase:last spare data: 0x%x,0x%x,0x%x.\n",
		       vt_block, vt_sect, page_len);
		/* memset(page_buffer + page_len, 0, nand->writesize - page_len); */
		if (offset + nand->writesize > base_offset + max_limit) {
			printf("nand_unsprase:"
			       "exceed max_limit:0x%08llx,offset:0x%08llx.\n",
			       max_limit,
			       offset + nand->writesize - base_offset);
			free(page_buffer);
			return -1;
		}
		/* skip bad block. */
		do {
			size_t block_offset;
			if (chip->phys_erase_shift == 0) {
				block_offset = mtd_mod_by_eb(offset, nand);
				if (nand_block_isbad(nand, mtd_div_by_eb(offset, nand) * nand->erasesize)) {
					printf("Skip bad block 0x%08llx.\n",
						(loff_t)mtd_div_by_eb(offset, nand) * nand->erasesize);
					offset += nand->erasesize - block_offset;
					(*skip_blocks)++;
					continue;
				} else {
					break;
				}
			} else {
				block_offset = offset & (nand->erasesize - 1);
				if (nand_block_isbad(nand, offset & ~(nand->erasesize - 1))) {
					printf("Skip bad block 0x%08llx.\n",
				       	offset & ~(nand->erasesize - 1));
					offset += nand->erasesize - block_offset;
					(*skip_blocks)++;
					continue;
				} else {
					break;
				}
			}
		} while (1);

		rval = nand_write_nftl(nand, offset, page_buffer,
				       vt_block, vt_sect, timestamp);
		if (rval) {
			printf("nand_unsprase: nand_write_nftl fail.\n");
			free(page_buffer);
			return -1;
		}
		page_len = 0;
		if (vt_sect + 1 == page_per_block) {
			vt_block++;
			vt_sect = 0;
			timestamp = timestamp == 0x7fff ? 0 : timestamp + 1;
		} else {
			vt_sect++;
		}
		offset += nand->writesize;
		outlen += nand->writesize;
	}
	if (vt_block % 8) {
		printf("write block:0x%x - 0x%x,vt_sector:0x%x.\n",
		       vt_block - (vt_block % 8), vt_block, vt_sect);
	}
	/*
	   while(offset +  nand->writesize <= max_limit)
	   {
	   rval =  nand_write_nftl(nand, offset, NULL,
	   vt_block, vt_sect,timestamp);
	   if (rval) {
	   printf("nand_unsprase: nand_write_nftl fail.\n");
	   free(page_buffer);
	   return -1;
	   }
	   if (vt_sect + 1 == page_per_block) {
	   vt_block++;
	   vt_sect = 0;
	   timestamp = timestamp == 0x7fff ?
	   0 : timestamp + 1;
	   } else {
	   vt_sect++;
	   }
	   offset += nand->writesize;
	   outlen += nand->writesize;
	   }
	 */
	free(page_buffer);
	printf("nand_unsprase end.\n");
	// printf("vt_block:0x%x.\n", vt_block);
	// printf("vt_sect:0x%x.\n", vt_sect);
	printf("*length:0x%x.\n", *length);
	printf("offset:0x%08llx.\n", offset);
	printf("out-length:0x%08llx.\n", outlen);
	printf("skip_blocks:0x%x.\n", *skip_blocks);
	return 0;
}

/**
 * nand_read_skip_bad_new:
 *
 * Read image from NAND flash.
 * Blocks that are marked bad are skipped and the next block is readen
 * instead as long as the image is short enough to fit even after skipping the
 * bad blocks.
 *
 * @param nand NAND device
 * @param offset offset in flash
 * @param length buffer length, on return holds remaining bytes to read
 * @param buffer buffer to write to
 * @param skip_block the bad block skipped
 * @return 0 in case of success
 */
int nand_read_skip_bad_new(nand_info_t * nand, loff_t offset,
			   size_t * length, u_char * buffer, u32 * skip_blocks)
{
	int rval;
	size_t left_to_read = *length;
	u_char *p_buffer = buffer;
	int need_skip;
	struct nand_chip *chip = nand->priv;

	*skip_blocks = 0;
	if (chip->page_shift == 0) {
		if (mtd_mod_by_ws(offset, nand) != 0) {
			printf("Attempt to read non page aligned data\n");
			*length = 0;
			return -EINVAL;
		}
	} else {
		if ((offset & (nand->writesize - 1)) != 0) {
			printf("Attempt to read non page aligned data\n");
			*length = 0;
			return -EINVAL;
		}
	}

	need_skip = check_skip_len(nand, offset, *length, 0);
	if (need_skip < 0) {
		printf("Attempt to read outside the flash area\n");
		*length = 0;
		return -EINVAL;
	}

	if (!need_skip) {
		rval = nand_read(nand, offset, length, buffer);
		if (!rval || rval == -EUCLEAN)
			return 0;

		*length = 0;
		printf("NAND read from offset %llx failed %d\n", offset, rval);
		return rval;
	}

	while (left_to_read > 0) {
		size_t block_offset;
		size_t read_length;

		WATCHDOG_RESET();
		if (chip->phys_erase_shift == 0) {
			block_offset = mtd_mod_by_eb(offset, nand);

			if (nand_block_isbad(nand, mtd_div_by_eb(offset, nand) * nand->erasesize)) {
				printf("Skipping bad block 0x%08llx\n",
					(loff_t)mtd_div_by_eb(offset, nand) * nand->erasesize);
				offset += nand->erasesize - block_offset;
				*skip_blocks += 1;
				continue;
			}
		} else {
			block_offset = offset & (nand->erasesize - 1);

			if (nand_block_isbad(nand, offset & ~(nand->erasesize - 1))) {
				printf("Skipping bad block 0x%08llx\n",
			       	offset & ~(nand->erasesize - 1));
				offset += nand->erasesize - block_offset;
				*skip_blocks += 1;
				continue;
			}
		}

		if (left_to_read < (nand->erasesize - block_offset))
			read_length = left_to_read;
		else
			read_length = nand->erasesize - block_offset;

		rval = nand_read(nand, offset, &read_length, p_buffer);
		if (rval && rval != -EUCLEAN) {
			printf("NAND read from offset %llx failed %d\n",
			       offset, rval);
			*length -= left_to_read;
			return rval;
		}

		left_to_read -= read_length;
		offset += read_length;
		p_buffer += read_length;
	}

	return 0;
}

/**
 * nand_write_skip_bad:
 *
 * Write image to NAND flash.
 * Blocks that are marked bad are skipped and the is written to the next
 * block instead as long as the image is short enough to fit even after
 * skipping the bad blocks.
 *
 * @param nand  	NAND device
 * @param offset	offset in flash
 * @param length	buffer length
 * @param buffer	buffer to read from
 * @param flags		flags modifying the behaviour of the write to NAND
 * @return		0 in case of success
 */
int nand_write_skip_bad(nand_info_t * nand, loff_t offset, size_t * length,
			u_char * buffer, int flags)
{
	u32 bad_blocks = 0;

	return nand_write_skip_bad_new(nand, offset, length, 0,
				       buffer, flags, &bad_blocks);
}

/**
 * nand_read_skip_bad:
 *
 * Read image from NAND flash.
 * Blocks that are marked bad are skipped and the next block is readen
 * instead as long as the image is short enough to fit even after skipping the
 * bad blocks.
 *
 * @param nand NAND device
 * @param offset offset in flash
 * @param length buffer length, on return holds remaining bytes to read
 * @param buffer buffer to write to
 * @return 0 in case of success
 */
int nand_read_skip_bad(nand_info_t * nand, loff_t offset, size_t * length,
		       u_char * buffer)
{
	u32 bad_blocks = 0;

	return nand_read_skip_bad_new(nand, offset, length, buffer,
				      &bad_blocks);
}
