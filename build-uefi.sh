#!/usr/bin/env bash
#

set -euo pipefail
shopt -s nullglob

platforms=('snb_ivb' 'hsw' 'byt' 'bdw' 'bsw' 'skl' 'apl' 'kbl' 'whl' 'glk' \
           'cml' 'jsl' 'tgl' 'adl' 'adl_n' 'mtl' 'str' 'pco' 'czn' 'mdn')
build_targets=()
debug_mode=false
edk2_pxe=false
edk2_ipxe=false
cbmem_mode=false
build_all=false

usage() {
	echo "Usage: $0 [--debug] [--cbmem] [--pxe] [--build-all] <board> [board ...]"
	echo "  --debug      Enable debug build (coreboot and edk2)"
	echo "  --cbmem      Enable EDK2 cbmem logging"
	echo "               Cannot be used with --debug"
	echo "  --pxe        Enable EDK2 PXE network support"
	echo "  --ipxe       Enable EDK2 iPXE network support"
	echo "  --build-all  Build all supported boards"
}

output_folder="../roms"
mkdir -p "${output_folder}"

resolve_cfg_file() {
	local device="$1"
	local matches=(configs/*/"config.${device}.uefi")

	if [ ${#matches[@]} -eq 0 ]; then
		echo "Unknown board: ${device} (no matching config found)" >&2
		return 1
	fi

	if [ ${#matches[@]} -gt 1 ]; then
		echo "Ambiguous board: ${device} (multiple matching configs found)" >&2
		printf ' - %s\n' "${matches[@]}" >&2
		return 1
	fi

	printf '%s\n' "${matches[0]}"
}

# Parse command line arguments
for arg in "$@"; do
	if [ "$arg" = "--debug" ]; then
		debug_mode=true
	elif [ "$arg" = "--cbmem" ]; then
		cbmem_mode=true
	elif [ "$arg" = "--pxe" ]; then
		edk2_pxe=true
	elif [ "$arg" = "--ipxe" ]; then
		edk2_ipxe=true
	elif [ "$arg" = "--build-all" ]; then
		build_all=true
	elif [ "$arg" = "--help" ] || [ "$arg" = "-h" ]; then
		usage
		exit 0
	elif [[ "$arg" == --* ]]; then
		echo "Unknown option: $arg"
		usage
		exit 1
	else
		build_targets+=("$arg")
	fi
done

if [ "$debug_mode" = true ] && [ "$cbmem_mode" = true ]; then
	echo "Error: --debug and --cbmem cannot be used together." >&2
	usage
	exit 1
fi

# If requested, build all configs
if [ "$build_all" = true ]; then
	for subdir in "${platforms[@]}"; do
		for cfg in configs/"$subdir"/config.*.uefi; do
			build_targets+=("$(basename "$cfg" | cut -f2 -d'.')")
		done
	done
fi

if [ ${#build_targets[@]} -eq 0 ]; then
	usage
	exit 1
fi

# get git rev
rev=$(git describe --tags --dirty)

for device in "${build_targets[@]}"; do
	if [ "$debug_mode" = true ]; then
		filename="coreboot_edk2-${device}-mrchromebox_debug_$(date +"%Y%m%d").rom"
	else
		filename="coreboot_edk2-${device}-mrchromebox_$(date +"%Y%m%d").rom"
	fi
	rm -f "${output_folder}/${filename}"*
	rm -rf ./build
	cfg_file="$(resolve_cfg_file "$device")"
	cp "$cfg_file" .config
	echo "CONFIG_LOCALVERSION=\"${rev}\"" >> .config
	if [ "$debug_mode" = true ]; then
		echo "CONFIG_CONSOLE_SERIAL=y" >> .config
		echo "CONFIG_EDK2_DEBUG=y" >> .config
	fi
	if [ "$cbmem_mode" = true ]; then
		echo "CONFIG_EDK2_CBMEM_LOGGING=y" >> .config
	fi
	if [ "$edk2_pxe" = true ]; then
		echo "CONFIG_EDK2_NETWORK_PXE_SUPPORT=y" >> .config
	fi
	if [ "$edk2_ipxe" = true ]; then
		echo "CONFIG_EDK2_ENABLE_IPXE=y" >> .config
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
