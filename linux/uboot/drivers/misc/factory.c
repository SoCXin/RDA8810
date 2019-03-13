#include <linux/types.h>
#include <common.h>
#include <errno.h>
#include <nand.h>
#include <pdl.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <mmc/mmcpart.h>
#include <jffs2/load_kernel.h>
#include <asm/arch/rda_iomap.h>
#include <asm/arch/factory.h>
#include <asm/arch/rda_sys.h>

enum FACTORY_DATA_STATUS {
	FACTORY_DATA_UNKNOWN = 0,
	FACTORY_DATA_INVALID,
	FACTORY_DATA_VALID,
};

struct ap_factory_config_v0 {
	u32 magic;
	u8 lcd_name[128];
	u8 clock_config[1024];
	u8 reserved[1024];
};

const static char factory_sector_name[] = "factorydata";

static enum FACTORY_DATA_STATUS factory_data_status
	= FACTORY_DATA_UNKNOWN;

static struct factory_data_sector factory_data
	__attribute__((__aligned__(32)));

static int factory_data_is_valid(u8 *data, u32 size)
{
	struct factory_data_sector *sector = (void *)data;
	struct ap_factory_config *config = (void *)sector->ap_factory_data;
	int valid = 0;
	u32 crc, crc_calc;
	u32 version;

	BUILD_BUG_ON(sizeof(struct ap_factory_config) > RDA_AP_FACT_LEN);

	memcpy(&version, &config->version, sizeof(version));
	printf("Factory version: 0x%08X\n", version);
	if ((version & 0xFFFF0000) == AP_FACTORY_MARK_VERSION) {
		u32 crc_size = RDA_FACT_TOTAL_LEN;
		/* Factory structure version the current version */
		if (version == AP_FACTORY_VERSION_NUMBER) {
			valid = 1;
		/* Factory structure version is old, but we can support it */
		} else if (version == AP_FACTORY_VERSION_1_NUMBER) {
			crc_size = AP_FACTORY_VERSION_1_LEN;
			valid = 1;
		} else {
			/* Factory structure version is invalid */
			printf("Factory version is unknown\n");
			printf("Current code version: 0x%08X\n",
					AP_FACTORY_VERSION_NUMBER);
		}

		if(size < crc_size) {
			valid = 0;
			printf("Factory data size is too small.\n");
		}

		if(valid) {
			memcpy(&crc, &config->crc, sizeof(crc));
			memset(&config->crc, 0, sizeof(crc));
			crc_calc = crc32(0, (u8 *)sector, crc_size);
			memcpy(&config->crc, &crc, sizeof(crc));
			if (crc == crc_calc) {
				printf("Factory CRC is OK %#x\n", crc);
			} else {
				printf("ERROR: Bad factory CRC, got %#x, expect %#x\n",
								crc_calc, crc);
				valid = 0;
			}
		}
	} else {
		printf("No valid factory data\n");
	}

	return valid;
}

extern int find_dev_and_part(const char *id, struct mtd_device **dev,
			     u8 *part_num, struct part_info **part);

static int nand_mtd_factory_init(struct mtd_info **minfo,
		struct part_info **pinfo)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;
	u32 block_size, page_size;
	u32 block_nums;

	ret = find_dev_and_part(factory_sector_name, &dev, &pnum, &part);
	if (ret) {
		printf("unknown partition name '%s'\n", factory_sector_name);
		return ret;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		printf("mtd dev type error: %d\n", dev->id->type);
		return -EIO;
	}
	printf("found factorydata part '%s' offset: 0x%llx length: 0x%llx\n",
	       part->name, part->offset, part->size);

	nand = &nand_info[dev->id->num];
	block_size = nand->erasesize;
	page_size = nand->writesize;

	printf("nand info: blockSize=%d pageSize=%d\n", block_size, page_size);

	if (block_size < sizeof(struct factory_data_sector)) {
		printf("Invalid block size: %d (should >= %d)\n",
				block_size, sizeof(struct factory_data_sector));
		hang();
	}
	block_nums = part->size / nand->erasesize;
	if (block_nums < 2) {
		printf("partition for factorydata is too small");
		printf(" at least two blocks\n");
		hang();
	}
	*minfo = nand;
	*pinfo = part;
	return 0;
}

