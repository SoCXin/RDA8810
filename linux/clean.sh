#!/bin/bash
set -e
##########################################
# This script is used to clean RDA8810 build.
# by: Qitas
# Date: 2019-03-27
##########################################

export ROOT=`pwd`

MENUSTR="Pls select build option"

OPTION=$(whiptail --title "RDA8810 Clean System" \
	--menu "Pls select clean option" 20 60 6 --cancel-button Finish --ok-button Select \
	"0"   "clean all" \
	"1"   "clean output only" \
	"2"   "clean kernel only" \
	"3"   "clean uboot only" \
	3>&1 1>&2 2>&3)

if [ $OPTION = "0" ]; then
	cd $ROOT		
	rm -rf output/
	cd $ROOT/kernel
	make clean
	cd $ROOT/uboot
	make clean 
	exit 0
elif [ $OPTION = "1" ]; then
	cd $ROOT		
	rm -rf output/
	exit 0
elif [ $OPTION = "2" ]; then
	cd $ROOT/kernel
	make clean
	exit 0
elif [ $OPTION = "3" ]; then
	cd $ROOT/uboot
	make clean 
	exit 0
else
	whiptail --title "RDA8810 clean files" \
		--msgbox "Pls select correct option" 10 50 0
	exit 0
fi
