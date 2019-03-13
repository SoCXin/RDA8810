#ifndef __IOMAP_RDA8820_H
#define __IOMAP_RDA8820_H

#define RDA_SRAM_BASE         0x00100000
#define RDA_SRAM_SIZE         SZ_64K

#define RDA_MD_MAILBOX_BASE   0x00200000
#define RDA_MD_MAILBOX_SIZE   SZ_8K

#define RDA_MODEM_BASE        0x10000000
#define RDA_MODEM_SIZE        SZ_256M

#define RDA_MD_SYSCTRL_BASE   0x11A00000
#define RDA_MD_SYSCTRL_SIZE   SZ_4K

#define RDA_GPIO_BASE         0x11A08000
#define RDA_GPIO_SIZE         SZ_4K

#define RDA_CFG_REGS_BASE     0x11A09000
#define RDA_CFG_REGS_SIZE     SZ_4K

#define RDA_MODEM_SPI2_BASE   0x11A14000
#define RDA_MODEM_SPI2_SIZE   SZ_4K

#define RDA_MODEM_XCPU_BASE   0x11A17000
#define RDA_MODEM_XCPU_SIZE   SZ_4K

#define RDA_MODEM_BCPU_BASE   0x11909000
#define RDA_MODEM_BCPU_SIZE   SZ_4K

#define RDA_MD_PSRAM_BASE     (RDA_MODEM_BASE + 0x02000000)
#define RDA_MD_PSRAM_SIZE     SZ_16M

#define RDA_CAMERA_BASE       0x20000000
#define RDA_CAMERA_SIZE       SZ_256K

#define RDA_GOUDA_MEM_BASE    0x20040000
#define RDA_GOUDA_MEM_SIZE    SZ_256K

#define RDA_GPU_BASE          0x20080000
#define RDA_GPU_SIZE          SZ_256K

#define RDA_VOC_BASE          0x200C0000
#define RDA_VOC_SIZE          SZ_256K

#define RDA_VOC2_BASE         0x20100000
#define RDA_VOC2_SIZE         SZ_256K

#define RDA_USB_BASE          0x20400000
#define RDA_USB_SIZE          SZ_256K

#define RDA_SPIFLASH_BASE     0x20440000
#define RDA_SPIFLASH_SIZE     SZ_256K

#define RDA_CONNECT_BASE      0x21000000
#define RDA_CONNECT_SIZE      SZ_4K

#define RDA_L2CC_BASE         0x21100000
#define RDA_L2CC_SIZE         SZ_4K

/* APB0 */
#define RDA_INTC_BASE         0x20800000
#define RDA_INTC_SIZE         SZ_4K

#define RDA_IMEM_BASE         0x20810000
#define RDA_IMEM_SIZE         SZ_4K

#define RDA_DMA_BASE          0x20820000
#define RDA_DMA_SIZE          SZ_4K

#define RDA_VPU_BASE          0x20830000
#define RDA_VPU_SIZE          SZ_64K

#define RDA_GOUDA_BASE        0x20840000
#define RDA_GOUDA_SIZE        SZ_4K

#define RDA_CAMERA_DMA_BASE   0x20850000
#define RDA_CAMERA_DMA_SIZE   SZ_4K

#define RDA_CAMERA2_DMA_BASE  0x20860000
#define RDA_CAMERA2_DMA_SIZE  SZ_64K

#define RDA_GPU_BASE          0x20870000
#define RDA_GPU_SIZE          SZ_64K

#define RDA_AES_BASE          0x20880000
#define RDA_AES_SIZE          SZ_64K

#define RDA_JPEG_BASE         0x20890000
#define RDA_JPEG_SIZE         SZ_64K

/* APB1 */
#define RDA_SYSCTRL_BASE      0x20900000
#define RDA_SYSCTRL_SIZE      SZ_4K

#define RDA_TIMER_BASE        0x20910000
#define RDA_TIMER_SIZE        SZ_4K

#define RDA_KEYPAD_BASE       0x20920000
#define RDA_KEYPAD_SIZE       SZ_4K

#define RDA_GPIO_A_BASE       0x20930000
#define RDA_GPIO_A_SIZE       SZ_4K

#define RDA_GPIO_B_BASE       0x20931000
#define RDA_GPIO_B_SIZE       SZ_4K

#define RDA_GPIO_D_BASE       0x20932000
#define RDA_GPIO_D_SIZE       SZ_4K

#define RDA_GPIO_E_BASE       0x20933000
#define RDA_GPIO_E_SIZE       SZ_4K

#define RDA_PWM_BASE          0x20940000
#define RDA_PWM_SIZE          SZ_4K

#define RDA_LVDS_BASE         0x20950000
#define RDA_LVDS_SIZE         SZ_4K

#define RDA_TVCLK_BASE        0x20960000
#define RDA_TVCLK_SIZE        SZ_4K

#define RDA_GIC400_BASE       0x20970000
#define RDA_GIC400_SIZE       SZ_4K

#define RDA_COMREGS_BASE      0x20980000
#define RDA_COMREGS_SIZE      SZ_4K

#define RDA_DMC400_BASE       0x20990000
#define RDA_DMC400_SIZE       SZ_4K

#define RDA_DDRPHY_BASE       0x209A0000
#define RDA_DDRPHY_SIZE       SZ_4K

#define RDA_PDCTRL_BASE       0x209B0000
#define RDA_PDCTRL_SIZE       SZ_4K

#define RDA_ISP_BASE          0x209C0000
#define RDA_ISP_SIZE          SZ_4K

#define RDA_AIF2_BASE         0x209D0000
#define RDA_AIF2_SIZE         SZ_4K

#define RDA_AIF_BASE          0x209E0000
#define RDA_AIF_SIZE          SZ_4K

#define RDA_AUIFC_BASE        0x209F0000
#define RDA_AUIFC_SIZE        SZ_4K

/* APB2 */
#define RDA_UART1_BASE        0x20A00000
#define RDA_UART1_SIZE        SZ_4K

#define RDA_UART2_BASE        0x20A10000
#define RDA_UART2_SIZE        SZ_4K

#define RDA_SPI1_BASE         0x20A20000
#define RDA_SPI1_SIZE         SZ_4K

#define RDA_SPI2_BASE         0x20A30000
#define RDA_SPI2_SIZE         SZ_4K

#define RDA_SPI3_BASE         0x20A40000
#define RDA_SPI3_SIZE         SZ_4K

#define RDA_SDMMC1_BASE       0x20A50000
#define RDA_SDMMC1_SIZE       SZ_4K

#define RDA_SDMMC2_BASE       0x20A60000
#define RDA_SDMMC2_SIZE       SZ_4K

#define RDA_SDMMC3_BASE       0x20A70000
#define RDA_SDMMC3_SIZE       SZ_4K

#define RDA_NAND_BASE         0x20A80000
#define RDA_NAND_SIZE         SZ_16K

#define RDA_UART3_BASE        0x20A90000
#define RDA_UART3_SIZE        SZ_16K

#define RDA_I2C1_BASE         0x20AB0000
#define RDA_I2C1_SIZE         SZ_4K

#define RDA_I2C2_BASE         0x20AC0000
#define RDA_I2C2_SIZE         SZ_4K

#define RDA_I2C3_BASE         0x20AD0000
#define RDA_I2C3_SIZE         SZ_4K

#define RDA_UART4_BASE        0x20AE0000
#define RDA_UART4_SIZE        SZ_16K

#define RDA_IFC_BASE          0x20AF0000
#define RDA_IFC_SIZE          SZ_4K

#endif /* __IOMAP_RDA8820_H */
