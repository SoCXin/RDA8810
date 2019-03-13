#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/v4l2-mediabus.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/module.h>

#include <media/soc_camera.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-chip-ident.h>

#include <float.h>

#include <mach/board.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>

#include <rda/tgt_ap_board_config.h>
#include "rda_isp_reg.h"
#include "rda_sensor.h"

#define HIST_LOW         0x20
#define HIST_HIGH        0x60


static uint curBV, targetBV, precurBV;
static int  preIndex, curIndex;
static int  histo;

static uint YaveDiffThresh = 0x20;
static uint TargetDiffThresh = 10;


static bool stable ;

void AE_getIndex(struct ae_control_list *ae_table);

void AE(int cBV, int hist, struct raw_sensor_info_data *raw, struct sensor_status *state_info,int first)
{
	struct ae_control_list *ae_table;
	static bool flag = true;
	int numAE_table, decBV;
	if(flag) {//first time to enter, set the starting point
		preIndex = curIndex = 30;
		precurBV = 0x60;
		flag = false;
	}

	ae_table = raw->ae_table;
	histo = hist;

	precurBV = curBV;
	curBV = cBV;
	targetBV =  state_info->targetBV;
	state_info->sensor_stable = 0;

	state_info->tBV_dec = 0;
	if (first == 1)
		return;

	AE_getIndex(ae_table);

	state_info->sensor_stable = stable;

	if (stable == 0) {
		state_info->gain = ((ae_table->val)+curIndex)->gain;
		state_info->exp= ((ae_table->val)+curIndex)->expo;
		if (( curIndex > 0x10 ) && (state_info->exp < 0xE)) {
			state_info->exp *= raw->flicker_50;
		}

	}

	numAE_table = ae_table->size;
	decBV = numAE_table - curIndex -1;
	if (decBV > 0x20 )
		state_info->tBV_dec = 0;
	else
		state_info->tBV_dec = (0x20 - decBV)/2;
}


void AE_getIndex(struct ae_control_list *ae_table)
{
	int Yave_diff=0, Target_diff=0;
	int curIndex_inc =0 ;
	int numAE_table;

	stable = 1;

	preIndex = curIndex;
	/* check if the Luminance change is big than threshold*/
	if (curBV > precurBV) {
		Yave_diff = curBV - precurBV;
	}
	else {
		Yave_diff = precurBV -  curBV;
	}

	if (Yave_diff >=  YaveDiffThresh) {
//		rda_dbg_camera("%s: Yave_diff is too big, Yave cBV = %02x , pre BV = %02x \n", __func__,curBV, precurBV );
		return ;
	}

/*less than threshold*/
	if ( targetBV > curBV ) {
		curIndex_inc =1;
		Target_diff = targetBV - curBV;
	}
	else {
		curIndex_inc =-1;
		Target_diff =  curBV - targetBV ;
	}

//	rda_dbg_camera("%s:  Target_diff = %x\n", __func__,Target_diff);
	if (Target_diff < TargetDiffThresh) {
		if (histo > HIST_HIGH)
			curIndex -=1;
//              rda_dbg_camera("%s: quite stable, Target_diff is quite small, Target_diff = %x\n", __func__,Target_diff);
	}
	else  if (Target_diff < TargetDiffThresh*2 ) {
		if (curIndex_inc ==-1)
			curIndex -=1;
		else if(histo < HIST_LOW)
			curIndex +=1;
//              rda_dbg_camera("%s:  Target_diff is less than 2*threshold, Target_diff = %x\n", __func__,Target_diff);
	}
	else  if (Target_diff < TargetDiffThresh*4 ) {
		if (curIndex_inc ==-1)
			curIndex -=2;
		else if(histo < HIST_LOW)
			curIndex +=2;
//              rda_dbg_camera("%s: Target_diff is less than 4*threshold, Target_diff = %x\n", __func__,Target_diff);
	}
	else  if (Target_diff < TargetDiffThresh*8 ) {
		if (curIndex_inc ==-1)
			curIndex -=4;
		else if(histo < HIST_LOW)
			curIndex +=4;
//              rda_dbg_camera("%s:  Target_diff is less than 8*threshold, Target_diff = %x\n", __func__,Target_diff);
	}
	else {
		if (curIndex_inc ==-1)
			curIndex -=8;
		else if(histo < HIST_LOW)
			curIndex +=8;
//              rda_dbg_camera("%s: Target_diff is bigger than 8*threshold, Target_diff = %x\n", __func__,Target_diff);
	}

	if (curIndex < 0 )
		curIndex = 0;

	numAE_table = ae_table->size;

//              rda_dbg_camera("%s:calculted index  = %x, AE tablesize = %x\n", __func__,curIndex, numAE_table);
	if (curIndex >= numAE_table)
		curIndex = numAE_table-1;
	if (preIndex != curIndex){
		stable =0;
		rda_dbg_camera("newIndex = %d ,preIndex = %d , target = %02x, Yave= %2x,"
			" hist=%02x,  exp = %04x, gain = %04x ",
			curIndex, preIndex,targetBV,curBV,histo, ((ae_table->val)+curIndex)->expo,
			((ae_table->val)+curIndex)->gain );
	}
}
