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

//#include "rda_dai.h"
#include "aif.h"



#define FAST_CLOCK 26000000 //50M

#define AIF_SOURCE_CLOCK 48000000 //50M

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



static  HWP_AIF_T __iomem *hwp_apAif;
/*
 * Stream DMA parameters

static struct omap_pcm_dma_data rda_dai_dai_dma_params = {
	.name		= "DMIC capture",
	.data_type	= OMAP_DMA_DATA_TYPE_S32,
	.sync_mode	= OMAP_DMA_SYNC_PACKET,
};

*/

static int rda_dai_dai_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct rda_dai *AifCfg = snd_soc_dai_get_drvdata(dai);
	int ret = 0;
	printk(KERN_INFO "rda_dai_dai_startup");
	mutex_lock(&AifCfg->mutex);
	if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
	{
		if (!AifCfg->PlayActive)
			AifCfg->PlayActive = 1;
		else
			ret = -EBUSY;
	}else{

		if (!AifCfg->RecordActive)
			AifCfg->RecordActive = 1;
		else
			ret = -EBUSY;
	}
	mutex_unlock(&AifCfg->mutex);

	return ret;
}

static void rda_dai_dai_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct rda_dai *AifCfg = snd_soc_dai_get_drvdata(dai);
	printk(KERN_INFO "rda_dai_dai_shutdown");
	mutex_lock(&AifCfg->mutex);
	if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
	{
		AifCfg->PlayActive = 0;
	}else{
		AifCfg->RecordActive = 0;
	}

	//close
	if(AifCfg->PlayActive == 0&&AifCfg->RecordActive == 0){

		hwp_apAif->ctrl=0;
		hwp_apAif->serial_ctrl=AIF_MASTER_MODE_MASTER;
		hwp_apAif->side_tone=0;
		hwp_apAif->Cfg_Aif_Tx_Stb=0;
		AifCfg->OpenStatus=0;
	}
	mutex_unlock(&AifCfg->mutex);
}



static int rda_dai_dai_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct rda_dai *AifCfg = snd_soc_dai_get_drvdata(dai);

	u32 ChannelNb;
	u32 Lrck;
	u32 Bck;
	u32 BckLrckRatio;

	printk(KERN_INFO "rda_dai_dai_hw_params");

	if(AifCfg->OpenStatus==0){
		u32 SampleRate=params_rate(params);

		AifCfg->TxStb_div=AIF_SOURCE_CLOCK/SampleRate-2;

		//LRCK frequency
		Lrck=SampleRate;


		switch(SampleRate){
		case 8000:
			BckLrckRatio=50;
			break;

		case 11025:
			BckLrckRatio=36;
			break;

		case 12000:
			BckLrckRatio=38;
			break;

		case 16000:
			BckLrckRatio=50;
			break;

		case 22050:
			BckLrckRatio=40;
			break;

		case 24000:
			BckLrckRatio=38;
			break;

		case 32000:
			BckLrckRatio=56;
			break;

		case 44100:
			BckLrckRatio=62;
			break;

		case 48000:
			BckLrckRatio=36;
			break;

		default:
		dev_err(AifCfg->dev,"Improper stream frequency.\n");
		return -EINVAL;

		break;
		}

		Bck=Lrck*BckLrckRatio;
		AifCfg->AudioBck_div=FAST_CLOCK/Bck-2;
		AifCfg->bcklrck_div=BckLrckRatio/2-16;


		ChannelNb = params_channels(params);

		switch (ChannelNb) {
		case 1:
		case 2:
			AifCfg->ChannelNb=ChannelNb;
			break;
		default:
			dev_err(AifCfg->dev, "invalid number of legacy channels\n");
			return -EINVAL;
		}

		AifCfg->MasterFlag=true;


		/* packet size is threshold * channels */
		//snd_soc_dai_set_dma_data(dai, substream, &rda_dai_dai_dma_params);

	}
	

	return 0;
}

