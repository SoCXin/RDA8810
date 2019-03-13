#ifndef _REG_CFG_H_
#define _REG_CFG_H_

#include <mach/hardware.h>
#include <mach/iomap.h>

// =============================================================================
//  MACROS
// =============================================================================
#define PROD_ID                                  (0X8810)

// ============================================================================
// BB_GPIO_MAPPING_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
/// BB GPIO Map
    BB_PIN_NONE_0                               = 0x00000000,
    BB_PIN_1                                    = 0x00000001,
    BB_PIN_2                                    = 0x00000002,
    BB_PIN_3                                    = 0x00000003,
    BB_PIN_4                                    = 0x00000004,
    BB_PIN_5                                    = 0x00000005,
    BB_PIN_UART1_RXD                            = 0x00000006,
    BB_PIN_UART2_RXD                            = 0x00000007,
    BB_PIN_UART2_TXD                            = 0x00000008,
    BB_PIN_SSD1_CLK                             = 0x00000009,
    BB_PIN_SSD1_CMD                             = 0x0000000A,
    BB_PIN_SDAT1_0                              = 0x0000000B,
    BB_PIN_SDAT1_1                              = 0x0000000C,
    BB_PIN_SDAT1_2                              = 0x0000000D,
    BB_PIN_SDAT1_3                              = 0x0000000E,
    BB_PIN_SSD2_CLK                             = 0x0000000F,
    BB_PIN_SSD2_CMD                             = 0x00000010,
    BB_PIN_SDAT2_0                              = 0x00000011,
    BB_PIN_SDAT2_1                              = 0x00000012,
    BB_PIN_SDAT2_2                              = 0x00000013,
    BB_PIN_SDAT2_3                              = 0x00000014,
    BB_PIN_SPI1_CLK                             = 0x00000015,
    BB_PIN_SPI1_CS_0                            = 0x00000016,
    BB_PIN_SPI1_DIO                             = 0x00000017,
    BB_PIN_SPI1_DI                              = 0x00000018,
    BB_PIN_SIM2_RST                             = 0x00000019,
    BB_PIN_SIM2_CLK                             = 0x0000001A,
    BB_PIN_SIM2_DIO                             = 0x0000001B,
    BB_PIN_SIM3_RST                             = 0x0000001C,
    BB_PIN_SIM3_CLK                             = 0x0000001D,
    BB_PIN_SIM3_DIO                             = 0x0000001E
} BB_GPIO_MAPPING_T;

#define BB_GPIO_NB                               (31)

// ============================================================================
// AP_GPIO_A_MAPPING_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
/// AP GPIO A Map
    AP_PIN_I2C2_SCL                             = 0x00000000,
    AP_PIN_I2C2_SDA                             = 0x00000001,
    AP_PIN_SPI2_CLK                             = 0x00000002,
    AP_PIN_SPI2_DIO                             = 0x00000003,
    AP_PIN_SPI2_DI                              = 0x00000004,
    AP_PIN_SPI2_CS_0                            = 0x00000005,
    AP_PIN_SPI2_CS_1                            = 0x00000006,
    AP_PIN_KEYIN_4                              = 0x00000007,
    AP_PIN_CLK_OUT                              = 0x00000008,
    AP_PIN_I2S_BCK                              = 0x00000009,
    AP_PIN_I2S_LRCK                             = 0x0000000A,
    AP_PIN_I2S_DI_0                             = 0x0000000B,
    AP_PIN_I2S_DI_1                             = 0x0000000C,
    AP_PIN_I2S_DO                               = 0x0000000D,
    AP_PIN_UART1_TXD                            = 0x0000000E,
    AP_PIN_UART1_CTS                            = 0x0000000F,
    AP_PIN_UART1_RTS                            = 0x00000010,
    AP_PIN_SPI1_CS_1                            = 0x00000011,
    AP_PIN_LCD_DATA_6                           = 0x00000012,
    AP_PIN_LCD_DATA_7                           = 0x00000013,
    AP_PIN_LCD_WR                               = 0x00000014,
    AP_PIN_LCD_RS                               = 0x00000015,
    AP_PIN_LCD_RD                               = 0x00000016,
    AP_PIN_LCD_FMARK                            = 0x00000017,
    AP_PIN_LCD_DATA_8                           = 0x00000018,
    AP_PIN_LCD_DATA_9                           = 0x00000019,
    AP_PIN_LCD_DATA_10                          = 0x0000001A,
    AP_PIN_LCD_DATA_11                          = 0x0000001B,
    AP_PIN_LCD_DATA_12                          = 0x0000001C,
    AP_PIN_LCD_DATA_13                          = 0x0000001D,
    AP_PIN_LCD_DATA_14                          = 0x0000001E,
    AP_PIN_LCD_DATA_15                          = 0x0000001F
} AP_GPIO_A_MAPPING_T;

#define AP_GPIO_A_NB                             (32)

// ============================================================================
// AP_GPIO_B_MAPPING_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
/// AP GPIO B Map
    AP_PIN_KEYIN_0                              = 0x00000000,
    AP_PIN_KEYIN_1                              = 0x00000001,
    AP_PIN_KEYIN_2                              = 0x00000002,
    AP_PIN_KEYOUT_0                             = 0x00000003,
    AP_PIN_KEYOUT_1                             = 0x00000004,
    AP_PIN_KEYOUT_2                             = 0x00000005,
    AP_PIN_I2C3_SCL                             = 0x00000006,
    AP_PIN_I2C3_SDA                             = 0x00000007,
    AP_PIN_UART2_CTS                            = 0x00000008,
    AP_PIN_UART2_RTS                            = 0x00000009,
    AP_PIN_CAM_RST                              = 0x0000000A,
    AP_PIN_CAM_PDN                              = 0x0000000B,
    AP_PIN_CAM_CLK                              = 0x0000000C,
    AP_PIN_CAM_VSYNC                            = 0x0000000D,
    AP_PIN_CAM_HREF                             = 0x0000000E,
    AP_PIN_CAM_PCLK                             = 0x0000000F,
    AP_PIN_CAM_DATA_0                           = 0x00000010,
    AP_PIN_CAM_DATA_1                           = 0x00000011,
    AP_PIN_CAM_DATA_2                           = 0x00000012,
    AP_PIN_CAM_DATA_3                           = 0x00000013,
    AP_PIN_CAM_DATA_4                           = 0x00000014,
    AP_PIN_CAM_DATA_5                           = 0x00000015,
    AP_PIN_CAM_DATA_6                           = 0x00000016,
    AP_PIN_CAM_DATA_7                           = 0x00000017,
    AP_PIN_M_SPI_CS_0                           = 0x00000018,
    AP_PIN_NFCLE                                = 0x00000019,
    AP_PIN_NFWEN                                = 0x0000001A,
    AP_PIN_NFWPN                                = 0x0000001B,
    AP_PIN_NFREN                                = 0x0000001C,
    AP_PIN_NFRB                                 = 0x0000001D,
    AP_PIN_I2C1_SCL                             = 0x0000001E,
    AP_PIN_I2C1_SDA                             = 0x0000001F
} AP_GPIO_B_MAPPING_T;

#define AP_GPIO_B_NB                             (32)

// ============================================================================
// AP_GPIO_D_MAPPING_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
/// AP GPIO D Map
    AP_PIN_UART3_RXD                            = 0x00000000,
    AP_PIN_UART3_TXD                            = 0x00000001,
    AP_PIN_UART3_CTS                            = 0x00000002,
    AP_PIN_UART3_RTS                            = 0x00000003,
    AP_PIN_NFDQS                                = 0x00000004
} AP_GPIO_D_MAPPING_T;

#define AP_GPIO_D_NB                             (5)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// CFG_REGS_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
//#define REG_CONFIG_REGS_BASE        0x01A09000

typedef volatile struct
{
    REG32                          CHIP_ID;                      //0x00000000
    /// This register contain the synthesis date and version
    REG32                          Build_Version;                //0x00000004
    /// Setting bit n to '1' selects GPIO Usage for PAD connected to GPIOn. Setting
    /// bit n to '0' selects Alt.
    REG32                          BB_GPIO_Mode;                 //0x00000008
    /// Setting bit n to '1' selects GPIO Usage for PAD connected to GPIOn. Setting
    /// bit n to '0' selects Alt.
    REG32                          AP_GPIO_A_Mode;               //0x0000000C
    /// Setting bit n to '1' selects GPIO Usage for PAD connected to GPIOn. Setting
    /// bit n to '0' selects Alt.
    REG32                          AP_GPIO_B_Mode;               //0x00000010
    /// Setting bit n to '1' selects GPIO Usage for PAD connected to GPIOn. Setting
    /// bit n to '0' selects Alt.
    REG32                          AP_GPIO_D_Mode;               //0x00000014
    REG32                          Alt_mux_select;               //0x00000018
    REG32                          IO_Drive1_Select;             //0x0000001C
    REG32                          IO_Drive2_Select;             //0x00000020
    REG32                          RAM_DRIVE;                    //0x00000024
    REG32                          H2X_AP_Offset;                //0x00000028
    REG32                          H2X_DDR_Offset;               //0x0000002C
    REG32                          audio_pd_set;                 //0x00000030
    REG32                          audio_pd_clr;                 //0x00000034
    REG32                          audio_sel_cfg;                //0x00000038
    REG32                          audio_mic_cfg;                //0x0000003C
    REG32                          audio_spk_cfg;                //0x00000040
    REG32                          audio_rcv_gain;               //0x00000044
    REG32                          audio_head_gain;              //0x00000048
    REG32                          TSC_DATA;                     //0x0000004C
    REG32                          GPADC_DATA_CH[8];             //0x00000050
} HWP_CFG_REGS_T;

