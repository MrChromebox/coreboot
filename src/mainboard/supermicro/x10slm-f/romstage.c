/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2010 coresystems GmbH
 * Copyright (C) 2012 Google Inc.
 * Copyright (C) 2018 Tristan Corrick <tristan@corrick.kiwi>
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

#include <cpu/intel/haswell/haswell.h>
#include <cpu/intel/romstage.h>
#include <northbridge/intel/haswell/haswell.h>
#include <northbridge/intel/haswell/pei_data.h>
#include <southbridge/intel/common/gpio.h>
#include <southbridge/intel/lynxpoint/pch.h>
#include <stdint.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct6776/nct6776.h>

static const struct rcba_config_instruction rcba_config[] = {
	RCBA_SET_REG_16(D31IR, DIR_ROUTE(PIRQA, PIRQD, PIRQC, PIRQA)),
	RCBA_SET_REG_16(D29IR, DIR_ROUTE(PIRQH, PIRQD, PIRQA, PIRQC)),
	RCBA_SET_REG_16(D28IR, DIR_ROUTE(PIRQA, PIRQB, PIRQC, PIRQD)),
	RCBA_SET_REG_16(D27IR, DIR_ROUTE(PIRQG, PIRQB, PIRQC, PIRQD)),
	RCBA_SET_REG_16(D26IR, DIR_ROUTE(PIRQA, PIRQF, PIRQC, PIRQD)),
	RCBA_SET_REG_16(D25IR, DIR_ROUTE(PIRQE, PIRQF, PIRQG, PIRQH)),
	RCBA_SET_REG_16(D22IR, DIR_ROUTE(PIRQA, PIRQB, PIRQC, PIRQD)),
	RCBA_SET_REG_16(D20IR, DIR_ROUTE(PIRQA, PIRQB, PIRQC, PIRQD)),

	RCBA_RMW_REG_32(FD, ~0UL, PCH_DISABLE_ALWAYS),

	RCBA_END_CONFIG,
};

void mainboard_config_superio(void)
{
	const pnp_devfn_t GLOBAL_PSEUDO_DEV = PNP_DEV(0x2e, 0);
	const pnp_devfn_t SERIAL_DEV = PNP_DEV(0x2e, NCT6776_SP1);
	const pnp_devfn_t ACPI_DEV = PNP_DEV(0x2e, NCT6776_ACPI);

	nuvoton_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);

	nuvoton_pnp_enter_conf_state(GLOBAL_PSEUDO_DEV);

	/* Select HWM/LED functions instead of floppy functions. */
	pnp_write_config(GLOBAL_PSEUDO_DEV, 0x1c, 0x03);
	pnp_write_config(GLOBAL_PSEUDO_DEV, 0x24, 0x24);

	/* Power RAM in S3 and let the PCH handle power failure actions. */
	pnp_set_logical_device(ACPI_DEV);
	pnp_write_config(ACPI_DEV, 0xe4, 0x70);

	nuvoton_pnp_exit_conf_state(GLOBAL_PSEUDO_DEV);
}

void mainboard_romstage_entry(unsigned long bist)
{
	struct pei_data pei_data = {
		.pei_version = PEI_VERSION,
		.mchbar = (uintptr_t)DEFAULT_MCHBAR,
		.dmibar = (uintptr_t)DEFAULT_DMIBAR,
		.epbar = DEFAULT_EPBAR,
		.pciexbar = DEFAULT_PCIEXBAR,
		.smbusbar = SMBUS_IO_BASE,
		.wdbbar = 0x4000000,
		.wdbsize = 0x1000,
		.hpet_address = HPET_ADDR,
		.rcba = (uintptr_t)DEFAULT_RCBA,
		.pmbase = DEFAULT_PMBASE,
		.gpiobase = DEFAULT_GPIOBASE,
		.temp_mmio_base = 0xfed08000,
		.system_type = 1, /* desktop/server */
		.tseg_size = CONFIG_SMM_TSEG_SIZE,
		.spd_addresses = { 0xa0, 0xa2, 0xa4, 0xa6 },
		.ec_present = 0,
		.ddr_refresh_2x = 1,
		.max_ddr3_freq = 1600,
		.usb2_ports = {
			/* Length, Enable, OCn#, Location */
			{ 0x0040, 1, 0, USB_PORT_INTERNAL },
			{ 0x0040, 1, 0, USB_PORT_BACK_PANEL },
			{ 0x0110, 1, 1, USB_PORT_BACK_PANEL },
			{ 0x0110, 1, 1, USB_PORT_BACK_PANEL },
			{ 0x0110, 1, 2, USB_PORT_BACK_PANEL },
			{ 0x0110, 1, 2, USB_PORT_BACK_PANEL },
			{ 0x0040, 0, USB_OC_PIN_SKIP, USB_PORT_SKIP },
			{ 0x0040, 0, USB_OC_PIN_SKIP, USB_PORT_SKIP },
			{ 0x0040, 1, 4, USB_PORT_BACK_PANEL },
			{ 0x0040, 1, 4, USB_PORT_BACK_PANEL },
			{ 0x0040, 0, USB_OC_PIN_SKIP, USB_PORT_SKIP },
			{ 0x0040, 0, USB_OC_PIN_SKIP, USB_PORT_SKIP },
			{ 0x0040, 1, 6, USB_PORT_BACK_PANEL },
			{ 0x0040, 1, 6, USB_PORT_BACK_PANEL },
		},
		.usb3_ports = {
			/* Enable, OCn# */
			{ 1, 1 },
			{ 1, 1 },
			{ 0, USB_OC_PIN_SKIP },
			{ 0, USB_OC_PIN_SKIP },
			{ 1, 3 },
			{ 1, 3 },
		},
	};

	struct romstage_params romstage_params = {
		.pei_data = &pei_data,
		.gpio_map = &mainboard_gpio_map,
		.rcba_config = rcba_config,
		.bist = bist,
	};

	romstage_common(&romstage_params);
}
