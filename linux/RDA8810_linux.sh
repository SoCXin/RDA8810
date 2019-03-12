#!/bin/bash

# This script is used to build RDA8810 environment.
# by: Qitas
# Date: 2019-03-11

if [ -z $TOP_ROOT ]; then
    TOP_ROOT=`pwd`
fi

# Github
kernel_GITHUB="https://github.com/orangepi-xunlong/OrangePiRDA_kernel.git"
uboot_GITHUB="https://github.com/orangepi-xunlong/OrangePiRDA_uboot.git"
scripts_GITHUB="https://github.com/orangepi-xunlong/OrangePiRDA_scripts.git"
external_GITHUB="https://github.com/orangepi-xunlong/OrangePiRDA_external.git"
toolchain="https://codeload.github.com/orangepi-xunlong/OrangePiH3_toolchain/zip/master"

# Prepare dirent
Prepare_dirent=(
kernel
uboot
external
scripts
)


if [ ! -d $TOP_ROOT/RDA8810 ]; then
    mkdir $TOP_ROOT/RDA8810
fi
# Download Source Code from Github
function download_Code()
{
    for dirent in ${Prepare_dirent[@]}; do
        echo -e "\e[1;31m Download $dirent from Github \e[0m"
        if [ ! -d $TOP_ROOT/RDA8810/$dirent ]; then
            cd $TOP_ROOT/RDA8810
            GIT="${dirent}_GITHUB"
            echo -e "\e[1;31m Github: ${!GIT} \e[0m"
            git clone --depth=1 ${!GIT}
            mv $TOP_ROOT/RDA8810/OrangePiRDA_${dirent} $TOP_ROOT/RDA8810/${dirent}
        else
            cd $TOP_ROOT/RDA8810/${dirent}
            git pull
        fi
    done
}

function dirent_check() 
{
    for ((i = 0; i < 100; i++)); do

        if [ $i = "99" ]; then
            whiptail --title "Note Box" --msgbox "Please ckeck your network" 10 40 0
            exit 0
        fi
        
        m="none"
        for dirent in ${Prepare_dirent[@]}; do
            if [ ! -d $TOP_ROOT/RDA8810/$dirent ]; then
                cd $TOP_ROOT/RDA8810
                GIT="${dirent}_GITHUB"
                git clone --depth=1 ${!GIT}
                mv $TOP_ROOT/RDA8810/OrangePiRDA_${dirent} $TOP_ROOT/RDA8810/${dirent}
                m="retry"
            fi
        done
        if [ $m = "none" ]; then
            i=200
        fi
    done
}

function end_op()
{
    if [ ! -f $TOP_ROOT/RDA8810/build.sh ]; then
        ln -s $TOP_ROOT/RDA8810/scripts/build.sh $TOP_ROOT/RDA8810/build.sh    
    fi
}

function git_configure()
{
    export GIT_CURL_VERBOSE=1
    export GIT_TRACE_PACKET=1
    export GIT_TRACE=1    
}

function install_toolchain()
{
    if [ ! -d $TOP_ROOT/RDA8810/toolchain/arm-linux-gnueabi ]; then
        mkdir -p $TOP_ROOT/RDA8810/.tmp_toolchain
        cd $TOP_ROOT/RDA8810/.tmp_toolchain
        curl -C - -o ./toolchain $toolchain
        unzip $TOP_ROOT/RDA8810/.tmp_toolchain/toolchain
        mkdir -p $TOP_ROOT/RDA8810/toolchain
        mv $TOP_ROOT/RDA8810/.tmp_toolchain/OrangePiH3_toolchain-master/* $TOP_ROOT/RDA8810/toolchain/
        sudo chmod 755 $TOP_ROOT/RDA8810/toolchain -R
        rm -rf $TOP_ROOT/RDA8810/.tmp_toolchain
        cd -
    fi
}

git_configure
download_Code
dirent_check
install_toolchain
end_op

whiptail --title "RDA8810 Build System" --msgbox \
 "`figlet RDA8810` Succeed to Create RDA8810 Build System!        Path:$TOP_ROOT/RDA8810" \
             15 50 0
clear
