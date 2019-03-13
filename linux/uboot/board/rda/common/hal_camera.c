////////////////////////////////////////////////////////////////////////////////
//																			//
//			Copyright (C) 2012-2022, RDA, Inc.			//
//							All Rights Reserved							 //
//																			//
//	  This source code is the property of Coolsand Technologies and is	  //
//	  confidential.  Any  modification, distribution,  reproduction or	  //
//	  exploitation  of  any content of this file is totally forbidden,	  //
//	  except  with the  written permission  of RDA.	  //
//																			//
////////////////////////////////////////////////////////////////////////////////

#include <common.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/reg_camera.h>

#include "hal_camera.h"
#include "hal_gpio.h"

#include <asm/arch/reg_ifc.h>
#include <asm/arch/ifc.h>
#include <asm/arch/reg_cfg_regs.h>

#ifdef TGT_CAMERA_PDN_CHANGE_WITH_ATV
extern BOOL IsAtvPowerOn(void);
#endif

VOID hal_print(UINT32 x)
{
	 printf("%x\n", (unsigned int)x);
}
#define hal_HstSendEvent(x) hal_print(x)

#define HAL_ASSERT(...)
#define HAL_TRACE(...)

#define CAMERA_DEBUG(BOOL, mess)				   \
	if (!(BOOL)) {			\
		printf("%s\n",mess);							 \
	}


// ============================================================================
// MACROS
// ============================================================================
//
/// Flag to enable the Camera Module test mode. This is useful when the
/// Debug Bus needs to be used: the Camera Bus and the Debug Bus are
/// multiplexed in the chip IOMux, so they cannot be used both at same
/// time. The idea of the Camera test mode is to displays a dummy image
/// instead of getting its data from the sensor.
/// So, when CT_RELEASE=cool_profile, the test mode is enabled: the PXTS,
/// EXL or signal spy cannot be used along with the Camera. When the camera
/// needs to be used, use CT_RELEASE=debug, for instance.
#ifdef ENABLE_PXTS
#define CAM_TEST_MODE	 CAMERA_TEST
#else
#define CAM_TEST_MODE	 0 // CAMERA_TEST
#endif

#ifdef HAL_CAMERA_PRINTF
#define HAL_CAMERA_TRACE(a, ...)	HAL_TRACE(HAL_CAMERA_TRC, 0, a, ##__VA_ARGS__)
#else
#define HAL_CAMERA_TRACE(a, ...)
#endif
// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

PRIVATE UINT8  g_halCameraIfcChan  = 0xff;

PRIVATE HAL_CAMERA_CFG_T g_halCameraConfig = {
	.rstActiveH = FALSE,
	.pdnActiveH = TRUE,
	.dropFrame  = FALSE,
	.camClkDiv  = 12,
	.endianess  = BYTE_SWAP,
	.camId	  = 0,
	.colRatio   = COL_RATIO_1_1,
	.rowRatio   = ROW_RATIO_1_1,
	.camPdnRemap.gpioId=HAL_GPIO_NONE,
	.camRstRemap.gpioId=HAL_GPIO_NONE,
};


// =============================================================================
// g_halCameraIrqHandler
// -----------------------------------------------------------------------------
/// Variable to store the user irq handler for the camera
/// interruption.
// =============================================================================
PRIVATE HAL_CAMERA_IRQ_HANDLER_T g_halCameraIrqHandler;

PUBLIC VOID hal_CameraSetVsyncInvert(BOOL polarity)
{
	if (polarity){
		// VSYNC low effective
		hwp_camera->CTRL |= CAMERA_VSYNC_POL_INVERT;
#if (CHIP_HAS_ASYNC_TCU)
		hwp_camera->CAM_SPI_REG_0 |= CAM_SPI_REG_VSYNC_INV_EN;
#endif // defined (CHIP_HAS_ASYNC_TCU)
	} else{
		// VSYNC high effective
		hwp_camera->CTRL &= ~CAMERA_VSYNC_POL_INVERT;
#if (CHIP_HAS_ASYNC_TCU)
		hwp_camera->CAM_SPI_REG_0 &= ~CAM_SPI_REG_VSYNC_INV_EN;
#endif // defined (CHIP_HAS_ASYNC_TCU)
	}
}
PUBLIC UINT32 hal_CameraIrqstatus(void)
{
	return hwp_camera->STATUS;
}

