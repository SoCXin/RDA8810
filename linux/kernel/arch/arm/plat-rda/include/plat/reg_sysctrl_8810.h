#include <mach/hardware.h>
#include <mach/iomap.h>

// =============================================================================
//  MACROS
// =============================================================================

// ============================================================================
// AP_CPU_ID_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// CPU IDs
    CPU0                                        = 0x00000000,
    CPU1                                        = 0x00000001
} AP_CPU_ID_T;


// ============================================================================
// CPU_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP CPU side System clocks
    CPU_CORE                                    = 0x00000000,
/// the following don't have an auto enable
    CPU_DUMMY                                   = 0x00000001
} CPU_CLKS_T;

#define NB_CPU_CLK_AEN                           (1)
#define NB_CPU_CLK_EN                            (2)
#define NB_CPU_CLK                               (2)

// ============================================================================
// AXI_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP AXI side System clocks: AXI and AHB0 and APB0
    AHB0_CONF                                   = 0x00000000,
    APB0_CONF                                   = 0x00000001,
    AXI_VOC                                     = 0x00000002,
    AXI_DMA                                     = 0x00000003,
/// the following don't have an auto enable
    AXI_ALWAYS                                  = 0x00000004,
    AXI_CONNECT                                 = 0x00000005,
    APB0_IRQ                                    = 0x00000006
} AXI_CLKS_T;

#define NB_AXI_CLK_AEN                           (4)
#define NB_AXI_CLK_EN                            (7)
#define NB_AXI_CLK                               (7)

// ============================================================================
// AXIDIV2_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP AXIdiv2 side System clocks
    AXIDIV2_IMEM                                = 0x00000000,
/// the following don't have an auto enable
    AXIDIV2_ALWAYS                              = 0x00000001,
    AXIDIV2_CONNECT                             = 0x00000002,
    AXIDIV2_VPU                                 = 0x00000003
} AXIDIV2_CLKS_T;

#define NB_AXIDIV2_CLK_AEN                       (1)
#define NB_AXIDIV2_CLK_EN                        (4)
#define NB_AXIDIV2_CLK                           (4)

// ============================================================================
// GCG_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP GCG side System clocks
    GCG_APB_CONF                                = 0x00000000,
    GCG_GOUDA                                   = 0x00000001,
    GCG_CAMERA                                  = 0x00000002,
/// the following don't have an auto enable
    GCG_ALWAYS                                  = 0x00000003,
    GCG_CONNECT                                 = 0x00000004
} GCG_CLKS_T;

#define NB_GCG_CLK_AEN                           (3)
#define NB_GCG_CLK_EN                            (5)
#define NB_GCG_CLK                               (5)

// ============================================================================
// AHB1_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP AHB1 side System clocks
    AHB1_USBC                                   = 0x00000000,
/// the following don't have an auto enable
    AHB1_ALWAYS                                 = 0x00000001,
    AHB1_SPIFLASH                               = 0x00000002
} AHB1_CLKS_T;

#define NB_AHB1_CLK_AEN                          (1)
#define NB_AHB1_CLK_EN                           (3)
#define NB_AHB1_CLK                              (3)

// ============================================================================
// APB1_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP APB1 side System clocks
    APB1_CONF                                   = 0x00000000,
    APB1_AIF                                    = 0x00000001,
    APB1_AUIFC                                  = 0x00000002,
    APB1_AUIFC_CH0                              = 0x00000003,
    APB1_AUIFC_CH1                              = 0x00000004,
    APB1_I2C1                                   = 0x00000005,
    APB1_I2C2                                   = 0x00000006,
    APB1_I2C3                                   = 0x00000007,
/// AP APB1 side divided clock (either divided by module or by clock_ctrl)
    APB1D_OSC                                   = 0x00000008,
    APB1D_PWM                                   = 0x00000009,
/// the following don't have an auto enable
    APB1_ALWAYS                                 = 0x0000000A,
    APB1_DAPLITE                                = 0x0000000B,
    APB1_TIMER                                  = 0x0000000C,
    APB1_GPIO                                   = 0x0000000D
} APB1_CLKS_T;

#define NB_APB1_CLK_AEN                          (10)
#define NB_APB1_CLK_EN                           (14)
#define NB_APB1_CLK                              (14)

// ============================================================================
// APB2_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP APB2 side System clocks
    APB2_CONF                                   = 0x00000000,
    APB2_IFC                                    = 0x00000001,
    APB2_IFC_CH0                                = 0x00000002,
    APB2_IFC_CH1                                = 0x00000003,
    APB2_IFC_CH2                                = 0x00000004,
    APB2_IFC_CH3                                = 0x00000005,
    APB2_IFC_CH4                                = 0x00000006,
    APB2_IFC_CH5                                = 0x00000007,
    APB2_IFC_CH6                                = 0x00000008,
    APB2_IFC_CH7                                = 0x00000009,
    APB2_UART1                                  = 0x0000000A,
    APB2_UART2                                  = 0x0000000B,
    APB2_UART3                                  = 0x0000000C,
    APB2_SPI1                                   = 0x0000000D,
    APB2_SPI2                                   = 0x0000000E,
    APB2_SPI3                                   = 0x0000000F,
    APB2_SDMMC1                                 = 0x00000010,
    APB2_SDMMC2                                 = 0x00000011,
    APB2_SDMMC3                                 = 0x00000012,
/// the following don't have an auto enable
    APB2_ALWAYS                                 = 0x00000013,
    APB2_NANDFLASH                              = 0x00000014
} APB2_CLKS_T;

#define NB_APB2_CLK_AEN                          (19)
#define NB_APB2_CLK_EN                           (21)
#define NB_APB2_CLK                              (21)

// ============================================================================
// MEM_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP MEM side System clocks
    MEM_CONF                                    = 0x00000000,
/// the following don't have an auto enable
    MEM_DMC                                     = 0x00000001,
    MEM_GPU                                     = 0x00000002,
    MEM_VPU                                     = 0x00000003,
    MEM_DDRPHY_P                                = 0x00000004,
    MEM_CONNECT                                 = 0x00000005
} MEM_CLKS_T;

#define NB_MEM_CLK_AEN                           (1)
#define NB_MEM_CLK_EN                            (6)
#define NB_MEM_CLK                               (6)

// ============================================================================
// APO_CLKS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP side Other clocks
/// clocks with auto enble
    APOC_VPU                                    = 0x00000000,
    APOC_BCK                                    = 0x00000001,
    APOC_UART1                                  = 0x00000002,
    APOC_UART2                                  = 0x00000003,
    APOC_UART3                                  = 0x00000004,
    APOC_VOC_CORE                               = 0x00000005,
    APOC_VOC                                    = 0x00000006,
/// the following don't have an auto enable
    APOC_VOC_ALWAYS                             = 0x00000007,
    APOC_DDRPHY_N                               = 0x00000008,
    APOC_DDRPHY2XP                              = 0x00000009,
    APOC_DDRPHY2XN                              = 0x0000000A,
    APOC_GPU                                    = 0x0000000B,
    APOC_USBPHY                                 = 0x0000000C,
    APOC_CSI                                    = 0x0000000D,
    APOC_DSI                                    = 0x0000000E,
    APOC_GPIO                                   = 0x0000000F,
    APOC_SPIFLASH                               = 0x00000010,
    APOC_PIX                                    = 0x00000011,
    APOC_PDGB                                   = 0x00000012
} APO_CLKS_T;

#define NB_CLK_VOC_AEN_SYNC                      (5)
#define NB_CLK_VOC_CORE                          (6)
#define NB_APO_CLK_AEN                           (7)
#define NB_CLK_VOC_END                           (8)
#define NB_APO_CLK_EN                            (19)
#define NB_APO_CLK                               (19)

// ============================================================================
// CPU_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP CPU side resets
    CPU_RST_CORE                                = 0x00000000,
    CPU_RST_SYS                                 = 0x00000001
} CPU_RESETS_T;

#define NB_CPU_RST                               (2)

// ============================================================================
// AXI_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP AXI side resets: AXI and AHB0 and APB0
    AXI_RST_VOC                                 = 0x00000000,
    AXI_RST_DMA                                 = 0x00000001,
    AXI_RST_SYS                                 = 0x00000002,
    AXI_RST_CONNECT                             = 0x00000003,
    AHB0_RST_GPU                                = 0x00000004,
    APB0_RST_VPU                                = 0x00000005,
    APB0_RST_IRQ                                = 0x00000006
} AXI_RESETS_T;

#define NB_AXI_RST                               (7)

// ============================================================================
// AXIDIV2_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP AXIDIV2 side resets
    AXIDIV2_RST_IMEM                            = 0x00000000,
    AXIDIV2_RST_SYS                             = 0x00000001,
    AXIDIV2_RST_VPU                             = 0x00000002
} AXIDIV2_RESETS_T;

#define NB_AXIDIV2_RST                           (3)

// ============================================================================
// GCG_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP GCG side resets
    GCG_RST_SYS                                 = 0x00000000,
    GCG_RST_GOUDA                               = 0x00000001,
    GCG_RST_CAMERA                              = 0x00000002
} GCG_RESETS_T;

#define NB_GCG_RST                               (3)

// ============================================================================
// AHB1_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP AHB1 side resets
    AHB1_RST_SYS                                = 0x00000000,
    AHB1_RST_USBC                               = 0x00000001,
    AHB1_RST_SPIFLASH                           = 0x00000002
} AHB1_RESETS_T;

#define NB_AHB1_RST                              (3)

// ============================================================================
// APB1_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP APB1 side resets
    APB1_RST_SYS                                = 0x00000000,
    APB1_RST_TIMER                              = 0x00000001,
    APB1_RST_KEYPAD                             = 0x00000002,
    APB1_RST_GPIO                               = 0x00000003,
    APB1_RST_PWM                                = 0x00000004,
    APB1_RST_AIF                                = 0x00000005,
    APB1_RST_AUIFC                              = 0x00000006,
    APB1_RST_I2C1                               = 0x00000007,
    APB1_RST_I2C2                               = 0x00000008,
    APB1_RST_I2C3                               = 0x00000009,
    APB1_RST_COM_REGS                           = 0x0000000A,
    APB1_RST_DMC                                = 0x0000000B,
    APB1_RST_DDRPHY_P                           = 0x0000000C,
    APB1_RST_BB2G_XCPU                          = 0x0000000D,
    APB1_RST_BB2G_BCPU                          = 0x0000000E,
    APB1_RST_BB2G_AHBC                          = 0x0000000F,
    APB1_RST_BB2G_DMA                           = 0x00000010,
    APB1_RST_BB2G_A2A                           = 0x00000011,
    APB1_RST_BB2G_XIFC                          = 0x00000012,
    APB1_RST_BB2G_BIFC                          = 0x00000013,
    APB1_RST_BB2G_BAHBC                         = 0x00000014,
    APB1_RST_BB2G_MEM_BRIDGE                    = 0x00000015
} APB1_RESETS_T;

