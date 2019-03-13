#include "rda_combo.h"

static u8 wifi_is_on = 0;

#ifndef RDA_COMBO_FROM_FIRMWARE

static const u16 wifi_off_data[][2] =
{
	{ 0x3F, 0x0001 }, //page up
	{ 0x31, 0x0B40 }, //power off wifi
	{ 0x3F, 0x0000 }, //page down
};

static const u16 wifi_en_data_90[][2] =
{
	{0x3f, 0x0001},
#ifdef WLAN_USE_DCDC 	/*houzhen update Mar 15 2012 */
	{0x23, 0x8F21},//20111001 higher AVDD voltage to improve EVM to 0x8f21 download current -1db 0x8fA1>>0x8bA1
#else
	{0x23, 0x0FA1},
#endif
	{0x31, 0x0B40 }, //power off wifi
//	  {0x22, 0xD3C7},//for ver.c 20111109, txswitch
	{0x24, 0x8048},//freq_osc_in[1:0]00  0x80C8 >> 0x80CB
	{0x27, 0x4925},//for ver.c20111109, txswitch
	//				  {0x28, 0x80A1}, //BT_enable
	{0x31, 0x8140},//enable wifi
	{0x32, 0x0113},//set_ rdenout_ldooff_wf=0; rden4in_ldoon_wf=1
	{0x33, 0x0507},//stable time chenggangdeng
	//				  {0x39, 0x0004},	//uart switch to wf
	{0x3F, 0x0000}, //page down
};

static const u16 wifi_dc_cal_data[][2]=
{
	{0x3f, 0x0000},
	{0x30, 0x0248},
	{0x30, 0x0249},
	//{wait 200ms; } here
};

static const u16 wifi_dig_reset_data_90[][2]=
{
	{0x3F,	0x0001},
	{0x31,	0x8D40},
	{0x31,	0x8F40},
	{0x31,	0x8b40},
	{0x3F,	0x0000},
};

static const u16 wifi_rf_init_data_90_verE[][2] =
{
	{0x3f, 0x0000},
	//{;;set_rf_swi},ch
	{0x05, 0x8000},
	{0x06, 0x0101},
	{0x07, 0x0101},
	{0x08, 0x0101},
	{0x09, 0x3040},
	{0x0A, 0x002C},//aain_0
	{0x0D, 0x0507},
	{0x0E, 0x2300},
	{0x0F, 0x5689},//
	//{;;//set_RF  },
	{0x10, 0x0f78},//20110824
	{0x11, 0x0602},
	{0x13, 0x0652},//adc_tuning_bit[011]
	{0x14, 0x8886},
	{0x15, 0x0910},
	{0x16, 0x049f},
#ifdef WLAN_USE_CRYSTAL
	{0x17, 0x0990},
	{0x18, 0x049F},
#else
	{0x17, 0x0910},
	{0x18, 0x249F},
#endif
	{0x19, 0x3C01},
	{0x1C, 0x0934},
	{0x1D, 0xFF00},//for ver.D20120119for temperature 70 degree
	//{0x1F, 0x01F8},//for ver.c20111109
	//{0x1F, 0x0300},//for burst tx ²»Ëø
	{0x20, 0x06E4},
	{0x21, 0x0ACF},//for ver.c20111109,dr dac reset,dr txflt reset
	{0x22, 0x24DC},
#ifdef WLAN_FOR_CTA
	{0x23, 0x0BFF},
#else
	{0x23, 0x23FF},
#endif
	{0x24, 0x00FC},
	{0x26, 0x004F},//004F >> 005f premote pa
	{0x27, 0x171D},///mdll*7
	{0x28, 0x031D},///mdll*7
#ifdef WLAN_USE_CRYSTAL
	{0x2A, 0x2860},//et0x2849-8.5p	:yd 0x2861-7pf C1,C2=6.8p
#else
	{0x2A, 0x7860},
#endif
	{0x2B, 0x0804},//bbpll,or ver.c20111116
	{0x32, 0x8a08},
	{0x33, 0x1D02},//liuyanan
	//{;;//agc_gain},
#if 1
	{0x36, 0x02f4}, //00F8;//gain_7
	{0x37, 0x01f4}, //0074;//aain_6
	{0x38, 0x21d4}, //0014;//gain_5
	{0x39, 0x25d4}, //0414;//aain_4
	{0x3A, 0x2584}, //1804;//gain_3
	{0x3B, 0x2dc4}, //1C04;//aain_2
	{0x3C, 0x2d04}, //1C02;//gain_1
	{0x3D, 0x2c02}, //3C01;//gain_0
#else
	{0x36, 0x01f8}, //00F8;//gain_7
	{0x37, 0x01f4}, //0074;//aain_6
	{0x38, 0x21d4}, //0014;//gain_5
	{0x39, 0x2073}, //0414;//aain_4
	{0x3A, 0x2473}, //1804;//gain_3
	{0x3B, 0x2dc7}, //1C04;//aain_2
	{0x3C, 0x2d07}, //1C02;//gain_1
	{0x3D, 0x2c04}, //3C01;//gain_0
#endif
	{0x33, 0x1502},//liuyanan
	//{;;SET_channe},_to_11
	{0x1B, 0x0001},//set_channel
	{0x30, 0x024D},
	{0x29, 0xD468},
	{0x29, 0x1468},
	{0x30, 0x0249},
	{0x3f, 0x0000},
};

