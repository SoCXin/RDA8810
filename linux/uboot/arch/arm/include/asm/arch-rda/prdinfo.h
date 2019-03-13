#ifndef __PRDINFO_H__
#define __PRDINFO_H__

#define PRDINFO_PART_NAME	"misc"
#define PRDINFO_TOTAL_SIZE	(16*1024)

/* 0 ~ 1024, produce infomation,
   includes pdl/calibration/test results */
#define PRDINFO_DATA_OFFSET		0
#define PRDINFO_DATA_MAX_SIZE	1024
#define PDL_IMAGE_MAX_NUM		16
#define PDL_IMAGE_NAME_LEN		31
#define FACTORY_TAG_NUM		0xFAC12345
struct prdinfo
{
	uint8_t pdl_result;
	uint8_t cali_result;
	uint8_t autocall_result;
	uint8_t factorytest_result;

	uint32_t pdl_image_count;
	struct {
		char name[PDL_IMAGE_NAME_LEN];
		uint8_t result;
	} pdl_images[PDL_IMAGE_MAX_NUM];

	uint32_t factory_tag; /* force to factory mode */

	uint8_t imei_result;
	uint8_t psn_result;
	uint8_t mbsn_result;
	uint8_t macaddr_result;
	uint8_t btaddr_result;

	uint8_t reserve[27];
};

/* 1024 ~ (PRDINFO_TOTAL_SIZE - 4), reserved */

/* (PRDINFO_TOTAL_SIZE - 4) ~ PRDINFO_TOTAL_SIZE,
   crc for all (PRDINFO_TOTAL_SIZE-4) data */
#define PRDINFO_CRC_OFFSET	(PRDINFO_TOTAL_SIZE-4)
#define PRDINFO_CRC_SIZE	4


/* related functions int u-boot */
int prdinfo_init(char *image_list);
int prdinfo_load(void);
int prdinfo_save(void);
int prdinfo_update_all(char *buf, unsigned long sz);
void prdinfo_dump(void);
int prdinfo_set_data(struct prdinfo *info);
int prdinfo_get_data(struct prdinfo *info);
int prdinfo_set_pdl_result(uint8_t rt, uint32_t ftm_tag);
int prdinfo_set_cali_result(uint8_t rt);
int prdinfo_set_autocall_result(uint8_t rt);
int prdinfo_set_coupling_result(uint8_t rt);
int prdinfo_set_factorytest_result(uint8_t rt);
int prdinfo_set_pdl_image_download_result(char *image_name, uint8_t dl_rt);
uint32_t prdinfo_get_factory_tag(void);

#endif