#define NB_APB1_RST                              (22)

// ============================================================================
// APB2_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP APB2 side resets
    APB2_RST_SYS                                = 0x00000000,
    APB2_RST_IFC                                = 0x00000001,
    APB2_RST_UART1                              = 0x00000002,
    APB2_RST_UART2                              = 0x00000003,
    APB2_RST_UART3                              = 0x00000004,
    APB2_RST_SPI1                               = 0x00000005,
    APB2_RST_SPI2                               = 0x00000006,
    APB2_RST_SPI3                               = 0x00000007,
    APB2_RST_SDMMC1                             = 0x00000008,
    APB2_RST_SDMMC2                             = 0x00000009,
    APB2_RST_SDMMC3                             = 0x0000000A,
    APB2_RST_NANDFLASH                          = 0x0000000B
} APB2_RESETS_T;

#define NB_APB2_RST                              (12)

// ============================================================================
// MEM_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP MEM side resets
    MEM_RST_SYS                                 = 0x00000000,
    MEM_RST_GPU                                 = 0x00000001,
    MEM_RST_VPU                                 = 0x00000002,
    MEM_RST_DMC                                 = 0x00000003,
    MEM_RST_DDRPHY_P                            = 0x00000004
} MEM_RESETS_T;

#define NB_MEM_RST                               (5)

// ============================================================================
// AP_OTHERS_RESETS_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef enum
{
/// AP Reset Other : resync on corresponding clock other
    AP_RSTO_VOC                                 = 0x00000000,
    AP_RSTO_DDRPHY_N                            = 0x00000001,
    AP_RSTO_DDRPHY2XP                           = 0x00000002,
    AP_RSTO_DDRPHY2XN                           = 0x00000003,
    AP_RSTO_GPU                                 = 0x00000004,
    AP_RSTO_VPU                                 = 0x00000005,
    AP_RSTO_BCK                                 = 0x00000006,
    AP_RSTO_UART1                               = 0x00000007,
    AP_RSTO_UART2                               = 0x00000008,
    AP_RSTO_UART3                               = 0x00000009,
    AP_RSTO_GPIO                                = 0x0000000A,
    AP_RSTO_TIMER                               = 0x0000000B,
    AP_RSTO_USBC                                = 0x0000000C,
    AP_RSTO_DSI                                 = 0x0000000D,
    AP_RSTO_SPIFLASH                            = 0x0000000E,
    AP_RSTO_TCK                                 = 0x0000000F,
    AP_RSTO_PDBG_XTAL                           = 0x00000010
} AP_OTHERS_RESETS_T;

#define NB_AP_RSTO                               (17)
/// For REG_DBG protect lock/unlock value
#define AP_CTRL_PROTECT_LOCK                     (0XA50000)
#define AP_CTRL_PROTECT_UNLOCK                   (0XA50001)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// SYS_CTRL_AP_T
// -----------------------------------------------------------------------------
///
// =============================================================================
typedef volatile struct
{
    /// <strong>This register is used to Lock and Unlock the protected registers.</strong>
    REG32                          REG_DBG;                      //0x00000000
    /// Register protected by Write_Unlocked_H.
    REG32                          Cfg_Pll_Ctrl[4];              //0x00000004
    /// This register is protected.
    REG32                          Sel_Clock;                    //0x00000014
    REG32                          Reset_Cause;                  //0x00000018
    /// This register is protected.
    REG32                          CPU_Rst_Set;                  //0x0000001C
    REG32                          CPU_Rst_Clr;                  //0x00000020
    /// This register is protected.
    REG32                          AXI_Rst_Set;                  //0x00000024
    REG32                          AXI_Rst_Clr;                  //0x00000028
    /// This register is protected.
    REG32                          AXIDIV2_Rst_Set;              //0x0000002C
    REG32                          AXIDIV2_Rst_Clr;              //0x00000030
    /// This register is protected.
    REG32                          GCG_Rst_Set;                  //0x00000034
    REG32                          GCG_Rst_Clr;                  //0x00000038
    /// This register is protected.
    REG32                          AHB1_Rst_Set;                 //0x0000003C
    REG32                          AHB1_Rst_Clr;                 //0x00000040
    /// This register is protected.
    REG32                          APB1_Rst_Set;                 //0x00000044
    REG32                          APB1_Rst_Clr;                 //0x00000048
    /// This register is protected.
    REG32                          APB2_Rst_Set;                 //0x0000004C
    REG32                          APB2_Rst_Clr;                 //0x00000050
    /// This register is protected.
    REG32                          MEM_Rst_Set;                  //0x00000054
    REG32                          MEM_Rst_Clr;                  //0x00000058
    REG32                          Clk_CPU_Mode;                 //0x0000005C
    REG32                          Clk_CPU_Enable;               //0x00000060
    /// This register is protected.
    REG32                          Clk_CPU_Disable;              //0x00000064
    REG32                          Clk_AXI_Mode;                 //0x00000068
    REG32                          Clk_AXI_Enable;               //0x0000006C
    /// This register is protected.
    REG32                          Clk_AXI_Disable;              //0x00000070
    REG32                          Clk_AXIDIV2_Mode;             //0x00000074
    REG32                          Clk_AXIDIV2_Enable;           //0x00000078
    /// This register is protected.
    REG32                          Clk_AXIDIV2_Disable;          //0x0000007C
    REG32                          Clk_GCG_Mode;                 //0x00000080
    REG32                          Clk_GCG_Enable;               //0x00000084
    /// This register is protected.
    REG32                          Clk_GCG_Disable;              //0x00000088
    REG32                          Clk_AHB1_Mode;                //0x0000008C
    REG32                          Clk_AHB1_Enable;              //0x00000090
    /// This register is protected.
    REG32                          Clk_AHB1_Disable;             //0x00000094
    REG32                          Clk_APB1_Mode;                //0x00000098
    REG32                          Clk_APB1_Enable;              //0x0000009C
    /// This register is protected.
    REG32                          Clk_APB1_Disable;             //0x000000A0
    REG32                          Clk_APB2_Mode;                //0x000000A4
    REG32                          Clk_APB2_Enable;              //0x000000A8
    /// This register is protected.
    REG32                          Clk_APB2_Disable;             //0x000000AC
    REG32                          Clk_MEM_Mode;                 //0x000000B0
    REG32                          Clk_MEM_Enable;               //0x000000B4
    /// This register is protected.
    REG32                          Clk_MEM_Disable;              //0x000000B8
    REG32                          Clk_APO_Mode;                 //0x000000BC
    REG32                          Clk_APO_Enable;               //0x000000C0
    /// This register is protected.
    REG32                          Clk_APO_Disable;              //0x000000C4
    REG32                          Cfg_Clk_AP_CPU;               //0x000000C8
    REG32                          Cfg_Clk_AP_AXI;               //0x000000CC
    REG32                          Cfg_Clk_AP_GCG;               //0x000000D0
    REG32                          Cfg_Clk_AP_AHB1;              //0x000000D4
    REG32                          Cfg_Clk_AP_APB1;              //0x000000D8
    REG32                          Cfg_Clk_AP_APB2;              //0x000000DC
    REG32                          Cfg_Clk_AP_MEM;               //0x000000E0
    REG32                          Cfg_Clk_AP_GPU;               //0x000000E4
    REG32                          Cfg_Clk_AP_VPU;               //0x000000E8
    REG32                          Cfg_Clk_AP_VOC;               //0x000000EC
    REG32                          Cfg_Clk_AP_SFLSH;             //0x000000F0
    REG32                          Cfg_Clk_Uart[3];              //0x000000F4
    REG32                          L2cc_Ctrl;                    //0x00000100
    REG32                          Spi_Ctrl;                     //0x00000104
    REG32                          Memory_Margin;                //0x00000108
    REG32                          Memory_Margin2;               //0x0000010C
    REG32                          Memory_Observe;               //0x00000110
    REG32 Reserved_00000114[58];                //0x00000114
    /// This register is reserved.
    REG32                          Cfg_Reserve;                  //0x000001FC
} HWP_SYS_CTRL_AP_T;

//REG_DBG
#define SYS_CTRL_AP_SCRATCH(n)      (((n)&0xFFFF)<<0)
#define SYS_CTRL_AP_WRITE_UNLOCK_STATUS (1<<30)
#define SYS_CTRL_AP_WRITE_UNLOCK    (1<<31)

//Cfg_Pll_Ctrl
#define SYS_CTRL_AP_AP_PLL_ENABLE   (1<<0)
#define SYS_CTRL_AP_AP_PLL_ENABLE_MASK (1<<0)
#define SYS_CTRL_AP_AP_PLL_ENABLE_SHIFT (0)
#define SYS_CTRL_AP_AP_PLL_ENABLE_POWER_DOWN (0<<0)
#define SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE (1<<0)
#define SYS_CTRL_AP_AP_PLL_LOCK_RESET (1<<4)
#define SYS_CTRL_AP_AP_PLL_LOCK_RESET_MASK (1<<4)
#define SYS_CTRL_AP_AP_PLL_LOCK_RESET_SHIFT (4)
#define SYS_CTRL_AP_AP_PLL_LOCK_RESET_RESET (0<<4)
#define SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET (1<<4)
#define SYS_CTRL_AP_AP_PLL_BYPASS   (1<<8)
#define SYS_CTRL_AP_AP_PLL_BYPASS_MASK (1<<8)
#define SYS_CTRL_AP_AP_PLL_BYPASS_SHIFT (8)
#define SYS_CTRL_AP_AP_PLL_BYPASS_PASS (0<<8)
#define SYS_CTRL_AP_AP_PLL_BYPASS_BYPASS (1<<8)
#define SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE (1<<12)
#define SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_MASK (1<<12)
#define SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_SHIFT (12)
#define SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_ENABLE (1<<12)
#define SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_DISABLE (0<<12)
#define SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW(n) (((n)&31)<<16)
#define SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW_MASK (31<<16)
#define SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW_SHIFT (16)
#define SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH(n) (((n)&31)<<24)
#define SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH_MASK (31<<24)
#define SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH_SHIFT (24)
#define SYS_CTRL_AP_PLL_AP_CFG(n)   (((n)&0x1F1F1111)<<0)
#define SYS_CTRL_AP_PLL_AP_CFG_MASK (0x1F1F1111<<0)
#define SYS_CTRL_AP_PLL_AP_CFG_SHIFT (0)

