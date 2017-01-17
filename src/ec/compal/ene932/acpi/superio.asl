/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 The ChromiumOS Authors.  All rights reserved.
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

// Scope is \_SB.PCI0.LPCB

Device (SIO) {
	Name (_HID, EisaId("PNP0A05"))
	Name (_UID, 0)
	Name (_ADR, 0)

// PS2 Keyboard
#ifdef SIO_EC_ENABLE_PS2K
	Device (PS2K)		// Keyboard
	{
		Name(_HID, EISAID("GGL0303"))
		Name(_CID, EISAID("PNP0303"))

		Name(_CRS, ResourceTemplate()
		{
			IO (Decode16, 0x60, 0x60, 0x01, 0x01)
			IO (Decode16, 0x64, 0x64, 0x01, 0x01)
			IRQ (Edge, ActiveHigh, Exclusive) { 0x01 } // IRQ 1
		})

		Method (_STA, 0)
		{
			Return (0xF)
		}
	}
#endif
}
