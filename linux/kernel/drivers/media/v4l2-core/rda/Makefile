#
# Copyright (C) 2010-2012 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
#
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#


# For each arch check: CROSS_COMPILE , KDIR , CFLAGS += -DARCH
EXTRA_CFLAGS += -DRDA_CUSTOMER_DRV_CSB -fno-pic
EXTRA_CFLAGS += -I$(srctree)/drivers/media/v4l2-core/rda/inc
EXTRA_CFLAGS += -I$(srctree)/drivers/media/platform/rda
EXTRA_CFLAGS += -DDEBUG
EXTRA_CFLAGS += -DGC2035_B

obj-m := rda_cam_sensor.o
