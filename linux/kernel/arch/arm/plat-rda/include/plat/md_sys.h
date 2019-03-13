/*
 * md_sys.h - A header file of definition of sys channel of modem of RDA
 *
 * Copyright (C) 2013 RDA Microelectronics Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __MD_SYS_H__
#define __MD_SYS_H__

#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>

#include <plat/devices.h>
#include <plat/rda_md.h>


/********************************
 * Definition of modules
 ********************************
 */

#define SYS_GEN_MOD	0x0
#define SYS_CALIB_MOD	0x1
#define SYS_PM_MOD	0x2
#define SYS_AUDIO_MOD	0x3
#define SYS_TS_MOD	0x4
/* To control gpio of bp */
#define SYS_GPIO_MOD	0x5
#define SYS_MAX_MOD	0x8

/********************************
 * Message from BP to AP
 ********************************
 */

/* Gereral Module (0x0) */
#define SYS_GEN_MESG_VER		0x0001
#define SYS_GEN_MESG_UNKNOWN		0x0002
#define SYS_GEN_MESG_RTC_TRIGGER	0x0003

/* Power Module (0x2) */
#define SYS_PM_MESG_BATT_STATUS		0x0001
#define SYS_PM_MESG_EP_STATUS		0x0002
#define SYS_PM_MESG_EP_KEY_STATUS	0x0003
/* For receiving temperature of chip from bp. */
#define SYS_PM_MESG_CHIP_TEMP_STATUS	0x0004

/* Audio Module (0x3) */
#define SYS_AUDIO_MESG_VOICE_HANDLER_CALLBACK	0x0001

/* Touch Screen Module (0x4) */
#define SYS_TS_MESG_RECEIVING		0x0001
#define SYS_TS_MESG_SENDING		0x0002

/* Gpio Module (0x5) */
#define SYS_GPIO_MESG_SOFT_IRQ		0x0001

/*********************************
 * Command from AP to BP
 *********************************
 */

/* -------------------- */
/* General Module (0x0) */
#define SYS_GEN_CMD_SHDW		0x1001
#define SYS_GEN_CMD_RESET		0x1002
#define SYS_GEN_CMD_TRACE		0x1003
/*
 * Parameter of CLK_OUT:
 * 0 : Disable
 * 1 : 32K clock
 * 2 : 26M clock
 */
#define SYS_GEN_CMD_CLK_OUT		0x1004
/*
 * Parameter of AUX_CLK:
 * 0 : Disable
 * 1 : 26M clock
 */
#define SYS_GEN_CMD_AUX_CLK		0x1005
/*
 * Parameter of CLK_32K
 * 0 : Disable
 * 1 : Enable 32K clock
 *
 */
#define SYS_GEN_CMD_CLK_32K		0x1006
/* Get information, such as version of stack, build data and so on. */
#define SYS_GEN_CMD_BP_INFO		0x1007
/* Get bluetooth device information */
#define SYS_GEN_CMD_GET_BT_INFO		0x1008
/*
 * Enable BP to output log
 * 0 : Disable
 * 1 : Enable
 */
#define SYS_GEN_CMD_ENABLE_TRACE_LOG	0x1009
#define SYS_GEN_CMD_QUERY_TRACE_STATUS	0x100A
#define SYS_GEN_CMD_SET_BT_INFO		0x100B
#define SYS_GEN_CMD_GET_WIFI_INFO	0x100C
#define SYS_GEN_CMD_SET_WIFI_INFO	0x100D
#define SYS_GEN_CMD_GET_LOGO_NAME	0x100E
#define SYS_GEN_CMD_SET_LOGO_NAME	0x100F

/* ------------------------ */
/* Calibration Module (0x1) */
#define SYS_CALIB_CMD_CALIB_STATUS	0x1002
#define SYS_CALIB_CMD_UPDATE_CALIB	0x1003
#define SYS_CALIB_CMD_UPDATE_FACTORTY	0x1004

/* Power Module (0x2) */
#define SYS_PM_CMD_EN			0x1001
#define SYS_PM_CMD_SET_LEVEL		0x1002
#define SYS_PM_CMD_SET_LDO_VOLT		0x1003
#define SYS_PM_CMD_BATT_VOLT		0x1004
#define SYS_PM_CMD_EP_VOLT		0x1005
#define SYS_PM_CMD_ADC_VALUE		0x1006
#define SYS_PM_CMD_EP_STATUS		0x1007
#define SYS_PM_CMD_ENABLE_ADC		0x1009
#define SYS_PM_CMD_GET_ADC_CALIB_VALUE	0x100A
#define SYS_PM_CMD_ENABLE_CHARGER	0x100B
/*
 * Set type of charger
 * 0 : AC charger
 * 1 : USB charger
 */
#define SYS_PM_CMD_SET_CHARGER_TYPE	0x100D
#define SYS_PM_CMD_GET_CHARGER_STATUS	0x100E
#define SYS_PM_CMD_NOTIFY_VPU_STATUS	0x1010

#define SYS_PM_CMD_EN_CHARGER_CURRENT	0x1011
#define SYS_PM_CMD_SET_CHARGER_CURRENT	0x1012
#define SYS_PM_CMD_SET_VOLT_PCT_MIN 	0x1013
#define SYS_PM_CMD_SET_VOLT_PCT_MAX 	0x1014

