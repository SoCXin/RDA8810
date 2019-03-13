#
# Makefile fragment for Rdamicro 802.11 Networking Device Driver
#
# Copyright (c) 2014 Rdamicro Corporation
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

ccflags-y += \
	-Idrivers/net/wireless/rdaw80211/rdawlan\
	-Idrivers/net/wireless/rdaw80211/include

ccflags-y += -D__CHECK_ENDIAN__

obj-$(CONFIG_RDAWFMAC) += rdawfmac.o
rdawfmac-objs += \
		linux_osl.o \
		wland_iw.o \
		wland_p2p.o \
		wland_dbg.o \
		wland_d11.o \
		wland_btcoex.o \
		wland_nvram.o \
		wland_utils.o \
		wland_trap.o \
		wland_bus.o \
		wland_cmds.o \
		wland_fwsmgr.o \
		wland_wid.o \
 		wland_fweh.o \
		wland_linux.o \
		wland_cfg80211.o \
		wland_linux_mon.o \
		wland_android.o 	

rdawfmac-$(CONFIG_RDAWFMAC_SDIO) += \
 		wland_sdio.o \
 		wland_sdmmc.o 
rdawfmac-$(CONFIG_RDAWFMAC_USB) += \
 		wland_usb.o
rdawfmac-$(CONFIG_RDAWFMAC_BT) += \
 		wland_bta.o 

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.*


