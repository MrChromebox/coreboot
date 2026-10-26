/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi_device.h>
#include <acpi/acpigen.h>
#include <device/device.h>
#if CONFIG(SOC_INTEL_NHLT_SOF_SELECT)
#include <intelblocks/nhlt_cbfs.h>
#include <stdio.h>
#include <string.h>
#endif

#include "chip.h"

static const char *get_spkr_tplg_str(enum _spkr_tplg tplg)
{
	switch (tplg) {
	case max98373: return "max98373";
	case max98373_ssp2: return "max98373-ssp2";
	case max98360a: return "max98360a";
	case max98357a: return "max98357a";
	case max98357a_tdm: return "max98357a-tdm";
	case max98390: return "max98390";
	case rt1011: return "rt1011";
	case rt1015: return "rt1015";
	case rt1019: return "rt1019";
	case rt5650_sp: return "rt5650";
	case tas2563: return "tas2563";
	default: return "default";
	}
}

static const char *get_jack_tplg_str(enum _jack_tplg tplg)
{
	switch (tplg) {
	case cs42l42: return "cs42l42";
	case da7219: return "da7219";
	case nau8825: return "nau8825";
	case rt5650_hp: return "rt5650";
	case rt5682: return "rt5682";
	default: return "default";
	}
}

static const char *get_mic_tplg_str(enum _mic_tplg tplg)
{
	switch (tplg) {
	case _1ch: return "1ch";
	case _2ch_pdm0: return "2ch-pdm0";
	case _2ch_pdm1: return "2ch-pdm1";
	case _4ch: return "4ch";
	default: return "default";
	}
}

#if CONFIG(SOC_INTEL_NHLT_SOF_SELECT)
extern struct chip_operations drivers_sof_ops;
extern struct chip_operations drivers_intel_soundwire_ops;

const char *intel_nhlt_sof_cbfs_name(const char *platform)
{
	const struct drivers_sof_config *selected = NULL;
	const char *speaker, *headset, *mic;
	static char name[128];

	if (!platform || !platform[0])
		return NULL;

	/* Observe existing enablement; NHLT selection must not disable SOF nodes. */
	for (const struct device *dev = all_devices; dev; dev = dev->next) {
		if (!dev->enabled)
			continue;
		/* SoundWire requires an explicit table choice in the devicetree. */
		if (CONFIG(DRIVERS_INTEL_SOUNDWIRE) &&
		    dev->chip_ops == &drivers_intel_soundwire_ops)
			return NULL;
		if (dev->chip_ops != &drivers_sof_ops)
			continue;
		if (selected || !dev->chip_info)
			return NULL;
		selected = dev->chip_info;
	}
	if (!selected)
		return NULL;

	speaker = get_spkr_tplg_str(selected->spkr_tplg);
	headset = get_jack_tplg_str(selected->jack_tplg);
	mic = get_mic_tplg_str(selected->mic_tplg);
	/*
	 * "default" is valid in the existing SOF _DSD, but it is not a concrete
	 * NHLT wiring description. Skip only this table, leaving _DSD unchanged.
	 */
	if (!strcmp(speaker, "default") || !strcmp(headset, "default") ||
	    !strcmp(mic, "default"))
		return NULL;

	int len = snprintf(name, sizeof(name), "nhlt/%s-%s-%s-%s.nhlt",
			   platform, speaker, headset, mic);
	return len > 0 && (size_t)len < sizeof(name) ? name : NULL;
}
#endif

static void sof_fill_ssdt_generator(const struct device *dev)
{
	struct drivers_sof_config *config = dev->chip_info;
	const char *scope = acpi_device_scope(dev);
	struct acpi_dp *dsd;

	if (!dev->enabled || !config || !scope)
		return;

	/* Device */
	acpigen_write_scope(scope);

	/* DSD */
	dsd = acpi_dp_new_table("_DSD");
	acpi_dp_add_string(dsd, "speaker-tplg",
				get_spkr_tplg_str(config->spkr_tplg));
	acpi_dp_add_string(dsd, "hp-tplg",
				get_jack_tplg_str(config->jack_tplg));
	acpi_dp_add_string(dsd, "mic-tplg",
				get_mic_tplg_str(config->mic_tplg));
	acpi_dp_write(dsd);
	acpigen_pop_len(); /* Scope */
}


static struct device_operations sof_ops = {
	.read_resources		= noop_read_resources,
	.set_resources		= noop_set_resources,
	.acpi_fill_ssdt		= sof_fill_ssdt_generator,
};

static void sof_enable(struct device *dev)
{
	dev->ops = &sof_ops;
}

struct chip_operations drivers_sof_ops = {
	.name = "SOF",
	.enable_dev = sof_enable
};
