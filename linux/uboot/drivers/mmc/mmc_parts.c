/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <linux/string.h>
#include "common.h"
#include <malloc.h>
#include <asm/arch/rda_sys.h>
#include "mmc.h"
#include <part.h>

#define EFI_ENTRIES	32

static int part_nums;

/* To support the Android-style naming of flash */
#define MAX_PTN (CONFIG_MAX_PARTITION_NUM - CONFIG_MIN_PARTITION_NUM + 1)
static disk_partition_t ptable[MAX_PTN];

/* special size referring to all the remaining space in a partition */
#define SIZE_REMAINING		(u64)(-1)

/* special offset value, it is used when not provided by user
 *
 * this value is used temporarily during parsing, later such offests
 * are recalculated */
#define OFFSET_NOT_SPECIFIED	(u64)(-1)


/* a bunch of this is duplicate of part_efi.c and part_efi.h
 * we should consider putting this code in there and layer
 * the partition info.
 */
#define EFI_VERSION 0x00010000
#define EFI_NAMELEN 36

static const u8 partition_type[16] = {
	0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
	0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};

static const u8 random_uuid[16] = {
	0xff, 0x1f, 0xf2, 0xf9, 0xd4, 0xa8, 0x0e, 0x5f,
	0x97, 0x46, 0x59, 0x48, 0x69, 0xae, 0xc3, 0x4e,
};

struct efi_entry {
	u8 type_uuid[16];
	u8 uniq_uuid[16];
	u64 first_lba;
	u64 last_lba;
	u64 attr;
	u16 name[EFI_NAMELEN];
};

struct efi_header {
	u8 magic[8];

	u32 version;
	u32 header_sz;

	u32 crc32;
	u32 reserved;

	u64 header_lba;
	u64 backup_lba;
	u64 first_lba;
	u64 last_lba;

	u8 volume_uuid[16];

	u64 entries_lba;

	u32 entries_count;
	u32 entries_size;
	u32 entries_crc32;
} __attribute__((packed));

struct ptable {
	u8 mbr[512];
	union {
		struct efi_header header;
		u8 block[512];
	};
	struct efi_entry entry[EFI_ENTRIES];
};

static struct ptable the_ptable;

/* in a board specific file */
struct fbt_partition {
	char name[32];
	char type[32];
	u64	offset;
	u64 	size;
	unsigned int attr;
};
/* For the 16GB eMMC part used in Tungsten, the erase group size is 512KB.
 * So every partition should be at least 512KB to make it possible to use
 * the mmc erase operation when doing 'fastboot erase'.
 * However, the xloader is an exception because in order for the OMAP4 ROM
 * bootloader to find it, it must be at offset 0KB, 128KB, 256KB, or 384KB.
 * Since the partition table is at 0KB, we choose 128KB.  Special care
 * must be kept to prevent erase the partition table when/if the xloader
 * partition is being erased.
 */
struct fbt_partition fbt_partitions[EFI_ENTRIES];

/**
 * Parses a string into a number.  The number stored at ptr is
 * potentially suffixed with K (for kilobytes, or 1024 bytes),
 * M (for megabytes, or 1048576 bytes), or G (for gigabytes, or
 * 1073741824).  If the number is suffixed with K, M, or G, then
 * the return value is the number multiplied by one kilobyte, one
 * megabyte, or one gigabyte, respectively.
 *
 * @param ptr where parse begins
 * @param retptr output pointer to next char after parse completes (output)
 * @return resulting unsigned int
 */
static unsigned long memsize_parse (const char *const ptr, const char **retptr)
{
	unsigned long ret = simple_strtoul(ptr, (char **)retptr, 0);

	switch (**retptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		(*retptr)++;
	default:
		break;
	}

	return ret;
}

#define MIN_PART_SIZE	4096
/**
 * Parse one partition definition, allocate memory and return pointer to this
 * location in retpart.
 *
 * @param partdef pointer to the partition definition string i.e. <part-def>
 * @param ret output pointer to next char after parse completes (output)
 * @param retpart pointer to the allocated partition (output)
 * @return 0 on success, 1 otherwise
 */
