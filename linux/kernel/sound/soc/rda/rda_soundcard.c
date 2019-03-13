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

static int rda_soundcard_hw_params(struct snd_pcm_substream *substream,
				       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int err = 0;

	//printk(KERN_INFO ">>>> %s \n", __func__);

	/* Set the codec system clock for DAC and ADC */
	err =
	    snd_soc_dai_set_sysclk(codec_dai, 0, CODEC_CLOCK, SND_SOC_CLOCK_IN);

	if (err < 0) {
		//printk(KERN_ERR "can't set codec system clock\n");
		return err;
	}

	return err;
}

static struct snd_soc_ops rda_soundcard_dai_link_ops = {
	.hw_params = rda_soundcard_hw_params,
};

static const struct snd_soc_dapm_route rda_soundcard_audio_map[] = {
	{"Headphone Jack", NULL, "LHPOUT"},
	{"Headphone Jack", NULL, "RHPOUT"},

	{"LLINEIN", NULL, "Line In"},
	{"RLINEIN", NULL, "Line In"},

	{"MICIN", NULL, "Mic Jack"},
};

/* Digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link rda_soundcard_dai_link[] = {
	// audio stream
	{
		.name = "rda-soundcard-dai-link-aud",	// machine dai link name
		.stream_name = "rdaaud-stream",	// stream name

		// cpu(soc)(platform) name - manage "data" and "audio ifc"
		.platform_name = "rda-pcm",
		// cpu dai - manage aif
		// .cpu_dai_name = "rdaaud-platform-cpu-dai", 
		.cpu_dai_name = "rda-aif",

		// codec name
		.codec_name = "rda-codec",
		// codec dai name - in fact, there is no codec dai in rda
		.codec_dai_name = "rda-codec-dai",

		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS,
		.ops = &rda_soundcard_dai_link_ops,
	},
	// voice stream
	{
		.name = "rda-soundcard-dai-link-voice",	// machine dai link name
		.stream_name = "rdavoice-stream",	// stream name

		// cpu(soc)(platform) name - comm with modem
		.platform_name = "rda-voice-pcm",
		// cpu dai - fake
		.cpu_dai_name = "rda-voice-cpu-dai",

		// codec name - fake
		.codec_name = "rda-voice-codec",
		// codec dai name - fake
		.codec_dai_name = "rda-voice-codec-dai",

		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS,
		.ops = &rda_soundcard_dai_link_ops,
	},
};

/* Audio machine driver */
static struct snd_soc_card rda_soundcard = {
	.name = "rda-soundcard",	// sound card name
	.owner = THIS_MODULE,
	.dai_link = &rda_soundcard_dai_link[0],
	.num_links = ARRAY_SIZE(rda_soundcard_dai_link),

	.dapm_widgets = NULL,
	.num_dapm_widgets = 0,
	.dapm_routes = NULL,
	.num_dapm_routes = 0,
};

static struct platform_device *rda_snd_device;

static int __init rda_soundcard_modinit(void)
{
	int err = 0;

	rda_snd_device = platform_device_alloc("soc-audio", -1);

	if (!rda_snd_device)
		return -ENOMEM;

	platform_set_drvdata(rda_snd_device, &rda_soundcard);

	err = platform_device_add(rda_snd_device);
	if (err)
		platform_device_put(rda_snd_device);

	return err;
}

static void __exit rda_soundcard_modexit(void)
{
	platform_device_unregister(rda_snd_device);
}

module_init(rda_soundcard_modinit);
module_exit(rda_soundcard_modexit);

MODULE_DESCRIPTION("ALSA SoC for RDA SoundCard");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_LICENSE("GPL");