static int nand_mtd_load_factory(u8 *dst)
{
	struct mtd_info *nand;
	struct part_info *part;
	size_t size;
	int ret;
	u64 offs;
	u32 block_size;
	u32 bad_blocks = 0;

	ret = nand_mtd_factory_init(&nand, &part);
	if (ret)
		return ret;

	block_size = nand->erasesize;
	size = sizeof(struct factory_data_sector);
	offs = part->offset;

	printf("read 0x%x bytes from '%s' offset: 0x%llx\n",
	       size, part->name, offs);
	ret = nand_read_skip_bad_new(nand, offs, &size, dst, &bad_blocks);
	if (ret) {
		printf("nand read fail\n");
		return ret;
	}

	if (factory_data_is_valid(dst, size))
		return 0;

	printf("Try the backup copy of factory data\n");

	size = sizeof(struct factory_data_sector);
	offs = part->offset + (bad_blocks + 1) * block_size;

	printf("read 0x%x bytes from '%s' offset: 0x%llx\n",
	       size, part->name, offs);

	ret = nand_read_skip_bad_new(nand, offs, &size, dst, &bad_blocks);
	if (ret) {
		printf("nand read fail\n");
		return ret;
	}

	if (factory_data_is_valid(dst, size))
		return 0;

	return -EIO;
}

static void factory_data_prepare_write(u8 *buf)
{
	struct factory_data_sector *sector = (void *)buf;
	struct ap_factory_config *config = (void *)sector->ap_factory_data;
	u32 version = AP_FACTORY_VERSION_NUMBER;
	u32 crc_size = sizeof(*sector);
	u32 crc;

	if(config->version == AP_FACTORY_VERSION_1_NUMBER)
		crc_size = AP_FACTORY_VERSION_1_LEN;
	else
		memcpy(&config->version, &version, sizeof(config->version));

	memset(&config->crc, 0, sizeof(config->crc));
	crc = crc32(0, (u8 *)sector, crc_size);
	memcpy(&config->crc, &crc, sizeof(config->crc));
}

static void factory_data_dump(u8 *data, size_t size, int flash)
{
	printf("dump factory data from %s :\n", flash ? "flash" : "tools");
	rda_dump_buf((char *)data, size);
}

static int factory_data_check(struct mtd_info *nand, u32 offs, u8 *data)
{
	int ret = 0;;
	u8 *dst;
	size_t size;
	u32 bad_blocks = 0;
	int i;

	if (pdl_dbg_rw_check == 0 && pdl_dbg_factory_part == 0)
		return 0;

	printf("Check the factory data just written ...\n");

	dst = malloc(sizeof(struct factory_data_sector));
	if (dst == NULL) {
		printf("Failed to malloc buffer %d bytes\n",
				sizeof(struct factory_data_sector));
		return -ENOMEM;
	}

	size = sizeof(struct factory_data_sector);

	printf("read 0x%x bytes offset: 0x%08x\n", size, offs);
	ret = nand_read_skip_bad_new(nand, offs, &size, dst, &bad_blocks);
	if (ret) {
		printf("nand read fail\n");
		goto _exit;
	}

	if (pdl_dbg_factory_part)
		factory_data_dump(dst, sizeof(struct factory_data_sector), 1);

	if (pdl_dbg_rw_check) {
		for (i = 0; i < sizeof(struct factory_data_sector); ++i) {
			if (data[i] != dst[i])
				break;
		}
		if (i < sizeof(struct factory_data_sector)) {
			ret = -EIO;
			size = sizeof(struct factory_data_sector) - i;
			if (size > 2048)
				size = 2048;
			printf("*** CHECK FAILURE: offset=%d\n", i);
			printf("==> Before %p is:\n", data);
			rda_dump_buf((char *)&data[i], size);
			printf("==> After  %p is:\n", dst);
			rda_dump_buf((char *)&dst[i], size);
		}
	}

_exit:
	free(dst);
	return ret;
}

static loff_t factory_backup_offs = 0;

