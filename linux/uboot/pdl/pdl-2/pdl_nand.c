#include <common.h>
#include <asm/types.h>
#include "packet.h"
#include <linux/string.h>
#include <jffs2/load_kernel.h>
#include <nand.h>
#include <malloc.h>
#include <asm/arch/spl_board_info.h>
#include <asm/arch/mtdparts_def.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/factory.h>
#include <asm/arch/prdinfo.h>
#include "cmd_defs.h"
#include "pdl_debug.h"
#include "pdl.h"
#include "pdl_command.h"
#include "pdl_nand.h"
#include <linux/mtd/nand.h>
#include <mtd/nand/rda_nand.h>

#define MAX_PART_NAME	30

struct dl_file_info {
	struct mtd_info *nand;

	struct part_info *part;
	unsigned long start_offset; /* partition start offset on flash */
	unsigned long end_offset; /* partition end offset on flash */
	unsigned long write_offset; /* write offset on flash */

	unsigned char *download_bufp; /* download buffer pointer */
	unsigned long download_size; /* data size in download buffer */

	unsigned long total_size; /* image file size */
	unsigned long recv_size; /* received data size */

	char part_name[MAX_PART_NAME];
	char vol_name[MAX_PART_NAME];
};

static struct dl_file_info dl_file;
static unsigned char *download_buf;
static unsigned long download_max_size = (24 * 1024 *1024);

extern int mtdparts_init_default(void);
extern int mtdparts_init_from_ptbl(void);
extern int mtdparts_save_ptbl(int need_erase);

static int __erase_partition(nand_info_t *nand,
				struct part_info *part)
{
	nand_erase_options_t opts;
	int ret;

	pdl_info("%s: erase part '%s', offset 0x%llx size 0x%llx\n", __func__,
			part->name, part->offset, part->size);
	memset(&opts, 0, sizeof(opts));
	opts.offset = (loff_t) part->offset;
	opts.length = (loff_t) part->size;
	opts.jffs2 = 0;
	opts.quiet = 0;

	pdl_dbg("opts off  0x%08x\n", (uint32_t) opts.offset);
	pdl_dbg("opts size 0x%08x\n", (uint32_t) opts.length);
	ret = nand_erase_opts(nand, &opts);
	if (!ret) {
		part->dirty = 0;
	}
	return ret;
}

static int pdl_ubi_start_write(void)
{
	int ret = 0;

	/* prepart to write ubi volume image, system/vendor partition */
#ifdef MTDPARTS_UBI_DEF
	pdl_info("init ubi part '%s'\n", dl_file.part_name);
	ret = ubi_part_scan(dl_file.part_name);
	if(ret) {
		pdl_info("ubi init failed.\n");
		return OPERATION_FAILED;
	}

	ret = ubi_check_default_vols(MTDPARTS_UBI_DEF);
	if(ret) {
		pdl_info("ubi volumes check failed.\n");
		return OPERATION_FAILED;
	}

	ret = ubi_prepare_write_vol(dl_file.vol_name, dl_file.total_size);
#endif

	return ret;
}

