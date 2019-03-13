/*******************************************************************************
 * Source file : RDAFM_fm_ctl.c
 * Description : RDAFM FM Receiver driver for linux.
 * Date        : 05/11/2011
 * 
 * Copyright (C) 2011 Spreadtum Inc.
 *
 ********************************************************************************
 * Revison
 2011-05-11  aijun.sun   initial version 
 *******************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_ARCH_SC8810
#include <mach/ldo.h>
#endif

//#define I2C_BOARD_INFO_REGISTER_STATIC
//#define USE_OUR_OWN_PWR_CTL

#define RDAFM_DEBUG 0

#define RDAFM_WAKEUP_CHECK_TIME         100   // ms 
#define RDAFM_WAKEUP_TIMEOUT            800   

#define RDAFM_SEEK_CHECK_TIME           50    // ms 

#define RDAFM_TUNE_DELAY                50    // ms 

#define I2C_RETRY_DELAY                   5
#define I2C_RETRIES                       3

#define FMLOG_FUNC		{ printk("[RDA5990FM] FUNC: %s\n", __func__); }

#ifdef	USE_OUR_OWN_PWR_CTL
	#define RDAFMPWR_DEVNAME   "rdafmpwr"
	int rda_fm_power_on(struct i2c_client *rdafmpwr_client);
	int rda_fm_power_off(struct i2c_client *rdafmpwr_client);
	static struct i2c_client *rdafmpwr_client;
#else
	extern int rda_fm_power_on(void);
	extern int rda_fm_power_off(void);
#endif

#define	RDAFM_DEV_NAME	"rdafm"
#define RDAFM_I2C_NAME    RDAFM_DEV_NAME
#define RDAFM_I2C_ADDR    0x11   //write addr 0x22  read addr 0x23

  /** The following define the IOCTL command values via the ioctl macros */
#define	RDAFM_FM_IOCTL_BASE     'R'
#define	RDAFM_FM_IOCTL_ENABLE		   _IOW(RDAFM_FM_IOCTL_BASE, 0, int)
#define RDAFM_FM_IOCTL_GET_ENABLE  _IOW(RDAFM_FM_IOCTL_BASE, 1, int)
#define RDAFM_FM_IOCTL_SET_TUNE    _IOW(RDAFM_FM_IOCTL_BASE, 2, int)
#define RDAFM_FM_IOCTL_GET_FREQ    _IOW(RDAFM_FM_IOCTL_BASE, 3, int)
#define RDAFM_FM_IOCTL_SEARCH      _IOW(RDAFM_FM_IOCTL_BASE, 4, int[4])
#define RDAFM_FM_IOCTL_STOP_SEARCH _IOW(RDAFM_FM_IOCTL_BASE, 5, int)
#define RDAFM_FM_IOCTL_MUTE        _IOW(RDAFM_FM_IOCTL_BASE, 6, int)
#define RDAFM_FM_IOCTL_SET_VOLUME  _IOW(RDAFM_FM_IOCTL_BASE, 7, int)
#define RDAFM_FM_IOCTL_GET_VOLUME  _IOW(RDAFM_FM_IOCTL_BASE, 8, int)


#ifdef I2C_BOARD_INFO_REGISTER_STATIC
//  #if (defined(CONFIG_MACH_SP6820A) || defined(CONFIG_MACH_SP8810))
 // #define I2C_STATIC_BUS_NUM        (4)

  //#define RDAFM_VDD_FROM_LDO      
 // #else
  #define I2C_STATIC_BUS_NUM        (0)
  //#endif

static struct i2c_board_info RDAFM_i2c_boardinfo = {
    I2C_BOARD_INFO(RDAFM_I2C_NAME, RDAFM_I2C_ADDR),
};
#endif


struct RDAFM_drv_data {
    struct i2c_client *client;
//    struct class      fm_class;
    int               opened_before_suspend;
    int               bg_play_enable; /* enable/disable background play. */
    struct mutex      mutex;
    atomic_t          fm_opened;
    atomic_t          fm_searching;
    int               current_freq;
    int               current_volume;
    u8                muteOn;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend; 
#endif
};

typedef struct
{
	uint8_t	address;
	uint16_t	value;
}RDA_FM_REG_T; 

struct RDAFM_drv_data *RDAFM_dev_data = NULL;

/***
 * Common i2c read and write function based i2c_transfer()
 *  The read operation sequence:
 *  7 bit chip address and Write command ("0") -> 8 bit
 *  register address n -> 7 bit chip address and Read command("1")
 *  The write operation sequence:
 * 7 bit chip address and Write command ("0") -> 8 bit
 * register address n -> write data n [15:8] -> write data n [7:0] 
 ***/
static int RDAFM_i2c_read(struct RDAFM_drv_data *cxt, u8 * buf, int len)
{
    int err = 0;
    int tries = 0;

    struct i2c_msg	msgs[] = {
        {
            .addr = cxt->client->addr,
            .flags = cxt->client->flags & I2C_M_TEN,
            .len = 1,
            .buf = buf,
        },
        {
            .addr = cxt->client->addr,
            .flags = (cxt->client->flags & I2C_M_TEN) | I2C_M_RD,
            .len = len,
            .buf = buf,
        },
    };

    do {
        err = i2c_transfer(cxt->client->adapter, msgs, 2);
        if (err != 2)
            msleep_interruptible(I2C_RETRY_DELAY);
    } while ((err != 2) && (++tries < I2C_RETRIES));

    if (err != 2) {
        dev_err(&cxt->client->dev, "Read transfer error\n");
        err = -EIO;
    } else {
        err = 0;
    }

    return err;
}


static int RDAFM_i2c_write(struct RDAFM_drv_data *cxt, u8 * buf, int len)
{
    int err = 0;
    int tries = 0;

    struct i2c_msg msgs[] = { 
        { 
            .addr = cxt->client->addr,
            .flags = cxt->client->flags & I2C_M_TEN,
            .len = len, 
            .buf = buf, 
        },
    };

    do {
        err = i2c_transfer(cxt->client->adapter, msgs, 1);
        if (err != 1)
            msleep_interruptible(I2C_RETRY_DELAY);
    } while ((err != 1) && (++tries < I2C_RETRIES));

    if (err != 1) {
        dev_err(&cxt->client->dev, "write transfer error\n");
        err = -EIO;
    } else {
        err = 0;
    }

    return err;
}


