#
# Makefile for the RDA framebuffer driver
#
subdir-ccflags-y := -Werror
subdir-ccflags-y += -I$(ANDROID_BUILD_TOP)/$(RDA_TARGET_DEVICE_DIR)/include
#TODO - remove this!!
subdir-ccflags-y += -I$(ANDROID_BUILD_TOP)/$(RDA_TARGET_DEVICE_DIR)/../../driver/panel
subdir-ccflags-y += -Idrivers/staging/android

obj-$(CONFIG_RDA_GOUDA)        += rda_gouda.o
obj-$(CONFIG_RDA_LCDC)	       += rda_lcdc.o rda_mipi_dsi.o

obj-$(CONFIG_FB_RDA)           += rdafb.o
rdafb-y                        := rda_fb.o
rdafb-y                        += rda_panel.o
rdafb-objs                     := $(rdafb-y)
