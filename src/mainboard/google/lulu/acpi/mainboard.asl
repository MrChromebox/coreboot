/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

#include "onboard.h"

Scope (\_SB)
{
	Device (LID0)
	{
		Name(_HID, EisaId("PNP0C0D"))
		Method(_LID, 0)
		{
			Store (\_SB.PCI0.LPCB.EC0.LIDS, \LIDS)
			Return (\LIDS)
		}


		// There is no GPIO for LID, the EC pulses WAKE# pin instead.
		// There is no GPE for WAKE#, so fake it with PCI_EXP_WAKE
		Name (_PRW, Package(){ 0x69, 5 }) // PCI_EXP
	}

	Device (PWRB)
	{
		Name(_HID, EisaId("PNP0C0C"))
	}
}

/*
 * LPC Trusted Platform Module
 */
Scope (\_SB.PCI0.LPCB)
	{
	#include <drivers/pc80/tpm/acpi/tpm.asl>
}

Scope (\_SB.PCI0.I2C0)
{
	Device (STPA)
	{
		Name (_HID, "SYNA0000")
		Name (_DDN, "Synaptics Touchpad")
		Name (_UID, 1)
		Name (ISTP, 1) /* Touchpad */

		Method(_CID, 0)
		{
			If (_OSI("Linux"))
			{
				Return ("PNP0C50")
			} Else
			{
				Return ("SYNA0000")
			}
		}

		Method(_CRS, 0x0, Serialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				I2cSerialBus (
					0x2C,                     // SlaveAddress
					ControllerInitiated,      // SlaveMode
					400000,                   // ConnectionSpeed
					AddressingMode7Bit,       // AddressingMode
					"\\_SB.PCI0.I2C0",        // ResourceSource
				)
				Interrupt (ResourceConsumer, Level, ActiveLow)
				{
					BOARD_TRACKPAD_IRQ
				}
			})
			Return(RBUF)
		}

		Method(_DSM, 0x4, NotSerialized)
		{
			/* I2C-HID UUID 3CDFF6F7-4267-4555-AD05-B30A3D8938DE */
			If (LEqual(Arg0, Buffer (0x10)
				{
					/* 0000 */   0xF7, 0xF6, 0xDF, 0x3C, 0x67, 0x42, 0x55, 0x45,
					/* 0008 */   0xAD, 0x05, 0xB3, 0x0A, 0x3D, 0x89, 0x38, 0xDE
				}))
			{
				If (LEqual(Arg2, Zero)) /* DSM Function */
				{
					/* Function 0: Query function, return based on revision */
					If (LEqual(Arg1, One)) /* Arg1 DSM Revision */
					{
						/* Revision 1: Function 1 supported */
						Return(Buffer(One) { 0x03 })
					} Else {
						/* Revision 2+: no functions supported */
						Return(Buffer(One) { 0x00 })
					}
				}
				If (LEqual(Arg2, One)) /* Function 1 : HID Function */
				{
					Return(0x0020) /* HID Descriptor Address */
				} Else {
					Return(Buffer(One) { 0x00 }) /* Functions 2+: not supported */
				}
			} Else {
				Return(Buffer(One) { 0x00 }) /* No other GUIDs supported */
			}
		}

		Method (_STA)
		{
			If (LEqual (\S1EN, 1)) {
				If (LEqual (\TID1, 0)) {
					Return (0xF)
				} Else {
					If(LEqual (\TID1, 1)) {
						Return (0xF)
					}
					Return (0x0)
				}
			} Else {
				Return (0x0)
			}
		}

		Name (_PRW, Package() { BOARD_TRACKPAD_WAKE_GPIO, 0x3 })

		Method (_DSW, 3, NotSerialized)
		{
			Store (BOARD_TRACKPAD_WAKE_GPIO, Local0)
			If (LEqual (Arg0, 1)) {
				// Enable GPIO as wake source
				\_SB.PCI0.LPCB.GPIO.GWAK (Local0)
			}
		}

		/* Allow device to power off in S0 */
		Name (_S0W, 4)
	}
}
Scope (\_SB.PCI0.I2C1)
{
	Device (ETSA)
	{
		Name (_HID, "ELAN0001")
		Name (_DDN, "Elan Touchscreen")
		Name (_UID, 6)
		Name (ISTP, 0) /* Touchscreen */

		Name (_CRS, ResourceTemplate()
		{
			I2cSerialBus (
				0x10,                     // SlaveAddress
				ControllerInitiated,      // SlaveMode
				400000,                   // ConnectionSpeed
				AddressingMode7Bit,       // AddressingMode
				"\\_SB.PCI0.I2C1",        // ResourceSource
			)
			Interrupt (ResourceConsumer, Level, ActiveLow)
			{
				BOARD_TOUCHSCREEN_IRQ
			}
		})

		Method (_STA)
		{
			If (LEqual (\S2EN, 1)) {
				Return (0xF)
			} Else {
				Return (0x0)
			}
		}

		Name (_PRW, Package() { BOARD_TOUCHSCREEN_WAKE_GPIO, 0x3 })

		Method (_DSW, 3, NotSerialized)
		{
			Store (BOARD_TOUCHSCREEN_WAKE_GPIO, Local0)
			If (LEqual (Arg0, 1)) {
				// Enable GPIO as wake source
				\_SB.PCI0.LPCB.GPIO.GWAK (Local0)
			}
		}

		/* Allow device to power off in S0 */
		Name (_S0W, 4)
	}
}

Scope (\_SB.PCI0.RP01)
{
	Device (WLAN)
	{
		Name (_ADR, 0x00000000)

		Name (_PRW, Package() { BOARD_WLAN_WAKE_GPIO, 3 })

		Method (_DSW, 3, NotSerialized)
		{
			Store (BOARD_WLAN_WAKE_GPIO, Local0)
			If (LEqual (Arg0, 1)) {
				// Enable GPIO as wake source
				\_SB.PCI0.LPCB.GPIO.GWAK (Local0)
			}
		}
	}
}
