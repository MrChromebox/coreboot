#!/usr/bin/env bash
#

set -e

# Optional: ./build-rwl_payload.sh cml
# CML/hatch stock ChromeOS leaves FADT PmTmrBlk at 0; enable CpuTimerLib for
# that family only. Default shared RWL keeps AcpiTimerLib (KBL/APL safe).
VARIANT="${1:-}"
OUT_FD=./rwl_payload.fd
EXTRA_PARAMS=""
case "$VARIANT" in
"" )
	;;
cml)
	EXTRA_PARAMS=" -D CPU_TIMER_LIB_ENABLE=TRUE"
	OUT_FD=./rwl_payload_cml.fd
	;;
*)
	echo "Usage: $0 [cml]" >&2
	exit 1
	;;
esac

rm -rf ./build || true
rm -f .config || true
# After wiping build/, recreate dirs before kconfig host tools compile
mkdir -p build/util/kconfig
cat > .config << EOF
CONFIG_BOARD_EMULATION_QEMU_X86_Q35=y
# CONFIG_VPD is not set
CONFIG_EDK2_TAG_OR_REV="origin/uefipayload_rwl_2609"
CONFIG_EDK2_DISABLE_TPM=y
# CONFIG_SMMSTORE is not set
# CONFIG_EDK2_HAVE_EFI_SHELL is not set
# CONFIG_EDK2_PRIORITIZE_INTERNAL is not set
# CONFIG_EDK2_DEBUG is not set
# CONFIG_EDK2_SERIAL_SUPPORT is not set
CONFIG_EDK2_CUSTOM_BUILD_PARAMS="-DPS2_MOUSE_ENABLE=FALSE${EXTRA_PARAMS}"
EOF

make olddefconfig
if ! make -j"$(nproc)"; then
        echo -e "Error building RWL payload"
        exit 1
fi
cp ./build/UEFIPAYLOAD.fd "$OUT_FD"
ls -la "$OUT_FD"
file "$OUT_FD"
