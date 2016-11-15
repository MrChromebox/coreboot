/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
 * Copyright (C) 2012 The Chromium OS Authors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <types.h>
#include <string.h>
#include <console/console.h>
#include <arch/io.h>
#include <arch/acpi.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include "haswell.h"
#include <cbmem.h>
#include <cbfs.h>
#include <arch/acpigen.h>
#include <cpu/cpu.h>
#include <drivers/intel/gma/i915.h>

unsigned long acpi_fill_mcfg(unsigned long current)
{
	device_t dev;
	u32 pciexbar = 0;
	u32 pciexbar_reg;
	int max_buses;

	dev = dev_find_slot(0, PCI_DEVFN(0, 0));
	if (!dev)
		return current;

	pciexbar_reg = pci_read_config32(dev, PCIEXBAR);

	// MMCFG not supported or not enabled.
	if (!(pciexbar_reg & (1 << 0)))
		return current;

	switch ((pciexbar_reg >> 1) & 3) {
	case 0: // 256MB
		pciexbar = pciexbar_reg & ((1 << 31)|(1 << 30)|(1 << 29)|(1 << 28));
		max_buses = 256;
		break;
	case 1: // 128M
		pciexbar = pciexbar_reg & ((1 << 31)|(1 << 30)|(1 << 29)|(1 << 28)|(1 << 27));
		max_buses = 128;
		break;
	case 2: // 64M
		pciexbar = pciexbar_reg & ((1 << 31)|(1 << 30)|(1 << 29)|(1 << 28)|(1 << 27)|(1 << 26));
		max_buses = 64;
		break;
	default: // RSVD
		return current;
	}

	if (!pciexbar)
		return current;

	current += acpi_create_mcfg_mmconfig((acpi_mcfg_mmconfig_t *) current,
			pciexbar, 0x0, 0x0, max_buses - 1);

	return current;
}

static void *get_intel_vbios(void)
{
	/* This should probably be looking at CBFS or we should always
	 * deploy the VBIOS on Intel systems, even if we don't run it
	 * in coreboot (e.g. SeaBIOS only scenarios).
	 */
	u8 *vbios = (u8 *)0xc0000;

	optionrom_header_t *oprom = (optionrom_header_t *)vbios;
	optionrom_pcir_t *pcir = (optionrom_pcir_t *)(vbios +
						oprom->pcir_offset);


	printk(BIOS_DEBUG, "GET_VBIOS: %x %x %x %x %x\n",
		oprom->signature, pcir->vendor, pcir->classcode[0],
		pcir->classcode[1], pcir->classcode[2]);


	if ((oprom->signature == OPROM_SIGNATURE) &&
		(pcir->vendor == PCI_VENDOR_ID_INTEL) &&
		(pcir->classcode[0] == 0x00) &&
		(pcir->classcode[1] == 0x00) &&
		(pcir->classcode[2] == 0x03))
		return (void *)vbios;

	return NULL;
}

/* Reading VBT table from flash */
const optionrom_vbt_t *get_uefi_vbt(uint32_t *vbt_len)
{
	size_t vbt_size;
	union {
		const optionrom_vbt_t *data;
		uint32_t *signature;
	} vbt;

	/* Locate the vbt file in cbfs */
	vbt.data = cbfs_boot_map_with_leak("vbt.bin", CBFS_TYPE_RAW, &vbt_size);
	if (!vbt.data) {
		printk(BIOS_INFO,
			"FSP_INFO: VBT data file (vbt.bin) not found in CBFS");
		return NULL;
	}

	/* Validate the vbt file */
	if (*vbt.signature != VBT_SIGNATURE) {
		printk(BIOS_WARNING,
			"FSP_WARNING: Invalid signature in VBT data file (vbt.bin)!\n");
		return NULL;
	}
	*vbt_len = vbt_size;
	printk(BIOS_DEBUG, "FSP_INFO: VBT found at %p, 0x%08x bytes\n",
		vbt.data, *vbt_len);

#if IS_ENABLED(CONFIG_DISPLAY_VBT)
	/* Display the vbt file contents */
	printk(BIOS_DEBUG, "VBT Data:\n");
	hexdump(vbt.data, *vbt_len);
	printk(BIOS_DEBUG, "\n");
#endif

	/* Return the pointer to the vbt file data */
	return vbt.data;
}

static int init_opregion_vbt(igd_opregion_t *opregion)
{
	void *vbios;
	vbios = get_intel_vbios();
	if (!vbios) {
		printk(BIOS_DEBUG, "VBIOS not found.\n");
		return 1;
	}

	printk(BIOS_DEBUG, " ... VBIOS found at %p\n", vbios);
	optionrom_header_t *oprom = (optionrom_header_t *)vbios;
	optionrom_vbt_t *vbt = (optionrom_vbt_t *)(vbios +
						oprom->vbt_offset);

	if (read32(vbt->hdr_signature) != VBT_SIGNATURE) {
		printk(BIOS_DEBUG, "VBT not found!\n");
		return 1;
	}

	memcpy(opregion->header.vbios_version, vbt->coreblock_biosbuild, 4);
	memcpy(opregion->vbt.gvd1, vbt, vbt->hdr_vbt_size < 7168 ?
						vbt->hdr_vbt_size : 7168);

	return 0;
}


