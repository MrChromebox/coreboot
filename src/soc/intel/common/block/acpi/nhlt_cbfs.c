/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_gnvs.h>
#include <cbfs.h>
#include <commonlib/bsd/helpers.h>
#include <commonlib/endian.h>
#include <console/console.h>
#include <fw_config.h>
#include <intelblocks/cfg.h>
#include <intelblocks/nhlt_cbfs.h>
#include <soc/nvs.h>
#include <string.h>

bool intel_nhlt_validate(const void *data, size_t size)
{
	const uint8_t *blob = data;
	size_t pos = sizeof(acpi_header_t) + 1;
	uint8_t sum = 0;

	if (!blob || size < pos || size > 65536 || memcmp(blob, "NHLT", 4) ||
	    read_le32(blob + 4) != size || blob[8] > 1 ||
	    !blob[sizeof(acpi_header_t)])
		return false;
	for (size_t i = 0; i < size; i++)
		sum += blob[i];
	if (sum)
		return false;

	for (unsigned int i = 0; i < blob[sizeof(acpi_header_t)]; i++) {
		if (size - pos < 24)
			return false;
		size_t length = read_le32(blob + pos);
		if (length < 24 || length > size - pos)
			return false;
		size_t end = pos + length;
		size_t caps = read_le32(blob + pos + 19);
		if (caps > length - 24)
			return false;
		size_t cursor = pos + 23 + caps;
		unsigned int formats = blob[cursor++];
		if (!formats)
			return false;
		for (unsigned int j = 0; j < formats; j++) {
			/* WAVEFORMATEXTENSIBLE (40) followed by capability size (4). */
			if (end - cursor < 44)
				return false;
			size_t settings = read_le32(blob + cursor + 40);
			cursor += 44;
			if (settings > end - cursor)
				return false;
			cursor += settings;
		}
		/* A version-specific device-info list may follow the formats. */
		pos = end;
	}
	/* Both coreboot's absent tail and length-prefixed OEM data are valid. */
	return pos == size || (size - pos >= 4 && read_le32(blob + pos) == size - pos - 4);
}

const char *__weak intel_nhlt_sof_cbfs_name(const char *platform)
{
	return NULL;
}

const char *intel_nhlt_cbfs_name(void)
{
	const struct nhlt_cbfs_config *config;
	const char *name = NULL;
	bool has_allowlist = false, allowed = false;

	if (!CONFIG(SOC_INTEL_NHLT_SOF_SELECT))
		return "intel-board.nhlt";

	config = &chip_get_common_soc_structure()->nhlt;
	if (config->disabled)
		return NULL;

	for (size_t i = 0; i < ARRAY_SIZE(config->allowed); i++) {
		if (!config->allowed[i])
			continue;
		has_allowlist = true;
		if (CONFIG(FW_CONFIG) && fw_config_is_provisioned() &&
		    fw_config_probe(config->allowed[i]))
			allowed = true;
	}
	if (has_allowlist && !allowed)
		return NULL;

	for (size_t i = 0; i < ARRAY_SIZE(config->overrides); i++) {
		const struct nhlt_cbfs_override *override = &config->overrides[i];

		if (!override->match || !CONFIG(FW_CONFIG) ||
		    !fw_config_is_provisioned() || !fw_config_probe(override->match))
			continue;
		if (name || !override->cbfs_name || !override->cbfs_name[0]) {
			printk(BIOS_ERR, "NHLT: invalid or ambiguous table overrides\n");
			return NULL;
		}
		name = override->cbfs_name;
	}
	if (name)
		return name;

	return intel_nhlt_sof_cbfs_name(config->platform);
}

unsigned long intel_nhlt_write_from_cbfs(unsigned long current, acpi_rsdp_t *rsdp)
{
	struct global_nvs *gnvs = acpi_get_gnvs();
	size_t size;
	void *blob;
	unsigned long start;
	const char *name;

	if (!gnvs) {
		printk(BIOS_ERR, "NHLT: GNVS unavailable\n");
		return current;
	}
	/* Preserve an earlier board publisher and avoid a duplicate table. */
	if (gnvs->nhla || gnvs->nhll) {
		printk(BIOS_INFO, "NHLT: existing board table retained\n");
		return current;
	}
	name = intel_nhlt_cbfs_name();
	if (!name) {
		printk(BIOS_ERR, "NHLT: no unambiguous board audio table selected\n");
		return current;
	}
	blob = cbfs_map(name, &size);
	if (!blob) {
		printk(BIOS_ERR, "NHLT: table %s missing from CBFS\n", name);
		return current;
	}
	if (!intel_nhlt_validate(blob, size)) {
		printk(BIOS_ERR, "NHLT: invalid board table\n");
		cbfs_unmap(blob);
		return current;
	}
	start = ALIGN_UP(current, 16);
	memcpy((void *)start, blob, size);
	cbfs_unmap(blob);
	acpi_add_table(rsdp, (void *)start);
	gnvs->nhla = start;
	gnvs->nhll = size;
	return ALIGN_UP(start + size, 16);
}
