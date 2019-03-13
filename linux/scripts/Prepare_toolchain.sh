#!/bin/sh

set -e

if ! hash apt-get 2>/dev/null; then
	whiptail --title "RDA8810 Build System" --msgbox "This scripts requires a Debian based distrbution. If you not use Debian/Ubunut, pls install:[ bsdtar mtools u-boot-tools pv bc sunxi-tools gcc automake make qemu dosfstools ]"
	exit 1
fi

apt-get -y --no-install-recommends --fix-missing install \
	bsdtar mtools u-boot-tools pv bc \
	gcc automake make \

# Prepare toolchains
ROOT=`cd .. && pwd`
chmod 755 -R $ROOT/toolchain/*