//#define hwp_configRegs              ((HWP_CFG_REGS_T*)(RDA_CFG_REGS_BASE))


//CHIP_ID
#define CFG_REGS_METAL_ID(n)        (((n)&0xFFF)<<0)
#define CFG_REGS_METAL_ID_MASK      (0xFFF<<0)
#define CFG_REGS_METAL_ID_SHIFT     (0)
#define CFG_REGS_BOND_ID(n)         (((n)&7)<<12)
#define CFG_REGS_BOND_ID_MASK       (7<<12)
#define CFG_REGS_BOND_ID_SHIFT      (12)
#define CFG_REGS_PROD_ID(n)         (((n)&0xFFFF)<<16)
#define CFG_REGS_PROD_ID_MASK       (0xFFFF<<16)
#define CFG_REGS_PROD_ID_SHIFT      (16)

//Build_Version
#define CFG_REGS_MAJOR(n)           (((n)&15)<<28)
#define CFG_REGS_YEAR(n)            (((n)&15)<<24)
#define CFG_REGS_MONTH(n)           (((n)&0xFF)<<16)
#define CFG_REGS_DAY(n)             (((n)&0xFF)<<8)
#define CFG_REGS_BUILD_STYLE_FPGA   (0<<4)
#define CFG_REGS_BUILD_STYLE_CHIP   (1<<4)
#define CFG_REGS_BUILD_STYLE_FPGA_USB (2<<4)
#define CFG_REGS_BUILD_STYLE_FPGA_GSM (3<<4)
#define CFG_REGS_BUILD_REVISION(n)  (((n)&15)<<0)

//BB_GPIO_Mode
#define CFG_REGS_MODE_BB_PIN_NONE_0 (1<<0)
#define CFG_REGS_MODE_BB_PIN_NONE_0_MASK (1<<0)
#define CFG_REGS_MODE_BB_PIN_NONE_0_SHIFT (0)
#define CFG_REGS_MODE_BB_PIN_NONE_0_ALT (0<<0)
#define CFG_REGS_MODE_BB_PIN_NONE_0_GPIO (1<<0)
#define CFG_REGS_MODE_BB_PIN_1      (1<<1)
#define CFG_REGS_MODE_BB_PIN_1_MASK (1<<1)
#define CFG_REGS_MODE_BB_PIN_1_SHIFT (1)
#define CFG_REGS_MODE_BB_PIN_1_ALT  (0<<1)
#define CFG_REGS_MODE_BB_PIN_1_GPIO (1<<1)
#define CFG_REGS_MODE_BB_PIN_2      (1<<2)
#define CFG_REGS_MODE_BB_PIN_2_MASK (1<<2)
#define CFG_REGS_MODE_BB_PIN_2_SHIFT (2)
#define CFG_REGS_MODE_BB_PIN_2_ALT  (0<<2)
#define CFG_REGS_MODE_BB_PIN_2_GPIO (1<<2)
#define CFG_REGS_MODE_BB_PIN_3      (1<<3)
#define CFG_REGS_MODE_BB_PIN_3_MASK (1<<3)
#define CFG_REGS_MODE_BB_PIN_3_SHIFT (3)
#define CFG_REGS_MODE_BB_PIN_3_ALT  (0<<3)
#define CFG_REGS_MODE_BB_PIN_3_GPIO (1<<3)
#define CFG_REGS_MODE_BB_PIN_4      (1<<4)
#define CFG_REGS_MODE_BB_PIN_4_MASK (1<<4)
#define CFG_REGS_MODE_BB_PIN_4_SHIFT (4)
#define CFG_REGS_MODE_BB_PIN_4_ALT  (0<<4)
#define CFG_REGS_MODE_BB_PIN_4_GPIO (1<<4)
#define CFG_REGS_MODE_BB_PIN_5      (1<<5)
#define CFG_REGS_MODE_BB_PIN_5_MASK (1<<5)
#define CFG_REGS_MODE_BB_PIN_5_SHIFT (5)
#define CFG_REGS_MODE_BB_PIN_5_ALT  (0<<5)
#define CFG_REGS_MODE_BB_PIN_5_GPIO (1<<5)
#define CFG_REGS_MODE_BB_PIN_UART1_RXD (1<<6)
#define CFG_REGS_MODE_BB_PIN_UART1_RXD_MASK (1<<6)
#define CFG_REGS_MODE_BB_PIN_UART1_RXD_SHIFT (6)
#define CFG_REGS_MODE_BB_PIN_UART1_RXD_ALT (0<<6)
#define CFG_REGS_MODE_BB_PIN_UART1_RXD_GPIO (1<<6)
#define CFG_REGS_MODE_BB_PIN_UART2_RXD (1<<7)
#define CFG_REGS_MODE_BB_PIN_UART2_RXD_MASK (1<<7)
#define CFG_REGS_MODE_BB_PIN_UART2_RXD_SHIFT (7)
#define CFG_REGS_MODE_BB_PIN_UART2_RXD_ALT (0<<7)
#define CFG_REGS_MODE_BB_PIN_UART2_RXD_GPIO (1<<7)
#define CFG_REGS_MODE_BB_PIN_UART2_TXD (1<<8)
#define CFG_REGS_MODE_BB_PIN_UART2_TXD_MASK (1<<8)
#define CFG_REGS_MODE_BB_PIN_UART2_TXD_SHIFT (8)
#define CFG_REGS_MODE_BB_PIN_UART2_TXD_ALT (0<<8)
#define CFG_REGS_MODE_BB_PIN_UART2_TXD_GPIO (1<<8)
#define CFG_REGS_MODE_BB_PIN_SSD1_CLK (1<<9)
#define CFG_REGS_MODE_BB_PIN_SSD1_CLK_MASK (1<<9)
#define CFG_REGS_MODE_BB_PIN_SSD1_CLK_SHIFT (9)
#define CFG_REGS_MODE_BB_PIN_SSD1_CLK_ALT (0<<9)
#define CFG_REGS_MODE_BB_PIN_SSD1_CLK_GPIO (1<<9)
#define CFG_REGS_MODE_BB_PIN_SSD1_CMD (1<<10)
#define CFG_REGS_MODE_BB_PIN_SSD1_CMD_MASK (1<<10)
#define CFG_REGS_MODE_BB_PIN_SSD1_CMD_SHIFT (10)
#define CFG_REGS_MODE_BB_PIN_SSD1_CMD_ALT (0<<10)
#define CFG_REGS_MODE_BB_PIN_SSD1_CMD_GPIO (1<<10)
#define CFG_REGS_MODE_BB_PIN_SDAT1_0 (1<<11)
#define CFG_REGS_MODE_BB_PIN_SDAT1_0_MASK (1<<11)
#define CFG_REGS_MODE_BB_PIN_SDAT1_0_SHIFT (11)
#define CFG_REGS_MODE_BB_PIN_SDAT1_0_ALT (0<<11)
#define CFG_REGS_MODE_BB_PIN_SDAT1_0_GPIO (1<<11)
#define CFG_REGS_MODE_BB_PIN_SDAT1_1 (1<<12)
#define CFG_REGS_MODE_BB_PIN_SDAT1_1_MASK (1<<12)
#define CFG_REGS_MODE_BB_PIN_SDAT1_1_SHIFT (12)
#define CFG_REGS_MODE_BB_PIN_SDAT1_1_ALT (0<<12)
#define CFG_REGS_MODE_BB_PIN_SDAT1_1_GPIO (1<<12)
#define CFG_REGS_MODE_BB_PIN_SDAT1_2 (1<<13)
#define CFG_REGS_MODE_BB_PIN_SDAT1_2_MASK (1<<13)
#define CFG_REGS_MODE_BB_PIN_SDAT1_2_SHIFT (13)
#define CFG_REGS_MODE_BB_PIN_SDAT1_2_ALT (0<<13)
#define CFG_REGS_MODE_BB_PIN_SDAT1_2_GPIO (1<<13)
#define CFG_REGS_MODE_BB_PIN_SDAT1_3 (1<<14)
#define CFG_REGS_MODE_BB_PIN_SDAT1_3_MASK (1<<14)
#define CFG_REGS_MODE_BB_PIN_SDAT1_3_SHIFT (14)
#define CFG_REGS_MODE_BB_PIN_SDAT1_3_ALT (0<<14)
#define CFG_REGS_MODE_BB_PIN_SDAT1_3_GPIO (1<<14)
#define CFG_REGS_MODE_BB_PIN_SSD2_CLK (1<<15)
#define CFG_REGS_MODE_BB_PIN_SSD2_CLK_MASK (1<<15)
#define CFG_REGS_MODE_BB_PIN_SSD2_CLK_SHIFT (15)
#define CFG_REGS_MODE_BB_PIN_SSD2_CLK_ALT (0<<15)
#define CFG_REGS_MODE_BB_PIN_SSD2_CLK_GPIO (1<<15)
#define CFG_REGS_MODE_BB_PIN_SSD2_CMD (1<<16)
#define CFG_REGS_MODE_BB_PIN_SSD2_CMD_MASK (1<<16)
#define CFG_REGS_MODE_BB_PIN_SSD2_CMD_SHIFT (16)
#define CFG_REGS_MODE_BB_PIN_SSD2_CMD_ALT (0<<16)
#define CFG_REGS_MODE_BB_PIN_SSD2_CMD_GPIO (1<<16)
#define CFG_REGS_MODE_BB_PIN_SDAT2_0 (1<<17)
#define CFG_REGS_MODE_BB_PIN_SDAT2_0_MASK (1<<17)
#define CFG_REGS_MODE_BB_PIN_SDAT2_0_SHIFT (17)
#define CFG_REGS_MODE_BB_PIN_SDAT2_0_ALT (0<<17)
#define CFG_REGS_MODE_BB_PIN_SDAT2_0_GPIO (1<<17)
#define CFG_REGS_MODE_BB_PIN_SDAT2_1 (1<<18)
#define CFG_REGS_MODE_BB_PIN_SDAT2_1_MASK (1<<18)
#define CFG_REGS_MODE_BB_PIN_SDAT2_1_SHIFT (18)
#define CFG_REGS_MODE_BB_PIN_SDAT2_1_ALT (0<<18)
#define CFG_REGS_MODE_BB_PIN_SDAT2_1_GPIO (1<<18)
#define CFG_REGS_MODE_BB_PIN_SDAT2_2 (1<<19)
#define CFG_REGS_MODE_BB_PIN_SDAT2_2_MASK (1<<19)
#define CFG_REGS_MODE_BB_PIN_SDAT2_2_SHIFT (19)
#define CFG_REGS_MODE_BB_PIN_SDAT2_2_ALT (0<<19)
#define CFG_REGS_MODE_BB_PIN_SDAT2_2_GPIO (1<<19)
#define CFG_REGS_MODE_BB_PIN_SDAT2_3 (1<<20)
#define CFG_REGS_MODE_BB_PIN_SDAT2_3_MASK (1<<20)
#define CFG_REGS_MODE_BB_PIN_SDAT2_3_SHIFT (20)
#define CFG_REGS_MODE_BB_PIN_SDAT2_3_ALT (0<<20)
#define CFG_REGS_MODE_BB_PIN_SDAT2_3_GPIO (1<<20)
#define CFG_REGS_MODE_BB_PIN_SPI1_CLK (1<<21)
#define CFG_REGS_MODE_BB_PIN_SPI1_CLK_MASK (1<<21)
#define CFG_REGS_MODE_BB_PIN_SPI1_CLK_SHIFT (21)
#define CFG_REGS_MODE_BB_PIN_SPI1_CLK_ALT (0<<21)
#define CFG_REGS_MODE_BB_PIN_SPI1_CLK_GPIO (1<<21)
#define CFG_REGS_MODE_BB_PIN_SPI1_CS_0 (1<<22)
#define CFG_REGS_MODE_BB_PIN_SPI1_CS_0_MASK (1<<22)
#define CFG_REGS_MODE_BB_PIN_SPI1_CS_0_SHIFT (22)
#define CFG_REGS_MODE_BB_PIN_SPI1_CS_0_ALT (0<<22)
#define CFG_REGS_MODE_BB_PIN_SPI1_CS_0_GPIO (1<<22)
#define CFG_REGS_MODE_BB_PIN_SPI1_DIO (1<<23)
#define CFG_REGS_MODE_BB_PIN_SPI1_DIO_MASK (1<<23)
#define CFG_REGS_MODE_BB_PIN_SPI1_DIO_SHIFT (23)
#define CFG_REGS_MODE_BB_PIN_SPI1_DIO_ALT (0<<23)
#define CFG_REGS_MODE_BB_PIN_SPI1_DIO_GPIO (1<<23)
#define CFG_REGS_MODE_BB_PIN_SPI1_DI (1<<24)
#define CFG_REGS_MODE_BB_PIN_SPI1_DI_MASK (1<<24)
#define CFG_REGS_MODE_BB_PIN_SPI1_DI_SHIFT (24)
#define CFG_REGS_MODE_BB_PIN_SPI1_DI_ALT (0<<24)
#define CFG_REGS_MODE_BB_PIN_SPI1_DI_GPIO (1<<24)
#define CFG_REGS_MODE_BB_PIN_SIM2_RST (1<<25)
#define CFG_REGS_MODE_BB_PIN_SIM2_RST_MASK (1<<25)
#define CFG_REGS_MODE_BB_PIN_SIM2_RST_SHIFT (25)
#define CFG_REGS_MODE_BB_PIN_SIM2_RST_ALT (0<<25)
#define CFG_REGS_MODE_BB_PIN_SIM2_RST_GPIO (1<<25)
#define CFG_REGS_MODE_BB_PIN_SIM2_CLK (1<<26)
#define CFG_REGS_MODE_BB_PIN_SIM2_CLK_MASK (1<<26)
#define CFG_REGS_MODE_BB_PIN_SIM2_CLK_SHIFT (26)
#define CFG_REGS_MODE_BB_PIN_SIM2_CLK_ALT (0<<26)
#define CFG_REGS_MODE_BB_PIN_SIM2_CLK_GPIO (1<<26)
#define CFG_REGS_MODE_BB_PIN_SIM2_DIO (1<<27)
#define CFG_REGS_MODE_BB_PIN_SIM2_DIO_MASK (1<<27)
#define CFG_REGS_MODE_BB_PIN_SIM2_DIO_SHIFT (27)
#define CFG_REGS_MODE_BB_PIN_SIM2_DIO_ALT (0<<27)
#define CFG_REGS_MODE_BB_PIN_SIM2_DIO_GPIO (1<<27)
#define CFG_REGS_MODE_BB_PIN_SIM3_RST (1<<28)
#define CFG_REGS_MODE_BB_PIN_SIM3_RST_MASK (1<<28)
#define CFG_REGS_MODE_BB_PIN_SIM3_RST_SHIFT (28)
#define CFG_REGS_MODE_BB_PIN_SIM3_RST_ALT (0<<28)
#define CFG_REGS_MODE_BB_PIN_SIM3_RST_GPIO (1<<28)
#define CFG_REGS_MODE_BB_PIN_SIM3_CLK (1<<29)
#define CFG_REGS_MODE_BB_PIN_SIM3_CLK_MASK (1<<29)
#define CFG_REGS_MODE_BB_PIN_SIM3_CLK_SHIFT (29)
#define CFG_REGS_MODE_BB_PIN_SIM3_CLK_ALT (0<<29)
#define CFG_REGS_MODE_BB_PIN_SIM3_CLK_GPIO (1<<29)
#define CFG_REGS_MODE_BB_PIN_SIM3_DIO (1<<30)
#define CFG_REGS_MODE_BB_PIN_SIM3_DIO_MASK (1<<30)
#define CFG_REGS_MODE_BB_PIN_SIM3_DIO_SHIFT (30)
#define CFG_REGS_MODE_BB_PIN_SIM3_DIO_ALT (0<<30)
#define CFG_REGS_MODE_BB_PIN_SIM3_DIO_GPIO (1<<30)
#define CFG_REGS_BB_GPIO_MODE(n)    (((n)&0x7FFFFFFF)<<0)
#define CFG_REGS_BB_GPIO_MODE_MASK  (0x7FFFFFFF<<0)
#define CFG_REGS_BB_GPIO_MODE_SHIFT (0)

