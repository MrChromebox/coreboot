
/*
 * This file is part of the coreboot project.
 *
 * Copyright 2013 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/io.h>
#include <bootstate.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <reg_script.h>
#include <stdlib.h>
#include <soc/gfx.h>
#include <soc/iosf.h>
#include <soc/pmc.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>
#include <cbmem.h>
#include <cbfs.h>
#include <types.h>
#include <string.h>

#include "chip.h"

#define GFX_TIMEOUT 100000 /* 100ms */

/*
 * Lock Power Context Base Register to point to a 24KB block
 * of memory in GSM.  Power context save data is stored here.
 */
static void gfx_lock_pcbase(device_t dev)
{
	struct resource *res = find_resource(dev, PCI_BASE_ADDRESS_0);
	const u16 gms_size_map[17] = { 0,32,64,96,128,160,192,224,256,
				       288,320,352,384,416,448,480,512 };
	u32 pcsize = 24 << 10;  /* 24KB */
	u32 wopcmsz = 0x100000; /* PAVP offset */
	u32 gms, gmsize, pcbase;

	gms = pci_read_config32(dev, GGC) & GGC_GSM_SIZE_MASK;
	gms >>= 3;
	if (gms > ARRAY_SIZE(gms_size_map))
		return;
	gmsize = gms_size_map[gms];

	/* PcBase = BDSM + GMS Size - WOPCMSZ - PowerContextSize */
	pcbase = pci_read_config32(dev, GSM_BASE) & 0xfff00000;
	pcbase += (gmsize-1) * wopcmsz - pcsize;
	pcbase |= 1; /* Lock */

	write32((u32 *)(uintptr_t)(res->base + 0x182120), pcbase);
}

static const struct reg_script gfx_init_script[] = {
	/* Allow-Wake render/media wells */
	REG_RES_RMW32(PCI_BASE_ADDRESS_0, 0x130090, ~1, 1),
	REG_RES_POLL32(PCI_BASE_ADDRESS_0, 0x130094, 1, 1, GFX_TIMEOUT),
	/* Render Force-Wake */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x1300b0, 0x80008000),
	REG_RES_POLL32(PCI_BASE_ADDRESS_0, 0x1300b4, 0x8000, 0x8000,
	               GFX_TIMEOUT),
	/* Media Force-Wake */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x1300b8, 0x80008000),
	REG_RES_POLL32(PCI_BASE_ADDRESS_0, 0x1300bc, 0x8000, 0x8000,
	               GFX_TIMEOUT),
	/* Workaround - X0:261954/A0:261955 */
	REG_RES_RMW32(PCI_BASE_ADDRESS_0, 0x182060, ~0xf, 1),

	/*
	 * PowerMeter Weights
	 */

	/* SET1 */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA800, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA804, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA808, 0x0000ff0A),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA80C, 0x1D000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA810, 0xAC004900),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA814, 0x000F0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA818, 0x5A000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA81C, 0x2600001F),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA820, 0x00090000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA824, 0x2000ff00),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA828, 0xff090016),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA82C, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA830, 0x00000100),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA834, 0x00A00F51),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA838, 0x000B0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA83C, 0xcb7D3307),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA840, 0x003C0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA844, 0xFFFF0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA848, 0x00220000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA84c, 0x43000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA850, 0x00000800),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA854, 0x00000F00),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA858, 0x00000021),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA85c, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA860, 0x00190000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xAA80, 0x00FF00FF),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xAA84, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0x1300A4, 0x00000000),
	/* SET2 */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA900, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA904, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA908, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa90c, 0x1D000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa910, 0xAC005000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa914, 0x000F0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa918, 0x5A000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa91c, 0x2600001F),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa920, 0x00090000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa924, 0x2000ff00),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa928, 0xff090016),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa92c, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa930, 0x00000100),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa934, 0x00A00F51),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xa938, 0x000B0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA93C, 0xcb7D3307),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA940, 0x003C0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA944, 0xFFFF0000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA948, 0x00220000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA94C, 0x43000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA950, 0x00000800),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA954, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA960, 0x00000000),
	/* SET3 */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xaa3c, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xaa54, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xaa60, 0x00000000),
	/* Enable PowerMeter Counters */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA248, 0x00000058),

	/* Program PUNIT_GPU_EC_VIRUS based on DPTF SDP */
	/* SDP Profile 4 == 0x11940, others 0xcf08 */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_GPU_EC_VIRUS, 0xcf08),

	/* GfxPause */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa000, 0x00071388),

	/* Dynamic EU Control Settings */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa080, 0x00000004),

	/* Lock ECO Bit Settings */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa180, 0x80000000),

	/* DOP Clock Gating */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x9424, 0x00000001),

	/* MBCunit will send the VCR (Fuse) writes as NP-W */
	REG_RES_RMW32(PCI_BASE_ADDRESS_0, 0x907c, 0xfffeffff, 0x00010000),

	/*
	 * RC6 Settings
	 */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA090, 0x00000000),
	/* RC1e - RC6/6p - RC6pp Wake Rate Limits */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA09C, 0x00280000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA0A8, 0x0001E848),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA0AC, 0x00000019),
	/* RC Sleep / RCx Thresholds */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA0B0, 0x00000000),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA0B8, 0x00000557),

	/*
	 * Turbo Settings
	 */

	/* Render/Video/Blitter Idle Max Count */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0x2054,  0x0000000A),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0x12054, 0x0000000A),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0x22054, 0x0000000A),
	/* RP Down Timeout */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA010,  0x000F4240),

	/*
	 * Turbo Control Settings
	 */

	/* RP Up/Down Threshold */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA02C, 0x0000E8E8),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA030, 0x0003BD08),
	/* RP Up/Down EI */
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA068, 0x000101D0),
	REG_RES_OR32(PCI_BASE_ADDRESS_0, 0xA06C, 0x00055730),

	/* RP Idle Hysteresis */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa070, 0x0000000a),

	/* HW RC6 Control Settings */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa090, 0x11000000),

	/* RP Control */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa024, 0x00000592),

	/* Enable PM Interrupts */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x44024, 0x03000000),
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x4402c, 0x03000076),
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0xa168, 0x0000007e),

	/* Aggressive Clock Gating */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x9400, 0),
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x9404, 0),
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x9408, 0),
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x940c, 0),

	/* Enable Gfx Turbo. */
	REG_IOSF_RMW(IOSF_PORT_PMC, SB_BIOS_CONFIG,
			~SB_BIOS_CONFIG_GFX_TURBO_DIS, 0),
	REG_SCRIPT_END
};

