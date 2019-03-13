/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 * Copyright (C) 2011-2012 Google, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <ide.h>
#include <part.h>
#include <mmc/sparse.h>

#undef	PART_DEBUG

#ifdef	PART_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

#if (defined(CONFIG_CMD_IDE) || \
     defined(CONFIG_CMD_MG_DISK) || \
     defined(CONFIG_CMD_SATA) || \
     defined(CONFIG_CMD_SCSI) || \
     defined(CONFIG_CMD_USB) || \
     defined(CONFIG_MMC) || \
     defined(CONFIG_SYSTEMACE) )

struct block_drvr {
	char *name;
	block_dev_desc_t* (*get_dev)(int dev);
};

static const struct block_drvr block_drvr[] = {
#if defined(CONFIG_CMD_IDE)
	{ .name = "ide", .get_dev = ide_get_dev, },
#endif
#if defined(CONFIG_CMD_SATA)
	{.name = "sata", .get_dev = sata_get_dev, },
#endif
#if defined(CONFIG_CMD_SCSI)
	{ .name = "scsi", .get_dev = scsi_get_dev, },
#endif
#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
	{ .name = "usb", .get_dev = usb_stor_get_dev, },
#endif
#if defined(CONFIG_MMC)
	{ .name = "mmc", .get_dev = mmc_get_dev, },
#endif
#if defined(CONFIG_SYSTEMACE)
	{ .name = "ace", .get_dev = systemace_get_dev, },
#endif
#if defined(CONFIG_CMD_MG_DISK)
	{ .name = "mgd", .get_dev = mg_disk_get_dev, },
#endif
	{ },
};

DECLARE_GLOBAL_DATA_PTR;

block_dev_desc_t *get_dev(const char* ifname, int dev)
{
	const struct block_drvr *drvr = block_drvr;
	block_dev_desc_t* (*reloc_get_dev)(int dev);
	char *name;

	if (!ifname)
		return NULL;

	name = drvr->name;
#ifdef CONFIG_NEEDS_MANUAL_RELOC
	name += gd->reloc_off;
#endif
	while (drvr->name) {
		name = drvr->name;
		reloc_get_dev = drvr->get_dev;
#ifdef CONFIG_NEEDS_MANUAL_RELOC
		name += gd->reloc_off;
		reloc_get_dev += gd->reloc_off;
#endif
		if (strncmp(ifname, name, strlen(name)) == 0)
			return reloc_get_dev(dev);
		drvr++;
	}
	return NULL;
}
/*
 * Given a devname with a concatenated ifname and an id (e.g. "mmc0"), return
 * a pointer to the associated block device structure.
 */
block_dev_desc_t *get_dev_by_name(char *devname)
{
	char *digit_pointer, *ep, c;
	int devid;
	for (digit_pointer = devname; (c = *digit_pointer); ) {
		if ('0' <= c && c <= '9') {
			/*
			 * Due to the ambiguity of determining if characters in
			 * the range of 'a' to 'f' are hex digits or part of
			 * the device interface, we use base 10.
			 */
			devid = (int)simple_strtoul(digit_pointer, &ep, 10);
			if (*ep != '\0') {
				/*
				 * There's non-digits after our digits, keep
				 * scanning for digits at the end.
				 */
				digit_pointer = ep;
				continue;
			}
			/*
			 * Normally, you would think we would put a '\0' at
			 * digit_pointer, but we don't want to modify the
			 * passed devname, and get_dev() uses strncmp() so it
			 * will match even if there is "junk" after the match.
			 * In our case, the "junk" is the id that we just
			 * parsed.  Go ahead and call get_dev().
			 */
			return get_dev(devname, devid);
		}
		digit_pointer++;
	}
	return NULL;
}

int get_partition_by_name(block_dev_desc_t *dev, const char *partition_name,
					disk_partition_t *partition_info)
{
	int i;
	if (!dev)
		return -ENODEV;
	if (!partition_name || !partition_info)
		return -EINVAL;
	if (dev->part_type == PART_TYPE_UNKNOWN) {
		init_part(dev); /* Maybe it is just uninitialized. */
		if (dev->part_type == PART_TYPE_UNKNOWN)
			return -ENOENT;
	}
	for (i = CONFIG_MIN_PARTITION_NUM; i <= CONFIG_MAX_PARTITION_NUM; i++) {
		if (get_partition_info(dev, i, partition_info))
			continue;
		if (!strcmp((char *)partition_info->name, partition_name))
			return 0;
	}
	return -ENOENT;
}