static const u16 wifi_rf_init_data_90_verD[][2] =
{
	{0x3f, 0x0000},
	//{;;set_rf_swi},ch
	{0x05, 0x8000},
	{0x06, 0x0101},
	{0x07, 0x0101},
	{0x08, 0x0101},
	{0x09, 0x3040},
	{0x0A, 0x002C},//aain_0
	{0x0D, 0x0507},
	{0x0E, 0x2300},//2012_02_20
	{0x0F, 0x5689},//
	//{;;//set_RF  },
	{0x10, 0x0f78},//20110824
	{0x11, 0x0602},
	{0x13, 0x0652},//adc_tuning_bit[011]
	{0x14, 0x8886},
	{0x15, 0x0910},
	{0x16, 0x049f},
#ifdef WLAN_USE_CRYSTAL
	{0x17, 0x0990},
	{0x18, 0x049F},
#else
	{0x17, 0x0910},
	{0x18, 0x249F},
#endif
	{0x19, 0x3C01},//sdm_vbit[3:0]=1111
	{0x1C, 0x0934},
	{0x1D, 0xFF00},//for ver.D20120119for temperature 70 degree 0xCE00 >> 0xFF00
	{0x1F, 0x0300},//div2_band_48g_dr=1;div2_band_48g_reg[8:0]
	{0x20, 0x06E4},
	{0x21, 0x0ACF},//for ver.c20111109,dr dac reset,dr txflt reset
	{0x22, 0x24DC},
#ifdef WLAN_FOR_CTA
	{0x23, 0x0BFF},
#else
	{0x23, 0x23FF},
#endif
	{0x24, 0x00FC},
	{0x26, 0x004F},//004F >> 005f premote pa
	{0x27, 0x171D},///mdll*7
	{0x28, 0x031D},///mdll*7
#ifdef WLAN_USE_CRYSTAL
	{0x2A, 0x2860},//et0x2849-8.5p  :yd 0x2861-7pf
#else
	{0x2A, 0x7860},
#endif
	{0x2B, 0x0804},//bbpll,or ver.c20111116
	{0x32, 0x8a08},
	{0x33, 0x1D02},//liuyanan
	//{;;//agc_gain},
#if 1
	{0x36, 0x02f4}, //00F8;//gain_7
	{0x37, 0x01f4}, //0074;//aain_6
	{0x38, 0x21d4}, //0014;//gain_5
	{0x39, 0x25d4}, //0414;//aain_4
	{0x3A, 0x2584}, //1804;//gain_3
	{0x3B, 0x2dc4}, //1C04;//aain_2
	{0x3C, 0x2d04}, //1C02;//gain_1
	{0x3D, 0x2c02}, //3C01;//gain_0
#else
	{0x36, 0x01f8}, //00F8;//gain_7
	{0x37, 0x01f4}, //0074;//aain_6
	{0x38, 0x21d4}, //0014;//gain_5
	{0x39, 0x2073}, //0414;//aain_4
	{0x3A, 0x2473}, //1804;//gain_3
	{0x3B, 0x2dc7}, //1C04;//aain_2
	{0x3C, 0x2d07}, //1C02;//gain_1
	{0x3D, 0x2c04}, //3C01;//gain_0
#endif
	{0x33, 0x1502},//liuyanan
	//{;;SET_channe},_to_11
	{0x1B, 0x0001},//set_channel
	{0x30, 0x024D},
	{0x29, 0xD468},
	{0x29, 0x1468},
	{0x30, 0x0249},
	{0x3f, 0x0000},
};

static const u16 wifi_tm_en_data_90[][2] =
{
	{0x3F,0x0001},
#ifdef WLAN_USE_DCDC 	/*houzhen update Mar 15 2012 */
	{0x23, 0x8F21},//20111001 higher AVDD voltage to improve EVM to 0x8f21 download current -1db 0x8fA1>>0x8bA1
#else
	{0x23, 0x0FA1},
#endif
	{0x22,0xD3C7},//for ver.c 20111109, tx
	{0x24,0x8048},//freq_osc_in[1:0]00  0x80C8 >> 0x80CB
	{0x27,0x4925},//for ver.c20111109, txs
	{0x28,0x80A1}, //BT_enable
	{0x29,0x111F},
	{0x31,0x8140},
	{0x32,0x0113},//set_ rdenout_ldooff_wf
	{0x39,0x0004},//uart switch to wf
	{0x3f,0x0000},
};

