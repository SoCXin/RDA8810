/*
 * omap-dmic.c  --  OMAP ASoC DMIC DAI driver
 *
 * Copyright (C) 2010 - 2011 Texas Instruments
 *
 * Author: David Lambert <dlambert@ti.com>
 *	   Misael Lopez Cruz <misael.lopez@ti.com>
 *	   Liam Girdwood <lrg@ti.com>
 *	   Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <plat/dma.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

static int rda_voice_cpu_dai_startup(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	return 0;
}

static void rda_voice_cpu_dai_shutdown(struct snd_pcm_substream *substream,
				     struct snd_soc_dai *dai)
{
}

static int rda_voice_cpu_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	return 0;
}

static int rda_voice_cpu_dai_prepare(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	return 0;
}

static int rda_voice_cpu_dai_trigger(struct snd_pcm_substream *substream,
				   int cmd, struct snd_soc_dai *dai)
{
	return 0;
}

static const struct snd_soc_dai_ops rda_voice_cpu_dai_driver_ops = {
	.startup = rda_voice_cpu_dai_startup,
	.shutdown = rda_voice_cpu_dai_shutdown,
	.hw_params = rda_voice_cpu_dai_hw_params,
	.prepare = rda_voice_cpu_dai_prepare,
	.trigger = rda_voice_cpu_dai_trigger,
};

static int rda_voice_cpu_dai_driver_probe(struct snd_soc_dai *dai)
{
	return 0;
}

static int rda_voice_cpu_dai_driver_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static struct snd_soc_dai_driver rda_voice_cpu_dai_driver = {
	.name = "rda-voice-cpu-dai-driver",
	.probe = rda_voice_cpu_dai_driver_probe,
	.remove = rda_voice_cpu_dai_driver_remove,
	.playback = {
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SNDRV_PCM_FMTBIT_S16_LE,
		     .sig_bits = 16,
		     },
	.capture = {
		    .channels_min = 1,
		    .channels_max = 1,
		    .rates = SNDRV_PCM_RATE_8000_48000,
		    .formats = SNDRV_PCM_FMTBIT_S16_LE,
		    .sig_bits = 16,
		    },
	.ops = &rda_voice_cpu_dai_driver_ops,
};

static struct snd_soc_component_driver rda_voice_component = {
	.name		= "rda_voice_component"
};

static int rda_voice_cpu_dai_platform_driver_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = snd_soc_register_component(&pdev->dev, &rda_voice_component, &rda_voice_cpu_dai_driver, 1);
	return 0;
}

static int __exit rda_voice_cpu_dai_platform_driver_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static struct platform_driver rda_voice_cpu_dai_platform_driver = {
	.driver = {
		.name = "rda-voice-cpu-dai",
		.owner = THIS_MODULE,
	},
	.probe = rda_voice_cpu_dai_platform_driver_probe,
	.remove = __exit_p(rda_voice_cpu_dai_platform_driver_remove),
};

static struct platform_device rda_voice_cpu_dai = {
	.name = "rda-voice-cpu-dai",
	.id = -1,
};

static int __init rda_voice_cpu_dai_modinit(void)
{
	platform_device_register(&rda_voice_cpu_dai);
	return platform_driver_register(&rda_voice_cpu_dai_platform_driver);
}

static void __exit rda_voice_cpu_dai_modexit(void)
{
	platform_driver_unregister(&rda_voice_cpu_dai_platform_driver);
}

module_init(rda_voice_cpu_dai_modinit);
module_exit(rda_voice_cpu_dai_modexit);

MODULE_DESCRIPTION("ALSA SoC for RDA CPU DAI");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_LICENSE("GPL");