/*
 * Note that the partition functions use environment variables for storing
 * commands that need to be executed before and after the operations.
 * Refer to doc/README.partition_funcs for information about this.
 */

/*
 * Provide a helper function that will get the value of an environment
 * variable and interpret the value as commands to be run.  This is used to
 * allow users to have commands executed before and after partition
 * operations.  The environment variable that will be used is based upon the
 * three passed arguments.  Assuming that you want commands to be executed
 * before and after writing to a partition named "bob", you could do something
 * like:
 *	setenv pre_write.bob echo before
 *	setenv post_write.bob echo after
 */
enum when_t {BEFORE, AFTER};
static int run_env(enum when_t when, const char *op_str, const uchar *ptn_name)
{
	char var_name[sizeof("post_write.")
				+ sizeof(((disk_partition_t *)0)->name)
				+ 8 /* Extra future-proofing insurance */];
	char *var_val;
	int len;

	len = snprintf(var_name, sizeof(var_name), "%s_%s.%s",
		       (when == BEFORE) ? "pre" : "post", op_str, ptn_name);
	if (len >= sizeof(var_name))
		return -EOVERFLOW;

	var_val = getenv(var_name);
	if (var_val && run_command(var_val, 0) < 0)
		return -ENOEXEC;
	return 0;
}
int partition_erase_pre(disk_partition_t *ptn)
{
	return run_env(BEFORE, "erase", ptn->name);
}
int partition_erase_post(disk_partition_t *ptn)
{
	return run_env(AFTER, "erase", ptn->name);
}
int partition_read_pre(disk_partition_t *ptn)
{
	return run_env(BEFORE, "read", ptn->name);
}
int partition_read_post(disk_partition_t *ptn)
{
	return run_env(AFTER, "read", ptn->name);
}
int partition_write_pre(disk_partition_t *ptn)
{
	return run_env(BEFORE, "write", ptn->name);
}
int partition_write_post(disk_partition_t *ptn)
{
	return run_env(AFTER, "write", ptn->name);
}

/* Simple helper for partition_* functions to validate arguments. */
static int _partition_validate(block_dev_desc_t *dev, disk_partition_t *ptn,
			loff_t *bytecnt, lbaint_t *blkcnt, const void *buffer)
{
	if (!dev)
		return -ENODEV;
	if (!ptn || !buffer || (blkcnt && bytecnt) /* One or the other */)
		return -EINVAL;
	if (blkcnt && *blkcnt > ptn->size)
		return -EFBIG;
	/*
	 * Don't validate bytecnt yet because the pre_ commands
	 * (e.g. nandoob enable) may change the block size.
	 */
	return 0;
}

static loff_t get_and_zero_loff_t(loff_t *p)
{
	loff_t n;
	if (p) {
		n = *p;
		*p = 0;
		return n;
	}
	return 0;
}
static lbaint_t get_and_zero_lbaint_t(lbaint_t *p)
{
	lbaint_t n;
	if (p) {
		n = *p;
		*p = 0;
		return n;
	}
	return 0;
}
/*
 * Define the guts of the partition_ functions.  The _blks and _bytes
 * permutations will call these to do the actual work.
 */
