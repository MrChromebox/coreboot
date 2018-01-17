/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Google Inc.
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

#include "../gpio.h"

#define BOARD_TOUCHPAD_I2C_ADDR			0x4a
#define BOARD_TOUCHPAD_IRQ			TOUCHPAD_INT_L
#define BOARD_TOUCHPAD_WAKE			GPE_TOUCHPAD_WAKE

#define BOARD_TOUCHSCREEN_I2C_ADDR		0x4b
#define BOARD_TOUCHSCREEN_IRQ			TOUCHSCREEN_INT_L

#define BOARD_HP_MIC_CODEC_I2C_ADDR		0x1a
#define BOARD_HP_MIC_CODEC_IRQ			MIC_INT_L
#define BOARD_LEFT_SPEAKER_AMP_I2C_ADDR		0x34
#define BOARD_RIGHT_SPEAKER_AMP_I2C_ADDR	0x35

#define BOARD_DIG_I2C_ADDR			0x09
#define BOARD_DIG_IRQ				DIG_INT_L
#define BOARD_DIG_PDCT				DIG_PDCT_L
#define BOARD_DIG_EJECT				GPE_DIG_EJECT

Scope (\_SB)
{
	Device (PWRB)
	{
		Name (_HID, EisaId ("PNP0C0C"))
	}

	Device (PENH)
	{
		Name (_HID, "PRP0001")

		Name (_CRS, ResourceTemplate () {
			GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionInputOnly,
				"\\_SB.PCI0.GPIO", 0, ResourceConsumer) { GPIO_DIG_EJECT }
		})

		Name (_DSD, Package () {
			ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				Package () {
					"compatible",
					Package () { "gpio-keys"}
				},
			}
		})

		Device (EJCT)
		{
			Name (_ADR, Zero)

			Name (_DSD, Package () {
				ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
				Package () {
					/* SW_PEN_INSERTED */
					Package () { "linux,code", 0xf },
					/* EV_SW type */
					Package () { "linux,input-type", 0x5 },
					Package () { "label", "pen_eject" },
					Package () { "gpios",
						Package () {
							^^PENH, 0, 0, 1 /* inserted active low */
						}
					},
				}
			})
		}
	}
}

Scope (\_SB.PCI0.I2C0)
{
	Name (FMCN, Package () { 87, 197, 26 })

	Device (ATSA)
	{
		Name (_HID, "ATML0001")
		Name (_DDN, "Atmel Touchscreen")
		Name (_UID, 1)
		Name (_S0W, 4)

		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBus (
				BOARD_TOUCHSCREEN_I2C_ADDR,
				ControllerInitiated,
				400000,
				AddressingMode7Bit,
				"\\_SB.PCI0.I2C0",
			)
			Interrupt (ResourceConsumer, Edge, ActiveLow)
			{
				BOARD_TOUCHSCREEN_IRQ
			}
		})

		Method (_STA)
		{
			Return (0xF)
		}
	}
}

Scope (\_SB.PCI0.I2C1)
{
	Name (FMCN, Package () { 87, 197, 26 })

	Device (ATPA)
	{
		Name (_HID, "ATML0000")
		Name (_DDN, "Atmel Touchpad")
		Name (_UID, 1)
		Name (_S0W, 4)
		Name (_PRW, Package() { BOARD_TOUCHPAD_WAKE, 3 })

		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBus (
				BOARD_TOUCHPAD_I2C_ADDR,
				ControllerInitiated,
				400000,
				AddressingMode7Bit,
				"\\_SB.PCI0.I2C1",
			)
			Interrupt (ResourceConsumer, Edge, ActiveLow)
			{
				BOARD_TOUCHPAD_IRQ
			}
		})

		Method (_STA)
		{
			Return (0xF)
		}
	}
}

Scope (\_SB.PCI0.I2C2)
{
	Name (FMCN, Package () { 87, 197, 26 })

	Device (DIGI)
	{
		Name (_HID, "ACPI0C50")
		Name (_CID, "PNP0C50")
		Name (_UID, 1)
		Name (_S0W, 4)
		Name (_PRW, Package () { BOARD_DIG_EJECT, 3 })

		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBus (
				BOARD_DIG_I2C_ADDR,
				ControllerInitiated,
				400000,
				AddressingMode7Bit,
				"\\_SB.PCI0.I2C2",
			)
			Interrupt (ResourceConsumer, Level, ActiveLow)
			{
				BOARD_DIG_IRQ
			}
		})

		/*
		 * Function 1 returns the offset in the I2C device register
		 * address space at which the HID descriptor can be read.
		 *
		 * Arg0 = UUID
		 * Arg1 = revision number of requested function
		 * Arg2 = requested function number
		 * Arg3 = function specific parameter
		 */
		Method (_DSM, 4, NotSerialized)
		{
			If (LEqual (Arg0, ToUUID
			            ("3cdff6f7-4267-4555-ad05-b30a3d8938de"))) {
				If (LEqual (Arg2, Zero)) {
					/* Function 0 - Query */
					If (LEqual (Arg1, One)) {
						/* Revision 1 Function 1 */
						Return (Buffer (One) { 0x03 })
					} Else {
						/* Revision 2+ not supported */
						Return (Buffer (One) { 0x00 })
					}
				} ElseIf (LEqual (Arg2, One)) {
					/* Function 1 - HID Descriptor Addr */
					Return (0x0001)
				} Else {
					/* Function 2+ not supported */
					Return (Buffer (One) { 0x00 })
				}
			} Else {
				Return (Buffer (One) { 0x00 })
			}
		}
	}
}

