#ifndef __ASM_ARCH_IRQS_RDA8810_H
#define __ASM_ARCH_IRQS_RDA8810_H

#define RDA_IRQ_PULSE_DUMMY    0
#define RDA_IRQ_I2C            1
#define RDA_IRQ_NAND_NFSC      2
#define RDA_IRQ_SDMMC1         3
#define RDA_IRQ_SDMMC2         4
#define RDA_IRQ_SDMMC3         5
#define RDA_IRQ_SPI1           6
#define RDA_IRQ_SPI2           7
#define RDA_IRQ_SPI3           8
#define RDA_IRQ_UART1          9
#define RDA_IRQ_UART2          10
#define RDA_IRQ_UART3          11
#define RDA_IRQ_GPIO1          12
#define RDA_IRQ_GPIO2          13
#define RDA_IRQ_GPIO3          14
#define RDA_IRQ_KEYPAD         15
#define RDA_IRQ_TIMER          16
#define RDA_IRQ_TIMEROS        17
#define RDA_IRQ_COMREG0        18
#define RDA_IRQ_COMREG1        19
#define RDA_IRQ_USB            20
#define RDA_IRQ_DMC            21
#define RDA_IRQ_DMA            22
#define RDA_IRQ_CAMERA         23
#define RDA_IRQ_GOUDA          24
#define RDA_IRQ_GPU            25
#define RDA_IRQ_VPU_JPG        26
#define RDA_IRQ_VPU_HOST       27
#define RDA_IRQ_VOC            28
#define RDA_IRQ_AUIFC0         29
#define RDA_IRQ_AUIFC1         30
#define RDA_IRQ_L2CC           31

#define NR_RDA_IRQS            32
#define RDA_IRQ_MASK_ALL       0xFFFFFFFF

/*
 * for every gpio bank, only 8 gpio pins can be input interrupt line
 */
#define RDA_GPIO_IRQ_START	(NR_RDA_IRQS)
#define	NR_GPIO_BANK_IRQS	8
#define NR_GPIO_IRQS		(NR_GPIO_BANK_IRQS * 3)
#define GPIO_IRQ_MASK		0xff //(bit0~7)

#define NR_IRQS               (NR_RDA_IRQS + NR_GPIO_IRQS)

#endif /* __ASM_ARCH_IRQS_RDA8810_H */
