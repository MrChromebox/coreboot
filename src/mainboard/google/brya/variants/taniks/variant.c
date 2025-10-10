/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/variants.h>
#include <device/device.h>
#include <option.h>
#include <sar.h>
#include <types.h>

enum storage_device {
	STORAGE_NVME = 0,
	STORAGE_EMMC = 1,
};

const char *get_wifi_sar_cbfs_filename(void)
{
	return "wifi_sar_0.hex";
}

void variant_devtree_update(void)
{
	/* Get device references for both storage options */
	struct device *nvme_dev = pcidev_on_root(0x06, 0);  /* CPU PCIe RP1 for NVMe */
	struct device *emmc_dev = pcidev_on_root(0x1d, 0);  /* PCH PCIe RP9 for eMMC */

	/* Get the selected storage device from CMOS */
	uint8_t storage_selection = get_uint_option("storage_device", STORAGE_NVME);

	/* Disable the non-selected storage device */
	switch (storage_selection) {
	case STORAGE_NVME:
		if (emmc_dev)
			emmc_dev->enabled = 0;
		break;
	case STORAGE_EMMC:
		if (nvme_dev)
			nvme_dev->enabled = 0;
		break;
	}
}