/**
 * Notes: all register are 16bit wide, so the register R&W function uses 
 *   2-byte arguments. */
static int RDAFM_register_read(struct RDAFM_drv_data *cxt, 
        u8 reg_address, 
        u16 *value) 
{
    int  ret = -EINVAL;
    u8   buf[8] = {0};

    buf[0] = reg_address;
    ret = RDAFM_i2c_read(cxt, buf, 2);
    if (ret >= 0) {
        *value = (buf[0]<<8)|(buf[1]); //MSB first
    }

    return ret;
}


static int RDAFM_register_write(struct RDAFM_drv_data *cxt, 
        u8 reg_address,
        u16 value)
{
    int  ret = -EINVAL;
    u8   buf[8] = {0};

    buf[0] = reg_address;
    buf[1] = value >> 8; // MSB first
    buf[2] = value & 0xff;
    ret = RDAFM_i2c_write(cxt, buf, 3);

    return ret;    
}


u8 FMshadowReg[80] = {
0xc4, 0x01, 0x00, 0x00, 0x04, 0x00, 0x86, 0xad, 0x00, 0x00, 0x42, 0xc6
};
const RDA_FM_REG_T RDA5802HcwFMDefault[]={
{0x02,0xC401}, //02H:         BIT10 ÷√∏ﬂ ºÊ»›1.8V SAMSUNG FLASH
{0x03,0x0000},
{0x04,0x0400},
{0x05,0x86ad}, //05H://86    
{0x06,0x4000},
{0x07,0x56C6},
{0x08,0x0000},
{0x09,0x0000},
{0x0a,0x0000},  //0X0AH
{0x0b,0x0000},
{0x0c,0x0000},
{0x0d,0x0000},
{0x0e,0x0000},
{0x0f,0x0000},
{0x10,0x0006},  //0X10H
{0x11,0x0019},
{0x12,0x2A11},
{0x13,0x002E},
{0x14,0x2A30},
{0x15,0xB83C},  //0X15H
{0x16,0x9000},
{0x17,0x2A91},
{0x18,0x8412},
{0x19,0x00A8},
{0x1a,0xC400},  //0X1AH
{0x1b,0xE000},
{0x1c,0x301D},  ///1cH
{0x1d,0x816A},
{0x1e,0x4608},
{0x1f,0x0086},  //0X1FH
{0x20,0x0661},// 20H
{0x21,0x0000},
{0x22,0x109E},
{0x23,0x24C9},//  23H
{0x24,0x0408},
{0x25,0x0608},  //0X25H
{0x26,0xE105},
{0x27,0x3B6C},
{0x28,0x2BEC},
{0x29,0x090F},
{0x2a,0x3414},  //0X2AH
{0x2b,0x1450},
{0x2c,0x096D},
{0x2d,0x2D96},
{0x2e,0x01DA},
{0x2f,0x2A7B},
{0x30,0x0821},   //0X30H
{0x31,0x13D5},
{0x32,0x4891},
{0x33,0x00bc},
{0x34,0x0896},//0830//0x34h
{0x35,0x153c},
{0x36,0x0b80},
{0x37,0x25c7},
{0x38,0x0000},
};
const RDA_FM_REG_T RDA5802N_initialization_reg[]={
	{0x02,0xc401}, //02h  
	{0x03,0x0000},        
	{0x04,0x0400},        
	{0x05,0x86ad}, //05h//
	{0x06,0x0000},        
	{0x07,0x42c6},        
	{0x08,0x0000},        
	{0x09,0x0000},        
	{0x0a,0x0000},  //0x0ah
	{0x0b,0x0000},        
	{0x0c,0x0000},                      
	{0x0d,0x0000},                      
	{0x0e,0x0000},                      
	{0x0f,0x0000},                      
	{0x10,0x0000},  //0x10h             
	{0x11,0x0019},                      
	{0x12,0x2a11},                      
	{0x13,0xa053},//0x80,0x53,          
	{0x14,0x3e11},//0x22,0x11,	      
	{0x15,0xfc7d},  //0x15h             
	{0x16,0xc000},                      
	{0x17,0x2a91},                      
	{0x18,0x9400},                      
	{0x19,0x00a8},                      
	{0x1a,0xc400},  //0x1ah             
	{0x1b,0xe000},                      
	{0x1c,0x2b1d}, //0x23,0x14          
	{0x1d,0x816a},                      
	{0x1e,0x4608},                      
	{0x1f,0x0086},                      
	{0x20,0x0661},  //0x20h             
	{0x21,0x0000},                      
	{0x22,0x109e},                      
	{0x23,0x2244},                      
	{0x24,0x0408},  //0x24              
	{0x25,0x0408},  //0x25    
};


static int RDAFM_check_chip_id(struct RDAFM_drv_data *cxt, u16 *chip_id) {
    int ret = 0;
	RDA_FM_REG_T  RDA5802_REG;
    /* read chip id of RDAFM */
	
	RDA5802_REG.address=0x02;
	RDA5802_REG.value=0x0002;
	RDAFM_register_write(cxt, RDA5802_REG.address, RDA5802_REG.value);
	msleep(25);
	ret = RDAFM_register_read(cxt, 0x0c, chip_id);
	printk("<%s:%d>ret[%d].111\n", __func__, __LINE__, *chip_id);

    if (ret < 0) {
        dev_err(&cxt->client->dev, "Read chip id failed.\n");
    }
    else {
        dev_info(&cxt->client->dev, "RDAFM chip id:0x%04x\n", *chip_id);//5802n  id :0x0c_reg  should be  0x5803  0x0e_reg should be 0x5808
    }

    return ret;
}


#ifdef RDAFM_VDD_FROM_LDO
/*
 * On sp6820, chip vdd is from internel ldo of 8810.
*/
static void RDAFM_chip_vdd_input(bool turn_on)
{
    if (turn_on) {
        printk("### %s turn on LDO_SIM2\n", __func__);
    	LDO_SetVoltLevel(LDO_LDO_SIM2, LDO_VOLT_LEVEL0);
    	LDO_TurnOnLDO(LDO_LDO_SIM2);
        msleep(1);
    }
    else {
        printk("### %s turn off LDO_SIM2\n", __func__);
	    LDO_TurnOffLDO(LDO_LDO_SIM2);
    }
	printk("<%s:%d>turn_on[%d].\n", __func__, __LINE__, turn_on);
}