typedef enum {
	PART_RAW = 0,
	PART_FACTORYDATA,
	PART_PRDINFO,
	PART_UBI_VOL,
} PART_TYPE;
static int update_part_type = PART_RAW;
int nand_data_start(const char *part_name, unsigned long file_size)
{
	struct part_info *part;
	struct mtd_device *dev;
	u8 part_num;
	int ret;

	update_part_type = PART_RAW;
	memset(&dl_file, 0, sizeof(dl_file));
	strncpy(dl_file.part_name, part_name, MAX_PART_NAME-1);
	strncpy(dl_file.vol_name, part_name, MAX_PART_NAME-1);

#ifdef MTDPARTS_UBI_DEF /* all ubi volumes are in one ubi part */
	if(strstr(MTDPARTS_UBI_DEF, part_name)) {
		strncpy(dl_file.part_name, MTDPARTS_UBI_PART_NAME, MAX_PART_NAME-1);
		strncpy(dl_file.vol_name, part_name, MAX_PART_NAME-1);
		pdl_info("'%s' is a ubi volume in '%s' part.\n",
						dl_file.vol_name, dl_file.part_name);
		update_part_type = PART_UBI_VOL;
	}
#endif

	ret = find_dev_and_part(dl_file.part_name, &dev, &part_num, &part);
	if (ret) {
		int i;
		pdl_info("invalid mtd part:%s  ", dl_file.part_name);
		for (i = 0; i < MAX_PART_NAME; i++)
			pdl_info(" 0x%x ", dl_file.part_name[i]);
		pdl_info("\n");
		return (INVALID_PARTITION);
	}

	pdl_info("found part '%s' offset: 0x%llx length: 0x%llx id: %d is %s\n",
		  part->name, part->offset, part->size, dev->id->num,
		  part->dirty ? "dirty" : "clean");
	dl_file.part = part;
	dl_file.end_offset = part->offset + part->size;
	dl_file.nand = &nand_info[dev->id->num];

	if (file_size > part->size) {
		pdl_info("%s, download file too large, the size is:%ld\n",
			__func__, file_size);
		pdl_info("but the mtd part %s 's size is %llu\n",
			part->name, part->size);
		return INVALID_SIZE;
	}

	if (!strcmp(part->name, "factorydata")) {
		pdl_dbg("update factorydata\n");
		update_part_type = PART_FACTORYDATA;
	} else if (!strcmp(part->name, PRDINFO_PART_NAME)) {
		pdl_dbg("update prdinfo\n");
		update_part_type = PART_PRDINFO;
	}
	//erase the mtd part;
	if (part->dirty
			&& (update_part_type == PART_RAW)
		) {
		ret = __erase_partition(dl_file.nand, part);
		if (ret) {
			pdl_info("erase mtd partitions error\n");
			return DEVICE_ERROR;
		}
	}
	/*now we'll write data to the partition, so dirty the part */
	part->dirty = 1;

	dl_file.total_size = file_size;
	dl_file.start_offset = part->offset;
	dl_file.write_offset = part->offset;
	dl_file.download_bufp = download_buf;
	dl_file.recv_size = 0;
	dl_file.download_size = 0;

	if((update_part_type == PART_UBI_VOL) && pdl_ubi_start_write()) {
		pdl_error("can't write ubi volume %s\n", dl_file.vol_name);
		return DEVICE_ERROR;
	}

	return 0;
}

#ifdef _TGT_AP_DDR_AUTO_CALI_ENABLE
static void add_ddr_cal_val_to_bootloader(void)
{
	struct nand_chip *chip = dl_file.nand->priv;
	struct rda_nand_info *info = chip->priv;
	unsigned int cal_addr_flag_offs;
	spl_bd_t * spl_board_info = (spl_bd_t *)CONFIG_SPL_BOARD_INFO_ADDR;
	u16 ddr_cal[2] = {0};
	if (strcmp(dl_file.part->name, "bootloader") == 0){
		if (spl_board_info->spl_ddr_cal_info.ddr_auto_cal_flag == CONIFG_DDR_CAL_VAL_FLAG)
			ddr_cal[0] = spl_board_info->spl_ddr_cal_info.ddr_auto_cal_flag;
			ddr_cal[1] = spl_board_info->spl_ddr_cal_info.ddr_auto_cal_val[0];
			/*SPL is 2K in every nand page, hardware limit in 8810 chip*/
			cal_addr_flag_offs = (((spl_board_info->spl_ddr_cal_info.ddr_auto_cal_offs / 2048)
							* 2048) * info->spl_adjust_ratio) / 2  +
							(spl_board_info->spl_ddr_cal_info.ddr_auto_cal_offs % 2048);
			memcpy((void *)(download_buf + cal_addr_flag_offs),
				 (void *)ddr_cal, 4);
	}
}
#endif

