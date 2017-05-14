/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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

#include "../gpio.h"

Scope (\_SB)
{
	Device (PWRB)
	{
		Name (_HID, EisaId ("PNP0C0C"))
	}
}

Scope (\_SB.PCI0.SDXC)
{
	Name (_CRS, ResourceTemplate () {
		GpioInt (Edge, ActiveBoth, SharedAndWake, PullNone, 10000,
			 "\\_SB.PCI0.GPIO", 0, ResourceConsumer)
		{
			GPIO_SD_CARD_DETECT
		}
	})

	Name (_DSD, Package () {
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package ()
		{
			Package () { "cd-gpio", Package () { ^SDXC, 0, 0, 1 } },
		}
	})
}
