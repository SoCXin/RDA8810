#include <common.h>
#include "asm/arch/cs_types.h"
#include "hal_gpio.h"
#include "rda_mipi_dsi.h"
#include "rda_lcdc.h"
#include "hal_camera.h"
#include <asm/arch/reg_spi.h>
#include <asm/arch/ispi.h>
#include <asm/io.h>

#define LDO_ACTIVE_SETTING1		(u32) 0x03
#define LDO_ACTIVE_SETTING2		(u32) 0x04

#define CONFIG_DRAM_START		0x80000000
#define CONFIG_TEST_DRAM_ADDR  (CONFIG_DRAM_START + 0x2000000)

#define CSI_BUFFER_SRC		0xa0000000
#define CSI_BUFFER_DST		0xa1000000
#define CSI_FRAME_BUFFER	0xa2000000

#define AP_GPIO_A_MODE		0x11a0900c
#define AP_GPIO_B_MODE		0x11a09010

#define PIN_MUX_LCD		~(0x3FFF << 18)
#define PIN_MUX_CAM		~(0x3FFF << 10)

#define TEST_RESULT_ADDR	0x11C010F4

static const struct rda_dsi_phy_ctrl pll_phy_260mhz = {
	{0x2800, 0x0000, 0x10, 0xbc00, 0x20a, 0x20b},
	{
		{0x6C, 0x53}, {0x10C, 0x3}, {0x108,0x2}, {0x118, 0x4}, {0x11C, 0x0},
		{0x120, 0xC}, {0x124, 0x2}, {0x128, 0x3}, {0x80, 0xE}, {0x84, 0xC},
		{0x130, 0xC}, {0x150, 0x12}, {0x170, 0x87A},
	},
	{
		{0x64, 0x3}, {0x134, 0x3}, {0x138, 0xB}, {0x14C, 0x2C}, {0x13C, 0x7},
		{0x114, 0x40}, {0x170, 0x87A}, {0x140, 0xFF},
	},
};

static const struct lcd_panel_info test_lcd = {
		.width = 480,
		.height = 800,
		.bpp = 16,
		.mipi_pinfo = {
			.data_lane = 2,
			.mipi_mode = DSI_VIDEO_MODE,
			.pixel_format = RGB_PIX_FMT_RGB565,
			.dsi_format = DSI_FMT_RGB565,
			.rgb_order = RGB_ORDER_BGR,
			.trans_mode = DSI_BURST,
			.bllp_enable = TRUE,
			.h_sync_active = 0x1c,
			.h_back_porch = 0x7a,
			.h_front_porch = 0x7b,
			.v_sync_active = 0x8,
			.v_back_porch = 0x5,
			.v_front_porch = 0x6,
			.frame_rate = 60,
			.te_sel = TRUE,
			.dsi_pclk_rate = 260,//260MHz
			.dsi_phy_db = &pll_phy_260mhz,
		}
};

/***************************************************
	Synchronization Short Packet Data Type Codes
	Data Type Description
	0x00 Frame Start Code
	0x01 Frame End Code
	0x02 Line Start Code (Optional)
	0x03 Line End Code (Optional)

	Each image frame shall begin with a Frame Start (FS) Packet containing the Frame Start Code.
	The FS Packet shall be followed by one or more long packets containing image data and zero or more short packets
	containing synchronization codes.Each image frame shall end with a Frame End (FE) Packet containing the Frame End Code.

	Frame Structure with Embedded Data at the Beginning and End of the Frame
	YUV Image Data Types
	Data Type Description
	0x18 YUV420 8-bit
	0x19 YUV420 10-bit
	0x1A Legacy YUV420 8-bit
	0x1B Reserved
	0x1C YUV420 8-bit (Chroma Shifted Pixel Sampling)
	0x1D YUV420 10-bit (Chroma Shifted Pixel Sampling)
	0x1E YUV422 8-bit
	0x1F YUV422 10-bit

	RGB Image Data
	defines the data type codes for RGB data formats described in this section.
	RGB Image Data Types
	Data Type Description
	0x20 RGB444
	0x21 RGB555
	0x22 RGB565
	0x23 RGB666
	0x24 RGB888
	0x25 Reserved
	0x26 Reserved
	0x27 Reserved
*****************************************************/
static void csi_send_frame(int len)
{
	U32 header = 0;

	dsi_enable(FALSE);
	dsi_op_mode(DSI_CMD);		  	//cmd mode
	dsi_pll_on(TRUE);
	rda_write_dsi_reg(0x40,0x2);
	rda_write_dsi_reg(0x44,0x3);
	rda_write_dsi_reg(0x180,0x02);		//Frame Start Code

	header |= (len & 0xff) << 16;
	header |= 0x1e << 8;
	header |= DSI_HDR_LONG_PKT;
	header |= DSI_HDR_HS;
	rda_write_dsi_reg(0x184,header);	//hs cmd mode send
	rda_write_dsi_reg(0x188,0x102); 	//Frame End Code
	dsi_enable(TRUE);
}

static void dsi_hs_frame_init(void)
{
	int i;
	unsigned char *frame = (unsigned char *)CSI_BUFFER_SRC;
	unsigned char *data = (unsigned char *)CSI_FRAME_BUFFER;
	for (i =0; i < 64; i += 4) {
		*(data + i) = 0x11;
		*(data + i + 1) = 0x22;
		*(data + i + 2) = 0x33;
		*(data + i + 3) = 0x44;
	}
	for (i = 0; i < 32; i++) {
		frame = (unsigned char *)CSI_BUFFER_SRC + 8 * i;
		*frame  = 0x11;
		*(frame + 1) = 0x22;
		*(frame + 2) = 0x33;
		*(frame + 3) = 0x00;
		*(frame + 4) = 0x44;
		*(frame + 5) = 0x00;
		*(frame + 6) = 0x00;
		*(frame + 7) = 0x00;
	}
}

