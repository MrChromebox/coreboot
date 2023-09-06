/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi_device.h>
#include <acpi/acpigen.h>
#include <amdblocks/acp.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/chip.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <commonlib/helpers.h>
#include "acp_def.h"

_Static_assert(!(CONFIG(SOC_AMD_COMMON_BLOCK_ACP_GEN1) && CONFIG(SOC_AMD_COMMON_BLOCK_ACP_GEN2)),
	"Cannot select both ACP_GEN1 and ACP_GEN2 - check your config");

static const char *acp_acpi_name(const struct device *dev)
{
	return "ACPD";
}

static void acp_fill_wov_method(const struct device *dev)
{
	const struct soc_amd_common_config *cfg = soc_get_common_config();
	const char *scope = acpi_device_path(dev);

	if (!cfg->acp_config.dmic_present || !scope)
		return;

	/* For ACP DMIC hardware runtime detection on the platform, _WOV method is populated. */
	acpigen_write_scope(scope); /* Scope */
	acpigen_write_method("_WOV", 0);
	acpigen_write_return_integer(1);
	acpigen_write_method_end();
	acpigen_write_scope_end();
}

static void acp_fill_ssdt(const struct device *dev)
{
	acpi_device_write_pci_dev(dev);
	acp_fill_wov_method(dev);

	if (CONFIG(SOC_AMD_CEZANNE)) {
		/*
		 * Add an interface to the host bridge for CZN
		 *
		 * On CZN, the ACP drivers need to notify the PSP after the DSP
		 * firmware has been loaded, so the PSP can validate the fw and
		 * set the qualifiier bit to enable running it. Under Linux, there
		 * is an existing interface to do so, but under Windows, this ACPI
		 * workaround is needed.
		 *
		 *	Scope (\_SB.PCI0.GP41.ACPD)
		 *	{
		 *		Method (SMNR, 1, NotSerialized)
		 *		{
		 *			\_SB.PCI0.GNB.SMNA = Arg0
		 *			Return (\_SB.PCI0.GNB.SMND)
		 *		}
		 *
		 *		Method (SMNW, 2, NotSerialized)
		 *		{
		 *			\_SB.PCI0.GNB.SMNA = Arg0
		 *			\_SB.PCI0.GNB.SMND = Arg1
		 *		}
		 *	}
		 */
		acpigen_write_scope(acpi_device_path(dev));
		acpigen_write_method("SMNR", 1);
		acpigen_write_store_op_to_namestr(ARG0_OP, "\\_SB.PCI0.GNB.SMNA");
		acpigen_write_return_namestr("\\_SB.PCI0.GNB.SMND");
		acpigen_write_method_end();
		acpigen_write_method("SMNW", 2);
		acpigen_write_store_op_to_namestr(ARG0_OP, "\\_SB.PCI0.GNB.SMNA");
		acpigen_write_store_op_to_namestr(ARG1_OP, "\\_SB.PCI0.GNB.SMND");
		acpigen_write_method_end();
		acpigen_write_scope_end();

	}
}

struct device_operations amd_acp_ops = {
	.read_resources = pci_dev_read_resources,
	.set_resources = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init = acp_init,
	.ops_pci = &pci_dev_ops_pci,
	.scan_bus = scan_static_bus,
	.acpi_name = acp_acpi_name,
	.acpi_fill_ssdt = acp_fill_ssdt,
};