/* ------------------ */
/* Audio Module (0x3) */
#define SYS_AUDIO_CMD_AUD_STREAM_START			0x1001
#define SYS_AUDIO_CMD_AUD_STREAM_RECORD			0x1002
#define SYS_AUDIO_CMD_AUD_STREAM_PAUSE			0x1003
#define SYS_AUDIO_CMD_AUD_STREAM_STOP			0x1004
#define SYS_AUDIO_CMD_AUD_SETUP				0x1005
#define SYS_AUDIO_CMD_AUD_LOUDSPEAKER_WITH_EARPIECE	0x1006
#define SYS_AUDIO_CMD_AUD_MIX_LINEIN_WITH_CODEC         0x1007
#define SYS_AUDIO_CMD_AUD_CODEC_APP_MODE		0x100E
#define SYS_AUDIO_CMD_AUD_TEST_MODE_SETUP		0x100A
#define SYS_AUDIO_CMD_AUD_FORCE_RECEIVER_MIC_SELECTION	0x100B
#define SYS_AUDIO_CMD_AUD_VOICE_RECORD_START		0x100F
#define SYS_AUDIO_CMD_AUD_VOICE_RECORD_STOP		0x1010
#define SYS_AUDIO_CMD_AUD_GET_AUD_CALIB			0x1011
#define SYS_AUDIO_CMD_AUD_GET_HOST_AUDIO_CALIB		0x1012
#define SYS_AUDIO_CMD_AUD_GET_MP3_BUFFER		0x1013
#define SYS_AUDIO_CMD_AUD_GET_AUD_IIR_CALIB		0x1014
/*Add for Audio Calibration -----start-----*/
#define SYS_AUDIO_CMD_AUD_GET_AUD_EQ_CALIB     0x1015
#define SYS_AUDIO_CMD_AUD_GET_AUD_DRC_CALIB     0x1016
#define SYS_AUDIO_CMD_AUD_GET_INGAINS_RECORD_CALIB    0x1017
/*Add for Audio Calibration -----end -----*/

/* ------------------------- */
/* Touch Screen Module (0x4) */
#define SYS_TS_CMD_INIT			0x1001
#define SYS_TS_CMD_SEND			0x1002
#define SYS_TS_CMD_RECV			0x1003
#define SYS_TS_CMD_SUSPEND		0x1004
#define SYS_TS_CMD_RESUME		0x1005
#define SYS_TS_CMD_REBOOT		0x1006
#define SYS_TS_CMD_GET_I2C_ADDR		0x1007
#define SYS_TS_CMD_GET_INFO		0x1008
#define SYS_TS_CMD_SWITCH_PSENSOR_MODE	0x1009

/* ----------------- */
/* Gpio Module (0x5) */
#define SYS_GPIO_CMD_OPEN		0x1001
#define SYS_GPIO_CMD_CLOSE		0x1002
#define SYS_GPIO_CMD_SET_IO		0x1003
#define SYS_GPIO_CMD_GET_VALUE		0x1004
#define SYS_GPIO_CMD_SET_VALUE		0x1005
#define SYS_GPIO_CMD_ENABLE_IRQ		0x1006
#define SYS_GPIO_CMD_OPERATION		0x1007


#define SYS_MESG_MASK			0x7FFF
#define SYS_REPLY_FLAG			0x8000

#define PARAM_SIZE_MAX			128

/* Return value as sending command */
#define SYS_CMD_FAIL			0x80000001

struct client_mesg {
	unsigned short mod_id;
	unsigned short mesg_id;

	int param_size;
	unsigned char param[PARAM_SIZE_MAX];
	unsigned int reserved0;
	unsigned int reserved1;
};

struct msys_async_item {
	struct list_head async_list;
	struct md_sys_hdr_ext *phdr;
};

struct msys_frame_slot {
	struct list_head list;
	struct md_sys_hdr_ext *phdr;
	unsigned int using;
};

#define MSYS_MAX_SLOTS 	32

struct msys_master {
	struct device dev;

	struct md_port *port;
	/* Hold the version of BP */
	unsigned int version;

	/* for rx */
	struct workqueue_struct *rx_wq;
	struct work_struct rx_work;

	struct list_head rx_pending_list;

	/* for tx */
	spinlock_t pending_lock;
	struct list_head pending_list;

	struct mutex tx_mutex;
	bool running;

	spinlock_t client_lock;
	struct list_head client_list;

	struct client_mesg cli_msg;
	struct client_mesg ts_msg;

	spinlock_t slot_lock;
	char *pslot_data;
	struct msys_frame_slot fr_slot[MSYS_MAX_SLOTS];
};

struct msys_device {
	struct msys_master *pmaster;

	struct list_head dev_entry;

	/* callback for notifier */
	struct notifier_block notifier;
	unsigned int module;

	/* Name of client device */
	const char	 *name;
	/* A pointer points to the private area */
	void *private;
};

struct msys_message {
	struct msys_device *pmsys_dev;

	unsigned short mod_id;
	unsigned short mesg_id;

	/* parameters from ap to bp. */
	const void *pdata;
	int data_size;
	/* parameters from bp to ap. */
	void *pbp_data;
	int bp_data_size;
	/* for pending list of tx */
	struct list_head list;

	/* completion is reported through a callback */
	void (*complete)(void *context);
	void *context;
	/* actual size of tx */
	int tx_len;
	/* received size from bp for debugging */
	int rx_len;
	unsigned int status;
};

struct client_cmd {
	struct msys_device *pmsys_dev;

	unsigned short mod_id;
	unsigned short mesg_id;

	/* parameters from ap to bp. */
	const void *pdata;
	int data_size;
	/* parameters from bp to ap. */
	void *pout_data;
	int out_size;
};


unsigned int rda_msys_send_cmd(struct client_cmd *pcmd);

unsigned int rda_msys_send_cmd_timeout(struct client_cmd *pcmd, int timeout);

struct msys_device *rda_msys_alloc_device(void);

void rda_msys_free_device(struct msys_device *pmsys_dev);

int rda_msys_register_device(struct msys_device *pmsys_dev);

int rda_msys_unregister_device(struct msys_device *pmsys_dev);

#endif /* __MD_SYS_H__ */

