#include <common.h>
#include <asm/types.h>
#include "packet.h"
#include <linux/string.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <mmc/sparse.h>
#include <mmc/mmcpart.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/factory.h>
#include <asm/arch/prdinfo.h>
#include "cmd_defs.h"
#include "pdl_debug.h"
#include "pdl.h"
#include "pdl_command.h"

#define MAX_PART_NAME	30
struct dl_file_info {
	disk_partition_t *ptn;
	uchar *start_addr;
	u64 total_size;
	u64 recv_size;
	char part_name[MAX_PART_NAME];
};

static struct dl_file_info dl_file;

static uchar *download_buf;
static unsigned long download_max_size;
static block_dev_desc_t *mmc_blkdev = NULL;

int emmc_data_start(const char *part_name, unsigned long file_size)
{
	disk_partition_t *ptn = 0;
	int blksz_shift;

	ptn = partition_find_ptn(part_name);
	if(!ptn) {
		/* init/update partition table */
		mmc_parts_format();
		/* read again */
		ptn = partition_find_ptn(part_name);
		if(!ptn) {
			int i;
			pdl_info("invalid mtd part:%s  ",part_name);
			for (i = 0; i < MAX_PART_NAME; i++)
				pdl_info(" 0x%x ", part_name[i]);
			pdl_info("\n");
			return INVALID_PARTITION;
		}
	}

	blksz_shift = LOG2(ptn->blksz);
	pdl_info("found part '%s' start: 0x%lx length: 0x%lx bytes\n",
		  ptn->name, ptn->start, (ptn->size << blksz_shift));

	memset(&dl_file, 0, sizeof(dl_file));
	strncpy(dl_file.part_name, part_name, MAX_PART_NAME);

	if (file_size > (ptn->size << blksz_shift)) {
		pdl_info("%s, download file too large, the size is:%ld\n",
			__func__, file_size);
		pdl_info("but the mmc part %s 's size is %ld\n",
			ptn->name, ptn->size << blksz_shift);
		return INVALID_SIZE;
	}

	dl_file.ptn = ptn;
	dl_file.total_size = file_size;
	dl_file.start_addr = download_buf;
	dl_file.recv_size = 0;

	return 0;
}

int emmc_data_midst(u8 *data, size_t size)
{
	if (!size) {
		pdl_error("oops, send the zero length packet\n");
		return INVALID_SIZE;
	}
	if (((dl_file.recv_size + size) > dl_file.total_size) ||
		(dl_file.recv_size + size) > download_max_size) {
		pdl_error("transfer size error receive (%lld), file (%lld)\n",
			dl_file.recv_size + size, dl_file.total_size);
		return INVALID_SIZE;
	}

	pdl_dbg("writing 0x%x bytes to offset: 0x%p\n",
		size,  dl_file.start_addr);

	memcpy(dl_file.start_addr, data, size);

	dl_file.start_addr += size;
	dl_file.recv_size += size;

	return 0;
}

int emmc_data_end(uint32_t crc)
{
	sparse_header_t *sparse_header = (sparse_header_t *)download_buf;
	disk_partition_t *ptn = dl_file.ptn;
	loff_t write_bytes = dl_file.recv_size;
	int ret;

	if (!strcmp(dl_file.part_name, "factorydata")) {
		pdl_info("flash to mmc part %s\n", ptn->name);
		ret = factory_update_all(download_buf, dl_file.recv_size);
		if (ret) {
			pdl_error("oops, flash to mmc error %d\n", ret);
			return OPERATION_FAILED;
		}
		return 0;
	}

	if (!strcmp(dl_file.part_name, PRDINFO_PART_NAME)) {
		pdl_info("flash to mmc part %s\n", ptn->name);
		ret = prdinfo_update_all((char *)download_buf, dl_file.recv_size);
		if (ret) {
			pdl_error("oops, flash to mmc error %d\n", ret);
			return OPERATION_FAILED;
		}
		return 0;
	}

	/* verify data crc before write to mmc */
	if(crc) {
		uint32_t new_crc = crc32(0, (unsigned char *)download_buf, write_bytes);
		if(crc != new_crc) {
			pdl_info("CRC verify failed, expect %#x, got %#x.\n", crc, new_crc);
			return CHECKSUM_ERROR;
		} else {
			pdl_info("CRC verify success, %#x.\n", crc);
		}
	}

	/* Using to judge if ext4 file system */
	if (sparse_header->magic != SPARSE_HEADER_MAGIC) {
		pdl_info("flash to mmc part %s\n", ptn->name);
		ret = partition_write_bytes(mmc_blkdev, ptn,
			&write_bytes, download_buf);
	} else {
		pdl_info("unsparese flash part %s start %lx, size %ld blks\n",
				ptn->name, ptn->start, ptn->size);
		ret = partition_unsparse(mmc_blkdev, ptn, download_buf,
			ptn->start, ptn->size);
	}
	if (ret) {
		pdl_error("oops, flash to mmc error %d\n", ret);
		return OPERATION_FAILED;
	}

	prdinfo_set_pdl_image_download_result((char *)ptn->name, 1);
	pdl_info("END: total flashed %lld to mmc\n", write_bytes);
	return 0;
}


