/* SPDX-License-Identifier: GPL-2.0-only */

/* VGBS reports 0x40 when NOT in tablet mode. */
/* Sent event 0xCC for tablet mode, 0xCD for laptop */
/* Linux driver expects SMBIOS_ENCLOSURE_TYPE=SMBIOS_ENCLOSURE_CONVERTIBLE */

Device (VBTN)
{
	Name (_HID, "INT33D6")
	Name (_DDN, "Tablet Virtual Buttons")

	Method (VBDL, 0)
	{
	}

	Method (VGBS)
	{
		If (LEqual (^^RCTM, One)) {
			Return (0x0)
		} Else {
			Return (0x40)
		}
	}
	Method(_STA, 0)
	{
		Return (0xF)
	}
}

Device (VBTO)
{
	Name (_HID, "INT33D3")
	Name (_CID, "PNP0C60")
	Method (_STA, 0)
	{
		Return (0xF)
	}
}