//AP_GPIO_A_Mode
#define CFG_REGS_MODE_AP_PIN_I2C2_SCL (1<<0)
#define CFG_REGS_MODE_AP_PIN_I2C2_SCL_MASK (1<<0)
#define CFG_REGS_MODE_AP_PIN_I2C2_SCL_SHIFT (0)
#define CFG_REGS_MODE_AP_PIN_I2C2_SCL_ALT (0<<0)
#define CFG_REGS_MODE_AP_PIN_I2C2_SCL_GPIO (1<<0)
#define CFG_REGS_MODE_AP_PIN_I2C2_SDA (1<<1)
#define CFG_REGS_MODE_AP_PIN_I2C2_SDA_MASK (1<<1)
#define CFG_REGS_MODE_AP_PIN_I2C2_SDA_SHIFT (1)
#define CFG_REGS_MODE_AP_PIN_I2C2_SDA_ALT (0<<1)
#define CFG_REGS_MODE_AP_PIN_I2C2_SDA_GPIO (1<<1)
#define CFG_REGS_MODE_AP_PIN_SPI2_CLK (1<<2)
#define CFG_REGS_MODE_AP_PIN_SPI2_CLK_MASK (1<<2)
#define CFG_REGS_MODE_AP_PIN_SPI2_CLK_SHIFT (2)
#define CFG_REGS_MODE_AP_PIN_SPI2_CLK_ALT (0<<2)
#define CFG_REGS_MODE_AP_PIN_SPI2_CLK_GPIO (1<<2)
#define CFG_REGS_MODE_AP_PIN_SPI2_DIO (1<<3)
#define CFG_REGS_MODE_AP_PIN_SPI2_DIO_MASK (1<<3)
#define CFG_REGS_MODE_AP_PIN_SPI2_DIO_SHIFT (3)
#define CFG_REGS_MODE_AP_PIN_SPI2_DIO_ALT (0<<3)
#define CFG_REGS_MODE_AP_PIN_SPI2_DIO_GPIO (1<<3)
#define CFG_REGS_MODE_AP_PIN_SPI2_DI (1<<4)
#define CFG_REGS_MODE_AP_PIN_SPI2_DI_MASK (1<<4)
#define CFG_REGS_MODE_AP_PIN_SPI2_DI_SHIFT (4)
#define CFG_REGS_MODE_AP_PIN_SPI2_DI_ALT (0<<4)
#define CFG_REGS_MODE_AP_PIN_SPI2_DI_GPIO (1<<4)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_0 (1<<5)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_0_MASK (1<<5)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_0_SHIFT (5)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_0_ALT (0<<5)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_0_GPIO (1<<5)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_1 (1<<6)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_1_MASK (1<<6)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_1_SHIFT (6)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_1_ALT (0<<6)
#define CFG_REGS_MODE_AP_PIN_SPI2_CS_1_GPIO (1<<6)
#define CFG_REGS_MODE_AP_PIN_KEYIN_4 (1<<7)
#define CFG_REGS_MODE_AP_PIN_KEYIN_4_MASK (1<<7)
#define CFG_REGS_MODE_AP_PIN_KEYIN_4_SHIFT (7)
#define CFG_REGS_MODE_AP_PIN_KEYIN_4_ALT (0<<7)
#define CFG_REGS_MODE_AP_PIN_KEYIN_4_GPIO (1<<7)
#define CFG_REGS_MODE_AP_PIN_CLK_OUT (1<<8)
#define CFG_REGS_MODE_AP_PIN_CLK_OUT_MASK (1<<8)
#define CFG_REGS_MODE_AP_PIN_CLK_OUT_SHIFT (8)
#define CFG_REGS_MODE_AP_PIN_CLK_OUT_ALT (0<<8)
#define CFG_REGS_MODE_AP_PIN_CLK_OUT_GPIO (1<<8)
#define CFG_REGS_MODE_AP_PIN_I2S_BCK (1<<9)
#define CFG_REGS_MODE_AP_PIN_I2S_BCK_MASK (1<<9)
#define CFG_REGS_MODE_AP_PIN_I2S_BCK_SHIFT (9)
#define CFG_REGS_MODE_AP_PIN_I2S_BCK_ALT (0<<9)
#define CFG_REGS_MODE_AP_PIN_I2S_BCK_GPIO (1<<9)
#define CFG_REGS_MODE_AP_PIN_I2S_LRCK (1<<10)
#define CFG_REGS_MODE_AP_PIN_I2S_LRCK_MASK (1<<10)
#define CFG_REGS_MODE_AP_PIN_I2S_LRCK_SHIFT (10)
#define CFG_REGS_MODE_AP_PIN_I2S_LRCK_ALT (0<<10)
#define CFG_REGS_MODE_AP_PIN_I2S_LRCK_GPIO (1<<10)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_0 (1<<11)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_0_MASK (1<<11)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_0_SHIFT (11)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_0_ALT (0<<11)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_0_GPIO (1<<11)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_1 (1<<12)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_1_MASK (1<<12)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_1_SHIFT (12)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_1_ALT (0<<12)
#define CFG_REGS_MODE_AP_PIN_I2S_DI_1_GPIO (1<<12)
#define CFG_REGS_MODE_AP_PIN_I2S_DO (1<<13)
#define CFG_REGS_MODE_AP_PIN_I2S_DO_MASK (1<<13)
#define CFG_REGS_MODE_AP_PIN_I2S_DO_SHIFT (13)
#define CFG_REGS_MODE_AP_PIN_I2S_DO_ALT (0<<13)
#define CFG_REGS_MODE_AP_PIN_I2S_DO_GPIO (1<<13)
#define CFG_REGS_MODE_AP_PIN_UART1_TXD (1<<14)
#define CFG_REGS_MODE_AP_PIN_UART1_TXD_MASK (1<<14)
#define CFG_REGS_MODE_AP_PIN_UART1_TXD_SHIFT (14)
#define CFG_REGS_MODE_AP_PIN_UART1_TXD_ALT (0<<14)
#define CFG_REGS_MODE_AP_PIN_UART1_TXD_GPIO (1<<14)
#define CFG_REGS_MODE_AP_PIN_UART1_CTS (1<<15)
#define CFG_REGS_MODE_AP_PIN_UART1_CTS_MASK (1<<15)
#define CFG_REGS_MODE_AP_PIN_UART1_CTS_SHIFT (15)
#define CFG_REGS_MODE_AP_PIN_UART1_CTS_ALT (0<<15)
#define CFG_REGS_MODE_AP_PIN_UART1_CTS_GPIO (1<<15)
#define CFG_REGS_MODE_AP_PIN_UART1_RTS (1<<16)
#define CFG_REGS_MODE_AP_PIN_UART1_RTS_MASK (1<<16)
#define CFG_REGS_MODE_AP_PIN_UART1_RTS_SHIFT (16)
#define CFG_REGS_MODE_AP_PIN_UART1_RTS_ALT (0<<16)
#define CFG_REGS_MODE_AP_PIN_UART1_RTS_GPIO (1<<16)
#define CFG_REGS_MODE_AP_PIN_SPI1_CS_1 (1<<17)
#define CFG_REGS_MODE_AP_PIN_SPI1_CS_1_MASK (1<<17)
#define CFG_REGS_MODE_AP_PIN_SPI1_CS_1_SHIFT (17)
#define CFG_REGS_MODE_AP_PIN_SPI1_CS_1_ALT (0<<17)
#define CFG_REGS_MODE_AP_PIN_SPI1_CS_1_GPIO (1<<17)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_6 (1<<18)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_6_MASK (1<<18)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_6_SHIFT (18)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_6_ALT (0<<18)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_6_GPIO (1<<18)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_7 (1<<19)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_7_MASK (1<<19)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_7_SHIFT (19)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_7_ALT (0<<19)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_7_GPIO (1<<19)
#define CFG_REGS_MODE_AP_PIN_LCD_WR (1<<20)
#define CFG_REGS_MODE_AP_PIN_LCD_WR_MASK (1<<20)
#define CFG_REGS_MODE_AP_PIN_LCD_WR_SHIFT (20)
#define CFG_REGS_MODE_AP_PIN_LCD_WR_ALT (0<<20)
#define CFG_REGS_MODE_AP_PIN_LCD_WR_GPIO (1<<20)
#define CFG_REGS_MODE_AP_PIN_LCD_RS (1<<21)
#define CFG_REGS_MODE_AP_PIN_LCD_RS_MASK (1<<21)
#define CFG_REGS_MODE_AP_PIN_LCD_RS_SHIFT (21)
#define CFG_REGS_MODE_AP_PIN_LCD_RS_ALT (0<<21)
#define CFG_REGS_MODE_AP_PIN_LCD_RS_GPIO (1<<21)
#define CFG_REGS_MODE_AP_PIN_LCD_RD (1<<22)
#define CFG_REGS_MODE_AP_PIN_LCD_RD_MASK (1<<22)
#define CFG_REGS_MODE_AP_PIN_LCD_RD_SHIFT (22)
#define CFG_REGS_MODE_AP_PIN_LCD_RD_ALT (0<<22)
#define CFG_REGS_MODE_AP_PIN_LCD_RD_GPIO (1<<22)
#define CFG_REGS_MODE_AP_PIN_LCD_FMARK (1<<23)
#define CFG_REGS_MODE_AP_PIN_LCD_FMARK_MASK (1<<23)
#define CFG_REGS_MODE_AP_PIN_LCD_FMARK_SHIFT (23)
#define CFG_REGS_MODE_AP_PIN_LCD_FMARK_ALT (0<<23)
#define CFG_REGS_MODE_AP_PIN_LCD_FMARK_GPIO (1<<23)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_8 (1<<24)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_8_MASK (1<<24)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_8_SHIFT (24)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_8_ALT (0<<24)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_8_GPIO (1<<24)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_9 (1<<25)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_9_MASK (1<<25)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_9_SHIFT (25)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_9_ALT (0<<25)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_9_GPIO (1<<25)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_10 (1<<26)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_10_MASK (1<<26)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_10_SHIFT (26)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_10_ALT (0<<26)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_10_GPIO (1<<26)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_11 (1<<27)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_11_MASK (1<<27)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_11_SHIFT (27)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_11_ALT (0<<27)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_11_GPIO (1<<27)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_12 (1<<28)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_12_MASK (1<<28)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_12_SHIFT (28)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_12_ALT (0<<28)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_12_GPIO (1<<28)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_13 (1<<29)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_13_MASK (1<<29)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_13_SHIFT (29)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_13_ALT (0<<29)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_13_GPIO (1<<29)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_14 (1<<30)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_14_MASK (1<<30)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_14_SHIFT (30)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_14_ALT (0<<30)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_14_GPIO (1<<30)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_15 (1<<31)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_15_MASK (1<<31)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_15_SHIFT (31)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_15_ALT (0<<31)
#define CFG_REGS_MODE_AP_PIN_LCD_DATA_15_GPIO (1<<31)
#define CFG_REGS_AP_GPIO_A_MODE(n)  (((n)&0xFFFFFFFF)<<0)
#define CFG_REGS_AP_GPIO_A_MODE_MASK (0xFFFFFFFF<<0)
#define CFG_REGS_AP_GPIO_A_MODE_SHIFT (0)