static const u16 wifi_tm_rf_init_data_90[][2] =
{
	{0x3f,0x0000},
	//set_rf_switch
	{0x06,0x0101},
	{0x07,0x0101},
	{0x08,0x0101},
	{0x09,0x3040},
	{0x0A,0x002C},//aain_0
	{0x0D,0x0507},
	{0x0E,0x2300},//2012_02_20
	{0x0F,0x5689},//
	//set_RF
	{0x10,0x0f78},//20110824
	{0x11,0x0602},
	{0x13,0x0652},//adc_tuning_bit[011]
	{0x14,0x8886},
	{0x15,0x0910},
	{0x16,0x049f},
#ifdef WLAN_USE_CRYSTAL
	{0x17,0x0990},
	{0x18,0x049F},
#else
	{0x17,0x0910},
	{0x18,0x249F},
#endif
	{0x19,0x3C01},//sdm_vbit[3:0]=1111
	{0x1C,0x0934},
	{0x1D,0xFF00},//for ver.D20120119for temperature 70 degree
	{0x1F,0x0300},//div2_band_48g_dr=1;div2_band_48g_reg[8:0]1000000000
	{0x20,0x06E4},
	{0x21,0x0ACF},//for ver.c20111109,dr dac reset,dr txflt reset
	{0x22,0x24DC},
#ifdef WLAN_FOR_CTA
	{0x23, 0x03FF},
#else
	{0x23, 0x0BFF},
#endif
	{0x24,0x00FC},
	{0x26,0x004F},
	{0x27,0x171D},///mdll*7
	{0x28,0x031D},///mdll*7
#ifdef WLAN_USE_CRYSTAL
	{0x2A,0x2860},
#else
	{0x2A,0x7860},
#endif
	{0x2B,0x0804},//bbpll,or ver.c20111116
	{0x32,0x8a08},
	{0x33,0x1D02},//liuyanan
	//agc_gain
#if 1
	{0x36, 0x02f4}, //00F8;//gain_7
	{0x37, 0x01f4}, //0074;//aain_6
	{0x38, 0x21d4}, //0014;//gain_5
	{0x39, 0x25d4}, //0414;//aain_4
	{0x3A, 0x2584}, //1804;//gain_3
	{0x3B, 0x2dc4}, //1C04;//aain_2
	{0x3C, 0x2d04}, //1C02;//gain_1
	{0x3D, 0x2c02}, //3C01;//gain_0
#else
	{0x36, 0x01f8}, //00F8;//gain_7
	{0x37, 0x01f4}, //0074;//aain_6
	{0x38, 0x21d4}, //0014;//gain_5
	{0x39, 0x2073}, //0414;//aain_4
	{0x3A, 0x2473}, //1804;//gain_3
	{0x3B, 0x2dc7}, //1C04;//aain_2
	{0x3C, 0x2d07}, //1C02;//gain_1
	{0x3D, 0x2c04}, //3C01;//gain_0
#endif
	{0x30,0x0248},
	{0x30,0x0249},
	//wait 200ms;
	{0x33,0x1502},//liuyanan
	//SET_channel_to_11
	{0x1B,0x0001},//set_channel
	{0x3f,0x0000},
};
#endif

/*houzhen update Mar 15 2012
  should be called when power up/down bt
  */
static int rda5990_wf_setup_A2_power(int enable)
{
	int ret;
	u16 temp_data=0;
	printk(KERN_INFO "***rda5990_wf_setup_A2_power start! \n");

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);
	if(ret)
		goto err;

	if(enable) {
		ret=i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x22, &temp_data);
		if(ret)
			goto err;
		printk(KERN_INFO "***0xA2 readback value:0x%X \n", temp_data);

		temp_data |=0x0200;   /*en reg4_pa bit*/
#ifdef WLAN_USE_CRYSTAL
		temp_data &= ~(1 << 14); //disable xen_out
#endif
		ret=i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x22, temp_data);
		if(ret)
			goto err;
	} else {
		ret=i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x28, &temp_data);
		if(ret)
			goto err;
		if(temp_data&0x8000) { // bt is on
			goto out;
		} else {
			ret=i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x22, &temp_data);
			if(ret)
				goto err;
			temp_data&=0xfdff;

			ret=i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x22, temp_data);
			if(ret)
				goto err;
		}
	}

out:
	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);
	if(ret)
		goto err;
	printk(KERN_INFO "***rda5990_wf_setup_A2_power succeed! \n");
	return 0;

err:
	printk(KERN_INFO "***rda5990_wf_setup_A2_power failed! \n");
	return -1;
}


