#!/bin/bash
#
set -e
[[ "$1" == "uefi" ]] && exit
filename="coreboot_seabios-swanky-mrchromebox_`date +"%Y%m%d"`.rom"
rm -f ~/firmware/${filename}*
rm -rf ./build
cp configs/.config-swanky .config
make
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} add -f ./cbfs/bootorder.emmc -n bootorder -t raw
cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
cbfstool ${filename} add-int -i 0xd071f000 -n etc/sdcard0
cbfstool ${filename} add-int -i 0xd071c000 -n etc/sdcard1
cbfstool ${filename} print
md5sum ${filename} > ${filename}.md5
mv ${filename}* ~/firmware/