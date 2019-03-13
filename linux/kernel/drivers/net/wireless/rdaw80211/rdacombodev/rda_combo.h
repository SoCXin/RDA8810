#ifndef __RDA_COMBO_H__
#define __RDA_COMBO_H__

#include <linux/types.h>
#include <linux/string.h>
#include <linux/i2c.h>
#include <linux/wakelock.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm-generic/ioctl.h>
#include <asm/uaccess.h>

#define RDA_COMBO_FIRMWARE_NAME "rda_combo.bin"
#define RDA_FIRMWARE_VERSION 3
#define RDA_FIRMWARE_TYPE_SIZE 16		//firmware type size
#define RDA_FIRMWARE_DATA_NAME_SIZE 50	//firmware data_type size

#pragma pack(push)
#pragma pack(1)
struct rda_device_firmware_head {
	char firmware_type[RDA_FIRMWARE_TYPE_SIZE];
	u32 version;
	u32 data_num;
};
struct rda_firmware_data_type {
	char data_name[RDA_FIRMWARE_DATA_NAME_SIZE];
	u16 crc;
	s8 chip_version;
	u32 size;
};
#pragma pack(pop)

struct rda_firmware {
	const u8 *data;
	u32 num;
	u32 size;
	struct mutex lock;
	int status;
};

/*get data form firmware*/
//#define RDA_COMBO_FROM_FIRMWARE