/* Initialize IGD OpRegion, called from ACPI code */
int init_igd_opregion(igd_opregion_t *opregion)
{
	device_t igd;
	u16 reg16;

	memset((void *)opregion, 0, sizeof(igd_opregion_t));

	// FIXME if IGD is disabled, we should exit here.

	memcpy(&opregion->header.signature, IGD_OPREGION_SIGNATURE,
		sizeof(opregion->header.signature));

	/* 8kb */
	opregion->header.size = sizeof(igd_opregion_t) / 1024;
	opregion->header.version = (IGD_OPREGION_VERSION << 24);

	// FIXME We just assume we're mobile for now
	opregion->header.mailboxes = MAILBOXES_MOBILE;

	//FIXME: Value copied
	opregion->header.pcon = 259;

	// TODO Initialize Mailbox 1
	opregion->mailbox1.clid = 1;

	// TODO Initialize Mailbox 3
	opregion->mailbox3.bclp = IGD_BACKLIGHT_BRIGHTNESS;
	opregion->mailbox3.pfit = IGD_FIELD_VALID | IGD_PFIT_STRETCH;
	opregion->mailbox3.pcft = 0; // should be (IMON << 1) & 0x3e
	opregion->mailbox3.cblv = IGD_FIELD_VALID | IGD_INITIAL_BRIGHTNESS;
	opregion->mailbox3.bclm[0] = IGD_WORD_FIELD_VALID + 0x0000;
	opregion->mailbox3.bclm[1] = IGD_WORD_FIELD_VALID + 0x0a19;
	opregion->mailbox3.bclm[2] = IGD_WORD_FIELD_VALID + 0x1433;
	opregion->mailbox3.bclm[3] = IGD_WORD_FIELD_VALID + 0x1e4c;
	opregion->mailbox3.bclm[4] = IGD_WORD_FIELD_VALID + 0x2866;
	opregion->mailbox3.bclm[5] = IGD_WORD_FIELD_VALID + 0x327f;
	opregion->mailbox3.bclm[6] = IGD_WORD_FIELD_VALID + 0x3c99;
	opregion->mailbox3.bclm[7] = IGD_WORD_FIELD_VALID + 0x46b2;
	opregion->mailbox3.bclm[8] = IGD_WORD_FIELD_VALID + 0x50cc;
	opregion->mailbox3.bclm[9] = IGD_WORD_FIELD_VALID + 0x5ae5;
	opregion->mailbox3.bclm[10] = IGD_WORD_FIELD_VALID + 0x64ff;

	const optionrom_vbt_t *vbt;
	uint32_t vbt_len;

	vbt = get_uefi_vbt(&vbt_len);

	if (!vbt)
		init_opregion_vbt(opregion);
	else {
		opregion->header.dver[0] = '5';
		opregion->header.dver[1] = '.';
		opregion->header.dver[2] = '5';
		opregion->header.dver[3] = '.';
		opregion->header.dver[4] = '1';
		opregion->header.dver[5] = '0';
		opregion->header.dver[6] = '3';
		opregion->header.dver[7] = '3';
		opregion->header.dver[8] = '\0';

		memcpy(opregion->vbt.gvd1, vbt, vbt->hdr_vbt_size <
		sizeof(opregion->vbt.gvd1) ? vbt->hdr_vbt_size :
		sizeof(opregion->vbt.gvd1));
	}

	/* TODO This needs to happen in S3 resume, too.
	 * Maybe it should move to the finalize handler
	 */
	igd = dev_find_slot(0, PCI_DEVFN(0x2, 0));

	pci_write_config32(igd, ASLS, (u32)opregion);
	reg16 = pci_read_config16(igd, SWSCI);
	reg16 &= ~(1 << 0);
	reg16 |= (1 << 15);
	pci_write_config16(igd, SWSCI, reg16);

	/* clear dmisci status */
	reg16 = inw(get_pmbase() + TCO1_STS);
	reg16 |= DMISCI_STS; // reference code does an &=
	outw(get_pmbase() + TCO1_STS, reg16);

	/* clear and enable ACPI TCO SCI */
	enable_tco_sci();

	return 0;
}

void *igd_make_opregion(void)
{
	igd_opregion_t *opregion;

	printk(BIOS_DEBUG, "ACPI:    * IGD OpRegion\n");
	opregion = cbmem_add(CBMEM_ID_IGD_OPREGION, sizeof (*opregion));
	init_igd_opregion(opregion);
	return opregion;
}