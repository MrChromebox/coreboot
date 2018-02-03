#!/bin/bash

for filename in configs/*/config.*.uefi; do
    [ -e "$filename" ] || continue
    cp "$filename" .config
    make savedefconfig
    # remove a bunch of default things 'make defconfig' leaves in
    sed -i '/^CONFIG_DRIVER_TPM_SPI_BUS=0x1/d' defconfig
    sed -i '/^CONFIG_DRIVER_TPM_SPI_BUS=0x1/d' defconfig
    sed -i '/^CONFIG_CONSOLE_CBMEM_BUFFER_SIZE=0x20000/d' defconfig
    sed -i '/^CONFIG_EDK2_BOOT_TIMEOUT=2/d' defconfig
    sed -i '/^CONFIG_UART_PCI_ADDR=0x0/d' defconfig
    sed -i '/^CONFIG_SUBSYSTEM_VENDOR_ID=0x0000/d' defconfig
    sed -i '/^CONFIG_SUBSYSTEM_DEVICE_ID=0x0000/d' defconfig
    sed -i '/^CONFIG_I2C_TRANSFER_TIMEOUT_US=500000/d' defconfig
    sed -i '/^CONFIG_SMMSTORE_SIZE=0x40000/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_BUSES=42/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_MEM=0xc200000/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_PREFETCH_MEM=0x1c000000/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_IO=0x2000/d' defconfig
    sed -i '/^CONFIG_EDK2_SD_MMC_TIMEOUT=10/d' defconfig
    sed -i '/^CONFIG_FSP_LOC=0xfff6e000/d' defconfig
    sed -i '/^CONFIG_LINEAR_FRAMEBUFFER_MAX_HEIGHT=1600/d' defconfig
    sed -i '/^CONFIG_LINEAR_FRAMEBUFFER_MAX_WIDTH=2560/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_BUSES=8/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_MEM=0x800000/d' defconfig
    sed -i '/^CONFIG_PCIEXP_HOTPLUG_PREFETCH_MEM=0x10000000/d' defconfig
    sed -i '/^CONFIG_CBFS_MCACHE_RW_PERCENTAGE=50/d' defconfig
    sed -i '/^CONFIG_VBOOT_KEYBLOCK_VERSION=1/d' defconfig
    sed -i '/^CONFIG_VBOOT_KEYBLOCK_PREAMBLE_FLAGS=0x0/d' defconfig
    sed -i '/^CONFIG_AMD_FWM_POSITION_INDEX=2/d' defconfig
    sed -i '/^CONFIG_ELOG_BOOT_COUNT_CMOS_OFFSET=144/d' defconfig
    sed -i '/^CONFIG_CBFS_SIZE=0x01000000/d' defconfig
    sed -i '/^CONFIG_AGESA_BINARY_PI_LOCATION=0xFFE00000/d' defconfig
    sed -i '/^CONFIG_BOTTOMIO_POSITION=0xD0000000/d' defconfig
    sed -i '/^CONFIG_AMD_FWM_POSITION_INDEX=1/d' defconfig
    sed -i '/^CONFIG_CBFS_SIZE=0x01000000/d' defconfig

    cp defconfig "$filename"
done
