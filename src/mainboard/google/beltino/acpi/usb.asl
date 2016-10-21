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
	Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
	{
		Zero,
		Zero,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)  // _PLD: Physical Location of Device
	{
		Return (GPLD (Zero, One))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT2)
{
	Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
	{
		0xFF,
		Zero,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)  // _PLD: Physical Location of Device
	{
		Return (GPLD (One, 0x02))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT3)
{
	Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
	{
		0xFF,
		Zero,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)  // _PLD: Physical Location of Device
	{
		Return (GPLD (One, 0x03))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT4)
{
	//Bluetooth
	Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
	{
		0xFF,
		0xFF,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)  // _PLD: Physical Location of Device
	{
		Return (GPLD (Zero, 0x04))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT5)
{
	//Bluetooth
        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
	{
		0xFF,
		Zero,
		Zero,
		Zero
	})
	Method (_PLD, 0, NotSerialized)  // _PLD: Physical Location of Device
	{
		Return (GPLD (One, 0x05))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.PRT6)
{
	Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
        {
        	0xFF,
                Zero,
                Zero,
                Zero
        })
	Method (_PLD, 0, NotSerialized)  // _PLD: Physical Location of Device
	{
		Return (GPLD (One, 0x06))
	}
}
Scope (\_SB.PCI0.XHCI.HUB7.SSP1)
{
        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
        {
        	0xFF,
                0x03,
                Zero,
                Zero
        })
}
Scope (\_SB.PCI0.XHCI.HUB7.SSP2)
{
        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
        {
        	0xFF,
                0x03,
                Zero,
                Zero
        })
}
Scope (\_SB.PCI0.XHCI.HUB7.SSP3)
{
        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
        {
        	0xFF,
                0x03,
                Zero,
                Zero
        })
}
Scope (\_SB.PCI0.XHCI.HUB7.SSP4)
{
        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
        {
        	0xFF,
                0x03,
                Zero,
                Zero
        })
}