static int pdl_nand_data_write(int data_end)
{
	unsigned int bad_blocks = 0;
	unsigned long write_bytes = dl_file.download_size;
	int ret = 0;

	pdl_info("flash data to '%s', size %lu bytes.\n",
			dl_file.vol_name, write_bytes);

	/* write ubi volume image, system/vendor partition */
	if(update_part_type == PART_UBI_VOL) {
#ifdef MTDPARTS_UBI_DEF
		ret = ubi_do_write_vol(dl_file.vol_name, download_buf, dl_file.download_size);

		/* clear download buffer position & size */
		dl_file.download_bufp = download_buf;
		dl_file.download_size = 0;

		if(data_end)
			ubi_finish_write_vol(dl_file.vol_name);
#endif

	} else if (update_part_type == PART_FACTORYDATA) {
		ret = factory_update_all(download_buf, write_bytes);
	} else if (update_part_type == PART_PRDINFO) {
		ret = prdinfo_update_all((char *)download_buf, write_bytes);
	} else {

#ifdef _TGT_AP_DDR_AUTO_CALI_ENABLE
		/* add DDR calibration value to bootloader image, it's a hack, FIXME */
		add_ddr_cal_val_to_bootloader();
#endif

		ret = nand_write_skip_bad_new(dl_file.nand, dl_file.write_offset,
				(size_t *)&write_bytes,
				dl_file.end_offset,
				download_buf,
				0, &bad_blocks);

		/* clear download buffer position & size */
		dl_file.download_bufp = download_buf;
		dl_file.download_size = 0;
		/* update write offset */
		dl_file.write_offset += write_bytes;
		dl_file.write_offset += bad_blocks * dl_file.nand->erasesize;

	}
	if (ret) {
		pdl_error("oops, flash to nand error %d\n", ret);
		return OPERATION_FAILED;
	}

	return 0;
}

int nand_data_midst(u8 *data, size_t size)
{
	int ret;
	extern uint8_t pdl_extended_status;

	if (!size) {
		pdl_error("oops, send the zero length packet\n");
		return INVALID_SIZE;
	}
	if (((dl_file.recv_size + size) > dl_file.total_size)) {
		pdl_error("transfer size error receive (%lu), file (%lu)\n",
			dl_file.recv_size + size, dl_file.total_size);
		return INVALID_SIZE;
	}

	/* if buffer is full */
	if((dl_file.download_size + size) > download_max_size) {
		if (pdl_extended_status)
			pdl_send_rsp(ACK_AGAIN_FLASH);
		ret = pdl_nand_data_write(0);
		if(ret) {
			pdl_info("ERROR: '%s' partition write failed.\n", dl_file.part_name);
			return ret;
		}
	}

	pdl_dbg("writing 0x%x bytes to offset: 0x%08lx\n",
		size,  (unsigned long)dl_file.download_bufp);

	/* save data to buffer */
	memcpy(dl_file.download_bufp, data, size);

	dl_file.download_bufp += size;
	dl_file.download_size += size;
	dl_file.recv_size += size;

	return 0;
}

