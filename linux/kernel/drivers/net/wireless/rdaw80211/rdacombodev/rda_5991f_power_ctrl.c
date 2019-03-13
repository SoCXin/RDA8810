#include "rda_combo.h"
#ifndef RDA_COMBO_FROM_FIRMWARE

#ifdef WLAN_USE_DCDC
static const u16 pmu_setting_5991f[][2] = {
	{0x3F,  0x0001},
#ifdef WLAN_USE_CRYSTAL
	{0x22,	0x21F3},
#else
	{0x22,	0xA1F3},
#endif
	{0x24,	0x8906}, //{0x24,  0x8908}  sleep mode with analog power on
	{0x26,	0x8055}, //0x8055
	{0x29,	0x1113},
	{0x32,	0x1113},
	{0x33,	0x0510},
	{0x39,	0xC208},
	{0x23,	0xA200}, //{0x23,  0x8200}
	{0x25,	0xA247}, //{0x25,  0x8247} heavy_load_dig
	{0x37,	0x0B8A},
	{0x3F,  0x0000},
};
#else
static const u16 pmu_setting_5991f[][2] = {
	{0x3F,  0x0001},
#ifdef WLAN_USE_CRYSTAL
	{0x22,	0x21F3},
#else
	{0x22,	0xA1F3},
#endif
	{0x24,	0x8906},
	{0x26,	0x8555},
	{0x29,	0x1113},
	{0x32,	0x1113},
	{0x33,	0x0510},
	{0x39,	0xC208},
	{0x23,	0x0200},
	{0x25,	0x027F},
	{0x3F,  0x0000},
};
#endif

/*add according to hongjun's new config*/
static const u16 soft_reset_5991f[][2] = {
	{0x3F, 0x0000},
	{0x30, 0x8000},
	{0x30, 0x0000},
};

static const u16 wf_en_5991f[][2] = {
	{0x3F, 0x0001},
	{0x31, 0x8B40},		//;WIFI_en=1
	{0x3F, 0x0000},
};

static const u16 wifi_disable_5991f[][2] = {
	{0x3F, 0x0001},
	{0x31, 0x0B40},		//;WIFI_en=0
	{0x26, 0x8055},
	{0x3F, 0x0000},
};

#ifdef COMBO_WITH_26MHZ
static const u16 wf_rf_setting_5991f[][2] = {
	{0x3F,  0x0000},
	{0x05,	0x0000},
	{0x06,	0x1124}, //wf_bbpll_cpaux_bit[2:0]=100
	{0x07,	0x0820},
	{0x10,	0x9ff7},
	{0x11,	0xFFFA},//filter bandwidth
	{0x13,	0x5054},
	{0x14,	0x988C},
	{0x15,	0x58e8},
	{0x16,	0x200B},
	{0x19,	0x9C01},
	{0x1C,	0x06E4},
	{0x1D,	0x3A8C},
	{0x22,	0xFF7B},
	{0x23,	0x283C},//dc cal
	{0x24,	0xA0C4},
	{0x28,	0x4320},
	{0x2A,	0x1036},//{0x2A,  0x0077}
	{0x2B,	0x41BB},
	{0x2D,	0xFF03},
	{0x2F,	0x15DE},
	{0x34,	0x3000},
	{0x35,	0x8011},
	{0x39,	0x6018},
	{0x3B,	0x3218},
	{0x3D,	0xFF00},//rxon delay
	{0x40,	0xFFFF},//wf_tmx_gain,wf_pabias;20140825
	{0x41,	0xFFFF},
	{0x7D,	0x4020},//paon delay
	{0x3F,  0x0001},
	{0x37,	0x0B8A},
	{0x3F,  0x0000},
	{0x30,	0x0100},
	{0x28,	0x4F20},
	 DELAY_MS(1)
	{0x28,	0x4320},
	{0x30,	0x0149},
};
#else
static const u16 wf_rf_setting_5991f[][2] = {
};
#endif

