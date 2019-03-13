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

LIB	= $(obj)lib$(BOARD).o

ifdef CONFIG_SPL_BUILD
COBJS-y	+= clock_config.o clock.o ddr3.o
else
COBJS-$(CONFIG_MACH_RDA8810H) += hal_config.o
endif

RDA_TARGET_DEF_FILE := $(ANDROID_BUILD_TOP)/$(RDA_TARGET_DEVICE_DIR)/target.def
RDA_GET_MAKEFILE_VAR_FUNC = $(strip $(shell sed -n 's/^[ 	]*$(1)[ 	]*[?:]\?=\([^\#]\+\)\#*.*$$/\1/p' $(2)))

NUMBER_OF_SIM := $(call RDA_GET_MAKEFILE_VAR_FUNC,NUMBER_OF_SIM,$(RDA_TARGET_DEF_FILE))
ifeq ($(NUMBER_OF_SIM),)
$(error Failed to get NUMBER_OF_SIM in $(RDA_TARGET_DEF_FILE))
endif

USER_SIM_ORDER := $(call RDA_GET_MAKEFILE_VAR_FUNC,USER_SIM_ORDER,$(RDA_TARGET_DEF_FILE))
ifeq ($(USER_SIM_ORDER),)
$(error Failed to get USER_SIM_ORDER in $(RDA_TARGET_DEF_FILE))
endif

NULL_STRING :=
SPACE := ${NULL_STRING} # end of the line
COMMA := ,
TGT_SIM_ORDER := $(foreach SIM,$(USER_SIM_ORDER),$(SIM)$(COMMA))
TGT_SIM_ORDER := $(subst ${SPACE},,${TGT_SIM_ORDER})

CPPFLAGS_board/$(BOARDDIR)/hal_config.o += -I$(TOPDIR)/board/$(VENDOR)/common/include -DTGT_SIM_ORDER=$(TGT_SIM_ORDER) -DNUMBER_OF_SIM=$(NUMBER_OF_SIM)

ifeq ($(strip $(TARGET_TABLET_MODE)),true)
CPPFLAGS_board/$(BOARDDIR)/clock.o += -DTARGET_TABLET_MODE
endif

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
