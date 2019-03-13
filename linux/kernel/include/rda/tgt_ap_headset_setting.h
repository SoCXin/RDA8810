#ifndef __TGT_AP_HEADSET_SETTING_H
#define __TGT_AP_HEADSET_SETTING_H

/* notice
 * 1. when ap do vmic gpadc detect, headset ( or headphone ) will be detected according to first gpadc value
 * 2. when ap donot do vmic gpadc detect, headset ( or headphone ) will be treated as headset only ( for now )
 * 3. when ap donot do vmic gpadc detect, only on key will be processed.
*/

// used always
// open this if ap do gpadc detect, or modem will report key to ap ( only KEY_MEDIA now )
#define AP_DETECT_VMIC_GPADC 1

// used when ap donot do gpadc detect
// key code ( input.h ) that report when key down or up
#define HEADSET_KEY_REPORT KEY_MEDIA

// used always
// 0 means gpio low when no headset, or high
#define HEADSET_OUT_GPIO_STATE 1

// used when ap do gpadc detect
// VMIC gpadc channel
#define AP_VMIC_GPADC_CHANNEL 0

// used when ap do gpadc detect
// debounce value
#define HEADSET_GPADC_DEBOUNCE_VALUE 200

// used when ap do gpadc detect
// FIXME, should sync with modem PMD_POWER_ID_T
#define BP_PMD_POWER_EARPIECE 9

// used when ap do gpadc detect
// under this value ,we think this is a headphone
#define DETECT_HEADPHON_MAX_VALUE 0xFF

// used when ap do gpadc detect
// when MODEM_REPORT_KEY_ADC define, we need give right min adc value and max adc value
// keycode in input.h (kernel) : min adc value : max adc value
#define HEADSET_KEY_CAPS \
	{KEY_MEDIA, 0x0, 0x70}, \
	{KEY_VOLUMEUP, 0x80, 0xC0}, \
	{KEY_VOLUMEDOWN, 0x130, 0x170},

// used when ap do gpadc detect
#define GPADC_DETECT_DELAY_MSECS 80
// used when ap do gpadc detect
#define GPADC_DETECT_DEBOUNCE_DELAY_MSECS 60

// used always
// for irq debounce
#define GPIO_IRQ_DETECT_DEBOUNCE_DELAY_MSECS 200

// used always
// for out debounce
#define GPIO_OUT_DETECT_DEBOUNCE_DELAY_MSECS 200

//////////////////////////// need not change ////////////////////////////////
#define RDA_HEADSET_KEYPAD_NAME "rda-headset-keypad"
#define RDA_HEADSET_DETECT_NAME "h2w"

#define RDA_HEADSET_DETECT_STATE_OUT "0"
#define RDA_HEADSET_DETECT_STATE_HEADSET "1"
#define RDA_HEADSET_DETECT_STATE_HEADPHONE "2"

#endif