static int rda_dai_dai_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct rda_dai *AifCfg = snd_soc_dai_get_drvdata(dai);
	u32 SerialCfgReg=0;
	
	printk(KERN_INFO "rda_dai_dai_prepare:%p\n",hwp_apAif);

	if(AifCfg->OpenStatus == 0)
	{
		hwp_apAif->Cfg_Aif_Tx_Stb=AIF_AIF_TX_STB_EN | AIF_AIF_TX_STB_DIV(AifCfg->TxStb_div);

		// Serial in and serial out
		AifCfg->ControlReg =  AIF_PARALLEL_OUT_CLR_PARA |
		                  		AIF_PARALLEL_IN_CLR_PARA |
		                  		AIF_LOOP_BACK_NORMAL;

		//SerialMode
		SerialCfgReg |=AIF_SERIAL_MODE_I2S_PCM;

		//Master
		SerialCfgReg |=AifCfg->MasterFlag?AIF_MASTER_MODE_MASTER:AIF_MASTER_MODE_SLAVE;

		//LSB first
		SerialCfgReg |=AIF_LSB_MSB;

		//LRCK polarity
		SerialCfgReg |=AIF_LRCK_POL_LEFT_H_RIGHT_L;

		//Rx delay
		SerialCfgReg |=AIF_RX_DLY_DLY_2;

		//Tx delay
		SerialCfgReg |=AIF_TX_DLY_DLY_1;

		//Rx Tx  Mode
		if(AifCfg->ChannelNb==1)
		{
			//Rx mode
			SerialCfgReg |=AIF_RX_MODE_STEREO_MONO_FROM_L;
			//Tx mode
			SerialCfgReg |=AIF_TX_MODE_MONO_STEREO_DUPLI;
		}else{
			//Rx mode
			SerialCfgReg |=AIF_RX_MODE_STEREO_STEREO;
			//Tx mode
			SerialCfgReg |=AIF_TX_MODE_STEREO_STEREO;
		}


		//we get the Bck from the audio clock
		hwp_apAif->Cfg_Clk_AudioBCK=AIF_AUDIOBCK_DIVIDER(AifCfg->AudioBck_div);

		SerialCfgReg |= AIF_BCK_LRCK(AifCfg->bcklrck_div);

		//BCK polarity
		SerialCfgReg |=AifCfg->MasterFlag?AIF_BCK_POL_NORMAL:AIF_BCK_POL_INVERT;

		// Output Half Cycle Delay
		SerialCfgReg |=AifCfg->MasterFlag?AIF_OUTPUT_HALF_CYCLE_DLY_DLY:AIF_OUTPUT_HALF_CYCLE_DLY_NO_DLY;

		// Input Half Cycle Delay
		SerialCfgReg |=AIF_INPUT_HALF_CYCLE_DLY_NO_DLY;

		//BckOut gating
		SerialCfgReg |=AIF_BCKOUT_GATE_NO_GATE;

		hwp_apAif->serial_ctrl=SerialCfgReg;

		AifCfg->OpenStatus=1;

	}

	printk(KERN_INFO "rda_dai_dai_prepare end:%p\n",hwp_apAif);

	return 0;
}

static int rda_dai_dai_trigger(struct snd_pcm_substream *substream,
				  int cmd, struct snd_soc_dai *dai)
{
	struct rda_dai *AifCfg = snd_soc_dai_get_drvdata(dai);
	printk(KERN_INFO "rda_dai_dai_trigger");
	if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
	{
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
			printk(KERN_INFO "rda_dai_dai_trigger start");
			hwp_apAif->ctrl=(AifCfg->ControlReg|AIF_ENABLE_H_ENABLE)&~AIF_TX_OFF;
			AifCfg->PlayStatus=1;
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			printk(KERN_INFO "rda_dai_dai_trigger try stop");
			if(AifCfg->RecordStatus==0){
				printk(KERN_INFO "rda_dai_dai_trigger stop");
				// To have the clock allowing the disabling.
				hwp_apAif->ctrl=0;
			}
			AifCfg->PlayStatus=0;
			break;
		default:
			break;
		}

	}else{

		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
			if(AifCfg->PlayStatus==0){
				hwp_apAif->ctrl=(AifCfg->ControlReg|AIF_ENABLE_H_ENABLE)|AIF_TX_OFF_TX_OFF;
			}
			AifCfg->RecordStatus=1;
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			if(AifCfg->PlayStatus==0){
				// Disable the AIF if not recording.
				hwp_apAif->ctrl=0;
			}
			AifCfg->RecordStatus=0;
			break;
		default:
			break;
		}

	}
//	printk(KERN_INFO "rda_dai_dai_trigger end");

	return 0;
}


static const struct snd_soc_dai_ops rda_dai_dai_ops = {
	.startup	= rda_dai_dai_startup,
	.shutdown	= rda_dai_dai_shutdown,
	.hw_params	= rda_dai_dai_hw_params,
	.prepare	= rda_dai_dai_prepare,
	.trigger	= rda_dai_dai_trigger,
//	.set_sysclk	= rda_dai_set_dai_sysclk,
};

