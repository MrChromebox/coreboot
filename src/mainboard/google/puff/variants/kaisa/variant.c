/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/variants.h>
#include <chip.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include <soc/pci_devs.h>
#include <static.h>

#define KAISA_NVME_SKU_ID	0x01000006

void variant_devtree_update(void)
{
	struct device *emmc_host;
	config_t *cfg = config_of_soc();
	const uint32_t sku_id = google_chromeec_get_board_sku();

	/*
	 * Kaisa SKU 0x01000006 uses NVMe storage. Hide the unused onboard
	 * eMMC controller while keeping the separate SD/TF reader enabled.
	 */
	if (sku_id != KAISA_NVME_SKU_ID)
		return;

	emmc_host = pcidev_path_on_root(PCH_DEVFN_EMMC);
	if (!emmc_host) {
		printk(BIOS_WARNING, "Kaisa: eMMC device not found\n");
		return;
	}

	printk(BIOS_INFO, "Kaisa: disable eMMC controller for NVMe SKU %#x\n", sku_id);
	emmc_host->enabled = 0;
	cfg->ScsEmmcHs400Enabled = 0;
}
