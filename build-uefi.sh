#!/bin/bash
#

set -e

platforms=('snb_ivb' 'hsw' 'byt' 'bdw' 'bsw' 'skl' 'apl' 'kbl' 'whl' 'glk' \
           'cml' 'jsl' 'tgl' 'str')
build_targets=()

json_file=cbmodels.json
rom_path=https://www.mrchromebox.tech/files/firmware/full_rom/
echo -e "{" > $json_file

if [ -z "$1" ]; then
	for subdir in "${platforms[@]}"; do
		for cfg in configs/$subdir/config*.*; do
			build_targets+=("$(basename $cfg | cut -f2 -d'.')")
		done
	done
else
	build_targets=($@)
fi

for device in "${build_targets[@]}"; do
	filename="coreboot_tiano-${device}-mrchromebox_$(date +"%Y%m%d").rom"
	rm -f ~/dev/firmware/${filename}*
	rm -rf ./build
	cfg_file=$(find ./configs -name "config.$device.uefi")
	cp "$cfg_file" .config
	make clean
	make olddefconfig
	make -j$(nproc)
	cp ./build/coreboot.rom ./${filename}
	sha1sum ${filename} > ${filename}.sha1
	echo -e "\t\"${device}\": {" >> $json_file
	echo -e "\t\t\"url\": \"${rom_path}${filename}\"," >> $json_file
	echo -e "\t\t\"sha1\": \"$(cat ${filename}.sha1 | awk 'NR==1{print $1}')\"" >> $json_file
	echo -e "\t}," >> $json_file
	if [ -d ~/dev/firmware/ ]
	then
		mv ${filename}* ~/dev/firmware/
	fi
done
echo -e "}" >> $json_file
