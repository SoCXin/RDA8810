/*
 * (C) Copyright 2013
 * RDA Microelectronics Inc.
 *
 * Derived from drivers/spi/rda_ispi.c
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */
#include <common.h>
#include <linux/types.h>
#include <asm/arch/reg_spi.h>
#include <asm/arch/ispi.h>

#ifdef CONFIG_MACH_RDA8850E 
#include <asm/arch/reg_rf_spi.h>

#define RDA6220_ADDR_MASK               0xFF
#define RDA6220_ADDR_OFFSET             23
#define RDA6220_DATA_MASK               0xFFFF
#define RDA6220_DATA_OFFSET             7

#define RDA6220_WRITE_FMT(addr, data) (((addr & RDA6220_ADDR_MASK) << RDA6220_ADDR_OFFSET) |\
        ((data & RDA6220_DATA_MASK) << RDA6220_DATA_OFFSET))

#define RDA6220_READ_FMT(addr, data)  ( ((addr & RDA6220_ADDR_MASK) << RDA6220_ADDR_OFFSET)|\
        ((data & RDA6220_DATA_MASK) << RDA6220_DATA_OFFSET)|\
        1<<31);

void modem_RfspiOpen(void)
{
	//  Setter
	hwp_rfSpi->Ctrl = 0x8fd82e45;

	// To remove the "FORCE ZERO" in case of low-active CS
	hwp_rfSpi->Command = 0;
	hwp_rfSpi->Divider = RF_SPI_DIVIDER(23);
}

void modem_rf_spiWrite(const u8 *Cmd, u32 CmdSize)
{
	u32 loop=0;

	// Flush the Tx fifo
	hwp_rfSpi->Command = RF_SPI_FLUSH_CMD_FIFO | RF_SPI_FLUSH_RX_FIFO;
	for (loop=0; loop<CmdSize; loop++)
	{
	    hwp_rfSpi->Cmd_Data = Cmd[loop];
	}
	// Set the cmd size
	hwp_rfSpi->Cmd_Size = RF_SPI_CMD_SIZE(CmdSize);

	// And send the command
	hwp_rfSpi->Command = RF_SPI_SEND_CMD;

	// Wait for the SPI to start - at least one byte has been sent
	while(GET_BITFIELD(hwp_rfSpi->Status, RF_SPI_CMD_DATA_LEVEL) >= CmdSize);
	// Wait for the SPI to finish
	while((hwp_rfSpi->Status & RF_SPI_ACTIVE_STATUS) != 0);
}


void modem_RfspiWrite(const u8 addr, u32 data)
{
	u8 cmd[4] = {0};
	u32 cmd_word = RDA6220_WRITE_FMT(addr, data);

	cmd[0] = (cmd_word >> 24) & 0xff;
	cmd[1] = (cmd_word >>16) & 0xff;
	cmd[2] = (cmd_word >> 8) & 0xff;
	cmd[3] = (cmd_word >> 0) & 0xff;

	modem_rf_spiWrite(cmd, 4);
}

void modem_RfspiInit_624M(void)
{
	modem_RfspiOpen();

	modem_RfspiWrite(0x30, 0x5182);

	mdelay(10);  // delay 10 ms
	modem_RfspiWrite(0x30, 0x5187);
	mdelay(10);  // delay 10 ms
	//  xtal setting
	modem_RfspiWrite(0xb0, 0xe404);
	modem_RfspiWrite(0xb8, 0x1800);
	modem_RfspiWrite(0xba, 0x0401);
	modem_RfspiWrite(0xc0, 0x0008);
	modem_RfspiWrite(0xfe, 0x4000);
	modem_RfspiWrite(0x10, 0x0880);
	modem_RfspiWrite(0x20, 0x03e4);
	modem_RfspiWrite(0x22, 0x0488);
	modem_RfspiWrite(0xe8, 0x1000);
	modem_RfspiWrite(0xe6, 0x0000);
	modem_RfspiWrite(0xe4, 0x0c00);
	modem_RfspiWrite(0xe4, 0x8c00);
	modem_RfspiWrite(0xfe, 0x0000);
	modem_RfspiWrite(0x4e, 0xe538);
	modem_RfspiWrite(0x4c, 0x04b9);
	modem_RfspiWrite(0x4c, 0x84b9);

}

void modem_8850eeco2_RfspiInit_624M(void)
{
	modem_RfspiOpen();

	modem_RfspiWrite(0x30, 0x5182);

	mdelay(10);  // delay 10 ms
	modem_RfspiWrite(0x30, 0x5187);
	mdelay(10);  // delay 10 ms
	//  xtal setting

	modem_RfspiWrite(0xfe, 0x4000);
	modem_RfspiWrite(0x10, 0x0880);
	modem_RfspiWrite(0xfe, 0x0000);
}
#endif

static HWP_SPI_T *hwp_ispi = hwp_spi3;

