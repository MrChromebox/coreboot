#!/bin/bash
#
set -e

hsw=("falco" "leon" "mccloud" "monroe" "panther" "peppy" "tricky" "wolf" "zako");
bdw=("auron_paine" "auron_yuna" "gandof" "guado" "lulu" "rikku" "samus" "tidus");
byt=("banjo" "candy" "clapper" "enguarde" "glimmer" "gnawty" "heli" \
	"kip" "ninja" "orco" "quawks" "squawks" "sumo" "swanky" "winky");
snb_ivb=("link" "parrot_ivb" "parrot_snb" "stumpy");

if [ -z "$1" ]; then
	build_targets=($(printf "%s " "${hsw[@]}" "${bdw[@]}" "${byt[@]}" "${snb_ivb[@]}"));
else
	build_targets=($@);
fi

for device in ${build_targets[@]}
do
	filename="coreboot_seabios-${device}-mrchromebox_`date +"%Y%m%d"`.rom"
	rm -f ~/dev/firmware/${filename}*
	rm -rf ./build
	cp configs/.config.${device}.legacy .config
	make
	cp ./build/coreboot.rom ./${filename}

	if [[ "${hsw[@]}" =~ "$device" || "${bdw[@]}" =~ "$device" ]]; then
		cbfstool ${filename} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
		cbfstool ${filename} add -f ./cbfs/links.hswbdw -n links -t raw
	elif [[ "${byt[@]}" =~ "$device" ]]; then
		cbfstool ${filename} add -f ./cbfs/bootorder.emmc -n bootorder -t raw
		cbfstool ${filename} add-int -i 0xd071f000 -n etc/sdcard0
		cbfstool ${filename} add-int -i 0xd071c000 -n etc/sdcard1
		cbfstool ${filename} add-int -i 0xd081f000 -n etc/sdcard2
		cbfstool ${filename} add-int -i 0xd081c000 -n etc/sdcard3
	elif [[ "${snb_ivb[@]}" =~ "$device" ]]; then
		cbfstool ${filename} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
		cbfstool ${filename} add -f ./cbfs/links.sbib -n links -t raw
	fi

	cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
	cbfstool ${filename} print
	sha1sum ${filename} > ${filename}.sha1
	mv ${filename}* ~/dev/firmware/

	#special case peppy trackpad type
	if [ "${device}" == "peppy" ]; then
		filename="coreboot_seabios-peppy_elan-mrchromebox_`date +"%Y%m%d"`.rom"
		rm -rf ./build
		sed -i 's/# CONFIG_ELAN_TRACKPAD_ACPI is not set/CONFIG_ELAN_TRACKPAD_ACPI=y/' .config
		make
		cp ./build/coreboot.rom ./${filename}
		cbfstool ${filename} add -f ./cbfs/bootorder.ssd -n bootorder -t raw
		cbfstool ${filename} add -f ./cbfs/links.hswbdw -n links -t raw
		cbfstool ${filename} add-int -i 3000 -n etc/boot-menu-wait
		cbfstool ${filename} print
		sha1sum ${filename} > ${filename}.sha1
		mv ${filename}* ~/dev/firmware/
	fi
done