// ============================================================================
// FUNCTIONS
// ============================================================================

// =============================================================================
// hal_CameraReset(BOOL InReset)
// -----------------------------------------------------------------------------
/// Puts the Camera sensor in Reset or out of Reset.
///
/// @param InReset if true, put the external camera sensor in reset
// =============================================================================
PUBLIC VOID hal_CameraReset(BOOL InReset)
{
	if (InReset)
		hwp_camera->CMD_SET = CAMERA_RESET;
	else
		hwp_camera->CMD_CLR = CAMERA_RESET;
}

#if (CHIP_HAS_ASYNC_TCU)
PRIVATE VOID hal_camera_spi_reg_init()
{
	// clear all SPI camera related regs
	hwp_camera->CAM_SPI_REG_0 = 0;
	hwp_camera->CAM_SPI_REG_1 = 0;
	hwp_camera->CAM_SPI_REG_2 = 0;
	hwp_camera->CAM_SPI_REG_3 = 0;
	hwp_camera->CAM_SPI_REG_4 = 0;
	hwp_camera->CAM_SPI_REG_5 = 0;
	hwp_camera->CAM_SPI_REG_6 = 0;

	// set all SPI camera related regs to default
	hwp_camera->CAM_SPI_REG_5 = 0x00ffffff; // sync code 0xffffff by default
	// by default, frame start id 0x01,frame end id 0x00,line start id 0x02,packet id 0x40
	hwp_camera->CAM_SPI_REG_6 = 0x01000240;
}
#endif


void hal_config_pinmux_for_camera(void)
{
	hwp_configRegs->AP_GPIO_B_Mode=(hwp_configRegs->AP_GPIO_B_Mode)&(~0x00fffc00);
}

void hal_config_pinmux_for_camera_csi2(void)
{
	hwp_configRegs->Alt_mux_select=((hwp_configRegs->Alt_mux_select)&(~0x00000030))|(1<<4);
}