void ispi_open(int modemSpi)
{
	u32 cfgReg = 0;
	u32 ctrlReg = 0;

	// spi_clk_freq = APB2 / ((div+1)*2)
	// the maximum clock frequency is 7MHz~8MHz
	// the spi clock frequency is 5MHz when div is 0x13
	cfgReg = 0x130003;
	ctrlReg = 0x2019d821;

	if (modemSpi)
		hwp_ispi = hwp_mspi2;
	else
		hwp_ispi = hwp_spi3;

	// Activate the ISPI.
	hwp_ispi->cfg = cfgReg;
	hwp_ispi->ctrl = ctrlReg;

	// No IRQ.
	hwp_ispi->irq = 0;
}

static u8 ispi_tx_fifo_avail(void)
{
	u8 freeRoom;

	// Get avail level.
	freeRoom = GET_BITFIELD(hwp_ispi->status, SPI_TX_SPACE);

	return freeRoom;
}

static int ispi_tx_finished(void)
{
	u32 spiStatus;
	spiStatus = hwp_ispi->status;

	// If ISPI FSM is active and the TX Fifo is empty
	// (ie available space == Fifo size), the tf is not done
	if ((!(hwp_ispi->status & SPI_ACTIVE_STATUS))
	    && (SPI_TX_FIFO_SIZE == GET_BITFIELD(spiStatus, SPI_TX_SPACE))) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static u32 ispi_send_data(u32 csId, u32 data, int read)
{
	u32 freeRoom;

	// Clear data upper bit to only keep the data frame.
	u32 reg = data & ~(SPI_CS_MASK | SPI_READ_ENA_MASK);

	// Add CS and read mode bit
	reg |= SPI_CS(csId) | (read ? SPI_READ_ENA : 0);

	// Enter critical section.
	//u32 status = hwp_sysIrq->SC;

	// Check FIFO availability.
	freeRoom = GET_BITFIELD(hwp_ispi->status, SPI_TX_SPACE);

	if (freeRoom > 0) {
		// Write data.
		hwp_ispi->rxtx_buffer = reg;

		// Exit critical section.
		//hwp_sysIrq->SC = status;
		return 1;
	} else {
		// Exit critical section.
		//hwp_sysIrq->SC = status;
		return 0;
	}
}

static u32 ispi_get_data(u32 * recData)
{
	u32 nbAvailable;

	// Enter critical section.
	//u32 status = hwp_sysIrq->SC;

	nbAvailable = GET_BITFIELD(hwp_ispi->status, SPI_RX_LEVEL);

	if (nbAvailable > 0) {
		*recData = hwp_ispi->rxtx_buffer;

		// Exit critical section.
		//hwp_sysIrq->SC = status;
		return 1;
	} else {
		// Exit critical section.
		//hwp_sysIrq->SC = status;
		return 0;
	}
}

void ispi_reg_write(u32 regIdx, u32 value)
{
	u32 wrData;

	wrData = (0 << 25) | ((regIdx & 0x1ff) << 16) | (value & 0xffff);

	while (ispi_tx_fifo_avail() < 1 ||
	       ispi_send_data(0, wrData, FALSE) == 0) ;

	//wait until any previous transfers have ended
	while (!ispi_tx_finished()) ;
}

u32 ispi_reg_read(u32 regIdx)
{
	u32 wrData, rdData = 0;
	u32 count;

	wrData = (1 << 25) | ((regIdx & 0x1ff) << 16) | 0;

	while (ispi_tx_fifo_avail() < 1 ||
	       ispi_send_data(0, wrData, TRUE) == 0) ;

	//wait until any previous transfers have ended
	while (!ispi_tx_finished()) ;

	count = ispi_get_data(&rdData);
	if (1 != count)
		serial_puts("ABB ISPI count err!");

	rdData &= 0xffff;

	return rdData;
}

u16 rda_read_efuse(int page_index)
{
	u16 rvalue, wvalue;

	ispi_open(1);

	wvalue = 0x2e0 | page_index;
	pmu_reg_write(0x51, wvalue);
	udelay(2000);
	wvalue = 0x2f0 | page_index;
	pmu_reg_write(0x51, wvalue);
	rvalue = pmu_reg_read(0x52);
	pmu_reg_write(0x51, 0x0200);

	ispi_open(0);
	return rvalue;
}

#ifdef EFUSE_DUMP
void dump_efuse(void)
{
	u16 rvalue,wvalue;
	int i;

	ispi_open(1);

	for(i = 0;i<16;i++){
		wvalue = 0x2e0 | i;
		pmu_reg_write(0x51, wvalue);
		udelay(2000);
		wvalue = 0x2f0 | i;
		pmu_reg_write(0x51, wvalue);
		rvalue = pmu_reg_read(0x52);
		pmu_reg_write(0x51, 0x0200);

		printf("page%d = %d \n", i, rvalue);
	}
	ispi_open(0);
}
#endif
