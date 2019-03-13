
#ifndef __RDA_DSP_AUD_RADIO_H__
#define __RDA_DSP_AUD_RADIO_H__


#include <linux/ioctl.h>
#include <linux/time.h>

//scan sort algorithm 
enum{
    DSP_AUD_SCAN_SORT_NON = 0,
    DSP_AUD_SCAN_SORT_UP,
    DSP_AUD_SCAN_SORT_DOWN,
    DSP_AUD_SCAN_SORT_MAX
};

//scan methods
enum{
    DSP_AUD_SCAN_SEL_HW = 0, //select hardware scan, advantage: fast
    DSP_AUD_SCAN_SEL_SW,     //select software scan, advantage: more accurate
    DSP_AUD_SCAN_SEL_MAX
};



#define DSP_AUD_NAME             "rdadspAud"
#define DSP_AUD_DEVICE_NAME      "/dev/rdadspAud"

// ********** ***********DSP_AUD  IOCTL define start *******************************

#define DSP_AUD_IOC_MAGIC        0xf9 // FIXME: any conflict?


//IOCTL and struct for test
#define GP0_1_LEVEL       _IOWR(DSP_AUD_IOC_MAGIC, 11, uint32_t*)

struct dsp_aud_em_parm {
	uint16_t group_idx;
	uint16_t item_idx;
	uint32_t item_value;	
};


#endif // __RDA_DSP_AUD_RADIO_H__