//AP_GPIO_B_Mode
#define CFG_REGS_MODE_AP_PIN_KEYIN_0 (1<<0)
#define CFG_REGS_MODE_AP_PIN_KEYIN_0_MASK (1<<0)
#define CFG_REGS_MODE_AP_PIN_KEYIN_0_SHIFT (0)
#define CFG_REGS_MODE_AP_PIN_KEYIN_0_ALT (0<<0)
#define CFG_REGS_MODE_AP_PIN_KEYIN_0_GPIO (1<<0)
#define CFG_REGS_MODE_AP_PIN_KEYIN_1 (1<<1)
#define CFG_REGS_MODE_AP_PIN_KEYIN_1_MASK (1<<1)
#define CFG_REGS_MODE_AP_PIN_KEYIN_1_SHIFT (1)
#define CFG_REGS_MODE_AP_PIN_KEYIN_1_ALT (0<<1)
#define CFG_REGS_MODE_AP_PIN_KEYIN_1_GPIO (1<<1)
#define CFG_REGS_MODE_AP_PIN_KEYIN_2 (1<<2)
#define CFG_REGS_MODE_AP_PIN_KEYIN_2_MASK (1<<2)
#define CFG_REGS_MODE_AP_PIN_KEYIN_2_SHIFT (2)
#define CFG_REGS_MODE_AP_PIN_KEYIN_2_ALT (0<<2)
#define CFG_REGS_MODE_AP_PIN_KEYIN_2_GPIO (1<<2)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_0 (1<<3)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_0_MASK (1<<3)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_0_SHIFT (3)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_0_ALT (0<<3)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_0_GPIO (1<<3)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_1 (1<<4)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_1_MASK (1<<4)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_1_SHIFT (4)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_1_ALT (0<<4)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_1_GPIO (1<<4)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_2 (1<<5)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_2_MASK (1<<5)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_2_SHIFT (5)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_2_ALT (0<<5)
#define CFG_REGS_MODE_AP_PIN_KEYOUT_2_GPIO (1<<5)
#define CFG_REGS_MODE_AP_PIN_I2C3_SCL (1<<6)
#define CFG_REGS_MODE_AP_PIN_I2C3_SCL_MASK (1<<6)
#define CFG_REGS_MODE_AP_PIN_I2C3_SCL_SHIFT (6)
#define CFG_REGS_MODE_AP_PIN_I2C3_SCL_ALT (0<<6)
#define CFG_REGS_MODE_AP_PIN_I2C3_SCL_GPIO (1<<6)
#define CFG_REGS_MODE_AP_PIN_I2C3_SDA (1<<7)
#define CFG_REGS_MODE_AP_PIN_I2C3_SDA_MASK (1<<7)
#define CFG_REGS_MODE_AP_PIN_I2C3_SDA_SHIFT (7)
#define CFG_REGS_MODE_AP_PIN_I2C3_SDA_ALT (0<<7)
#define CFG_REGS_MODE_AP_PIN_I2C3_SDA_GPIO (1<<7)
#define CFG_REGS_MODE_AP_PIN_UART2_CTS (1<<8)
#define CFG_REGS_MODE_AP_PIN_UART2_CTS_MASK (1<<8)
#define CFG_REGS_MODE_AP_PIN_UART2_CTS_SHIFT (8)
#define CFG_REGS_MODE_AP_PIN_UART2_CTS_ALT (0<<8)
#define CFG_REGS_MODE_AP_PIN_UART2_CTS_GPIO (1<<8)
#define CFG_REGS_MODE_AP_PIN_UART2_RTS (1<<9)
#define CFG_REGS_MODE_AP_PIN_UART2_RTS_MASK (1<<9)
#define CFG_REGS_MODE_AP_PIN_UART2_RTS_SHIFT (9)
#define CFG_REGS_MODE_AP_PIN_UART2_RTS_ALT (0<<9)
#define CFG_REGS_MODE_AP_PIN_UART2_RTS_GPIO (1<<9)
#define CFG_REGS_MODE_AP_PIN_CAM_RST (1<<10)
#define CFG_REGS_MODE_AP_PIN_CAM_RST_MASK (1<<10)
#define CFG_REGS_MODE_AP_PIN_CAM_RST_SHIFT (10)
#define CFG_REGS_MODE_AP_PIN_CAM_RST_ALT (0<<10)
#define CFG_REGS_MODE_AP_PIN_CAM_RST_GPIO (1<<10)
#define CFG_REGS_MODE_AP_PIN_CAM_PDN (1<<11)
#define CFG_REGS_MODE_AP_PIN_CAM_PDN_MASK (1<<11)
#define CFG_REGS_MODE_AP_PIN_CAM_PDN_SHIFT (11)
#define CFG_REGS_MODE_AP_PIN_CAM_PDN_ALT (0<<11)
#define CFG_REGS_MODE_AP_PIN_CAM_PDN_GPIO (1<<11)
#define CFG_REGS_MODE_AP_PIN_CAM_CLK (1<<12)
#define CFG_REGS_MODE_AP_PIN_CAM_CLK_MASK (1<<12)
#define CFG_REGS_MODE_AP_PIN_CAM_CLK_SHIFT (12)
#define CFG_REGS_MODE_AP_PIN_CAM_CLK_ALT (0<<12)
#define CFG_REGS_MODE_AP_PIN_CAM_CLK_GPIO (1<<12)
#define CFG_REGS_MODE_AP_PIN_CAM_VSYNC (1<<13)
#define CFG_REGS_MODE_AP_PIN_CAM_VSYNC_MASK (1<<13)
#define CFG_REGS_MODE_AP_PIN_CAM_VSYNC_SHIFT (13)
#define CFG_REGS_MODE_AP_PIN_CAM_VSYNC_ALT (0<<13)
#define CFG_REGS_MODE_AP_PIN_CAM_VSYNC_GPIO (1<<13)
#define CFG_REGS_MODE_AP_PIN_CAM_HREF (1<<14)
#define CFG_REGS_MODE_AP_PIN_CAM_HREF_MASK (1<<14)
#define CFG_REGS_MODE_AP_PIN_CAM_HREF_SHIFT (14)
#define CFG_REGS_MODE_AP_PIN_CAM_HREF_ALT (0<<14)
#define CFG_REGS_MODE_AP_PIN_CAM_HREF_GPIO (1<<14)
#define CFG_REGS_MODE_AP_PIN_CAM_PCLK (1<<15)
#define CFG_REGS_MODE_AP_PIN_CAM_PCLK_MASK (1<<15)
#define CFG_REGS_MODE_AP_PIN_CAM_PCLK_SHIFT (15)
#define CFG_REGS_MODE_AP_PIN_CAM_PCLK_ALT (0<<15)
#define CFG_REGS_MODE_AP_PIN_CAM_PCLK_GPIO (1<<15)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_0 (1<<16)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_0_MASK (1<<16)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_0_SHIFT (16)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_0_ALT (0<<16)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_0_GPIO (1<<16)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_1 (1<<17)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_1_MASK (1<<17)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_1_SHIFT (17)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_1_ALT (0<<17)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_1_GPIO (1<<17)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_2 (1<<18)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_2_MASK (1<<18)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_2_SHIFT (18)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_2_ALT (0<<18)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_2_GPIO (1<<18)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_3 (1<<19)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_3_MASK (1<<19)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_3_SHIFT (19)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_3_ALT (0<<19)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_3_GPIO (1<<19)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_4 (1<<20)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_4_MASK (1<<20)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_4_SHIFT (20)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_4_ALT (0<<20)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_4_GPIO (1<<20)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_5 (1<<21)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_5_MASK (1<<21)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_5_SHIFT (21)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_5_ALT (0<<21)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_5_GPIO (1<<21)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_6 (1<<22)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_6_MASK (1<<22)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_6_SHIFT (22)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_6_ALT (0<<22)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_6_GPIO (1<<22)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_7 (1<<23)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_7_MASK (1<<23)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_7_SHIFT (23)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_7_ALT (0<<23)
#define CFG_REGS_MODE_AP_PIN_CAM_DATA_7_GPIO (1<<23)
#define CFG_REGS_MODE_AP_PIN_M_SPI_CS_0 (1<<24)
#define CFG_REGS_MODE_AP_PIN_M_SPI_CS_0_MASK (1<<24)
#define CFG_REGS_MODE_AP_PIN_M_SPI_CS_0_SHIFT (24)
#define CFG_REGS_MODE_AP_PIN_M_SPI_CS_0_ALT (0<<24)
#define CFG_REGS_MODE_AP_PIN_M_SPI_CS_0_GPIO (1<<24)
#define CFG_REGS_MODE_AP_PIN_NFCLE  (1<<25)
#define CFG_REGS_MODE_AP_PIN_NFCLE_MASK (1<<25)
#define CFG_REGS_MODE_AP_PIN_NFCLE_SHIFT (25)
#define CFG_REGS_MODE_AP_PIN_NFCLE_ALT (0<<25)
#define CFG_REGS_MODE_AP_PIN_NFCLE_GPIO (1<<25)
#define CFG_REGS_MODE_AP_PIN_NFWEN  (1<<26)
#define CFG_REGS_MODE_AP_PIN_NFWEN_MASK (1<<26)
#define CFG_REGS_MODE_AP_PIN_NFWEN_SHIFT (26)
#define CFG_REGS_MODE_AP_PIN_NFWEN_ALT (0<<26)
#define CFG_REGS_MODE_AP_PIN_NFWEN_GPIO (1<<26)
#define CFG_REGS_MODE_AP_PIN_NFWPN  (1<<27)
#define CFG_REGS_MODE_AP_PIN_NFWPN_MASK (1<<27)
#define CFG_REGS_MODE_AP_PIN_NFWPN_SHIFT (27)
#define CFG_REGS_MODE_AP_PIN_NFWPN_ALT (0<<27)
#define CFG_REGS_MODE_AP_PIN_NFWPN_GPIO (1<<27)
#define CFG_REGS_MODE_AP_PIN_NFREN  (1<<28)
#define CFG_REGS_MODE_AP_PIN_NFREN_MASK (1<<28)
#define CFG_REGS_MODE_AP_PIN_NFREN_SHIFT (28)
#define CFG_REGS_MODE_AP_PIN_NFREN_ALT (0<<28)
#define CFG_REGS_MODE_AP_PIN_NFREN_GPIO (1<<28)
#define CFG_REGS_MODE_AP_PIN_NFRB   (1<<29)
#define CFG_REGS_MODE_AP_PIN_NFRB_MASK (1<<29)
#define CFG_REGS_MODE_AP_PIN_NFRB_SHIFT (29)
#define CFG_REGS_MODE_AP_PIN_NFRB_ALT (0<<29)
#define CFG_REGS_MODE_AP_PIN_NFRB_GPIO (1<<29)
#define CFG_REGS_MODE_AP_PIN_I2C1_SCL (1<<30)
#define CFG_REGS_MODE_AP_PIN_I2C1_SCL_MASK (1<<30)
#define CFG_REGS_MODE_AP_PIN_I2C1_SCL_SHIFT (30)
#define CFG_REGS_MODE_AP_PIN_I2C1_SCL_ALT (0<<30)
#define CFG_REGS_MODE_AP_PIN_I2C1_SCL_GPIO (1<<30)
#define CFG_REGS_MODE_AP_PIN_I2C1_SDA (1<<31)
#define CFG_REGS_MODE_AP_PIN_I2C1_SDA_MASK (1<<31)
#define CFG_REGS_MODE_AP_PIN_I2C1_SDA_SHIFT (31)
#define CFG_REGS_MODE_AP_PIN_I2C1_SDA_ALT (0<<31)
#define CFG_REGS_MODE_AP_PIN_I2C1_SDA_GPIO (1<<31)
#define CFG_REGS_AP_GPIO_B_MODE(n)  (((n)&0xFFFFFFFF)<<0)
#define CFG_REGS_AP_GPIO_B_MODE_MASK (0xFFFFFFFF<<0)
#define CFG_REGS_AP_GPIO_B_MODE_SHIFT (0)

