/*
 * sound/soc/rda/soundcard_rdafpga.c
 *
 * Copyright (C) 2012 RDA.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/gpio.h>
#include <linux/module.h>

#define CODEC_CLOCK 	12000000


static int rda_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int err=0;

	
	printk(KERN_INFO "rda_hw_params\n");

	/* Set the codec system clock for DAC and ADC */
	err =
	    snd_soc_dai_set_sysclk(codec_dai, 0, CODEC_CLOCK, SND_SOC_CLOCK_IN);

	if (err < 0) {
		printk(KERN_ERR "can't set codec system clock\n");
		return err;
	}

	return err;
}

static struct snd_soc_ops rda_ops = {
	.hw_params = rda_hw_params,
};

static const struct snd_soc_dapm_widget tlv320aic23_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	{"Headphone Jack", NULL, "LHPOUT"},
	{"Headphone Jack", NULL, "RHPOUT"},

	{"LLINEIN", NULL, "Line In"},
	{"RLINEIN", NULL, "Line In"},

	{"MICIN", NULL, "Mic Jack"},
};

/* Digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link rda_dai = {
	.name = "TLV320AIC23",
	.stream_name = "AIC23",
	.cpu_dai_name = "rda-aif",//"rdafpga-dai-i2s",
	.codec_dai_name = "tlv320aic23-hifi",
	.platform_name = "rdafpga-pcm-audio",
	.codec_name ="spi0.0",// "tlv320aic23-spi",
	.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		   SND_SOC_DAIFMT_CBS_CFS,
	.ops = &rda_ops,
};

/* Audio machine driver */
static struct snd_soc_card snd_soc_card_rda = {
	.name = "rdafpga_soundcard",
	.owner = THIS_MODULE,
	.dai_link = &rda_dai,
	.num_links = 1,

	.dapm_widgets = tlv320aic23_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tlv320aic23_dapm_widgets),
	.dapm_routes = audio_map,
	.num_dapm_routes = ARRAY_SIZE(audio_map),
};

static struct platform_device *rda_snd_device;

static int __init rdasoundcard_modinit(void)
{
	int err;
	
	printk(KERN_INFO "rdasoundcard_modinit\n");
	
	rda_snd_device = platform_device_alloc("soc-audio", -1);
	if (!rda_snd_device)
		return -ENOMEM;

	platform_set_drvdata(rda_snd_device, &snd_soc_card_rda);
	err = platform_device_add(rda_snd_device);
	if (err)
	platform_device_put(rda_snd_device);

	return err;

}

static void __exit rdasoundcard_modexit(void)
{
	//clk_put(tlv320aic23_mclk);
	platform_device_unregister(rda_snd_device);
}
module_init(rdasoundcard_modinit);
module_exit(rdasoundcard_modexit);

MODULE_DESCRIPTION("ALSA SoC for RDA FPGA tlv320aic23");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_LICENSE("GPL");
