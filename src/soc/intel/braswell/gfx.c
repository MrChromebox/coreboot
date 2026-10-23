/* SPDX-License-Identifier: GPL-2.0-only */

#include "chip.h"
#include <acpi/acpi.h>
#include <bootmode.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <drivers/intel/gma/i915.h>
#include <drivers/intel/gma/libgfxinit.h>
#include <drivers/intel/gma/opregion.h>
#include <reg_script.h>
#include <soc/gfx.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>

static const struct reg_script gpu_pre_vbios_script[] = {
	/* Make sure GFX is bus master with MMIO access */
	REG_PCI_OR32(PCI_COMMAND, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY),
	REG_SCRIPT_END
};

static const struct reg_script gfx_post_vbios_script[] = {
	/* Set Lock bits */
	REG_PCI_RMW32(GGC,      0xffffffff, GGC_GGCLCK),
	REG_PCI_RMW32(GSM_BASE, 0xffffffff, GSM_BDSM_LOCK),
	REG_PCI_RMW32(GTT_BASE, 0xffffffff, GTT_BGSM_LOCK),
	REG_SCRIPT_END
};

static inline void gfx_run_script(struct device *dev, const struct reg_script *ops)
{
	reg_script_run_on_dev(dev, ops);
}

static void gfx_pre_vbios_init(struct device *dev)
{
	printk(BIOS_INFO, "GFX: Pre VBIOS Init\n");
	gfx_run_script(dev, gpu_pre_vbios_script);
}

static void gfx_post_vbios_init(struct device *dev)
{
	printk(BIOS_INFO, "GFX: Post VBIOS Init\n");
	gfx_run_script(dev, gfx_post_vbios_script);
}

static void gfx_init(struct device *dev)
{
	intel_gma_init_igd_opregion();

	if (CONFIG(RUN_FSP_GOP))
		return;

	/* Pre VBIOS Init */
	gfx_pre_vbios_init(dev);

	if (CONFIG(MAINBOARD_USE_LIBGFXINIT)) {
		if (!acpi_is_wakeup_s3() && display_init_required()) {
			int lightup_ok;
			gma_gfxinit(&lightup_ok);
			gfx_set_init_done(lightup_ok);
		}
	} else {
		/* Run VBIOS */
		pci_dev_init(dev);
	}

	/* Post VBIOS Init */
	gfx_post_vbios_init(dev);
}

static void gma_generate_ssdt(const struct device *dev)
{
	const struct soc_intel_braswell_config *chip = dev->chip_info;

	drivers_intel_gma_displays_ssdt_generate(&chip->gfx);
}

static struct device_operations gfx_device_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= gfx_init,
	.ops_pci		= &soc_pci_ops,
	.acpi_fill_ssdt		= gma_generate_ssdt,
};

static const unsigned short gfx_device_ids[] = {
	PCI_DID_INTEL_BSW_IGD_0,
	PCI_DID_INTEL_BSW_IGD_1,
	PCI_DID_INTEL_BSW_IGD_2,
	PCI_DID_INTEL_BSW_IGD_3,
	0
};

static const struct pci_driver gfx_driver __pci_driver = {
	.ops		= &gfx_device_ops,
	.vendor		= PCI_VID_INTEL,
	.devices	= gfx_device_ids,
};
