#!/usr/bin/env bash
#

set -e

rm -rf ./build || true
rm .config || true
make clean
cat > .config << 'EOF'
CONFIG_BOARD_EMULATION_QEMU_X86_Q35=y
# CONFIG_VPD is not set
CONFIG_EDK2_TAG_OR_REV="origin/uefipayload_rwl"
CONFIG_EDK2_DISABLE_TPM=y
# CONFIG_SMMSTORE is not set
# CONFIG_EDK2_HAVE_EFI_SHELL is not set
# CONFIG_EDK2_PRIORITIZE_INTERNAL is not set
CONFIG_EDK2_CUSTOM_BUILD_PARAMS="-DPS2_MOUSE_ENABLE=FALSE"
EOF

make olddefconfig
if ! make -j"$(nproc)"; then
        echo -e "Error building RWL payload"
        exit 1
fi
cp ./build/UEFIPAYLOAD.fd ./rwl_payload.fd