static int part_parse(const char *const partdef, const char **ret, struct fbt_partition *part)
{
	u64 size;
	u64 offset;
	const char *name;
	int name_len;
	unsigned int mask_flags;
	const char *p;

	p = partdef;
	*ret = NULL;

	/* fetch the partition size */
	if (*p == '-') {
		/* assign all remaining space to this partition */
		debug("'-': remaining size assigned\n");
		size = SIZE_REMAINING;
		p++;
	} else {
		size = memsize_parse(p, &p);
		if (size < MIN_PART_SIZE) {
			printf("partition size too small (%llx)\n", size);
			return 1;
		}
	}

	/* check for offset */
	offset = OFFSET_NOT_SPECIFIED;
	if (*p == '@') {
		p++;
		offset = memsize_parse(p, &p);
	}

	/* now look for the name */
	if (*p == '(') {
		name = ++p;
		if ((p = strchr(name, ')')) == NULL) {
			printf("no closing ) found in partition name\n");
			return 1;
		}
		name_len = p - name + 1;
		if ((name_len - 1) == 0) {
			printf("empty partition name\n");
			return 1;
		}
		p++;
	} else {
		/* 0x00000000@0x00000000 */
		name_len = 22;
		name = NULL;
	}

	/* test for options */
	mask_flags = 0;
	if (strncmp(p, "ro", 2) == 0) {
		mask_flags |= 1;
		p += 2;
	}

	/* check for next partition definition */
	if (*p == ',') {
		if (size == SIZE_REMAINING) {
			*ret = NULL;
			printf("no partitions allowed after a fill-up partition\n");
			return 1;
		}
		*ret = ++p;
	} else if ((*p == ';') || (*p == '\0')) {
		*ret = p;
	} else {
		printf("unexpected character '%c' at the end of partition\n", *p);
		*ret = NULL;
		return 1;
	}

	memset(part, 0, sizeof(struct fbt_partition));
	part->size = size;
	part->offset = offset;
	part->attr = mask_flags;

	if (name) {
		/* copy user provided name */
		strncpy(part->name, name, name_len - 1);
	} else {
		/* auto generated name in form of size@offset */
		sprintf(part->name, "0x%llx@0x%llx", size, offset);
	}

	part->name[name_len - 1] = '\0';

	debug("+ partition: name %-22s size 0x%llx offset 0x%llx mask flags %x\n",
			part->name, part->size,
			part->offset, part->attr);

	return 0;
}

/**
 * Performs sanity check for supplied partition. Offset and size are verified
 * to be within valid range. Partition type is checked and either
 * parts_validate_nor() or parts_validate_nand() is called with the argument
 * of part.
 *
 * @param id of the parent device
 * @param part partition to validate
 * @return 0 if partition is valid, 1 otherwise
 */
static int part_validate(struct mmc *mmc, struct fbt_partition *part)
{

	if (part->size == SIZE_REMAINING)
		part->size = mmc->capacity - part->offset;
/*

	if (part->offset > id->size) {
		printf("%s: offset %08x beyond flash size 0x%llx\n",
				id->mtd_id, part->offset, id->size);
		return 1;
	}

	if (((u64)part->offset + (u64)part->size) <= (u64)part->offset) {
		printf("%s%d: partition (%s) size too big\n",
				MTD_DEV_TYPE(id->type), id->num, part->name);
		return 1;
	}

	if (part->offset + part->size > id->size) {
		printf("%s: partitioning exceeds flash size %llx, part %x+%x\n",
			id->mtd_id, id->size, part->offset, part->size);
		return 1;
	}
*/
	/*
	 * Now we need to check if the partition starts and ends on
	 * sector (eraseblock) regions
	 */
	return 0;
}

static const char *const mmcparts_default = MTDPARTS_DEF;