int rda_5990_wifi_rf_init(void)
{
	int ret = 0;

	rda_combo_i2c_lock();

	if( rda_wlan_version() == WLAN_VERSION_90_D) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client,
			wifi_rf_init_data_90_verD);
		if(ret)
			goto err;
	} else if(rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client,
			wifi_rf_init_data_90_verE);
		if(ret)
			goto err;
	} else {
		printk("unknown version of this chip\n");
		goto err;
	}

	rda_combo_i2c_unlock();

	printk(KERN_INFO "***rda_5990_wifi_rf_init_succceed \n");
	msleep(5);	 //5ms delay
	return 0;
err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_5990_wifi_rf_init failed! \n");
	return -1;
}

int rda_5990_wifi_dc_cal(void)
{
	int ret = 0;

	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wifi_dc_cal_data);
	if(ret)
		goto err;

	rda_combo_i2c_unlock();

	printk(KERN_INFO "***rda_wifi_rf_dc_calsuccceed \n");
	msleep(50);   //50ms delay
	return 0;

err:
	rda_combo_i2c_unlock();

	printk(KERN_INFO "***rda_wifi_rf_dc_calf_failed! \n");
	return -1;
}

int rda_5990_wifi_en(void)
{
	int ret = 0;
	enable_26m_regulator(CLOCK_WLAN);
	enable_32k_rtc(CLOCK_WLAN);
	enable_26m_rtc(CLOCK_WLAN);

	msleep(8);

	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client, wifi_en_data_90);
		if(ret)
			goto err;
	}

	ret=rda5990_wf_setup_A2_power(1);	//en pa_reg for wf
	if(ret)
		goto err;

	rda_combo_i2c_unlock();

	msleep(8);	 //8ms delay

	printk(KERN_INFO "***rda_5990_wifi_en_succceed \n");
	//in rda platform should close 26M after wifi power on success
	disable_26m_rtc(CLOCK_WLAN);
	return 0;
err:
	rda_combo_i2c_unlock();
	disable_32k_rtc(CLOCK_WLAN);
	disable_26m_rtc(CLOCK_WLAN);
	disable_26m_regulator(CLOCK_WLAN);
	printk(KERN_INFO "***rda_5990_wifi_en failed! \n");
	return -1;
}

static int rda_5990_wifi_debug_en(int enable)
{
	int ret = 0;
	u16 temp_data = 0;

	rda_combo_i2c_lock();

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);
	if (ret < 0)
		goto err;
	if (enable == 1) {
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x28, 0x80A1);
		if (ret < 0)
			goto err;

		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x39, &temp_data);
		if (ret < 0)
			goto err;

		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39,
			temp_data | (1 << 2));
		if (ret < 0)
			goto err;
	} else {
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x28, 0x00A1);
		if (ret < 0)
			goto err;

		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x39, &temp_data);
		if (ret < 0)
			goto err;

		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39,
			temp_data & (~(1 << 2)));
		if (ret < 0)
			goto err;
	}

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);
	if (ret < 0)
		goto err;

err:
	rda_combo_i2c_unlock();
	return ret;
}

int rda_5990_tm_wifi_en(void)
{
	int ret = 0;
	enable_26m_regulator(CLOCK_WLAN);
	enable_32k_rtc(CLOCK_WLAN);
	enable_26m_rtc(CLOCK_WLAN);
	msleep(5);
	rda_combo_i2c_lock();
	if(rda_wlan_version() == WLAN_VERSION_90_D ||
			rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client,
			wifi_tm_en_data_90);
		if(ret)
			goto err;
	}
	rda_combo_i2c_unlock();
	msleep(8);	 //8ms delay
	printk(KERN_INFO "***rda_5990_tm_wifi_en succceed \n");
	disable_26m_rtc(CLOCK_WLAN);
	return 0;
err:
	disable_32k_rtc(CLOCK_WLAN);
	disable_26m_rtc(CLOCK_WLAN);
	disable_26m_regulator(CLOCK_WLAN);
	printk(KERN_INFO "***rda_5990_tm_wifi_en failed! \n");
	return -1;
}

int rda_5990_tm_wifi_rf_init(void)
{
	int ret = 0;
	rda_combo_i2c_lock();
	if(rda_wlan_version() == WLAN_VERSION_90_D ||
			rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_wifi_rf_client,
			wifi_tm_rf_init_data_90);
		if(ret)
			goto err;
	}else
		return -1;
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_5990_tm_wifi_rf_init \n");
	msleep(5);	 //5ms delay
	return 0;

err:
	printk(KERN_INFO "***rda_5990_tm_wifi_rf_init failed! \n");
	return -1;
}
/*houzhen add 2012 04 09
  add to ensure wf dig powerup
  */

int rda_5990_wifi_dig_reset(void)
{
	int ret = 0;

	msleep(8);	 //8ms delay

	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wifi_dig_reset_data_90);
	if(ret)
		goto err;

	rda_combo_i2c_unlock();

	msleep(8);	 //8ms delay

	printk(KERN_INFO "***rda_5990_wifi_dig_reset \n");
	return 0;
