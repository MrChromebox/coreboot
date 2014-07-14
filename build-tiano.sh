#!/bin/bash
#

set -e
if [ -z "$1" ]; then
	build_targets=("auron_paine" "auron_yuna" "falco" "gandof" \
    		"guado" "leon" "lulu" "mccloud" "monroe" "panther" \
    		"parrot_ivb" "parrot_snb" "peppy" "rikku" "samus" \
    		"tidus" "tricky" "wolf" "zako");
else
	build_targets=($@)
fi

for device in ${build_targets[@]}
do
	filename="coreboot_tiano-${device}-mrchromebox_`date +"%Y%m%d"`.rom"
	rm -f ~/firmware/${filename}*
	rm -rf ./build
	cp configs/.config-${device}-tiano .config
	make
	cp ./build/coreboot.rom ./${filename}
	cbfstool ${filename} print
	md5sum ${filename} > ${filename}.md5
	mv ${filename}* ~/firmware/
	if [ "${device}" == "peppy" ]; then
		filename="coreboot_tiano-${device}_elan-mrchromebox_`date +"%Y%m%d"`.rom"
		rm -rf ./build
		sed -i 's/# CONFIG_ELAN_TRACKPAD_ACPI is not set/CONFIG_ELAN_TRACKPAD_ACPI=y/' .config
		make
		cp ./build/coreboot.rom ./${filename}
		cbfstool ${filename} print
		md5sum ${filename} > ${filename}.md5
		mv ${filename}* ~/firmware/
	fi
done