#endif

#ifdef OLD_POWER_ON
/* NOTES:
 * This function is private call by 'probe function', so not add re-entry
 * protect. */
static int RDAFM_fm_power_on(struct RDAFM_drv_data *cxt)
{
    u16     reg_value        = 0x0;
    int     check_times      = 0;
    int     ret              = -EINVAL;
    ulong   jiffies_comp     = 0;
     RDA_FM_REG_T  RDA5802_REG;
    u8      i,j;
    dev_info(&cxt->client->dev, "%s\n", __func__);

#ifdef RDAFM_VDD_FROM_LDO
    /* turn on vdd */
    RDAFM_chip_vdd_input(true);
#endif

		printk("<%s:%d>\n", __func__, __LINE__);
		RDA5802_REG.address=0x02;
		RDA5802_REG.value=0xc401;
		
		RDAFM_register_write(cxt, RDA5802_REG.address, RDA5802_REG.value);
		msleep(25);   //delay 25 miliseconds
		RDA5802_REG.value=0x0002;
		RDAFM_register_write(cxt, RDA5802_REG.address, RDA5802_REG.value);
		printk("<%s:%d>\n", __func__, __LINE__);
		msleep(25);	 //delay 25 miliseconds
		RDA5802_REG.value=0xc401;
		RDAFM_register_write(cxt, RDA5802_REG.address, RDA5802_REG.value);

		msleep(25);	 //delay 25 miliseconds

	//printk("<%s:%d>[%d]\n", __func__, __LINE__, sizeof(RDA5802HcwFMDefault)/sizeof(RDA5802HcwFMDefault[0]));	
		for(i=0;i<sizeof(RDA5802HcwFMDefault)/sizeof(RDA5802HcwFMDefault[0]);i++)
		{
			//printk("<%s:%d>i[%d]\n", __func__, __LINE__, i);
			RDAFM_register_write(cxt, RDA5802HcwFMDefault[i].address, RDA5802HcwFMDefault[i].value);
			//printk("<%s:%d>value[%x]\n", __func__, __LINE__, RDA5802HcwFMDefault[i].value);
			//FMshadowReg[2*i]=(RDA5802HcwFMDefault[i].value&0xff00) >> 8;
			//FMshadowReg[2*i+1]=RDA5802HcwFMDefault[i].value&0x00ff;
			//printk("<%s:%d>[%x][%x]\n", __func__, __LINE__,FMshadowReg[2*i], FMshadowReg[2*i + 1]);
			msleep(5);  //delay 5 miliseconds
			//printk("<%s:%d>i[%d]\n", __func__, __LINE__, i);
		}
//	printk("<%s:%d>i[%d]\n", __func__, __LINE__, i);
    return 0;
}

#endif /* OLD_POWER_ON */

static int RDAFM_fm_close(struct RDAFM_drv_data *cxt)
{
    int ret = -EINVAL;
    RDA_FM_REG_T  RDA5802_REG;

	FMLOG_FUNC;
	
    printk(KERN_ERR "!!!! FM CLOSE");
	RDA5802_REG.address=0x02;
	RDA5802_REG.value=RDA5802HcwFMDefault[0].value&0x00;
	
	ret=RDAFM_register_write(cxt, RDA5802_REG.address, RDA5802_REG.value);
	
	#ifdef RDAFM_VDD_FROM_LDO
	    /* turn off vdd */
	    RDAFM_chip_vdd_input(false);
	#endif
	    
    atomic_cmpxchg(&cxt->fm_opened, 1, 0);
    dev_info(&cxt->client->dev, "FM close: RDAFM will run standby.\n");

    return ret;
}


/**
 * Notes: Before this call, device must be power on.
 **/
static int RDAFM_fm_open(struct RDAFM_drv_data *cxt)
{
	FMLOG_FUNC;
	
    if (atomic_read(&cxt->fm_opened)) {
        dev_info(&cxt->client->dev, 
            "FM open: already opened, ignore this operation\n");
        return 0;
    }

    atomic_cmpxchg(&cxt->fm_opened, 0, 1);

    dev_err(&cxt->client->dev, "FM open: FM is opened\n");

    return 0;
}


static u16 RDA5802_FreqToChan(uint16_t frequency) 
{
	u8 channelSpacing = 1;
	u16 bottomOfBand = 870;
	u16 channel = 0;

	if ((FMshadowReg[3] & 0x0c) == 0x00) 
		bottomOfBand = 870;
	else if ((FMshadowReg[3]& 0x0c) == 0x04)	
		bottomOfBand = 760;
	else if ((FMshadowReg[3] & 0x0c) == 0x08)	
		bottomOfBand = 760;
	else if((FMshadowReg[3] & 0x0c) == 0x0c)
		bottomOfBand = 650;
	if ((FMshadowReg[3]& 0x03) == 0x00) 
		channelSpacing = 1;				//100KHz step
	else if ((FMshadowReg[3]& 0x03) == 0x01) 
		channelSpacing = 2;

	channel = (frequency - bottomOfBand) / channelSpacing;
	return (channel);
}


void RDA5802_SetFreq( struct RDAFM_drv_data *cxt,u16 curFreq )
{   
	u16 RDA5802_channel_start_tune[] ={0xc4,0x01,0x00,0x10}; 	//87.0MHz
	u16 curChan;
	RDA_FM_REG_T  RDA5802_REG[2]={{0x02,0x0},{0x03,0x0}};
	u8  i;
	
	curChan=RDA5802_FreqToChan(curFreq);//return (freq-870)/1
	
	//SetNoMute
	FMshadowReg[0] |=	1<<6;//FMshadowReg[0] = 0xc4;
	
	RDA5802_channel_start_tune[0]=FMshadowReg[0];  //0xc4
	RDA5802_channel_start_tune[1]=FMshadowReg[1];  //0x01
	RDA5802_channel_start_tune[2]=curChan>>2;		 //0x02(879)
	RDA5802_channel_start_tune[3]=(((curChan&0x0003)<<6)|0x10) | (FMshadowReg[3]&0x0f);	//set tune bit

	RDA5802_REG[0].value=RDA5802_channel_start_tune[0]<<8|RDA5802_channel_start_tune[1];
	RDA5802_REG[1].value=RDA5802_channel_start_tune[2]<<8|RDA5802_channel_start_tune[3];

		for(i=0;i<sizeof(RDA5802_REG)/sizeof(RDA5802_REG[0]);i++)
		{
			RDAFM_register_write(cxt, RDA5802_REG[i].address, RDA5802_REG[i].value);
			msleep(5);  //delay 5 miliseconds
		}

}
/**
 * Notes:Set FM tune, the frequency 100KHz unit.
 **/