//Sel_Clock
#define SYS_CTRL_AP_SLOW_SEL_RF_OSCILLATOR (1<<0)
#define SYS_CTRL_AP_SLOW_SEL_RF_RF  (0<<0)
#define SYS_CTRL_AP_CPU_SEL_FAST_SLOW (1<<4)
#define SYS_CTRL_AP_CPU_SEL_FAST_FAST (0<<4)
#define SYS_CTRL_AP_BUS_SEL_FAST_SLOW (1<<5)
#define SYS_CTRL_AP_BUS_SEL_FAST_FAST (0<<5)
#define SYS_CTRL_AP_TIMER_SEL_FAST_SLOW (1<<7)
#define SYS_CTRL_AP_TIMER_SEL_FAST_FAST (0<<7)
#define SYS_CTRL_AP_RF_DETECTED_OK  (1<<8)
#define SYS_CTRL_AP_RF_DETECTED_NO  (0<<8)
#define SYS_CTRL_AP_RF_DETECT_BYPASS (1<<9)
#define SYS_CTRL_AP_RF_DETECT_RESET (1<<10)
#define SYS_CTRL_AP_RF_SELECTED_L   (1<<11)
#define SYS_CTRL_AP_PLL_LOCKED_CPU  (1<<12)
#define SYS_CTRL_AP_PLL_LOCKED_CPU_MASK (1<<12)
#define SYS_CTRL_AP_PLL_LOCKED_CPU_SHIFT (12)
#define SYS_CTRL_AP_PLL_LOCKED_CPU_LOCKED (1<<12)
#define SYS_CTRL_AP_PLL_LOCKED_CPU_NOT_LOCKED (0<<12)
#define SYS_CTRL_AP_PLL_LOCKED_BUS  (1<<13)
#define SYS_CTRL_AP_PLL_LOCKED_BUS_MASK (1<<13)
#define SYS_CTRL_AP_PLL_LOCKED_BUS_SHIFT (13)
#define SYS_CTRL_AP_PLL_LOCKED_BUS_LOCKED (1<<13)
#define SYS_CTRL_AP_PLL_LOCKED_BUS_NOT_LOCKED (0<<13)
#define SYS_CTRL_AP_PLL_LOCKED_MEM  (1<<14)
#define SYS_CTRL_AP_PLL_LOCKED_MEM_MASK (1<<14)
#define SYS_CTRL_AP_PLL_LOCKED_MEM_SHIFT (14)
#define SYS_CTRL_AP_PLL_LOCKED_MEM_LOCKED (1<<14)
#define SYS_CTRL_AP_PLL_LOCKED_MEM_NOT_LOCKED (0<<14)
#define SYS_CTRL_AP_PLL_LOCKED_USB  (1<<15)
#define SYS_CTRL_AP_PLL_LOCKED_USB_MASK (1<<15)
#define SYS_CTRL_AP_PLL_LOCKED_USB_SHIFT (15)
#define SYS_CTRL_AP_PLL_LOCKED_USB_LOCKED (1<<15)
#define SYS_CTRL_AP_PLL_LOCKED_USB_NOT_LOCKED (0<<15)
#define SYS_CTRL_AP_PLL_BYPASS_LOCK_CPU (1<<20)
#define SYS_CTRL_AP_PLL_BYPASS_LOCK_BUS (1<<21)
#define SYS_CTRL_AP_FAST_SELECTED_CPU_L (1<<30)
#define SYS_CTRL_AP_FAST_SELECTED_CPU_L_MASK (1<<30)
#define SYS_CTRL_AP_FAST_SELECTED_CPU_L_SHIFT (30)
#define SYS_CTRL_AP_FAST_SELECTED_BUS_L (1<<31)
#define SYS_CTRL_AP_FAST_SELECTED_BUS_L_MASK (1<<31)
#define SYS_CTRL_AP_FAST_SELECTED_BUS_L_SHIFT (31)

//Reset_Cause
#define SYS_CTRL_AP_WATCHDOG_RESET_HAPPENED (1<<0)
#define SYS_CTRL_AP_WATCHDOG_RESET_NO (0<<0)
#define SYS_CTRL_AP_GLOBALSOFT_RESET_HAPPENED (1<<4)
#define SYS_CTRL_AP_GLOBALSOFT_RESET_NO (0<<4)
#define SYS_CTRL_AP_HOSTDEBUG_RESET_HAPPENED (1<<5)
#define SYS_CTRL_AP_HOSTDEBUG_RESET_NO (0<<5)
#define SYS_CTRL_AP_ALARMCAUSE_HAPPENED (1<<6)
#define SYS_CTRL_AP_ALARMCAUSE_NO   (0<<6)
#define SYS_CTRL_AP_BOOT_MODE(n)    (((n)&0xFFFF)<<8)
#define SYS_CTRL_AP_BOOT_MODE_MASK  (0xFFFF<<8)
#define SYS_CTRL_AP_BOOT_MODE_SHIFT (8)
#define SYS_CTRL_AP_SW_BOOT_MODE(n) (((n)&0x7F)<<24)
#define SYS_CTRL_AP_SW_BOOT_MODE_MASK (0x7F<<24)
#define SYS_CTRL_AP_SW_BOOT_MODE_SHIFT (24)
#define SYS_CTRL_AP_FONCTIONAL_TEST_MODE (1<<31)

//CPU_Rst_Set
#define SYS_CTRL_AP_SET_CPU_RST_CORE (1<<0)
#define SYS_CTRL_AP_SET_CPU_RST_SYS (1<<1)
#define SYS_CTRL_AP_SOFT_RST        (1<<31)
#define SYS_CTRL_AP_SET_CPU_RST(n)  (((n)&3)<<0)
#define SYS_CTRL_AP_SET_CPU_RST_MASK (3<<0)
#define SYS_CTRL_AP_SET_CPU_RST_SHIFT (0)

//CPU_Rst_Clr
#define SYS_CTRL_AP_CLR_CPU_RST_CORE (1<<0)
#define SYS_CTRL_AP_CLR_CPU_RST_SYS (1<<1)
#define SYS_CTRL_AP_CLR_CPU_RST(n)  (((n)&3)<<0)
#define SYS_CTRL_AP_CLR_CPU_RST_MASK (3<<0)
#define SYS_CTRL_AP_CLR_CPU_RST_SHIFT (0)

//AXI_Rst_Set
#define SYS_CTRL_AP_SET_AXI_RST_VOC (1<<0)
#define SYS_CTRL_AP_SET_AXI_RST_DMA (1<<1)
#define SYS_CTRL_AP_SET_AXI_RST_SYS (1<<2)
#define SYS_CTRL_AP_SET_AXI_RST_CONNECT (1<<3)
#define SYS_CTRL_AP_SET_AHB0_RST_GPU (1<<4)
#define SYS_CTRL_AP_SET_APB0_RST_VPU (1<<5)
#define SYS_CTRL_AP_SET_APB0_RST_IRQ (1<<6)
#define SYS_CTRL_AP_SET_AXI_RST(n)  (((n)&0x7F)<<0)
#define SYS_CTRL_AP_SET_AXI_RST_MASK (0x7F<<0)
#define SYS_CTRL_AP_SET_AXI_RST_SHIFT (0)

//AXI_Rst_Clr
#define SYS_CTRL_AP_CLR_AXI_RST_VOC (1<<0)
#define SYS_CTRL_AP_CLR_AXI_RST_DMA (1<<1)
#define SYS_CTRL_AP_CLR_AXI_RST_SYS (1<<2)
#define SYS_CTRL_AP_CLR_AXI_RST_CONNECT (1<<3)
#define SYS_CTRL_AP_CLR_AHB0_RST_GPU (1<<4)
#define SYS_CTRL_AP_CLR_APB0_RST_VPU (1<<5)
#define SYS_CTRL_AP_CLR_APB0_RST_IRQ (1<<6)
#define SYS_CTRL_AP_CLR_AXI_RST(n)  (((n)&0x7F)<<0)
#define SYS_CTRL_AP_CLR_AXI_RST_MASK (0x7F<<0)
#define SYS_CTRL_AP_CLR_AXI_RST_SHIFT (0)

//AXIDIV2_Rst_Set
#define SYS_CTRL_AP_SET_AXIDIV2_RST_IMEM (1<<0)
#define SYS_CTRL_AP_SET_AXIDIV2_RST_SYS (1<<1)
#define SYS_CTRL_AP_SET_AXIDIV2_RST_VPU (1<<2)
#define SYS_CTRL_AP_SET_AXIDIV2_RST(n) (((n)&7)<<0)
#define SYS_CTRL_AP_SET_AXIDIV2_RST_MASK (7<<0)
#define SYS_CTRL_AP_SET_AXIDIV2_RST_SHIFT (0)

//AXIDIV2_Rst_Clr
#define SYS_CTRL_AP_CLR_AXIDIV2_RST_IMEM (1<<0)
#define SYS_CTRL_AP_CLR_AXIDIV2_RST_SYS (1<<1)
#define SYS_CTRL_AP_CLR_AXIDIV2_RST_VPU (1<<2)
#define SYS_CTRL_AP_CLR_AXIDIV2_RST(n) (((n)&7)<<0)
#define SYS_CTRL_AP_CLR_AXIDIV2_RST_MASK (7<<0)
#define SYS_CTRL_AP_CLR_AXIDIV2_RST_SHIFT (0)