static int pdl_nand_data_verify(uint32_t crc)
{
	unsigned int bad_blocks = 0;
	unsigned long read_bytes = 0;
	unsigned long total_size = dl_file.recv_size;
	unsigned long read_offset = 0;
	uint32_t new_crc = 0;
	int ret = 0;

	if(!crc)
		return 0;

	/* read back data from nand, for crc verify */
	pdl_info("Read back data from flash to do CRC verify.\n");

	if(update_part_type == PART_UBI_VOL) {
#ifdef MTDPARTS_UBI_DEF
		while(1) {
			read_bytes = (total_size > download_max_size) ? download_max_size : total_size;
			ret = ubi_read_vol(dl_file.vol_name, read_offset, download_buf, read_bytes);
			if(ret)
				break;

			new_crc = crc32(new_crc, download_buf, read_bytes);

			if(read_bytes >= total_size)
				break;
			total_size -= read_bytes;
			read_offset += read_bytes;
		}
#endif

	} else if (update_part_type == PART_RAW) {
		read_offset = dl_file.start_offset;
		while(1) {
			read_bytes = (total_size > download_max_size) ? download_max_size : total_size;
			ret = nand_read_skip_bad_new(dl_file.nand, read_offset,
						(size_t *)&read_bytes, download_buf, &bad_blocks);
			if(ret)
				break;

			new_crc = crc32(new_crc, download_buf, read_bytes);

			if(read_bytes >= total_size)
				break;
			total_size -= read_bytes;
			read_offset += read_bytes + (bad_blocks * dl_file.nand->erasesize);
		}

	} else {
		return 0;
	}

	if (ret) {
		pdl_error("oops, read data from nand error %d\n", ret);
		return OPERATION_FAILED;
	}

	if(crc != new_crc) {
		pdl_info("CRC verify failed, expect %#x, got %#x.\n", crc, new_crc);
		return CHECKSUM_ERROR;
	} else {
		pdl_info("CRC verify success, %#x.\n", crc);
	}

	return 0;
}

int nand_data_end(uint32_t crc)
{
	int ret;

	/* force to verify CRC, ignore bootloader/factorydata */
	if ((strcmp(dl_file.part->name, "bootloader") == 0 ||
		 strcmp(dl_file.part->name, "factorydata") == 0) ||
		 strcmp(dl_file.part->name, PRDINFO_PART_NAME) == 0) {
		crc = 0;
	} else if(!crc) {
		pdl_info("ERROR: CRC checking is necessary!");
		pdl_info("ERROR: please use the new version of download tools.\n");
		return CHECKSUM_ERROR;
	}

	ret = pdl_nand_data_write(1);
	if(ret) {
		pdl_info("ERROR: '%s' partition write failed.\n", dl_file.part_name);
		return ret;
	}

	ret = pdl_nand_data_verify(crc);
	if(ret) {
		pdl_info("ERROR: '%s' partition data verify failed.\n", dl_file.part_name);
		return ret;
	}

	pdl_info("END: total flashed %ld to nand\n", dl_file.recv_size);

	dl_file.download_bufp = download_buf;
	dl_file.download_size = 0;
	update_part_type = PART_RAW;

	/* the mtd partition table is saved in 'bootloader' partition,
	 * if 'bootloader' is updated, need to re-write the partition table. */
	if(strcmp(dl_file.part->name, "bootloader") == 0) {
		mtdparts_save_ptbl(1);
	}

	prdinfo_set_pdl_image_download_result(dl_file.part->name, 1);
	return 0;
}


int nand_read_partition(const char *part_name, u8 *data, size_t size,
			size_t *actual_len)
{
	struct part_info *part = NULL;
        u8 part_num;
	struct mtd_info *nand = NULL;
	struct mtd_device *dev;
	int ret;

	if(size > download_max_size) {
		pdl_info("can't read partition '%s', size is too large\n", part_name);
		return OPERATION_FAILED;
	}

#ifdef MTDPARTS_UBI_DEF /* all ubi volumes are in one ubi part */
	if(strstr(MTDPARTS_UBI_DEF, part_name)) {
		size_t vol_size = ubi_sizeof_vol(part_name);
		size = (size > vol_size) ? vol_size: size;
		pdl_info("'%s' is a ubi volume in '%s' part, of size %zu.\n",
			 part_name, MTDPARTS_UBI_PART_NAME, vol_size);
		ret = ubi_part_scan(MTDPARTS_UBI_PART_NAME);
		if(ret) {
			pdl_info("ubi init failed.\n");
			return OPERATION_FAILED;
		}

		ret = ubi_read_vol(part_name, 0, data, size);
		if(ret) {
			pdl_info("read ubi volume error: %s\n", part_name);
			return DEVICE_ERROR;
		}
		if(actual_len)
			*actual_len = size;
		pdl_info("read volume %s ok, %u bytes\n", part_name, size);
		return 0;
	}
#endif

	ret = find_dev_and_part(part_name, &dev, &part_num, &part);
	if (ret) {
		pdl_error("%s: invalid mtd part '%s'\n",__func__, part_name);
		return INVALID_PARTITION;
	}

	pdl_info("%s: read %#x bytes from '%s'\n", __func__, size, part_name);
	nand = &nand_info[dev->id->num];
	pdl_dbg("found part '%s' offset: 0x%llx length: %d/%llu id: %d\n",
		  part->name, part->offset, size, part->size, dev->id->num);

	size = (size < part->size) ? size : part->size;

	if (!strcmp(part_name, "factorydata")) {
		size = factory_get_all(data);
		if (size <= 0 ) {
			return DEVICE_ERROR;
		}
	} else {
		ret = nand_read_skip_bad(nand, part->offset, &size,
			(u_char *)data);
		if(ret < 0) {
			pdl_error("%s: read from nand error\n", __func__);
			return DEVICE_ERROR;
		}
	}
	if(actual_len)
		*actual_len = size;

	pdl_dbg("%s: done!", __func__);
	return 0;
}