err:
	rda_combo_i2c_unlock();

	printk(KERN_INFO "***rda_5990_wifi_dig_reset failed! \n");
	return -1;
}

int rda_5990_wifi_power_off(void)
{
	int ret = 0;
	u16 temp=0x0000;
	printk(KERN_INFO "rda_5990_wifi_power_off \n");

	if(!rda_wifi_rf_client) {
		printk(KERN_INFO "rda_5990_wifi_power_off failed on:i2c client \n");
		return -1;
	}

	rda_combo_i2c_lock();

	ret=rda5990_wf_setup_A2_power(0);	//disable pa_reg for wf
	if(ret)
		goto err;

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E){
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client,
			0x3f, 0x0001);   //page up
		if(ret)
			goto err;

		ret=i2c_read_1_addr_2_data(rda_wifi_rf_client,
			0x28, &temp);	//poll bt status
		if(ret)
			goto err;

		if(temp&0x8000) {
			ret = i2c_write_1_addr_2_data(rda_wifi_rf_client,
				0x3f, 0x0000);   //page down
			if(ret)
				goto err;

			ret = i2c_write_1_addr_2_data(rda_wifi_rf_client,
				0x0f, 0x2223);   // set antenna for bt
			if(ret)
				goto err;

		}
	}

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wifi_off_data);
	if(ret)
		goto err;

	rda_combo_i2c_unlock();

	wifi_is_on = 0;
	disable_32k_rtc(CLOCK_WLAN);
	disable_26m_rtc(CLOCK_WLAN);
	disable_26m_regulator(CLOCK_WLAN);
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&wifi_fw_status, 0);
	rda_release_firmware();
#endif
	printk(KERN_INFO "***rda_5990_wifi_power_off success!!! \n");
	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_5990_wifi_power_off failed! \n");
	return -1;

}

int rda_5990_wifi_power_on(void)
{
	int ret;
	char retry = 3;

	if(!rda_wifi_rf_client) {
		printk(KERN_INFO "rda_5990_wifi_power_on failed on:i2c client \n");
		return -1;
	}

_retry:
	if( !check_test_mode() )
	{
		ret = rda_5990_wifi_en();
		if(ret < 0)
			goto err;

		ret = rda_5990_wifi_rf_init();
		if(ret < 0)
			goto err;

		ret = rda_5990_wifi_dc_cal();
		if(ret < 0)
			goto err;

		msleep(20);   //20ms delay
		ret=rda_5990_wifi_dig_reset();	//houzhen add to ensure wf power up safely

		if(ret < 0)
			goto err;
		msleep(20);   //20ms delay
	}
	else{
		ret = rda_5990_tm_wifi_en();
		if(ret < 0)
			goto err;

		ret = rda_5990_tm_wifi_rf_init();
		if(ret < 0)
			goto err;
	}
	printk(KERN_INFO "rda_5990_wifi_power_on_succeed!! \n");
	wifi_is_on = 1;
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&wifi_fw_status, 1);
#endif
	return 0;

err:
	printk(KERN_INFO "rda_5990_wifi_power_on_failed retry:%d \n", retry);
	if(retry -- > 0)	{
		rda_5990_wifi_power_off();
		goto _retry;
	}
	wifi_is_on = 0;
	return -1;
}

