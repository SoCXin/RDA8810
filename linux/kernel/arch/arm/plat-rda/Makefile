#
# Makefile for the linux kernel.
#

subdir-ccflags-y := -Werror

# Common support
obj-y := cpu.o comreg0_misc.o md.o md_sys.o timer.o ifc.o dma.o \
	vibrator.o counter_16k.o ap_clk.o mem_sc.o boot_mode.o modem_xcpu.o \
	pm_ddr.o
obj-m :=
obj-n :=
obj-  :=
obj-$(CONFIG_PM) += pm.o
obj-$(CONFIG_RDA_SLEEP_OFF_MODE) += sleep.o

obj-$(CONFIG_ARCH_RDA) += intc.o
obj-$(CONFIG_CPU_IDLE) += cpuidle.o
obj-$(CONFIG_ARCH_RDA8850E) += pd_ctrl.o
obj-$(CONFIG_RDA_RMNET) += smd.o
obj-$(CONFIG_RDA_ISPI) += ispi.o