static int rda_dai_probe(struct snd_soc_dai *dai)
{
	struct rda_dai *dmic = snd_soc_dai_get_drvdata(dai);
	printk(KERN_INFO "rda_dai_probe");

	pm_runtime_enable(dmic->dev);

	/* Disable lines while request is ongoing */
	pm_runtime_get_sync(dmic->dev);
	//omap_dmic_write(dmic, rda_dai_CTRL_REG, 0x00);
	pm_runtime_put_sync(dmic->dev);

	/* Configure DMIC threshold value */
	//dmic->threshold = rda_dai_THRES_MAX - 3;
	return 0;
}

static int rda_dai_remove(struct snd_soc_dai *dai)
{
	struct rda_dai *dmic = snd_soc_dai_get_drvdata(dai);

	pm_runtime_disable(dmic->dev);

	return 0;
}

static struct snd_soc_dai_driver rda_dai_dai = {
	.name = "rda-aif",
	.probe = rda_dai_probe,
	.remove = rda_dai_remove,
	.playback={
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
		.sig_bits = 16,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
		.sig_bits = 16,
	},
	.ops = &rda_dai_dai_ops,
};

static __devinit int asoc_dmic_probe(struct platform_device *pdev)
{
	struct rda_dai *AifCfg;
	struct resource *res;
	int ret;

	printk(KERN_INFO "asoc_dmic_probe");

	AifCfg = devm_kzalloc(&pdev->dev, sizeof(struct rda_dai), GFP_KERNEL);
	if (!AifCfg)
		return -ENOMEM;

	platform_set_drvdata(pdev, AifCfg);
	AifCfg->dev = &pdev->dev;
//	dmic->sysclk = rda_dai_SYSCLK_SYNC_MUX_CLKS;


	mutex_init(&AifCfg->mutex);

//	dmic->fclk = clk_get(dmic->dev, "dmic_fck");
//	if (IS_ERR(dmic->fclk)) {
//		dev_err(dmic->dev, "cant get dmic_fck\n");
//		return -ENODEV;
//	}

//	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma");
//	if (!res) {
//		dev_err(dmic->dev, "invalid dma memory resource\n");
//		ret = -ENODEV;
//		goto err_put_clk;
//	}
//	omap_dmic_dai_dma_params.port_addr = res->start + rda_dai_DATA_REG;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(AifCfg->dev, "invalid dma resource\n");
		ret = -ENODEV;
		goto err_put_clk;
	}
/*	rda_dai_dai_dma_params.dma_req = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mpu");
	if (!res) {
		dev_err(dmic->dev, "invalid memory resource\n");
		ret = -ENODEV;
		goto err_put_clk;
	}
*/
	if (!devm_request_mem_region(&pdev->dev, res->start,
				     resource_size(res), pdev->name)) {
		dev_err(AifCfg->dev, "memory region already claimed\n");
		ret = -ENODEV;
		goto err_put_clk;
	}

	hwp_apAif = devm_ioremap(&pdev->dev, res->start,
				     resource_size(res));
	if (!hwp_apAif) {
		ret = -ENOMEM;
		goto err_put_clk;
	}

	ret = snd_soc_register_dai(&pdev->dev, &rda_dai_dai);
	if (ret)
		goto err_put_clk;

	return 0;

err_put_clk:
	//clk_put(dmic->fclk);
	return ret;
}

static int __devexit asoc_dmic_remove(struct platform_device *pdev)
{
//	struct rda_dai *AifCfg = platform_get_drvdata(pdev);

	snd_soc_unregister_dai(&pdev->dev);
	//clk_put(dmic->fclk);

	return 0;
}

static struct platform_driver rda_dai_driver = {
	.driver = {
		.name = "rda-aif",
		.owner = THIS_MODULE,
	},
	.probe = asoc_dmic_probe,
	.remove = __devexit_p(asoc_dmic_remove),
};



static int __init rdafpag_dai_modinit(void)
{
	return platform_driver_register(&rda_dai_driver);
}

static void __exit rdafpag_dai_modexit(void)
{
	platform_driver_unregister(&rda_dai_driver);
}


module_init(rdafpag_dai_modinit);
module_exit(rdafpag_dai_modexit);



MODULE_DESCRIPTION("ALSA SoC for RDA FPGA DAI");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_LICENSE("GPL");
