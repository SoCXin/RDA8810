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

#include "rda_aif.h"

//#define DEBUG 1

#define AIF_FIX_SAMPLERATE_WHEN_CAPTURE 44100

#define FAST_CLOCK 26000000	//50M

#define AIF_SOURCE_CLOCK 48000000	//50M

struct rda_dai {
	struct device *dev;

	bool PlayActive;
	bool RecordActive;

	struct mutex mutex;

	u32 TxStb_div;
	u32 AudioBck_div;
	u32 bcklrck_div;
	u32 ChannelNb;

	u32 ControlReg;
	//u32 SerialMode;
	bool MasterFlag;

	bool OpenStatus;

	bool PlayStatus;
	bool RecordStatus;

};

static HWP_AIF_T __iomem *hwp_apAif;

static int rda_cpu_dai_startup(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct rda_dai *aif_cfg = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	mutex_lock(&aif_cfg->mutex);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (!aif_cfg->PlayActive) {
			aif_cfg->PlayActive = true;
		} else {
			ret = -EBUSY;
		}
	} else {
		if (!aif_cfg->RecordActive) {
			aif_cfg->RecordActive = true;
		} else {
			ret = -EBUSY;
		}
	}

	mutex_unlock(&aif_cfg->mutex);

	return ret;
}

static void rda_cpu_dai_shutdown(struct snd_pcm_substream *substream,
				     struct snd_soc_dai *dai)
{
	struct rda_dai *aif_cfg = snd_soc_dai_get_drvdata(dai);


	mutex_lock(&aif_cfg->mutex);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		aif_cfg->PlayActive = false;
	} else {
		aif_cfg->RecordActive = false;
	}

	if (!aif_cfg->PlayActive && !aif_cfg->RecordActive) {
		hwp_apAif->ctrl = 0;
		hwp_apAif->serial_ctrl = AIF_MASTER_MODE_MASTER;
		hwp_apAif->side_tone = 0;
		hwp_apAif->Cfg_Aif_Tx_Stb = 0;
		aif_cfg->OpenStatus = false;
	}
	mutex_unlock(&aif_cfg->mutex);
}

static int rda_cpu_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	struct rda_dai *aif_cfg = snd_soc_dai_get_drvdata(dai);

	u32 channel_num = 0;
	u32 lrck = 0;
	u32 bck = 0;
	u32 bck_lrck_ratio = 0;
	u32 sample_rate = 0;
	u32 aif_source_clock = 0;

#ifdef DEBUG
	printk(KERN_INFO"[%s] \n", __func__);
#endif

	if (!aif_cfg->OpenStatus) {

		sample_rate = params_rate(params);

#ifdef DEBUG
	printk(KERN_INFO"sample_rate : [%d] \n", sample_rate);
#endif
		// on rda : aif just config the output params
		// when input sample_rate is 8000, we should still config aif to playback sample_rate
		// like android-fixed 44100
#ifdef AIF_FIX_SAMPLERATE_WHEN_CAPTURE
		if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			printk(KERN_INFO"fix capture sample rate : [%d] \n", AIF_FIX_SAMPLERATE_WHEN_CAPTURE);
			sample_rate = AIF_FIX_SAMPLERATE_WHEN_CAPTURE;
		}