int nand_write_partition(const char *part_name, unsigned char *data, size_t size)
{
	struct part_info *part = NULL;
	u8 part_num;
	struct mtd_info *nand = NULL;
	struct mtd_device *dev;
	int ret;
	u32 bad_blocks = 0;

	ret = find_dev_and_part(part_name, &dev, &part_num, &part);
	if (ret) {
		pdl_error("%s: invalid mtd part '%s'\n",__func__, part_name);
		return INVALID_PARTITION;
	}

	pdl_info("%s: write %#x bytes to '%s'\n", __func__, size, part_name);
	nand = &nand_info[dev->id->num];
	pdl_dbg("found part '%s' offset: 0x%llx length: %d/%llu id: %d\n",
		  part->name, part->offset, size, part->size, dev->id->num);

	size = (size < part->size) ? size : part->size;

	if (!strcmp(part_name, "factorydata")) {
		return factory_update_all(data, size);
	} else {
		ret = nand_write_skip_bad_new(nand, part->offset, &size,
			part->offset + part->size,
			(u_char *)data, 0, &bad_blocks);
		part->dirty = 1;
		if(ret < 0) {
			pdl_error("%s: write to nand error.\n", __func__);
			return DEVICE_ERROR;
		}
	}

	pdl_dbg("%s: done!\n", __func__);
	return 0;
}

extern struct mtd_device *current_mtd_dev;

extern int mtdparts_ptbl_check_factorydata(int *same);
int nand_format_flash(void)
{
	nand_erase_options_t opts;
	nand_info_t *nand;
	u8 nand_num= 0;
	int same = 0;

	/* re-init partition table by default */
	if(mtdparts_init_default()) {
		pdl_info("mtdparts init failed, abandon flash format..\n");
		return DEVICE_ERROR;
	}

	pdl_info("format the whole flash part\n");
	if (!current_mtd_dev) {
		pdl_info("%s mtd device info error\n", __func__);
		return DEVICE_ERROR;
	}
	/*
	 *erase all flash parts
	 */
	nand_num = current_mtd_dev->id->num;
	nand = &nand_info[nand_num];
	if (!nand) {
		pdl_info("%s mtd device info error\n", __func__);
		return DEVICE_ERROR;
	}
	memset(&opts, 0, sizeof(opts));
	opts.offset = 0;
	opts.length = (loff_t) nand->size;
	opts.jffs2 = 0;
	opts.quiet = 0;
	opts.scrub = 1;

	/* if partition bootloader & factorydata is not changed, don't erase them */
	mtdparts_ptbl_check_factorydata(&same);
	if(same) {
		struct part_info *part;
		struct mtd_device *dev;
		u8 part_num;
		int ret = find_dev_and_part("factorydata", &dev, &part_num, &part);
		if (!ret) {
			opts.offset = part->offset + part->size;
			opts.length -= opts.offset;
		}
	}
	factory_load();

	pdl_dbg("opts off  0x%08x\n", (uint32_t) opts.offset);
	pdl_dbg("opts size 0x%08x\n", (uint32_t) opts.length);
	pdl_dbg("nand write size 0x%08x\n", nand->writesize);
	pdl_info("erase 0x%llx bytes to '%s' offset: 0x%llx\n",
		  opts.length, nand->name, opts.offset);
	nand_erase_opts(nand, &opts);
	mtdparts_clear_all_dirty(current_mtd_dev);

	/* enable dirty for bootloader & factorydata part, if need */
	if(opts.offset > 0) {
		struct part_info *part;
		struct mtd_device *dev;
		u8 part_num;
		int ret = find_dev_and_part("bootloader", &dev, &part_num, &part);
		if (!ret) {
			part->dirty = 1;
		}
		ret = find_dev_and_part("factorydata", &dev, &part_num, &part);
		if (!ret) {
			part->dirty = 1;
		}
	}

	/* if 'factorydata' was erased, write back */
	if (opts.offset <= 0)
		factory_burn();

	return 0;
}