static const struct reg_script gpu_pre_vbios_script[] = {
	/* Make sure GFX is bus master with MMIO access */
	REG_PCI_OR32(PCI_COMMAND, PCI_COMMAND_MASTER|PCI_COMMAND_MEMORY),
	/* Display */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_PWRGT_CONTROL, 0xc0),
	REG_IOSF_POLL(IOSF_PORT_PMC, PUNIT_PWRGT_STATUS, 0xc0, 0xc0,
	              GFX_TIMEOUT),
	/* Tx/Rx Lanes */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_PWRGT_CONTROL, 0xfff0c0),
	REG_IOSF_POLL(IOSF_PORT_PMC, PUNIT_PWRGT_STATUS, 0xfff0c0, 0xfff0c0,
	              GFX_TIMEOUT),
	/* Common Lane */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_PWRGT_CONTROL, 0xfffcc0),
	REG_IOSF_POLL(IOSF_PORT_PMC, PUNIT_PWRGT_STATUS, 0xfffcc0, 0xfffcc0,
	              GFX_TIMEOUT),
	/* Ungating Tx only */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_PWRGT_CONTROL, 0xf00cc0),
	REG_IOSF_POLL(IOSF_PORT_PMC, PUNIT_PWRGT_STATUS, 0xfffcc0, 0xf00cc0,
	              GFX_TIMEOUT),
	/* Ungating Common Lane only */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_PWRGT_CONTROL, 0xf000c0),
	REG_IOSF_POLL(IOSF_PORT_PMC, PUNIT_PWRGT_STATUS, 0xffffc0, 0xf000c0,
	              GFX_TIMEOUT),
	/* Ungating Display */
	REG_IOSF_WRITE(IOSF_PORT_PMC, PUNIT_PWRGT_CONTROL, 0xf00000),
	REG_IOSF_POLL(IOSF_PORT_PMC, PUNIT_PWRGT_STATUS, 0xfffff0, 0xf00000,
	              GFX_TIMEOUT),
	REG_SCRIPT_END
};

