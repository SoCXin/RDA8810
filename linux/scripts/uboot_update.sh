#!/bin/bash

UBOOT_PATH=
UBOOT=${ROOT}/output/u-boot.rda

function Notice()
{
    whiptail --title "RDA8810 Build System" \
         --msgbox "Warning!! Please check the device node of UDisk! It's very necessary to check before writing data into SDcard! If not, you will write data into dangerous area, and it's very terrible for your system! So, please check device node of UDisk" 10 80 0 --ok-button Continue

    whiptail --title "RDA8810 Build System" \
                  --msgbox "`df -l`" 20 80 0 --ok-button Continue
}

function GetPath()
{
    for ((i = 0; i < 5; i++)); do
        UBOOT_PATH=$(whiptail --title "RDA8810 Build System" \
            --inputbox "Pls input device node of SDcard.(/dev/sdc)" \
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

Notice
GetPath

sudo dd bs=512 seek=256 if=${UBOOT} of=${UBOOT_PATH} && sync
clear
