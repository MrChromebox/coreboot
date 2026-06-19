/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/cpu.h>
#include <device/device.h>
#include <mainboard/ramstage.h>
#include <smbios.h>

/* Match the vendor firmware's SMBIOS where the value is deterministic and not
 * a per-unit string sourced from the EC/flash (serial, UUID, SKU, version). */

smbios_wakeup_type smbios_system_wakeup_type(void)
{
	/* Type 1: vendor reports "Power Switch" instead of the default
	 * SMBIOS_WAKEUP_TYPE_RESERVED. */
	return SMBIOS_WAKEUP_TYPE_POWER_SWITCH;
}

smbios_enclosure_type smbios_mainboard_enclosure_type(void)
{
	/* Type 3: vendor reports "Notebook" (0x0a). SYSTEM_TYPE_LAPTOP would
	 * otherwise yield "Laptop" (0x09). */
	return SMBIOS_ENCLOSURE_NOTEBOOK;
}

u8 smbios_mainboard_feature_flags(void)
{
	/* Type 2 feature flags: vendor sets "hosting board" (bit 0) and
	 * "replaceable" (bit 3). */
	return (1 << 0) | (1 << 3);
}

unsigned int smbios_processor_family(struct cpuid_result res)
{
	/* Type 4: there is no SMBIOS family code for Core Ultra, so the
	 * generic CPUID-based mapping reports the bogus "Pentium Pro". The
	 * vendor reports "Other"; match it. */
	return SMBIOS_PROCESSOR_FAMILY_OTHER;
}

/*
 * Type 8: the four USB-C ports. The vendor exposes one connector per port
 * (JTYPEC0..3), all "Access Bus (USB)" / port type USB.
 */
static const struct port_information smbios_type8_info[] = {
	{
		.internal_reference_designator = "JTYPEC0",
		.internal_connector_type = CONN_NONE,
		.external_reference_designator = "USB",
		.external_connector_type = CONN_ACCESS_BUS_USB,
		.port_type = TYPE_USB,
	},
	{
		.internal_reference_designator = "JTYPEC1",
		.internal_connector_type = CONN_NONE,
		.external_reference_designator = "USB",
		.external_connector_type = CONN_ACCESS_BUS_USB,
		.port_type = TYPE_USB,
	},
	{
		.internal_reference_designator = "JTYPEC2",
		.internal_connector_type = CONN_NONE,
		.external_reference_designator = "USB",
		.external_connector_type = CONN_ACCESS_BUS_USB,
		.port_type = TYPE_USB,
	},
	{
		.internal_reference_designator = "JTYPEC3",
		.internal_connector_type = CONN_NONE,
		.external_reference_designator = "USB",
		.external_connector_type = CONN_ACCESS_BUS_USB,
		.port_type = TYPE_USB,
	},
};

int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	return smbios_write_type8(current, handle, smbios_type8_info,
				  ARRAY_SIZE(smbios_type8_info));
}