//AP_GPIO_D_Mode
#define CFG_REGS_MODE_AP_PIN_UART3_RXD (1<<0)
#define CFG_REGS_MODE_AP_PIN_UART3_RXD_MASK (1<<0)
#define CFG_REGS_MODE_AP_PIN_UART3_RXD_SHIFT (0)
#define CFG_REGS_MODE_AP_PIN_UART3_RXD_ALT (0<<0)
#define CFG_REGS_MODE_AP_PIN_UART3_RXD_GPIO (1<<0)
#define CFG_REGS_MODE_AP_PIN_UART3_TXD (1<<1)
#define CFG_REGS_MODE_AP_PIN_UART3_TXD_MASK (1<<1)
#define CFG_REGS_MODE_AP_PIN_UART3_TXD_SHIFT (1)
#define CFG_REGS_MODE_AP_PIN_UART3_TXD_ALT (0<<1)
#define CFG_REGS_MODE_AP_PIN_UART3_TXD_GPIO (1<<1)
#define CFG_REGS_MODE_AP_PIN_UART3_CTS (1<<2)
#define CFG_REGS_MODE_AP_PIN_UART3_CTS_MASK (1<<2)
#define CFG_REGS_MODE_AP_PIN_UART3_CTS_SHIFT (2)
#define CFG_REGS_MODE_AP_PIN_UART3_CTS_ALT (0<<2)
#define CFG_REGS_MODE_AP_PIN_UART3_CTS_GPIO (1<<2)
#define CFG_REGS_MODE_AP_PIN_UART3_RTS (1<<3)
#define CFG_REGS_MODE_AP_PIN_UART3_RTS_MASK (1<<3)
#define CFG_REGS_MODE_AP_PIN_UART3_RTS_SHIFT (3)
#define CFG_REGS_MODE_AP_PIN_UART3_RTS_ALT (0<<3)
#define CFG_REGS_MODE_AP_PIN_UART3_RTS_GPIO (1<<3)
#define CFG_REGS_MODE_AP_PIN_NFDQS  (1<<4)
#define CFG_REGS_MODE_AP_PIN_NFDQS_MASK (1<<4)
#define CFG_REGS_MODE_AP_PIN_NFDQS_SHIFT (4)
#define CFG_REGS_MODE_AP_PIN_NFDQS_ALT (0<<4)
#define CFG_REGS_MODE_AP_PIN_NFDQS_GPIO (1<<4)
#define CFG_REGS_AP_GPIO_D_MODE(n)  (((n)&31)<<0)
#define CFG_REGS_AP_GPIO_D_MODE_MASK (31<<0)
#define CFG_REGS_AP_GPIO_D_MODE_SHIFT (0)