//GCG_Rst_Set
#define SYS_CTRL_AP_SET_GCG_RST_SYS (1<<0)
#define SYS_CTRL_AP_SET_GCG_RST_GOUDA (1<<1)
#define SYS_CTRL_AP_SET_GCG_RST_CAMERA (1<<2)
#define SYS_CTRL_AP_SET_GCG_RST(n)  (((n)&7)<<0)
#define SYS_CTRL_AP_SET_GCG_RST_MASK (7<<0)
#define SYS_CTRL_AP_SET_GCG_RST_SHIFT (0)

//GCG_Rst_Clr
#define SYS_CTRL_AP_CLR_GCG_RST_SYS (1<<0)
#define SYS_CTRL_AP_CLR_GCG_RST_GOUDA (1<<1)
#define SYS_CTRL_AP_CLR_GCG_RST_CAMERA (1<<2)
#define SYS_CTRL_AP_CLR_GCG_RST(n)  (((n)&7)<<0)
#define SYS_CTRL_AP_CLR_GCG_RST_MASK (7<<0)
#define SYS_CTRL_AP_CLR_GCG_RST_SHIFT (0)

//AHB1_Rst_Set
#define SYS_CTRL_AP_SET_AHB1_RST_SYS (1<<0)
#define SYS_CTRL_AP_SET_AHB1_RST_USBC (1<<1)
#define SYS_CTRL_AP_SET_AHB1_RST_SPIFLASH (1<<2)
#define SYS_CTRL_AP_SET_AHB1_RST(n) (((n)&7)<<0)
#define SYS_CTRL_AP_SET_AHB1_RST_MASK (7<<0)
#define SYS_CTRL_AP_SET_AHB1_RST_SHIFT (0)

//AHB1_Rst_Clr
#define SYS_CTRL_AP_CLR_AHB1_RST_SYS (1<<0)
#define SYS_CTRL_AP_CLR_AHB1_RST_USBC (1<<1)
#define SYS_CTRL_AP_CLR_AHB1_RST_SPIFLASH (1<<2)
#define SYS_CTRL_AP_CLR_AHB1_RST(n) (((n)&7)<<0)
#define SYS_CTRL_AP_CLR_AHB1_RST_MASK (7<<0)
#define SYS_CTRL_AP_CLR_AHB1_RST_SHIFT (0)

//AXI_Rst_Set & AXI_Rst_Clr
#define SYS_CTRL_AP_AXI_RST_CLR_VPU         (1 << 5)

//AXIDIV2_Rst_Set & AXIDIV2_Rst_Clr
#define SYS_CTRL_AP_AXIDIV2_RST_CLR_VPU         (1 << 2)

//GCG_Set & GCG_Clr for gouda
#define SYS_CTRL_AP_GCG_RST_CLR_GOUDA		(1 << 1)

//GCG_Set & GCG_Clr for lcdc
#define SYS_CTRL_AP_GCG_RST_CLR_LCDC		(1 << 4)

//APB1_Rst_Set
#define SYS_CTRL_AP_SET_APB1_RST_SYS (1<<0)
#define SYS_CTRL_AP_SET_APB1_RST_TIMER (1<<1)
#define SYS_CTRL_AP_SET_APB1_RST_KEYPAD (1<<2)
#define SYS_CTRL_AP_SET_APB1_RST_GPIO (1<<3)
#define SYS_CTRL_AP_SET_APB1_RST_PWM (1<<4)
#define SYS_CTRL_AP_SET_APB1_RST_AIF (1<<5)
#define SYS_CTRL_AP_SET_APB1_RST_AUIFC (1<<6)
#define SYS_CTRL_AP_SET_APB1_RST_I2C1 (1<<7)
#define SYS_CTRL_AP_SET_APB1_RST_I2C2 (1<<8)
#define SYS_CTRL_AP_SET_APB1_RST_I2C3 (1<<9)
#define SYS_CTRL_AP_SET_APB1_RST_COM_REGS (1<<10)
#define SYS_CTRL_AP_SET_APB1_RST_DMC (1<<11)
#define SYS_CTRL_AP_SET_APB1_RST_DDRPHY_P (1<<12)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_XCPU (1<<13)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_BCPU (1<<14)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_AHBC (1<<15)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_DMA (1<<16)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_A2A (1<<17)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_XIFC (1<<18)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_BIFC (1<<19)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_BAHBC (1<<20)
#define SYS_CTRL_AP_SET_APB1_RST_BB2G_MEM_BRIDGE (1<<21)
#define SYS_CTRL_AP_SET_APB1_RST(n) (((n)&0x3FFFFF)<<0)
#define SYS_CTRL_AP_SET_APB1_RST_MASK (0x3FFFFF<<0)
#define SYS_CTRL_AP_SET_APB1_RST_SHIFT (0)

//APB1_Rst_Clr
#define SYS_CTRL_AP_CLR_APB1_RST_SYS (1<<0)
#define SYS_CTRL_AP_CLR_APB1_RST_TIMER (1<<1)
#define SYS_CTRL_AP_CLR_APB1_RST_KEYPAD (1<<2)
#define SYS_CTRL_AP_CLR_APB1_RST_GPIO (1<<3)
#define SYS_CTRL_AP_CLR_APB1_RST_PWM (1<<4)
#define SYS_CTRL_AP_CLR_APB1_RST_AIF (1<<5)
#define SYS_CTRL_AP_CLR_APB1_RST_AUIFC (1<<6)
#define SYS_CTRL_AP_CLR_APB1_RST_I2C1 (1<<7)
#define SYS_CTRL_AP_CLR_APB1_RST_I2C2 (1<<8)
#define SYS_CTRL_AP_CLR_APB1_RST_I2C3 (1<<9)
#define SYS_CTRL_AP_CLR_APB1_RST_COM_REGS (1<<10)
#define SYS_CTRL_AP_CLR_APB1_RST_DMC (1<<11)
#define SYS_CTRL_AP_CLR_APB1_RST_DDRPHY_P (1<<12)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_XCPU (1<<13)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_BCPU (1<<14)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_AHBC (1<<15)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_DMA (1<<16)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_A2A (1<<17)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_XIFC (1<<18)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_BIFC (1<<19)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_BAHBC (1<<20)
#define SYS_CTRL_AP_CLR_APB1_RST_BB2G_MEM_BRIDGE (1<<21)
#define SYS_CTRL_AP_CLR_APB1_RST(n) (((n)&0x3FFFFF)<<0)
#define SYS_CTRL_AP_CLR_APB1_RST_MASK (0x3FFFFF<<0)
#define SYS_CTRL_AP_CLR_APB1_RST_SHIFT (0)

//APB2_Rst_Set
#define SYS_CTRL_AP_SET_APB2_RST_SYS (1<<0)
#define SYS_CTRL_AP_SET_APB2_RST_IFC (1<<1)
#define SYS_CTRL_AP_SET_APB2_RST_UART1 (1<<2)
#define SYS_CTRL_AP_SET_APB2_RST_UART2 (1<<3)
#define SYS_CTRL_AP_SET_APB2_RST_UART3 (1<<4)
#define SYS_CTRL_AP_SET_APB2_RST_SPI1 (1<<5)
#define SYS_CTRL_AP_SET_APB2_RST_SPI2 (1<<6)
#define SYS_CTRL_AP_SET_APB2_RST_SPI3 (1<<7)
#define SYS_CTRL_AP_SET_APB2_RST_SDMMC1 (1<<8)
#define SYS_CTRL_AP_SET_APB2_RST_SDMMC2 (1<<9)
#define SYS_CTRL_AP_SET_APB2_RST_SDMMC3 (1<<10)
#define SYS_CTRL_AP_SET_APB2_RST_NANDFLASH (1<<11)
#define SYS_CTRL_AP_SET_APB2_RST(n) (((n)&0xFFF)<<0)
#define SYS_CTRL_AP_SET_APB2_RST_MASK (0xFFF<<0)
#define SYS_CTRL_AP_SET_APB2_RST_SHIFT (0)

//APB2_Rst_Clr
#define SYS_CTRL_AP_CLR_APB2_RST_SYS (1<<0)
#define SYS_CTRL_AP_CLR_APB2_RST_IFC (1<<1)
#define SYS_CTRL_AP_CLR_APB2_RST_UART1 (1<<2)
#define SYS_CTRL_AP_CLR_APB2_RST_UART2 (1<<3)
#define SYS_CTRL_AP_CLR_APB2_RST_UART3 (1<<4)
#define SYS_CTRL_AP_CLR_APB2_RST_SPI1 (1<<5)
#define SYS_CTRL_AP_CLR_APB2_RST_SPI2 (1<<6)
#define SYS_CTRL_AP_CLR_APB2_RST_SPI3 (1<<7)
#define SYS_CTRL_AP_CLR_APB2_RST_SDMMC1 (1<<8)
#define SYS_CTRL_AP_CLR_APB2_RST_SDMMC2 (1<<9)
#define SYS_CTRL_AP_CLR_APB2_RST_SDMMC3 (1<<10)
#define SYS_CTRL_AP_CLR_APB2_RST_NANDFLASH (1<<11)
#define SYS_CTRL_AP_CLR_APB2_RST(n) (((n)&0xFFF)<<0)
#define SYS_CTRL_AP_CLR_APB2_RST_MASK (0xFFF<<0)
#define SYS_CTRL_AP_CLR_APB2_RST_SHIFT (0)

//MEM_Rst_Set
#define SYS_CTRL_AP_SET_MEM_RST_SYS (1<<0)
#define SYS_CTRL_AP_SET_MEM_RST_GPU (1<<1)
#define SYS_CTRL_AP_SET_MEM_RST_VPU (1<<2)
#define SYS_CTRL_AP_SET_MEM_RST_DMC (1<<3)
#define SYS_CTRL_AP_SET_MEM_RST_DDRPHY_P (1<<4)
#define SYS_CTRL_AP_SET_MEM_RST(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_SET_MEM_RST_MASK (31<<0)
#define SYS_CTRL_AP_SET_MEM_RST_SHIFT (0)