PUBLIC VOID hal_CameraOpen(HAL_CAMERA_CFG_T* camConfig)
{
	UINT32 decimDiv = 0;

	if (HAL_UNKNOWN_CHANNEL != g_halCameraIfcChan){
		printf("hal_CameraOpen: Camera already open\n");
		return;
	}

	g_halCameraConfig = *camConfig;
	hal_config_pinmux_for_camera();

	if(camConfig->csi_cs == 1){
		hal_config_pinmux_for_camera_csi2();
	}

#if (CHIP_HAS_ASYNC_TCU)
	HAL_ASSERT(((camConfig->spi_mode == SPI_MODE_MASTER2_1) ||((camConfig->spi_mode == SPI_MODE_MASTER1))
						|| (camConfig->spi_mode == SPI_MODE_MASTER2_2)
						|| (camConfig->spi_mode == SPI_MODE_MASTER2_4)
						|| (camConfig->spi_mode == SPI_MODE_NO)),
			   "hal_CameraOpen: unsupported spi mode");
#endif

	switch (camConfig->rowRatio){
		case ROW_RATIO_1_1:
			decimDiv |= CAMERA_DECIMROW_ORIGINAL;
			break;
		case ROW_RATIO_1_2:
			decimDiv |= CAMERA_DECIMROW_DIV_2;
			break;
		case ROW_RATIO_1_3:
			decimDiv |= CAMERA_DECIMROW_DIV_3;
			break;
		case ROW_RATIO_1_4:
			decimDiv |= CAMERA_DECIMROW_DIV_4;
			break;
		default:
			HAL_ASSERT(FALSE, "Camera: Wrong row ratio: %d", camConfig->rowRatio);
			break;
	}

	switch (camConfig->colRatio){
		case COL_RATIO_1_1:
			decimDiv |= CAMERA_DECIMCOL_ORIGINAL;
			break;
		case COL_RATIO_1_2:
			decimDiv |= CAMERA_DECIMCOL_DIV_2;
			break;
		case COL_RATIO_1_3:
			decimDiv |= CAMERA_DECIMCOL_DIV_3;
			break;
		case COL_RATIO_1_4:
			decimDiv |= CAMERA_DECIMCOL_DIV_4;
			break;
		default:
			HAL_ASSERT(FALSE, "Camera: Wrong column ratio: %d", camConfig->colRatio);
			break;
	}
	hwp_camera->CTRL  = CAM_TEST_MODE;
	hwp_camera->CTRL |= CAMERA_DATAFORMAT_YUV422;
	hwp_camera->CTRL |= decimDiv;

	if (camConfig->cropEnable)
	{
		hwp_camera->CTRL |= CAMERA_CROPEN_ENABLE;
		hwp_camera->DSTWINCOL =
			CAMERA_DSTWINCOLSTART(camConfig->dstWinColStart) |CAMERA_DSTWINCOLEND(camConfig->dstWinColEnd);
		hwp_camera->DSTWINROW =
			CAMERA_DSTWINROWSTART(camConfig->dstWinRowStart) |CAMERA_DSTWINROWEND(camConfig->dstWinRowEnd);
	}

	if(!g_halCameraConfig.rstActiveH)
		hwp_camera->CTRL |= CAMERA_RESET_POL;
	if(!g_halCameraConfig.pdnActiveH)
		hwp_camera->CTRL |= CAMERA_PWDN_POL;
	if (g_halCameraConfig.dropFrame)
		hwp_camera->CTRL |= CAMERA_DECIMFRM_DIV_2;

	hwp_camera->CTRL |= CAMERA_REORDER(camConfig->reOrder);

#if (CHIP_HAS_ASYNC_TCU)
	hal_camera_spi_reg_init();
	if ((camConfig->spi_mode != SPI_MODE_NO)&&((camConfig->csi_mode == FALSE))){
		// CAM_SPI_REG_0
		hwp_camera->CAM_SPI_REG_0
					|= CAM_SPI_REG_LINE_PER_FRM(camConfig->spi_pixels_per_column);
		hwp_camera->CAM_SPI_REG_0
					|= CAM_SPI_REG_BLK_PER_LINE((camConfig->spi_pixels_per_line) >> 1);
		hwp_camera->CAM_SPI_REG_0
					|= CAM_SPI_REG_YUV_OUT_FMT(camConfig->spi_yuv_out);
		hwp_camera->CAM_SPI_REG_0
					|= (camConfig->spi_href_inv) ? CAM_SPI_REG_HREF_INV_EN : 0;
		hwp_camera->CAM_SPI_REG_0
					|= (camConfig->spi_little_endian_en) ? CAM_SPI_REG_LITTLE_END_EN : 0;
		// CAM_SPI_REG_1
		hwp_camera->CAM_SPI_REG_1 |= CAM_SPI_REG_CLK_DIV(camConfig->spi_ctrl_clk_div);
		// Other REGs according to the spi mode
		switch (camConfig->spi_mode){
		case SPI_MODE_MASTER2_1:
			hwp_camera->CAM_SPI_REG_4 |=
						CAM_SPI_REG_BLK_PER_PACK((camConfig->spi_pixels_per_line) >> 1) |
						CAM_SPI_REG_LINE(0) |
						CAM_SPI_REG_PACK_SIZE_FROM_REG |
						CAM_SPI_REG_IMG_WIDTH_FROM_REG |
						CAM_SPI_REG_IMG_HEIGHT_FROM_REG |
						CAM_SPI_REG_MASTER2_EN;
			break;
		case SPI_MODE_MASTER2_2:
			hwp_camera->CAM_SPI_REG_4 |=
						CAM_SPI_REG_BLK_PER_PACK((camConfig->spi_pixels_per_line) >> 1) |
						CAM_SPI_REG_LINE(1) |
						CAM_SPI_REG_PACK_SIZE_FROM_REG |
						CAM_SPI_REG_IMG_WIDTH_FROM_REG |
						CAM_SPI_REG_IMG_HEIGHT_FROM_REG |
						CAM_SPI_REG_MASTER2_EN;
			break;
		case SPI_MODE_MASTER2_4:
			hwp_camera->CAM_SPI_REG_4 |=
						CAM_SPI_REG_BLK_PER_PACK((camConfig->spi_pixels_per_line) >> 1) |
						CAM_SPI_REG_LINE(2) |
						CAM_SPI_REG_PACK_SIZE_FROM_REG |
						CAM_SPI_REG_IMG_WIDTH_FROM_REG |
						CAM_SPI_REG_IMG_HEIGHT_FROM_REG |
						CAM_SPI_REG_MASTER2_EN;
			break;
	   case SPI_MODE_MASTER1:
			hwp_camera->CAM_SPI_REG_0 |= CAM_SPI_REG_MASTER_EN;
			if (camConfig->spi_ssn_high_en)
				hwp_camera->CAM_SPI_REG_1 |=  CAM_SPI_REG_SSN_HIGH_EN;
			break;
		default:
			break;
		}
	}
#endif // defined (CHIP_HAS_ASYNC_TCU)

	if (camConfig->csi_mode == TRUE){
		hwp_camera->CAM_SPI_REG_0 = 0x200;

		//hwp_mipi_csi->CSI_OBSERVE_CLK = 0x1;
		serial_puts("set csi reg  \r\n");
		hwp_camera->CSI_CONFIG_REG0 =0xA0000000|(camConfig->num_d_term_en&0xff)|((camConfig->frame_line_number&0x3ff)<<8);
		hwp_camera->CSI_CONFIG_REG1 = 0x00020000|(camConfig->num_hs_settle&0xff);
		hwp_camera->CSI_CONFIG_REG2= (camConfig->num_c_term_en<<16)|camConfig->num_c_hs_settle;//; //;//;/
		hwp_camera->CSI_CONFIG_REG3= 0x9C0A0000|((camConfig->csi_cs&1)<<20)|(camConfig->csi_lane_2v8<<11);//0x9c0a0800; for csi1  0x9c1a***  for csi2
		hwp_camera->CSI_CONFIG_REG4= 0xffffffff;
		hwp_camera->CSI_CONFIG_REG5= 0x40dc4000;
		hwp_camera->CSI_CONFIG_REG6= 0x800420ea;
		hwp_camera->CSI_ENABLE_PHY = 1;

		hal_HstSendEvent(0x12346666);
	}

	// Configure Camera Clock Divider
	hal_CameraSetupClockDivider(g_halCameraConfig.camClkDiv);
	// set vsync pole
	hal_CameraSetVsyncInvert(camConfig->vsync_inv);

	//  hwp_camera->CTRL |= (1<<31);  //  rda test mode

	hal_CameraReset(FALSE);
	hal_CameraPowerDown(FALSE);
}


