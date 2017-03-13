/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

Scope (\_SB)
{
	Device (LID0)
	{
		Name (_HID, EisaId("PNP0C0D"))
		Name (_PRW, Package() {0x18, 4})

		Method (_LID, 0)
		{
			Store (\_SB.PCI0.LPCB.EC0.HPLD, \LIDS)
			Return (\LIDS)
		}

		Method (_PSW, 1)
		{
			// Enable/Disable LID as a wake source
			Store (Arg0, \_SB.PCI0.LPCB.EC0.HWLO)
		}
	}

	Device (PWRB)
	{
		Name (_HID, EisaId("PNP0C0C"))
	}

	Device (TPAD)
        {
            Name (_ADR, Zero)  // _ADR: Address
            Name (_UID, One)  // _UID: Unique ID
            Name (_HID, "CYSM0000")  // _HID: Hardware ID
            Name (PCRS, ResourceTemplate ()  // _CRS: Current Resource Settings
            {
                Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, ,, )
                {
                    0x00000014,
                }
                VendorShort ()
                {
                     0x67
                }
            })
            Name (DCRS, ResourceTemplate ()  // _CRS: Current Resource Settings
            {
                Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, ,, )
                {
                    0x00000010,
                }
                VendorShort ()
                {
                     0x67
                }
            })
            Method (_CRS, 0, NotSerialized)
            {
            	If (\TPIQ == 16){
            		Return (DCRS)
            	} Else {
            		Return (PCRS)
            	}
            }
        }

	Device (MB) {
		Name(_HID, EisaId("PNP0C01")) // System Board

		/* Lid open */
		Method (LIDO) { /* Not needed on this board */ }
		/* Lid closed */
		Method (LIDC) { /* Not needed on this board */ }
		/* Increase brightness */
		Method (BRTU) { /* Not needed on this board */ }
		/* Decrease brightness */
		Method (BRTD) { /* Not needed on this board */ }
		/* Switch display */
		Method (DSPS) { /* Not needed on this board */ }
		/* Toggle wireless */
		Method (WLTG) { /* Not needed on this board */ }
		/* Return lid state */
		Method (LIDS)
		{
			Return (GP15)
		}
	}
}