static const u16 wf_agc_setting_for_dccal_5991f[][2] = {

};

static const u16 wf_agc_setting_5991f[][2] = {
	{0x3F,  0x0000},
	{0x0F,	0x01F7},//lower max gain
	{0x0E,	0x01F0},
	{0x0D,	0x00F0},
	{0x0C,	0x0070},
	{0x0B,	0x0030},
	{0x0A,	0x0010},
	//{0x09,	0x3033},
	{0x09,	0x3031},//20140825
	{0x08,	0x0830},
	{0x3F,  0x0001},
	{0x07,	0x7030},
	{0x06,	0x7010},
	{0x05,	0x7870},
	{0x04,	0x7830},
	//{0x03,	0x7811},
	{0x03,	0x7810},//20140825
	{0x02,	0x7800},
	{0x01,	0x7800},
	{0x00,	0x7800},
	{0x3F,  0x0000},
};

static const u16 wf_calibration_5991f[][2] = {
	{0x3F,  0x0000},
	{0x30,	0x0148},
	{0x30,	0x0149},
	 DELAY_MS(50)
};

static const u16 fix_agc_gain_5991f[][2] = {
	{0x3F,  0x0000},
	{0x30,  0x0349},
};

static const u16 bt_rf_setting_5991f[][2] = {
	{0x3F,  0x0000},
#ifdef COMBO_WITH_26MHZ
	{0x2E,	0xCAB3},
#endif /* COMBO_WITH_26MHZ */
	{0x02,	0x0E00},
	{0x08,	0xEFFF},
	{0x0A,	0x09FF},
	{0x11,	0x00B5},
	{0x13,	0x07C0},
	{0x14,	0xFDC4},
	{0x18,	0x2010},
	{0x19,	0x7956},
	{0x1B,	0xDF7F},
	{0x26,	0x6640},
	{0x2B,	0x007F},
	{0x2C,	0x600F},
	{0x2D,	0x007F},
	{0x2F,	0x1000},
	{0x30,	0x0141},
	{0x37,	0x3333}, //PSK
	{0x38,	0x357A}, //PSK
	{0x3B,	0x1111}, //GFSK
	{0x3C,	0x1235}, //GFSK
	{0x3F,  0x0001},
	{0x00,	0xFFFF},
	{0x01,	0xFFE7}, // bt power
	{0x02,	0xFFFF},
	{0x03,	0xFFF7},
	{0x04,	0xFFFF},
	{0x05,	0xFFE5},
	{0x06,	0xFFFF},
	{0x07,	0xFFE5},
	{0x08,	0xFFFF},
	{0x09,	0xFFD1},
	{0x0A,	0xFFFF},
	{0x0B,	0xFFD1},
	//{0x39,	0x1208},
	{0x3F,  0x0000},
};

static const u16 control_mode_disable_5991f[][2] = {
	{0x3F,  0x0000},
	{0x30,	0x0341},
};

static const u16 bt_en_5991f[][2] = {
	{0x3F, 0x0001},
	{0x28, 0x85A1},		//;bt_en=1
	{0x3F, 0x0000},
};

static const u16 bt_disable_5991f[][2] = {
	{0x3F, 0x0001},
	{0x28, 0x05A1},		//;bt_en=1
	{0x3F, 0x0000},
};

// add for pta
static const u16 rda_5991f_bt_dc_ca_fix_gain[][2] =
{
	{0x3f, 0x0000 },
	{0x30, 0x0141 },
	{0x30, 0x0140 },
	{0x30, 0x0141 },
//	{0x30, 0x0341 },  //force gain level to 7 before lna issue fixed
#ifdef COMBO_WITH_26MHZ
	{0x2e, 0x8ab3 },  //force gain level to 7 before lna issue fixed
#endif
	{0x3f, 0x0000 },
};
// add for pta

