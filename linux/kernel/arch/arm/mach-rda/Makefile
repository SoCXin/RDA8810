#
# Makefile for the linux kernel.
#

# Object file lists.

subdir-ccflags-y := -Werror

obj-y	:= devices.o gpio.o regulator.o regulator-devices.o rda-debug.o

obj-$(CONFIG_MACH_RDA8810)	+= board-rda8810.o io-rda8810.o
obj-$(CONFIG_MACH_RDA8810E)	+= board-rda8810e.o io-rda8810e.o
obj-$(CONFIG_MACH_RDA8820)	+= board-rda8820.o io-rda8820.o
obj-$(CONFIG_MACH_RDA8850E)	+= board-rda8850e.o io-rda8850e.o
obj-$(CONFIG_MACH_RDA8810H)	+= board-rda8810h.o io-rda8810h.o

obj-$(CONFIG_CLK_RDA)	+= clk_rda.o clk_ops_rda.o

obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o
