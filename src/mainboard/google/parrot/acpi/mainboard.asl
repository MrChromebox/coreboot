/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011-2012 Google Inc.
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

#include <mainboard/google/parrot/onboard.h>

Scope (\_GPE) {
	Method(_L1F, 0x0, NotSerialized)
	{
		/*
		 * Invert the interrupt level bit for the lid GPIO
		 * so we don't get another _SB.LID0 until the state
		 * changes again. GIV1 is the interrupt level control
		 * register for GPIO bits 15:8
		 */
		Xor(GIV1, 0x80, GIV1)
		Notify(\_SB.LID0,0x80)
	}
}

Scope (\_SB) {
	Device (LID0)
	{
		Name(_HID, EisaId("PNP0C0D"))
		Method(_LID, 0)
		{
			Store (GP15, \LIDS)
			Return (\LIDS)
		}
	}

	Device (PWRB)
	{
		Name(_HID, EisaId("PNP0C0C"))
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
