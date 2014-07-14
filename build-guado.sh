#!/bin/bash
#
set -e
filename="coreboot_seabios-guado-mrchromebox_`date +"%Y%m%d"`.rom"
filename2="coreboot_seabios_duet-guado-mrchromebox_`date +"%Y%m%d"`.rom"
rm -f ~/firmware/${filename}*
rm -rf ./build
cp configs/.config-guado .config
make
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
cbfstool ${filename} add -f ./cbfs/links.hswbdw -n links -t raw
cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
cbfstool ${filename} print
md5sum ${filename} > ${filename}.md5
mv ${filename}* ~/firmware/
cp ./build/coreboot.rom ./${filename2}
cbfstool ${filename2} remove -n fallback/payload
cbfstool ${filename2} add-payload -n fallback/payload -f ./seabios-hswbdw-duet.bin.elf -c lzma
cbfstool ${filename2} add -f ./cbfs/show-boot-menu-duet -n etc/show-boot-menu -t raw
cbfstool ${filename2} add -f ./cbfs/bootorder.floppy -n bootorder -t raw
cbfstool ${filename2} add -f ./cbfs/links.hswbdw -n links -t raw
cbfstool ${filename2} add -f ./duet-hswbdw-box.img -n floppyimg/tianocore.img.lzma -t raw -c lzma
cbfstool ${filename2} print
md5sum ${filename2} > ${filename2}.md5
mv ${filename2}* ~/firmware/