static int _partition_erase(block_dev_desc_t *dev, disk_partition_t *ptn,
				loff_t *bytecnt_p, lbaint_t *blkcnt_p)
{
	/*
	 * Fetch the number of bytes or blocks and then zero them right away
	 * to make the error handling easier.
	 */
	loff_t bytes_to_do = get_and_zero_loff_t(bytecnt_p);
	lbaint_t blks_to_do = get_and_zero_lbaint_t(blkcnt_p), blks_done;
	int err = _partition_validate(dev, ptn, bytecnt_p, blkcnt_p,
							(void *)-1/*fake*/);
	if (err)
		return err;

	err = partition_erase_pre(ptn);
	if (err)
		return err;

	if (bytes_to_do) {
		blks_to_do = DIV_ROUND_UP(bytes_to_do, dev->blksz);
		if (blks_to_do > ptn->size)
			return -EFBIG;
	} else if (!blks_to_do)
		blks_to_do = ptn->size;

	blks_done = dev->block_erase(dev->dev, ptn->start, blks_to_do);
	if (blkcnt_p)
		*blkcnt_p = blks_done;
	if (bytecnt_p)
		*bytecnt_p = (loff_t)blks_done * dev->blksz;

	err = partition_erase_post(ptn);

	if (blks_done != blks_to_do)
		return -EIO;
	return err;
}
int partition_erase_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
							lbaint_t *blkcnt)
{
	return _partition_erase(dev, ptn, NULL, blkcnt);
}
int partition_erase_bytes(block_dev_desc_t *dev, disk_partition_t *ptn,
							loff_t *bytecnt)
{
	return _partition_erase(dev, ptn, bytecnt, NULL);
}

#ifdef CONFIG_MD5
void partition_md5_helper(block_dev_desc_t *dev, lbaint_t blk_start,
				lbaint_t *blkcnt, unsigned char md5[16])
{
	struct MD5Context context;
	lbaint_t buf_blks, blks_left, blks_read;
	unsigned char *buf;
#ifndef CONF_MD5_BUFLEN
#define CONF_MD5_BUFLEN (16*1024)
#endif

	buf_blks = CONF_MD5_BUFLEN / dev->blksz;
	if (!buf_blks) {
		*blkcnt = 0;
		return;
	}
	buf = malloc(buf_blks * dev->blksz);
	if (!buf) {
		*blkcnt = 0;
		return;
	}

	MD5Init(&context);
	blks_left = *blkcnt;
	while (blks_left) {
		if (blks_left < buf_blks)
			buf_blks = blks_left;

		blks_read = dev->block_read(dev->dev, blk_start, buf_blks,
									buf);
		MD5Update(&context, buf, blks_read * dev->blksz);
		blk_start += blks_read;
		blks_left -= blks_read;
		if (blks_read != buf_blks)
			break;
	}
	MD5Final(md5, &context);

	free(buf);
	*blkcnt -= blks_left;
}
static int _partition_md5(block_dev_desc_t *dev, disk_partition_t *ptn,
				loff_t *bytecnt_p, lbaint_t *blkcnt_p,
				unsigned char md5[16])
{
	/*
	 * Fetch the number of bytes or blocks and then zero them right away
	 * to make the error handling easier.
	 */
	loff_t bytes_to_do = get_and_zero_loff_t(bytecnt_p);
	lbaint_t blks_to_do = get_and_zero_lbaint_t(blkcnt_p), blks_done;
	int err = _partition_validate(dev, ptn, bytecnt_p, blkcnt_p,
							(void *)-1/*fake*/);
	if (err)
		return err;

	err = partition_read_pre(ptn);
	if (err)
		return err;

	if (bytes_to_do) {
		blks_to_do = DIV_ROUND_UP(bytes_to_do, dev->blksz);
		if (blks_to_do > ptn->size)
			return -EFBIG;
	} else if (!blks_to_do)
		blks_to_do = ptn->size;

	blks_done = blks_to_do;
	partition_md5_helper(dev, ptn->start, &blks_done, md5);

	if (blkcnt_p)
		*blkcnt_p = blks_done;
	if (bytecnt_p)
		*bytecnt_p = (loff_t)blks_done * dev->blksz;

	err = partition_read_post(ptn);

	if (blks_done != blks_to_do)
		return -EIO;
	return err;
}
int partition_md5_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
				lbaint_t *blkcnt, unsigned char md5[16])
{
	return _partition_md5(dev, ptn, NULL, blkcnt, md5);
}
int partition_md5_bytes(block_dev_desc_t *dev, disk_partition_t *ptn,
				loff_t *bytecnt, unsigned char md5[16])
{
	return _partition_md5(dev, ptn, bytecnt, NULL, md5);
}
#endif /* CONFIG_MD5 */

