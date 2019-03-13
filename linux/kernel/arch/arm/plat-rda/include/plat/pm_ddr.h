#ifndef __RDA_PLAT_PM_DDR_H__
#define __RDA_PLAT_PM_DDR_H__

enum ddr_master {
	PM_DDR_CPU = 0,
	PM_DDR_VOC,
	PM_DDR_IFC0,
	PM_DDR_IFC1,
	PM_DDR_IFC2,
	PM_DDR_IFC3,
	PM_DDR_IFC4,
	PM_DDR_IFC5,
	PM_DDR_IFC6,
	PM_DDR_IFC7,
	PM_DDR_USB_DMA0,
	PM_DDR_USB_DMA1,
	PM_DDR_USB_DMA2,
	PM_DDR_USB_DMA3,
	PM_DDR_USB_DMA4,
	PM_DDR_USB_DMA5,
	PM_DDR_USB_DMA6,
	PM_DDR_USB_DMA7,
	PM_DDR_SD_DMA0,
	PM_DDR_SD_DMA1,
	PM_DDR_VPU_DMA,
	PM_DDR_GPU_DMA,
	PM_DDR_GOUDA_DMA,
	PM_DDR_FB_DMA,
	PM_DDR_CAMERA_DMA,
	PM_DDR_AUDIO_IFC_PLAYBACK,
	PM_DDR_AUDIO_IFC_CAPTURE,
	PM_DDR_MASTER_MAX
};

int pm_ddr_get(enum ddr_master master);
int pm_ddr_put(enum ddr_master master);
int pm_ddr_idle_status(void);

void vpu_bug_ddr_freq_adjust(void);
void vpu_bug_ddr_freq_adjust_restore(void);

#endif
