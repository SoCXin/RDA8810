/*
 * (C) Copyright 2003
 * Gerry Hamel, geh@ti.com, Texas Instruments
 *
 * (C) Copyright 2006
 * Bryan O'Donoghue, bodonoghue@codehermit.ie, CodeHermit
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#ifndef __USB_SERIAL_H__
#define __USB_SERIAL_H__

#define USB_ACM_CHAN_0	0
#define USB_ACM_CHAN_1	1

int usbser_tstc (int chan);
int usbser_getc (int chan);
void usbser_putc (int chan, const char c);
int usbser_read(int chan, unsigned char *_buf, unsigned int len);
int usbser_write(int chan, const unsigned char *_buf, unsigned int len);
int drv_usbser_init (void);

#endif