Scope (\_SB.PCI0.I2C4)
{
	Name (FMCN, Package () { 87, 197, 26 })

	/* Headphone Codec */
	Device (HPMC)
	{
		Name (_HID, "10508825")
		Name (_DDN, "NAU88L25 Codec")
		Name (_UID, 1)

		/*
		 * Add DT style bindings with _DSD
		 * Device property values are documented in kernel doc
		 * Documentation/devicetree/bindings/sound/nau8825.txt
		 */
		Name (_DSD, Package () {
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package () {
				/* Enable jack detection via JKDET pin */
				Package () {"nuvoton,jkdet-enable", 1},
				/*
				 * JKDET pin is pulled up by R389 on board.
				 * JKDET pin polarity = active low
				 */
				Package () {"nuvoton,jkdet-polarity", 1},
				/* VREF Impedance = 125 kOhm */
				Package () {"nuvoton,vref-impedance", 2},
				/* VDDA(1.8) * 1.53 = 2.754 */
				Package () {"nuvoton,micbias-voltage", 6},
				/*
				 * Setup 4 buttons impedance according to
				 * Android specification
				 */
				Package () {"nuvoton,sar-threshold-num", 4},
				Package () {"nuvoton,sar-threshold",
					Package () {0x0c, 0x1c, 0x38, 0x60}},
				Package () {"nuvoton,sar-hysteresis", 1},
				/* VDDA for button impedance measurement */
				Package () {"nuvoton,sar-voltage", 0},
				Package () {"nuvoton,sar-compare-time", 0 },
				Package () {"nuvoton,sar-sampling-time", 0 },
				/* 100ms short key press debounce */
				Package () {"nuvoton,short-key-debounce", 2},
				/* 2^(7+2) = 512 ms insert/eject debounce */
				Package () {"nuvoton,jack-insert-debounce", 7},
				Package () {"nuvoton,jack-eject-debounce", 7},
			}
		})

		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBus (
				BOARD_HP_MIC_CODEC_I2C_ADDR,
				ControllerInitiated,
				400000,
				AddressingMode7Bit,
				"\\_SB.PCI0.I2C4",
			)
			Interrupt (ResourceConsumer, Level, ActiveLow)
			{
				BOARD_HP_MIC_CODEC_IRQ
			}
		})

		Method (_STA)
		{
			Return (0xF)
		}
	}

	/* Left Speaker Amp */
	Device (SPKL)
	{
		Name (_HID, "INT343B")
		Name (_DDN, "SSM4567 Speaker Amp")
		Name (_UID, 0)

		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBus (
				BOARD_LEFT_SPEAKER_AMP_I2C_ADDR,
				ControllerInitiated,
				400000,
				AddressingMode7Bit,
				"\\_SB.PCI0.I2C4",
			)
		})

		Method (_STA)
		{
			Return (0xF)
		}
	}

	/* Right Speaker Amp */
	Device (SPKR)
	{
		Name (_HID, "INT343B")
		Name (_DDN, "SSM4567 Speaker Amp")
		Name (_UID, 1)

		Name (_CRS, ResourceTemplate ()
		{
			I2cSerialBus (
				BOARD_RIGHT_SPEAKER_AMP_I2C_ADDR,
				ControllerInitiated,
				400000,
				AddressingMode7Bit,
				"\\_SB.PCI0.I2C4",
			)
		})

		Method (_STA)
		{
			Return (0xF)
		}
	}
}

Scope (\_SB.PCI0.SDXC)
{
	Name (_CRS, ResourceTemplate ()
	{
		GpioInt (Edge, ActiveBoth, SharedAndWake, PullNone, 10000,
			 "\\_SB.PCI0.GPIO", 0, ResourceConsumer)
		{
			GPIO_SD_CARD_DETECT
		}
	})

	Name (_DSD, Package () {
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package ()
		{
			Package () { "cd-gpio", Package () { ^SDXC, 0, 0, 1 } },
		}
	})
}