static int RDAFM_fm_set_tune(struct RDAFM_drv_data *cxt, u16 frequency)
{
    //u16  channel = 0;
    //u16  reg_value_0A=0x0;
    u16  reg_value_0B= 0x0;
    int  ret = -EPERM;
   u8   falseStation=0,i=0;

	FMLOG_FUNC;

    if (!atomic_read(&cxt->fm_opened)) {
        dev_err(&cxt->client->dev, "Set tune: FM not open\n");
        return ret;
    }


	RDA5802_SetFreq(cxt,frequency);
	
#if (RDAFM_DEBUG)    
    dev_info(&cxt->client->dev, "Set tune: channel=%d, frequency=%d\n",
            channel, frequency);
#endif


	msleep(40);   	//delay 40 miliseconds

	//waiting for FmReady
	do
	{
		i++;
		if(i>10) return 0; 
		msleep(20);
		//read REG0A&0B	
		 //RDAFM_register_read(cxt, 0x0a, &reg_value_0A);
		 RDAFM_register_read(cxt, 0x0b, &reg_value_0B);
	}while((reg_value_0B&0x0080)==0);
  	
	//check FM_TURE
	if((reg_value_0B&0x0100)==0) falseStation=1;
	
	if(frequency==960) falseStation=1;
	
	if (falseStation==1)
		return 0;            //the current chanel is a station
	else 
		return 1;	     //the current chanel is not  a station

}


/*
 ** NOTES: Get tune frequency, with 100KHz unit.
 */
static int RDAFM_fm_get_frequency(struct RDAFM_drv_data *cxt)
{
    u16 reg_value = 0;
    u16 frequency = 0;
    int       ret = -EPERM;

	FMLOG_FUNC;

    if (!atomic_read(&cxt->fm_opened)) {
        dev_err(&cxt->client->dev, "Get frequency: FM not open\n");
        return ret;
    }

    RDAFM_register_read(cxt, 0x03, &reg_value);

    /* freq (MHz) = 50KHz X CHAN + 64MHz ==> freq (100KHz) = 0.5 * ch + 640 */
    frequency  =  ((reg_value & 0xffc0) >> 6)  + 870;

    dev_info(&cxt->client->dev, "Get frequency %d\n", frequency);

    return frequency;
}


/* 
 * NOTES: Start searching process. Different "from RDAFM_fm_full_search",
 * this function just do seek, NOT read channel found. */
static int RDAFM_fm_do_seek(struct RDAFM_drv_data *cxt, 
        u16 frequency, 
        u8  seek_dir)
{
    u16 reg_value = 0x0;
    int  ret = -EPERM;

#if (RDAFM_DEBUG)
    dev_info(&cxt->client->dev, 
            "%s, frequency %d, seekdir %d\n", __func__, frequency, seek_dir);
#endif    
    if (!atomic_read(&cxt->fm_opened)) {
        dev_err(&cxt->client->dev, "Do seek: FM not open\n");
        return ret;
    }

/*
    if (atomic_read(&cxt->fm_searching)) {
        dev_err(&cxt->client->dev, "Seeking is not stoped.%s\n", __func__);
        return -EBUSY;
    }
*/
    mutex_lock(&cxt->mutex);    

    /* Set start frequency */
    ret=RDAFM_fm_set_tune(cxt, frequency);

	reg_value=RDA5802HcwFMDefault[0].value;
    /* Set seek direction */
    if (seek_dir == 0/*RDAFM_SEEK_DIR_UP*/) {
        reg_value |= 1<<9;
    }
    else {
        reg_value &= ~(1<<9);
    }

    /* Seeking start */
    reg_value |= 1<<8;
    RDAFM_register_write(cxt, RDA5802HcwFMDefault[0].address, reg_value);

    mutex_unlock(&cxt->mutex);

    return 0;
}


/* NOTES:
 * Stop fm search and clear search status */
static int RDAFM_fm_stop_search(struct RDAFM_drv_data *cxt)
{
    int ret = -EPERM;
    u16 reg_value = 0x0;

	FMLOG_FUNC;

    if (atomic_read(&cxt->fm_searching)) {
        
        atomic_cmpxchg(&cxt->fm_searching, 1, 0);

        /* clear seek enable bit of seek register. */
        RDAFM_register_read(cxt, RDA5802HcwFMDefault[0].address, &reg_value);
        if (reg_value & (1<<8)) {
            reg_value &= ~ (1<<8);
            RDAFM_register_write(cxt, RDA5802HcwFMDefault[0].address, reg_value);
        }

#if (RDAFM_DEBUG)        
        dev_info(&cxt->client->dev, "%s, search stopped", __func__);
#endif        
        ret = 0;
    }

    return ret;
}


/* NOTES: 
 * Search frequency from current frequency, if a channel is found, the 
 * frequency will be read out.
 * This function is timeout call. If no channel is found when time out, a error 
 * code will be given. The caller can retry search(skip "do seek")   to get 
 * channel. */
