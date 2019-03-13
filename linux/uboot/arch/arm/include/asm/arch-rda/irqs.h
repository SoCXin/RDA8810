/* include/asm-arm/arch-goldfish/irqs.h
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

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

#define RDA_IRQ_NUM            32

#define RDA_IRQ_MASK_ALL       0xFFFFFFFF

#define NR_IRQS         RDA_IRQ_NUM

#endif