// add for pta
static const u16 rda_5991f_bt_dc_ca_fix_gain_no_wf[][2] =
{
	{0x3f, 0x0000 },
	{0x30, 0x0140 },
	{0x30, 0x0141 },
	{0x3f, 0x0000 },
};
// add for pta

// add for pta
static const u16 rda_5991f_bt_force_swtrx[][2] =
{
	{0x3f, 0x0000 },
#ifdef COMBO_WITH_26MHZ
	{0x2e, 0xcab3 },  //force gain level to 7 before lna issue fixed
#endif
	{0x3f, 0x0000 },
};
// add for pta

// add for pta
static const u16 rda_5991f_bt_no_force_swtrx[][2] =
{
	{0x3f, 0x0000 },
#ifdef COMBO_WITH_26MHZ
	{0x2e, 0x8ab3 },  //force gain level to 7 before lna issue fixed
#endif
	{0x3f, 0x0000 },
};
// add for pta



static const u16  bt_dc_cal_5991f[][2] = {
	{0x3F,  0x0000},
	{0x30,  0x0140},
	{0x30,  0x0141},
	 DELAY_MS(50)
};
#endif

static int check_wifi_power_on(void)
{
	int ret = 0;
	u16 temp_data;

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);
	if (ret)
		goto err;

	ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x31, &temp_data);
	if (ret)
		goto err;

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);

	if (temp_data & 0x8000)
		return 1;
err:
	return 0;
}

static int check_bt_power_on(void)
{
	int ret = 0;
	u16 temp_data;

	ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0001);
	if (ret)
		goto err;

	ret = i2c_read_1_addr_2_data(rda_bt_rf_client, 0x28, &temp_data);
	if (ret)
		goto err;

	ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0000);
	if (temp_data & 0x8000)
		return 1;
err:
	return 0;
}

static int power_on(int isWifi)
{
	int ret = 0;
	ret = rda_write_data_to_rf(rda_wifi_rf_client, pmu_setting_5991f);
	if (ret)
		goto err;
	printk(KERN_INFO "%s write pmu_setting succeed!! \n", __func__);

	if (isWifi) {		// for wifi
		ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_en_5991f);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write wf_en succeed!! \n", __func__);
	} else {		// for bt
		ret = rda_write_data_to_rf(rda_bt_rf_client, bt_en_5991f);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write bt_en succeed!! \n", __func__);
	}

	rda_combo_i2c_unlock();
	msleep(5);
	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_wifi_rf_client, soft_reset_5991f);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write soft_reset succeed!! \n", __func__);

	rda_combo_i2c_unlock();
	msleep(10);
	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_rf_setting_5991f);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_rf_setting succeed!! \n", __func__);

	ret = rda_write_data_to_rf(rda_bt_rf_client, bt_rf_setting_5991f);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write bt_rf_setting succeed!! \n", __func__);

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_agc_setting_5991f);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_agc_setting succeed!! \n",
	       __func__);

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_calibration_5991f);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_calibration succeed!! \n", __func__);

	ret = rda_write_data_to_rf(rda_wifi_rf_client, fix_agc_gain_5991f);
	if(ret)
		goto power_off;
	printk(KERN_INFO "%s write fix_agc_gain succeed!! \n", __func__);
	return 0;
power_off:
	if (isWifi) {		// for wifi
		rda_write_data_to_rf(rda_wifi_rf_client, wifi_disable_5991f);
	} else {
		rda_write_data_to_rf(rda_bt_rf_client, bt_disable_5991f);
	}
err:
	return -1;
}

