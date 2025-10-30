#!/usr/bin/env bash
#

set -e

platforms=('snb_ivb' 'hsw' 'byt' 'bdw' 'bsw' 'skl' 'apl' 'kbl' 'whl' 'glk' \
           'cml' 'jsl' 'tgl' 'adl' 'adl_n' 'mtl' 'str' 'pco' 'czn' 'mdn')
build_targets=()
debug_mode=false
cbmem_mode=false

output_folder="../roms"
mkdir -p ${output_folder}

# Parse command line arguments
for arg in "$@"; do
	if [ "$arg" = "--debug" ]; then
		debug_mode=true
	elif [ "$arg" = "--cbmem" ]; then
		cbmem_mode=true
	else
		build_targets+=("$arg")
	fi
done

# If no build targets specified, build all configs
if [ ${#build_targets[@]} -eq 0 ]; then
	for subdir in "${platforms[@]}"; do
		for cfg in configs/"$subdir"/config*.*; do
			build_targets+=("$(basename "$cfg" | cut -f2 -d'.')")
		done
	done
fi

# get git rev
rev=$(git describe --tags --dirty)

for device in "${build_targets[@]}"; do
	if [ "$debug_mode" = true ]; then
		filename="coreboot_edk2-${device}-mrchromebox_debug_$(date +"%Y%m%d").rom"
	else
		filename="coreboot_edk2-${device}-mrchromebox_$(date +"%Y%m%d").rom"
	fi
	rm -f ${output_folder}/"${filename}"*
	rm -rf ./build
	cfg_file=$(find ./configs -name "config.$device.uefi")
	cp "$cfg_file" .config
	echo "CONFIG_LOCALVERSION=\"${rev}\"" >> .config
	if [ "$debug_mode" = true ]; then
		echo "CONFIG_CONSOLE_SERIAL=y" >> .config
		echo "CONFIG_EDK2_DEBUG=y" >> .config
	fi
	if [ "$cbmem_mode" = true ]; then
		echo "CONFIG_EDK2_CBMEM_LOGGING=y" >> .config
	fi
	make clean
	make olddefconfig
	if ! make -j"$(nproc)"; then
		echo -e "Error building $device"
		exit 1
	fi
	cp ./build/coreboot.rom ./"${filename}"
	sha1sum "${filename}" > "${filename}.sha1"
	mv "${filename}"* "${output_folder}"
done