static int _partition_read(block_dev_desc_t *dev, disk_partition_t *ptn,
				loff_t *bytecnt_p, lbaint_t *blkcnt_p,
				void *buffer)
{
	/*
	 * Fetch the number of bytes or blocks and then zero them right away
	 * to make the error handling easier.
	 */
	loff_t bytes_to_do = get_and_zero_loff_t(bytecnt_p);
	lbaint_t blks_to_do = get_and_zero_lbaint_t(blkcnt_p), blks_done;
	int err = _partition_validate(dev, ptn, bytecnt_p, blkcnt_p, buffer);
	if (err)
		return err;

	err = partition_read_pre(ptn);
	if (err)
		return err;

	if (bytes_to_do) {
		blks_to_do = DIV_ROUND_UP(bytes_to_do, dev->blksz);
		if (blks_to_do > ptn->size)
			return -EFBIG;
	} else if (!blks_to_do)
		blks_to_do = ptn->size;

	blks_done = dev->block_read(dev->dev, ptn->start, blks_to_do, buffer);
	if (blkcnt_p)
		*blkcnt_p = blks_done;
	if (bytecnt_p)
		*bytecnt_p = (loff_t)blks_done * dev->blksz;

	err = partition_read_post(ptn);

	if (blks_done != blks_to_do)
		return -EIO;
	return err;
}
int partition_read_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
					lbaint_t *blkcnt, void *buffer)
{
	return _partition_read(dev, ptn, NULL, blkcnt, buffer);
}
int partition_read_bytes(block_dev_desc_t *dev, disk_partition_t *ptn,
					loff_t *bytecnt, void *buffer)
{
	return _partition_read(dev, ptn, bytecnt, NULL, buffer);
}

static int _partition_write(block_dev_desc_t *dev, disk_partition_t *ptn,
				loff_t *bytecnt_p, lbaint_t *blkcnt_p,
				const void *buffer)
{
#if defined(CONFIG_ERASE_PARTITION_ALWAYS)
	int do_erase = 1;
#else
	int do_erase = 0;
#endif
	/*
	 * Fetch the number of bytes or blocks and then zero them right away
	 * to make the error handling easier.
	 */
	loff_t bytes_to_do = get_and_zero_loff_t(bytecnt_p);
	lbaint_t blks_done,  blks_to_do = get_and_zero_lbaint_t(blkcnt_p);
	int err = _partition_validate(dev, ptn, bytecnt_p, blkcnt_p, buffer);
	if (err)
		return err;

	err = partition_write_pre(ptn);
	if (!err) {
		if (bytes_to_do) {
			blks_to_do = DIV_ROUND_UP(bytes_to_do, dev->blksz);
			if (blks_to_do > ptn->size)
				err = -EFBIG;
		} else if (!blks_to_do)
			blks_to_do = ptn->size;
	}
	blks_done = blks_to_do;
	if (!err) {
		if (do_erase)
			blks_done = dev->block_erase(dev->dev, ptn->start,
								blks_to_do);
		if (!do_erase || (blks_done == blks_to_do)) {
			blks_done = dev->block_write(dev->dev, ptn->start,
							blks_to_do, buffer);
			if (blkcnt_p)
				*blkcnt_p = blks_done;
			if (bytecnt_p)
				*bytecnt_p = (loff_t)blks_done * dev->blksz;
		}

		err = partition_write_post(ptn);
	}

	if (blks_done != blks_to_do)
		return -EIO;
	return err;
}

int partition_write_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
					lbaint_t *blkcnt, const void *buffer)
{
	return _partition_write(dev, ptn, NULL, blkcnt, buffer);
}
int partition_write_bytes(block_dev_desc_t *dev, disk_partition_t *ptn,
					loff_t *bytecnt, const void *buffer)
{
	return _partition_write(dev, ptn, bytecnt, NULL, buffer);
}

#define SPARSE_HEADER_MAJOR_VER 1