#endif

		lrck = sample_rate;

		switch (sample_rate) {
		case 8000:
			bck_lrck_ratio = 50;
			break;

		case 11025:
			bck_lrck_ratio = 36;
			break;

		case 12000:
			bck_lrck_ratio = 38;
			break;

		case 16000:
			bck_lrck_ratio = 50;
			break;

		case 22050:
			bck_lrck_ratio = 40;
			break;

		case 24000:
			bck_lrck_ratio = 38;
			break;

		case 32000:
			bck_lrck_ratio = 56;
			break;

		case 44100:
			bck_lrck_ratio = 62;
			break;

		case 48000:
			bck_lrck_ratio = 36;
			break;

		default:
			mutex_unlock(&aif_cfg->mutex);
			return -EINVAL;
			break;
		}

		bck = lrck * bck_lrck_ratio;
		aif_cfg->AudioBck_div = FAST_CLOCK / bck - 2;
		aif_cfg->bcklrck_div = bck_lrck_ratio / 2 - 16;

		channel_num = params_channels(params);

		switch (channel_num) {
		case 1:
		case 2:
			aif_cfg->ChannelNb = channel_num;
			break;
		default:
			mutex_unlock(&aif_cfg->mutex);
			return -EINVAL;
		}

		if (sample_rate == 8000 || sample_rate == 12000
		    || sample_rate == 16000 || sample_rate == 24000
		    || sample_rate == 32000 || sample_rate == 48000) {
			aif_source_clock = 24576000;
		} else if (sample_rate == 11025 || sample_rate == 22050
			   || sample_rate == 44100) {
			aif_source_clock = 22579200;
		} else {
			aif_source_clock = 24576000;
		}

		aif_cfg->TxStb_div = aif_source_clock / sample_rate - 2;
		aif_cfg->MasterFlag = true;
	}

	return 0;
}

static int rda_cpu_dai_prepare(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct rda_dai *aif_cfg = snd_soc_dai_get_drvdata(dai);

	u32 serialCfgReg = 0;

	if (!aif_cfg->OpenStatus) {

		hwp_apAif->Cfg_Aif_Tx_Stb =
		    AIF_AIF_TX_STB_EN | AIF_AIF_TX_STB_DIV(aif_cfg->TxStb_div);

		// config - communicate with codec NOT I2S
		aif_cfg->ControlReg |=
		    AIF_PARALLEL_OUT_SET_PARA | AIF_PARALLEL_IN_SET_PARA |
		    AIF_LOOP_BACK_NORMAL | AIF_TX_STB_MODE;

		// FIXME
		// we assume : 
		// 1. playback always use channel number 2
		// 2. this config of aif (not use i2s) just affect playback not capture
		// when this is called because record, we should know this configure will just used by playback
		// when capture config channel number 1, we should also configure this to channel number 2,
		// because playback will happen when capture

		// BUG : playback and capture interface maybe called 1 2 11 2 2 11025
		// so, aif_cfg->ChannelNb (capture set) maybe used by playback and STREO_DUPLT will be set and sound will be slower.
		// TODO : different ops when different stream, now we just set all to stereo cause android use 44100 & 2 channels
		// if (aif_cfg->ChannelNb == 1 && substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		// 	serialCfgReg |=
		// 	    AIF_MASTER_MODE_MASTER |
		// 	    AIF_TX_MODE_MONO_STEREO_DUPLI;
		// } else {
			serialCfgReg |=
			    AIF_MASTER_MODE_MASTER | AIF_TX_MODE_STEREO_STEREO;
		//}

		hwp_apAif->serial_ctrl = serialCfgReg;

		aif_cfg->OpenStatus = true;
	}

	return 0;
}

static int rda_cpu_dai_trigger(struct snd_pcm_substream *substream,
				   int cmd, struct snd_soc_dai *dai)
{
	struct rda_dai *aif_cfg = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
			hwp_apAif->ctrl =
			    (aif_cfg->ControlReg | AIF_ENABLE_H_ENABLE) &
			    ~AIF_TX_OFF;
			aif_cfg->PlayStatus = true;
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			if (!aif_cfg->RecordStatus) {
				hwp_apAif->ctrl = 0;
			}
			aif_cfg->PlayStatus = false;
			break;
		default:
			break;
		}

	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
			if (!aif_cfg->PlayStatus) {
				hwp_apAif->ctrl =
				    (aif_cfg->ControlReg | AIF_ENABLE_H_ENABLE)
				    | AIF_TX_OFF_TX_OFF;
			}
			aif_cfg->RecordStatus = true;
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			if (!aif_cfg->PlayStatus) {
				hwp_apAif->ctrl = 0;
			}
			aif_cfg->RecordStatus = false;
			break;
		default:
			break;
		}
	}

	return 0;
}

