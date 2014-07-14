#!/bin/bash
#
set -e
filename="coreboot_seabios-quawks-mrchromebox_`date +"%Y%m%d"`.rom"
rm -f ~/firmware/${filename}*
rm -rf ./build
cp configs/.config-quawks .config
make
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} add -f ./cbfs/bootorder.emmc -n bootorder -t raw
cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
cbfstool ${filename} add-int -i 0xd071f000 -n etc/sdcard0
cbfstool ${filename} add-int -i 0xd071d000 -n etc/sdcard1
cbfstool ${filename} add-int -i 0xd071c000 -n etc/sdcard2
cbfstool ${filename} add-int -i 0xd081f000 -n etc/sdcard3
cbfstool ${filename} add-int -i 0xd081c000 -n etc/sdcard4
cbfstool ${filename} add-int -i 0xd091f000 -n etc/sdcard5
cbfstool ${filename} add-int -i 0xd091c000 -n etc/sdcard6
cbfstool ${filename} print
md5sum ${filename} > ${filename}.md5
mv ${filename}* ~/firmware/