//MEM_Rst_Clr
#define SYS_CTRL_AP_CLR_MEM_RST_SYS (1<<0)
#define SYS_CTRL_AP_CLR_MEM_RST_GPU (1<<1)
#define SYS_CTRL_AP_CLR_MEM_RST_VPU (1<<2)
#define SYS_CTRL_AP_CLR_MEM_RST_DMC (1<<3)
#define SYS_CTRL_AP_CLR_MEM_RST_DDRPHY_P (1<<4)
#define SYS_CTRL_AP_CLR_MEM_RST(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_CLR_MEM_RST_MASK (31<<0)
#define SYS_CTRL_AP_CLR_MEM_RST_SHIFT (0)

//Clk_CPU_Mode
#define SYS_CTRL_AP_MODE_CLK_CPU_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_CLK_CPU_MANUAL (1<<0)

//Clk_CPU_Enable
#define SYS_CTRL_AP_ENABLE_CPU_CORE (1<<0)
#define SYS_CTRL_AP_ENABLE_CPU_DUMMY (1<<1)
#define SYS_CTRL_AP_ENABLE_CLK_CPU(n) (((n)&3)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_CPU_MASK (3<<0)
#define SYS_CTRL_AP_ENABLE_CLK_CPU_SHIFT (0)

//Clk_CPU_Disable
#define SYS_CTRL_AP_DISABLE_CPU_CORE (1<<0)
#define SYS_CTRL_AP_DISABLE_CPU_DUMMY (1<<1)
#define SYS_CTRL_AP_DISABLE_CLK_CPU(n) (((n)&3)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_CPU_MASK (3<<0)
#define SYS_CTRL_AP_DISABLE_CLK_CPU_SHIFT (0)

//Clk_AXI_Mode
#define SYS_CTRL_AP_MODE_AHB0_CONF_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_AHB0_CONF_MANUAL (1<<0)
#define SYS_CTRL_AP_MODE_APB0_CONF_AUTOMATIC (0<<1)
#define SYS_CTRL_AP_MODE_APB0_CONF_MANUAL (1<<1)
#define SYS_CTRL_AP_MODE_AXI_VOC_AUTOMATIC (0<<2)
#define SYS_CTRL_AP_MODE_AXI_VOC_MANUAL (1<<2)
#define SYS_CTRL_AP_MODE_AXI_DMA_AUTOMATIC (0<<3)
#define SYS_CTRL_AP_MODE_AXI_DMA_MANUAL (1<<3)
#define SYS_CTRL_AP_MODE_CLK_AXI(n) (((n)&15)<<0)
#define SYS_CTRL_AP_MODE_CLK_AXI_MASK (15<<0)
#define SYS_CTRL_AP_MODE_CLK_AXI_SHIFT (0)

//Clk_AXI_Enable
#define SYS_CTRL_AP_ENABLE_AHB0_CONF (1<<0)
#define SYS_CTRL_AP_ENABLE_APB0_CONF (1<<1)
#define SYS_CTRL_AP_ENABLE_AXI_VOC  (1<<2)
#define SYS_CTRL_AP_ENABLE_AXI_DMA  (1<<3)
#define SYS_CTRL_AP_ENABLE_AXI_ALWAYS (1<<4)
#define SYS_CTRL_AP_ENABLE_AXI_CONNECT (1<<5)
#define SYS_CTRL_AP_ENABLE_APB0_IRQ (1<<6)
#define SYS_CTRL_AP_ENABLE_CLK_AXI(n) (((n)&0x7F)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_AXI_MASK (0x7F<<0)
#define SYS_CTRL_AP_ENABLE_CLK_AXI_SHIFT (0)

//Clk_AXI_Disable
#define SYS_CTRL_AP_DISABLE_AHB0_CONF (1<<0)
#define SYS_CTRL_AP_DISABLE_APB0_CONF (1<<1)
#define SYS_CTRL_AP_DISABLE_AXI_VOC (1<<2)
#define SYS_CTRL_AP_DISABLE_AXI_DMA (1<<3)
#define SYS_CTRL_AP_DISABLE_AXI_ALWAYS (1<<4)
#define SYS_CTRL_AP_DISABLE_AXI_CONNECT (1<<5)
#define SYS_CTRL_AP_DISABLE_APB0_IRQ (1<<6)
#define SYS_CTRL_AP_DISABLE_CLK_AXI(n) (((n)&0x7F)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_AXI_MASK (0x7F<<0)
#define SYS_CTRL_AP_DISABLE_CLK_AXI_SHIFT (0)

//Clk_AXIDIV2_Mode
#define SYS_CTRL_AP_MODE_CLK_AXIDIV2_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_CLK_AXIDIV2_MANUAL (1<<0)

//Clk_AXIDIV2_Enable
#define SYS_CTRL_AP_ENABLE_AXIDIV2_IMEM (1<<0)
#define SYS_CTRL_AP_ENABLE_AXIDIV2_ALWAYS (1<<1)
#define SYS_CTRL_AP_ENABLE_AXIDIV2_CONNECT (1<<2)
#define SYS_CTRL_AP_ENABLE_AXIDIV2_VPU (1<<3)
#define SYS_CTRL_AP_ENABLE_CLK_AXIDIV2(n) (((n)&15)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_AXIDIV2_MASK (15<<0)
#define SYS_CTRL_AP_ENABLE_CLK_AXIDIV2_SHIFT (0)

//Clk_AXIDIV2_Disable
#define SYS_CTRL_AP_DISABLE_AXIDIV2_IMEM (1<<0)
#define SYS_CTRL_AP_DISABLE_AXIDIV2_ALWAYS (1<<1)
#define SYS_CTRL_AP_DISABLE_AXIDIV2_CONNECT (1<<2)
#define SYS_CTRL_AP_DISABLE_AXIDIV2_VPU (1<<3)
#define SYS_CTRL_AP_DISABLE_CLK_AXIDIV2(n) (((n)&15)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_AXIDIV2_MASK (15<<0)
#define SYS_CTRL_AP_DISABLE_CLK_AXIDIV2_SHIFT (0)

//Clk_GCG_Mode
#define SYS_CTRL_AP_MODE_GCG_APB_CONF_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_GCG_APB_CONF_MANUAL (1<<0)
#define SYS_CTRL_AP_MODE_GCG_GOUDA_AUTOMATIC (0<<1)
#define SYS_CTRL_AP_MODE_GCG_GOUDA_MANUAL (1<<1)
#define SYS_CTRL_AP_MODE_GCG_CAMERA_AUTOMATIC (0<<2)
#define SYS_CTRL_AP_MODE_GCG_CAMERA_MANUAL (1<<2)
#define SYS_CTRL_AP_MODE_CLK_GCG(n) (((n)&7)<<0)
#define SYS_CTRL_AP_MODE_CLK_GCG_MASK (7<<0)
#define SYS_CTRL_AP_MODE_CLK_GCG_SHIFT (0)

//Clk_GCG_Enable
#define SYS_CTRL_AP_ENABLE_GCG_APB_CONF (1<<0)
#define SYS_CTRL_AP_ENABLE_GCG_GOUDA (1<<1)
#define SYS_CTRL_AP_ENABLE_GCG_CAMERA (1<<2)
#define SYS_CTRL_AP_ENABLE_GCG_ALWAYS (1<<3)
#define SYS_CTRL_AP_ENABLE_GCG_CONNECT (1<<4)
#define SYS_CTRL_AP_ENABLE_GCG_DPI (1<<7)
#define SYS_CTRL_AP_ENABLE_CLK_GCG(n) (((n)&31)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_GCG_MASK (31<<0)
#define SYS_CTRL_AP_ENABLE_CLK_GCG_SHIFT (0)

//Clk_GCG_Disable
#define SYS_CTRL_AP_DISABLE_GCG_APB_CONF (1<<0)
#define SYS_CTRL_AP_DISABLE_GCG_GOUDA (1<<1)
#define SYS_CTRL_AP_DISABLE_GCG_CAMERA (1<<2)
#define SYS_CTRL_AP_DISABLE_GCG_ALWAYS (1<<3)
#define SYS_CTRL_AP_DISABLE_GCG_CONNECT (1<<4)
#define SYS_CTRL_AP_DISABLE_GCG_DPI (1<<7)
#define SYS_CTRL_AP_DISABLE_CLK_GCG(n) (((n)&31)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_GCG_MASK (31<<0)
#define SYS_CTRL_AP_DISABLE_CLK_GCG_SHIFT (0)

//Clk_AHB1_Mode
#define SYS_CTRL_AP_MODE_CLK_AHB1_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_CLK_AHB1_MANUAL (1<<0)

//Clk_AHB1_Enable
#define SYS_CTRL_AP_ENABLE_AHB1_USBC (1<<0)
#define SYS_CTRL_AP_ENABLE_AHB1_ALWAYS (1<<1)
#define SYS_CTRL_AP_ENABLE_AHB1_SPIFLASH (1<<2)
#define SYS_CTRL_AP_ENABLE_CLK_AHB1(n) (((n)&7)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_AHB1_MASK (7<<0)
#define SYS_CTRL_AP_ENABLE_CLK_AHB1_SHIFT (0)

//Clk_AHB1_Disable
#define SYS_CTRL_AP_DISABLE_AHB1_USBC (1<<0)
#define SYS_CTRL_AP_DISABLE_AHB1_ALWAYS (1<<1)
#define SYS_CTRL_AP_DISABLE_AHB1_SPIFLASH (1<<2)
#define SYS_CTRL_AP_DISABLE_CLK_AHB1(n) (((n)&7)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_AHB1_MASK (7<<0)
#define SYS_CTRL_AP_DISABLE_CLK_AHB1_SHIFT (0)

