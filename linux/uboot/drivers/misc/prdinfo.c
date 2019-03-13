#include <linux/types.h>
#include <common.h>
#include <command.h>
#include <errno.h>
#include <nand.h>
#include <pdl.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <mmc/mmcpart.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/mtdparts_def.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/prdinfo.h>

int nand_read_partition(const char *part_name, unsigned char *data,
		size_t size, size_t *actual_len);
int nand_write_partition(const char *part_name, unsigned char *data,
		size_t size);
int nand_erase_partition(const char *part_name);
int emmc_erase_partition(const char *part_name);
int emmc_read_partition(const char *part_name, unsigned char *data, size_t size,
			size_t *actual_len);
int emmc_write_partition(const char *part_name, unsigned char *data, size_t size);

#ifdef CONFIG_SYS_CACHELINE_SIZE
static unsigned char prdinfo_buf[PRDINFO_TOTAL_SIZE]
			__attribute__((__aligned__(CONFIG_SYS_CACHELINE_SIZE)));
#else
static unsigned char prdinfo_buf[PRDINFO_TOTAL_SIZE]
			__attribute__((__aligned__(32)));
#endif
int prdinfo_load(void)
{
	int ret = 0;
	static int loaded = 0;
	unsigned char *data = prdinfo_buf;
	struct prdinfo *info = (struct prdinfo *)data;
	size_t actual_sz = 0;
	size_t total_sz = PRDINFO_TOTAL_SIZE;
	uint32_t crc = 0, new_crc = 0;

	if(loaded)
		return 0;

	if (rda_media_get() == MEDIA_MMC)
		ret = emmc_read_partition(PRDINFO_PART_NAME, data,
						total_sz, &actual_sz);
	else
		ret = nand_read_partition(PRDINFO_PART_NAME, data,
						total_sz, &actual_sz);

	if(ret)
		return -1;

	/* crc verify */
	memcpy(&crc, &data[PRDINFO_CRC_OFFSET], sizeof(uint32_t));
	new_crc = crc32(0, data, total_sz - sizeof(uint32_t));

	/* if data is invalid, fill by zero */
	if(crc == 0 || crc == 0xffffffff || crc != new_crc)
		memset((void *)data, 0, total_sz);

	if(info->pdl_image_count > PDL_IMAGE_MAX_NUM)
		info->pdl_image_count = PDL_IMAGE_MAX_NUM;

	loaded = 1;
	return 0;
}

int prdinfo_save(void)
{
	int ret = 0;
	unsigned char *data = prdinfo_buf;
	size_t total_sz = PRDINFO_TOTAL_SIZE;
	uint32_t *crc_p;

	crc_p = (uint32_t *)&data[PRDINFO_CRC_OFFSET];
	*crc_p = crc32(0, data, total_sz - sizeof(uint32_t));

	if (rda_media_get() == MEDIA_MMC) {
		ret = emmc_write_partition(PRDINFO_PART_NAME, data, total_sz);
	} else {
		ret = nand_erase_partition(PRDINFO_PART_NAME);
		if(!ret)
			ret = nand_write_partition(PRDINFO_PART_NAME, data, total_sz);
	}

	return ret;
}

int prdinfo_update_all(char *buf, unsigned long sz)
{
	if(sz != PRDINFO_TOTAL_SIZE) {
		printf("%s: size %lu is invaild.\n", __func__, sz);
		return -1;
	}
	memcpy(prdinfo_buf, buf, sz);
	prdinfo_save();
	return 0;
}

int prdinfo_set_data(struct prdinfo *info)
{
	memcpy(prdinfo_buf, (void *)info, sizeof(struct prdinfo));
	prdinfo_save();
	return 0;
}

int prdinfo_get_data(struct prdinfo *info)
{
	if(prdinfo_load())
		return -1;
	memcpy((void *)info, prdinfo_buf, sizeof(struct prdinfo));
	return 0;
}

static int prdinfo_clear(void)
{
	memset(prdinfo_buf, 0, PRDINFO_TOTAL_SIZE);

	if (rda_media_get() == MEDIA_MMC)
		return emmc_erase_partition(PRDINFO_PART_NAME);
	else
		return nand_erase_partition(PRDINFO_PART_NAME);
}

void prdinfo_dump(void)
{
	int i;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	printf("product infomation:\n");
	printf("  Download      : %s\n", info->pdl_result ? "OK" : "None");
	for(i = 0; i < info->pdl_image_count; i++) {
		int r = info->pdl_images[i].result;
		printf("    %10s: %s\n", info->pdl_images[i].name,
				r ? "OK" : "None");
	}
	printf("  Calibration   : %s\n", info->cali_result ? "OK" : "None");
	printf("  Autocall Test : %s\n", info->autocall_result ? "OK" : "None");
	printf("  Factory Test  : %s\n", info->factorytest_result ? "OK" : "None");
	printf("  IMEI Write    : %s\n", info->imei_result ? "OK" : "None");
	printf("  PSN Write     : %s\n", info->psn_result ? "OK" : "None");
	printf("  MBSN Write    : %s\n", info->mbsn_result ? "OK" : "None");
	printf("  MACAddr Write : %s\n", info->macaddr_result ? "OK" : "None");
	printf("  BTAddr Write  : %s\n", info->btaddr_result ? "OK" : "None");
	printf("  FTM TAG       : %#x\n", info->factory_tag);
	printf("  Reserve Data  : ");
	for(i = 0; i < 10; i++) {
		printf("0x%x ", ((u32)info->reserve[i]) & 0xff);
	}
	printf("\n");
}

