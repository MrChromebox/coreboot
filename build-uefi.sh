#!/bin/bash
#

set -e

hsw=("falco" "leon" "mccloud" "monroe" "panther" "peppy" "tricky" "wolf" \
	"zako");
bdw=("auron_paine" "auron_yuna" "gandof" "guado" "lulu" "rikku" "samus" \
	"tidus");
byt=("banjo" "candy" "clapper" "enguarde" "glimmer" "gnawty" "heli" \
	"kip" "ninja" "orco" "quawks" "squawks" "sumo" "swanky" "winky");
snb_ivb=("butterfly" "link" "lumpy" "parrot_ivb" "parrot_snb" "stout" "stumpy")
skl=("chell" "sentry")

if [ -z "$1" ]; then
	build_targets=($(printf "%s " "${hsw[@]}" "${bdw[@]}" "${byt[@]}" "${snb_ivb[@]}" "${skl[@]}"));
else
	build_targets=($@)
fi

for device in ${build_targets[@]}
do
	filename="coreboot_tiano-${device}-mrchromebox_`date +"%Y%m%d"`.rom"
	rm -f ~/firmware/${filename}*
	rm -rf ./build
	cp configs/.config.${device}.uefi .config
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
