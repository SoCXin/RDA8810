#!/bin/bash
set -e
##########################################
# This script is used to build RDA8810 Linux.
# by: Qitas
# Date: 2019-03-12
##########################################

export ROOT=`pwd`
SCRIPTS=$ROOT/scripts
export BOOT_PATH
export ROOTFS_PATH
export UBOOT_PATH

function git_configure()
{
    export GIT_CURL_VERBOSE=1
    export GIT_TRACE_PACKET=1
    export GIT_TRACE=1    
}


toolchain="https://codeload.github.com/sochub/arm-linux-eabi/zip/master"

function get_toolchain()
{
    if [ ! -d $ROOT/toolchain/arm-linux-gnueabi ]; then
        mkdir -p $ROOT/.tmp_toolchain
        cd $ROOT/.tmp_toolchain
        curl -C - -o ./toolchain $toolchain
        unzip $ROOT/.tmp_toolchain/toolchain
        mkdir -p $ROOT/toolchain
        mv $ROOT/.tmp_toolchain/arm-linux-eabi-master/gcc-arm-linux/* $ROOT/toolchain/
        sudo chmod 755 $ROOT/toolchain -R
        rm -rf $ROOT/.tmp_toolchain
        cd -
    fi
}

## Check cross tools
if [ ! -d $ROOT/toolchain/arm-linux-gnueabi ]; then
	git_configure
	get_toolchain
	apt -y --no-install-recommends --fix-missing install \
	bsdtar mtools u-boot-tools pv bc \
	gcc automake make \
	chmod 755 -R $ROOT/toolchain/*
fi

root_check()
{
	if [ "$(id -u)" -ne "0" ]; then
		echo "This option requires root."
		echo "Pls use command: sudo ./build.sh"
		exit 0
	fi	
}

UBOOT_check()
{
	for ((i = 0; i < 5; i++)); do
		UBOOT_PATH=$(whiptail --title "RDA8810 Build System" \
			--inputbox "Pls input device node of SDcard.(/dev/sdb)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "RDA8810 Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -b "$UBOOT_PATH" ]; then
			whiptail --title "RDA8810 Build System" --msgbox \
				"The input path invalid! Pls input correct path!" \
				--ok-button Continue 10 40 0	
		else
			i=200 
		fi 
	done
}

BOOT_check()
{
	## Get mount path of u-disk
	for ((i = 0; i < 5; i++)); do
		BOOT_PATH=$(whiptail --title "RDA8810 Build System" \
			--inputbox "Pls input mount path of BOOT.(/media/RDA8810/BOOT)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "RDA8810 Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -d "$BOOT_PATH" ]; then
			whiptail --title "RDA8810 Build System" --msgbox \
				"The input path invalid! Pls input correct path!" \
				--ok-button Continue 10 40 0	
		else
			i=200 
		fi 
	done
}

ROOTFS_check()
{
	for ((i = 0; i < 5; i++)); do
		ROOTFS_PATH=$(whiptail --title "RDA8810 Build System" \
			--inputbox "Pls input mount path of rootfs.(/media/RDA8810/rootfs)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "RDA8810 Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -d "$ROOTFS_PATH" ]; then
			whiptail --title "RDA8810 Build System" --msgbox \
				"The input path invalid! Pls input correct path!" \
				--ok-button Continue 10 40 0	
		else
			i=200 
		fi 
	done
}

if [ ! -d $ROOT/output ]; then
    mkdir -p $ROOT/output
fi

##########################################
## Root Password check
for ((i = 0; i < 5; i++)); do
	PASSWD=$(whiptail --title "Build RDA8810 System" \
		--passwordbox "Enter root password instead of using sudo to run this!" \
		10 60 3>&1 1>&2 2>&3)
	
	if [ $i = "4" ]; then
		whiptail --title "Note Box" --msgbox "Error, Invalid password" 10 40 0	
		exit 0
	fi

	sudo -k
	if sudo -lS &> /dev/null << EOF
$PASSWD
EOF
	then
		i=10
	else
		whiptail --title "RDA8810 Build System" --msgbox "Invalid password, Pls input corrent password" \
			10 40 0	--cancel-button Exit --ok-button Retry
	fi
done

echo $PASSWD | sudo ls &> /dev/null 2>&1



if [ ! -d $ROOT/output ]; then
    mkdir -p $ROOT/output
fi

MENUSTR="Pls select build option"

OPTION=$(whiptail --title "RDA8810 Build System" \
	--menu "$MENUSTR" 20 60 6 --cancel-button Finish --ok-button Select \
	"0"   "Build Linux" \
	"1"   "Build Kernel only" \
	"2"   "Build Module only" \
	"3"   "Build Uboot" \
	"4"   "Install Uboot" \
	3>&1 1>&2 2>&3)

if [ $OPTION = "0" ]; then
	cd $SCRIPTS
	./kernel_compile.sh "1"
	exit 0
elif [ $OPTION = "1" ]; then
	cd $SCRIPTS
	./kernel_compile.sh "2"
	exit 0
elif [ $OPTION = "2" ]; then
	cd $SCRIPTS
	./kernel_compile.sh "3"
	exit 0
elif [ $OPTION = "3" ]; then
	cd $SCRIPTS
	./uboot_compile.sh 
	exit 0
elif [ $OPTION = "4" ]; then
	cd $SCRIPTS
	./uboot_update.sh 
	exit 0
else
	whiptail --title "RDA8810 Build System" \
		--msgbox "Pls select correct option" 10 50 0
	exit 0
fi