static int write_factory(nand_info_t *nand, loff_t offset, size_t size,
			 loff_t max_limit, u8 *buf)
{
	nand_erase_options_t opts;
	struct nand_chip *chip = nand->priv;
	int ret;

	if (chip->bbt_erase_shift == 0) {
		if(mtd_mod_by_eb(offset, nand) != 0)
			return -EINVAL;
	} else {
		if ((offset & (nand->erasesize -1)) != 0)
			return -EINVAL;
	}

	if (chip->bbt_erase_shift == 0) {
		while (offset < max_limit) {
			if (nand_block_isbad (nand,
		 		mtd_div_by_eb(offset, nand) * nand->erasesize)) {
				printf ("Skip bad block 0x%08llx\n",
		 			(loff_t)mtd_div_by_eb(offset, nand) * nand->erasesize);
				offset += nand->erasesize;
				continue;
			} else {
				break;
			}
		}
	} else {
		while (offset < max_limit) {
			if (nand_block_isbad (nand,
				offset & ~(nand->erasesize - 1))) {
				printf ("Skip bad block 0x%08llx\n",
					offset & ~(nand->erasesize - 1));
				offset += nand->erasesize;
				continue;
			} else {
				break;
			}
		}
	}

	if (offset >= max_limit) {
		printf("no good block for factory data\n");
		return -EIO;
	}
	/* erase the factory partition */
	memset(&opts, 0, sizeof(opts));
	opts.offset = offset;
	opts.length = nand->erasesize;
	opts.jffs2 = 0;
	opts.quiet = 0;
	printf("erase 0x%x bytes to  offset: 0x%08x\n",
		(unsigned int)opts.length,
		(unsigned int)opts.offset);
	ret = nand_erase_opts(nand, &opts);
	if (ret) {
		printf("nand erase fail\n");
		return ret;
	}

	ret = nand_write(nand, offset, &size, buf);
	if (ret) {
		printf("nand write fail\n");
		return ret;
	} else {
		/* Check the factory data in flash */
		ret = factory_data_check(nand, offset, buf);
		if (ret)
			return ret;
	}

	factory_backup_offs = offset + nand->erasesize;
	return 0;
}

static int nand_mtd_write_factory(u8 *buf)
{
	struct mtd_info *nand;
	struct part_info *part;
	int ret;
	size_t size;
	u64 offs;
	loff_t max_limit;

	ret = nand_mtd_factory_init(&nand, &part);
	if (ret)
		return ret;

	max_limit = part->offset + part->size;

	/* Dump the data from tools */
	if (pdl_dbg_factory_part)
		factory_data_dump(buf, sizeof(struct factory_data_sector), 0);

	/* Set version and crc */
	factory_data_prepare_write(buf);

	/* write back the factory */
	size = sizeof(struct factory_data_sector);
	offs = part->offset;
	printf("write 0x%x bytes to '%s' offset: 0x%llx\n",
		size, part->name, offs);

	ret = write_factory(nand, offs, size, max_limit, buf);
	if (ret) {
		printf("factorydata write fail\n");
		return ret;
	}

	printf("Write the backup copy of factory data\n");

	size = sizeof(struct factory_data_sector);
	offs = factory_backup_offs;
	printf("write 0x%x bytes to '%s' offset: 0x%llx\n",
		size, part->name, offs);

	ret = write_factory(nand, offs, size, max_limit, buf);
	if (ret) {
		printf("factorydata backup write fail, ");
		printf("and only one copy in factorydata\n");
	}
	return 0;
}

#define BACKUP_FACTORY_START	128	/* 128 mmc blocks, 64KB */
int emmc_load_factory(u8 *dst)
{
	disk_partition_t *ptn;
	block_dev_desc_t *mmc_blkdev;
	struct mmc *mmc;
	u64 offset;
	u64 backup_offset;
	size_t size;
	int blksz_shift;

	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (mmc_blkdev)
		mmc = container_of(mmc_blkdev, struct mmc, block_dev);
	else
		return -1;

	if (!mmc) {
		printf("mmc doesn't exist");
		return -1;
	}

	ptn = partition_find_ptn("factorydata");
	if(!ptn) {
		printf("mmc partition table doesn't exist");
		return -1;
	}

	blksz_shift = LOG2(ptn->blksz);
	offset = (u64)ptn->start << blksz_shift;
	size = sizeof(struct factory_data_sector);
	printf("read 0x%x bytes from '%s' offset: 0x%08llx\n",
	       size, ptn->name, offset);
	if (mmc_read(mmc, offset, dst, size) <= 0) {
		printf("mmc read fail\n");
		return -EIO;
	}

	if (factory_data_is_valid(dst, size))
		return 0;

	printf("Try the backup copy of factory data\n");

	size = sizeof(struct factory_data_sector);
	backup_offset = offset + (BACKUP_FACTORY_START << blksz_shift);

	printf("read 0x%x bytes from '%s' offset: 0x%08llx\n",
	       size, ptn->name, backup_offset);

	if (mmc_read(mmc, backup_offset, dst, size) <= 0) {
		printf("mmc read backup factorydata fail\n");
		return -EIO;
	}

	if (factory_data_is_valid(dst, size))
		return 0;

	return -EIO;
}

