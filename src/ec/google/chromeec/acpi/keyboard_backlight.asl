/* SPDX-License-Identifier: GPL-2.0-only */

/* Windows drivers require proper scoping to access child devices */
Scope (\_SB.PCI0.LPCB.EC0.CREC)
{
	/*
	 * Chrome EC Keyboard Backlight interface
	 */
	Device (KBLT)
	{
		Name (_HID, "GOOG0002")
		Name (_UID, 1)

		Method (_STA, 0, NotSerialized)
		{
			/* If this code is compiled in, assume the backlight
			 * exists physically.
			 */
			Return (0xf)
		}
	}
}

/* Stub for linux driver which hardcodes KBLT directly under _SB */
Scope (\_SB)
{
	Device (KBLT)
	{
		Name (_HID, "GOOG0002")
		Name (_UID, 1)

		Method (_STA, 0, NotSerialized)
		{
			Return (0x0)
		}

		/* Read current backlight value */
		Method (KBQC, 0, NotSerialized)
		{
			Return (\_SB.PCI0.LPCB.EC0.KBLV)
		}

		/* Write new backlight value */
		Method (KBCM, 1, NotSerialized)
		{
			\_SB.PCI0.LPCB.EC0.KBLV = Arg0
		}
	}
}