#ifdef RDA_COMBO_FROM_FIRMWARE
int rda_write_data_to_rf_from_firmware(struct i2c_client* client, char *data_name);
#define rda_write_data_to_rf(CLIENT, DATA_NAME)\
	rda_write_data_to_rf_from_firmware(CLIENT, #DATA_NAME)

#else
#define rda_write_data_to_rf(CLIENT, ARRAY_DATA)\
	rda_i2c_write_data_to_rf(CLIENT, ARRAY_DATA, ARRAY_SIZE(ARRAY_DATA) )
#endif

#define RDA_BT_IOCTL_MAGIC 'u'

/* bt module */
#define RDA_BT_POWER_ON_IOCTL				   _IO(RDA_BT_IOCTL_MAGIC ,0x01)
#define RDA_BT_RF_INIT_IOCTL				   _IO(RDA_BT_IOCTL_MAGIC ,0x02)
#define RDA_BT_DC_CAL_IOCTL 				   _IO(RDA_BT_IOCTL_MAGIC ,0x03)
#define RDA_BT_RF_SWITCH_IOCTL				   _IO(RDA_BT_IOCTL_MAGIC ,0x04)
#define RDA_BT_POWER_OFF_IOCTL				   _IO(RDA_BT_IOCTL_MAGIC ,0x05)
#define RDA_BT_EN_CLK						   _IO(RDA_BT_IOCTL_MAGIC ,0x06)
#define RDA_BT_DC_DIG_RESET_IOCTL			   _IO(RDA_BT_IOCTL_MAGIC ,0x07)
#define RDA_BT_GET_ADDRESS_IOCTL			   _IO(RDA_BT_IOCTL_MAGIC ,0x08)
// add for pta
#define RDA_BT_DC_CAL_IOCTL_FIX_5991_LNA_GAIN           _IO(RDA_BT_IOCTL_MAGIC ,0x26)
// add for pta
/* wifi module */
#define RDA_WIFI_POWER_ON_IOCTL 			   _IO(RDA_BT_IOCTL_MAGIC ,0x10)
#define RDA_WIFI_POWER_OFF_IOCTL			   _IO(RDA_BT_IOCTL_MAGIC ,0x11)
#define RDA_WIFI_POWER_SET_TEST_MODE_IOCTL	   _IO(RDA_BT_IOCTL_MAGIC ,0x12)
#define RDA_WIFI_POWER_CANCEL_TEST_MODE_IOCTL  _IO(RDA_BT_IOCTL_MAGIC ,0x13)
#define RDA_WIFI_DEBUG_MODE_IOCTL			   _IO(RDA_BT_IOCTL_MAGIC ,0x14)
#define RDA_WLAN_COMBO_VERSION			   	   _IO(RDA_BT_IOCTL_MAGIC ,0x15)
#define RDA_COMBO_I2C_OPS    			   	   _IO(RDA_BT_IOCTL_MAGIC ,0x16)


/* add for wifi role ( sta, softap, p2p )*/
#define RDA_WIFI_STA_MODE_IOCTL                _IO(RDA_BT_IOCTL_MAGIC ,0x20)
#define RDA_WIFI_SOFTAP_MODE_IOCTL             _IO(RDA_BT_IOCTL_MAGIC ,0x21)
#define RDA_WIFI_P2P_MODE_IOCTL                _IO(RDA_BT_IOCTL_MAGIC ,0x22)

//#define WLAN_USE_CRYSTAL // if use share crystal should close this
#define WLAN_USE_DCDC  // if use LDO mode, should close this
//#define WLAN_FOR_CTA		// if need pass CTA authenticate, should open this define
#define CHINA_VERSION   // Low snr agc setting

#define RDA_I2C_CHANNEL 	(0)
#define RDA_WIFI_CORE_ADDR (0x13)
#define RDA_WIFI_RF_ADDR (0x14) //correct add is 0x14
#define RDA_BT_CORE_ADDR (0x15)
#define RDA_BT_RF_ADDR (0x16)

#define I2C_MASTER_ACK				(1<<0)
#define I2C_MASTER_RD				(1<<4)
#define I2C_MASTER_STO				(1<<8)
#define I2C_MASTER_WR				(1<<12)
#define I2C_MASTER_STA				(1<<16)

/* If defined COMBO_WITH_26MHZ, use 26MHZ FREQ,
  * else use 24MHZ FREQ.
*/
#define COMBO_WITH_26MHZ
#define WLAN_VERSION_90_D (1)
#define WLAN_VERSION_90_E (2)
#define WLAN_VERSION_91   (3)
#define WLAN_VERSION_91_E (4)
#define WLAN_VERSION_91_F (5)
#define WLAN_VERSION_91_G (6)


#define CLOCK_WLAN (1 << 0)
#define CLOCK_BT (1 << 1)
#define CLOCK_FM (1 << 2)
#define CLOCK_GPS (1 << 3)
#define CLOCK_MASK_ALL (0x0f)

#define I2C_DELAY_FLAG (0xFFFF)
#define DELAY_MS(x) {I2C_DELAY_FLAG, x},
#define RDA_WIFI_RF_I2C_DEVNAME "rda_wifi_rf_i2c"
#define RDA_WIFI_CORE_I2C_DEVNAME "rda_wifi_core_i2c"
#define RDA_BT_RF_I2C_DEVNAME "rda_bt_rf_i2c"
#define RDA_BT_CORE_I2C_DEVNAME "rda_bt_core_i2c"

extern struct i2c_client * rda_wifi_core_client;
extern struct i2c_client * rda_wifi_rf_client;
extern struct i2c_client * rda_bt_core_client;
extern struct i2c_client * rda_bt_rf_client;
extern struct completion rda_wifi_bt_comp;
#ifdef RDA_COMBO_FROM_FIRMWARE
extern atomic_t wifi_fw_status;
extern atomic_t bt_fw_status;
#endif

int i2c_write_1_addr_2_data(struct i2c_client* client, const u8 addr, const u16 data);
int i2c_read_1_addr_2_data(struct i2c_client* client, const u8 addr, u16* data);
int rda_i2c_write_data_to_rf(struct i2c_client* client, const u16 (*data)[2], u32 count);
void rda_release_firmware(void);

void enable_26m_regulator(u8 mask);
void disable_26m_regulator(u8 mask);
void enable_32k_rtc(u8 mask);
void disable_32k_rtc(u8 mask);
void enable_26m_rtc(u8 mask);
void disable_26m_rtc(u8 mask);

void rda_combo_i2c_lock(void);
void rda_combo_i2c_unlock(void);

u32 rda_wlan_version(void);
u8  check_test_mode(void);

int rda_5990_wifi_power_off(void);
int rda_5990_wifi_power_on(void);
int rda_5990_bt_power_on(void);
int rda_5990_bt_power_off(void);
int rda_5990_fm_power_on(void);
int rda_5990_fm_power_off(void);
long rda_5990_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

int rda_5991_wifi_power_on(void);
int rda_5991_wifi_power_off(void);
int rda_5991_bt_power_on(void);
int rda_5991_bt_power_off(void);
int rda_5991_fm_power_on(void);
int rda_5991_fm_power_off(void);
long rda_5991_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

int rda_5991e_wifi_power_on(void);
int rda_5991f_wifi_power_on(void);
int rda_5991g_wifi_power_on(void);

int rda_5991e_wifi_power_off(void);
int rda_5991f_wifi_power_off(void);
int rda_5991g_wifi_power_off(void);

int rda_5991e_bt_power_on(void);
int rda_5991f_bt_power_on(void);
int rda_5991g_bt_power_on(void);

int rda_5991e_bt_power_off(void);
int rda_5991f_bt_power_off(void);
int rda_5991g_bt_power_off(void);

int rda_5991e_fm_power_on(void);
int rda_5991f_fm_power_on(void);
int rda_5991g_fm_power_on(void);

int rda_5991e_fm_power_off(void);
int rda_5991f_fm_power_off(void);
int rda_5991g_fm_power_off(void);

long rda_5991e_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
long rda_5991f_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
long rda_5991g_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif

