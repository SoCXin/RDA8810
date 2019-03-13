#
# (C) Copyright 2000, 2001, 2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(VENDOR).o

COBJS-y	+= misc.o board.o
ifndef CONFIG_SPL_BUILD
COBJS-$(CONFIG_VPU_TEST) += vpu_test.o rda_md5.o
COBJS-$(CONFIG_CPU_TEST) += cpu_test.o
COBJS-$(CONFIG_DDR_TEST) += ddr_test.o
COBJS-$(CONFIG_TIMER_TEST) += timer_test.o
COBJS-$(CONFIG_UART_TEST) += uart_test.o
COBJS-$(CONFIG_I2C_TEST) += i2c_test.o
COBJS-$(CONFIG_GIC_TEST) += gic_test.o
COBJS-$(CONFIG_CACHE_TEST) += a7_cp15_reg.o		\
				cache_test.o
COBJS-$(CONFIG_MIPI_LOOP_TEST) += rda_lcdc.o		\
				rda_mipi_dsi.o		\
			        hal_camera.o 		\
			        mipi_loop_test.o

endif

CPPFLAGS_board/$(VENDOR)/common/vpu_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/rda_md5.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/cpu_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/ddr_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/timer_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/uart_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/i2c_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/gic_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/cache_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include

CPPFLAGS_board/$(VENDOR)/common/rda_lcdc.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/rda_mipi_dsi.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/hal_camera.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
CPPFLAGS_board/$(VENDOR)/common/mipi_loop_test.o += -I$(TOPDIR)/board/$(VENDOR)/common/include
COBJS   := $(COBJS-y)

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak *~ .depend

#########################################################################
# This is for $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