static int RDAFM_fm_full_search(struct RDAFM_drv_data *cxt, 
        u16  frequency, 
        u8   seek_dir,
        u32  time_out,
        u16 *freq_found)
{
    int      ret               = -EPERM;
    u16      reg_value         = 0x0;
    ulong    jiffies_comp      = 0;
    u8       is_timeout;
    u8       is_search_end;

	FMLOG_FUNC;

    if (!atomic_read(&cxt->fm_opened)) {
        dev_err(&cxt->client->dev, "Full search: FM not open\n");
        return ret;
    }

    /* revese seek dir. Temporarily to fit the application. */
    seek_dir ^= 0x1;

    if (!atomic_read(&cxt->fm_searching)) {
        atomic_cmpxchg(&cxt->fm_searching, 0, 1);

        if (frequency == 0)
        {
            RDAFM_fm_do_seek(cxt, cxt->current_freq, seek_dir);
        }
        else
            RDAFM_fm_do_seek(cxt, frequency, seek_dir);
    }
    else
    {
#if (RDAFM_DEBUG)
        dev_info(&cxt->client->dev, "%s, busy searching!", __func__);
#endif
        return -EBUSY;
    }

    jiffies_comp = jiffies;
    do {
        /* search is stopped manually */
        if (atomic_read(&cxt->fm_searching) == 0)
            break;
        
        if (msleep_interruptible(RDAFM_SEEK_CHECK_TIME) ||
                signal_pending(current)) 
            break;

    //    RDAFM_register_read(cxt, RDAFM_REG_STATUSA, &reg_value);
		
	RDAFM_register_read(cxt, 0x0b, &reg_value);


        is_search_end = (0x0080 == 
                (0x0080 & reg_value));

        is_timeout = time_after(jiffies, 
                jiffies_comp + msecs_to_jiffies(time_out));
    }while(is_search_end ==0 && is_timeout == 0);

    /* If search is not completed, need to re-search 
       or stop seek (RDAFM_fm_stop_seek) */
    if (is_search_end) {
        if (0 == (0x0100 & reg_value)) 
            ret = -EAGAIN;
        else
            ret = 0;
    }
    else {
        ret = -EAGAIN;
    }

    atomic_cmpxchg(&cxt->fm_searching, 1, 0);

    *freq_found = RDAFM_fm_get_frequency(cxt);
    cxt->current_freq = *freq_found;

#if (RDAFM_DEBUG)
    dev_info(&cxt->client->dev, 
            "%s: ret %d, seek_dir %d, timeout %d, seek_end %d, freq %d\n",
            __func__, ret, seek_dir, is_timeout, is_search_end, *freq_found);
#endif
    return ret;
}


/* NOTES: Set fm output side volume 
* control register: 0x4, volume control register. (3:0)
* 0000 --  MUTE 
* 0001 -- -42dBFS
* 0010 -- -39dbFS 
* 1111 --  FS */
static int RDAFM_fm_set_volume(struct RDAFM_drv_data *cxt, u8 volume)
{
    int ret       =  -EPERM;
    RDA_FM_REG_T  RDA5802_REG;

	FMLOG_FUNC;

    FMshadowReg[7]=(FMshadowReg[7] & 0xf0 ) | volume;    //volume:0~15 
//    ret=i2c_master_send(cxt->client, &(FMshadowReg[0]), 8);
	RDA5802_REG.address=0x05;
	RDA5802_REG.value=FMshadowReg[6]<<8|FMshadowReg[7];
	ret = RDAFM_register_write(cxt, RDA5802_REG.address, RDA5802_REG.value);
    if (ret == 0) 
    {
        cxt->current_volume = volume;
    }

    return ret;
}


static int RDAFM_fm_get_volume(struct RDAFM_drv_data *cxt)
{
    u16 reg_value  = 0x0;

	FMLOG_FUNC;

    if (!atomic_read(&cxt->fm_opened)) {
        dev_err(&cxt->client->dev, "Get volume: FM not open\n");
        return -EPERM;
    }

     RDAFM_register_read(cxt, 0x05, &reg_value);
     reg_value &= 0x000f;

    return reg_value;
}

static atomic_t pwrstat = ATOMIC_INIT(1);

static int RDAFM_fm_misc_open(struct inode *inode, struct file *filep)
{
    int ret = -EINVAL;

	FMLOG_FUNC;

    if (!atomic_dec_and_test(&pwrstat)) {
	    atomic_inc(&pwrstat);
	    return -EBUSY;
    }
#ifdef USE_OUR_OWN_PWR_CTL
    rda_fm_power_on(rdafmpwr_client);
#else
    rda_fm_power_on();
#endif

	printk("<%s:%d>\n", __func__, __LINE__);
    ret = nonseekable_open(inode, filep);
    if (ret < 0) {
        pr_err("RDAFM open misc device failed.\n");
        return ret;
    }
	printk("<%s:%d>\n", __func__, __LINE__);
    filep->private_data = RDAFM_dev_data;

    return 0;
}


//static int RDAFM_fm_misc_ioctl(struct inode *inode, struct file *filep,
//        unsigned int cmd, unsigned long arg)
static long RDAFM_fm_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)

{
    void __user              *argp       = (void __user *)arg;
    int                       ret        = 0;
    int                       iarg       = 0;  
    int                       buf[4]     = {0};
    struct RDAFM_drv_data  *dev_data   = file->private_data;

	printk("%s: ioctl = 0x%x, 0x%x\n", __func__, cmd, RDAFM_FM_IOCTL_ENABLE);

    switch (cmd) {
        case RDAFM_FM_IOCTL_ENABLE:
			printk("%s: test 111\n", __func__);
            if (copy_from_user(&iarg, argp, sizeof(iarg)) || iarg > 1) {
                ret =  -EFAULT;
            }
			printk("%s: test 222\n", __func__);
            if (iarg == 1) {
                ret = RDAFM_fm_open(dev_data);
            }
            else {
                ret = RDAFM_fm_close(dev_data);
            }
 
            break;

        case RDAFM_FM_IOCTL_GET_ENABLE:
            iarg = atomic_read(&dev_data->fm_opened);
            if (copy_to_user(argp, &iarg, sizeof(iarg))) {
                ret = -EFAULT;
            }
            break;

        case RDAFM_FM_IOCTL_SET_TUNE:
            if (copy_from_user(&iarg, argp, sizeof(iarg))) {
                ret = -EFAULT;
            }
            ret = RDAFM_fm_set_tune(dev_data, iarg);
            break;

        case RDAFM_FM_IOCTL_GET_FREQ:
            iarg = RDAFM_fm_get_frequency(dev_data);
            if (copy_to_user(argp, &iarg, sizeof(iarg))) {
                ret = -EFAULT;
            }
            break;

        case RDAFM_FM_IOCTL_SEARCH:
            if (copy_from_user(buf, argp, sizeof(buf))) {
                ret = -EFAULT;
            }
            ret = RDAFM_fm_full_search(dev_data, 
                    buf[0], /* start frequency */
                    buf[1], /* seek direction*/
                    buf[2], /* time out */
                    (u16*)&buf[3]);/* frequency found will be stored to */
	    if (copy_to_user( ((int*)argp + 3), &buf[3], sizeof(u32))) {
		ret = -EFAULT;
	    }

            break;

        case RDAFM_FM_IOCTL_STOP_SEARCH:
            ret = RDAFM_fm_stop_search(dev_data);
            break;

        case RDAFM_FM_IOCTL_MUTE:
            break;

        case RDAFM_FM_IOCTL_SET_VOLUME:
            if (copy_from_user(&iarg, argp, sizeof(iarg))) {
                ret = -EFAULT;
            }
            ret = RDAFM_fm_set_volume(dev_data, (u8)iarg);
            break;            

        case RDAFM_FM_IOCTL_GET_VOLUME:
            iarg = RDAFM_fm_get_volume(dev_data);
            if (copy_to_user(argp, &iarg, sizeof(iarg))) {
                ret = -EFAULT;
            }
            break;            

        default:
			printk("%s: test fff\n", __func__);
            return -EINVAL;
    }

    return ret;
}

