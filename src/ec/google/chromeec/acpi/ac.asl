/* SPDX-License-Identifier: GPL-2.0-only */

// Scope (EC0)

#if !CONFIG(MINIPC_HIDE_AC)
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
		Return (0x0F)
	}
}
#endif
