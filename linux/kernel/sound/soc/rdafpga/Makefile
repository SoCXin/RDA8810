# rda Platform Support

rda-audio-ifc-objs					:= aud_ifc.o
audio-codec-spi-objs					:= tlv320aic23_spi.o
snd-rda-rdasoundcard-fpba-objs				:= rdafpga_soundcard.o
snd-rda-rdasoundcard-fpba-pcm-objs			:= rdafpga_pcm.o
snd-rda-rdasoundcard-fpba-dai-objs			:= rdafpga_dai.o

obj-$(CONFIG_SND_RDA_SOC_RDASOUNDCARD_FPGA)		+= rda-audio-ifc.o
obj-$(CONFIG_SND_RDA_SOC_RDASOUNDCARD_FPGA)		+= audio-codec-spi.o

obj-$(CONFIG_SND_RDA_SOC_RDASOUNDCARD_FPGA)		+= snd-rda-rdasoundcard-fpba.o
obj-$(CONFIG_SND_RDA_SOC_RDASOUNDCARD_FPGA_PCM)	+= snd-rda-rdasoundcard-fpba-pcm.o
obj-$(CONFIG_SND_RDA_SOC_RDASOUNDCARD_FPGA_DAI)	+= snd-rda-rdasoundcard-fpba-dai.o
