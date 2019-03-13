#include "rda_combo.h"
#ifndef RDA_COMBO_FROM_FIRMWARE

#ifdef WLAN_USE_DCDC
static const u16 pmu_setting_5991[][2] = {
	{0x3F, 0x0001},
	{0x22, 0xA1F3},		//Pu_rf_dr=0;Nonov_delay_dig=11,Nonov_delay_ana=11
	{0x24, 0x8808},		//pu_dcdc_dr_ana=0;pu_ldo_dr_ana=0;AVDD=2.07V;sleep vol=0000;clk_mode_sel_dr_ana =0
	{0x26, 0x8255},		//sleep voltage=800mV;clk_mode_sel_dr_dig=0
	{0x23, 0xC200},		//Dcdc_en_ana=1
	{0x25, 0xC27F},		//Dcdc_en_dig=1;sleep_ldo_vout=111=1.1v
	{0x29, 0x1113},
	{0x32, 0x1113},
	{0x37, 0x0B8A},
	{0x39, 0x1200},		//for dcdc;uart_bypass_en
	{0x3F, 0x0000},
};
#else
static const u16 pmu_setting_5991[][2] = {
	{0x3F, 0x0001},
	{0x22, 0xA1F3},		//Pu_rf_dr=0;Nonov_delay_dig=11,Nonov_delay_ana=11
	{0x24, 0x8808},		//pu_dcdc_dr_ana=0;pu_ldo_dr_ana=0;AVDD=2.07V;sleep vol=0000;clk_mode_sel_dr_ana =0
	{0x26, 0x8255},		//sleep voltage=800mV;clk_mode_sel_dr_dig=0;pu_ldo_dr_dig=1;pu_ldo_reg_dig =1
	{0x23, 0x4200},		//Dcdc_en_ana=0
	{0x25, 0x427F},		//Dcdc_en_dig=0;sleep_ldo_vout=111=1.1v
	{0x29, 0x1113},
	{0x32, 0x1113},
	{0x37, 0x0B8A},
	{0x39, 0x1200},		//for dcdc;uart_bypass_en
	{0x3F, 0x0000},
};
#endif

/*add according to hongjun's new config*/
static const u16 soft_reset_5991[][2] = {
	{0x3F, 0x0000},
	{0x30, 0x8000},
	{0x30, 0x0000},
};

static const u16 wf_en_5991[][2] = {
	{0x3F, 0x0001},
	{0x31, 0x8B40},		//;WIFI_en=1
	{0x3F, 0x0000},
};

static const u16 wifi_disable_5991[][2] = {
	{0x3F, 0x0001},
	{0x31, 0x0B40},		//;WIFI_en=0
	{0x3F, 0x0000},
};

static const u16 wf_rf_setting_5991[][2] = {
	{0x3F, 0x0000},
	{0x10, 0x9f33},		//wf_lna_bpf_en =1
	{0x11, 0xFF8A},
	{0x13, 0x5054},
	{0x14, 0x988C},		//wf_pll_regbit_presc[3:0]=1100,for temperatrue
	{0x15, 0x596F},
	{0x16, 0x200B},		//wf_pll_r_bit[1:0] =01;wf_pll_r_bit[1:0]=00
	{0x19, 0x9C01},		//wf_pll_sinc_mode[2:0]=001 ,ver D
	{0x1C, 0x06E4},		//wf_dac_cm_bit[1:0]=00,1.1V
	{0x1D, 0x3A8C},
	{0x22, 0xFF4B},		//ver D
	{0x23, 0xAA3C},
	{0x24, 0x88C4},		//wf_dac_cal_dr=1;ver D
	{0x28, 0x1320},
	{0x2A, 0x0036},         //CL=7.3pf crystal, 2A=0x0036
	{0x2B, 0x41BB},		//wf_ovd_resbit_for_wf_tx[7:0]=FF;wf_pa_cap2_open_for_wf_tx=1
	{0x2D, 0xFF03},		//wf_pa_capbit_for_wf_tx[7:0]=3
	{0x2F, 0x00DE},		//wf_dsp_resetn_tx_dr=1;wf_dsp_resetn_tx_reg=1; ;;wf_dsp_resetn_rx_dr=1;wf_dsp_resetn_rx_reg=1;
	{0x34, 0x3000},		//wf_dac_clk_inv[1:0]=11
	{0x39, 0x8000},
	{0x40, 0x7FFF},		//wf_tmx_gain,wf_pabias
	{0x41, 0xFFFF},		//wf_pabias
};