static const struct reg_script gfx_post_vbios_script[] = {
	/* Deassert Render Force-Wake */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x1300b0, 0x80000000),
	REG_RES_POLL32(PCI_BASE_ADDRESS_0, 0x1300b4, 0x8000, 0, GFX_TIMEOUT),
	/* Deassert Media Force-Wake */
	REG_RES_WRITE32(PCI_BASE_ADDRESS_0, 0x1300b8, 0x80000000),
	REG_RES_POLL32(PCI_BASE_ADDRESS_0, 0x1300bc, 0x8000, 0, GFX_TIMEOUT),
	/* Set Lock bits */
	REG_PCI_RMW32(GGC, 0xffffffff, 1),
	REG_PCI_RMW32(GSM_BASE, 0xffffffff, 1),
	REG_PCI_RMW32(GTT_BASE, 0xffffffff, 1),
	REG_SCRIPT_END
};

static inline void gfx_run_script(device_t dev, const struct reg_script *ops)
{
	reg_script_run_on_dev(dev, ops);
}

static void gfx_pre_vbios_init(device_t dev)
{
	printk(BIOS_INFO, "GFX: Pre VBIOS Init\n");
	gfx_run_script(dev, gpu_pre_vbios_script);
}

static void gfx_pm_init(device_t dev)
{
	printk(BIOS_INFO, "GFX: Power Management Init\n");
	gfx_run_script(dev, gfx_init_script);

	/* Lock power context base */
	gfx_lock_pcbase(dev);
}

static void gfx_post_vbios_init(device_t dev)
{
	printk(BIOS_INFO, "GFX: Post VBIOS Init\n");
	gfx_run_script(dev, gfx_post_vbios_script);
}

static void set_backlight_pwm(device_t dev, uint32_t bklt_reg, int req_hz)
{
	int divider;
	struct resource *res;

	res = find_resource(dev, PCI_BASE_ADDRESS_0);

	if (res == NULL)
		return;

	/* Default to 200 Hz if nothing is set. */
	if (req_hz == 0)
		req_hz = 200;

	/* Base clock is 25MHz */
	divider = 25 * 1000 * 1000 / (16 * req_hz);

	/* Do not set duty cycle (lower 16 bits). Just set the divider. */
	write32((u32 *)(uintptr_t)(res->base + bklt_reg), divider << 16);
}

static void gfx_panel_setup(device_t dev)
{
	struct soc_intel_baytrail_config *config = dev->chip_info;
	struct reg_script gfx_pipea_init[] = {
		/* CONTROL */
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, PIPEA_REG(PP_CONTROL),
				PP_CONTROL_UNLOCK | PP_CONTROL_EDP_FORCE_VDD),
		/* POWER ON */
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, PIPEA_REG(PP_ON_DELAYS),
				(config->gpu_pipea_port_select << 30 |
				 config->gpu_pipea_power_on_delay << 16 |
				 config->gpu_pipea_light_on_delay)),
		/* POWER OFF */
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, PIPEA_REG(PP_OFF_DELAYS),
				(config->gpu_pipea_power_off_delay << 16 |
				 config->gpu_pipea_light_off_delay)),
		/* DIVISOR */
		REG_RES_RMW32(PCI_BASE_ADDRESS_0, PIPEA_REG(PP_DIVISOR),
			      ~0x1f, config->gpu_pipea_power_cycle_delay),
		REG_SCRIPT_END
	};
	struct reg_script gfx_pipeb_init[] = {
		/* CONTROL */
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, PIPEB_REG(PP_CONTROL),
				PP_CONTROL_UNLOCK | PP_CONTROL_EDP_FORCE_VDD),
		/* POWER ON */
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, PIPEB_REG(PP_ON_DELAYS),
				(config->gpu_pipeb_port_select << 30 |
				 config->gpu_pipeb_power_on_delay << 16 |
				 config->gpu_pipeb_light_on_delay)),
		/* POWER OFF */
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, PIPEB_REG(PP_OFF_DELAYS),
				(config->gpu_pipeb_power_off_delay << 16 |
				 config->gpu_pipeb_light_off_delay)),
		/* DIVISOR */
		REG_RES_RMW32(PCI_BASE_ADDRESS_0, PIPEB_REG(PP_DIVISOR),
			      ~0x1f, config->gpu_pipeb_power_cycle_delay),
		REG_SCRIPT_END
	};

	if (config->gpu_pipea_port_select) {
		printk(BIOS_INFO, "GFX: Initialize PIPEA\n");
		reg_script_run_on_dev(dev, gfx_pipea_init);
		set_backlight_pwm(dev, PIPEA_REG(BACKLIGHT_CTL),
		                  config->gpu_pipea_pwm_freq_hz);
	}

	if (config->gpu_pipeb_port_select) {
		printk(BIOS_INFO, "GFX: Initialize PIPEB\n");
		reg_script_run_on_dev(dev, gfx_pipeb_init);
		set_backlight_pwm(dev, PIPEB_REG(BACKLIGHT_CTL),
		                  config->gpu_pipeb_pwm_freq_hz);
	}
}

