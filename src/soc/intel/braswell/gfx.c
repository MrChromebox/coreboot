/*
 * This file is part of the coreboot project.
 *
 * Copyright 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
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

#include "chip.h"
#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <drivers/intel/gma/opregion.h>
#include <drivers/intel/gma/i915.h>
#include <reg_script.h>
#include <soc/gfx.h>
#include <soc/nvs.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>

static const struct reg_script gpu_pre_vbios_script[] = {
	/* Make sure GFX is bus master with MMIO access */
	REG_PCI_OR32(PCI_COMMAND, PCI_COMMAND_MASTER|PCI_COMMAND_MEMORY),
	REG_SCRIPT_END
};

static const struct reg_script gfx_post_vbios_script[] = {
	/* Set Lock bits */
	REG_PCI_RMW32(GGC, 0xffffffff, GGC_GGCLCK),
	REG_PCI_RMW32(GSM_BASE, 0xffffffff, GSM_BDSM_LOCK),
	REG_PCI_RMW32(GTT_BASE, 0xffffffff, GTT_BGSM_LOCK),
	REG_SCRIPT_END
};

static inline void gfx_run_script(struct device *dev,
				  const struct reg_script *ops)
{
	reg_script_run_on_dev(dev, ops);
}

static void gfx_pre_vbios_init(struct device *dev)
{
	printk(BIOS_SPEW, "%s/%s ( %s )\n",
			__FILE__, __func__, dev_name(dev));
	printk(BIOS_INFO, "GFX: Pre VBIOS Init\n");
	gfx_run_script(dev, gpu_pre_vbios_script);
}

static void gfx_post_vbios_init(struct device *dev)
{
	printk(BIOS_SPEW, "%s/%s ( %s )\n",
			__FILE__, __func__, dev_name(dev));
	printk(BIOS_INFO, "GFX: Post VBIOS Init\n");
	gfx_run_script(dev, gfx_post_vbios_script);
}

static void gfx_init(struct device *dev)
{
	printk(BIOS_SPEW, "%s/%s ( %s )\n",
			__FILE__, __func__, dev_name(dev));

	if (!IS_ENABLED(CONFIG_RUN_FSP_GOP)) {
		/* Pre VBIOS Init */
		gfx_pre_vbios_init(dev);

		/* Run VBIOS */
		pci_dev_init(dev);

		/* Post VBIOS Init */
		gfx_post_vbios_init(dev);
	}
	intel_gma_restore_opregion();
}

uintptr_t gma_get_gnvs_aslb(const void *gnvs)
{
	const global_nvs_t *gnvs_ptr = gnvs;
	return (uintptr_t)(gnvs_ptr ? gnvs_ptr->aslb : 0);
}

void gma_set_gnvs_aslb(void *gnvs, uintptr_t aslb)
{
	global_nvs_t *gnvs_ptr = gnvs;
	if (gnvs_ptr)
		gnvs_ptr->aslb = aslb;
}

const struct i915_gpu_controller_info *
intel_gma_get_controller_info(void)
{
	device_t dev = dev_find_slot(0, PCI_DEVFN(0x2, 0));
	if (!dev) {
		return NULL;
	}
	struct soc_intel_braswell_config *chip = dev->chip_info;
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
	.acpi_fill_ssdt_generator = gma_ssdt,
	.init			= gfx_init,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver gfx_driver __pci_driver = {
	.ops	= &gfx_device_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= GFX_DEVID,
};