int rda_5990_fm_power_on(void)
{
	int ret = 0;
	u16 temp = 0;

	if(!rda_wifi_rf_client){
		printk(KERN_INFO
			"rda_wifi_rf_client is NULL, rda_fm_power_on failed!\n");
		return -1;
	}

	enable_32k_rtc(CLOCK_FM);
	msleep(8);
	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {

		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);   // page down
		if(ret < 0) {
			printk(KERN_INFO
				"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0x3f, 0x0001);
			goto err;
		}

		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x22, &temp); //read 0xA2
		if(ret < 0) {
			printk(KERN_INFO "%s() read from address(0x%02x) failed! \n",
				__func__, 0x22);
			goto err;
		}
		temp = temp & (~(1 << 15)); 	//clear bit[15]
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x22, temp);	 //write back
		if(ret < 0) {
			printk(KERN_INFO
				"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0x3f, 0x0001);
			goto err;
		}
	}
	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);  // page up
	if(ret < 0) {
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

int rda_5990_fm_power_off(void)
{
	int ret = 0;
	u16 temp = 0;

	if(!rda_wifi_rf_client) {
		printk(KERN_INFO
			"rda_wifi_rf_client is NULL, rda_fm_power_off failed!\n");
		return -1;
	}

	rda_combo_i2c_lock();

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);   // page down
	if(ret < 0) {
		printk(KERN_INFO
			"%s() write address(0x%02x) with value(0x%04x) failed! \n",
			__func__, 0x3f, 0x0001);
		goto err;
	}

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E){
		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x22, &temp);	//read 0xA2
		if(ret < 0) {
			printk(KERN_INFO
				"%s() read from address(0x%02x) failed! \n",
				__func__, 0x22);
			goto err;
		}
		temp = temp | (1 << 15);		//set bit[15]
		ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x22, temp);	 //write back
		if(ret < 0) {
			printk(KERN_INFO
				"%s() write address(0x%02x) with value(0x%04x) failed! \n",
				__func__, 0x3f, 0x0001);
			goto err;
		}
	}

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);   // page up
	if(ret < 0) {
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

#ifndef RDA_COMBO_FROM_FIRMWARE
static const u16 rda_5990_bt_off_data[][2] =
{
	{0x3f, 0x0001 }, //pageup
	{0x28, 0x00A1 }, //power off bt
	{0x3f, 0x0000 }, //pagedown
};

/*houzhen update 2012 03 06*/
static const u16 rda_5990_bt_en_data[][2] =
{
	{0x3f, 0x0001 },		//pageup
#ifdef WLAN_USE_DCDC
	{0x23, 0x8FA1}, 	  // //20111001 higher AVDD voltage to improve EVM
#else
	{0x23, 0x0FA1},
#endif
	{0x24, 0x8048}, 	  // ;//freq_osc_in[1:0]00
	{0x26, 0x47A5}, 	  //  reg_vbit_normal_bt[2:0] =111
	{0x27, 0x4925}, 	  // //for ver.c20111109, txswitch
	{0x29, 0x111F}, 	  // // rden4in_ldoon_bt=1
	{0x32, 0x0111}, 	  // set_ rdenout_ldooff_wf=0;
	{0x39, 0x0000}, 	  //	  //uart switch to bt

	{0x28, 0x80A1}, 		// bt en
	{0x3f, 0x0000}, 		//pagedown
};

static const u16 rda_5990_bt_dc_cal[][2] =
{
	{0x3f, 0x0000 },
	{0x30, 0x0129 },
	{0x30, 0x012B },
	{0x3f, 0x0000 },
};

static const u16 rda_5990_bt_set_rf_switch_data[][2] =
{
	{0x3f, 0x0000 },
	{0x0F, 0x2223 },
	{0x3f, 0x0000 },
};

static const u16 RDA5990_bt_enable_clk_data[][2] =
{
	{0x3f, 0x0000 },
	{0x30, 0x0040 },
	{0x2a, 0x285d },
	{0x3f, 0x0000 },
};

static const u16 RDA5990_bt_dig_reset_data[][2] =
{
	{0x3f, 0x0001 }, //pageup
	{0x28, 0x86A1 },
	{0x28, 0x87A1 },
	{0x28, 0x85A1 },
	{0x3f, 0x0000 }, //pagedown
};

/*houzhen update 2012 03 06*/
static const u16 rda_5990_bt_rf_data[][2] =
{
	{0x3f, 0x0000}, //pagedown
	{0x01, 0x1FFF},
	{0x06, 0x07F7},
	{0x08, 0x29E7},
	{0x09, 0x0520},
	{0x0B, 0x03DF},
	{0x0C, 0x85E8},
	{0x0F, 0x0DBC},
	{0x12, 0x07F7},
	{0x13, 0x0327},
	{0x14, 0x0CCC},
	{0x15, 0x0526},
	{0x16, 0x8918},
	{0x18, 0x8800},
	{0x19, 0x10C8},
	{0x1A, 0x9078},
	{0x1B, 0x80E2},
	{0x1C, 0x361F},
	{0x1D, 0x4363},
	{0x1E, 0x303F},
	{0x23, 0x2222},
	{0x24, 0x359D},
	{0x27, 0x0011},
	{0x28, 0x124F},
	{0x39, 0xA5FC},
	{0x3f, 0x0001}, //page 1
	{0x00, 0x043F},
	{0x01, 0x467F},
	{0x02, 0x28FF},
	{0x03, 0x67FF},
	{0x04, 0x57FF},
	{0x05, 0x7BFF},
	{0x06, 0x3FFF},
	{0x07, 0x7FFF},
	{0x18, 0xF3F5},
	{0x19, 0xF3F5},
	{0x1A, 0xE7F3},
	{0x1B, 0xF1FF},
	{0x1C, 0xFFFF},
	{0x1D, 0xFFFF},
	{0x1E, 0xFFFF},
	{0x1F, 0xFFFF},
	//	{0x22, 0xD3C7},
	//	{0x23, 0x8fa1},
	//	{0x24, 0x80c8},
	//	{0x26, 0x47A5},
	//	{0x27, 0x4925},
	//	{0x28, 0x85a1},
	//	{0x29, 0x111f},
	//	{0x32, 0x0111},
	//	{0x39, 0x0000},
	{0x3f, 0x0000}, //pagedown
};
#endif

/*houzhen update Mar 15 2012
  should be called when power up/down bt
  */
static int rda5990_bt_setup_A2_power(int enable)
{
	int ret;
	u16 temp_data=0;

	ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0001);
	if(ret)
		goto err;

	if(enable) {
		ret=i2c_read_1_addr_2_data(rda_bt_rf_client,0x22,&temp_data);
		if(ret)
			goto err;
		printk(KERN_INFO "***0xA2 readback value:0x%X \n", temp_data);

		temp_data |=0x0200;   /*en reg4_pa bit*/

		ret=i2c_write_1_addr_2_data(rda_bt_rf_client,0x22,temp_data);
		if(ret)
			goto err;
	} else {
		ret=i2c_read_1_addr_2_data(rda_bt_rf_client,0x31,&temp_data);
		if(ret)
			goto err;

		if(temp_data&0x8000) {	// wf is on
			goto out;
		} else {
			ret=i2c_read_1_addr_2_data(rda_bt_rf_client,0x22,&temp_data);
			if(ret)
				goto err;
			temp_data&=0xfdff;

			ret=i2c_write_1_addr_2_data(rda_bt_rf_client,0x22,temp_data);
			if(ret)
				goto err;
		}

	}

out:
	ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0000);
	if(ret)
		goto err;
	return 0;