static int emmc_write_factory(u8 *buf)
{
	disk_partition_t *ptn;
	block_dev_desc_t *mmc_blkdev;
	struct mmc *mmc;
	u64 offset;
	u64 backup_offset;
	size_t size;
	int blksz_shift;

	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (mmc_blkdev)
		mmc = container_of(mmc_blkdev, struct mmc, block_dev);
	else
		return -1;

	if (!mmc) {
		printf("mmc doesn't exist");
		return -1;
	}

	ptn = partition_find_ptn("factorydata");
	if (!ptn) {
		printf("mmc partition table doesn't exist");
		return -1;
	}

	/* Dump the data from tools */
	if (pdl_dbg_factory_part)
		factory_data_dump(buf, sizeof(struct factory_data_sector), 0);

	/* Set version and crc */
	factory_data_prepare_write(buf);

	/* write back the factory */
	size = sizeof(struct factory_data_sector);
	blksz_shift = LOG2(ptn->blksz);
	offset = (u64)ptn->start << blksz_shift;
	printf("write 0x%x bytes to '%s' offset: 0x%llx\n",
		size, ptn->name, offset);

	if (mmc_write(mmc, offset, buf, size) <= 0) {
		printf("factorydata write fail\n");
		return -EIO;
	}

	printf("Write the backup copy of factory data\n");

	size = sizeof(struct factory_data_sector);
	backup_offset = offset + (BACKUP_FACTORY_START << blksz_shift);
	printf("write 0x%x bytes to '%s' offset: 0x%llx\n",
		size, ptn->name, backup_offset);

	if (mmc_write(mmc, backup_offset, buf, size) <= 0) {
		printf("factorydata backup write fail, ");
		printf("and only one copy in factorydata\n");
	}
	return 0;
}

int factory_copy_from_mem(const u8 *buf)
{
	u8 *data = (u8 *)image_get_data((const image_header_t *)buf);
	size_t size = image_get_data_size((const image_header_t *)buf);

	if (factory_data_is_valid(data, size)) {
		printf("Calibration data is valid, copying to memory\n");
		memcpy(&factory_data, data, sizeof(factory_data));
		factory_data_status = FACTORY_DATA_VALID;
		return 0;
	} else {
		printf("Calibration data is invalid\n");
		return 1;
	}
}

int factory_load(void)
{
	int ret = 0;

	if (factory_data_status != FACTORY_DATA_UNKNOWN) {
		printf("Factory already loaded\n");
		return ret;
	}

	printf("Load factory from flash ...\n");

	if (rda_media_get() == MEDIA_MMC)
		ret = emmc_load_factory((u8 *)&factory_data);
	else
		ret = nand_mtd_load_factory((u8 *)&factory_data);
	if (ret) {
		printf("Load Failed\n");
		memset(&factory_data, 0, sizeof(factory_data));
		factory_data_status = FACTORY_DATA_INVALID;
		return ret;
	}

	printf("Done\n");
	factory_data_status = FACTORY_DATA_VALID;
	return ret;
}

const unsigned char* factory_get_lcd_name(void)
{
	struct ap_factory_config *config =
		(void *)factory_data.ap_factory_data;
	factory_load();
	return config->lcd_name;
}

const unsigned char* factory_get_bootlogo_name(void)
{
	struct ap_factory_config *config =
		(void *)factory_data.ap_factory_data;
	factory_load();
	return config->bootlogo_name;
}