static int _unsparse(block_dev_desc_t *bdev, unsigned char *source,
					lbaint_t sector, lbaint_t num_blks)
{
	sparse_header_t *header = (void *) source;
	u32 i;
	unsigned long blksz = bdev->blksz;
	u64 section_size = (u64)num_blks * blksz;
	u64 outlen = 0;
	int blksz_shift;

	blksz_shift = LOG2(blksz);
	PRINTF("sparse_header:\n");
	PRINTF("\t         magic=0x%08X\n", header->magic);
	PRINTF("\t       version=%u.%u\n", header->major_version,
						header->minor_version);
	PRINTF("\t file_hdr_size=%u\n", header->file_hdr_sz);
	PRINTF("\tchunk_hdr_size=%u\n", header->chunk_hdr_sz);
	PRINTF("\t        blk_sz=%u\n", header->blk_sz);
	PRINTF("\t    total_blks=%u\n", header->total_blks);
	PRINTF("\t  total_chunks=%u\n", header->total_chunks);
	PRINTF("\timage_checksum=%u\n", header->image_checksum);
	PRINTF("\tblock device shift=%d\n", blksz_shift);

	if (header->magic != SPARSE_HEADER_MAGIC) {
		printf("sparse: bad magic\n");
		return 1;
	}

	if (((u64)header->total_blks * header->blk_sz) > section_size) {
		printf("sparse: section size %llu MB limit: exceeded\n",
				section_size/(1024*1024));
		return 1;
	}

	if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
	    (header->file_hdr_sz != sizeof(sparse_header_t)) ||
	    (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
		printf("sparse: incompatible format\n");
		return 1;
	}

	/* Skip the header now */
	source += header->file_hdr_sz;

	for (i = 0; i < header->total_chunks; i++) {
		u64 clen = 0;
		lbaint_t blkcnt;
		chunk_header_t *chunk = (void *) source;

		PRINTF("chunk_header:\n");
		PRINTF("\t    chunk_type=%u\n", chunk->chunk_type);
		PRINTF("\t      chunk_sz=%u\n", chunk->chunk_sz);
		PRINTF("\t      total_sz=%u\n", chunk->total_sz);
		/* move to next chunk */
		source += sizeof(chunk_header_t);

		switch (chunk->chunk_type) {
		case CHUNK_TYPE_RAW:
			clen = (u64)chunk->chunk_sz * header->blk_sz;
			PRINTF("sparse: RAW blk=%d bsz=%d:"
			       " write(sector=%lu,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, sector, clen);

			if (chunk->total_sz != (clen + sizeof(chunk_header_t))) {
				printf("sparse: bad chunk size for"
				       " chunk %d, type Raw\n", i);
				return 1;
			}

			outlen += clen;
			if (outlen > section_size) {
				printf("sparse: section size %llu MB limit:"
				       " exceeded\n", section_size/(1024*1024));
				return 1;
			}
			blkcnt = clen >> blksz_shift;
			debug("sparse: RAW blk=%d bsz=%d:"
			       " write(sector=%lu,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, sector, clen);
			if (bdev->block_write(bdev->dev,
						       sector, blkcnt, source)
						!= blkcnt) {
				printf("sparse: block write to sector %lu"
					" of %llu bytes (%ld blkcnt) failed\n",
					sector, clen, blkcnt);
				return 1;
			}

			sector += (clen >> blksz_shift);
			source += clen;
			break;

		case CHUNK_TYPE_DONT_CARE:
			if (chunk->total_sz != sizeof(chunk_header_t)) {
				printf("sparse: bogus DONT CARE chunk\n");
				return 1;
			}
			clen = (u64)chunk->chunk_sz * header->blk_sz;
			debug("sparse: DONT_CARE blk=%d bsz=%d:"
			       " skip(sector=%lu,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, sector, clen);

			outlen += clen;
			if (outlen > section_size) {
				printf("sparse: section size %llu MB limit:"
				       " exceeded\n", section_size/(1024*1024));
				return 1;
			}
			sector += (clen >> blksz_shift);
			break;

		default:
			printf("sparse: unknown chunk ID %04x\n",
			       chunk->chunk_type);
			return 1;
		}
	}

	printf("sparse: out-length %llu MB\n", outlen/(1024*1024));
	return 0;
}
/*
  unsparse an image with sparse header
*/

int partition_unsparse(block_dev_desc_t *dev, disk_partition_t *ptn,
		unsigned char *source, lbaint_t sector, lbaint_t num_blks)
{
	int rtn;
	if (partition_write_pre(ptn))
		return 1;

	rtn = _unsparse(dev, source, sector, num_blks);

	if (partition_write_post(ptn))
		return 1;

	return rtn;
}


#else
block_dev_desc_t *get_dev(char* ifname, int dev)
{
	return NULL;
}
block_dev_desc_t *get_dev_by_name(char *devname)
{
	return NULL;
}
int get_partition_by_name(block_dev_desc_t *dev, const char *partition_name,
					disk_partition_t *partition_info)
{
	return -ENODEV;
}
int partition_erase_pre(disk_partition_t *ptn)
{
	return -ENODEV;
}
int partition_erase_post(disk_partition_t *ptn)
{
	return -ENODEV;
}
int partition_read_pre(disk_partition_t *ptn)
{
	return -ENODEV;
}
int partition_read_post(disk_partition_t *ptn)
{
	return -ENODEV;
}
int partition_write_pre(disk_partition_t *ptn)
{
	return -ENODEV;
}
int partition_write_post(disk_partition_t *ptn)
{
	return -ENODEV;
}
int partition_erase_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
							lbaint_t *blkcnt)
{
	return -ENODEV;
}
int partition_erase_bytes(block_dev_desc_t *dev, disk_partition_t *partition,
							loff_t *bytecnt)
{
	return -ENODEV;
}
#ifdef CONFIG_MD5
int partition_md5_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
							lbaint_t *blkcnt)
{
	return -ENODEV;
}
int partition_md5_bytes(block_dev_desc_t *dev, disk_partition_t *partition,
							loff_t *bytecnt)
{
	return -ENODEV;
}
#endif /* CONFIG_MD5 */
int partition_read_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
					lbaint_t *blkcnt, void *buffer)
{
	return -ENODEV;
}
int partition_read_bytes(block_dev_desc_t *dev, disk_partition_t *partition,
					loff_t *bytecnt, void *buffer)
{
	return -ENODEV;
}
int partition_write_blks(block_dev_desc_t *dev, disk_partition_t *ptn,
					lbaint_t *blkcnt, const void *buffer)
{
	return -ENODEV;
}
int partition_write_bytes(block_dev_desc_t *dev, disk_partition_t *partition,
					loff_t *bytecnt, const void *buffer)
{
	return -ENODEV;
}
#endif