static int RDAFM_fm_misc_release(struct inode *inode, struct file *file)
{
	FMLOG_FUNC;

#ifdef USE_OUR_OWN_PWR_CTL
	rda_fm_power_off(rdafmpwr_client);
#else
	rda_fm_power_off();
#endif
	atomic_inc(&pwrstat);
	return 0;
}

static const struct file_operations RDAFM_fm_misc_fops = {
    .owner = THIS_MODULE,
    .open  = RDAFM_fm_misc_open,
    .unlocked_ioctl = RDAFM_fm_misc_ioctl,
    .release = RDAFM_fm_misc_release,
};

static struct miscdevice RDAFM_fm_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = RDAFM_DEV_NAME,
    .fops  = &RDAFM_fm_misc_fops,
};


/*------------------------------------------------------------------------------
 * RDAFM class attribute method
 ------------------------------------------------------------------------------*/
static ssize_t RDAFM_fm_attr_open(struct class *class,
		struct class_attribute *attr, const char *buf, size_t size)
{
    u8 open;
	printk("<%s:%d>\n", __func__, __LINE__);
    if (size) {
        open = simple_strtol(buf, NULL, 10);
        if (open)
            RDAFM_fm_open(RDAFM_dev_data);
        else 
            RDAFM_fm_close(RDAFM_dev_data);
    }

    return size;
}


static ssize_t RDAFM_fm_attr_get_open(struct class *class,
		struct class_attribute *attr, char *buf)
{
    u8 opened;
printk("<%s:%d>\n", __func__, __LINE__);
    opened = atomic_read(&RDAFM_dev_data->fm_opened);
    if (opened) 
        return sprintf(buf, "Opened\n");
    else
        return sprintf(buf, "Closed\n");
}


static ssize_t RDAFM_fm_attr_set_tune(struct class *class,
		struct class_attribute *attr, const char *buf, size_t size)
{
    u16 frequency;
printk("<%s:%d>\n", __func__, __LINE__);
    if (size) {
        frequency = simple_strtol(buf, NULL, 10);/* decimal string to int */


        RDAFM_fm_set_tune(RDAFM_dev_data, frequency);
    }

    return size;
}

static ssize_t RDAFM_fm_attr_get_frequency(struct class *class,
		struct class_attribute *attr, char *buf)
{
    u16 frequency;
printk("<%s:%d>\n", __func__, __LINE__);
    frequency = RDAFM_fm_get_frequency(RDAFM_dev_data);

    return sprintf(buf, "Frequency %d\n", frequency);
}


static ssize_t RDAFM_fm_attr_search(struct class *class,
		struct class_attribute *attr, const char *buf, size_t size)
{
    u32 timeout;
    u16 frequency;
    u8  seek_dir;
    u16 freq_found;
    char *p = (char*)buf;
    char *pi = NULL;
printk("<%s:%d>\n", __func__, __LINE__);
    if (size) {
        while (*p == ' ') p++;
        frequency = simple_strtol(p, &pi, 10); /* decimal string to int */
        if (pi == p) goto out;

        p = pi;
        while (*p == ' ') p++;
        seek_dir = simple_strtol(p, &pi, 10);
        if (pi == p) goto out;

        p = pi;
        while (*p == ' ') p++;
        timeout = simple_strtol(p, &pi, 10);
        if (pi == p) goto out;

        RDAFM_fm_full_search(RDAFM_dev_data, frequency, seek_dir, timeout, &freq_found);
    } 

out:
    return size;
}


static ssize_t RDAFM_fm_attr_set_volume(struct class *class,
		struct class_attribute *attr, const char *buf, size_t size)
{
    u8 volume;
printk("<%s:%d>\n", __func__, __LINE__);
    if (size) {
        volume = simple_strtol(buf, NULL, 10);/* decimal string to int */

        RDAFM_fm_set_volume(RDAFM_dev_data, volume);
    }

    return size;
}

static ssize_t RDAFM_fm_attr_get_volume(struct class *class,
		struct class_attribute *attr, char *buf)
{
    u8 volume;
printk("<%s:%d>\n", __func__, __LINE__);
    volume = RDAFM_fm_get_volume(RDAFM_dev_data);

    return sprintf(buf, "Volume %d\n", volume);
}

static struct class_attribute RDAFM_fm_attrs[] = {
    __ATTR(fm_open,   S_IRUGO|S_IWUGO, RDAFM_fm_attr_get_open,      RDAFM_fm_attr_open),
    __ATTR(fm_tune,   S_IRUGO|S_IWUGO, RDAFM_fm_attr_get_frequency, RDAFM_fm_attr_set_tune),
    __ATTR(fm_seek,   S_IWUGO,         NULL,                          RDAFM_fm_attr_search),
    __ATTR(fm_volume, S_IRUGO|S_IWUGO, RDAFM_fm_attr_get_volume,    RDAFM_fm_attr_set_volume),
    {},     
};

void RDAFM_fm_sysfs_init(struct class *class)
{
    class->class_attrs = RDAFM_fm_attrs;
}


