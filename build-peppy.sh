#!/bin/bash
#
set -e
filename="coreboot_seabios-peppy-mrchromebox_`date +"%Y%m%d"`.rom"
filename2="coreboot_seabios_duet-peppy-mrchromebox_`date +"%Y%m%d"`.rom"
filename3="coreboot_seabios-peppy_elan-mrchromebox_`date +"%Y%m%d"`.rom"
filename4="coreboot_seabios_duet-peppy_elan-mrchromebox_`date +"%Y%m%d"`.rom"
rm -f ~/firmware/${filename}*
rm -f ~/firmware/${filename2}*
rm -f ~/firmware/${filename3}*
rm -f ~/firmware/${filename4}*
# build for Cypress trackpad
rm -rf ./build
cp configs/.config-peppy .config
make
if [ "$1" != "uefi" ]; then
	cp ./build/coreboot.rom ./${filename}
	cbfstool ${filename} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
	cbfstool ${filename} add -f ./cbfs/links.hswbdw -n links -t raw
	cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
	cbfstool ${filename} print
	md5sum ${filename} > ${filename}.md5
	mv ${filename}* ~/firmware/
fi
if [ "$1" != "legacy" ]; then
	cp ./build/coreboot.rom ./${filename2}
	cbfstool ${filename2} remove -n fallback/payload
	cbfstool ${filename2} remove -n pci8086,0406.rom
	cbfstool ${filename2} add-payload -n fallback/payload -f ./seabios-hswbdw-duet.bin.elf -c lzma
	cbfstool ${filename2} add -f ./cbfs/show-boot-menu-duet -n etc/show-boot-menu -t raw
	cbfstool ${filename2} add -f ./cbfs/bootorder.floppy -n bootorder -t raw
	cbfstool ${filename2} add -f ./duet-hswbdw-book-ssd.img -n floppyimg/tianocore.img.lzma -t raw -c lzma
	cbfstool ${filename2} print
	md5sum ${filename2} > ${filename2}.md5
	mv ${filename2}* ~/firmware/
fi
# build for Elan trackpad
rm -rf ./build
sed -i 's/# CONFIG_ELAN_TRACKPAD_ACPI is not set/CONFIG_ELAN_TRACKPAD_ACPI=y/' .config
make
if [ "$1" != "uefi" ]; then
	cp ./build/coreboot.rom ./${filename3}
	cbfstool ${filename3} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
	cbfstool ${filename3} add -f ./cbfs/links.hswbdw -n links -t raw
	cbfstool ${filename3} add-int -i 3000 -n etc/boot-menu-wait
	cbfstool ${filename3} print
	md5sum ${filename3} > ${filename3}.md5
	mv ${filename3}* ~/firmware/
fi
if [ "$1" != "legacy" ]; then
	cp ./build/coreboot.rom ./${filename4}
	cbfstool ${filename4} remove -n fallback/payload
	cbfstool ${filename4} remove -n pci8086,0406.rom
	cbfstool ${filename4} add-payload -n fallback/payload -f ./seabios-hswbdw-duet.bin.elf -c lzma
	cbfstool ${filename4} add -f ./cbfs/show-boot-menu-duet -n etc/show-boot-menu -t raw
	cbfstool ${filename4} add -f ./cbfs/bootorder.floppy -n bootorder -t raw
	cbfstool ${filename4} add -f ./duet-hswbdw-book-ssd.img -n floppyimg/tianocore.img.lzma -t raw -c lzma
	cbfstool ${filename4} print
	md5sum ${filename4} > ${filename4}.md5
	mv ${filename4}* ~/firmware/
fi