int prdinfo_set_pdl_result(uint8_t rt, uint32_t ftm_tag)
{
	int ret = 0;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	ret = prdinfo_load();
	if(!ret) {
		info->pdl_result = !!rt;
		info->factory_tag = ftm_tag ? FACTORY_TAG_NUM : 0;
		ret = prdinfo_save();
	}
	return ret;
}

int prdinfo_set_cali_result(uint8_t rt)
{
	int ret = 0;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	ret = prdinfo_load();
	if(!ret) {
		info->cali_result = !!rt;
		ret = prdinfo_save();
	}
	return ret;
}

int prdinfo_set_autocall_result(uint8_t rt)
{
	int ret = 0;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	ret = prdinfo_load();
	if(!ret) {
		info->autocall_result = !!rt;
		ret = prdinfo_save();
	}
	return ret;
}

int prdinfo_set_coupling_result(uint8_t rt)
{
	int ret = 0;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	ret = prdinfo_load();
	if(!ret) {
		info->reserve[0] = !!rt;
		ret = prdinfo_save();
	}
	return ret;
}

int prdinfo_set_factorytest_result(uint8_t rt)
{
	int ret = 0;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	ret = prdinfo_load();
	if(!ret) {
		info->factorytest_result = !!rt;
		ret = prdinfo_save();
	}
	return ret;
}

int prdinfo_init(char *image_list)
{
	int ret = 0;
	int i, len;
	char *buff, *p, *q;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	/* clear old data first */
	prdinfo_clear();

	len = strlen(image_list);
	if(len < 1) {
		printf("%s: name list is empty!\n", __func__);
		return -1;
	}

	buff = malloc(len + 1);
	if(!buff)
		return -1;
	memcpy(buff, image_list, len+1);

	/* get image names */
	p = buff;
	i = 0;
	while(1) {
		q = strstr(p, ",");
		if(q) *q = '\0';

		if(strlen(p) > 0) {
			strncpy(info->pdl_images[i++].name, p, PDL_IMAGE_NAME_LEN-1);
			if(i >= PDL_IMAGE_MAX_NUM) break;
		}

		if(!q) break;

		p = q + 1;
		if(*p == '\0') break;
	}
	info->pdl_image_count = i;

	/* write data to flash */
	ret = prdinfo_save();

	free(buff);
	return ret;

}

int prdinfo_set_pdl_image_download_result(char *image_name, uint8_t dl_rt)
{
	int ret = 0;
	int update = 0;
	int i;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	ret = prdinfo_load();
	if(ret)
		return ret;

	for(i = 0; i < info->pdl_image_count; i++) {
		if(strcmp(image_name, info->pdl_images[i].name) == 0) {
			if(info->pdl_images[i].result != !!dl_rt) {
				info->pdl_images[i].result = !!dl_rt;
				update = 1;
			}
			break;
		}
	}

	if(update) {
		ret = prdinfo_save();
		printf("part '%s', set download result '%s'\n", image_name,
				(!!dl_rt) ? "success" : "fail");
	}

	return ret;
}

uint32_t prdinfo_get_factory_tag(void)
{
	uint32_t tag = 0;
	struct prdinfo *info = (struct prdinfo *)prdinfo_buf;

	if(!prdinfo_load())
		tag = info->factory_tag;
	return tag;
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
#ifdef CONFIG_CMD_PRDINFO
int do_dump_prdinfo(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	prdinfo_dump();
	return CMD_RET_SUCCESS;
}

int do_rda_check_factory_tag(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if(prdinfo_get_factory_tag() == FACTORY_TAG_NUM){
		enum rda_bm_type bm;
		char str[2];

		bm = rda_bm_get();
		if(bm == RDA_BM_NORMAL) {
			rda_bm_set(RDA_BM_FACTORY);
			sprintf(str, "%1d", RDA_BM_FACTORY);
			setenv("bootmode", str);
			printf("Found factory tag, change bootmode to %1d\n", RDA_BM_FACTORY);
		}
	}
	return 0;
}

U_BOOT_CMD(prdinfo, 1, 1, do_dump_prdinfo,
	"dump product infomation", "");

U_BOOT_CMD(rdachkfactag ,    1,    1,     do_rda_check_factory_tag,
	"check FTM tag 0xFAC12345 in 'misc' part", "");

#endif
