/*
 * ALSA SoC RDA codec driver
 *
 * Author:      Arun KS, <arunks@mistralsolutions.com>
 * Copyright:   (C) 2008 Mistral Solutions Pvt Ltd.,
 *
 * Based on sound/soc/codecs/wm8731.c by Richard Purdie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Notes:
 *  rda codec
 *
 *  The machine layer should disable unsupported inputs/outputs by
 *  snd_soc_dapm_disable_pin(codec, "LHPOUT"), etc.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <mach/iomap.h>
#include <asm/io.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <plat/reg_spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <plat/rda_debug.h>
#include <plat/md_sys.h>

static int rda_voice_codec_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	return 0;
}

static int rda_voice_codec_dai_pcm_prepare(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	return 0;
}

static int rda_voice_codec_dai_trigger(struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *dai)
{
	return 0;
}

static int rda_voice_codec_dai_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	return 0;
}

static void rda_voice_codec_dai_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
}

static int rda_voice_codec_dai_dig_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int rda_voice_codec_dai_set_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	return 0;
}

static int rda_voice_codec_dai_set_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int rda_voice_codec_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	return 0;
}

#define RDA_CODEC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops rda_voice_codec_dai_ops = {
	.startup = rda_voice_codec_dai_startup,
	.hw_params = rda_voice_codec_dai_hw_params,
	.prepare = rda_voice_codec_dai_pcm_prepare,
	.trigger = rda_voice_codec_dai_trigger,
	.shutdown = rda_voice_codec_dai_shutdown,
	.digital_mute = rda_voice_codec_dai_dig_mute,
	.set_fmt = rda_voice_codec_dai_set_fmt,
	.set_sysclk = rda_voice_codec_dai_set_sysclk,
};

static struct snd_soc_dai_driver rda_voice_codec_dai_driver = {
	.name = "rda-voice-codec-dai",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = RDA_CODEC_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = RDA_CODEC_FORMATS,
	},
	.ops = &rda_voice_codec_dai_ops,
};

static int rda_voice_codec_suspend(struct snd_soc_codec *codec)
{
	return 0;
}

static int rda_voice_codec_resume(struct snd_soc_codec *codec)
{
	return 0;
}

static int rda_voice_codec_probe(struct snd_soc_codec *codec)
{
	return 0;
}

static int rda_voice_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_rda_voice_codec_driver = {
	.reg_cache_size = 0,
	.reg_word_size = sizeof(u16),
	.reg_cache_default = NULL,
	.probe = rda_voice_codec_probe,
	.remove = rda_voice_codec_remove,
	.suspend = rda_voice_codec_suspend,
	.resume = rda_voice_codec_resume,
	.set_bias_level = rda_voice_codec_set_bias_level,
	.dapm_widgets = NULL,
	.num_dapm_widgets = 0,
	.dapm_routes = NULL,
	.num_dapm_routes = 0,
};

static int rda_voice_codec_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	ret = snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_rda_voice_codec_driver,
			&rda_voice_codec_dai_driver, 1);
	return 0;
}

static int __exit rda_voice_codec_platform_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_voice_codec_driver = {
	.driver = {
		.name = "rda-voice-codec",
		.owner = THIS_MODULE,
	},

	.probe = rda_voice_codec_platform_probe,
	.remove = __exit_p(rda_voice_codec_platform_remove),
};

static struct platform_device rda_voice_codec = {
	.name = "rda-voice-codec",
	.id = -1,
};

static int __init rda_voice_codec_modinit(void)
{
	platform_device_register(&rda_voice_codec);
	return platform_driver_register(&rda_voice_codec_driver);
}

static void __exit rda_voice_codec_modexit(void)
{
	platform_driver_unregister(&rda_voice_codec_driver);
}

static void __exit rdafpag_pcm_modexit(void)
{
	platform_driver_unregister(&rda_voice_codec_driver);
}

module_init(rda_voice_codec_modinit);
module_exit(rda_voice_codec_modexit);

MODULE_DESCRIPTION("ASoC RDA codec driver");
MODULE_AUTHOR("Arun KS <arunks@mistralsolutions.com>");
MODULE_LICENSE("GPL");
