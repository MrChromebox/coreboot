#!/bin/bash
#
filename="coreboot-seabios-falco-`date +"%Y%m%d"`-md.rom"
rm -f ${filename}*
rm -rf ./build
cp .config-falco .config
make
if [ $? -eq 0 ]; then
dd if=./3rdparty/mainboard/google/peppy/descriptor.bin of=./build/coreboot.rom conv=notrunc
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} add -f ./cbfs/boot-menu-wait -n etc/boot-menu-wait -t raw
cbfstool ${filename} add -f ./cbfs/boot-menu-key -n etc/boot-menu-key -t raw
cbfstool ${filename} add -f ./cbfs/boot-menu-message -n etc/boot-menu-message -t raw
cbfstool ${filename} add -f ./cbfs/links -n links -t raw
cbfstool ${filename} print
md5sum ${filename} > ${filename}.md5
fi