/*------------------------------------------------------------------------------ 
 * RDAFM i2c device driver.
 ------------------------------------------------------------------------------*/

static int __devexit RDAFM_remove(struct i2c_client *client)
{
    struct RDAFM_drv_data  *cxt = i2c_get_clientdata(client);

    RDAFM_fm_close(cxt);
    misc_deregister(&RDAFM_fm_misc_device);
//    class_unregister(&cxt->fm_class);
    kfree(cxt);

    dev_info(&client->dev, "%s\n", __func__);

    return 0;    
}


static int RDAFM_resume(struct i2c_client *client)
{
    struct RDAFM_drv_data *cxt = i2c_get_clientdata(client);

    dev_info(&cxt->client->dev, "%s, FM opened before suspend: %d\n", 
        __func__, cxt->opened_before_suspend);

    if (cxt->opened_before_suspend) {
        cxt->opened_before_suspend = 0;
        
        RDAFM_fm_open(cxt);

        RDAFM_fm_set_volume(cxt, cxt->current_volume);
    }

    return 0;
}


static int RDAFM_suspend(struct i2c_client *client, pm_message_t mesg)
{
    struct RDAFM_drv_data *cxt = i2c_get_clientdata(client);

    dev_info(&cxt->client->dev, "%s, FM opend: %d\n", 
        __func__, atomic_read(&cxt->fm_opened));

    if (atomic_read(&cxt->fm_opened) && cxt->bg_play_enable == 0) {
        cxt->opened_before_suspend = 1;
        RDAFM_fm_close(cxt);
    }

    return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND

static void RDAFM_early_resume (struct early_suspend* es)
{
#ifdef RDAFM_DEBUG
    printk("%s.\n", __func__);
#endif    
    if (RDAFM_dev_data) {
        RDAFM_resume(RDAFM_dev_data->client);
    }
}

static void RDAFM_early_suspend (struct early_suspend* es)
{
#ifdef RDAFM_DEBUG
    printk("%s.\n", __func__);
#endif    
    if (RDAFM_dev_data) {
        RDAFM_suspend(RDAFM_dev_data->client, (pm_message_t){.event=0});
    }
}

#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef	USE_OUR_OWN_PWR_CTL
static u8 isBigEnded = 0;
static int i2c_write_1_addr_2_data(struct i2c_client* client, const u8 addr, const u16 data)
{
    unsigned char  DATA[3];
    int ret = 0;

    if(!isBigEnded)
        {
	        DATA[0] = addr;
            DATA[1] = data >> 8;
            DATA[2] = data >> 0;
        }
    else
        {
	        DATA[0] = addr;
      	    DATA[1] = data >> 0;
            DATA[2] = data >> 8;
        }

    ret = i2c_master_send(client, (char*)DATA, 3);
    if (ret < 0)
    {
        printk(KERN_INFO "***i2c_write_1_addr_2_data send:0x%X err:%d bigendia: %d \n", addr,ret, isBigEnded);
        return -1;
    }

    return 0;
}

static int i2c_read_1_addr_2_data(struct i2c_client* client, const u8 addr, u16* data)
{
    unsigned char DATA[2];
    int ret = 0;

    ret = i2c_master_send(client, (char*)&addr, 1);
    if (ret < 0)
    {
        printk(KERN_INFO "***i2c_read_1_addr_2_data send:0x%X err:%d\n", addr,ret);
        return -1;
    }

    ret = i2c_master_recv(client, DATA, 2);
    if (ret < 0)
    {
        printk(KERN_INFO "***i2c_read_1_addr_2_data recv:0x%X err:%d\n", addr,ret);
        return -1;
    }
    
     if(!isBigEnded)
     {
         *data = (DATA[0] << 8) | DATA[1];
     }
     else
     {
         *data = (DATA[1] << 8) | DATA[0];
     }
    return 0;
}

int rda_fm_power_on(struct i2c_client *rdafmpwr_client)
{
    u16 temp_data = 0;
    int ret = 0;

	FMLOG_FUNC;

    //gfayn rda_fm_client -> fm_wifi_rf_client
    if(!rdafmpwr_client)
    {
        printk(KERN_INFO "rda_fm_power_on failed on:i2c client \n");
        return -1;
    }


    ret=i2c_write_1_addr_2_data(rdafmpwr_client,0x3f, 1);
    if(ret) {
    	printk(KERN_INFO "write 0x3f ret = %d", ret);
	goto err;
    }

    ret = i2c_read_1_addr_2_data(rdafmpwr_client,0x22, &temp_data);
    if (ret) {
    	printk(KERN_INFO "read 0x22 ret = %d", ret);
        goto err;
    }
    printk(KERN_INFO "***0xA2 readback value:0x%X \n", temp_data);

    temp_data &= ~0x8000;

    ret=i2c_write_1_addr_2_data(rdafmpwr_client,0x22,temp_data);
    if(ret)
     	goto err;   		



    ret = i2c_read_1_addr_2_data(rdafmpwr_client,0x22, &temp_data);
    if (ret) {
    	printk(KERN_INFO "read 0x22 ret = %d", ret);
        goto err;
    }
    printk(KERN_INFO "***0xA2 readback value:0x%X \n", temp_data);



    ret=i2c_write_1_addr_2_data(rdafmpwr_client,0x3f, 0);
    if(ret)
     	goto err;   		
    printk(KERN_INFO "rda_fm_power_on_succeed!! \n");
    
    return 0;

err:
    printk(KERN_INFO "rda_fm_power_on_failed\n");

    return -1;
}

int rda_fm_power_off(struct i2c_client *rdafmpwr_client)
{
    u16 temp_data = 0;
    int ret = 0;

	FMLOG_FUNC;

    if(!rdafmpwr_client)
    {
        printk(KERN_INFO "rda_fm_power_off failed on:i2c client \n");
        return -1;
    }

    ret=i2c_read_1_addr_2_data(rdafmpwr_client,0xA2, &temp_data);
    if(ret)
   	 goto err;
    printk(KERN_INFO "***0xA2 readback value:0x%X \n", temp_data);

    temp_data |= 0x8000;

    ret=i2c_write_1_addr_2_data(rdafmpwr_client,0xA2,temp_data);
    if(ret)
     	goto err;   		
    
    printk(KERN_INFO "rda_fm_power_off_succeed!! \n");

    return 0;

err:
    printk(KERN_INFO "rda_fm_power_off_failed\n");
    
    return -1;
}
#endif

static int RDAFM_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    u16    reg_value = 0x0;
    int    ret = -EINVAL;

    struct RDAFM_drv_data *cxt = NULL;

#ifdef USE_OUR_OWN_PWR_CTL
    if (strcmp(id->name, RDAFMPWR_DEVNAME) == 0) {
	    rdafmpwr_client = client;
	    return 0;
    }
#endif

    printk(KERN_ERR "i2c_device_id: %s", id->name);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "RDAFM driver: client is not i2c capable.\n");
        ret = -ENODEV;
        goto i2c_functioality_failed;
    }

    cxt = kmalloc(sizeof(struct RDAFM_drv_data), GFP_KERNEL);
    if (cxt == NULL) {
        dev_err(&client->dev, "Can't alloc memory for module data.\n");
        ret = -ENOMEM;
        goto alloc_data_failed;
    }

    mutex_init(&cxt->mutex);
    mutex_lock(&cxt->mutex);

    cxt->client = client;
    i2c_set_clientdata(client, cxt);

    atomic_set(&cxt->fm_searching, 0);
    atomic_set(&cxt->fm_opened, 0);

    RDAFM_check_chip_id(cxt, &reg_value);