// =============================================================================
// hal_CameraClose
// -----------------------------------------------------------------------------
/// Power off the camera sensor by setting the PowerDown bit.
/// Resets the camera sensor by enabling the Camera Reset bit.
/// This function can only be called after the camera transfer has been stopped
/// by a call to #hal_CameraStopXfer().
// =============================================================================
PUBLIC VOID hal_CameraClose(VOID)
{
	hal_CameraControllerEnable(FALSE);
	hal_CameraReset(TRUE);
	hal_CameraPowerDown(TRUE);
}

PUBLIC VOID hal_CameraPowerDownBoth(BOOL PowerDown)
{

}

PUBLIC VOID hal_CameraClkOut(BOOL out)
{
	if (out)
		hwp_camera->CLK_OUT = 0x11;
	else
		hwp_camera->CLK_OUT = 0x3f00;

	return;
}

// // 0x11 = 13M   0x1 = 30M
PUBLIC VOID hal_SetCameraClkOut(u16 value)
{
	hwp_camera->CLK_OUT = value;
}

PUBLIC VOID hal_CameraPowerDown(BOOL PowerDown)
{
	if(PowerDown){
		hwp_camera->CMD_SET = CAMERA_PWDN;
	}else{
		hwp_camera->CMD_CLR = CAMERA_PWDN;
	}

	return;
}


// =============================================================================
// hal_CameraControllerEnable(BOOL enable)
// -----------------------------------------------------------------------------
// =============================================================================
PUBLIC VOID hal_CameraControllerEnable(BOOL enable)
{
	if(enable){
		// Turn on controller and apbi
		hwp_camera->CMD_SET  =  CAMERA_FIFO_RESET;
		hwp_camera->CTRL	|=  CAMERA_DATAFORMAT_YUV422;
		hwp_camera->CTRL	|=  CAMERA_ENABLE;
	} else{
		// Turn off controller and apbi
		hwp_camera->CTRL	&= ~CAMERA_ENABLE;
	}
}

PUBLIC VOID hal_CameraIrqSetHandler(HAL_CAMERA_IRQ_HANDLER_T handler)
{
	g_halCameraIrqHandler = handler;
}