err:
	printk(KERN_INFO "***rda5990_bt_setup_A2_power failed! \n");
	return -1;
}

int rda_5990_bt_power_on(void)
{
	int ret = 0;

	printk(KERN_INFO "rda_bt_power_on \n");

	if(!rda_bt_rf_client) {
		printk(KERN_INFO "rda_bt_power_on failed on:i2c client \n");
		return -1;
	}
	enable_26m_regulator(CLOCK_BT);
	enable_26m_rtc(CLOCK_BT);
	enable_32k_rtc(CLOCK_BT);
	msleep(8);

	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5990_bt_en_data);
		if(ret)
			goto err;
	}

	ret=rda5990_bt_setup_A2_power(1);
	if(ret) {
		printk(KERN_INFO
			"***rda5990_bt_setup_A2_power fail!!! \n");
		goto err;
	}

	printk(KERN_INFO "***rda_bt_power_on success!!! \n");
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&bt_fw_status, 1);
#endif
	rda_combo_i2c_unlock();
	/*houzhen update 2012 03 06*/
	msleep(10); 	//delay 10 ms after power on
	disable_26m_rtc(CLOCK_BT);
	return 0;

err:
	rda_combo_i2c_unlock();
	disable_26m_rtc(CLOCK_BT);
	disable_32k_rtc(CLOCK_BT);
	disable_26m_regulator(CLOCK_BT);
	printk(KERN_INFO "***rda_bt_power_on failed! \n");
	return -1;

}

int rda_5990_bt_power_off(void)
{
	int ret = 0;
	printk(KERN_INFO "rda_5990_bt_power_off \n");

	if(!rda_bt_rf_client) {
		printk(KERN_INFO "rda_5990_bt_power_off failed on:i2c client \n");
		return -1;
	}

	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5990_bt_off_data);
		if(ret)
			goto err;
	}

	msleep(10);   //10ms
	printk(KERN_INFO "***rda_5990_bt_power_off success!!! \n");
	ret=rda5990_bt_setup_A2_power(0);//disable ldo_pa reg
	if(ret)
		goto err;

	rda_combo_i2c_unlock();
	disable_26m_rtc(CLOCK_BT);
	disable_32k_rtc(CLOCK_BT);
	disable_26m_regulator(CLOCK_BT);
#ifdef RDA_COMBO_FROM_FIRMWARE
	atomic_set(&bt_fw_status, 0);
	rda_release_firmware();
#endif
	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_5990_bt_power_off failed! \n");
	return -1;

}


static int RDA5990_bt_rf_init(void)
{
	int ret = 0;

	if(!rda_bt_rf_client){
		printk(KERN_INFO "RDA5990_bt_rf_init on:i2c client \n");
		return -1;
	}

	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5990_bt_rf_data);
		if(ret)
			goto err;
	}

	rda_combo_i2c_unlock();

	printk(KERN_INFO "***RDA5990_bt_rf_init success!!! \n");
	msleep(5);	 //5ms
	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5990_bt_rf_init failed! \n");
	return -1;
}

/*houzhen add 2012 04 09
  add to ensure bt dig powerup
  */
static int RDA5990_bt_dig_reset(void)
{
	int ret = 0;

	if(!rda_bt_rf_client){
		printk(KERN_INFO "RDA5990_bt_dig_reset on:i2c client \n");
		return -1;
	}

	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_bt_rf_client,
		RDA5990_bt_dig_reset_data);
	if(ret)
		goto err;

	rda_combo_i2c_unlock();

	printk(KERN_INFO "***RDA5990_bt_dig_reset success!!! \n");
	msleep(5);	 //5ms

	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5990_bt_dig_reset failed! \n");
	return -1;
}