//Clk_APB1_Mode
#define SYS_CTRL_AP_MODE_APB1_CONF_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_APB1_CONF_MANUAL (1<<0)
#define SYS_CTRL_AP_MODE_APB1_AIF_AUTOMATIC (0<<1)
#define SYS_CTRL_AP_MODE_APB1_AIF_MANUAL (1<<1)
#define SYS_CTRL_AP_MODE_APB1_AUIFC_AUTOMATIC (0<<2)
#define SYS_CTRL_AP_MODE_APB1_AUIFC_MANUAL (1<<2)
#define SYS_CTRL_AP_MODE_APB1_AUIFC_CH0_AUTOMATIC (0<<3)
#define SYS_CTRL_AP_MODE_APB1_AUIFC_CH0_MANUAL (1<<3)
#define SYS_CTRL_AP_MODE_APB1_AUIFC_CH1_AUTOMATIC (0<<4)
#define SYS_CTRL_AP_MODE_APB1_AUIFC_CH1_MANUAL (1<<4)
#define SYS_CTRL_AP_MODE_APB1_I2C1_AUTOMATIC (0<<5)
#define SYS_CTRL_AP_MODE_APB1_I2C1_MANUAL (1<<5)
#define SYS_CTRL_AP_MODE_APB1_I2C2_AUTOMATIC (0<<6)
#define SYS_CTRL_AP_MODE_APB1_I2C2_MANUAL (1<<6)
#define SYS_CTRL_AP_MODE_APB1_I2C3_AUTOMATIC (0<<7)
#define SYS_CTRL_AP_MODE_APB1_I2C3_MANUAL (1<<7)
#define SYS_CTRL_AP_MODE_APB1D_OSC_AUTOMATIC (0<<8)
#define SYS_CTRL_AP_MODE_APB1D_OSC_MANUAL (1<<8)
#define SYS_CTRL_AP_MODE_APB1D_PWM_AUTOMATIC (0<<9)
#define SYS_CTRL_AP_MODE_APB1D_PWM_MANUAL (1<<9)
#define SYS_CTRL_AP_MODE_CLK_APB1(n) (((n)&0x3FF)<<0)
#define SYS_CTRL_AP_MODE_CLK_APB1_MASK (0x3FF<<0)
#define SYS_CTRL_AP_MODE_CLK_APB1_SHIFT (0)

//Clk_APB1_Enable
#define SYS_CTRL_AP_ENABLE_APB1_CONF (1<<0)
#define SYS_CTRL_AP_ENABLE_APB1_AIF (1<<1)
#define SYS_CTRL_AP_ENABLE_APB1_AUIFC (1<<2)
#define SYS_CTRL_AP_ENABLE_APB1_AUIFC_CH0 (1<<3)
#define SYS_CTRL_AP_ENABLE_APB1_AUIFC_CH1 (1<<4)
#define SYS_CTRL_AP_ENABLE_APB1_I2C1 (1<<5)
#define SYS_CTRL_AP_ENABLE_APB1_I2C2 (1<<6)
#define SYS_CTRL_AP_ENABLE_APB1_I2C3 (1<<7)
#define SYS_CTRL_AP_ENABLE_APB1D_OSC (1<<8)
#define SYS_CTRL_AP_ENABLE_APB1D_PWM (1<<9)
#define SYS_CTRL_AP_ENABLE_APB1_ALWAYS (1<<10)
#define SYS_CTRL_AP_ENABLE_APB1_DAPLITE (1<<11)
#define SYS_CTRL_AP_ENABLE_APB1_TIMER (1<<12)
#define SYS_CTRL_AP_ENABLE_APB1_GPIO (1<<13)
#define SYS_CTRL_AP_ENABLE_CLK_APB1(n) (((n)&0x3FFF)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_APB1_MASK (0x3FFF<<0)
#define SYS_CTRL_AP_ENABLE_CLK_APB1_SHIFT (0)

//Clk_APB1_Disable
#define SYS_CTRL_AP_DISABLE_APB1_CONF (1<<0)
#define SYS_CTRL_AP_DISABLE_APB1_AIF (1<<1)
#define SYS_CTRL_AP_DISABLE_APB1_AUIFC (1<<2)
#define SYS_CTRL_AP_DISABLE_APB1_AUIFC_CH0 (1<<3)
#define SYS_CTRL_AP_DISABLE_APB1_AUIFC_CH1 (1<<4)
#define SYS_CTRL_AP_DISABLE_APB1_I2C1 (1<<5)
#define SYS_CTRL_AP_DISABLE_APB1_I2C2 (1<<6)
#define SYS_CTRL_AP_DISABLE_APB1_I2C3 (1<<7)
#define SYS_CTRL_AP_DISABLE_APB1D_OSC (1<<8)
#define SYS_CTRL_AP_DISABLE_APB1D_PWM (1<<9)
#define SYS_CTRL_AP_DISABLE_APB1_ALWAYS (1<<10)
#define SYS_CTRL_AP_DISABLE_APB1_DAPLITE (1<<11)
#define SYS_CTRL_AP_DISABLE_APB1_TIMER (1<<12)
#define SYS_CTRL_AP_DISABLE_APB1_GPIO (1<<13)
#define SYS_CTRL_AP_DISABLE_CLK_APB1(n) (((n)&0x3FFF)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_APB1_MASK (0x3FFF<<0)
#define SYS_CTRL_AP_DISABLE_CLK_APB1_SHIFT (0)

//Clk_APB2_Mode
#define SYS_CTRL_AP_MODE_APB2_CONF_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_APB2_CONF_MANUAL (1<<0)
#define SYS_CTRL_AP_MODE_APB2_IFC_AUTOMATIC (0<<1)
#define SYS_CTRL_AP_MODE_APB2_IFC_MANUAL (1<<1)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH0_AUTOMATIC (0<<2)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH0_MANUAL (1<<2)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH1_AUTOMATIC (0<<3)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH1_MANUAL (1<<3)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH2_AUTOMATIC (0<<4)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH2_MANUAL (1<<4)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH3_AUTOMATIC (0<<5)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH3_MANUAL (1<<5)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH4_AUTOMATIC (0<<6)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH4_MANUAL (1<<6)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH5_AUTOMATIC (0<<7)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH5_MANUAL (1<<7)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH6_AUTOMATIC (0<<8)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH6_MANUAL (1<<8)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH7_AUTOMATIC (0<<9)
#define SYS_CTRL_AP_MODE_APB2_IFC_CH7_MANUAL (1<<9)
#define SYS_CTRL_AP_MODE_APB2_UART1_AUTOMATIC (0<<10)
#define SYS_CTRL_AP_MODE_APB2_UART1_MANUAL (1<<10)
#define SYS_CTRL_AP_MODE_APB2_UART2_AUTOMATIC (0<<11)
#define SYS_CTRL_AP_MODE_APB2_UART2_MANUAL (1<<11)
#define SYS_CTRL_AP_MODE_APB2_UART3_AUTOMATIC (0<<12)
#define SYS_CTRL_AP_MODE_APB2_UART3_MANUAL (1<<12)
#define SYS_CTRL_AP_MODE_APB2_SPI1_AUTOMATIC (0<<13)
#define SYS_CTRL_AP_MODE_APB2_SPI1_MANUAL (1<<13)
#define SYS_CTRL_AP_MODE_APB2_SPI2_AUTOMATIC (0<<14)
#define SYS_CTRL_AP_MODE_APB2_SPI2_MANUAL (1<<14)
#define SYS_CTRL_AP_MODE_APB2_SPI3_AUTOMATIC (0<<15)
#define SYS_CTRL_AP_MODE_APB2_SPI3_MANUAL (1<<15)
#define SYS_CTRL_AP_MODE_APB2_SDMMC1_AUTOMATIC (0<<16)
#define SYS_CTRL_AP_MODE_APB2_SDMMC1_MANUAL (1<<16)
#define SYS_CTRL_AP_MODE_APB2_SDMMC2_AUTOMATIC (0<<17)
#define SYS_CTRL_AP_MODE_APB2_SDMMC2_MANUAL (1<<17)
#define SYS_CTRL_AP_MODE_APB2_SDMMC3_AUTOMATIC (0<<18)
#define SYS_CTRL_AP_MODE_APB2_SDMMC3_MANUAL (1<<18)
#define SYS_CTRL_AP_MODE_CLK_APB2(n) (((n)&0x7FFFF)<<0)
#define SYS_CTRL_AP_MODE_CLK_APB2_MASK (0x7FFFF<<0)
#define SYS_CTRL_AP_MODE_CLK_APB2_SHIFT (0)

//Clk_APB2_Enable
#define SYS_CTRL_AP_ENABLE_APB2_CONF (1<<0)
#define SYS_CTRL_AP_ENABLE_APB2_IFC (1<<1)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH0 (1<<2)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH1 (1<<3)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH2 (1<<4)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH3 (1<<5)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH4 (1<<6)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH5 (1<<7)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH6 (1<<8)
#define SYS_CTRL_AP_ENABLE_APB2_IFC_CH7 (1<<9)
#define SYS_CTRL_AP_ENABLE_APB2_UART1 (1<<10)
#define SYS_CTRL_AP_ENABLE_APB2_UART2 (1<<11)
#define SYS_CTRL_AP_ENABLE_APB2_UART3 (1<<12)
#define SYS_CTRL_AP_ENABLE_APB2_SPI1 (1<<13)
#define SYS_CTRL_AP_ENABLE_APB2_SPI2 (1<<14)
#define SYS_CTRL_AP_ENABLE_APB2_SPI3 (1<<15)
#define SYS_CTRL_AP_ENABLE_APB2_SDMMC1 (1<<16)
#define SYS_CTRL_AP_ENABLE_APB2_SDMMC2 (1<<17)
#define SYS_CTRL_AP_ENABLE_APB2_SDMMC3 (1<<18)
#define SYS_CTRL_AP_ENABLE_APB2_ALWAYS (1<<19)
#define SYS_CTRL_AP_ENABLE_APB2_NANDFLASH (1<<20)
#define SYS_CTRL_AP_ENABLE_CLK_APB2(n) (((n)&0x1FFFFF)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_APB2_MASK (0x1FFFFF<<0)
#define SYS_CTRL_AP_ENABLE_CLK_APB2_SHIFT (0)

