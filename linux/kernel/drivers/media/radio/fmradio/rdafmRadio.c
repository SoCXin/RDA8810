/*
 * =====================================================================================
 *
 *       Filename:  rdafmRadio.c
 *
 *    Description:  RDA5990 FM Receiver driver for linux.
 *
 *        Version:  1.0
 *        Created:  06/12/2013 04:19:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Naiquan Hu, 
 *   Organization:  RDA Microelectronics Inc.
 *
  * Copyright (C) 2013 RDA Microelectronics Inc.
 * =====================================================================================
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h> // udelay()
#include <linux/device.h> // device_create()
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
//#include <linux/version.h>      /* constant of kernel version */
#include <asm/uaccess.h> // get_user()
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/audiocontrol.h>

#include "rdafmRadio.h"


// if need debug, define RDAFM_DEBUG 1
#define RDAFM_DEBUG 1

//#define I2C_BOARD_INFO_REGISTER_STATIC

#define FM_ERROR(f, s...) \
	do { \
		printk(KERN_ERR "RDAFM " f, ## s); \
	} while(0)

#if (RDAFM_DEBUG)
#define FM_DEBUG(f, s...) \
	do { \
		printk(KERN_INFO"RDAFM " f, ## s); \
	} while(0)
#else
#define FM_DEBUG(f, s...)
#endif

#define RDA599X_SCANTBL_SIZE  16 //16*uinit16_t
#define RETRY_MAX 5

extern int rda_fm_power_off(void);
extern int rda_fm_power_on(void);

/******************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************/
#ifdef I2C_BOARD_INFO_REGISTER_STATIC
#define I2C_BUS_NUM           4
#define RDAFM_DEV_ADDR        0x11    // write addr 0x22; read addr 0x23
#endif

#define RDAFM_RSSI_MASK       0X7F // RSSI
#define RDAFM_DEV             "rda_fm_radio_i2c"

#define RDA5820NS_CHIPID      0x5820
#define RDA5801_CHIPID        0x5801

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

static struct fm *g_fm_struct = NULL;
static atomic_t isScanCanceled;

#define FM_SLEEP_TIME	40
#define FM_SEEK_THRESHOLD	8

#define FM_DEV_MAJOR	0 //0 -- dynamic allocation; xxx -- static allocation
#define FM_DEV_MINOR	0
int fm_dev_major =	FM_DEV_MAJOR;
int fm_dev_minor =	FM_DEV_MINOR;

/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/
typedef struct
{
	uint8_t		address;
	uint16_t	value;
}RDA_FM_REG_T;   

typedef enum
{
	FM_RECEIVER,				//5800,5802,5804
	FM_TRANSMITTER,            	//5820
}RDA_RADIO_WORK_MODE_T;


struct fm {
	uint32_t ref;
	bool powerup;
	uint16_t chip_id;
	uint16_t device_id;
	dev_t dev_t;
	uint16_t min_freq; // KHz
	uint16_t max_freq; // KHz
	uint8_t band;   // TODO
	struct class *cls;
	struct device *dev;
	struct cdev cdev;
	struct i2c_client *i2c_client;
	struct audiocontrol_dev acdev;
};




/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/

static int hal_unmute(struct i2c_client *client);
static int hal_mute(struct i2c_client *client);
//static int hal_setStereo(struct i2c_client *client,uint8_t b);
//static int hal_setRssiThreshold(struct i2c_client *client,uint8_t RssiThreshold);
//static int hal_setDeEmphasis(struct i2c_client *client,uint8_t index);
static bool hal_scan(struct i2c_client *client,
		uint16_t min_freq, uint16_t max_freq,
		uint16_t *pFreq, //get the valid freq after scan
		uint16_t *pScanTBL,
		uint16_t *ScanTBLsize,
		uint16_t scandir,
		uint16_t space);


static int hal_read(struct i2c_client *client, uint8_t addr, uint16_t *val);
static int hal_write(struct i2c_client *client, uint8_t addr, uint16_t val);
//static void hal_em_test(struct i2c_client *client, uint16_t group_idx, uint16_t item_idx, uint32_t item_value);
static int fm_setup_cdev(struct fm *fm);
static long fm_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int fm_ops_open(struct inode *inode, struct file *filp);
static int fm_ops_release(struct inode *inode, struct file *filp);

static int fm_init(struct i2c_client *client);
static int fm_destroy(struct fm *fm);
static int fm_powerUp(struct fm *fm, struct fm_tune_parm *parm);
static int fm_powerDown(struct fm *fm);

static int fm_tune(struct fm *fm, struct fm_tune_parm *parm);
static int fm_seek(struct fm *fm, struct fm_seek_parm *parm);
static int fm_scan(struct fm *fm, struct fm_scan_parm *parm);
static int fm_setVol(struct fm *fm, uint32_t vol);
static int fm_getVol(struct fm *fm, uint32_t *vol);
static int fm_getRssi(struct fm *fm, int32_t *rssi);

static int fm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int fm_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int fm_i2c_remove(struct i2c_client *client);

/******************************************************************************
 * GLOBAL DATA
 *****************************************************************************/
#ifdef I2C_BOARD_INFO_REGISTER_STATIC
static struct i2c_board_info i2c_rdafm={ I2C_BOARD_INFO(RDAFM_DEV, RDAFM_DEV_ADDR)};
#endif

/* Addresses to scan */
static const struct i2c_device_id fm_i2c_id[] = {{RDAFM_DEV, 0}, {}};

static struct i2c_driver RDAFM_driver = {
		.class = I2C_CLASS_HWMON,
		.probe = fm_i2c_probe,
		.remove = fm_i2c_remove,
		.detect = fm_i2c_detect,
		.driver.name = RDAFM_DEV,
		.id_table = fm_i2c_id,
};

static uint16_t gChipID = 0x0;
static RDA_RADIO_WORK_MODE_T workMode = FM_RECEIVER;

struct fmvolume{
	int vol_db;
	int dacgain;
};

static const struct fmvolume fmvol[] = {
	{0, 0x0000},
	{600, 0x1},  // 0001, 600 means 6db
	{1200, 0x2}, // 0010
	{1556, 0x3}, //0011, 1556 means 15.56db
	{1841, 0x4}, // 0100
	{2056, 0x5}, // 0101
	{2182, 0x6}, // 0110
	{2292, 0x7}, // 0111
	{2371, 0x8}, // 1000
	{2443, 0x9}, // 1001
	{2526, 0xa}, // 1010
	{2602, 0xb}, // 1011
	{2630, 0xc}, // 1100
	{2795, 0xd}, // 1101
	{2924, 0xe}, // 1110
	{3045, 0xf}, // 1111
};

static const RDA_FM_REG_T RDA5820NS_TX_initialization_reg[]={
	{0x02, 0xE003},
	{0xFF, 100},    // if address is 0xFF, sleep value ms
	{0x02, 0xE001},
	{0x19, 0x88A8},
	{0x1A, 0x4290},
	{0x68, 0x0AF0},
	{0x40, 0x0001},
	{0x41, 0x41FF},
	{0xFF, 500},
//	{0x03, 0x1B90},
};

static const RDA_FM_REG_T RDA5820NS_RX_initialization_reg[]={
	{0x02, 0x0002}, //Soft reset
	{0xFF, 100},    // wait
	{0x02, 0xC001},  //Power Up 
	{0x05, 0x86AF},  //LNAP  0x884F --LNAN; 0x888F -- 差分输入; 0x0x88AF -- 单端输入
	{0x06, 0x6000},
	{0x13, 0x80E1},
	{0x14, 0x2A11},
	{0x1C, 0x22DE},
	{0x21, 0x0020},
	//{0x03, 0x1B90},
};

// TODO
static const RDA_FM_REG_T RDA5801_TX_initialization_reg[]={
	{0x02, 0x0002}, //Soft reset
	{0xFF, 100},    // wait
	{0x02, 0xC001},  //Power Up
	{0x05, 0x88CF},  //LNAP  0x884F --LNAN; 0x888F -- 差分输入; 0x0x88AF -- 单端输入
	{0x06, 0x6000},
	{0x13, 0x80E1},
	{0x14, 0x2A11},
	{0x1C, 0x22DE},
	{0x1F, 0x0081},
	{0x21, 0x0020},
	{0x03, 0x1B90},
	//{0x03, 0x1B90},
};

static const RDA_FM_REG_T RDA5801_RX_initialization_reg[]={
	{0x02, 0x0002}, //Soft reset
	{0xFF, 100},    // wait
	{0x02, 0xC001},  //Power Up
	{0x05, 0x86CF},  //LNAP  0x884F --LNAN; 0x888F -- 差分输入; 0x0x88AF -- 单端输入
	{0x06, 0x6000},
	{0x13, 0x80E1},
	{0x14, 0x2A11},
	{0x1C, 0x22DE},
	{0x1F, 0x0080}, //0x0081
	{0x20, 0x0021},
	{0x21, 0x0020},
	{0x03, 0x1B90},
	//{0x03, 0x1B90},
};



static struct file_operations fm_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = fm_ops_ioctl,
	.open = fm_ops_open,
	.release = fm_ops_release,
};

DEFINE_SEMAPHORE(fm_ops_mutex);


static int hal_getChipID(struct i2c_client *client, uint16_t *pChipID)
{
	int err;
	int ret = -1;
	uint16_t val = 0x0002;

	//Reset RDA FM
	err = hal_write(client, 0x02, val);
	if(err < 0){
		FM_ERROR("hal_getChipID: reset FM chip failed!\n");
		return -1;
	}
	msleep(2*FM_SLEEP_TIME);

	val = 0;
	err = hal_read(client, 0x0C, &val);
	if (err == 0)
	{
		if ((0x5802 == val) || (0x5803 == val))
		{
			err = hal_read(client, 0x0E, &val);

			if (err == 0){
				*pChipID = val;
			}else{
				*pChipID = 0x5802;
			}

#ifdef FMDEBUG
			FM_DEBUG("hal_getChipID: Chip ID = %04X\n", val);
#endif
			ret = 0;

		}
		else if ((0x5805 == val) || (0x5820 == val))
		{
			*pChipID = val;
			ret = 0;
		}
		else
		{
			FM_ERROR("hal_getChipID: get chip ID failed! get value = %04X\n", val);
			ret = -1;
		}

	}
	else
	{
		FM_ERROR("hal_getChipID: get chip ID failed!\n");
		ret = -1;
	}

	return ret;
}


/*
 *  hal_read
 */
static int hal_read(struct i2c_client *client, uint8_t addr, uint16_t *val)
{
	int ret = -1;
	char buf[2] = {0};

	// first, send addr to RDAFM
	ret = i2c_master_send(client, (char*)&addr, 1);
	if (ret < 0)
	{
		FM_ERROR("hal_read send:0x%X err:%d\n", addr, ret);
		return -1;
	}

	// second, receive two byte from RDAFM
	ret = i2c_master_recv(client, buf, 2);
	if (ret < 0)
	{
		FM_ERROR("hal_read recv:0x%X err:%d\n", addr, ret);
		return -1;
	}

	*val = (uint16_t)(buf[0] << 8 | buf[1]);

	return 0;
}

/*
 *  hal_write
 */
static int hal_write(struct i2c_client *client, uint8_t addr, uint16_t val)
{
	int ret;
	char buf[3];

	buf[0] = addr;
	buf[1] = (char)(val >> 8);
	buf[2] = (char)(val & 0xFF);

	ret = i2c_master_send(client, buf, 3);
	if (ret < 0)
	{
		FM_ERROR("hal_write send:0x%X err:%d\n", addr, ret);
		return -1;
	}

	return 0;
}


static int hal_unmute(struct i2c_client *client)
{
	int ret = 0;
	uint16_t tRegValue = 0;

	FM_DEBUG("hal_unmute\n");

	ret = hal_read(client, 0x02, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_unmute  read register failed!\n");
		return -1;
	}

	FM_DEBUG("reg 02H value read is: %04x\n", tRegValue);
	tRegValue |= (1 << 14);
	FM_DEBUG("reg 02H value write is: %04x\n", tRegValue);
	ret = hal_write(client, 0x02, tRegValue);

	if (ret < 0)
	{
		FM_ERROR("hal_unmute  write register failed!\n");
		return -1;
	}

	return 0;
}



static int hal_mute(struct i2c_client *client)
{
	int ret = 0;
	uint16_t tRegValue = 0;

	FM_DEBUG("hal_mute\n");

	ret = hal_read(client, 0x02, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_mute  read register failed!\n");
		return -1;
	}

	FM_DEBUG("reg 02H value read is: %04x\n", tRegValue);
	tRegValue &= (~(1 << 14));
	FM_DEBUG("reg 02H value write is: %04x\n", tRegValue);
	ret = hal_write(client, 0x02, tRegValue);

	if (ret < 0)
	{
		FM_ERROR("hal_mute  write register failed!\n");
		return -1;
	}

	return 0;
}

static int hal_is_mute(struct i2c_client *client)
{
	int ret = 0;
	uint16_t tRegValue = 0;

	FM_DEBUG("hal_is_mute\n");

	ret = hal_read(client, 0x02, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_is_mute  read register failed!\n"); 
		return -1;
	}

	FM_DEBUG("reg 02H value read is: %04x\n", tRegValue);
	if (tRegValue & (1 << 14))
		return 0;
	else
		return 1;

	return 0;
}



/*
//b=true set stereo else set mono
static int hal_setStereo(struct i2c_client *client, uint8_t b)
{
	int ret = 0;
	uint16_t tRegValue = 0;

	FM_DEBUG("hal_setStereo\n");

	ret = hal_read(client, 0x02, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_setStereo  read register failed!\n"); 
		return -1;
	}
	if (b)
		tRegValue &= (~(1 << 13));//set stereo
	else
		tRegValue |= (1 << 13); //set mono

	ret = hal_write(client, 0x02, tRegValue);

	if (ret < 0)
	{
		FM_ERROR("hal_setStereo  write register failed!\n"); 
		return -1;
	}


	return 0;

}


static int hal_setRssiThreshold(struct i2c_client *client, uint8_t RssiThreshold)
{
	int ret = 0;
	uint16_t tRegValue = 0;

	FM_DEBUG("hal_setRssiThreshold\n");

	ret = hal_read(client, 0x05, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_setRssiThreshold  read register failed!\n"); 
		return -1;
	}

	tRegValue &= 0x80FF;//clear valume
	tRegValue |= ((RssiThreshold & 0x7f) << 8); //set valume

	ret = hal_write(client, 0x05, tRegValue);

	if (ret < 0)
	{
		FM_ERROR("hal_setRssiThreshold  write register failed!\n"); 
		return -1;
	}


	return 0;

}



static int hal_setDeEmphasis(struct i2c_client *client, uint8_t index)
{
	int ret = 0;
	uint16_t tRegValue = 0;

	FM_DEBUG("hal_setRssiThreshold\n");

	ret = hal_read(client, 0x04, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_setRssiThreshold  read register failed!\n"); 
		return -1;
	}

	if (0 == index)
	{
		tRegValue &= (~(1 << 11));//De_Emphasis=75us
	}
	else if (1 == index)
	{
		tRegValue |= (1 << 11);//De_Emphasis=50us
	}


	ret = hal_write(client, 0x04, tRegValue);

	if (ret < 0)
	{
		FM_ERROR("hal_setRssiThreshold  write register failed!\n"); 
		return -1;
	}


	return 0;


}


static void hal_em_test(struct i2c_client *client, uint16_t group_idx, uint16_t item_idx, uint32_t item_value)
{
	FM_DEBUG("hal_em_test  %d:%d:%d\n", group_idx, item_idx, item_value); 
	switch (group_idx)
	{
		case mono:
			if(item_value == 1)
			{
				hal_setStereo(client, 0); //force mono
			}
			else
			{
				hal_setStereo(client, 1); //stereo

			}

			break;
		case stereo:
			if(item_value == 0)
			{
				hal_setStereo(client, 1); //stereo
			}
			else
			{
				hal_setStereo(client, 0); //force mono	
			}
			break;
		case RSSI_threshold:
			item_value &= 0x7F;
			hal_setRssiThreshold(client, item_value);
			break;		    
		case Softmute_Enable:
			if (item_idx)
			{
				hal_mute(client);
			}
			else
			{
				hal_unmute(client);
			}
			break;
		case De_emphasis:
			if(item_idx >= 2) //0us
			{
				FM_DEBUG("RDAFM not support De_emphasis 0\n");		
			}
			else
			{
				hal_setDeEmphasis(client,item_idx);//0=75us,1=50us
			}
			break;

		case HL_Side:

			break;
		default:
			FM_ERROR("RDAFM not support this setting\n");
			break;   
	}
}
*/

static bool hal_scan(struct i2c_client *client, 
		uint16_t min_freq, uint16_t max_freq,
		uint16_t *pFreq,
		uint16_t *pScanTBL, 
		uint16_t *ScanTBLsize, 
		uint16_t scandir, 
		uint16_t space)
{
	uint16_t tFreq, tRegValue = 0;
	uint16_t tmp_scanTBLsize = *ScanTBLsize;
	int ret = -1;
	bool isTrueStation = false;
	uint16_t oldValue = 0;
	int channel = 0;
	bool isFirstChannel = false;

	if((!pScanTBL) || (tmp_scanTBLsize == 0)) {
		return false;
	}

	//clear the old value of pScanTBL
	memset(pScanTBL, 0, sizeof(uint16_t)*RDA599X_SCANTBL_SIZE);

	if(tmp_scanTBLsize > RDA599X_SCANTBL_SIZE)
	{
		tmp_scanTBLsize = RDA599X_SCANTBL_SIZE;
	}

	//scan up
	if(scandir == FM_SEEK_UP){ // now, only support scan up
		tFreq = min_freq;
	}else{ //scan down
		tFreq = max_freq;//max_freq compare need or not   
	}

	//mute FM
	ret = hal_mute(client);
	if (ret < 0)
	{
		FM_ERROR("hal_mute failed!\n"); 
		return false;
	}

	//set seekth
	tRegValue = 0;
	ret = hal_read(client, 0x05, &tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_read failed!\n"); 
		return false;
	}
	tRegValue &= (~(0x7f<<8));
	tRegValue |= ((FM_SEEK_THRESHOLD & 0x7f) << 8);
	ret = hal_write(client, 0x05, tRegValue);
	if (ret < 0)
	{
		FM_ERROR("hal_write failed!\n"); 
		return false;
	}
	msleep(FM_SLEEP_TIME);

	atomic_set(&isScanCanceled, 0);
	do {
		if(atomic_read(&isScanCanceled) == 1)
			break;
		isTrueStation = false;

		//set channel and enable TUNE
		tRegValue = 0;
		ret = hal_read(client, 0x03, &tRegValue);
		if (ret < 0)
		{
			FM_ERROR("hal_read failed!\n"); 
			return false;
		}

		tRegValue &= (~(0x03ff<<6)); //clear bit[15:6]
		channel = tFreq - min_freq; 
		tRegValue |= (((channel+5) << 6) | (1 << 4)); //set bit[15:6] and bit[4]
		ret = hal_write(client, 0x03, tRegValue);
		if (ret < 0)
		{
			FM_ERROR("hal_write failed!\n"); 
			return false;
		}
		msleep(FM_SLEEP_TIME);

		//read 0x0B and check FM_TRUE(bit[8])
		tRegValue = 0;
		ret = hal_read(client, 0x0B, &tRegValue);
		if (ret < 0)
		{
			FM_ERROR("hal_read failed!\n"); 
			return false;
		} else if(!ret){
			if((tRegValue & 0x0100) == 0x0100){
				isTrueStation = true;
			}
		}

		//if this freq is a true station, read the channel
		if(isTrueStation){
			FM_DEBUG("Frequency %d (channel %d) is a true station.\n", tFreq, channel);
			if(!isFirstChannel){
				isFirstChannel = true;
				*pFreq = tFreq;
			}
			oldValue = *(pScanTBL+(channel/16));
			oldValue |= (1<<(channel%16));
			*(pScanTBL+(channel/16)) = oldValue;
		}else{
			FM_DEBUG("Frequency %d (channel %d) is not a true station.\n", tFreq, channel);
		}

		//increase freq
		tFreq += space;
	}while( tFreq <= max_freq );


	*ScanTBLsize = tmp_scanTBLsize;

	//clear FM mute
	ret = hal_unmute(client);
	if (ret < 0)
	{
		FM_ERROR("hal_unmute failed!\n"); 
		return false;
	}

	return true;
}


static int fm_setup_cdev(struct fm *fm)
{
	int err;

	if(fm_dev_major){
		fm->dev_t = MKDEV(fm_dev_major, fm_dev_minor);
		err = register_chrdev_region(fm->dev_t, 1, FM_NAME);
		if (err) {
			FM_ERROR("register dev_t failed\n");
			return -1;
		}
		FM_DEBUG("register device %s:%d:%d\n", FM_NAME,
				MAJOR(fm->dev_t), MINOR(fm->dev_t));
	}else{
		err = alloc_chrdev_region(&fm->dev_t, 0, 1, FM_NAME);
		if (err) {
			FM_ERROR("alloc dev_t failed\n");
			return -1;
		}
		FM_DEBUG("alloc device %s:%d:%d\n", FM_NAME,
				MAJOR(fm->dev_t), MINOR(fm->dev_t));
	}

	cdev_init(&fm->cdev, &fm_ops);

	fm->cdev.owner = THIS_MODULE;
	fm->cdev.ops = &fm_ops;

	err = cdev_add(&fm->cdev, fm->dev_t, 1);
	if (err) {
		FM_ERROR("alloc dev_t failed\n");
		return -1;
	}

	fm->cls = class_create(THIS_MODULE, FM_NAME);
	if (IS_ERR(fm->cls)) {
		err = PTR_ERR(fm->cls);
		FM_ERROR("class_create err:%d\n", err);
		return err;            
	}    
	fm->dev = device_create(fm->cls, NULL, fm->dev_t, NULL, FM_NAME);

	return 0;
}



static long fm_ops_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct fm *fm = container_of(filp->f_dentry->d_inode->i_cdev, struct fm, cdev);

	FM_DEBUG("%s cmd(%x)\n", __func__, cmd);

	switch(cmd)
	{
		case FM_IOCTL_POWERUP:
			{
				struct fm_tune_parm parm;
				FM_DEBUG("FM_IOCTL_POWERUP\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//                return -EPERM;

				if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_tune_parm)))
					return -EFAULT;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				ret = fm_powerUp(fm, &parm);
				up(&fm_ops_mutex);
				if (copy_to_user((void*)arg, &parm, sizeof(struct fm_tune_parm)))
					return -EFAULT;
				//			fm_low_power_wa(1);
				break;
			}

		case FM_IOCTL_POWERDOWN:
			{
				FM_DEBUG("FM_IOCTL_POWERDOWN\n");
				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//                return -EPERM;
				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				ret = fm_powerDown(fm);
				up(&fm_ops_mutex);
				//			fm_low_power_wa(0);
				break;
			}

			// tune (frequency, auto Hi/Lo ON/OFF )
		case FM_IOCTL_TUNE:
			{
				struct fm_tune_parm parm;
				FM_DEBUG("FM_IOCTL_TUNE\n");
				// FIXME!
				//            if (!capable(CAP_SYS_ADMIN))
				//                return -EPERM;

				if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_tune_parm)))
					return -EFAULT;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				ret = fm_tune(fm, &parm);
				up(&fm_ops_mutex);

				if (copy_to_user((void*)arg, &parm, sizeof(struct fm_tune_parm)))
					return -EFAULT;

				break;
			}

		case FM_IOCTL_SEEK:
			{
				struct fm_seek_parm parm;
				FM_DEBUG("FM_IOCTL_SEEK\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;

				if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_seek_parm)))
					return -EFAULT;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				ret = fm_seek(fm, &parm);
				up(&fm_ops_mutex);

				if (copy_to_user((void*)arg, &parm, sizeof(struct fm_seek_parm)))
					return -EFAULT;

				break;
			}

		case FM_IOCTL_SETVOL:
			{
				uint32_t vol;
				FM_DEBUG("FM_IOCTL_SETVOL\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;

				if(copy_from_user(&vol, (void*)arg, sizeof(uint32_t))) {
					FM_ERROR("copy_from_user failed\n");
					return -EFAULT;
				}

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				ret = fm_setVol(fm, vol);
				up(&fm_ops_mutex);

				break;
			}

		case FM_IOCTL_GETVOL:
			{
				uint32_t vol;
				FM_DEBUG("FM_IOCTL_GETVOL\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				ret = fm_getVol(fm, &vol);
				up(&fm_ops_mutex);

				if (copy_to_user((void*)arg, &vol, sizeof(uint32_t)))
					return -EFAULT;

				break;
			}

		case FM_IOCTL_MUTE:
			{
				uint32_t bmute;
				FM_DEBUG("FM_IOCTL_MUTE\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;
				if (copy_from_user(&bmute, (void*)arg, sizeof(uint32_t)))
				{
					FM_DEBUG("copy_from_user mute failed!\n");
					return -EFAULT;    
				}

				FM_DEBUG("FM_IOCTL_MUTE:%d\n", bmute); 
				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;

				if (bmute){
					ret = hal_mute(fm->i2c_client);
				}else{
					ret = hal_unmute(fm->i2c_client);
				}

				up(&fm_ops_mutex);

				break;
			}

		case FM_IOCTL_GETRSSI:
			{
				int32_t rssi;
				FM_DEBUG("FM_IOCTL_GETRSSI\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;

				ret = fm_getRssi(fm, &rssi);
				up(&fm_ops_mutex);

				if (copy_to_user((void*)arg, &rssi, sizeof(uint32_t)))
					return -EFAULT;

				break;
			}

		case FM_IOCTL_RW_REG:
			{
				struct fm_ctl_parm parm_ctl;
				FM_DEBUG("FM_IOCTL_RW_REG\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;

				if (copy_from_user(&parm_ctl, (void*)arg, sizeof(struct fm_ctl_parm)))
					return -EFAULT;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;

				if(parm_ctl.rw_flag == 0) //write
				{
					ret = hal_write(fm->i2c_client, parm_ctl.addr, parm_ctl.val);
				}
				else
				{
					ret = hal_read(fm->i2c_client, parm_ctl.addr, &parm_ctl.val);
				}

				up(&fm_ops_mutex);
				if ((parm_ctl.rw_flag == 0x01) && (!ret)) // Read success.
				{ 
					if (copy_to_user((void*)arg, &parm_ctl, sizeof(struct fm_ctl_parm)))
						return -EFAULT;
				}
				break;
			}

		case FM_IOCTL_GETCHIPID:
			{
				uint16_t chipid;            

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;

				hal_getChipID(fm->i2c_client, &chipid);
				FM_DEBUG("FM_IOCTL_GETCHIPID:%04x\n", chipid);   
				up(&fm_ops_mutex);

				if (copy_to_user((void*)arg, &chipid, sizeof(uint16_t)))
					return -EFAULT;

				break;
			}

		case FM_IOCTL_EM_TEST:
			{
				struct fm_em_parm parm_em;
				FM_DEBUG("FM_IOCTL_EM_TEST\n");

				// FIXME!!
				//            if (!capable(CAP_SYS_ADMIN))
				//              return -EPERM;

				if (copy_from_user(&parm_em, (void*)arg, sizeof(struct fm_em_parm)))
					return -EFAULT;

				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;

				//hal_em_test(fm->i2c_client, parm_em.group_idx, parm_em.item_idx, parm_em.item_value);

				up(&fm_ops_mutex);

				break;
			}
		case FM_IOCTL_IS_FM_POWERED_UP:
			{
				uint32_t powerup;
				FM_DEBUG("FM_IOCTL_IS_FM_POWERED_UP");
				if (fm->powerup) {
					powerup = 1;
				} else {
					powerup = 0;
				}
				if (copy_to_user((void*)arg, &powerup, sizeof(uint32_t)))
					return -EFAULT;
				break;
			}

#ifdef FMDEBUG
		case FM_IOCTL_DUMP_REG:
			{
				uint16_t chipid = 0;
				if (down_interruptible(&fm_ops_mutex))
					return -EFAULT;
				hal_getChipID(fm->i2c_client, &chipid);
				up(&fm_ops_mutex);

				break;
			}
#endif

		case FM_IOCTL_SCAN:
			{
				struct fm_scan_parm parm;
				FM_DEBUG("FM_IOCTL_SCAN\n");
				if (! fm->powerup){
					return -EFAULT;
				}
				if(copy_from_user(&parm, (void*)arg, sizeof(struct fm_scan_parm))){
					return -EFAULT;
				}
				if (down_interruptible(&fm_ops_mutex)){
					return -EFAULT;
				}
				fm_scan(fm, &parm);
				up(&fm_ops_mutex);

				if(copy_to_user((void*)arg, &parm, sizeof(struct fm_scan_parm))){
					return -EFAULT;
				}

				break;
			}

		case FM_IOCTL_STOP_SCAN:
			{
				FM_DEBUG("FM_IOCTL_STOP_SCAN\n");
				atomic_set(&isScanCanceled, 1);
				break;
			}
			
		default:
			{
				FM_DEBUG("default\n");
				break;
			}
	}

	return ret;
}

static int fm_ops_open(struct inode *inode, struct file *filp)
{
	struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

	FM_DEBUG("%s\n", __func__);

	if (down_interruptible(&fm_ops_mutex))
		return -EFAULT;

	// TODO: only have to set in the first time?
	// YES!!!!

	fm->ref++;

	up(&fm_ops_mutex);

	filp->private_data = fm;

	// TODO: check open flags

	return 0;
}

static int fm_ops_release(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

	FM_DEBUG("%s\n", __func__);

	if (down_interruptible(&fm_ops_mutex))
		return -EFAULT;
	fm->ref--;
	if(fm->ref < 1) {
		if(fm->powerup) {
			fm_powerDown(fm);           
		}
	}

	up(&fm_ops_mutex);

	return err;
}

static int fm_audiocontrol_get_volume(struct audiocontrol_dev* acdev)
{
	struct fm *fm =
		container_of(acdev, struct fm, acdev);

	if(!fm)
		return -EFAULT;

	return acdev->volume;
}
static int fm_audiocontrol_set_volume(struct audiocontrol_dev* acdev, int volume)
{
	int i = 0, reg = -1;
	struct fm *fm =
		container_of(acdev, struct fm, acdev);

	if(!fm)
		return -EFAULT;

	for(i = 0; i < sizeof(fmvol)/sizeof(struct fmvolume); ++i) {
		if(volume == fmvol[i].vol_db) {
			reg = fmvol[i].dacgain;
			printk(KERN_INFO"fm set volume : found reg value %d\n", reg);
			break;
		}
	}

	if(reg == -1) {
		printk(KERN_INFO"fm set volume fail: %d\n", volume);
		return -EFAULT;
	}

	if (down_interruptible(&fm_ops_mutex))
		return -EFAULT;
	fm_setVol(fm, reg);
	up(&fm_ops_mutex);

	acdev->volume = volume;

	return 0;
}
static int fm_audiocontrol_get_mute(struct audiocontrol_dev* acdev)
{
	struct fm *fm =
		container_of(acdev, struct fm, acdev);

	if(!fm) {
		printk(KERN_INFO"%s : fm is NULL.", __func__);
		return -EFAULT;
	}

	if (down_interruptible(&fm_ops_mutex))
		return -EFAULT;

	acdev->mute = hal_is_mute(fm->i2c_client);

	up(&fm_ops_mutex);

	return acdev->mute;
}
static int fm_audiocontrol_set_mute(struct audiocontrol_dev* acdev, int mute)
{
	struct fm *fm =
		container_of(acdev, struct fm, acdev);

	if(!fm) {
		printk(KERN_INFO"%s : fm is NULL.", __func__);
		return -1;
	}

	if (down_interruptible(&fm_ops_mutex))
		return -EFAULT;

	if(mute) {
		if (hal_mute(fm->i2c_client) < 0) {
			up(&fm_ops_mutex);
			return -1;
		}
		acdev->mute = 1;
	}
	else {
		if (hal_unmute(fm->i2c_client) < 0) {
			up(&fm_ops_mutex);
			return -1;
		}
		acdev->mute = 0;
	}

	up(&fm_ops_mutex);

	return 0;
}

static int fm_init(struct i2c_client *client)
{
	int err;
	struct fm *fm = NULL;

	FM_DEBUG("%s()\n", __func__);
	if (!(fm = kmalloc(sizeof(struct fm), GFP_KERNEL)))
	{
		FM_ERROR("-ENOMEM\n");
		err = -ENOMEM;
		goto ERR_EXIT;
	}

	fm->ref = 0;
	fm->powerup = false;
	gChipID = 0;
	fm->chip_id = gChipID;
	atomic_set(&isScanCanceled, 0);


	if ((err = fm_setup_cdev(fm)))
	{
		goto ERR_EXIT;
	}

	g_fm_struct = fm;
	fm->i2c_client = client;
	i2c_set_clientdata(client, fm);


	// register audiocontrol
	fm->acdev.name = "fmaudio";
	fm->acdev.mute = 0;
	fm->acdev.volume = 0;
	fm->acdev.get_volume = fm_audiocontrol_get_volume;
	fm->acdev.set_volume = fm_audiocontrol_set_volume;
	fm->acdev.get_mute = fm_audiocontrol_get_mute;
	fm->acdev.set_mute = fm_audiocontrol_set_mute;
	audiocontrol_dev_register(&fm->acdev);

	FM_DEBUG("fm_init is ok!\n");

	return 0;

ERR_EXIT:
	kfree(fm);

	return err;
}

static int fm_destroy(struct fm *fm)
{
	int err = 0;

	FM_DEBUG("%s\n", __func__);

	audiocontrol_dev_unregister(&fm->acdev);

	device_destroy(fm->cls, fm->dev_t);
	class_destroy(fm->cls);

	cdev_del(&fm->cdev);
	unregister_chrdev_region(fm->dev_t, 1);



	// FIXME: any other hardware configuration ?

	// free all memory
	kfree(fm);

	return err;
}

/*
 *  fm_powerUp
 */
static int fm_powerUp(struct fm *fm, struct fm_tune_parm *parm)
{
	int i;
	uint16_t tRegValue = 0x0002;
	int ret = -1;
	int retry = 0, retryMax = RETRY_MAX;
	int size = 0;
	const RDA_FM_REG_T* init_regs = NULL;

	struct i2c_client *client = fm->i2c_client;

	if (fm->powerup)
	{
		parm->err = FM_BADSTATUS;
		return -EPERM;
	}

	// If it's the first powerUp, FM chip_id is unknown. Then get the chip_id
	if(fm->chip_id == 0){
		// enable the FM chip in RDA5990P
		FM_DEBUG("%s() enable the FM chip in RDA5990P\n", __func__);
		while((ret < 0) && (retry < retryMax)){
			FM_DEBUG("%s() enable the FM chip in RDA5990P, retry time: %d\n", __func__, (retry + 1));
			ret = rda_fm_power_on();
			retry++;
		}

		if(ret < 0){
			FM_ERROR("%s() enable the FM chip in RDA5990P failed!\n", __func__);
			return -EPERM;
		}
		msleep(2*FM_SLEEP_TIME);
		ret = hal_getChipID(client, &gChipID);
		if(ret < 0){
			FM_ERROR("%s() Get the FM chip ID in RDA5990P failed!\n", __func__);
			return -EPERM;
		}else{
			FM_DEBUG("%s() the FM in RDA5990P chip ID = 0x%04x\n", __func__, gChipID);
			fm->chip_id = gChipID;
		}

		// disable the FM chip for power saving
		ret = -1;
		retry = 0;
		while((ret < 0) && (retry < retryMax)){
			FM_DEBUG("%s() disable the FM chip in RDA5990P, retry time: %d\n", __func__, (retry + 1));
			ret = rda_fm_power_off();
			retry++;
		}
		if(ret < 0){
			FM_ERROR("%s() power off the FM chip in RDA5990P failed!\n", __func__);
			return -EPERM;
		}
		FM_DEBUG("%s() power off the FM chip in RDA5990P success!\n", __func__);
	}


	// if chip_id is RDA5820NS_CHIPID, enable the FM chip in combo
	if(fm->chip_id == RDA5820NS_CHIPID || fm->chip_id == RDA5801_CHIPID){
		ret = -1;
		retry = 0;
		while((ret < 0) && (retry < retryMax)){
			FM_DEBUG("%s() power on the FM chip in RDA5990P, retry time: %d\n", __func__, (retry + 1));
			ret = rda_fm_power_on();
			retry++;
		}
		if(ret < 0){
			FM_ERROR("%s() power on the FM chip in combo failed!\n", __func__);
			return -EPERM;
		}
		msleep(2*FM_SLEEP_TIME);
	}

	//Reset RDA FM
	tRegValue = 0x0002;
	ret = hal_write(client, 0x02, tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_write failed!\n", __func__);
		return -EPERM;
	}
	msleep(2*FM_SLEEP_TIME);

	if (RDA5820NS_CHIPID == gChipID || RDA5801_CHIPID == gChipID){
		if(RDA5820NS_CHIPID == gChipID) {
			if(workMode == FM_RECEIVER) {
				size = sizeof(RDA5820NS_RX_initialization_reg)/sizeof(RDA_FM_REG_T);
				init_regs = &RDA5820NS_RX_initialization_reg[0];
			}
			else {
				size = sizeof(RDA5820NS_TX_initialization_reg)/sizeof(RDA_FM_REG_T);
				init_regs = &RDA5820NS_TX_initialization_reg[0];
			}
		}
		else if(RDA5801_CHIPID == gChipID) {
			if(workMode == FM_RECEIVER) {
				size = sizeof(RDA5801_RX_initialization_reg)/sizeof(RDA_FM_REG_T);
				init_regs = &RDA5801_RX_initialization_reg[0];
			}
			else {
				size = sizeof(RDA5801_TX_initialization_reg)/sizeof(RDA_FM_REG_T);
				init_regs = &RDA5801_TX_initialization_reg[0];
			}
		}
		for (i = 0; i < size; i++)
		{
			if(init_regs[i].address == 0xFF){
				msleep(init_regs[i].value);
			}else{
				ret = hal_write(client, init_regs[i].address, init_regs[i].value);
				if (ret < 0)
				{
					FM_DEBUG("fm_powerUp init failed!\n");
					parm->err = FM_FAILED;
					return -EPERM;
				}
			}
		}
	}


	FM_DEBUG("pwron ok\n");
	fm->powerup = true;

	if (fm_tune(fm, parm) < 0)
	{
		FM_ERROR("%s() fm_tune failed!\n", __func__);
		return -EPERM;
	}

	parm->err = FM_SUCCESS;

	return 0;

}

/*
 *  fm_powerDown
 */
static int fm_powerDown(struct fm *fm)
{
	uint16_t tRegValue = 0;
	int ret = -1;
	int retry=0, retryMax = RETRY_MAX;
	struct i2c_client *client = fm->i2c_client;

	ret = hal_read(client, 0x02, &tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_read failed!\n", __func__);
		return -EPERM;
	}

	tRegValue &= (~(1 << 15));
	ret = hal_write(client, 0x02, tRegValue);//clear DHIZ first to avoid pop noise
	tRegValue &= (~(1 << 0));
	ret = hal_write(client, 0x02, tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_write failed!\n", __func__);
		return -EPERM;
	}

	if(fm->chip_id == RDA5820NS_CHIPID || fm->chip_id == RDA5801_CHIPID){
		ret = -1;
		retry = 0;
		while((ret < 0) && (retry < retryMax)){
			FM_DEBUG("%s() disable the FM chip in RDA5990P, retry time: %d\n", __func__, (retry + 1));
			ret = rda_fm_power_off();
			retry++;
		}
		if(ret < 0){
			FM_ERROR("%s() rda_fm_power_off failed!\n", __func__);
			return -EPERM;
		}
	}

	fm->powerup = false;
	FM_DEBUG("pwrdown ok\n");

	return 0;
}

/*
 *  fm_seek
 */
static int fm_seek(struct fm *fm, struct fm_seek_parm *parm)
{
	int ret = 0;
	uint16_t val = 0;
	uint8_t space = 1;
	uint16_t tFreq = 875;
	uint16_t tRegValue = 0;
	uint16_t bottomOfBand = 875;
	int falseStation = -1;


	struct i2c_client *client = fm->i2c_client;

	if (!fm->powerup)
	{
		FM_ERROR("%s() FM is power off, cann't seek!\n", __func__);
		parm->err = FM_BADSTATUS;
		return -EPERM;
	}

	if (parm->space == FM_SPACE_100K)
	{
		space = 1;
		val &= (~(0x03<<0)); //clear bit[0:1]
	}
	else if (parm->space == FM_SPACE_200K)
	{
		space = 2;
		val &= (~(1<<1)); //clear bit[1]
		val |= (1<<0);    //set bit[0]
	}
	else
	{
		parm->err = FM_EPARM;
		return -EPERM;
	}

	if (parm->band == FM_BAND_UE)
	{
		val &= (~(0x3<<2)); //clear bit[2:3]
		bottomOfBand = 875;
		fm->min_freq = 875;
		fm->max_freq = 1080;
	}
	else if (parm->band == FM_BAND_JAPAN) 
	{
		val &= (~(1<<3)); //clear bit[3]
		val |= (1 << 2);  //set bit[2]
		bottomOfBand = 760;
		fm->min_freq = 760;
		fm->max_freq = 910;
	}
	else if (parm->band == FM_BAND_JAPANW) {
		val &= (~(1<<2)); //clear bit[2]
		val |= (1 << 3);  //set bit[3]
		bottomOfBand = 760;
		fm->min_freq = 760;
		fm->max_freq = 1080;
	}
	else
	{
		FM_ERROR("band:%d out of range\n", parm->band);
		parm->err = FM_EPARM;
		return -EPERM;
	}

	if (parm->freq < fm->min_freq || parm->freq > fm->max_freq) {
		FM_ERROR("freq:%d out of range\n", parm->freq);
		parm->err = FM_EPARM;
		return -EPERM;
	}

	if (parm->seekth > 0x0B) {
		FM_ERROR("seekth:%d out of range\n", parm->seekth);
		parm->err = FM_EPARM;
		return -EPERM;
	}

	ret = hal_read(client, 0x05, &tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_read failed!\n", __func__);
		return -EPERM;
	}

	tRegValue &= (~(0x7f<<8));
	//tRegValue |= ((parm->seekth & 0x7f) << 8);
	tRegValue |= ((FM_SEEK_THRESHOLD & 0x7f) << 8);
	ret = hal_write(client, 0x05, tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_write failed!\n", __func__);
		return -EPERM;
	}


#ifdef FMDEBUG
	if (parm->seekdir == FM_SEEK_UP)
		FM_DEBUG("seek %d up\n", parm->freq);
	else
		FM_DEBUG("seek %d down\n", parm->freq);
#endif

	// (1) set hmute bit
	ret = hal_mute(client);
	if(ret < 0){
		FM_ERROR("%s() hal_mute failed!\n", __func__);
		return -EPERM;
	}

	tFreq = parm->freq;

	do {
		if (parm->seekdir == FM_SEEK_UP)
			tFreq += space;
		else
			tFreq -= space;

		if (tFreq > fm->max_freq)
			tFreq = fm->min_freq;
		if (tFreq < fm->min_freq)
			tFreq = fm->max_freq;

		val = ((((tFreq - bottomOfBand) + 5) << 6) | (1 << 4) | (val & 0x0f));
		ret = hal_write(client, 0x03, val);
		if(ret < 0){
			FM_ERROR("%s() hal_write failed!\n", __func__);
			return -EPERM;
		}
		msleep(FM_SLEEP_TIME);
		ret = hal_read(client, 0x0B, &tRegValue);
		if (ret < 0)
		{
			FM_DEBUG("fm_seek: read register failed tunning freq = %4X\n", tFreq);
			falseStation = -1;
		}
		else
		{
			if ((tRegValue & 0x0100) == 0x0100)
				falseStation = 0;
			else
				falseStation = -1;
		}

		if(falseStation == 0)
			break;

	}while(tFreq != parm->freq);


	//clear hmute
	ret = hal_unmute(client);
	if(ret < 0){
		FM_ERROR("%s() hal_unmute failed!\n", __func__);
		return -EPERM;
	}

	if (falseStation == 0) // seek successfully
	{    
		parm->freq = tFreq;
		FM_DEBUG("fm_seek success, freq:%d\n", parm->freq);
		parm->err = FM_SUCCESS;
	}
	else
	{
		FM_ERROR("fm_seek failed, invalid freq\n");
		parm->err = FM_SEEK_FAILED;
		ret = -1;
	}

	return ret;
}

/*
 *  fm_scan
 */
static int  fm_scan(struct fm *fm, struct fm_scan_parm *parm)
{
	int ret = 0;
	uint16_t tRegValue = 0;
	uint16_t scandir = FM_SEEK_UP; //scandir 搜索方向
	uint8_t space = 1; 
	struct i2c_client *client = fm->i2c_client;

	if (!fm->powerup){
		parm->err = FM_BADSTATUS;
		return -EPERM;
	}

	ret = hal_read(client, 0x03, &tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_read failed!\n", __func__);
		return -EPERM;
	}

	if (parm->space == FM_SPACE_100K){
		space = 1;
		tRegValue &= (~(0x03<<0)); //clear bit[0:1]
	}else if (parm->space == FM_SPACE_200K) {
		space = 2;
		tRegValue &= (~(1<<1)); //clear bit[1]
		tRegValue |= (1<<0);    //set bit[0]
	}else{
		//default
		space = 1;
		tRegValue &= (~(0x03<<0)); //clear bit[0:1]
	}

	if(parm->band == FM_BAND_UE){
		tRegValue &= (~(0x3<<2)); //clear bit[2:3]
		fm->min_freq = 875;
		fm->max_freq = 1080;
	}else if(parm->band == FM_BAND_JAPAN){
		tRegValue &= (~(1<<3)); //clear bit[3]
		tRegValue |= (1 << 2); //set bit[2]
		fm->min_freq = 760;
		fm->max_freq = 900;
	}else if(parm->band == FM_BAND_JAPANW){
		tRegValue &= (~(1<<2)); //clear bit[2]
		tRegValue |= (1 << 3); //set bit[3]
		fm->min_freq = 760;
		fm->max_freq = 1080;
	}else{
		parm->err = FM_EPARM;
		return -EPERM;
	}

	//set space and band
	ret = hal_write(client, 0x03, tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_write failed!\n", __func__);
		return -EPERM;
	}
	msleep(FM_SLEEP_TIME);


	if(hal_scan(client, fm->min_freq, fm->max_freq, &(parm->freq), parm->ScanTBL, &(parm->ScanTBLSize), scandir, space)){
		parm->err = FM_SUCCESS;
	}else{
		parm->err = FM_SCAN_FAILED;
	}

	return ret;
}


static int fm_setVol(struct fm *fm, uint32_t vol)
{
	int ret = 0;
	uint16_t tRegValue = 0;
	struct i2c_client *client = fm->i2c_client;

	if (vol > 15)
		vol = 15;

	FM_DEBUG("fm_setVol:%d\n", vol);

	ret = hal_read(client, 0x05, &tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_read failed!\n", __func__);
		return -EPERM;
	}

	if(vol > 0x0F){
		FM_ERROR("%s() vol is out of range!\n", __func__);
		return -EPERM;
	}

	tRegValue &= ~(0x0f);
	tRegValue |= vol;

	ret = hal_write(client, 0x05, tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_write failed!\n", __func__);
		return -EPERM;
	}

	return 0;
}

static int fm_getVol(struct fm *fm, uint32_t *vol)
{
	int ret = 0;
	uint16_t tRegValue;
	struct i2c_client *client = fm->i2c_client;

	ret = hal_read(client, 0x05, &tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_read failed!\n", __func__);
		return -EPERM;
	}

	*vol = (tRegValue & 0x0F);

	return 0;
}

static int fm_getRssi(struct fm *fm, int32_t *rssi)
{
	int ret = 0;
	uint16_t tRegValue;
	struct i2c_client *client = fm->i2c_client;

	ret = hal_read(client, 0x0B, &tRegValue);
	if(ret < 0){
		FM_ERROR("%s() hal_read failed!\n", __func__);
		return -EPERM;
	}


	*rssi = (uint32_t)((tRegValue >> 9) & RDAFM_RSSI_MASK);

	FM_DEBUG("rssi value:%d\n", *rssi);

	return 0;
}

/*
 *  fm_tune
 */
static int fm_tune(struct fm *fm, struct fm_tune_parm *parm)
{
	int ret;
	uint16_t val = 0;
	uint8_t space = 1;
	uint16_t bottomOfBand = 875;

	struct i2c_client *client = fm->i2c_client;

	FM_DEBUG("%s\n", __func__);

	if (!fm->powerup)
	{
		parm->err = FM_BADSTATUS;
		return -EPERM;
	} 

	if (parm->space == FM_SPACE_100K)
	{
		space = 1;
		val &= (~(0x03<<0)); //clear bit[0:1]
	}
	else if (parm->space == FM_SPACE_200K)
	{
		space = 2;
		val &= (~(1<<1)); //clear bit[1]
		val |= (1<<0);    //set bit[0]
	}
	else
	{
		parm->err = FM_EPARM;
		return -EPERM;
	}

	if (parm->band == FM_BAND_UE)
	{
		val &= (~(0x3<<2)); //clear bit[2:3]
		bottomOfBand = 875;
		fm->min_freq = 875;
		fm->max_freq = 1080;
	}
	else if (parm->band == FM_BAND_JAPAN) 
	{
		val &= (~(1<<3)); //clear bit[3]
		val |= (1 << 2);  //set bit[2]
		bottomOfBand = 760;
		fm->min_freq = 760;
		fm->max_freq = 910;
	}
	else if (parm->band == FM_BAND_JAPANW) {
		val &= (~(1<<2)); //clear bit[2]
		val |= (1 << 3);  //set bit[3]
		bottomOfBand = 760;
		fm->min_freq = 760;
		fm->max_freq = 1080;
	}
	else
	{
		FM_ERROR("band:%d out of range\n", parm->band);
		parm->err = FM_EPARM;
		return -EPERM;
	}

	if (parm->freq < fm->min_freq || parm->freq > fm->max_freq) {
		FM_ERROR("freq:%d out of range\n", parm->freq);
		parm->err = FM_EPARM;
		return -EPERM;
	}

	FM_DEBUG("fm_tune, freq:%d\n", parm->freq);

	//hal_mute(client);

	val = (((parm->freq - bottomOfBand + 5) << 6) | (1 << 4) | (val & 0x0f));

	ret = hal_write(client, 0x03, val);
	if (ret < 0)
	{
		FM_ERROR("fm_tune write freq failed\n");
		parm->err = FM_TUNE_FAILED;
		return ret;
	}
	msleep(FM_SLEEP_TIME);

	return ret;
}

static int fm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -1;
	FM_DEBUG("fm_i2c_probe\n");    
	if ((err = fm_init(client)))
	{
		FM_ERROR("fm_init ERR:%d\n", err);
		goto ERR_EXIT;
	}   

	return 0;   

ERR_EXIT:
	return err;    
}

static int fm_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	FM_DEBUG("fm_i2c_detect\n");
	strcpy(info->type, RDAFM_DEV);
	return 0;
}

static int fm_i2c_remove(struct i2c_client *client)
{
	int err = 0;
	struct fm *fm = i2c_get_clientdata(client);

	FM_DEBUG("fm_i2c_remove\n");
	if(fm)
	{    
		fm_destroy(fm);
		fm = NULL;
	}

	return err;
}

#ifdef I2C_BOARD_INFO_REGISTER_STATIC
int  i2c_static_add_device(struct i2c_board_info *info)
{
	struct i2c_adapter *adapter;
	struct i2c_client  *client;
	int    ret; 

	adapter = i2c_get_adapter(I2C_BUS_NUM);
	if (!adapter) {
		FM_DEBUG("%s: can't get i2c adapter\n", __func__);
		ret = -ENODEV;
		goto i2c_err;
	}    

	client = i2c_new_device(adapter, info);
	if (!client) {
		FM_DEBUG("%s:  can't add i2c device at 0x%x\n",
				__FUNCTION__, (unsigned int)info->addr);
		ret = -ENODEV;
		goto i2c_err;
	}    

	i2c_put_adapter(adapter);

	return 0;

i2c_err:
	return ret;
}
#endif //I2C_BOARD_INFO_REGISTER_STATIC


/*
 *  rda_fm_init
 */
static int __init rda_fm_init(void)
{
	int err = -1;
	FM_DEBUG("rda_fm_init\n");
#ifdef I2C_BOARD_INFO_REGISTER_STATIC
	err = i2c_static_add_device(&i2c_rdafm);
	if (err < 0){
		FM_DEBUG("%s(): add i2c device error, err = %d\n", __func__, err);
		return err;
	}
#endif //I2C_BOARD_INFO_REGISTER_STATIC

	// i2c_register_board_info(I2C_BUS_NUM, &i2c_rdafm, 1);
	// Open I2C driver
	err = i2c_add_driver(&RDAFM_driver);
	if (err)
	{
		FM_ERROR("i2c err\n");
	}

	return err;   
}

/*
 *  rda_fm_exit
 */
static void __exit rda_fm_exit(void)
{
	FM_DEBUG("mt_fm_exit\n");
#ifdef I2C_BOARD_INFO_REGISTER_STATIC
	i2c_unregister_device(g_fm_struct->i2c_client);
#endif //I2C_BOARD_INFO_REGISTER_STATIC

	i2c_del_driver(&RDAFM_driver); 
}

module_init(rda_fm_init);
module_exit(rda_fm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RDA FM Driver");
MODULE_AUTHOR("Naiquan Hu <naiquanhu@rdamicro.com>");


