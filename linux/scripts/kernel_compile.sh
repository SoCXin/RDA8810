#!/bin/bash
set -e

if [ -z ${ROOT} ]; then
    ROOT=`cd .. & pwd`
fi
KERNEL=${ROOT}/kernel
TOOL=${ROOT}/toolchain/bin/arm-linux-gnueabi-
OUTPUT=${ROOT}/output

clear
# Import kernel configure
if [ ! -f ${KERNEL}/.config ]; then
    make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${TOOL} OrangePi_2G-IOT_defconfig
fi

if [ $1 = "0" ]; then
    make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${TOOL} clean
fi

# Compile kernel
if [ $1 = "2" -o $1 = "1" ]; then
    echo -e "\e[1;31m Compiling Kernel \e[0m"
    make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${TOOL} zImage 
    make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${TOOL} headers_install O=${OUTPUT} 
fi

# Compile modules
if [ $1 = "3" -o $1 = "1" ]; then
    echo -e "\e[1;31m Compile Modules \e[0m"
    make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${TOOL} modules
    make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${TOOL} modules_install INSTALL_MOD_PATH=${OUTPUT}
fi

# Install zImage and modules 
cp -rfa ${ROOT}/kernel/arch/arm/boot/zImage ${ROOT}/output/

clear
whiptail --title "RDA8810 Build System" \
        --msgbox "Comiple finish! kernel path: ${ROOT}/output/zImage  Module Path: ${ROOT}/output/lib" \
        12 60 0 --ok-button Continue
