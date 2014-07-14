#!/bin/bash
#
set -e
[[ "$1" == "uefi" ]] && exit
filename="coreboot_seabios-stumpy-mrchromebox_`date +"%Y%m%d"`.rom"
rm -f ~/firmware/${filename}*
rm -rf ./build
cp configs/.config-stumpy .config
make
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
cbfstool ${filename} add -f ./cbfs/links.sbib -n links -t raw
cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
cbfstool ${filename} print
md5sum ${filename} > ${filename}.md5
mv ${filename}* ~/firmware/