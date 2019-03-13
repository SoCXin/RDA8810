#
# Makefile for the RDA camera driver
#
#subdir-ccflags-y := -Werror
#EXTRA_CFLAGS += -DGPIO_CAM_PWDN1 

#rda_sensor_base-objs := isp_ae.o rda_camera_8810.o
obj-$(CONFIG_VIDEO_RDA) += isp_ae.o

obj-$(CONFIG_VIDEO_RDA) += rda_sensor_base.o
obj-$(CONFIG_VIDEO_RDA) += rda_camera_8810.o