int mmc_part_parse(struct mmc *mmc)
{
	const char *p;
	struct fbt_partition *part;
	u64 offset;
	int index = 0;
	int err = 0;

	p = mmcparts_default;
	offset = 0;
	while (p && (*p != '\0') && (*p != ';')) {
		err = 1;
		part =  &fbt_partitions[index];
		if ((part_parse(p, &p, part) != 0))
			break;

		/* calculate offset when not specified */
		if (part->offset == OFFSET_NOT_SPECIFIED)
			part->offset = offset;
		else
			offset = part->offset;

		/* verify alignment and size */
		if (part_validate(mmc, part) != 0)
			break;

		offset += part->size;

		/* partition is ok, add it to the list */
		index++;
		if (index > EFI_ENTRIES)
			break;
		err = 0;
	debug("++++++++++++ partition: name %-22s size 0x%llx offset 0x%llx mask flags %x\n",
			part->name, part->size,
			part->offset, part->attr);
	}
	part_nums = index;
	return err;
}


static void init_mbr(u8 *mbr, u32 blocks)
{
	mbr[0x1be] = 0x00; // nonbootable
	mbr[0x1bf] = 0xFF; // bogus CHS
	mbr[0x1c0] = 0xFF;
	mbr[0x1c1] = 0xFF;

	mbr[0x1c2] = 0xEE; // GPT partition
	mbr[0x1c3] = 0xFF; // bogus CHS
	mbr[0x1c4] = 0xFF;
	mbr[0x1c5] = 0xFF;

	mbr[0x1c6] = 0x01; // start
	mbr[0x1c7] = 0x00;
	mbr[0x1c8] = 0x00;
	mbr[0x1c9] = 0x00;

	memcpy(mbr + 0x1ca, &blocks, sizeof(u32));

	mbr[0x1fe] = 0x55;
	mbr[0x1ff] = 0xaa;
}

static void start_ptbl(struct ptable *ptbl, unsigned blocks)
{
	struct efi_header *hdr = &ptbl->header;

	memset(ptbl, 0, sizeof(*ptbl));

	init_mbr(ptbl->mbr, blocks - 1);

	memcpy(hdr->magic, "EFI PART", 8);
	hdr->version = EFI_VERSION;
	hdr->header_sz = sizeof(struct efi_header);
	hdr->header_lba = 1;
	hdr->backup_lba = blocks - 1;
	hdr->first_lba = 34;
	hdr->last_lba = blocks - 1;
	memcpy(hdr->volume_uuid, random_uuid, 16);
	hdr->entries_lba = 2;
	hdr->entries_count = EFI_ENTRIES;
	hdr->entries_size = sizeof(struct efi_entry);
}

static void end_ptbl(struct ptable *ptbl)
{
	struct efi_header *hdr = &ptbl->header;
	u32 n;

	n = crc32(0, 0, 0);
	n = crc32(n, (void*) ptbl->entry, sizeof(ptbl->entry));
	hdr->entries_crc32 = n;

	n = crc32(0, 0, 0);
	n = crc32(0, (void*) &ptbl->header, sizeof(ptbl->header));
	hdr->crc32 = n;
}

static int add_ptn(struct ptable *ptbl, u64 first, u64 last, const char *name)
{
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n;

	if (first < 34) {
		printf("partition '%s' overlaps partition table\n", name);
		return -1;
	}

	if (last > hdr->last_lba) {
		printf("partition '%s' does not fit\n", name);
		return -1;
	}
	for (n = 0; n < EFI_ENTRIES; n++, entry++) {
		if (entry->last_lba)
			continue;
		memcpy(entry->type_uuid, partition_type, 16);
		memcpy(entry->uniq_uuid, random_uuid, 16);
		entry->uniq_uuid[0] = n;
		entry->first_lba = first;
		entry->last_lba = last;
		for (n = 0; (n < EFI_NAMELEN) && *name; n++)
			entry->name[n] = *name++;
		return 0;
	}
	printf("out of partition table entries\n");
	return -1;
}

