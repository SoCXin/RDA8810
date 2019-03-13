#ifndef __ASM_ARCH_DEVICES_H
#define __ASM_ARCH_DEVICES_H

#include <asm/sizes.h>
#include <mach/ifc.h>

#define RDA_UART_DRV_NAME         "rda-uart"
#define RDA_MMC_DRV_NAME          "rda-mmc"
#define RDA_FB_DRV_NAME           "rda-fb"
#define RDA_DBI_PANEL_DRV_NAME    "rda-dbi-panel"
#define RDA_DPI_PANEL_DRV_NAME    "rda-dpi-panel"
#define RDA_DSI_PANEL_DRV_NAME    "rda-dsi-panel"
#define RDA_BL_DRV_NAME           "rda-backlight"
#define RDA_TS_DRV_NAME           "rda-ts"
#define RDA_I2C_DRV_NAME          "rda-i2c"
#define RDA_NAND_DRV_NAME         "rda-nand"
#define RDA_NAND_HD_DRV_NAME      "rda-nand-hd"
#define RDA_SPI_NAND_DRV_NAME     "rda-spinand"
#define RDA_KEYPAD_DRV_NAME       "rda-keypad"
#define RDA_DMA_DRV_NAME          "rda-dma"
#define RDA_CAMERA_DRV_NAME       "rda-camera"
#define RDA_SENSOR_DRV_NAME       "rda-sensor"
#define RDA_GSENSOR_DRV_NAME      "rda-gsensor"
#define RDA_LIGHTSENSOR_DRV_NAME  "rda-lightsensor"
#define RDA_GOUDA_DRV_NAME        "rda-gouda"
#define RDA_LCDC_DRV_NAME         "rda-lcdc"
#define RDA_ION_DRV_NAME          "rda-ion"
#define RDA_SPI_DRV_NAME          "rda-spi"
#define RDA_VIBRATOR_DRV_NAME	  "rda-vibrator"
#define RDA_LEDS_DRV_NAME	"rda-leds"
#define RDA_COMREG0_DRV_NAME	"rda-comreg0"
#define RDA_MD_DRV_NAME           "rda-md"
#define RDA_MSYS_DRV_NAME         "rda-msys"
#define RDA_RTC_DRV_NAME          "rda-rtc"
#define RDA_AUIFC_DRV_NAME        "rda-audifc"
#define RDA_AIF_DRV_NAME          "rda-aif"
#define RDA_CODEC_DRV_NAME        "rda-codec"
#define RDA_PCM_DRV_NAME          "rda-pcm"
#define RDA_HEADSET_DRV_NAME      "rda-headset"
#define RDA_PWM_DRV_NAME          "rda_pwm"
#define RDA_SMD_DRV_NAME          "rda_smd"
#define RDA_RMNET_DRV_NAME        "rda_rmnet"
#define RDA_GPIOC_DRV_NAME        "rda-gpioc"

/****************************************************************\
 * RDA UART Controller                                          *
\****************************************************************/
struct rda_uart_device_data {
	unsigned char dev_id;
	unsigned int uartclk;
	unsigned int rxdmarequestid;
	unsigned int txdmarequestid;
	unsigned int wakeup;
};

/****************************************************************\
 * RDA MMC Controller                                           *
\****************************************************************/
struct rda_mmc_device_data {
	unsigned int		f_min;
	unsigned int		f_max;
	unsigned int		mclk_adj;
	unsigned int		ocr_avail;
	unsigned long		caps;
	unsigned char		eirq_enable;
	unsigned long		eirq_gpio;
	unsigned int		eirq_sense;
	int			debounce;
	unsigned char		sys_suspend;
	unsigned int		pm_caps;	/* supported pm features */
	const char *		dev_label;
	int			clk_inv;	/* clock phase parameter */
	int			det_pin;
};

#define MMC_MFR_TOSHIBA         0x11
#define MMC_MFR_SAMSUNG         0x15
#define MMC_MFR_SANDISK         0x45
#define MMC_MFR_HYNIX           0x90
#define MMC_MFR_MICRON          0x13
#define MMC_MFR_MICRON1         0xfe
#define MMC_MFR_GIGADEVICE      0xC8

struct emmc_mfr_mclk_adj_inv{
        u8 mfr_id;
        u8 mclk_adj;
        u8 mclk_inv;
        u8 reserved;
};

/****************************************************************\
 * RDA Framebuffer                                              *
\****************************************************************/
struct rda_fb_device_data {
	unsigned int fbclk;
};

/****************************************************************\
 * RDA I2C Master                                               *
\****************************************************************/
struct rda_i2c_device_data {
	unsigned int speed;
};

/****************************************************************\
 * Touch Screen on I2C                                          *
\****************************************************************/
struct rda_ts_device_data {
	int use_irq;
	unsigned long irqflags;
};