int nand_erase_partition(const char *part_name)
{
	struct part_info *part;
	struct mtd_device *dev;
	u8 part_num;
	int ret;
	struct mtd_info *nand;

#ifdef MTDPARTS_UBI_DEF /* all ubi volumes are in one ubi part */
	if(strstr(MTDPARTS_UBI_DEF, part_name)) {
		pdl_info("'%s' is a ubi volume in '%s' part, erase volume.\n",
						part_name, MTDPARTS_UBI_PART_NAME);
		ret = ubi_part_scan(MTDPARTS_UBI_PART_NAME);
		if(ret) {
			pdl_info("ubi init failed.\n");
			return OPERATION_FAILED;
		}

		ret = ubi_erase_vol((char *)part_name);
		if(ret) {
			pdl_info("erase ubi volume error: %s\n", part_name);
			return DEVICE_ERROR;
		}
		pdl_info("erase ubi volume ok: %s\n", part_name);
		return 0;
	}
#endif

	ret = find_dev_and_part(part_name, &dev, &part_num, &part);
	if (ret) {
		pdl_error("%s: invalid mtd part '%s'\n", __func__, part_name);
		return INVALID_PARTITION;
	}

	pdl_dbg("%s: erase part '%s'\n", __func__, part_name);
	pdl_dbg("found part '%s' offset: 0x%llx length: 0x%llx id: %d\n",
		  part->name, part->offset, part->size, dev->id->num);
	nand = &nand_info[dev->id->num];

	//erase the mtd part;
	ret = __erase_partition(nand, part);
	if (ret) {
		pdl_error("%s: erase part error\n", __func__);
		return DEVICE_ERROR;
	}

	pdl_dbg("%s: done!", __func__);
	return 0;
}

int pdl_nand_init(void)
{
	int ret;
	download_buf = (unsigned char *)SCRATCH_ADDR;
	download_max_size = 24*1024*1024;//FB_DOWNLOAD_BUF_SIZE;
	pdl_dbg("download buffer %p, max size %ld\n", download_buf, download_max_size);

	ret = mtdparts_init_from_ptbl();
	/* if fail to load partition table, try default */
	if(ret) {
		ret = mtdparts_init_default();
	}
	if(ret) {
		return DEVICE_ERROR;
	}

	/* check page size */
#ifdef NAND_PAGE_SIZE
	if(current_mtd_dev) {
		nand_info_t *nand = &nand_info[current_mtd_dev->id->num];
		if(nand->writesize != NAND_PAGE_SIZE) {
			pdl_info("nand page size is %#x, expect %#x, not compatible.\n",
					nand->writesize, NAND_PAGE_SIZE);
			return DEVICE_INCOMPATIBLE;
		}
	}
#endif

	return 0;
}