static int RDA5990_bt_dc_cal(void)
{
	int ret = 0;

	if(!rda_bt_rf_client) {
		printk(KERN_INFO "rda_bt_rf_client \n");
		return -1;
	}

	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5990_bt_dc_cal);
		if(ret)
			goto err;
	}

	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_bt_dc_cal success!!! \n");
	msleep(200);   //200ms

	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***rda_bt_dc_cal	failed! \n");
	return -1;

}


/*houzhen update Mar 15 2012
  bypass RDA5990_bt_set_rf_switch when wf is already on
*/

static int RDA5990_bt_set_rf_switch(void)
{
	int ret = 0;
	u16 temp_data=0;

	if(!rda_bt_rf_client || !rda_wifi_rf_client){
		printk(KERN_INFO
			"RDA5990_bt_set_rf_switch failed on:i2c client \n");
		return -1;
	}

	rda_combo_i2c_lock();

	if(rda_wlan_version() == WLAN_VERSION_90_D ||
		rda_wlan_version() == WLAN_VERSION_90_E) {
		ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0001);
		if(ret)
			goto err;

		ret=i2c_read_1_addr_2_data(rda_bt_rf_client,0x31,&temp_data);
		if(ret)
			goto err;

		if(temp_data&0x8000) { // if wf is already on
			printk(KERN_INFO
				"wf already en, bypass RDA5990_bt_set_rf_switch function \n");
			ret = i2c_write_1_addr_2_data(rda_bt_rf_client,
				0x3f, 0x0000);
			if(ret)
				goto err;

			rda_combo_i2c_unlock();
			return 0;
		}

		ret = rda_write_data_to_rf(rda_wifi_rf_client,
			rda_5990_bt_set_rf_switch_data);
		if(ret)
			goto err;
	}

	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5990_bt_set_rf_switch success!!! \n");
	msleep(50);   //50ms
	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5990_bt_set_rf_switch  failed! \n");
	return -1;

}


/*houzhen update Mar 15 2012
  bypass RDA5990_bt_enable_clk when wf is already on
  */
static int RDA5990_bt_enable_clk(void)
{
	int ret = 0;
	u16 temp_data=0;

	if(!rda_bt_rf_client) {
		printk(KERN_INFO
			"RDA5990_bt_enable_clk failed on:i2c client \n");
		return -1;
	}

	rda_combo_i2c_lock();
	ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0001);
	if(ret)
		goto err;

	ret=i2c_read_1_addr_2_data(rda_bt_rf_client,0x31,&temp_data);

	if(ret)
		goto err;

	if(temp_data&0x8000){ // if wf is already on
		printk(KERN_INFO
			"wf already en, bypass RDA5990_bt_enable_clk function \n");
		ret = i2c_write_1_addr_2_data(rda_bt_rf_client, 0x3f, 0x0000);
		if(ret)
			goto err;
		rda_combo_i2c_unlock();
		return 0;
	}


	ret = rda_write_data_to_rf(rda_wifi_rf_client,
		RDA5990_bt_enable_clk_data);
	if(ret)
		goto err;

	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5990_bt_enable_clk success!!! \n");
	msleep(50);   //50ms
	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5990_bt_enable_clk	failed! \n");
	return -1;
}

long rda_5990_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch(cmd) {
	case RDA_WIFI_POWER_ON_IOCTL:
		ret = rda_5990_wifi_power_on();
		break;

	case RDA_WIFI_POWER_OFF_IOCTL:
		ret = rda_5990_wifi_power_off();
		break;

	case RDA_WIFI_DEBUG_MODE_IOCTL:
		{
			int enable = 0;
			if(copy_from_user(&enable, (void*)arg, sizeof(int))) {
				printk(KERN_ERR "copy_from_user enable failed!\n");
				return -EFAULT;
			}
			ret = rda_5990_wifi_debug_en(enable);
			break;
		}
	case RDA_BT_POWER_ON_IOCTL:
		ret = rda_5990_bt_power_on();
		break;

	/* should call this function after bt_power_on*/
	case RDA_BT_EN_CLK:
		ret = RDA5990_bt_enable_clk();
		break;

	case RDA_BT_RF_INIT_IOCTL:
		ret = RDA5990_bt_rf_init();
		break;

	case RDA_BT_DC_CAL_IOCTL:
		ret = RDA5990_bt_dc_cal();
		break;

	case RDA_BT_DC_DIG_RESET_IOCTL:
		ret = RDA5990_bt_dig_reset();
		break;

	case RDA_BT_RF_SWITCH_IOCTL:
		ret = RDA5990_bt_set_rf_switch();
		break;

	case RDA_BT_POWER_OFF_IOCTL:
		ret = rda_5990_bt_power_off();
		break;

	default:
		ret = -EFAULT;
		printk(KERN_ERR "******rda_5990_pw_ioctl cmd[0x%02x]******.\n", cmd);
		break;
	}

	printk(KERN_INFO "rda_bt_pw_ioctl cmd=0x%02x \n", cmd);
	return ret;
}