// =============================================================================
// hal_CameraIrqConfig()
// -----------------------------------------------------------------------------
/// Configure the desired interrupts
///
/// @param mask Mask to enable specific interrupts.  Valid interrupts are
/// liste in HAL_CAMERA_IRQ_CAUSE_T.
// =============================================================================
PUBLIC VOID hal_CameraIrqSetMask(HAL_CAMERA_IRQ_CAUSE_T mask)
{
	UINT32 realMask = 0;

	if (mask.overflow)
		realMask |= CAMERA_OVFL;

	if (mask.fstart)
		realMask |= CAMERA_VSYNC_R;

	if (mask.fend)
		realMask |= CAMERA_VSYNC_F;

	if (mask.dma)
		realMask |= CAMERA_DMA_DONE;

	hwp_camera->IRQ_CLEAR = realMask;
	hwp_camera->IRQ_MASK  = realMask;
}

PROTECTED VOID hal_CameraIrqHandler(UINT8 interruptId)
{
	HAL_CAMERA_IRQ_CAUSE_T cause = {0, };
	UINT32				 realCause;

	realCause = hwp_camera->IRQ_CAUSE;

	// Clear IRQ
	hwp_camera->IRQ_CLEAR = realCause;

	if(realCause & CAMERA_OVFL)
		cause.overflow = 1;

	if(realCause & CAMERA_VSYNC_R)
		cause.fstart = 1;

	if(realCause & CAMERA_VSYNC_F)
		cause.fend= 1;

	if(realCause & CAMERA_DMA_DONE)
		cause.dma = 1;

	// Call User handler
	if (g_halCameraIrqHandler)
		g_halCameraIrqHandler(cause);
}

PUBLIC UINT32 hal_getCameraStatus(HAL_CAMERA_IRQ_CAUSE_T *cause)
{
  //  HAL_CAMERA_IRQ_CAUSE_T cause = {0, };
	UINT32				 realCause;

	realCause = hwp_camera->IRQ_CAUSE;
	// Clear IRQ
	hwp_camera->IRQ_CLEAR = realCause;


	if(realCause & CAMERA_OVFL)
		cause->overflow = 1;

	if(realCause & CAMERA_VSYNC_R)
	  cause->fstart = 1;

	if(realCause & CAMERA_VSYNC_F)
		cause->fend= 1;

	if(realCause & CAMERA_DMA_DONE)
		cause->dma = 1;

  	return realCause;
}

  UINT32 hal_AxiGetTc(void)
  {
	   return hwp_camera->CAM_TC_COUNT;
  }

// =============================================================================
// hal_CameraStopXfer
// -----------------------------------------------------------------------------
/// Must be called at the end of the Frame Capture
/// If an underflow occured and the IFC tranfer is not complete,
/// this function will handle the channel release
///
/// @param stop If \c TRUE, stops the camera controller.
/// @return 0 when the IC transfer was complete
///		 1 when the IFC transfer was not complete and the channel had to be released
// =============================================================================
PUBLIC HAL_CAMERA_XFER_STATUS_T hal_CameraStopXfer(BOOL stop)
{
	UINT32 tc = 0;

	printf(" stop xfer ");
	// Disable the Camera controller in any case to avoid toggling
	if(stop)
		hal_CameraControllerEnable(FALSE);

	tc = hal_AxiGetTc();

	printf("\r\n ytt   loss  tc =0x ");
	hal_HstSendEvent(tc);

	if(tc != 0)
		printf("\r\n---------------- --------------------------------------- \n");

	printf("\r\n receive line = ");
	hal_HstSendEvent((  640*480*2 - tc )/(640*2));
	printf("\r\n");

	if(tc != 0){

		if(!stop){
			hal_CameraControllerEnable(FALSE);
			hal_CameraControllerEnable(TRUE);
		}

		// Try to determine why we missed data
		if(hwp_camera->IRQ_CAUSE & CAMERA_OVFL)
			printf("Overflow during transfer between camera module and IFC");
		else
			printf("Missing data between external camera and camera module");

		return XFER_NOT_FINISHED;
	}

	if(!(hwp_camera->IRQ_CAUSE & CAMERA_OVFL))
		return XFER_SUCCESS;

	return(XFER_FINISHED_WITH_OVERFLOW);
}