//Clk_APB2_Disable
#define SYS_CTRL_AP_DISABLE_APB2_CONF (1<<0)
#define SYS_CTRL_AP_DISABLE_APB2_IFC (1<<1)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH0 (1<<2)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH1 (1<<3)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH2 (1<<4)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH3 (1<<5)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH4 (1<<6)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH5 (1<<7)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH6 (1<<8)
#define SYS_CTRL_AP_DISABLE_APB2_IFC_CH7 (1<<9)
#define SYS_CTRL_AP_DISABLE_APB2_UART1 (1<<10)
#define SYS_CTRL_AP_DISABLE_APB2_UART2 (1<<11)
#define SYS_CTRL_AP_DISABLE_APB2_UART3 (1<<12)
#define SYS_CTRL_AP_DISABLE_APB2_SPI1 (1<<13)
#define SYS_CTRL_AP_DISABLE_APB2_SPI2 (1<<14)
#define SYS_CTRL_AP_DISABLE_APB2_SPI3 (1<<15)
#define SYS_CTRL_AP_DISABLE_APB2_SDMMC1 (1<<16)
#define SYS_CTRL_AP_DISABLE_APB2_SDMMC2 (1<<17)
#define SYS_CTRL_AP_DISABLE_APB2_SDMMC3 (1<<18)
#define SYS_CTRL_AP_DISABLE_APB2_ALWAYS (1<<19)
#define SYS_CTRL_AP_DISABLE_APB2_NANDFLASH (1<<20)
#define SYS_CTRL_AP_DISABLE_CLK_APB2(n) (((n)&0x1FFFFF)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_APB2_MASK (0x1FFFFF<<0)
#define SYS_CTRL_AP_DISABLE_CLK_APB2_SHIFT (0)

//Clk_MEM_Mode
#define SYS_CTRL_AP_MODE_CLK_MEM_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_CLK_MEM_MANUAL (1<<0)

//Clk_MEM_Enable
#define SYS_CTRL_AP_ENABLE_MEM_CONF (1<<0)
#define SYS_CTRL_AP_ENABLE_MEM_DMC  (1<<1)
#define SYS_CTRL_AP_ENABLE_MEM_GPU  (1<<2)
#define SYS_CTRL_AP_ENABLE_MEM_VPU  (1<<3)
#define SYS_CTRL_AP_ENABLE_MEM_DDRPHY_P (1<<4)
#define SYS_CTRL_AP_ENABLE_MEM_CONNECT (1<<5)
#define SYS_CTRL_AP_ENABLE_CLK_MEM(n) (((n)&0x3F)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_MEM_MASK (0x3F<<0)
#define SYS_CTRL_AP_ENABLE_CLK_MEM_SHIFT (0)

//Clk_MEM_Disable
#define SYS_CTRL_AP_DISABLE_MEM_CONF (1<<0)
#define SYS_CTRL_AP_DISABLE_MEM_DMC (1<<1)
#define SYS_CTRL_AP_DISABLE_MEM_GPU (1<<2)
#define SYS_CTRL_AP_DISABLE_MEM_VPU (1<<3)
#define SYS_CTRL_AP_DISABLE_MEM_DDRPHY_P (1<<4)
#define SYS_CTRL_AP_DISABLE_MEM_CONNECT (1<<5)
#define SYS_CTRL_AP_DISABLE_CLK_MEM(n) (((n)&0x3F)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_MEM_MASK (0x3F<<0)
#define SYS_CTRL_AP_DISABLE_CLK_MEM_SHIFT (0)

//Clk_APO_Mode
#define SYS_CTRL_AP_MODE_APOC_VPU_AUTOMATIC (0<<0)
#define SYS_CTRL_AP_MODE_APOC_VPU_MANUAL (1<<0)
#define SYS_CTRL_AP_MODE_APOC_BCK_AUTOMATIC (0<<1)
#define SYS_CTRL_AP_MODE_APOC_BCK_MANUAL (1<<1)
#define SYS_CTRL_AP_MODE_APOC_UART1_AUTOMATIC (0<<2)
#define SYS_CTRL_AP_MODE_APOC_UART1_MANUAL (1<<2)
#define SYS_CTRL_AP_MODE_APOC_UART2_AUTOMATIC (0<<3)
#define SYS_CTRL_AP_MODE_APOC_UART2_MANUAL (1<<3)
#define SYS_CTRL_AP_MODE_APOC_UART3_AUTOMATIC (0<<4)
#define SYS_CTRL_AP_MODE_APOC_UART3_MANUAL (1<<4)
#define SYS_CTRL_AP_MODE_APOC_VOC_CORE_AUTOMATIC (0<<5)
#define SYS_CTRL_AP_MODE_APOC_VOC_CORE_MANUAL (1<<5)
#define SYS_CTRL_AP_MODE_APOC_VOC_AUTOMATIC (0<<6)
#define SYS_CTRL_AP_MODE_APOC_VOC_MANUAL (1<<6)
#define SYS_CTRL_AP_MODE_CLK_APO(n) (((n)&0x7F)<<0)
#define SYS_CTRL_AP_MODE_CLK_APO_MASK (0x7F<<0)
#define SYS_CTRL_AP_MODE_CLK_APO_SHIFT (0)

//Clk_APO_Enable
#define SYS_CTRL_AP_ENABLE_APOC_VPU (1<<0)
#define SYS_CTRL_AP_ENABLE_APOC_BCK (1<<1)
#define SYS_CTRL_AP_ENABLE_APOC_UART1 (1<<2)
#define SYS_CTRL_AP_ENABLE_APOC_UART2 (1<<3)
#define SYS_CTRL_AP_ENABLE_APOC_UART3 (1<<4)
#define SYS_CTRL_AP_ENABLE_APOC_VOC_CORE (1<<5)
#define SYS_CTRL_AP_ENABLE_APOC_VOC (1<<6)
#define SYS_CTRL_AP_ENABLE_APOC_VOC_ALWAYS (1<<7)
#define SYS_CTRL_AP_ENABLE_APOC_DDRPHY_N (1<<8)
#define SYS_CTRL_AP_ENABLE_APOC_DDRPHY2XP (1<<9)
#define SYS_CTRL_AP_ENABLE_APOC_DDRPHY2XN (1<<10)
#define SYS_CTRL_AP_ENABLE_APOC_GPU (1<<11)
#define SYS_CTRL_AP_ENABLE_APOC_USBPHY (1<<12)
#define SYS_CTRL_AP_ENABLE_APOC_CSI (1<<13)
#define SYS_CTRL_AP_ENABLE_APOC_DSI (1<<14)
#define SYS_CTRL_AP_ENABLE_APOC_GPIO (1<<15)
#define SYS_CTRL_AP_ENABLE_APOC_SPIFLASH (1<<16)
#define SYS_CTRL_AP_ENABLE_APOC_PIX (1<<17)
#define SYS_CTRL_AP_ENABLE_APOC_PDGB (1<<18)
#define SYS_CTRL_AP_ENABLE_CLK_APO(n) (((n)&0x7FFFF)<<0)
#define SYS_CTRL_AP_ENABLE_CLK_APO_MASK (0x7FFFF<<0)
#define SYS_CTRL_AP_ENABLE_CLK_APO_SHIFT (0)

//Clk_APO_Disable
#define SYS_CTRL_AP_DISABLE_APOC_VPU (1<<0)
#define SYS_CTRL_AP_DISABLE_APOC_BCK (1<<1)
#define SYS_CTRL_AP_DISABLE_APOC_UART1 (1<<2)
#define SYS_CTRL_AP_DISABLE_APOC_UART2 (1<<3)
#define SYS_CTRL_AP_DISABLE_APOC_UART3 (1<<4)
#define SYS_CTRL_AP_DISABLE_APOC_VOC_CORE (1<<5)
#define SYS_CTRL_AP_DISABLE_APOC_VOC (1<<6)
#define SYS_CTRL_AP_DISABLE_APOC_VOC_ALWAYS (1<<7)
#define SYS_CTRL_AP_DISABLE_APOC_DDRPHY_N (1<<8)
#define SYS_CTRL_AP_DISABLE_APOC_DDRPHY2XP (1<<9)
#define SYS_CTRL_AP_DISABLE_APOC_DDRPHY2XN (1<<10)
#define SYS_CTRL_AP_DISABLE_APOC_GPU (1<<11)
#define SYS_CTRL_AP_DISABLE_APOC_USBPHY (1<<12)
#define SYS_CTRL_AP_DISABLE_APOC_CSI (1<<13)
#define SYS_CTRL_AP_DISABLE_APOC_DSI (1<<14)
#define SYS_CTRL_AP_DISABLE_APOC_GPIO (1<<15)
#define SYS_CTRL_AP_DISABLE_APOC_SPIFLASH (1<<16)
#define SYS_CTRL_AP_DISABLE_APOC_PIX (1<<17)
#define SYS_CTRL_AP_DISABLE_APOC_PDGB (1<<18)
#define SYS_CTRL_AP_DISABLE_CLK_APO(n) (((n)&0x7FFFF)<<0)
#define SYS_CTRL_AP_DISABLE_CLK_APO_MASK (0x7FFFF<<0)
#define SYS_CTRL_AP_DISABLE_CLK_APO_SHIFT (0)

//Cfg_Clk_AP_CPU
#define SYS_CTRL_AP_AP_CPU_FREQ(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_CPU_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_CPU_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_BUS_DIV_SEL(n)  (((n)&3)<<16)
#define SYS_CTRL_AP_BUS_DIV_SEL_MASK (3<<16)
#define SYS_CTRL_AP_BUS_DIV_SEL_SHIFT (16)
#define SYS_CTRL_AP_BUS_DIV_SEL_DIV2 (0<<16)
#define SYS_CTRL_AP_BUS_DIV_SEL_DIV3 (1<<16)
#define SYS_CTRL_AP_BUS_DIV_SEL_DIV4 (2<<16)
#define SYS_CTRL_AP_AP_CPU_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_AXI
#define SYS_CTRL_AP_AP_AXI_FREQ(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_AXI_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_AXI_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_AXI_SRC_SEL  (1<<12)
#define SYS_CTRL_AP_AP_AXI_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_GCG
#define SYS_CTRL_AP_AP_GCG_FREQ(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_GCG_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_GCG_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_GCG_SRC_SEL  (1<<12)
#define SYS_CTRL_AP_AP_GCG_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_AHB1
#define SYS_CTRL_AP_AP_AHB1_FREQ(n) (((n)&31)<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_AHB1_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_AHB1_SRC_SEL (1<<12)
#define SYS_CTRL_AP_AP_AHB1_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_APB1
#define SYS_CTRL_AP_AP_APB1_FREQ(n) (((n)&31)<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_APB1_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_APB1_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_APB1_SRC_SEL (1<<12)
#define SYS_CTRL_AP_AP_APB1_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_APB2
#define SYS_CTRL_AP_AP_APB2_FREQ(n) (((n)&31)<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_APB2_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_APB2_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_APB2_SRC_SEL (1<<12)
#define SYS_CTRL_AP_AP_APB2_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_MEM
#define SYS_CTRL_AP_AP_MEM_SRC_DIV2 (1<<12)

