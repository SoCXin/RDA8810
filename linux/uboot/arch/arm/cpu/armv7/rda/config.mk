#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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
include $(TOPDIR)/slt.mk
include $(TOPDIR)/customer.mk
include $(TOPDIR)/board/$(SOC)/$(BOARD)/config.mk

image_flags :=

ifneq ($(TARGET_SYSTEMIMAGE_USE_UBIFS),)
image_flags += -DUBIFS_SYSTEM_IMAGE
endif

ifneq ($(TARGET_USERIMAGES_USE_UBIFS),)
image_flags += -DUBIFS_USER_IMAGES
endif

ifneq ($(TARGET_SYSTEMIMAGE_USE_EXT4),)
image_flags += -DEXTFS_SYSTEM_IMAGE
endif

ifneq ($(TARGET_USERIMAGES_USE_EXT4),)
image_flags += -DEXTFS_USER_IMAGES
endif

image_flags += -DSPL_APPENDING_TO=$(SPL_APPENDING_TO)

pdl_flags :=
ifeq ($(pdl), 1)
pdl_flags += -DCONFIG_RDA_PDL
endif

nand_flags :=
ifneq ($(BOARD_NAND_PAGE_SIZE),)
nand_flags += -DNAND_PAGE_SIZE=$(BOARD_NAND_PAGE_SIZE)
else
nand_flags += -DNAND_PAGE_SIZE=4096
endif
ifneq ($(BOARD_FLASH_BLOCK_SIZE),)
nand_flags += -DNAND_BLOCK_SIZE=$(BOARD_FLASH_BLOCK_SIZE)
else
nand_flags += -DNAND_BLOCK_SIZE=262144
endif
ifneq ($(BOARD_NAND_SPARE_SIZE),)
nand_flags += -DNAND_SPARE_SIZE=$(BOARD_NAND_SPARE_SIZE)
else
nand_flags += -DNAND_SPARE_SIZE=218
endif
ifneq ($(BOARD_NAND_ECCMSGLEN),)
nand_flags += -DNAND_ECCMSGLEN=$(BOARD_NAND_ECCMSGLEN)
else
nand_flags += -DNAND_ECCMSGLEN=1024
endif
ifneq ($(BOARD_NAND_ECCBITS),)
nand_flags += -DNAND_ECCBITS=$(BOARD_NAND_ECCBITS)
else
nand_flags += -DNAND_ECCBITS=24
endif
ifneq ($(BOARD_NAND_OOB_SIZE),)
nand_flags += -DNAND_OOBSIZE=$(BOARD_NAND_OOB_SIZE)
else
nand_flags += -DNAND_OOBSIZE=32
endif

build_variant_flags :=

ifneq ($(UBOOT_VARIANT), user)
build_variant_flags += -DCONFIG_UBOOT_VARIANT_DEBUG
endif

ifneq ($(BUILD_DISPLAY_ID),)
build_variant_flags += -DBUILD_DISPLAY_ID="\"$(BUILD_DISPLAY_ID)\""
endif

PLATFORM_CPPFLAGS += $(image_flags) $(pdl_flags) $(build_variant_flags) $(nand_flags)