/****************************************************************\
 * Gsensor  on I2C					  *
\****************************************************************/
struct rda_gsensor_device_data {
	unsigned long irqflags;
};

/****************************************************************\
 * Lightsensor  on I2C					  *
\****************************************************************/
struct rda_lightsensor_device_data {
	unsigned long irqflags;
	unsigned long irqpin;
};

/****************************************************************\
 * NAND Controller                                              *
\****************************************************************/
struct rda_nand_device_data {
	unsigned int max_clock;
};

/****************************************************************\
 * SPINAND Controller                                              *
\****************************************************************/
struct rda_spinand_device_data {
	unsigned int max_clock;
};

/****************************************************************\
 * DMA Controller                                               *
\****************************************************************/
struct rda_dma_ch {
	u8 id;
	const char *dev_name;
	void (*callback) (unsigned char ch, void *data);
	void *data;
};

#ifdef CONFIG_ARCH_RDA8850E
#define RDA_DMA_CHANS 4
#else
#define RDA_DMA_CHANS 1
#endif
struct rda_dma_device_data {
	u32 ch_num;
	u32 ch_inuse_bitmask;
	struct rda_dma_ch chan[RDA_DMA_CHANS];
};

/****************************************************************\
 * AUDIFC Controller                                               *
\****************************************************************/
struct rda_audifc_ch {
	int id;
	u8 inuse;
	const char *dev_name;
	void (*callback) (int ch, int state, void *data);
	void *data;
};

struct rda_audifc_device_data {
	int ch_num;
	struct rda_audifc_ch chan[2];
};
/****************************************************************\
 * AIF Controller                                               *
\****************************************************************/
struct rda_aif_ch {
	int id;
	const char *dev_name;
	void (*callback) (int ch, int state, void *data);
	void *data;
};

struct rda_aif_device_data {
	int ch_num;
	struct rda_aif_ch chan[2];
};
/****************************************************************\
 * Keypad Controller                                            *
\****************************************************************/
struct rda_keypad_device_data {
	unsigned int dbn_time;
	unsigned int itv_time;
	unsigned int rows;
	unsigned int cols;
	unsigned int gpio_volume_up;
	unsigned int gpio_volume_down;
};

/****************************************************************\
 * Camera Controller                                            *
\****************************************************************/
struct rda_camera_device_data {
	u8 hsync_act_low;
	u8 vsync_act_low;
	u8 pclk_act_falling;
};

/****************************************************************\
 * Backlight                                            *
\****************************************************************/
struct rda_bl_device_data {
	unsigned int min;
	unsigned int max;
};

struct rda_bl_pwm_device_data {
	unsigned int min;
	unsigned int max;
	unsigned long pwm_clk;
	int gpio_bl_on;
	int gpio_bl_on_valid;
	int pwm_invert;
	int pwt_used;
};

/****************************************************************\
 * Leds                                            *
\****************************************************************/
struct rda_led_device_data {
	char *led_name;
	char *ldo_name;
	int is_keyboard;
	int trigger;
};

/****************************************************************\
 * RDA SPI Master                                               *
\****************************************************************/
struct rda_spi_device_data {
	unsigned char ifc_rxchannel;
	unsigned char ifc_txchannel;
	unsigned char csnum;
	unsigned char dmaMode;
};

typedef enum {
	/* Delay of 0 half-period */
	RDA_SPI_HALF_CLK_PERIOD_0,
	/* Delay of 1 half-period */
	RDA_SPI_HALF_CLK_PERIOD_1,
	/* Delay of 2 half-period */
	RDA_SPI_HALF_CLK_PERIOD_2,
	/* Delay of 3 half-period */
	RDA_SPI_HALF_CLK_PERIOD_3,

	RDA_SPI_HALF_CLK_PERIOD_QTY
} RDA_SPI_DELAY_T;

typedef enum {
	/* 1 Data spot is empty in the Tx FIFO   */
	RDA_SPI_TX_TRIGGER_1_EMPTY,
	/* 2 Data spots are empty in the Tx FIFO */
	RDA_SPI_TX_TRIGGER_4_EMPTY,
	/* 8 Data spots are empty in the Tx FIFO */
	RDA_SPI_TX_TRIGGER_8_EMPTY,
	/* 12 Data spots are empty in the Tx FIFO */
	RDA_SPI_TX_TRIGGER_12_EMPTY,

	RDA_SPI_TX_TRIGGER_QTY
} RDA_SPI_TX_TRIGGER_CFG_T;

typedef enum {
	/* 1 Data received in the Rx FIFO */
	RDA_SPI_RX_TRIGGER_1_BYTE,
	/*  2 Data received in the Rx FIFO */
	RDA_SPI_RX_TRIGGER_4_BYTE,
	/* 3 Data received in the Rx FIFO */
	RDA_SPI_RX_TRIGGER_8_BYTE,
	/* 4 Data received in the Rx FIFO */
	RDA_SPI_RX_TRIGGER_12_BYTE,

	RDA_SPI_RX_TRIGGER_QTY
} RDA_SPI_RX_TRIGGER_CFG_T;

