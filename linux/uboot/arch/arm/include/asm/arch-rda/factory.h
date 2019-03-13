#ifndef __FACTORY_DATA_H__
#define __FACTORY_DATA_H__

#include "defs_mdcom.h"

/* data size define */
#define RDA_MODEM_CAL_LEN		8192 /* rf/audio calibration data */
#define RDA_MODEM_FACT_LEN		4096 /* modem factory info */
#define RDA_AP_FACT_LEN			4096 /* ap factory info */
#define RDA_MODEM_EXT_CAL_LEN	16384 /* extended rf/audio calibration data, includes wcdma data */
#define RDA_FACT_TOTAL_LEN		\
	(RDA_MODEM_CAL_LEN + RDA_MODEM_FACT_LEN + RDA_AP_FACT_LEN \
	 + RDA_MODEM_EXT_CAL_LEN)

/* loading addresses */
/* extended calibration data loading address */
#define RDA_MODEM_EXT_CAL_ADDR		(RDA_MODEM_RAM_END - RDA_FACT_TOTAL_LEN)
/* calibration data loading address */
#define RDA_MODEM_CAL_ADDR			(RDA_MODEM_EXT_CAL_ADDR + RDA_MODEM_EXT_CAL_LEN)
/* modem factory info loading address */
#define RDA_MODEM_FACT_ADDR			(RDA_MODEM_CAL_ADDR + RDA_MODEM_CAL_LEN)
/* ap factory info loading address */
#define RDA_AP_FACT_ADDR			(RDA_MODEM_FACT_ADDR + RDA_MODEM_FACT_LEN)

/* version define & magic number */
#define AP_FACTORY_MAJOR_VERSION	2
#define AP_FACTORY_MINOR_VERSION	1 /* always be 1 */
#define AP_FACTORY_MARK_VERSION		0xFAC40000
#define AP_FACTORY_VERSION_NUMBER	(AP_FACTORY_MARK_VERSION | \
		(AP_FACTORY_MAJOR_VERSION << 8) | AP_FACTORY_MINOR_VERSION)
#define AP_FACTORY_CLOCK_MAGIC		0x55515263

/* old version compatibility */
#define AP_FACTORY_MAJOR_VERSION_1				1
#define AP_FACTORY_VERSION_1_LEN			(RDA_MODEM_CAL_LEN + \
										RDA_MODEM_FACT_LEN + RDA_AP_FACT_LEN)
#define AP_FACTORY_VERSION_1_NUMBER			(AP_FACTORY_MARK_VERSION | \
						(AP_FACTORY_MAJOR_VERSION_1 << 8) | AP_FACTORY_MINOR_VERSION)

#define FACT_NAME_LEN				128
#define AP_FACTORY_CLOCK_CFG_LEN	1024
#define GS_CALI_DATA_LEN			32

struct ap_factory_config {
	unsigned int version;
	unsigned int crc;
	unsigned char lcd_name[FACT_NAME_LEN];
	unsigned char bootlogo_name[FACT_NAME_LEN];
	unsigned char clock_config[AP_FACTORY_CLOCK_CFG_LEN];
	unsigned char gs_cali_data[GS_CALI_DATA_LEN];
};

struct factory_data_sector {
	unsigned char modem_calib_data[RDA_MODEM_CAL_LEN];
	unsigned char modem_factory_data[RDA_MODEM_FACT_LEN];
	unsigned char ap_factory_data[RDA_AP_FACT_LEN];
	unsigned char modem_ext_calib_data[RDA_MODEM_EXT_CAL_LEN];
};

int factory_load(void);
unsigned long factory_get_all(unsigned char *buf);
const unsigned char* factory_get_ap_factory(void);
const unsigned char* factory_get_modem_calib(void);
const unsigned char* factory_get_modem_ext_calib(void);
const unsigned char* factory_get_modem_factory(void);
const unsigned char* factory_get_lcd_name(void);
const unsigned char* factory_get_bootlogo_name(void);
int factory_set_ap_factory(unsigned char *data);
int factory_set_modem_calib(unsigned char *data);
int factory_set_modem_ext_calib(unsigned char *data);
int factory_set_modem_factory(unsigned char *data);
int factory_burn(void);
int factory_update_modem_calib(unsigned char *data);
int factory_update_modem_ext_calib(unsigned char *data);
int factory_update_modem_factory(unsigned char *data);
int factory_update_ap_factory(unsigned char *data);
int factory_update_all(unsigned char *data, unsigned long size);
int factory_copy_from_mem(const u8 *buf);


/* Define a simple message function, for PC calib tool and u-boot.
   Borrow ap_factory_data top 1024 bytes to send and receive message. */
#define RDA_AP_CALIB_MSG_MAGIC	0xca1b5353
#define RDA_AP_CALIB_MSG_LEN	1024
#define RDA_AP_CALIB_MSG_ADDR	(RDA_AP_FACT_ADDR + RDA_AP_FACT_LEN - RDA_AP_CALIB_MSG_LEN)

#define RDA_AP_CALIB_MSG_DATA_LEN	1012
struct ap_calib_message {
	unsigned int magic; /* must be RDA_AP_CALIB_MSG_MAGIC */
	unsigned int id; /* see below defines */
	unsigned int size; /* the size of message data */
	unsigned char data[RDA_AP_CALIB_MSG_DATA_LEN];
};

/* PC calib tool -> u-boot */
#define RDA_AP_CALIB_MSG_SET_PRDINFO			0x1
int factory_get_ap_calib_msg(unsigned int *id, unsigned int *size, unsigned char *data);

/* u-boot -> PC calib tool */
#define RDA_AP_CALIB_MSG_GET_PRDINFO			0x80000001
int factory_set_ap_calib_msg(unsigned int id, unsigned int size, unsigned char *data);


#endif