static unsigned int pcount;
void fbt_add_ptn(disk_partition_t *ptn)
{
	if (pcount < MAX_PTN) {
		memcpy(ptable + pcount, ptn, sizeof(*ptn));
		pcount++;
	}
}


static block_dev_desc_t *mmc_blkdev;

static int __def_fbt_load_ptbl(void)
{
	u64 length;
	disk_partition_t ptn;
	int n;
	int res = -1;
	block_dev_desc_t *blkdev = mmc_blkdev;
	unsigned long blksz = blkdev->blksz;

	init_part(blkdev);
	if (blkdev->part_type == PART_TYPE_UNKNOWN) {
		printf("unknown partition table on %s\n", CONFIG_MMC_DEV_NAME);
		return -1;
	}

	printf("lba size = %lu\n", blksz);
	printf("lba_start      partition_size          name\n");
	printf("=========  ======================  ==============\n");
	for (n = CONFIG_MIN_PARTITION_NUM; n <= CONFIG_MAX_PARTITION_NUM; n++) {
		if (get_partition_info(blkdev, n, &ptn))
			continue;	/* No partition <n> */
		if (!ptn.size || !ptn.blksz || !ptn.name[0])
			continue;	/* Partition <n> is empty (or sick) */
		fbt_add_ptn(&ptn);

		length = (u64)blksz * ptn.size;
		if (length > (1024 * 1024))
			printf(" %8lu  %12llu(%7lluM)  %s\n",
						ptn.start,
						length, length/(1024*1024),
						ptn.name);
		else
			printf(" %8lu  %12llu(%7lluK)  %s\n",
						ptn.start,
						length, length/1024,
						ptn.name);
		res = 0;
	}
	printf("=========  ======================  ==============\n");
	return res;
}

int board_fbt_load_ptbl(void)
	__attribute__((weak, alias("__def_fbt_load_ptbl")));

disk_partition_t *partition_find_ptn(const char *name);

static int fbt_load_partition_table(void)
{
	if (board_fbt_load_ptbl()) {
		printf("board_fbt_load_ptbl() failed\n");
		return -1;
	}

	return 0;
}

disk_partition_t *partition_find_ptn(const char *name)
{
	unsigned int n;

	if (pcount == 0) {
		if (fbt_load_partition_table()) {
			printf("Unable to load partition table, aborting\n");
			return NULL;
		}
	}

	for (n = 0; n < pcount; n++)
		if (!strcmp((char *)ptable[n].name, name))
			return ptable + n;
	return NULL;
}

void fbt_reset_ptn(void)
{
	pcount = 0;
	if (fbt_load_partition_table())
		printf("Unable to load partition table\n");
}

