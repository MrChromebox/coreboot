#!/bin/bash
#
filename="coreboot-haswell-chromebox-`date +"%Y%m%d"`-md.rom"
filename2="coreboot-haswell-chromebox-headless-`date +"%Y%m%d"`-md.rom"
rm -f ${filename}*
rm -rf ./build
cp .config-panther .config
make
if [ $? -eq 0 ]; then
dd if=./3rdparty/mainboard/google/panther/descriptor.bin of=./build/coreboot.rom conv=notrunc
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} add -f ./cbfs/boot-menu-wait -n etc/boot-menu-wait -t raw
cbfstool ${filename} add -f ./cbfs/boot-menu-key -n etc/boot-menu-key -t raw
cbfstool ${filename} add -f ./cbfs/boot-menu-message -n etc/boot-menu-message -t raw
cbfstool ${filename} add -f ./cbfs/links -n links -t raw
cbfstool ${filename} print
md5sum ${filename} > ${filename}.md5
cp ${filename} ${filename2}
cbfstool ${filename2} remove -n pci8086,0406.rom
cbfstool ${filename2} add -f ./3rdparty/mainboard/google/panther/hsw_1035_cbox_headless.dat -n pci8086,0406.rom -t optionrom
md5sum ${filename2} > ${filename2}.md5
fi