static const u16 wf_agc_setting_for_dccal_5991[][2] = {
	{0x3F, 0x0000},
	{0x0F, 0x61F7},		//;//0F
	{0x0E, 0x61F0},		//
	{0x0D, 0x60F0},		//
	{0x0C, 0x6070},		//
	{0x0B, 0x6030},		//
	{0x0A, 0x6010},		//
	{0x09, 0x7033},		//
	{0x08, 0x6830},		//;;//08
	{0x3F, 0x0001},
	{0x07, 0x7031},		//;;//07
	{0x06, 0x7011},		//;;//06
	{0x05, 0x7871},		//
	{0x04, 0x7831},		//;;//04
	{0x03, 0x7811},		//;;//03
	{0x02, 0x7801},		//;;//02
	{0x01, 0x7800},		//;;//01
	{0x00, 0x7800},		//;;//00
	{0x3F, 0x0000},
};

static const u16 wf_agc_setting_5991[][2] = {
	{0x3F, 0x0000},
	{0x0F, 0x01F7},		//;//0F
	{0x0E, 0x01F0},		//
	{0x0D, 0x00F0},		//
	{0x0C, 0x0070},		//
	{0x0B, 0x0030},		//
	{0x0A, 0x0010},		//
	{0x09, 0x3033},		//
	{0x08, 0x0830},		//;;//08
	{0x3F, 0x0001},
	{0x07, 0x7031},		//;;//07
	{0x06, 0x7011},		//;;//06
	{0x05, 0x7871},		//
	{0x04, 0x7831},		//;;//04
	{0x03, 0x7811},		//;;//03
	{0x02, 0x7801},		//;;//02
	{0x01, 0x7800},		//;;//01
	{0x00, 0x7800},		//;;//00
	{0x3F, 0x0000},
};

static const u16 wf_calibration_5991[][2] = {
#if 1 //hongjun's new config
        {0x3F,0x0000},
        {0x1a,0x0026},//设定频点2487MHZ
        {0x1B,0xDC00},//设定频点2487MHZ
        {0x28,0x1F20},
        {0x28,0x1320},
        {0x30,0x0159},//设定frequency mode
        {0x30,0x0158},
        {0x30,0x0159},//dc_cal
        DELAY_MS(200)
#else
	{0x3F, 0x0000},
	{0x30, 0x0148},
	DELAY_MS(100)
	{0x28, 0x1F20},	//mdll_startup
	{0x28, 0x1320},		//mdll_startup_done
	{0x30, 0x0149},
	DELAY_MS(100)
	{0x30, 0x0349},	//wf_chip_self_cal_en=1
#endif
};

static const u16 bt_rf_setting_5991[][2] = {
	{0x3F, 0x0000},
	{0x02, 0x0E00},		//BT_Agc<2:0>;
	{0x08, 0xEFFF},		//bt_lna_gain2_7[2:0=11111
	{0x0A, 0x09FF},		//bt_rxflt_gain_5=11
	{0x11, 0x0035},		//
	{0x13, 0x2C68},		//
	{0x14, 0x91C4},		//bt_adc_digi_pwr_bit_reg[2:0]=011
	{0x18, 0x2010},		//bt_pll_phase_ctrl_dly[1:0]=00
	{0x30, 0x0141},
	{0x3F, 0x0001},
	{0x01, 0x66F3},		//bt_tmx_therm_gain_f[3:0]=0011
	{0x3F, 0x0000},
	{0x2E, 0xCAA3},		//bt_swtrx_dr=1;bt_swtrx_reg=1
	{0x37, 0x4411},		//PSK
	{0x38, 0x1348},		//PSK
	{0x3B, 0x2200},		//GFSK
	{0x3C, 0x0124},		//GFSK
	{0x3F, 0x0001},
	{0x00, 0xBBBB},		//
	{0x01, 0x66F3},		//
	{0x02, 0xBBBB},		//
	{0x03, 0x66F3},		//
	{0x04, 0xBBBB},		//
	{0x05, 0x66F3},		//
	{0x06, 0xBBBB},		//
	{0x07, 0x66FF},		//
	{0x08, 0xBBBB},		//
	{0x09, 0x66F7},		//
	{0x0A, 0xBBBB},		//
	{0x0B, 0x66F0},		//
	{0x39, 0x1200},		//uart_bypass_en=0
	{0x3F, 0x0000},
};

static const u16 control_mode_disable_5991[][2] = {
#if 1 //hongjun's new config
        {0x3F,0x0000},
        {0x30,0x0149},//设定channel mode
        {0x30,0x014D},
        {0x30,0x0141},
#else
	{0x3F, 0x0000},
	{0x30, 0x0141},
#endif
};