/* ************************************************************
// RDA_SPI_TRANSFERT_MODE_T
// Data transfert mode: via DMA or direct.
// To allow for an easy use of the SPI modules, a non blocking Hardware
// Abstraction Layer interface is provided. Each transfer direction
// (send/receive) can be configured as:
*************************************************************** */
typedef enum {
	// Direct polling: The application sends/receives the data directly to/from
	// the hardware module. The number of bytes actually sent/received is
	// returned. No Irq is generated.
	RDA_SPI_DIRECT_POLLING = 0,

	// Direct IRQ: The application sends/receives the data directly to/from
	// the hardware module. The number of bytes actually sent/received is
	// returned. An Irq can be generated when the Tx/Rx FIFO reaches the
	// pre-programmed level.
	RDA_SPI_DIRECT_IRQ,

	// DMA polling: The application sends/receives the data through a DMA to
	// the hardware module. The function returns 0 when no DMA channel is
	// available. No bytes are sent. The function returns the number of bytes
	// to send when a DMA resource is available. They will all be sent. A
	// function allows to check if the previous DMA transfer is finished. No
	// new DMA transfer in the same direction will be allowed before the end
	// of the previous transfer.
	RDA_SPI_DMA_POLLING,

	// DMA IRQ: The application sends/receives the data through a DMA to the
	// hardware module. The function returns 0 when no DMA channel is
	// available. No bytes are sent. The function returns the number of bytes
	// to send when a DMA resource is available. They will all be sent. An
	// IRQ is generated when the current transfer is finished. No new DMA
	// transfer in the same direction will be allowed before the end of the
	// previous transfer.
	RDA_SPI_DMA_IRQ,

	// The SPI is off
	RDA_SPI_OFF,

	RDA_SPI_TM_QTY
} RDA_SPI_TRANSFERT_MODE_T;

/* **************************************************************
// RDA_SPI_IRQ_STATUS_T
// This structure is used to represent the IRQ status and mask
// of the SPI module.
   ************************************************************* */
typedef struct {
	// receive FIFO overflow irq
	unsigned int rxOvf:1;
	// transmit FIFO threshold irq
	unsigned int txTh:1;
	// transmit Dma Done irq
	unsigned int txDmaDone:1;
	// receive FIFO threshold irq
	unsigned int rxTh:1;
	// receive Dma Done irq
	unsigned int rxDmaDone:1;
} RDA_SPI_IRQ_STATUS_T;

typedef void (*RDA_SPI_IRQ_HANDLER_T) (RDA_SPI_IRQ_STATUS_T);

typedef struct {
	/*When  1, the emission commands will  enable the ability to receive data.
	   When 0, It is not possible to read received data, which are discarded.  */
	unsigned char inputEn;

	/* The delay between the CS activation and the first clock edge,
	   can be 0 to 2 half clocks. */
	RDA_SPI_DELAY_T clkDelay;

	/* The delay between the CS activation and the output of the data,
	   can be 0 to 2 half clocks. */
	RDA_SPI_DELAY_T doDelay;

	/* The delay between the CS activation and the sampling of the input data,
	   can be 0 to 3 half clocks. */
	RDA_SPI_DELAY_T diDelay;

	/* The delay between the end of transfer and the CS deactivation, can be
	   0 to 3 half clocks.  */
	RDA_SPI_DELAY_T csDelay;

	/* The time when the CS must remain deactivated before a new transfer,
	   can be 0 to 3 half clocks. */
	RDA_SPI_DELAY_T csPulse;

	/* Frame size in bits */
	u32 frameSize;

	/* OE ratio - Value from 0 to 31 is the number of data out to transfert
	   before the SPI_DO pin switches to input. When 0m the SPI_DO pin switching
	   direction mode is not enabled.  */
	u8 oeRatio;

	/* Value for the reception FIFO above which an interrupt may be generated. */
	RDA_SPI_RX_TRIGGER_CFG_T rxTrigger;

	/* Value for the emission FIFO above which an interrupt may be generated. */
	RDA_SPI_TX_TRIGGER_CFG_T txTrigger;

	/// Reception transfer mode
	RDA_SPI_TRANSFERT_MODE_T rxMode;

	/// Emission transfer mode
	RDA_SPI_TRANSFERT_MODE_T txMode;

	/// IRQ mask for this CS
	RDA_SPI_IRQ_STATUS_T mask;

	/// IRQ handler for this CS;
	RDA_SPI_IRQ_HANDLER_T handler;

	/// Use for spi read
	u8 spi_read_bits;
} RDA_SPI_PARAMETERS;

#endif /* __ASM_ARCH_DEVICES_H */