static void camera_reg_config(void)
{
	u8 data_lane = 2;
	HAL_CAMERA_CFG_T CamConfig = {0,};
	HAL_CAMERA_IRQ_CAUSE_T mask = {0,0,0,0};
	CamConfig.cam1PdnRemap.gpioId = HAL_GPIO_NONE;
	CamConfig.camPdnRemap.gpioId = HAL_GPIO_NONE;
	CamConfig.camRstRemap.gpioId = HAL_GPIO_NONE;
	CamConfig.cam1RstRemap.gpioId = HAL_GPIO_NONE;

	hal_SetCameraClkOut(0x101); 	// 0x11 = 13M	0x1 = 30M

	CamConfig.rstActiveH = FALSE;
	CamConfig.pdnActiveH = TRUE;
	CamConfig.dropFrame = FALSE;
	CamConfig.camClkDiv = 6;	// 156/x: 13M(x=12), 26M(x=6)
	CamConfig.endianess = NO_SWAP;
	CamConfig.csi_mode  =  TRUE;
	CamConfig.num_d_term_en=21;
	CamConfig.num_hs_settle=21;
	CamConfig.num_c_term_en=55;
	CamConfig.num_c_hs_settle=55;
	CamConfig.csi_cs=0;		// 0 use csi1 . 1 use csi2
	CamConfig.frame_line_number=1;

	CamConfig.cropEnable = FALSE;

	CamConfig.colRatio= 0;
	CamConfig.rowRatio= 0;
	CamConfig.reOrder = 4;

	hal_CameraOpen(&CamConfig);
	hal_CameraIrqSetMask(mask);
	hal_CameraSetLane(data_lane);
}

static int receive_data(unsigned char * data)
{
	int data_len = 64;
	int ret, times = 0;
	HAL_CAMERA_IRQ_CAUSE_T cause = {0};

	HAL_CAMERA_IRQ_CAUSE_T mask = {1,1,1,1};
	hal_CameraStartXfer(data_len, (UINT8*)data);
	hal_CameraIrqSetMask(mask);
	hal_CameraControllerEnable(TRUE);

	csi_send_frame(data_len);
	mdelay(50);
	ret = rda_lcdc_irq_status();
	if (ret)
		return 1;

	while(times < 100) {
		hal_getCameraStatus(&cause);

		if (cause.fstart == 1)
			printf("frame: fstart\n");
		if (cause.fend == 1) {
			hal_CameraControllerEnable(FALSE);
			printf("frame:	fend\n");
			if (hal_CameraStopXfer(FALSE) == XFER_SUCCESS)
				printf("cam:AXI ok\n");
		}
		if (cause.overflow == 1)
			printf("camera_cause .overflow\n");
		if (cause.dma == 1)
			printf("frame: dma donn\n");
		if (cause.fend == 1)
			break;
		udelay(100);
		times++;
	}
	if (times == 100)
		return 2;
	else
		return 0;
}

static void set_pmu(void)
{
	u32 value = 0;

	ispi_open(1);
	value = ispi_reg_read(LDO_ACTIVE_SETTING1);
	value |= 0x3 << 7;
	ispi_reg_write(LDO_ACTIVE_SETTING1, value);		// v_cam_act, v_lcd_act

	value = ispi_reg_read(LDO_ACTIVE_SETTING2);
	value |= 0x3 << 9;
	ispi_reg_write(LDO_ACTIVE_SETTING2, value);		//vcam=1.8v, vlcd=1.8v
	ispi_open(0);
}

static void set_pinmux(void)
{
	unsigned long temp;

	/* pinmux for lcd */
	temp = readl(AP_GPIO_A_MODE);
	temp &= PIN_MUX_LCD;
	writel(temp, AP_GPIO_A_MODE);
	/* pinmux for cam */
	temp = readl(AP_GPIO_B_MODE);
	temp &= PIN_MUX_CAM;
	writel(temp, AP_GPIO_B_MODE);
}

int test_dsi_csi_loop(int times)
{
	int test_times = 1;
	int ret;

	set_pmu();
	set_pinmux();
	dsi_hs_frame_init();

	while(test_times <= times) {
		printf("\n%s: times %d / %d ...\n", __func__, test_times, times);
		memset((char *)CSI_BUFFER_DST, 0, 100);
		camera_reg_config();
		rda_lcdc_set(test_lcd.mipi_pinfo.dsi_phy_db);
		dsi_config(&test_lcd);
		set_lcdc_for_cmd(CSI_BUFFER_SRC, 16);
		ret = receive_data((unsigned char *)CSI_BUFFER_DST);

		if (ret) {
			printf("mipi_loop failed, ");
			writel(0xdeaddead, TEST_RESULT_ADDR);
			if (ret == 1) {
				printf("dsi failed\n");

				return 1;
			} else if (ret == 2) {
				printf("csi failed\n");

				return 1;
			}
		}

		// if ret == 0, dsi and csi both correct, loop should be successful
		if(memcmp((char*)CSI_BUFFER_DST, (char*)CSI_FRAME_BUFFER, 64)) {
			printf("mipi_loop failed, unknown reason\n");
			writel(0xdeaddead, TEST_RESULT_ADDR);	//unknown reason

			return 1;
		} else {
			printf("loop success\n");
			writel(0x55aaaa55, TEST_RESULT_ADDR);	//success
		}

		printf("test done\n\n");
		/*
		char  *p= (char *)CSI_BUFFER_DST;
		for (i = 0; i < 64; i++) {
			printf("%x\n", *p++);
		}*/
		test_times++;
	}

	return 0;
}