static const u16 bt_en_5991[][2] = {
	{0x3F, 0x0001},
	{0x28, 0x85A1},		//;bt_en=1
	{0x3F, 0x0000},
};

static const u16 bt_disable_5991[][2] = {
	{0x3F, 0x0001},
	{0x28, 0x05A1},		//;bt_en=1
	{0x3F, 0x0000},
};
// add for pta
static const u16 rda_5991_bt_dc_ca_fix_gain[][2] =
{
	{0x3f, 0x0000 },
	{0x30, 0x0141 },
	{0x30, 0x0140 },
	{0x30, 0x0141 },
	{0x30, 0x0341 },  //force gain level to 7 before lna issue fixed
	{0x2e, 0x8aa3 },  //force gain level to 7 before lna issue fixed
	{0x3f, 0x0000 },
};
// add for pta

// add for pta
static const u16 rda_5991_bt_dc_ca_fix_gain_no_wf[][2] =
{
	{0x3f, 0x0000 },
	{0x30, 0x0140 },
	{0x30, 0x0141 },
	{0x3f, 0x0000 },
};
// add for pta

// add for pta
static const u16 rda_5991_bt_force_swtrx[][2] =
{
	{0x3f, 0x0000 },
	{0x2e, 0xcaa3 },  //force gain level to 7 before lna issue fixed
	{0x3f, 0x0000 },
};
// add for pta

// add for pta
static const u16 rda_5991_bt_no_force_swtrx[][2] =
{
	{0x3f, 0x0000 },
	{0x2e, 0x8aa3 },  //force gain level to 7 before lna issue fixed
	{0x3f, 0x0000 },
};
// add for pta
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
	ret = rda_write_data_to_rf(rda_wifi_rf_client, pmu_setting_5991);
	if (ret)
		goto err;
	printk(KERN_INFO "%s write pmu_setting succeed!! \n", __func__);

	if (isWifi) {		// for wifi
		ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_en_5991);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write wf_en succeed!! \n", __func__);
	} else {		// for bt
		ret = rda_write_data_to_rf(rda_bt_rf_client, bt_en_5991);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write bt_en succeed!! \n", __func__);
	}

	rda_combo_i2c_unlock();
	msleep(5);
	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_wifi_rf_client, soft_reset_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write soft_reset succeed!! \n", __func__);

	rda_combo_i2c_unlock();
	msleep(10);
	rda_combo_i2c_lock();

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_rf_setting_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_rf_setting succeed!! \n", __func__);

	ret = rda_write_data_to_rf(rda_bt_rf_client, bt_rf_setting_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write bt_rf_setting succeed!! \n", __func__);

	ret = rda_write_data_to_rf(rda_wifi_rf_client,
		wf_agc_setting_for_dccal_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_agc_setting_for_dccal succeed!! \n",
		__func__);

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_calibration_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_calibration succeed!! \n", __func__);

	ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_agc_setting_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write wf_agc_setting succeed!! \n", __func__);

	return 0;
power_off:
	if (isWifi) {		// for wifi
		rda_write_data_to_rf(rda_wifi_rf_client, wifi_disable_5991);
	} else {
		rda_write_data_to_rf(rda_bt_rf_client, bt_disable_5991);
	}
err:
	return -1;
}

static int rda_5991_wifi_debug_en(void)
{
	u16 temp_data = 0;
	int ret = 0;
	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0001);
	if (ret < 0)
		return -1;

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x28, 0x80a1);
	if (ret < 0)
		return -1;
	ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x39, &temp_data);
	if (ret < 0)
		return -1;

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x39,
		temp_data | 0x04);
	if (ret < 0)
		return -1;

	ret = i2c_write_1_addr_2_data(rda_wifi_rf_client, 0x3f, 0x0000);
	if (ret < 0)
		return -1;
	return ret;
}