static int rda_5991f_wifi_debug_en(int enable)
{
	u16 temp_data = 0;
	int ret = 0;
	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);
	if (ret < 0)
		return -1;
	if (enable == 1) {
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x28, 0x80a1);
		if (ret < 0)
			return -1;
		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x39, &temp_data);
		if (ret < 0)
			return -1;

		ret =  i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39,
			temp_data | (1 << 2));
		if (ret < 0)
			return -1;
	} else {
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x28, 0x00a1);
		if (ret < 0)
			return -1;
		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x39, &temp_data);
		if (ret < 0)
			return -1;

		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39,
			temp_data & (~(1 << 2)));
		if (ret < 0)
			return -1;
	}

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);
	if (ret < 0)
		return -1;
	return ret;
}

int rda_5991f_wifi_power_on(void)
{
	int ret = 0, bt_power_on = 0;

	//if bt is power on wait until it's complete
	wait_for_completion(&rda_wifi_bt_comp);

	rda_combo_i2c_lock();
	enable_26m_regulator(CLOCK_WLAN);
	enable_32k_rtc(CLOCK_WLAN);
	enable_26m_rtc(CLOCK_WLAN);
	bt_power_on = check_bt_power_on();
	printk(KERN_INFO "%s bt_power_on=%d \n", __func__, bt_power_on);

	if (bt_power_on) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_en_5991f);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write wf_en succeed!! \n", __func__);

		//add for pta
		//handle btswtrx dr
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5991f_bt_no_force_swtrx);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write rda_5991_bt_no_force_swtrx succeed!! \n",
			__func__);
		// add for pta
		rda_combo_i2c_unlock();
		msleep(5);
		rda_combo_i2c_lock();
	} else {
		ret = power_on(1);
		if (ret)
			goto err;
	}

	ret = rda_write_data_to_rf(rda_wifi_rf_client, control_mode_disable_5991f);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write control_mode_disable succeed!! \n", __func__);
	if (check_test_mode()) {
		rda_5991f_wifi_debug_en(1);
		printk(KERN_INFO "%s: IN test mode, switch uart to WIFI succeed!! \n",
			__func__);
	}

	rda_combo_i2c_unlock();
	msleep(100);
	disable_26m_rtc(CLOCK_WLAN);
	complete(&rda_wifi_bt_comp);
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&wifi_fw_status, 1);
#endif
	return ret;

power_off:
	rda_write_data_to_rf(rda_wifi_rf_client, wifi_disable_5991f);
err:
	disable_26m_rtc(CLOCK_WLAN);
	disable_32k_rtc(CLOCK_WLAN);
	disable_26m_regulator(CLOCK_WLAN);
	rda_combo_i2c_unlock();
	return -1;
}

int rda_5991f_wifi_power_off(void)
{
	int ret = 0, bt_power_on = 0;
	rda_combo_i2c_lock();
	bt_power_on = check_bt_power_on();
	//add for pta
	if(bt_power_on){
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5991f_bt_force_swtrx);
		if (ret) {
			printk(KERN_INFO "%s  rda_5991_bt_force_swtrx failed!! \n",
				__func__);
		} else {
			printk(KERN_INFO "%s  rda_5991_bt_force_swtrx succeed!! \n",
				__func__);
		}
	}
	// add for pta
	ret = rda_write_data_to_rf(rda_wifi_rf_client, wifi_disable_5991f);
	if (ret) {
		printk(KERN_INFO "%s failed!! \n", __func__);
	} else {
		printk(KERN_INFO "%s succeed!! \n", __func__);
	}

	bt_power_on = check_bt_power_on();
	if (bt_power_on) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client, fix_agc_gain_5991f);
		printk(KERN_INFO "%s write fix_agc_gain succeed!! \n", __func__);
	}
	disable_26m_rtc(CLOCK_WLAN);
	disable_32k_rtc(CLOCK_WLAN);
	disable_26m_regulator(CLOCK_WLAN);
	rda_combo_i2c_unlock();
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&wifi_fw_status, 0);
	rda_release_firmware();
#endif
	return ret;
}

