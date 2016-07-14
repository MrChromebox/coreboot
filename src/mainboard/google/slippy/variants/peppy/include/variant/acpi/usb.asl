/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Google Inc.
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

Scope (\_SB.PCI0.XHCI.HUB7.PRT1)
{
	//WWAN M.2 Slot
	Name (_UPC, Package (0x04)
	{
		0xFF,
		0xFF,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (Zero, One))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT2)
{
	//Left USB 2.0
	Name (_UPC, Package (0x04)
	{
		0xFF,
		Zero,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One, 2))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT3)
{
	//Webcam
	Name (_UPC, Package (0x04)
	{
		0xFF,
		0xFF,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (Zero, 3))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT4)
{
	//Bluetooth")
	Name (_UPC, Package (0x04)
	{
		0xFF,
		0xFF,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (Zero, 4))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT5)
{
	//Right USB 2.0
	Name (_UPC, Package (0x04)
	{
		0xFF,
		Zero,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One, 5))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT6)
{
	Name (_UPC, Package (0x04)
	{
		Zero,
		Zero,
		Zero,
		Zero
	})
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT7)
{
	//SD Card
	Name (_UPC, Package (0x04)
	{
		0xFF,
		0xFF,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (Zero, 7))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.SSP1)
{
	//Left USB 3.0
	Name (_UPC, Package (0x04)
	{
		0xFF,
		0x03,
		Zero,
		Zero
	})
}