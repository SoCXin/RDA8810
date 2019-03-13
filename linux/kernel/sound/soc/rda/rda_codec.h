/*
 * ALSA SoC RDA codec driver
 *
 * Author:      Arun KS, <arunks@mistralsolutions.com>
 * Copyright:   (C) 2008 Mistral Solutions Pvt Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _RDA_CODEC_H
#define _RDA_CODEC_H

///////////////////////// abb spi /////////////////////////
///////////////////////////////////////////////////////////

// =============================================================================
// BOOT_ISPI_DELAY_T
// -----------------------------------------------------------------------------
/// Delays
/// Used to define the configuration delays
// =============================================================================
typedef enum {
	/// Delay of 0 half-period
	BOOT_ISPI_HALF_CLK_PERIOD_0,
	/// Delay of 1 half-period
	BOOT_ISPI_HALF_CLK_PERIOD_1,
	/// Delay of 2 half-period
	BOOT_ISPI_HALF_CLK_PERIOD_2,
	/// Delay of 3 half-period
	BOOT_ISPI_HALF_CLK_PERIOD_3,

	BOOT_ISPI_HALF_CLK_PERIOD_QTY
} BOOT_ISPI_DELAY_T;

// =============================================================================
// BOOT_ISPI_CFG_T
// -----------------------------------------------------------------------------
/// Structure for configuration. 
/// A configuration structure allows to open or change the SPI with the desired 
/// parameters.
// =============================================================================
typedef struct {
	/// Polarity of the FM CS.
	u8 csFmActiveLow;

	/// Polarity of the ABB CS.
	u8 csAbbActiveLow;

	/// Polarity of the PMU CS.
	u8 csPmuActiveLow;

	/// If the first edge after the CS activation is a falling edge, set to 
	/// \c 1.\n Otherwise, set to \c 0.
	u8 clkFallEdge;

	/// The delay between the CS activation and the first clock edge,
	/// can be 0 to 2 half clocks.
	BOOT_ISPI_DELAY_T clkDelay;

	/// The delay between the CS activation and the output of the data, 
	/// can be 0 to 2 half clocks.
	BOOT_ISPI_DELAY_T doDelay;

	/// The delay between the CS activation and the sampling of the input data,
	/// can be 0 to 3 half clocks.
	BOOT_ISPI_DELAY_T diDelay;

	/// The delay between the end of transfer and the CS deactivation, can be 
	/// 0 to 3 half clocks.
	BOOT_ISPI_DELAY_T csDelay;

	/// The time when the CS must remain deactivated before a new transfer, 
	/// can be 0 to 3 half clocks.
	BOOT_ISPI_DELAY_T csPulse;

	/// Frame size in bits
	u32 frameSize;

	/// OE ratio - Value from 0 to 31 is the number of data out to transfert 
	/// before the SPI_DO pin switches to input. 
	/// Not needed in the chip, but needed for the FPGA
	u8 oeRatio;

	/// SPI maximum clock frequency: the SPI clock will be the highest
	/// possible value inferior to this parameter.
	u32 spiFreq;

} BOOT_ISPI_CFG_T;

// =============================================================================
// BOOT_ISPI_CS_T
// -----------------------------------------------------------------------------
/// Chip Select
/// Used to select a Chip Select
// =============================================================================
typedef enum {
	/// Chip Select for the PMU analog module.
	BOOT_ISPI_CS_PMU = 0,
	/// Chip Select for the ABB analog module.
	BOOT_ISPI_CS_ABB,
	/// Chip Select for the FM analog module.
	BOOT_ISPI_CS_FM,

	BOOT_ISPI_CS_QTY
} BOOT_ISPI_CS_T;

// ==============================================================================
// Codec registers
// ==============================================================================

// reg 0x12d - s_cnt_constant
#define RDA_CODEC_REG_SAMPLE_RATE 0x12D
#define RDA_CODEC_REG_SAMPLE_RATE_SHIFT (6)
#define RDA_CODEC_REG_SAMPLE_RATE_MASK (0x1f<<(RDA_CODEC_REG_SAMPLE_RATE_SHIFT))

#define RDA_CODEC_REG_DIG_MUTE 0x12A
#define RDA_CODEC_REG_DIG_MUTE_SHIFT (7)
#define RDA_CODEC_REG_DIG_MUTE_MASK (0x1<<(RDA_CODEC_REG_DIG_EN_SHIFT))

#define RDA_CODEC_REG_DIG_EN 0x12A
#define RDA_CODEC_REG_DIG_EN_SHIFT (2)
#define RDA_CODEC_REG_DIG_EN_MASK (0x1<<(RDA_CODEC_REG_DIG_EN_SHIFT))

#define RDA_CODEC_REG_SELECT_SPK_PA 0x42
#define RDA_CODEC_REG_SELECT_SPK_PA_SHIFT (2)
#define RDA_CODEC_REG_SELECT_SPK_PA_MASK (0x1<<(RDA_CODEC_REG_SELECT_SPK_PA_SHIFT))

#define RDA_CODEC_REG_SELECT_HEAD_PA 0x42
#define RDA_CODEC_REG_SELECT_HEAD_PA_SHIFT (1)
#define RDA_CODEC_REG_SELECT_HEAD_PA_MASK (0x1<<(RDA_CODEC_REG_SELECT_HEAD_PA_SHIFT))

#define RDA_CODEC_REG_SELECT_RECV_PA 0x42
#define RDA_CODEC_REG_SELECT_RECV_PA_SHIFT (1)
#define RDA_CODEC_REG_SELECT_RECV_PA_MASK (0x1<<(RDA_CODEC_REG_SELECT_RECV_PA_SHIFT))

#endif /* _RDA_CODEC_H */
