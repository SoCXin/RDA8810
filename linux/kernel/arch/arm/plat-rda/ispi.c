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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <plat/reg_spi.h>

static HWP_SPI_T *hwp_ispi = hwp_spi3;
static spinlock_t ispi_lock;

void ispi_open(void)
{
	u32 cfgReg = 0;
	u32 ctrlReg = 0;

	// hard code for now
	cfgReg = 0x180003;
	ctrlReg = 0x2019d821;
/*
	if (modemSpi)
		hwp_ispi = hwp_mspi2;
	else
*/
	hwp_ispi = hwp_spi3;

	// Activate the ISPI.
	hwp_ispi->cfg = cfgReg;
	hwp_ispi->ctrl = ctrlReg;

	// No IRQ.
	hwp_ispi->irq = 0;
	spin_lock_init(&ispi_lock);
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
		return true;
	} else {
		return false;
	}
}

static u32 ispi_send_data(u32 csId, u32 data, int read)
{
	u32 freeRoom;
	unsigned long flags;

	// Clear data upper bit to only keep the data frame.
	u32 reg = data & ~(SPI_CS_MASK | SPI_READ_ENA_MASK);

	// Add CS and read mode bit
	reg |= SPI_CS(csId) | (read ? SPI_READ_ENA : 0);

	// Enter critical section.
	//u32 status = hwp_sysIrq->SC;
	spin_lock_irqsave(&ispi_lock, flags);

	// Check FIFO availability.
	freeRoom = GET_BITFIELD(hwp_ispi->status, SPI_TX_SPACE);

	if (freeRoom > 0) {
		// Write data.
		hwp_ispi->rxtx_buffer = reg;

		// Exit critical section.
		//hwp_sysIrq->SC = status;
		spin_unlock_irqrestore(&ispi_lock, flags);
		return 1;
	} else {
		// Exit critical section.
		//hwp_sysIrq->SC = status;
		spin_unlock_irqrestore(&ispi_lock, flags);
		return 0;
	}
}

static u32 ispi_get_data(u32 * recData)
{
	u32 nbAvailable;
	unsigned long flags;

	// Enter critical section.
	//u32 status = hwp_sysIrq->SC;
	spin_lock_irqsave(&ispi_lock, flags);

	nbAvailable = GET_BITFIELD(hwp_ispi->status, SPI_RX_LEVEL);

	if (nbAvailable > 0) {
		*recData = hwp_ispi->rxtx_buffer;

		// Exit critical section.
		//hwp_sysIrq->SC = status;
		spin_unlock_irqrestore(&ispi_lock, flags);
		return 1;
	} else {
		// Exit critical section.
		//hwp_sysIrq->SC = status;
		spin_unlock_irqrestore(&ispi_lock, flags);
		return 0;
	}
}

void ispi_reg_write(u32 regIdx, u32 value)
{
	u32 wrData;

	wrData = (0 << 25) | ((regIdx & 0x1ff) << 16) | (value & 0xffff);

	while (ispi_tx_fifo_avail() < 1 ||
	       ispi_send_data(0, wrData, false) == 0) ;

	//wait until any previous transfers have ended
	while (!ispi_tx_finished()) ;
}

u32 ispi_reg_read(u32 regIdx)
{
	u32 wrData, rdData = 0;
	u32 count;

	wrData = (1 << 25) | ((regIdx & 0x1ff) << 16) | 0;

	while (ispi_tx_fifo_avail() < 1 ||
	       ispi_send_data(0, wrData, true) == 0) ;

	//wait until any previous transfers have ended
	while (!ispi_tx_finished()) ;

	count = ispi_get_data(&rdData);
	if (1 != count)
		pr_err("ABB ISPI count err!");

	rdData &= 0xffff;

	return rdData;
}
