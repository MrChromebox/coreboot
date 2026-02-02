/* SPDX-License-Identifier: GPL-2.0-only */

// Scope (EC0)

Device (AC)
{
	Name (_HID, "ACPI0003")
	Name (_PCL, Package () { \_SB })

	Method (_PSR)
	{
		Return (ACEX)
	}

	Method (_STA)
	{
#if CONFIG(EC_FOR_CHROMEBOX)
		Return (0)
#else
		Return (0x0F)
#endif
	}
}