int emmc_read_partition(const char *part_name, unsigned char *data, size_t size,
			size_t *actual_len)
{
	disk_partition_t *ptn;
	loff_t read_bytes = size;
	int ret;

	if(!mmc_blkdev) {
		mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
		if (!mmc_blkdev)
			return DEVICE_ERROR;
	}

	ptn = partition_find_ptn(part_name);
	if(!ptn) {
		/* init/update partition table */
		mmc_parts_format();
		/* read again */
		ptn = partition_find_ptn(part_name);
		if(!ptn) {
			pdl_error("partition table doesn't exist");
			return INVALID_PARTITION;
		}
	}

	pdl_dbg("%s from %s\n", __func__, part_name);

	if (!strcmp(part_name, "factorydata")) {
		size = factory_get_all(data);
		if (size == 0) {
			pdl_error("read from emmc error\n");
			return DEVICE_ERROR;
		}
	} else  {
		ret = partition_read_bytes(mmc_blkdev, ptn, &read_bytes, data);
		if(ret < 0) {
			pdl_error("read from emmc error\n");
			return DEVICE_ERROR;
		}
	}
	if(actual_len)
		*actual_len = size;

	pdl_dbg("read actrual %d ok \n", *actual_len);
	return 0;
}

int emmc_write_partition(const char *part_name, unsigned char *data, size_t size)
{
	disk_partition_t *ptn;
	loff_t write_bytes = size;
	int ret;

	if(!mmc_blkdev) {
		mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
		if (!mmc_blkdev)
			return DEVICE_ERROR;
	}

	ptn = partition_find_ptn(part_name);
	if(!ptn) {
		/* init/update partition table */
		mmc_parts_format();
		/* read again */
		ptn = partition_find_ptn(part_name);
		if(!ptn) {
			pdl_error("partition table doesn't exist");
			return INVALID_PARTITION;
		}
	}

	pdl_dbg("%s write to %s\n", __func__, part_name);

	if (!strcmp(part_name, "factorydata")) {
		return factory_update_all(data, size);
	} else  {
		ret = partition_write_bytes(mmc_blkdev, ptn, &write_bytes, data);
		if(ret < 0) {
			pdl_error("write to emmc error\n");
			return DEVICE_ERROR;
		}
	}

	pdl_dbg("write actrual %u ok \n", size);
	return 0;
}

extern struct mtd_device *current_mtd_dev;

int emmc_format_flash(void)
{
	int ret;
	lbaint_t start = 0;
	lbaint_t count = 0;
	disk_partition_t *ptn_old, *ptn_new;

	if (!mmc_blkdev) {
		mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
		if (!mmc_blkdev)
			return DEVICE_ERROR;
	}
	count = mmc_blkdev->lba;

	ptn_old = partition_find_ptn("factorydata");
	factory_load();

	pdl_info("refresh partition table\n");
	ret = mmc_parts_format();
	if (ret) {
		pdl_error("refresh partition table fail %d\n", ret);
		return DEVICE_ERROR;
	}

	/* if 'factorydata' part is not changed, don't
	   erase '0' offset to the end offset of 'factorydata' */
	ptn_new = partition_find_ptn("factorydata");
	if (ptn_old && ptn_new) {
		if(ptn_old->start == ptn_new->start &&
			ptn_old->size == ptn_new->size)
			start = ptn_new->start + ptn_new->size;
	}

	count -= start;
	pdl_info("format the whole mmc, from %ld, %ld blocks\n", start, count);
	ret = mmc_blkdev->block_erase(CONFIG_MMC_DEV_NUM, start, count);
	if (ret != count) {
		pdl_error("mmc format partition fail %d\n", ret);
		return DEVICE_ERROR;
	}

	/* if 'factorydata' was erased, write back */
	if (!start)
		factory_burn();

	return 0;
}

int emmc_erase_partition(const char *part_name)
{
	disk_partition_t *ptn;
	int ret;

	if(!mmc_blkdev) {
		mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
		if (!mmc_blkdev)
			return DEVICE_ERROR;
	}

	ptn = partition_find_ptn(part_name);
	if(!ptn) {
		/* init/update partition table */
		mmc_parts_format();
		/* read again */
		ptn = partition_find_ptn(part_name);
		if(!ptn) {
			pdl_error("partition table doesn't exist");
			return INVALID_PARTITION;
		}
	}

	pdl_info("erase the mmc part %s\n", part_name);
	ret = partition_erase_blks(mmc_blkdev, ptn, &ptn->size);
	if (ret) {
		pdl_error("erase parttion fail\n");
		return DEVICE_ERROR;
	}
	return 0;
}

int pdl_emmc_init(void)
{
	download_buf = (unsigned char *)SCRATCH_ADDR;
	download_max_size = FB_DOWNLOAD_BUF_SIZE;
	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (!mmc_blkdev)
		return DEVICE_ERROR;
	pdl_dbg("download buffer %p, max size %ld\n", download_buf, download_max_size);
	return 0;
}