// =============================================================================
// hal_CameraStartXfer
// -----------------------------------------------------------------------------
/// This function begins the IFC transfer of the camera data.  The camera
/// itself is reset and the camera module internal fifo is cleared.  The IFC
/// transfer is then started.
/// @param BufSize This is the size of the buffer in _bytes_
/// @param Buffer Pointer to the video buffer where the IFC will store the data.
/// @return IFC channel number.
// =============================================================================
PUBLIC UINT8 hal_CameraStartXfer(UINT32 bufSize, UINT8* buffer)
{
	REG32 value ;

	hwp_camera->CMD_SET   =  CAMERA_FIFO_RESET;
	hwp_camera->IRQ_CLEAR = CAMERA_OVFL;	   // Clear the Overflow bit for checking later
	//  g_halCameraIfcChan	= hal_IfcTransferStart(HAL_IFC_CAMERA_RX, buffer, bufSize, HAL_IFC_SIZE_32_MODE_AUTO);
	//   HAL_CAMERA_TRACE("hal_CameraStartXfer: channel %i", g_halCameraIfcChan);
	hwp_camera->CAM_FRAME_START_ADDR   =(UINT32)  buffer;
	hwp_camera->CAM_FRAME_SIZE   =  bufSize;
	hwp_camera->CAM_AXI_CONFIG =( hwp_camera->CAM_AXI_CONFIG)|0x20;
	value = hwp_camera->CAM_AXI_CONFIG;
	printf("cam: 2013 hal_CameraStartXfer	  \r\n");
	hal_HstSendEvent((UINT32)bufSize);
	hal_HstSendEvent((UINT32)value);
	printf("\n");

	return 1;
}

// =============================================================================
// hal_CameraWaitXferFinish
// -----------------------------------------------------------------------------
///if IFC transfer not finish, do nothg.
/// @return 0 when the IC transfer was complete
///		 1 when the IFC transfer was not complete
// =============================================================================

PUBLIC HAL_CAMERA_XFER_STATUS_T hal_CameraWaitXferFinish(void)
{
	return XFER_SUCCESS;
}

// =============================================================================
// hal_CameraSetupClockDivider
// -----------------------------------------------------------------------------
// =============================================================================
PUBLIC VOID hal_CameraSetupClockDivider(UINT8 divider)
{


#if 0 // ytt-temp
	UINT32 newClkScale;
	UINT32 ctrlReg;

	// Save enable bit
	ctrlReg = hwp_sysCtrl->Cfg_Clk_Camera_Out & SYS_CTRL_CLK_CAMERA_OUT_EN_ENABLE;

	// Saturate the divider to the maximum value supported
	// by the hardware.
	if(divider-2 > (SYS_CTRL_CLK_CAMERA_OUT_DIV_MASK>>SYS_CTRL_CLK_CAMERA_OUT_DIV_SHIFT))
	{
		newClkScale = (SYS_CTRL_CLK_CAMERA_OUT_DIV_MASK>>SYS_CTRL_CLK_CAMERA_OUT_DIV_SHIFT) + 2;
	}
	else if(divider < 2)
	{
		newClkScale = 2;
	}
	else
	{
		newClkScale = divider;
	}

	// Divider is register value+2, so we take off 2 here.
	ctrlReg |=  SYS_CTRL_CLK_CAMERA_DIV_SRC_SEL_156_MHZ	  |
				SYS_CTRL_CLK_CAMERA_OUT_SRC_SEL_FROM_DIVIDER |
				SYS_CTRL_CLK_CAMERA_OUT_DIV(newClkScale-2)
#if (CHIP_HAS_ASYNC_TCU)
				| SYS_CTRL_CLK_SPI_CAMERA_DIV(0)
#endif
				;

	// Restore initial config with new clock scal.
	hwp_sysCtrl->Cfg_Clk_Camera_Out = ctrlReg;
#endif
}

PUBLIC VOID hal_CameraSetLane(UINT8 lane)
{
	hwp_camera->CSI_CONFIG_REG3 &= 0x3fffffff;

	if (lane == 1)
		hwp_camera->CSI_CONFIG_REG3 |= (0x2 << 30);
	else
		hwp_camera->CSI_CONFIG_REG3 |= (0x1 << 30);
}


