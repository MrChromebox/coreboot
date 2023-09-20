/* SPDX-License-Identifier: GPL-2.0-only */

Device (CIND)
{
	Name (_HID, "AMD33D3")  // _HID: Hardware ID
	Name (_CID, "PNP0C60" /* Display Sensor Device */)  // _CID: Compatible ID
	Method (_STA, 0, Serialized)  // _STA: Status
	{
		Return (0x0F)
	}
}

Device (VGBI)
{
	Name (_HID, "AMD33D6")  // _HID: Hardware ID

	Method (_STA, 0, NotSerialized)  // _STA: Status
	{
		Return (0x0F)
	}

	Method (AMWF, 0, Serialized)
	{
		If (LEqual (^^RCTM, One)) {
			Return (Zero)
		} Else {
			Return (One)
		}
	}
}
