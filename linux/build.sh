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


if [ -z $TOP_ROOT ]; then
    TOP_ROOT=`pwd`
fi

toolchain="https://codeload.github.com/sochub/arm-linux-eabi/zip/master"

function install_toolchain()
{
    if [ ! -d $TOP_ROOT/toolchain/arm-linux-gnueabi ]; then
        mkdir -p $TOP_ROOT/.tmp_toolchain
        cd $TOP_ROOT/.tmp_toolchain
        curl -C - -o ./toolchain $toolchain
        unzip $TOP_ROOT/.tmp_toolchain/toolchain
        mkdir -p $TOP_ROOT/toolchain
        mv $TOP_ROOT/.tmp_toolchain/arm-linux-eabi-master/* $TOP_ROOT/toolchain/
        sudo chmod 755 $TOP_ROOT/toolchain -R
        rm -rf $TOP_ROOT/RDA8810/.tmp_toolchain
        cd -
    fi
}

git_configure
install_toolchain

whiptail --title "RDA8810 Build System" --msgbox \
 "`figlet RDA8810` Succeed to Create RDA8810 Build System!        Path:$TOP_ROOT" \
             15 50 0
clear

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
		UBOOT_PATH=$(whiptail --title "OrangePi Build System" \
			--inputbox "Pls input device node of SDcard.(/dev/sdb)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "OrangePi Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -b "$UBOOT_PATH" ]; then
			whiptail --title "OrangePi Build System" --msgbox \
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
		BOOT_PATH=$(whiptail --title "OrangePi Build System" \
			--inputbox "Pls input mount path of BOOT.(/media/orangepi/BOOT)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "OrangePi Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -d "$BOOT_PATH" ]; then
			whiptail --title "OrangePi Build System" --msgbox \
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
		ROOTFS_PATH=$(whiptail --title "OrangePi Build System" \
			--inputbox "Pls input mount path of rootfs.(/media/orangepi/rootfs)" \
			10 60 3>&1 1>&2 2>&3)
	
		if [ $i = "4" ]; then
			whiptail --title "OrangePi Build System" --msgbox "Error, Invalid Path" 10 40 0	
			exit 0
		fi


		if [ ! -d "$ROOTFS_PATH" ]; then
			whiptail --title "OrangePi Build System" --msgbox \
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

export PLATFORM="OrangePi_2G-IOT"
##########################################
## Root Password check
for ((i = 0; i < 5; i++)); do
	PASSWD=$(whiptail --title "OrangePi Build System" \
		--passwordbox "Enter your root password. Note! Don't use root to run this scripts" \
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
		whiptail --title "OrangePi Build System" --msgbox "Invalid password, Pls input corrent password" \
			10 40 0	--cancel-button Exit --ok-button Retry
	fi
done

echo $PASSWD | sudo ls &> /dev/null 2>&1

## Check cross tools
if [ ! -f $ROOT/output/.tmp_toolchain ]; then
    cd $SCRIPTS
    sudo ./Prepare_toolchain.sh
    sudo touch $ROOT/output/.tmp_toolchain
    cd -
fi

if [ ! -d $ROOT/output ]; then
    mkdir -p $ROOT/output
fi

MENUSTR="Pls select build option"

OPTION=$(whiptail --title "OrangePi Build System" \
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
	whiptail --title "OrangePi Build System" \
		--msgbox "Pls select correct option" 10 50 0
	exit 0
fi