#if (defined(CONFIG_CMD_IDE) || \
     defined(CONFIG_CMD_MG_DISK) || \
     defined(CONFIG_CMD_SATA) || \
     defined(CONFIG_CMD_SCSI) || \
     defined(CONFIG_CMD_USB) || \
     defined(CONFIG_MMC) || \
     defined(CONFIG_SYSTEMACE) )

/* ------------------------------------------------------------------------- */
/*
 * reports device info to the user
 */

#ifdef CONFIG_LBA48
typedef uint64_t lba512_t;
#else
typedef lbaint_t lba512_t;
#endif

/*
 * Overflowless variant of (block_count * mul_by / div_by)
 * when div_by > mul_by
 */
static lba512_t lba512_muldiv (lba512_t block_count, lba512_t mul_by, lba512_t div_by)
{
	lba512_t bc_quot, bc_rem;

	/* x * m / d == x / d * m + (x % d) * m / d */
	bc_quot = block_count / div_by;
	bc_rem  = block_count - div_by * bc_quot;
	return bc_quot * mul_by + (bc_rem * mul_by) / div_by;
}

void dev_print (block_dev_desc_t *dev_desc)
{
	lba512_t lba512; /* number of blocks if 512bytes block size */

	if (dev_desc->type == DEV_TYPE_UNKNOWN) {
		puts ("not available\n");
		return;
	}

	switch (dev_desc->if_type) {
	case IF_TYPE_SCSI:
		printf ("(%d:%d) Vendor: %s Prod.: %s Rev: %s\n",
			dev_desc->target,dev_desc->lun,
			dev_desc->vendor,
			dev_desc->product,
			dev_desc->revision);
		break;
	case IF_TYPE_ATAPI:
	case IF_TYPE_IDE:
	case IF_TYPE_SATA:
		printf ("Model: %s Firm: %s Ser#: %s\n",
			dev_desc->vendor,
			dev_desc->revision,
			dev_desc->product);
		break;
	case IF_TYPE_SD:
	case IF_TYPE_MMC:
	case IF_TYPE_USB:
		printf ("Vendor: %s Rev: %s Prod: %s\n",
			dev_desc->vendor,
			dev_desc->revision,
			dev_desc->product);
		break;
	case IF_TYPE_DOC:
		puts("device type DOC\n");
		return;
	case IF_TYPE_UNKNOWN:
		puts("device type unknown\n");
		return;
	default:
		printf("Unhandled device type: %i\n", dev_desc->if_type);
		return;
	}
	puts ("            Type: ");
	if (dev_desc->removable)
		puts ("Removable ");
	switch (dev_desc->type & 0x1F) {
	case DEV_TYPE_HARDDISK:
		puts ("Hard Disk");
		break;
	case DEV_TYPE_CDROM:
		puts ("CD ROM");
		break;
	case DEV_TYPE_OPDISK:
		puts ("Optical Device");
		break;
	case DEV_TYPE_TAPE:
		puts ("Tape");
		break;
	default:
		printf ("# %02X #", dev_desc->type & 0x1F);
		break;
	}
	puts ("\n");
	if ((dev_desc->lba * dev_desc->blksz)>0L) {
		ulong mb, mb_quot, mb_rem, gb, gb_quot, gb_rem;
		lbaint_t lba;

		lba = dev_desc->lba;

		lba512 = (lba * (dev_desc->blksz/512));
		/* round to 1 digit */
		mb = lba512_muldiv(lba512, 10, 2048);	/* 2048 = (1024 * 1024) / 512 MB */

		mb_quot	= mb / 10;
		mb_rem	= mb - (10 * mb_quot);

		gb = mb / 1024;
		gb_quot	= gb / 10;
		gb_rem	= gb - (10 * gb_quot);
#ifdef CONFIG_LBA48
		if (dev_desc->lba48)
			printf ("            Supports 48-bit addressing\n");
#endif
#if defined(CONFIG_SYS_64BIT_LBA)
		printf ("            Capacity: %ld.%ld MB = %ld.%ld GB (%Ld x %ld)\n",
			mb_quot, mb_rem,
			gb_quot, gb_rem,
			lba,
			dev_desc->blksz);
#else
		printf ("            Capacity: %ld.%ld MB = %ld.%ld GB (%ld x %ld)\n",
			mb_quot, mb_rem,
			gb_quot, gb_rem,
			(ulong)lba,
			dev_desc->blksz);
#endif
	} else {
		puts ("            Capacity: not available\n");
	}
}
#endif

