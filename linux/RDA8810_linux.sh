#!/bin/bash

# This script is used to build RDA8810 Linux.
# by: Qitas
# Date: 2019-03-12

if [ -z $TOP_ROOT ]; then
    TOP_ROOT=`pwd`
fi

# Github
#toolchain="https://codeload.github.com/orangepi-xunlong/arm-linux-eabi/zip/master"
toolchain="https://codeload.github.com/sochub/arm-linux-eabi/zip/master"

if [ ! -d $TOP_ROOT/RDA8810 ]; then
    mkdir $TOP_ROOT/RDA8810
fi

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
        mv $TOP_ROOT/RDA8810/.tmp_toolchain/arm-linux-eabi-master/* $TOP_ROOT/RDA8810/toolchain/
        sudo chmod 755 $TOP_ROOT/RDA8810/toolchain -R
        rm -rf $TOP_ROOT/RDA8810/.tmp_toolchain
        cd -
    fi
}

git_configure
install_toolchain

whiptail --title "RDA8810 Build System" --msgbox \
 "`figlet RDA8810` Succeed to Create RDA8810 Build System!        Path:$TOP_ROOT/RDA8810" \
             15 50 0
clear