static void gfx_init(device_t dev)
{
	/* Pre VBIOS Init */
	gfx_pre_vbios_init(dev);

	/* Power Management Init */
	gfx_pm_init(dev);

	gfx_panel_setup(dev);

	/* Run VBIOS */
	pci_dev_init(dev);

	/* Post VBIOS Init */
	gfx_post_vbios_init(dev);
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

static void igd_finalize_opregion(void *unused)
{
	device_t igd;
	igd_opregion_t *opregion;
	u16 reg16;

	igd = dev_find_slot(0, PCI_DEVFN(0x2, 0));
	opregion = cbmem_find(CBMEM_ID_IGD_OPREGION);

	if (!opregion)
		return;

	pci_write_config32(igd, ASLS, (u32)opregion);
	reg16 = pci_read_config16(igd, SWSCI);
	reg16 &= ~(1 << 0);
	reg16 |= (1 << 15);
	pci_write_config16(igd, SWSCI, reg16);
}

/* Initialize IGD OpRegion, called from ACPI code */
int init_igd_opregion(igd_opregion_t *opregion)
{
	memset((void *)opregion, 0, sizeof(igd_opregion_t));

	// FIXME if IGD is disabled, we should exit here.

	memcpy(&opregion->header.signature, IGD_OPREGION_SIGNATURE,
		sizeof(opregion->header.signature));

	/* 8kb */
	opregion->header.size = sizeof(igd_opregion_t) / 1024;

	opregion->header.version = (IGD_OPREGION_VERSION << 24);

	// For BYT, MailBOX2(SCI)is not supported.
	opregion->header.mailboxes = (IGD_MBOX1 | IGD_MBOX3 | IGD_MBOX4);

	//FIXME: Value copied
	opregion->header.pcon = 20;

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

	if (vbt){
		opregion->header.dver[0] = '7';
		opregion->header.dver[1] = '.';
		opregion->header.dver[2] = '0';
		opregion->header.dver[3] = '.';
		opregion->header.dver[4] = '1';
		opregion->header.dver[5] = '0';
		opregion->header.dver[6] = '1';
		opregion->header.dver[7] = '1';
		opregion->header.dver[8] = '\0';

		memcpy(opregion->vbt.gvd1, vbt, vbt->hdr_vbt_size <
		sizeof(opregion->vbt.gvd1) ? vbt->hdr_vbt_size :
		sizeof(opregion->vbt.gvd1));
	}

	/* Set PCI registers */
	igd_finalize_opregion((void *)opregion);

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

const struct i915_gpu_controller_info *
intel_gma_get_controller_info(void)
{
	device_t dev = dev_find_slot(0, PCI_DEVFN(0x2,0));
	if (!dev) {
		return NULL;
	}
	struct soc_intel_baytrail_config *chip = dev->chip_info;
	return &chip->gfx;
}

static void gma_ssdt(device_t device)
{
	const struct i915_gpu_controller_info *gfx = intel_gma_get_controller_info();
	if (gfx && IS_ENABLED(CONFIG_INTEL_GMA_ACPI)) {
		drivers_intel_gma_displays_ssdt_generate(gfx);
	}
}

static struct device_operations gfx_device_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= gfx_init,
	.acpi_fill_ssdt_generator = gma_ssdt,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver gfx_driver __pci_driver = {
	.ops	= &gfx_device_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= GFX_DEVID,
};

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, igd_finalize_opregion, NULL);