#if (defined(CONFIG_CMD_IDE) || \
     defined(CONFIG_CMD_MG_DISK) || \
     defined(CONFIG_CMD_SATA) || \
     defined(CONFIG_CMD_SCSI) || \
     defined(CONFIG_CMD_USB) || \
     defined(CONFIG_MMC)		|| \
     defined(CONFIG_SYSTEMACE) )

#if defined(CONFIG_MAC_PARTITION) || \
    defined(CONFIG_DOS_PARTITION) || \
    defined(CONFIG_ISO_PARTITION) || \
    defined(CONFIG_AMIGA_PARTITION) || \
    defined(CONFIG_EFI_PARTITION)

void init_part (block_dev_desc_t * dev_desc)
{
#ifdef CONFIG_ISO_PARTITION
	if (test_part_iso(dev_desc) == 0) {
		dev_desc->part_type = PART_TYPE_ISO;
		return;
	}
#endif

#ifdef CONFIG_MAC_PARTITION
	if (test_part_mac(dev_desc) == 0) {
		dev_desc->part_type = PART_TYPE_MAC;
		return;
	}
#endif

/* must be placed before DOS partition detection */
#ifdef CONFIG_EFI_PARTITION
	if (test_part_efi(dev_desc) == 0) {
		dev_desc->part_type = PART_TYPE_EFI;
		return;
	}
#endif

#ifdef CONFIG_DOS_PARTITION
	if (test_part_dos(dev_desc) == 0) {
		dev_desc->part_type = PART_TYPE_DOS;
		return;
	}
#endif

#ifdef CONFIG_AMIGA_PARTITION
	if (test_part_amiga(dev_desc) == 0) {
	    dev_desc->part_type = PART_TYPE_AMIGA;
	    return;
	}
#endif
}


