/*
 * rda_pcm.c  --  ALSA PCM interface for the RDA SoC
 *
 * Copyright (C) 2012 RDA Microelectronics (Beijing) Co., Ltd.
 *
 * Contact: Xu Mingliang <mingliangxu@rdamicro.com>
 * 
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

#include <plat/pm_ddr.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <plat/rda_debug.h>
#include "rda_audifc.h"
#include "rda_pcm.h"

struct rda_pcm_dma_data dma_data;

static const struct snd_pcm_hardware rda_pcm_hardware = {
	.info = SNDRV_PCM_INFO_MMAP |
	    SNDRV_PCM_INFO_MMAP_VALID |
	    SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
	    SNDRV_PCM_INFO_RESUME ,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min = 32,
	.period_bytes_max = 32 * 1024,//64 * 1024,
	.periods_min = 4,
	.periods_max = 4,
	.buffer_bytes_max = 128 * 1024,//256 * 1024,
};

struct rda_runtime_data {
	spinlock_t lock;
	struct rda_pcm_dma_data *dma_data;
	int dma_ch;
};

static void rda_pcm_dma_irq(int ch, int stat, void *data)
{
	struct snd_pcm_substream *substream = data;

	snd_pcm_period_elapsed(substream);
}

/* this may get called several times by oss emulation */
static int rda_pcm_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rda_runtime_data *prtd = runtime->private_data;
	int err = 0;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);

	prtd->dma_data = &dma_data;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data.dma_req = 1;
		dma_data.name = "play";

	} else {
		dma_data.dma_req = 0;
		dma_data.name = "record";
	}

	err = rda_request_audifc(dma_data.dma_req, dma_data.name,
				 rda_pcm_dma_irq, substream, &prtd->dma_ch);

	return err;
}

static int rda_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rda_runtime_data *prtd = runtime->private_data;

	if (prtd->dma_data == NULL)
		return 0;

	rda_free_audifc(prtd->dma_ch);
	prtd->dma_data = NULL;

	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int rda_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rda_runtime_data *prtd = runtime->private_data;
	struct rda_audifc_chan_params dma_params;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_params.src_addr = runtime->dma_addr;
		dma_params.dst_addr = 0;
		dma_params.xfer_size = runtime->dma_bytes;
		dma_params.audifc_mode = 0;

	} else {
		dma_params.src_addr = runtime->dma_addr;
		dma_params.dst_addr = 0;
		dma_params.xfer_size = runtime->dma_bytes;
		dma_params.audifc_mode = 0;
	}

	/*
	 * Set DMA transfer frame size equal to ALSA period size and frame
	 * count as no. of ALSA periods. Then with DMA frame interrupt enabled,
	 * we can transfer the whole ALSA buffer with single DMA transfer but
	 * still can get an interrupt at each period bounary
	 */

	rda_set_audifc_params(prtd->dma_ch, &dma_params);
	return 0;
}

static int rda_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rda_runtime_data *prtd = runtime->private_data;
	struct rda_pcm_dma_data *dma_data = prtd->dma_data;
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (dma_data->set_threshold)
			dma_data->set_threshold(substream);

		rda_dbg_audio("[start aud ifc] : substream->stream is [%d], %s, cmd is [%d] \n",
				substream->stream, substream->stream == SNDRV_PCM_STREAM_PLAYBACK?"PLAYBACK":"CAPTURE", cmd);
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			pm_ddr_get(PM_DDR_AUDIO_IFC_PLAYBACK);
		else
			pm_ddr_get(PM_DDR_AUDIO_IFC_CAPTURE);
		rda_start_audifc(prtd->dma_ch);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		rda_dbg_audio("[stop aud ifc] : substream->stream is [%d], %s, cmd is [%d] \n",
				substream->stream, substream->stream == SNDRV_PCM_STREAM_PLAYBACK?"PLAYBACK":"CAPTURE", cmd);
		rda_stop_audifc(prtd->dma_ch);
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			pm_ddr_put(PM_DDR_AUDIO_IFC_PLAYBACK);
		else
			pm_ddr_put(PM_DDR_AUDIO_IFC_CAPTURE);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t rda_pcm_pointer(struct snd_pcm_substream
					     *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rda_runtime_data *prtd = runtime->private_data;
	audifc_addr_t ptr;
	snd_pcm_uframes_t offset;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		ptr = rda_get_audifc_dst_pos(prtd->dma_ch);
		offset = bytes_to_frames(runtime, ptr - runtime->dma_addr);
	} else {
		ptr = rda_get_audifc_src_pos(prtd->dma_ch);
		offset = bytes_to_frames(runtime, ptr - runtime->dma_addr);
	}

	if (offset >= runtime->buffer_size)
		offset = 0;

	return offset;
}

static int rda_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rda_runtime_data *prtd;
	int ret;
	snd_soc_set_runtime_hwparams(substream, &rda_pcm_hardware);

	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		printk(KERN_INFO "[%s], snd_pcm_hw_constraint_integer < 0\n", __func__);
		goto out;
	}

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (prtd == NULL) {
		printk(KERN_INFO "[%s], kzalloc == NULL\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	spin_lock_init(&prtd->lock);
	runtime->private_data = prtd;

out:
	return ret;
}

static int rda_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	kfree(runtime->private_data);
	return 0;
}

static int rda_pcm_mmap(struct snd_pcm_substream *substream,
			    struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr, runtime->dma_bytes);
}

static struct snd_pcm_ops rda_pcm_ops = {
	.open = rda_pcm_open,
	.close = rda_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = rda_pcm_hw_params,
	.hw_free = rda_pcm_hw_free,
	.prepare = rda_pcm_prepare,
	.trigger = rda_pcm_trigger,
	.pointer = rda_pcm_pointer,
	.mmap = rda_pcm_mmap,
};

static u64 rda_pcm_dmamask = DMA_BIT_MASK(32);

static int rda_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = rda_pcm_hardware.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);

	if (!buf->area) {
		printk(KERN_INFO "rda audio :  %s,  alloc dma buffer fail. \n", __func__);
		return -ENOMEM;
	}

	buf->bytes = size;
	return 0;
}

static void rda_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static int rda_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &rda_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = rda_pcm_preallocate_dma_buffer(pcm,
							 SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = rda_pcm_preallocate_dma_buffer(pcm,
							 SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}

out:
	/* free preallocated buffers in case of error */
	if (ret)
		rda_pcm_free_dma_buffers(pcm);

	return ret;
}

static struct snd_soc_platform_driver rda_soc_platform = {
	.ops = &rda_pcm_ops,
	.pcm_new = rda_pcm_new,
	.pcm_free = rda_pcm_free_dma_buffers,
};

static int rda_pcm_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &rda_soc_platform);
}

static int __exit rda_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver rda_pcm_driver = {
	.driver = {
		   .name = "rda-pcm",
		   .owner = THIS_MODULE,
		   },

	.probe = rda_pcm_probe,
	.remove = __exit_p(rda_pcm_remove),
};

static int __init rda_pcm_modinit(void)
{
	return platform_driver_register(&rda_pcm_driver);
}

static void __exit rda_pcm_modexit(void)
{
	platform_driver_unregister(&rda_pcm_driver);
}

module_init(rda_pcm_modinit);
module_exit(rda_pcm_modexit);

MODULE_DESCRIPTION("ALSA SoC for RDA FPGA PCM");
MODULE_AUTHOR("Xu Mingliang <mingliangxu@rdamicro.com>");
MODULE_LICENSE("GPL");