#if (RDAFM_DEBUG)
//    RDAFM_read_all_registers(cxt);
#endif

    RDAFM_fm_close(cxt);

//    cxt->fm_class.owner = THIS_MODULE;
//    cxt->fm_class.name = "fm_class";
//    RDAFM_fm_sysfs_init(&cxt->fm_class);
//    ret = class_register(&cxt->fm_class);
//    if (ret < 0) {
//        dev_err(&client->dev, "RDAFM class init failed.\n");
//        goto class_init_failed;
//    }

    ret = misc_register(&RDAFM_fm_misc_device);
    if (ret < 0) {
        dev_err(&client->dev, "RDAFM misc device register failed.\n");
        goto misc_register_failed;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    cxt->early_suspend.suspend = RDAFM_early_suspend;
    cxt->early_suspend.resume  = RDAFM_early_resume;
    cxt->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    register_early_suspend(&cxt->early_suspend);
#endif
    cxt->opened_before_suspend = 0;
    cxt->bg_play_enable = 1;
    cxt->current_freq = 870; /* init current frequency, search may use it. */
    
    RDAFM_dev_data = cxt;

    mutex_unlock(&cxt->mutex);

    return ret; 
misc_register_failed:
    misc_deregister(&RDAFM_fm_misc_device);
//class_init_failed:    
//    class_unregister(&cxt->fm_class);
    mutex_unlock(&cxt->mutex);
    kfree(cxt);
alloc_data_failed:
i2c_functioality_failed:
    dev_err(&client->dev,"RDAFM driver init failed.\n");
    return ret;
}

static const struct i2c_device_id RDAFM_i2c_id[] = { 
#ifdef USE_OUR_OWN_PWR_CTL
    { RDAFMPWR_DEVNAME, 0}, 
#endif
    { RDAFM_I2C_NAME, 0 }, 
    { },
};

MODULE_DEVICE_TABLE(i2c, RDAFM_i2c_id);

static struct i2c_driver RDAFM_i2c_driver = {
    .driver = {
        .name = RDAFM_I2C_NAME,
    },
    .probe    =  RDAFM_probe,
    .remove   =  __devexit_p(RDAFM_remove),
    //replaced by early suspend and resume.
    //.resume   =  RDAFM_resume,
    //.suspend  =  RDAFM_suspend,
    .id_table =  RDAFM_i2c_id,
};
#ifdef I2C_BOARD_INFO_REGISTER_STATIC

int i2c_static_add_device(struct i2c_board_info *info)
{
    struct i2c_adapter *adapter;
    struct i2c_client  *client;
    int    ret;

    adapter = i2c_get_adapter(I2C_STATIC_BUS_NUM);
    if (!adapter) {
        pr_err("%s: can't get i2c adapter\n", __func__);
        ret = -ENODEV;
        goto i2c_err;
    }

    client = i2c_new_device(adapter, info);
    if (!client) {
        pr_err("%s:  can't add i2c device at 0x%x\n",
                __FUNCTION__, (unsigned int)info->addr);
        ret = -ENODEV;
        goto i2c_err;
    }

    i2c_put_adapter(adapter);

    return 0;

i2c_err:
    return ret;
}

#endif /* I2C_BOARD_INFO_REGISTER_STATIC */ 

#define RDA5990_FM_I2C_CHANNEL		0
static struct i2c_board_info rda5990_fm_i2c_boardinfo = {
    I2C_BOARD_INFO(RDAFM_I2C_NAME, RDAFM_I2C_ADDR),
};

static int __init RDAFM_driver_init(void)
{
    int  ret = 0;        

    printk("RDAFM driver: init\n");

#ifdef I2C_BOARD_INFO_REGISTER_STATIC
    ret = i2c_static_add_device(&RDAFM_i2c_boardinfo);
    if (ret < 0) {
        pr_err("%s: add i2c device error %d\n", __func__, ret);
        goto init_err;
    }
#endif

	i2c_register_board_info(RDA5990_FM_I2C_CHANNEL, &rda5990_fm_i2c_boardinfo, 1);
    if((ret = i2c_add_driver(&RDAFM_i2c_driver))) {
	    pr_err("%s: add rdafm driver error %d\n", __func__, ret);
	    goto init_err;
    }

    return 0;

init_err:
    return ret;
}

static void __exit RDAFM_driver_exit(void)
{
#if 1//def DEBUG
    printk("RDAFM driver exit\n");
#endif

#ifdef I2C_BOARD_INFO_REGISTER_STATIC
    i2c_unregister_device(RDAFM_dev_data->client);
#endif

    i2c_del_driver(&RDAFM_i2c_driver);
}

module_init(RDAFM_driver_init);
module_exit(RDAFM_driver_exit);

MODULE_DESCRIPTION("RDAFM FM radio driver");
MODULE_AUTHOR("RDA Micro Inc.");
MODULE_LICENSE("GPL");