int get_partition_info (block_dev_desc_t *dev_desc, int part
					, disk_partition_t *info)
{
	if (dev_desc->part_type == PART_TYPE_UNKNOWN)
		init_part(dev_desc); /* Maybe it is just uninitialized. */

	switch (dev_desc->part_type) {
#ifdef CONFIG_MAC_PARTITION
	case PART_TYPE_MAC:
		if (get_partition_info_mac(dev_desc,part,info) == 0) {
			PRINTF ("## Valid MAC partition found ##\n");
			return (0);
		}
		break;
#endif

#ifdef CONFIG_DOS_PARTITION
	case PART_TYPE_DOS:
		if (get_partition_info_dos(dev_desc,part,info) == 0) {
			PRINTF ("## Valid DOS partition found ##\n");
			return (0);
		}
		break;
#endif

#ifdef CONFIG_ISO_PARTITION
	case PART_TYPE_ISO:
		if (get_partition_info_iso(dev_desc,part,info) == 0) {
			PRINTF ("## Valid ISO boot partition found ##\n");
			return (0);
		}
		break;
#endif

#ifdef CONFIG_AMIGA_PARTITION
	case PART_TYPE_AMIGA:
	    if (get_partition_info_amiga(dev_desc, part, info) == 0)
	    {
		PRINTF ("## Valid Amiga partition found ##\n");
		return (0);
	    }
	    break;
#endif

#ifdef CONFIG_EFI_PARTITION
	case PART_TYPE_EFI:
		if (get_partition_info_efi(dev_desc,part,info) == 0) {
			PRINTF ("## Valid EFI partition found ##\n");
			return (0);
		}
		break;
#endif
	default:
		break;
	}
	return (-1);
}

static void print_part_header (const char *type, block_dev_desc_t * dev_desc)
{
	puts ("\nPartition Map for ");
	switch (dev_desc->if_type) {
	case IF_TYPE_IDE:
		puts ("IDE");
		break;
	case IF_TYPE_SATA:
		puts ("SATA");
		break;
	case IF_TYPE_SCSI:
		puts ("SCSI");
		break;
	case IF_TYPE_ATAPI:
		puts ("ATAPI");
		break;
	case IF_TYPE_USB:
		puts ("USB");
		break;
	case IF_TYPE_DOC:
		puts ("DOC");
		break;
	case IF_TYPE_MMC:
		puts ("MMC");
		break;
	default:
		puts ("UNKNOWN");
		break;
	}
	printf (" device %d  --   Partition Type: %s\n\n",
			dev_desc->dev, type);
}

void print_part (block_dev_desc_t * dev_desc)
{
	if (dev_desc->part_type == PART_TYPE_UNKNOWN)
		init_part(dev_desc); /* Maybe it is just uninitialized. */

		switch (dev_desc->part_type) {
#ifdef CONFIG_MAC_PARTITION
	case PART_TYPE_MAC:
		PRINTF ("## Testing for valid MAC partition ##\n");
		print_part_header ("MAC", dev_desc);
		print_part_mac (dev_desc);
		return;
#endif
#ifdef CONFIG_DOS_PARTITION
	case PART_TYPE_DOS:
		PRINTF ("## Testing for valid DOS partition ##\n");
		print_part_header ("DOS", dev_desc);
		print_part_dos (dev_desc);
		return;
#endif

#ifdef CONFIG_ISO_PARTITION
	case PART_TYPE_ISO:
		PRINTF ("## Testing for valid ISO Boot partition ##\n");
		print_part_header ("ISO", dev_desc);
		print_part_iso (dev_desc);
		return;
#endif

#ifdef CONFIG_AMIGA_PARTITION
	case PART_TYPE_AMIGA:
	    PRINTF ("## Testing for a valid Amiga partition ##\n");
	    print_part_header ("AMIGA", dev_desc);
	    print_part_amiga (dev_desc);
	    return;
#endif

#ifdef CONFIG_EFI_PARTITION
	case PART_TYPE_EFI:
		PRINTF ("## Testing for valid EFI partition ##\n");
		print_part_header ("EFI", dev_desc);
		print_part_efi (dev_desc);
		return;
#endif
	}
	puts ("## Unknown partition table\n");
}


#else	/* neither MAC nor DOS nor ISO nor AMIGA nor EFI partition configured */
# error neither CONFIG_MAC_PARTITION nor CONFIG_DOS_PARTITION
# error nor CONFIG_ISO_PARTITION nor CONFIG_AMIGA_PARTITION
# error nor CONFIG_EFI_PARTITION configured!
#endif

#endif