//Alt_mux_select
#define CFG_REGS_LCD_MODE_MASK      (3<<0)
#define CFG_REGS_LCD_MODE_PARALLEL_16BIT (0<<0)
#define CFG_REGS_LCD_MODE_DSI       (1<<0)
#define CFG_REGS_LCD_MODE_RGB_24BIT (2<<0)
#define CFG_REGS_LCD_MODE_RGB_16BIT (3<<0)
#define CFG_REGS_SPI_LCD_MASK       (1<<2)
#define CFG_REGS_SPI_LCD_NONE       (0<<2)
#define CFG_REGS_SPI_LCD_SPI_LCD    (1<<2)
#define CFG_REGS_CAM_I2C2_MASK      (1<<3)
#define CFG_REGS_CAM_I2C2_CAM       (0<<3)
#define CFG_REGS_CAM_I2C2_I2C2      (1<<3)
#define CFG_REGS_CSI2_MASK          (3<<4)
#define CFG_REGS_CSI2_PARALLEL_CAM  (0<<4)
#define CFG_REGS_CSI2_CSI2          (1<<4)
#define CFG_REGS_CSI2_SPI_CAM       (2<<4)
#define CFG_REGS_UART2_MASK         (1<<6)
#define CFG_REGS_UART2_HOST_UART    (0<<6)
#define CFG_REGS_UART2_UART2        (1<<6)
#define CFG_REGS_UART1_8LINE_MASK   (1<<7)
#define CFG_REGS_UART1_8LINE_UART2  (0<<7)
#define CFG_REGS_UART1_8LINE_UART1_8_LINE (1<<7)
#define CFG_REGS_DAI_MASK           (3<<8)
#define CFG_REGS_DAI_I2S            (0<<8)
#define CFG_REGS_DAI_DAI            (1<<8)
#define CFG_REGS_DAI_DAI_SIMPLE     (2<<8)
#define CFG_REGS_KEYIN_3_MASK       (1<<10)
#define CFG_REGS_KEYIN_3_SPI2_CS_1  (0<<10)
#define CFG_REGS_KEYIN_3_KEYIN_3    (1<<10)
#define CFG_REGS_LPSCO_1_MASK       (1<<11)
#define CFG_REGS_LPSCO_1_KEYIN_4    (0<<11)
#define CFG_REGS_LPSCO_1_LPSCO_1    (1<<11)
#define CFG_REGS_SPI1_CS_2_MASK     (1<<12)
#define CFG_REGS_SPI1_CS_2_KEYIN_1  (0<<12)
#define CFG_REGS_SPI1_CS_2_SPI1_CS_2 (1<<12)
#define CFG_REGS_I2S_DI_2_MASK      (1<<13)
#define CFG_REGS_I2S_DI_2_KEYIN_2   (0<<13)
#define CFG_REGS_I2S_DI_2_I2S_DI_2  (1<<13)
#define CFG_REGS_TCO_0_MASK         (1<<14)
#define CFG_REGS_TCO_0_KEYOUT_0     (0<<14)
#define CFG_REGS_TCO_0_TCO_0        (1<<14)
#define CFG_REGS_TCO_1_MASK         (1<<15)
#define CFG_REGS_TCO_1_KEYOUT_1     (0<<15)
#define CFG_REGS_TCO_1_TCO_1        (1<<15)
#define CFG_REGS_TCO_2_MASK         (1<<16)
#define CFG_REGS_TCO_2_KEYOUT_2     (0<<16)
#define CFG_REGS_TCO_2_TCO_2        (1<<16)
#define CFG_REGS_KEYOUT_3_4_MASK    (1<<17)
#define CFG_REGS_KEYOUT_3_4_I2C3    (0<<17)
#define CFG_REGS_KEYOUT_3_4_KEYOUT_3_4 (1<<17)
#define CFG_REGS_KEYOUT_6_MASK      (1<<18)
#define CFG_REGS_KEYOUT_6_UART2_RTS (0<<18)
#define CFG_REGS_KEYOUT_6_KEYOUT_6  (1<<18)
#define CFG_REGS_KEYOUT_7_MASK      (1<<19)
#define CFG_REGS_KEYOUT_7_UART1_RTS (0<<19)
#define CFG_REGS_KEYOUT_7_KEYOUT_7  (1<<19)
#define CFG_REGS_GPO_0_MASK         (3<<20)
#define CFG_REGS_GPO_0_GPO_0        (0<<20)
#define CFG_REGS_GPO_0_PWT          (1<<20)
#define CFG_REGS_GPO_0_KEYIN_5      (2<<20)
#define CFG_REGS_GPO_1_MASK         (3<<22)
#define CFG_REGS_GPO_1_GPO_1        (0<<22)
#define CFG_REGS_GPO_1_LPG          (1<<22)
#define CFG_REGS_GPO_1_KEYOUT_5     (2<<22)
#define CFG_REGS_GPO_2_MASK         (3<<24)
#define CFG_REGS_GPO_2_GPO_2        (0<<24)
#define CFG_REGS_GPO_2_PWL_1        (1<<24)
#define CFG_REGS_GPO_2_CLK_32K      (2<<24)
#define CFG_REGS_GPO_3_MASK         (1<<26)
#define CFG_REGS_GPO_3_LCD_CS_1     (0<<26)
#define CFG_REGS_GPO_3_GPO_3        (1<<26)
#define CFG_REGS_GPO_4_MASK         (1<<27)
#define CFG_REGS_GPO_4_LCD_CS_0     (0<<27)
#define CFG_REGS_GPO_4_GPO_4        (1<<27)
#define CFG_REGS_CLK_OUT_MASK       (1<<28)
#define CFG_REGS_CLK_OUT_HST_CLK    (0<<28)
#define CFG_REGS_CLK_OUT_CLK_OUT    (1<<28)
#define CFG_REGS_DEBUG_MASK         (1<<29)
#define CFG_REGS_DEBUG_NONE         (0<<29)
#define CFG_REGS_DEBUG_DEBUG_MONITOR (1<<29)
#define CFG_REGS_AP_SPI1_MASK       (1<<30)
#define CFG_REGS_AP_SPI1_BB_SPI1    (0<<30)
#define CFG_REGS_AP_SPI1_AP_SPI1    (1<<30)
#define CFG_REGS_LCD24_CAM_MASK     (1<<31)
#define CFG_REGS_LCD24_CAM_NAND_IO  (0<<31)
#define CFG_REGS_LCD24_CAM_CAM_IO   (1<<31)