//Cfg_Clk_AP_GPU
#define SYS_CTRL_AP_AP_GPU_FREQ(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_GPU_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_GPU_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_GPU_SRC_DIV2 (1<<12)
#define SYS_CTRL_AP_AP_GPU_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_VPU
#define SYS_CTRL_AP_AP_VPU_FREQ(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_VPU_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_VPU_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_VPU_SRC_DIV2 (1<<12)
#define SYS_CTRL_AP_AP_VPU_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_VOC
#define SYS_CTRL_AP_AP_VOC_FREQ(n)  (((n)&31)<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_VOC_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_VOC_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_VOC_SRC_DIV2 (1<<12)
#define SYS_CTRL_AP_AP_VOC_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_AP_SFLSH
#define SYS_CTRL_AP_AP_SFLSH_FREQ(n) (((n)&31)<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_MASK (31<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_SHIFT (0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_19M (6<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_20M (7<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_300M (26<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_340M (27<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_400M (28<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_480M (29<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_600M (30<<0)
#define SYS_CTRL_AP_AP_SFLSH_FREQ_1_2G (31<<0)
#define SYS_CTRL_AP_AP_SFLSH_SRC_DIV2 (1<<12)
#define SYS_CTRL_AP_AP_SFLSH_REQ_DIV_UPDATE (1<<31)

//Cfg_Clk_Uart
#define SYS_CTRL_AP_UART_DIVIDER(n) (((n)&0x3FF)<<0)
#define SYS_CTRL_AP_UART_DIVIDER_MASK (0x3FF<<0)
#define SYS_CTRL_AP_UART_DIVIDER_SHIFT (0)
#define SYS_CTRL_AP_UART_SEL_PLL_SLOW (0<<12)
#define SYS_CTRL_AP_UART_SEL_PLL_PLL (1<<12)

//L2cc_Ctrl
#define SYS_CTRL_AP_ARQOS_L2CC_1_M(n) (((n)&15)<<0)
#define SYS_CTRL_AP_ARQOS_L2CC_1_M_MASK (15<<0)
#define SYS_CTRL_AP_ARQOS_L2CC_1_M_SHIFT (0)
#define SYS_CTRL_AP_AWQOS_L2CC_1_M(n) (((n)&15)<<4)
#define SYS_CTRL_AP_AWQOS_L2CC_1_M_MASK (15<<4)
#define SYS_CTRL_AP_AWQOS_L2CC_1_M_SHIFT (4)
#define SYS_CTRL_AP_ARQOS_MODEM_MEM_AHBS_M(n) (((n)&15)<<8)
#define SYS_CTRL_AP_ARQOS_MODEM_MEM_AHBS_M_MASK (15<<8)
#define SYS_CTRL_AP_ARQOS_MODEM_MEM_AHBS_M_SHIFT (8)
#define SYS_CTRL_AP_AWQOS_MODEM_MEM_AHBS_M(n) (((n)&15)<<12)
#define SYS_CTRL_AP_AWQOS_MODEM_MEM_AHBS_M_MASK (15<<12)
#define SYS_CTRL_AP_AWQOS_MODEM_MEM_AHBS_M_SHIFT (12)
#define SYS_CTRL_AP_ARAP_L2CC_1_M   (1<<16)
#define SYS_CTRL_AP_AWAP_L2CC_1_M   (1<<17)
#define SYS_CTRL_AP_ARAP_MODEM_MEM_AHBS_M (1<<18)
#define SYS_CTRL_AP_AWAP_MODEM_MEM_AHBS_M (1<<19)
#define SYS_CTRL_AP_ARAP_MERGE1M    (1<<20)
#define SYS_CTRL_AP_AWAP_MERGE1M    (1<<21)
#define SYS_CTRL_AP_ARAP_MERGE2M    (1<<22)
#define SYS_CTRL_AP_AWAP_MERGE2M    (1<<23)
#define SYS_CTRL_AP_RESERVE1(n)     (((n)&0xFF)<<24)
#define SYS_CTRL_AP_RESERVE1_MASK   (0xFF<<24)
#define SYS_CTRL_AP_RESERVE1_SHIFT  (24)

//Spi_Ctrl
#define SYS_CTRL_AP_LIMITED_EN_SPI1 (1<<0)
#define SYS_CTRL_AP_LPSEN_SPI1      (1<<1)
#define SYS_CTRL_AP_LIMITED_EN_SPI2 (1<<2)
#define SYS_CTRL_AP_LPSEN_SPI2      (1<<3)
#define SYS_CTRL_AP_LIMITED_EN_SPI3 (1<<4)
#define SYS_CTRL_AP_LPSEN_SPI3      (1<<5)
#define SYS_CTRL_AP_FBUSWID_NFSC    (1<<6)
#define SYS_CTRL_AP_RESERVE3        (1<<7)
#define SYS_CTRL_AP_DMC_CFG(n)      (((n)&0x3FF)<<8)
#define SYS_CTRL_AP_DMC_CFG_MASK    (0x3FF<<8)
#define SYS_CTRL_AP_DMC_CFG_SHIFT   (8)
#define SYS_CTRL_AP_RESERVE2(n)     (((n)&0x3FFF)<<18)
#define SYS_CTRL_AP_RESERVE2_MASK   (0x3FFF<<18)
#define SYS_CTRL_AP_RESERVE2_SHIFT  (18)

//Memory_Margin
#define SYS_CTRL_AP_EMAW_VOC(n)     (((n)&3)<<0)
#define SYS_CTRL_AP_EMAW_VOC_MASK   (3<<0)
#define SYS_CTRL_AP_EMAW_VOC_SHIFT  (0)
#define SYS_CTRL_AP_EMA_VOC(n)      (((n)&7)<<2)
#define SYS_CTRL_AP_EMA_VOC_MASK    (7<<2)
#define SYS_CTRL_AP_EMA_VOC_SHIFT   (2)
#define SYS_CTRL_AP_EMAW_VPU(n)     (((n)&3)<<5)
#define SYS_CTRL_AP_EMAW_VPU_MASK   (3<<5)
#define SYS_CTRL_AP_EMAW_VPU_SHIFT  (5)
#define SYS_CTRL_AP_EMA_VPU(n)      (((n)&7)<<7)
#define SYS_CTRL_AP_EMA_VPU_MASK    (7<<7)
#define SYS_CTRL_AP_EMA_VPU_SHIFT   (7)
#define SYS_CTRL_AP_EMAW_GPU(n)     (((n)&3)<<10)
#define SYS_CTRL_AP_EMAW_GPU_MASK   (3<<10)
#define SYS_CTRL_AP_EMAW_GPU_SHIFT  (10)
#define SYS_CTRL_AP_EMA_GPU(n)      (((n)&7)<<12)
#define SYS_CTRL_AP_EMA_GPU_MASK    (7<<12)
#define SYS_CTRL_AP_EMA_GPU_SHIFT   (12)
#define SYS_CTRL_AP_EMAW_GOUDA(n)   (((n)&3)<<15)
#define SYS_CTRL_AP_EMAW_GOUDA_MASK (3<<15)
#define SYS_CTRL_AP_EMAW_GOUDA_SHIFT (15)
#define SYS_CTRL_AP_EMA_GOUDA(n)    (((n)&7)<<17)
#define SYS_CTRL_AP_EMA_GOUDA_MASK  (7<<17)
#define SYS_CTRL_AP_EMA_GOUDA_SHIFT (17)
#define SYS_CTRL_AP_EMAW_IMEM(n)    (((n)&3)<<20)
#define SYS_CTRL_AP_EMAW_IMEM_MASK  (3<<20)
#define SYS_CTRL_AP_EMAW_IMEM_SHIFT (20)
#define SYS_CTRL_AP_EMA_IMEM(n)     (((n)&7)<<22)
#define SYS_CTRL_AP_EMA_IMEM_MASK   (7<<22)
#define SYS_CTRL_AP_EMA_IMEM_SHIFT  (22)
#define SYS_CTRL_AP_EMAW_USB(n)     (((n)&3)<<25)
#define SYS_CTRL_AP_EMAW_USB_MASK   (3<<25)
#define SYS_CTRL_AP_EMAW_USB_SHIFT  (25)
#define SYS_CTRL_AP_EMA_USB(n)      (((n)&7)<<27)
#define SYS_CTRL_AP_EMA_USB_MASK    (7<<27)
#define SYS_CTRL_AP_EMA_USB_SHIFT   (27)

//Memory_Margin2
#define SYS_CTRL_AP_EMAW_NFSC(n)    (((n)&3)<<0)
#define SYS_CTRL_AP_EMAW_NFSC_MASK  (3<<0)
#define SYS_CTRL_AP_EMAW_NFSC_SHIFT (0)
#define SYS_CTRL_AP_EMA_NFSC(n)     (((n)&7)<<2)
#define SYS_CTRL_AP_EMA_NFSC_MASK   (7<<2)
#define SYS_CTRL_AP_EMA_NFSC_SHIFT  (2)
#define SYS_CTRL_AP_EMA_VPUROM(n)   (((n)&7)<<5)
#define SYS_CTRL_AP_EMA_VPUROM_MASK (7<<5)
#define SYS_CTRL_AP_EMA_VPUROM_SHIFT (5)
#define SYS_CTRL_AP_EMA_IMEMROM(n)  (((n)&7)<<8)
#define SYS_CTRL_AP_EMA_IMEMROM_MASK (7<<8)
#define SYS_CTRL_AP_EMA_IMEMROM_SHIFT (8)
#define SYS_CTRL_AP_KEN_VPUROM      (1<<11)
#define SYS_CTRL_AP_PGEN_VPUROM     (1<<12)
#define SYS_CTRL_AP_KEN_IMEMROM     (1<<13)
#define SYS_CTRL_AP_PGEN_IMEMROM    (1<<14)

//Memory_Observe
#define SYS_CTRL_AP_MEM_OBSERVE(n)  (((n)&0xFFFFFFFF)<<0)

//Cfg_Reserve
#define SYS_CTRL_AP_RESERVE(n)      (((n)&0xFFFFFFFF)<<0)