static const struct snd_soc_dai_ops rda_cpu_dai_driver_ops = {
	.startup = rda_cpu_dai_startup,
	.shutdown = rda_cpu_dai_shutdown,
	.hw_params = rda_cpu_dai_hw_params,
	.prepare = rda_cpu_dai_prepare,
	.trigger = rda_cpu_dai_trigger,
};

static int rda_cpu_dai_driver_probe(struct snd_soc_dai *dai)
{
	struct rda_dai *dmic = snd_soc_dai_get_drvdata(dai);

	pm_runtime_enable(dmic->dev);

	/* Disable lines while request is ongoing */
	pm_runtime_get_sync(dmic->dev);
	//omap_dmic_write(dmic, rda_dai_CTRL_REG, 0x00);
	pm_runtime_put_sync(dmic->dev);

	return 0;
}

static int rda_cpu_dai_driver_remove(struct snd_soc_dai *dai)
{
	struct rda_dai *dmic = snd_soc_dai_get_drvdata(dai);

	pm_runtime_disable(dmic->dev);

	return 0;
}

static struct snd_soc_dai_driver rda_cpu_dai_driver = {
	.name = "rda-cpu-dai-driver",
	.probe = rda_cpu_dai_driver_probe,
	.remove = rda_cpu_dai_driver_remove,
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
	.ops = &rda_cpu_dai_driver_ops,
};

static struct snd_soc_component_driver rda_component ={
	.name		= "rda-soundcard"
};
static int rda_cpu_dai_platform_driver_probe(struct
							   platform_device
							   *pdev)
{
	struct rda_dai *AifCfg;
	struct resource *res;
	int ret;

	AifCfg =
	    devm_kzalloc(&pdev->dev, sizeof(struct rda_dai), GFP_KERNEL);
	if (!AifCfg)
		return -ENOMEM;

	platform_set_drvdata(pdev, AifCfg);
	AifCfg->dev = &pdev->dev;

	mutex_init(&AifCfg->mutex);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(AifCfg->dev, "invalid dma resource\n");
		ret = -ENODEV;
		goto err_put_clk;
	}

	if (!devm_request_mem_region(&pdev->dev, res->start,
				     resource_size(res), pdev->name)) {
		dev_err(AifCfg->dev, "memory region already claimed\n");
		ret = -ENODEV;
		goto err_put_clk;
	}

	hwp_apAif = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!hwp_apAif) {
		ret = -ENOMEM;
		goto err_put_clk;
	}

	ret = snd_soc_register_component(&pdev->dev,&rda_component, &rda_cpu_dai_driver, 1);
	if (ret)
		goto err_put_clk;

	return 0;

err_put_clk:
	return ret;
}

static int __exit rda_cpu_dai_platform_driver_remove(struct
							    platform_device
							    *pdev)
{
	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static struct platform_driver rda_cpu_dai_platform_driver = {
	.driver = {
		   .name = "rda-aif",
		   .owner = THIS_MODULE,
		   },
	.probe = rda_cpu_dai_platform_driver_probe,
	.remove = __exit_p(rda_cpu_dai_platform_driver_remove),
};

static int __init rda_cpu_dai_modinit(void)
{
	return platform_driver_register(&rda_cpu_dai_platform_driver);
}

static void __exit rda_cpu_dai_modexit(void)
{
	platform_driver_unregister(&rda_cpu_dai_platform_driver);
}

module_init(rda_cpu_dai_modinit);
module_exit(rda_cpu_dai_modexit);

MODULE_DESCRIPTION("ALSA SoC for RDA CPU DAI");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_LICENSE("GPL");