int rda_5991_wifi_power_on(void)
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
		ret = rda_write_data_to_rf(rda_wifi_rf_client, wf_en_5991);
		if (ret)
			goto err;
		printk(KERN_INFO "%s write wf_en succeed!! \n", __func__);

		//add for pta
		//handle btswtrx dr
		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5991_bt_no_force_swtrx);
		if (ret)
			goto err;
		printk(KERN_INFO
			"%s write rda_5991_bt_no_force_swtrx succeed!! \n",
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

	ret = rda_write_data_to_rf(rda_wifi_rf_client,
		control_mode_disable_5991);
	if (ret)
		goto power_off;
	printk(KERN_INFO "%s write control_mode_disable succeed!! \n",
		__func__);
	if(check_test_mode()) {
		rda_5991_wifi_debug_en();
		printk(KERN_INFO
			"%s: IN test mode, switch uart to WIFI succeed!! \n",
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
	rda_write_data_to_rf(rda_wifi_rf_client, wifi_disable_5991);
err:
	disable_26m_rtc(CLOCK_WLAN);
	disable_32k_rtc(CLOCK_WLAN);
	disable_26m_regulator(CLOCK_WLAN);
	rda_combo_i2c_unlock();
	return -1;
}

int rda_5991_wifi_power_off(void)
{
	int ret = 0;
	int bt_power_on = 0;  // add for pta
	rda_combo_i2c_lock();
	bt_power_on = check_bt_power_on();

	//add for pta
	if(bt_power_on) {

		ret = rda_write_data_to_rf(rda_bt_rf_client,
			rda_5991_bt_force_swtrx);

		if (ret) {
			printk(KERN_INFO
				"%s  rda_5991_bt_force_swtrx failed!! \n",
				__func__);
		} else {
			printk(KERN_INFO
				"%s  rda_5991_bt_force_swtrx succeed!! \n",
				__func__);
		}

	}
	// add for pta
	ret = rda_write_data_to_rf(rda_wifi_rf_client, wifi_disable_5991);
	if (ret) {
		printk(KERN_INFO "%s failed!! \n", __func__);
	} else {
		printk(KERN_INFO "%s succeed!! \n", __func__);
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

int rda_5991_bt_power_on(void)
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
		ret = rda_write_data_to_rf(rda_bt_rf_client, bt_en_5991);
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

int rda_5991_bt_power_off(void)
{
	int ret = 0;
	rda_combo_i2c_lock();
	ret = rda_write_data_to_rf(rda_bt_rf_client, bt_disable_5991);
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
static int RDA5991_bt_dc_cal_fix_gain(void)
{
	int ret = 0;
	int is_wfen;

	if(!rda_bt_rf_client) {
		printk(KERN_INFO "rda_bt_rf_client is NULL!\n");
		return -1;
	}

	rda_combo_i2c_lock();
	is_wfen= check_wifi_power_on();
	//check the version and make sure this applies to 5991
	if(rda_wlan_version() == WLAN_VERSION_91) {
		if(is_wfen) {
			ret = rda_write_data_to_rf(rda_bt_rf_client,
				rda_5991_bt_dc_ca_fix_gain);
		} else {
			ret = rda_write_data_to_rf(rda_bt_rf_client,
				rda_5991_bt_dc_ca_fix_gain_no_wf);
		}

		if(ret)
			goto err;
	}

	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5991_bt_dc_cal_fix_gain_update  success!!! \n");
	msleep(200);   //200ms

	return 0;

err:
	rda_combo_i2c_unlock();
	printk(KERN_INFO "***RDA5991_bt_dc_cal_fix_gain	failed! \n");
	return -1;

}

// add for pta

long rda_5991_pw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case RDA_WIFI_POWER_ON_IOCTL:
		ret = rda_5991_wifi_power_on();
		break;

	case RDA_WIFI_POWER_OFF_IOCTL:
		ret = rda_5991_wifi_power_off();
		break;

	case RDA_BT_POWER_ON_IOCTL:
		ret = rda_5991_bt_power_on();
		break;

	case RDA_BT_POWER_OFF_IOCTL:
		ret = rda_5991_bt_power_off();
		break;

	case RDA_WIFI_DEBUG_MODE_IOCTL:
		ret = rda_5991_wifi_debug_en();
		break;
		// add for pta
	case RDA_BT_DC_CAL_IOCTL_FIX_5991_LNA_GAIN:
		ret = RDA5991_bt_dc_cal_fix_gain();
		break;
		// add for pta
	default:
		break;
	}

	printk(KERN_INFO "rda_bt_pw_ioctl cmd=0x%02x \n", cmd);
	return ret;
}

int rda_5991_fm_power_on(void)
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

	if (rda_wlan_version() == WLAN_VERSION_91) {

		ret = i2c_read_1_addr_2_data(rda_wifi_rf_client, 0x27, &temp);	//read 0xA7
		if (ret < 0) {
			printk(KERN_INFO
				"%s() read from address(0x%02x) failed! \n",
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
			printk(KERN_INFO
				"%s() read from address(0x%02x) failed! \n",
				__func__, 0xB9);
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

int rda_5991_fm_power_off(void)
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

	if (rda_wlan_version() == WLAN_VERSION_91) {
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