//IO_Drive1_Select
#define CFG_REGS_DDR_DRIVE_MASK     (7<<0)
#define CFG_REGS_DDR_DRIVE_FAST_AND_MOST_WEAK (7<<0)
#define CFG_REGS_DDR_DRIVE_FAST_AND_WEAK (6<<0)
#define CFG_REGS_DDR_DRIVE_FAST_AND_STRONG (5<<0)
#define CFG_REGS_DDR_DRIVE_FAST_AND_MOST_STRONG (4<<0)
#define CFG_REGS_DDR_DRIVE_SLOW_AND_MOST_WEAK (3<<0)
#define CFG_REGS_DDR_DRIVE_SLOW_AND_WEAK (2<<0)
#define CFG_REGS_DDR_DRIVE_SLOW_AND_STRONG (1<<0)
#define CFG_REGS_DDR_DRIVE_SLOW_AND_MOST_STRONG (0<<0)
#define CFG_REGS_PSRAM1_DRIVE_MASK  (7<<3)
#define CFG_REGS_PSRAM1_DRIVE_FAST_AND_MOST_WEAK (7<<3)
#define CFG_REGS_PSRAM1_DRIVE_FAST_AND_WEAK (6<<3)
#define CFG_REGS_PSRAM1_DRIVE_FAST_AND_STRONG (5<<3)
#define CFG_REGS_PSRAM1_DRIVE_FAST_AND_MOST_STRONG (4<<3)
#define CFG_REGS_PSRAM1_DRIVE_SLOW_AND_MOST_WEAK (3<<3)
#define CFG_REGS_PSRAM1_DRIVE_SLOW_AND_WEAK (2<<3)
#define CFG_REGS_PSRAM1_DRIVE_SLOW_AND_STRONG (1<<3)
#define CFG_REGS_PSRAM1_DRIVE_SLOW_AND_MOST_STRONG (0<<3)
#define CFG_REGS_PSRAM2_DRIVE_MASK  (7<<6)
#define CFG_REGS_PSRAM2_DRIVE_FAST_AND_MOST_WEAK (7<<6)
#define CFG_REGS_PSRAM2_DRIVE_FAST_AND_WEAK (6<<6)
#define CFG_REGS_PSRAM2_DRIVE_FAST_AND_STRONG (5<<6)
#define CFG_REGS_PSRAM2_DRIVE_FAST_AND_MOST_STRONG (4<<6)
#define CFG_REGS_PSRAM2_DRIVE_SLOW_AND_MOST_WEAK (3<<6)
#define CFG_REGS_PSRAM2_DRIVE_SLOW_AND_WEAK (2<<6)
#define CFG_REGS_PSRAM2_DRIVE_SLOW_AND_STRONG (1<<6)
#define CFG_REGS_PSRAM2_DRIVE_SLOW_AND_MOST_STRONG (0<<6)
#define CFG_REGS_NFLSH_DRIVE_MASK   (7<<9)
#define CFG_REGS_NFLSH_DRIVE_FAST_AND_MOST_WEAK (7<<9)
#define CFG_REGS_NFLSH_DRIVE_FAST_AND_WEAK (6<<9)
#define CFG_REGS_NFLSH_DRIVE_FAST_AND_STRONG (5<<9)
#define CFG_REGS_NFLSH_DRIVE_FAST_AND_MOST_STRONG (4<<9)
#define CFG_REGS_NFLSH_DRIVE_SLOW_AND_MOST_WEAK (3<<9)
#define CFG_REGS_NFLSH_DRIVE_SLOW_AND_WEAK (2<<9)
#define CFG_REGS_NFLSH_DRIVE_SLOW_AND_STRONG (1<<9)
#define CFG_REGS_NFLSH_DRIVE_SLOW_AND_MOST_STRONG (0<<9)
#define CFG_REGS_LCD1_DRIVE_MASK    (7<<12)
#define CFG_REGS_LCD1_DRIVE_FAST_AND_MOST_WEAK (7<<12)
#define CFG_REGS_LCD1_DRIVE_FAST_AND_WEAK (6<<12)
#define CFG_REGS_LCD1_DRIVE_FAST_AND_STRONG (5<<12)
#define CFG_REGS_LCD1_DRIVE_FAST_AND_MOST_STRONG (4<<12)
#define CFG_REGS_LCD1_DRIVE_SLOW_AND_MOST_WEAK (3<<12)
#define CFG_REGS_LCD1_DRIVE_SLOW_AND_WEAK (2<<12)
#define CFG_REGS_LCD1_DRIVE_SLOW_AND_STRONG (1<<12)
#define CFG_REGS_LCD1_DRIVE_SLOW_AND_MOST_STRONG (0<<12)
#define CFG_REGS_LCD2_DRIVE_MASK    (7<<15)
#define CFG_REGS_LCD2_DRIVE_FAST_AND_MOST_WEAK (7<<15)
#define CFG_REGS_LCD2_DRIVE_FAST_AND_WEAK (6<<15)
#define CFG_REGS_LCD2_DRIVE_FAST_AND_STRONG (5<<15)
#define CFG_REGS_LCD2_DRIVE_FAST_AND_MOST_STRONG (4<<15)
#define CFG_REGS_LCD2_DRIVE_SLOW_AND_MOST_WEAK (3<<15)
#define CFG_REGS_LCD2_DRIVE_SLOW_AND_WEAK (2<<15)
#define CFG_REGS_LCD2_DRIVE_SLOW_AND_STRONG (1<<15)
#define CFG_REGS_LCD2_DRIVE_SLOW_AND_MOST_STRONG (0<<15)
#define CFG_REGS_SDAT1_DRIVE_MASK   (7<<18)
#define CFG_REGS_SDAT1_DRIVE_FAST_AND_MOST_WEAK (7<<18)
#define CFG_REGS_SDAT1_DRIVE_FAST_AND_WEAK (6<<18)
#define CFG_REGS_SDAT1_DRIVE_FAST_AND_STRONG (5<<18)
#define CFG_REGS_SDAT1_DRIVE_FAST_AND_MOST_STRONG (4<<18)
#define CFG_REGS_SDAT1_DRIVE_SLOW_AND_MOST_WEAK (3<<18)
#define CFG_REGS_SDAT1_DRIVE_SLOW_AND_WEAK (2<<18)
#define CFG_REGS_SDAT1_DRIVE_SLOW_AND_STRONG (1<<18)
#define CFG_REGS_SDAT1_DRIVE_SLOW_AND_MOST_STRONG (0<<18)
#define CFG_REGS_SDAT2_DRIVE_MASK   (7<<21)
#define CFG_REGS_SDAT2_DRIVE_FAST_AND_MOST_WEAK (7<<21)
#define CFG_REGS_SDAT2_DRIVE_FAST_AND_WEAK (6<<21)
#define CFG_REGS_SDAT2_DRIVE_FAST_AND_STRONG (5<<21)
#define CFG_REGS_SDAT2_DRIVE_FAST_AND_MOST_STRONG (4<<21)
#define CFG_REGS_SDAT2_DRIVE_SLOW_AND_MOST_WEAK (3<<21)
#define CFG_REGS_SDAT2_DRIVE_SLOW_AND_WEAK (2<<21)
#define CFG_REGS_SDAT2_DRIVE_SLOW_AND_STRONG (1<<21)
#define CFG_REGS_SDAT2_DRIVE_SLOW_AND_MOST_STRONG (0<<21)
#define CFG_REGS_CAM_DRIVE_MASK     (3<<24)
#define CFG_REGS_CAM_DRIVE_MOST_STRONG (0<<24)
#define CFG_REGS_CAM_DRIVE_STRONG   (1<<24)
#define CFG_REGS_CAM_DRIVE_WEAK     (2<<24)
#define CFG_REGS_CAM_DRIVE_MOST_WEAK (3<<24)
#define CFG_REGS_SIM1_DRIVE_MASK    (3<<26)
#define CFG_REGS_SIM1_DRIVE_MOST_STRONG (0<<26)
#define CFG_REGS_SIM1_DRIVE_STRONG  (1<<26)
#define CFG_REGS_SIM1_DRIVE_WEAK    (2<<26)
#define CFG_REGS_SIM1_DRIVE_MOST_WEAK (3<<26)
#define CFG_REGS_SIM2_DRIVE_MASK    (3<<28)
#define CFG_REGS_SIM2_DRIVE_MOST_STRONG (0<<28)
#define CFG_REGS_SIM2_DRIVE_STRONG  (1<<28)
#define CFG_REGS_SIM2_DRIVE_WEAK    (2<<28)
#define CFG_REGS_SIM2_DRIVE_MOST_WEAK (3<<28)
#define CFG_REGS_SIM3_DRIVE_MASK    (3<<30)
#define CFG_REGS_SIM3_DRIVE_MOST_STRONG (0<<30)
#define CFG_REGS_SIM3_DRIVE_STRONG  (1<<30)
#define CFG_REGS_SIM3_DRIVE_WEAK    (2<<30)
#define CFG_REGS_SIM3_DRIVE_MOST_WEAK (3<<30)

