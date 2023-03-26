/* SPDX-License-Identifier: GPL-2.0-only */

/* Scope is \_SB.PCI0.LPCB */

Device (SIO)
{
	Name (_UID, 0)
	Name (_ADR, 0)

	Device (COM1)
	{
		Name (_HID, EisaId ("PNP0501"))
		Name (_UID, 1)

		Method (_STA, 0, NotSerialized)
		{
#if CONFIG(DRIVERS_UART_8250IO)
			Return (0x0f)
#else
			Return (0)
#endif
		}

		Name (_CRS, ResourceTemplate ()
		{
			IO (Decode16, 0x03F8, 0x3F8, 0x08, 0x08)
			IRQNoFlags () {4}
		})
	}
}

Scope (\_SB.PCI0)
{
	Device (PS2K)
	{
		Name (_HID, "GOOG000A")
		Name (_CID, Package() { EISAID("PNP0303"), EISAID("PNP030B") })
		Name (_UID, 0)

		Method (_STA, 0, NotSerialized)
		{
#ifdef SIO_EC_ENABLE_PS2K
			Return (0x0f)
#else
			Return (0)
#endif
		}

		Name (_CRS, ResourceTemplate()
		{
			IO (Decode16, 0x60, 0x60, 0x01, 0x01)
			IO (Decode16, 0x64, 0x64, 0x01, 0x01)
			IRQ (Edge, ActiveHigh, Exclusive) {1}
		})
	}

	Device (PS2M)
	{
		Name (_HID, EisaId ("PNP0F13"))
		Name (_UID, 0)

		Method (_STA, 0, NotSerialized)
		{
#ifdef SIO_EC_ENABLE_PS2M
			Return (0x0f)
#else
			Return (0)
#endif
		}

		Name (_CRS, ResourceTemplate()
		{
			IRQ (Edge, ActiveHigh, Exclusive) {12}
		})
	}
}