int rda_5991f_bt_power_on(void)
{
	int ret = 0, wifi_power_on = 0;

	//if wifi is power on wait until it's complete
	wait_for_completion(&rda_wifi_bt_comp);

	rda_combo_i2c_lock();
	enable_26m_regulator(CLOCK_BT);
	enable_26m_rtc(CLOCK_BT);
	enable_32k_rtc(CLOCK_BT);

	wifi_power_on = check_wifi_power_on();
	printk(KERN_INFO "%s wifi_power_on=%d \n", __func__, wifi_power_on);

	if (wifi_power_on) {
		ret = rda_write_data_to_rf(rda_bt_rf_client, bt_en_5991f);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write bt_en succeed!! \n", __func__);
		rda_combo_i2c_unlock();
		msleep(5);
		rda_combo_i2c_lock();
	} else {
		ret = power_on(0);
		if (ret)
			goto err;
	}

	printk(KERN_INFO "%s succeed!! \n", __func__);

	rda_combo_i2c_unlock();
	msleep(10);
	disable_26m_rtc(CLOCK_BT);
	complete(&rda_wifi_bt_comp);
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&bt_fw_status, 1);
#endif
	return ret;

err:
	disable_26m_rtc(CLOCK_BT);
	disable_32k_rtc(CLOCK_BT);
	disable_26m_regulator(CLOCK_BT);
	rda_combo_i2c_unlock();
	return -1;
}

int rda_5991f_bt_power_off(void)
{
	int ret = 0;
	rda_combo_i2c_lock();
	ret = rda_write_data_to_rf(rda_bt_rf_client, bt_disable_5991f);
	if (ret) {
		printk(KERN_INFO "%s failed!! \n", __func__);
	} else {
		printk(KERN_INFO "%s succeed!! \n", __func__);
	}
	disable_26m_rtc(CLOCK_BT);
	disable_32k_rtc(CLOCK_BT);
	disable_26m_regulator(CLOCK_BT);
	rda_combo_i2c_unlock();
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&bt_fw_status, 0);
	rda_release_firmware();
#endif
	return ret;
}
// add for pta
static int RDA5991f_bt_dc_cal_fix_gain(void)
{
	int ret = 0;
	int is_wfen;

	if(!rda_bt_rf_client){
		printk(KERN_INFO "rda_bt_rf_client is NULL!\n");
		return -1;
	}

	rda_combo_i2c_lock();
	is_wfen= check_wifi_power_on();
	//check the version and make sure this applies to 5991
	if(rda_wlan_version() == WLAN_VERSION_91_F) {
		if(is_wfen) {
			ret = rda_write_data_to_rf(rda_bt_rf_client,
				rda_5991f_bt_dc_ca_fix_gain);
		} else {
			ret = rda_write_data_to_rf(rda_bt_rf_client,
				rda_5991f_bt_dc_ca_fix_gain_no_wf);
		}

		if(ret)
			goto err;
	}

	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5991f_bt_dc_cal_fix_gain success!!!\n");
	msleep(200);   //200ms

	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5991f_bt_dc_cal_fix_gain failed! \n");
	return -1;

}
// add for pta
long rda_5991f_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case RDA_WIFI_POWER_ON_IOCTL:
		ret = rda_5991f_wifi_power_on();
		break;

	case RDA_WIFI_POWER_OFF_IOCTL:
		ret = rda_5991f_wifi_power_off();
		break;

	case RDA_BT_POWER_ON_IOCTL:
		ret = rda_5991f_bt_power_on();
		break;

	case RDA_BT_POWER_OFF_IOCTL:
		ret = rda_5991f_bt_power_off();
		break;

	case RDA_WIFI_DEBUG_MODE_IOCTL:
		{
			int enable = 0;
			if(copy_from_user(&enable, (void*)arg, sizeof(int))) {
				printk(KERN_ERR "copy_from_user enable failed!\n");
				return -EFAULT;
			}
			ret = rda_5991f_wifi_debug_en(enable);
			break;
		}
		// add for pta
	case RDA_BT_DC_CAL_IOCTL_FIX_5991_LNA_GAIN:
		ret = RDA5991f_bt_dc_cal_fix_gain();
		break;
		// add for pta
	default:
		break;
	}

	printk(KERN_INFO "rda_bt_pw_ioctl cmd=0x%02x \n", cmd);
	return ret;
}

