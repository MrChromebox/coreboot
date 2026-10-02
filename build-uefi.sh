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
build_release=false
show_stats=false

usage() {
	echo "Usage: $0 [options] <board> [board ...]"
	echo "Options:"
	echo "  --debug          Enable debug build (coreboot and edk2)"
	echo "  --cbmem          Enable EDK2 cbmem logging"
	echo "                   Cannot be used with --debug"
	echo "  --pxe            Enable EDK2 PXE network support (replaces iPXE)"
	echo "  --ipxe           Enable EDK2 iPXE network support"
	echo "  --build-all      Build all supported boards into ../roms/"
	echo "  --build-release  Build all supported boards into ../roms/<git-version>/"
	echo "  --show-stats     Print ccache and edk2 payload cache statistics"
}

sanitize_version() {
	local v="$1"
	v="${v//\//-}"
	printf '%s' "$v"
}

output_folder="../roms"

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
	elif [ "$arg" = "--build-release" ]; then
		build_release=true
	elif [ "$arg" = "--show-stats" ]; then
		show_stats=true
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

# get git rev
rev=$(git describe --tags --dirty)

if [ "$build_release" = true ]; then
	output_folder="../roms/$(sanitize_version "$rev")"
	build_all=true
fi

mkdir -p "${output_folder}"

# Boards with identical edk2 inputs get byte-identical payloads, so build each
# variant once per run and reuse it via CONFIG_PAYLOAD_FILE.
edk2_cache=$(mktemp -d)
trap 'rm -rf "$edk2_cache"' EXIT

# edk2 compiles with $(GCC_BIN)gcc, so a directory with gcc -> ccache routes
# it through ccache. The remaining GCC_BIN-prefixed tools must be real.
use_ccache=false
if command -v ccache >/dev/null; then
	use_ccache=true
	ccache_bin="${XDG_CACHE_HOME:-$HOME/.cache}/coreboot-edk2-ccache-bin"
	mkdir -p "$ccache_bin"
	ln -sf "$(command -v ccache)" "$ccache_bin/gcc"
	ln -sf "$(command -v gcc-ar)" "$ccache_bin/gcc-ar"
	ln -sf "$(command -v objcopy)" "$ccache_bin/objcopy"
	export GCC_BIN="$ccache_bin/"
fi

# coreboot prints per-build ccache statistics when CCACHE_STATSLOG names an
# existing file; overriding it with an empty value suppresses them. Its
# default is relative to the coreboot tree, which loses edk2's entries since
# edk2 compiles from other directories.
run_statslog="${edk2_cache}/ccache.stats"
if [ "$show_stats" = true ]; then
	stats_args=(CCACHE_STATSLOG="$PWD/build/ccache.stats")
else
	stats_args=(CCACHE_STATSLOG=)
fi
edk2_built=0
edk2_reused=0

# Hash everything the edk2 build consumes. File inputs are hashed by content,
# not path.
# --eval text is parsed before the Makefiles, so CONFIG_* may only be
# referenced in the recipe.
edk2_payload_key() {
	make -s --eval='
edk2-key:
	@printf "edk2-key:%s\n" \
		$(filter-out CONFIG_EDK2_BOOTSPLASH_FILE=%,$(EDK2_PAYLOAD_ARGS)) \
		$(if $(filter y,$(CONFIG_EDK2_ENABLE_IPXE)), \
			$(foreach v,$(sort $(filter CONFIG_IPXE_%,$(.VARIABLES))),$(v)=$($(v))) \
			$(IPXE_UART) $(IPXE_SERIAL_CONSOLE) $(CONFIG_TTYS0_BAUD)) \
		$(foreach f,$(wildcard $(call strip_quotes,$(CONFIG_EDK2_BOOTSPLASH_FILE) \
			$(if $(filter y,$(CONFIG_EDK2_ENABLE_IPXE)),$(IPXE_CONFIG_SCRIPT)))), \
			$(firstword $(shell sha1sum $(f))))
' edk2-key | sed -n 's/^edk2-key://p' | sha1sum | cut -c1-16
}

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
		sed -i '/^CONFIG_EDK2_ENABLE_IPXE=y/d' .config
		echo "CONFIG_EDK2_NETWORK_PXE_SUPPORT=y" >> .config
	fi
	if [ "$edk2_ipxe" = true ]; then
		echo "CONFIG_EDK2_ENABLE_IPXE=y" >> .config
	fi
	if [ "$use_ccache" = true ]; then
		echo "CONFIG_CCACHE=y" >> .config
	fi
	make clean
	make olddefconfig
	edk2_key="$(edk2_payload_key)"
	edk2_cached="${edk2_cache}/${edk2_key}.fd"
	if [ -f "$edk2_cached" ]; then
		echo "Reusing edk2 payload ${edk2_key}"
		sed -i "s|^CONFIG_PAYLOAD_FILE=.*|CONFIG_PAYLOAD_FILE=\"${edk2_cached}\"|" .config
		edk2_reused=$((edk2_reused + 1))
	fi
	if ! make -j"$(nproc)" "${stats_args[@]}"; then
		echo -e "Error building $device"
		exit 1
	fi
	if [ ! -f "$edk2_cached" ] && [ -f build/UEFIPAYLOAD.fd ]; then
		cp build/UEFIPAYLOAD.fd "$edk2_cached"
		edk2_built=$((edk2_built + 1))
	fi
	if [ "$show_stats" = true ] && [ -f build/ccache.stats ]; then
		cat build/ccache.stats >> "$run_statslog"
	fi
	cp ./build/coreboot.rom ./"${filename}"
	sha1sum "${filename}" > "${filename}.sha1"
	mv "${filename}"* "${output_folder}"
done

if [ "$show_stats" = true ]; then
	printf "\nedk2 payloads: %d built, %d reused across %d boards\n" \
		"$edk2_built" "$edk2_reused" "${#build_targets[@]}"
	if [ -s "$run_statslog" ]; then
		printf "\nccache statistics (all boards)\n"
		CCACHE_STATSLOG="$run_statslog" ccache --show-log-stats -v
	fi
fi
