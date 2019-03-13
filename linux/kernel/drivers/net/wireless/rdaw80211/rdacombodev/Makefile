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

subdir-ccflags-y := -Werror

MODULEPFX := rdacombodev

obj-$(CONFIG_RDACBDEV) := $(MODULEPFX).o

$(MODULEPFX)-objs := \
	rda_combo_power_main.o \
	rda_5990_power_ctrl.o  \
	rda_5991_power_ctrl.o  \
	rda_5991e_power_ctrl.o \
	rda_5991f_power_ctrl.o \
	rda_5991g_power_ctrl.o