int rda_5991f_fm_power_on(void)
{
	int ret = 0;
	u16 temp = 0;

	if (!rda_wifi_rf_client) {
		printk(KERN_INFO
			"rda_wifi_rf_client is NULL, rda_fm_power_on failed!\n");
		return -1;
	}

	enable_32k_rtc(CLOCK_FM);
	msleep(8);
	rda_combo_i2c_lock();

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);	// page down
	if (ret < 0) {
		printk(KERN_INFO
			"%s() write address(0x%02x) with value(0x%04x) failed! \n",
			__func__, 0x3f, 0x0001);
		goto err;
	}

	if (rda_wlan_version() == WLAN_VERSION_91_F) {

		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x27, &temp);	//read 0xA7
		if (ret < 0) {
			printk(KERN_INFO "%s() read from address(0x%02x) failed! \n",
				__func__, 0xA7);
			goto err;
		}
		temp = temp | 0x1;	//set bit[0]
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x27, temp);	//write back
		if (ret < 0) {
			printk(KERN_INFO
				"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0xA7, temp);
			goto err;
		}

		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x39, &temp);	//read 0xB9
		if (ret < 0) {
			printk(KERN_INFO  "%s() read from address(0x%02x) failed! \n",
				__func__, 0xB9);
			goto err;
		}

		temp = temp & 0x7fff; //set bit[15]=0
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39, temp); //write back
			if (ret < 0) {
				printk(KERN_INFO
					"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0xB9, temp);
			goto err;
		}

		temp = temp | (0x1 << 15);	//set bit[15]
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39, temp);	//write back
		if (ret < 0) {
			printk(KERN_INFO
				"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0xB9, temp);
			goto err;
		}
	}

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);	// page up
	if (ret < 0) {
		printk(KERN_INFO
			"%s() write address(0x%02x) with value(0x%04x) failed! \n",
			__func__, 0x3f, 0x0001);
		goto err;
	}

	rda_combo_i2c_unlock();
	return 0;

err:
	rda_combo_i2c_unlock();
	disable_32k_rtc(CLOCK_FM);
	printk(KERN_INFO "***rda_fm_power_on failed! \n");
	return -1;
}

int rda_5991f_fm_power_off(void)
{
	int ret = 0;
	u16 temp = 0;

	if (!rda_wifi_rf_client) {
		printk(KERN_INFO
		       "rda_wifi_rf_client is NULL, rda_fm_power_off failed!\n");
		return -1;
	}

	rda_combo_i2c_lock();

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);	// page down
	if (ret < 0) {
		printk(KERN_INFO
			"%s() write address(0x%02x) with value(0x%04x) failed! \n",
			__func__, 0x3f, 0x0001);
		goto err;
	}

	if (rda_wlan_version() == WLAN_VERSION_91_F) {
		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x27, &temp);	//read 0xA7
		if (ret < 0) {
			printk(KERN_INFO
				"%s() read from address(0x%02x) failed! \n",
				__func__, 0xA7);
			goto err;
		}
		temp = temp & ~(0x1);	//clear bit[0]
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x27, temp);	//write back
		if (ret < 0) {
			printk(KERN_INFO
				"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0xA7, temp);
			goto err;
		}
	}

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);	// page up
	if (ret < 0) {
		printk(KERN_INFO
			"%s() write address(0x%02x) with value(0x%04x) failed! \n",
			__func__, 0x3f, 0x0001);
		goto err;
	}

	rda_combo_i2c_unlock();
	disable_32k_rtc(CLOCK_FM);
	return 0;
err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_fm_power_off failed! \n");
	return -1;
}