static int do_format(void)
{
	struct ptable *ptbl = &the_ptable;
	unsigned next;
	int n;
	block_dev_desc_t *dev_desc;
	unsigned long blocks_to_write, result;
	struct mmc *mmc;

	dev_desc = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (!dev_desc) {
		printf("error getting device %s\n", CONFIG_MMC_DEV_NAME);
		return -1;
	}
	if (!dev_desc->lba) {
		printf("device %s has no space\n", CONFIG_MMC_DEV_NAME);
		return -1;
	}
	mmc = container_of(dev_desc, struct mmc, block_dev);

	mmc_part_parse(mmc);
	start_ptbl(ptbl, dev_desc->lba);
	for (n = 0; n < part_nums; n++) {
		u64 sz = fbt_partitions[n].size / 512;
		next = fbt_partitions[n].offset / 512;
		if (fbt_partitions[n].name[0] == '-') {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = dev_desc->lba - next;

		if (add_ptn(ptbl, next, next + sz - 1, fbt_partitions[n].name))
			return -1;
	}
	end_ptbl(ptbl);

	blocks_to_write = DIV_ROUND_UP(sizeof(struct ptable), dev_desc->blksz);
	result = dev_desc->block_write(dev_desc->dev, 0, blocks_to_write, ptbl);
	if (result != blocks_to_write) {
		printf("\nFormat failed, block_write() returned %lu instead of %lu\n",
				 result, blocks_to_write);
		return -1;
	}

	debug("\nnew partition table of %lu %lu-byte blocks\n",
		blocks_to_write, dev_desc->blksz);
	fbt_reset_ptn();

	return 0;
}

/* check if the partition table is same with the table stored in device */
int mmc_check_parts(int *same)
{
	struct ptable *ptbl = NULL;
	struct ptable *ptbl_r = NULL;
	unsigned next;
	int n;
	block_dev_desc_t *dev_desc;
	unsigned long blocks_to_read, result;
	struct mmc *mmc;

	if(!same) return -1;
	*same = 0;

	dev_desc = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (!dev_desc) {
		printf("error getting device %s\n", CONFIG_MMC_DEV_NAME);
		return -1;
	}
	if (!dev_desc->lba) {
		printf("device %s has no space\n", CONFIG_MMC_DEV_NAME);
		return -1;
	}
	mmc = container_of(dev_desc, struct mmc, block_dev);

	ptbl = (struct ptable *)malloc(sizeof(struct ptable));
	if(!ptbl) {
		printf("can't malloc memory.\n");
		return -1;
	}
	ptbl_r = (struct ptable *)malloc(sizeof(struct ptable));
	if(!ptbl_r) {
		printf("can't malloc memory.\n");
		free(ptbl);
		return -1;
	}

	mmc_part_parse(mmc);
	start_ptbl(ptbl, dev_desc->lba);
	for (n = 0; n < part_nums; n++) {
		u64 sz = fbt_partitions[n].size / 512;
		next = fbt_partitions[n].offset / 512;
		if (fbt_partitions[n].name[0] == '-') {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = dev_desc->lba - next;

		if (add_ptn(ptbl, next, next + sz - 1, fbt_partitions[n].name)) {
			free(ptbl);
			free(ptbl_r);
			return -1;
		}
	}
	end_ptbl(ptbl);

	memset((void *)ptbl_r, 0, sizeof(struct ptable));
	blocks_to_read = DIV_ROUND_UP(sizeof(struct ptable), dev_desc->blksz);
	result = dev_desc->block_read(dev_desc->dev, 0, blocks_to_read, ptbl_r);
	if (result != blocks_to_read) {
		printf("\nCheck Failed, block_read() returned %lu instead of %lu\n",
				 result, blocks_to_read);
		free(ptbl);
		free(ptbl_r);
		return -1;
	}

#if 0
	/* dump */
	printf("New paritition table:\n");
	for(n = 0; n < sizeof(struct ptable); n++) {
		char *p = (char *)ptbl;
		printf("%02x ", p[n]);
		if((n + 1) % 32 == 0 )
			printf("\n");
	}
	printf("Old partition table:\n");
	for(n = 0; n < sizeof(struct ptable); n++) {
		char *p = (char *)ptbl_r;
		printf("%02x ", p[n]);
		if((n + 1) % 32 == 0 )
			printf("\n");
	}
#endif

	if(memcmp(ptbl, ptbl_r, sizeof(struct ptable)) == 0) {
		*same = 1;
	}

	free(ptbl);
	free(ptbl_r);
	return 0;
}

int mmc_parts_format(void)
{
	if (!mmc_blkdev) {
		printf("mmc block device hasn't initialize\n");
		return -1;
	}

	return do_format();
}

int mmc_parts_init(void)
{
	struct mmc* mmc = NULL;

	/* We register only one device. So, the dev id is always 0 */
	mmc = find_mmc_device(CONFIG_MMC_DEV_NUM);
	if (!mmc) {
		printf("make_write_mbr_ebr: mmc device not found!!\n");
		return -1;
	}
	mmc_init(mmc);

	if (part_nums > EFI_ENTRIES) {
		printf("too many mmc parts\n");
		return -1;
	}


	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (!mmc_blkdev) {
		printf("%s: fastboot device %s not found\n",
						__func__, CONFIG_MMC_DEV_NAME);
		return -1;
	}

	return 0;
}