//IO_Drive2_Select
#define CFG_REGS_GPIO_DRIVE_MASK    (3<<0)
#define CFG_REGS_GPIO_DRIVE_MOST_STRONG (0<<0)
#define CFG_REGS_GPIO_DRIVE_STRONG  (1<<0)
#define CFG_REGS_GPIO_DRIVE_WEAK    (2<<0)
#define CFG_REGS_GPIO_DRIVE_MOST_WEAK (3<<0)

//RAM_DRIVE
#define CFG_REGS_MBRAM_A(n)         (((n)&7)<<0)
#define CFG_REGS_MBRAM_A_MASK       (7<<0)
#define CFG_REGS_MBRAM_A_SHIFT      (0)
#define CFG_REGS_MBRAM_W(n)         (((n)&3)<<3)
#define CFG_REGS_MBRAM_W_MASK       (3<<3)
#define CFG_REGS_MBRAM_W_SHIFT      (3)
#define CFG_REGS_MBRAM_R            (1<<5)
#define CFG_REGS_BBRAM_A(n)         (((n)&7)<<6)
#define CFG_REGS_BBRAM_A_MASK       (7<<6)
#define CFG_REGS_BBRAM_A_SHIFT      (6)
#define CFG_REGS_BBRAM_W(n)         (((n)&3)<<9)
#define CFG_REGS_BBRAM_W_MASK       (3<<9)
#define CFG_REGS_BBRAM_W_SHIFT      (9)
#define CFG_REGS_BBRAM_R            (1<<11)
#define CFG_REGS_XP_A(n)            (((n)&7)<<12)
#define CFG_REGS_XP_A_MASK          (7<<12)
#define CFG_REGS_XP_A_SHIFT         (12)
#define CFG_REGS_XP_W(n)            (((n)&3)<<15)
#define CFG_REGS_XP_W_MASK          (3<<15)
#define CFG_REGS_XP_W_SHIFT         (15)
#define CFG_REGS_XP_R               (1<<17)
#define CFG_REGS_BP_A(n)            (((n)&7)<<18)
#define CFG_REGS_BP_A_MASK          (7<<18)
#define CFG_REGS_BP_A_SHIFT         (18)
#define CFG_REGS_BP_W(n)            (((n)&3)<<21)
#define CFG_REGS_BP_W_MASK          (3<<21)
#define CFG_REGS_BP_W_SHIFT         (21)
#define CFG_REGS_BP_R               (1<<23)
#define CFG_REGS_EV_A(n)            (((n)&7)<<24)
#define CFG_REGS_EV_A_MASK          (7<<24)
#define CFG_REGS_EV_A_SHIFT         (24)
#define CFG_REGS_EV_W(n)            (((n)&3)<<27)
#define CFG_REGS_EV_W_MASK          (3<<27)
#define CFG_REGS_EV_W_SHIFT         (27)
#define CFG_REGS_EV_R               (1<<29)

//H2X_AP_Offset
#define CFG_REGS_H2X_AP_OFFSET(n)   (((n)&0xFF)<<0)
#define CFG_REGS_H2X_AP_OFFSET_MASK (0xFF<<0)
#define CFG_REGS_H2X_AP_OFFSET_SHIFT (0)

//H2X_DDR_Offset
#define CFG_REGS_H2X_DDR_OFFSET(n)  (((n)&0xFF)<<0)
#define CFG_REGS_H2X_DDR_OFFSET_MASK (0xFF<<0)
#define CFG_REGS_H2X_DDR_OFFSET_SHIFT (0)

//audio_pd_set
#define CFG_REGS_AU_DEEP_PD_N       (1<<0)
#define CFG_REGS_AU_REF_PD_N        (1<<1)
#define CFG_REGS_AU_MIC_PD_N        (1<<2)
#define CFG_REGS_AU_AUXMIC_PD_N     (1<<3)
#define CFG_REGS_AU_AD_PD_N         (1<<4)
#define CFG_REGS_AU_DAC_PD_N        (1<<5)
#define CFG_REGS_AU_DAC_RESET_N     (1<<8)
#define CFG_REGS_AU_PLL_PU          (1<<16)

//audio_pd_clr
//#define CFG_REGS_AU_DEEP_PD_N     (1<<0)
//#define CFG_REGS_AU_REF_PD_N      (1<<1)
//#define CFG_REGS_AU_MIC_PD_N      (1<<2)
//#define CFG_REGS_AU_AUXMIC_PD_N   (1<<3)
//#define CFG_REGS_AU_AD_PD_N       (1<<4)
//#define CFG_REGS_AU_DAC_PD_N      (1<<5)
//#define CFG_REGS_AU_DAC_RESET_N   (1<<8)
//#define CFG_REGS_AU_PLL_PU        (1<<16)

//audio_sel_cfg
#define CFG_REGS_AU_AUXMIC_SEL      (1<<0)
#define CFG_REGS_AU_SPK_SEL         (1<<1)
#define CFG_REGS_AU_SPK_MONO_SEL    (1<<2)
#define CFG_REGS_AU_RCV_SEL         (1<<3)
#define CFG_REGS_AU_HEAD_SEL        (1<<4)

//audio_mic_cfg
#define CFG_REGS_AU_MIC_GAIN(n)     (((n)&15)<<0)
#define CFG_REGS_AU_MIC_GAIN_MASK   (15<<0)
#define CFG_REGS_AU_MIC_GAIN_SHIFT  (0)
#define CFG_REGS_AU_MIC_MUTE_N      (1<<4)

//audio_spk_cfg
#define CFG_REGS_AU_SPK_GAIN(n)     (((n)&15)<<0)
#define CFG_REGS_AU_SPK_GAIN_MASK   (15<<0)
#define CFG_REGS_AU_SPK_GAIN_SHIFT  (0)
#define CFG_REGS_AU_SPK_MUTE_N      (1<<4)

//audio_rcv_gain
#define CFG_REGS_AU_RCV_GAIN(n)     (((n)&15)<<0)
#define CFG_REGS_AU_RCV_GAIN_MASK   (15<<0)
#define CFG_REGS_AU_RCV_GAIN_SHIFT  (0)

//audio_head_gain
#define CFG_REGS_AU_HEAD_GAIN(n)    (((n)&15)<<0)
#define CFG_REGS_AU_HEAD_GAIN_MASK  (15<<0)
#define CFG_REGS_AU_HEAD_GAIN_SHIFT (0)

//TSC_DATA
#define CFG_REGS_TSC_X_VALUE_BIT(n) (((n)&0x3FF)<<0)
#define CFG_REGS_TSC_X_VALUE_BIT_MASK (0x3FF<<0)
#define CFG_REGS_TSC_X_VALUE_BIT_SHIFT (0)
#define CFG_REGS_TSC_X_VALUE_VALID  (1<<10)
#define CFG_REGS_TSC_Y_VALUE_BIT(n) (((n)&0x3FF)<<11)
#define CFG_REGS_TSC_Y_VALUE_BIT_MASK (0x3FF<<11)
#define CFG_REGS_TSC_Y_VALUE_BIT_SHIFT (11)
#define CFG_REGS_TSC_Y_VALUE_VALID  (1<<21)

//GPADC_DATA_CH
#define CFG_REGS_GPADC_DATA(n)      (((n)&0x3FF)<<0)
#define CFG_REGS_GPADC_DATA_MASK    (0x3FF<<0)
#define CFG_REGS_GPADC_DATA_SHIFT   (0)
#define CFG_REGS_GPADC_VALID        (1<<10)
#define CFG_REGS_GPADC_CH_EN        (1<<31)





#endif

