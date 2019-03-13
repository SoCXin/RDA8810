#!/bin/bash

#bootloader layout

#normal bootloader(for pagesize_is_power_of_2)
#################################
#                               #
# 48K (spl.img + padding 0)     #
#                               #
#                               #
#################################
#                               #
#                               #
# 24K resevered for part table  #
#     (fill 0)                  #
#                               #
#################################
#                               #
#                               #
# u-boot                        #
#                               #
#                               #
#################################

#pagesize not aligned bootloader(pagesize is 12288, not power_of_2)
#e.g. 12k pagesize
#################################
#                               #
# spl(1st 2k)                   #
#                               #
#                               #
#################################
#                               #
# 10K (padding with 0xff)       #
#                               #
#                               #
#################################
#                               #
# spl(2nd 2k)                   #
#                               #
#                               #
#################################
#                               #
# 10K (padding with 0xff)       #
#                               #
#                               #
#################################
#                               #
# ....                          #
#                               #
#                               #
#################################
#                               #
# spl(24th 2k)                  #
#                               #
#                               #
#################################
#                               #
# 10K (padding with 0xff)       #
#                               #
#                               #
#################################
#                               #
#                               #
# 24K resevered for part table  #
#     (fill 0)                  #
#                               #
#################################
#                               #
#                               #
# u-boot                        #
#                               #
#                               #
#################################
pagesize=$1
spl_image=$2
uboot_image=$3
spl_append_to=$4
output_image=$5
#expand the spl to whole 48k
dd if=/dev/zero ibs=1k count=48 of=spl-padding.img obs=1k conv=sync  >& /dev/null
dd if=$spl_image of=spl-padding.img bs=2k conv=notrunc >& /dev/null

temp_image=tmp_image
if [ $pagesize -eq 12288 ]
then :
	#segments is the max_sizeof_spl(48K) / 2k(romcode's real pagesize)
	segments=24
	i=0
	iblock=0
	oblock=0
	tr "\000" "\377" < /dev/zero | dd ibs=12k count=26 of=$temp_image conv=sync
	while [ $i -lt $segments ]
	do
		#echo "read " "spl-padding.img" " pos " $iblock
		#echo "write to " $temp_image " pos " $oblock
		dd if=spl-padding.img of=$temp_image ibs=1k skip=$iblock obs=1k\
			seek=$oblock count=2 conv=notrunc >& /dev/null
		i=$[$i + 1]
		iblock=$[$iblock + 2]
		oblock=$[$oblock + 12]
	done
	#echo "write to " $temp_image " pos " $oblock
	dd if=/dev/zero of=$temp_image ibs=1k obs=1k seek=$oblock count=24 \
			conv=notrunc >& /dev/null
else #this is normal case
	dd if=/dev/zero ibs=1k count=$spl_append_to of=$temp_image obs=1k \
			conv=sync >& /dev/null
	dd if=spl-padding.img of=$temp_image bs=2k conv=notrunc >& /dev/null
fi
cat $temp_image $uboot_image > $output_image
rm -rf $temp_image spl-padding.img