const unsigned char* factory_get_ap_factory(void)
{
	factory_load();
	return factory_data.ap_factory_data;
}

const unsigned char* factory_get_modem_factory(void)
{
	factory_load();
	return factory_data.modem_factory_data;
}

const unsigned char* factory_get_modem_calib(void)
{
	factory_load();
	return factory_data.modem_calib_data;
}

const unsigned char* factory_get_modem_ext_calib(void)
{
	factory_load();
	return factory_data.modem_ext_calib_data;
}

unsigned long factory_get_all(unsigned char *buf)
{
	struct ap_factory_config *config =
		(void *)factory_data.ap_factory_data;
	if (!buf)
		return 0;

	factory_load();
	memcpy(buf, &factory_data, sizeof(factory_data));

	if(config->version == AP_FACTORY_VERSION_1_NUMBER)
		return AP_FACTORY_VERSION_1_LEN;

	return sizeof(factory_data);
}

int factory_set_ap_factory(unsigned char *data)
{
	factory_load();
	memcpy(factory_data.ap_factory_data, data,
			sizeof(factory_data.ap_factory_data));
	return 0;
}

int factory_set_modem_calib(unsigned char *data)
{
	factory_load();
	memcpy(factory_data.modem_calib_data, data,
			sizeof(factory_data.modem_calib_data));
	return 0;
}

int factory_set_modem_ext_calib(unsigned char *data)
{
	factory_load();
	memcpy(factory_data.modem_ext_calib_data, data,
			sizeof(factory_data.modem_ext_calib_data));
	return 0;
}

int factory_set_modem_factory(unsigned char *data)
{
	factory_load();
	memcpy(factory_data.modem_factory_data, data,
			sizeof(factory_data.modem_factory_data));
	return 0;
}

int factory_burn(void)
{
	if (rda_media_get() == MEDIA_MMC)
		return emmc_write_factory((u8 *)&factory_data);
	else
		return nand_mtd_write_factory((u8 *)&factory_data);
}

int factory_update_modem_calib(unsigned char *data)
{
	factory_set_modem_calib(data);
	return factory_burn();
}

int factory_update_modem_ext_calib(unsigned char *data)
{
	factory_set_modem_ext_calib(data);
	return factory_burn();
}

int factory_update_modem_factory(unsigned char *data)
{
	factory_set_modem_factory(data);
	return factory_burn();
}

int factory_update_ap_factory(unsigned char *data)
{
	factory_set_ap_factory(data);
	return factory_burn();
}

int factory_update_all(unsigned char *data, unsigned long size)
{
	if (!factory_data_is_valid(data, size)) {
		printf("factory data is invalid.\n");
		return -EINVAL;
	}
	factory_load();
	memcpy(&factory_data, data, sizeof(factory_data));
	return factory_burn();
}


/* message rx/tx for PC calib tool and u-boot */
int factory_get_ap_calib_msg(unsigned int *id, unsigned int *size, unsigned char *data)
{
	struct ap_calib_message *msg = (struct ap_calib_message *)RDA_AP_CALIB_MSG_ADDR;
	if (msg->magic == RDA_AP_CALIB_MSG_MAGIC &&
		!(msg->id & 0x80000000) ) {
		*id = msg->id;
		if (size)
			*size = msg->size;
		if (data)
			memcpy(data, msg->data, msg->size);
		/* clear */
		memset(msg, 0, RDA_AP_CALIB_MSG_LEN);
		flush_dcache_range(RDA_AP_CALIB_MSG_ADDR, RDA_AP_CALIB_MSG_ADDR+RDA_AP_CALIB_MSG_LEN);
		return 0;
	}
	return -1;
}

int factory_set_ap_calib_msg(unsigned int id, unsigned int size, unsigned char *data)
{
	struct ap_calib_message *msg = (struct ap_calib_message *)RDA_AP_CALIB_MSG_ADDR;
	msg->magic = RDA_AP_CALIB_MSG_MAGIC;
	msg->id = id;
	msg->size = size;
	if (data)
		memcpy(msg->data, data, size);
	flush_dcache_range(RDA_AP_CALIB_MSG_ADDR, RDA_AP_CALIB_MSG_ADDR+RDA_AP_CALIB_MSG_LEN);
	return 0;
}

