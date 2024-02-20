/* SPDX-License-Identifier: GPL-2.0-only */

External (\PPKG, MethodObj)

#include <variant/thermal.h>

// Thermal Zone

#define HAVE_THERMALZONE
Scope (\_TZ)
{
	ThermalZone (THRM)
	{
		Name (_TC1, 0x02)
		Name (_TC2, 0x05)

		// Thermal zone polling frequency: 10 seconds
		Name (_TZP, 100)

		// Thermal sampling period for passive cooling: 2 seconds
		Name (_TSP, 20)

		// Convert from Degrees C to 1/10 Kelvin for ACPI
		Method (CTOK, 1) {
			// 10th of Degrees C
			Local0 = Arg0 * 10

			// Convert to Kelvin
			Local0 += 2732

			Return (Local0)
		}

		// Threshold for OS to shutdown
		Method (_CRT, 0, Serialized)
		{
			Return (CTOK (\TCRT))
		}

		// Threshold for passive cooling
		Method (_PSV, 0, Serialized)
		{
			Return (CTOK (\TPSV))
		}

		// Processors used for passive cooling
		Method (_PSL, 0, Serialized)
		{
			Return (\PPKG ())
		}

		// Start fan at state 4 = lowest temp state
		Method (_INI)
		{
			\FLVL = 4
			\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN4_PWM
			Notify (\_TZ.THRM, 0x81)
		}

		Method (TCHK, 0, Serialized)
		{
			// Get CPU Temperature from PECI via SuperIO TMPIN3
			Local0 = \_SB.PCI0.LPCB.SIO.ENVC.TIN3

			// Check for "no reading available"
			If (Local0 == 0x80) {
				Return (CTOK (FAN0_THRESHOLD_ON))
			}

			// Check for invalid readings
			If ((Local0 == 255) || (Local0 == 0)) {
				Return (CTOK (FAN0_THRESHOLD_ON))
			}

			// PECI raw value is an offset from Tj_max
			Local1 = 255 - Local0

			// Handle values greater than Tj_max
			If (Local1 >= \TMAX) {
				Return (CTOK (\TMAX))
			}

			// Subtract from Tj_max to get temperature
			Local0 = \TMAX - Local1
			Return (CTOK (Local0))
		}

		Method (_TMP, 0, Serialized)
		{
			// Get temperature from SuperIO in deci-kelvin
			Local0 = TCHK ()

			// Critical temperature in deci-kelvin
			Local1 = CTOK (\TMAX)

			If (Local0 >= Local1) {
				Printf ("CRITICAL TEMPERATURE: %o", Local0)

				// Wait 1 second for SuperIO to re-poll
				Sleep (1000)

				// Re-read temperature from SuperIO
				Local0 = TCHK ()

				Printf ("RE-READ TEMPERATURE: %o", Local0)
			}

			Return (Local0)
		}

		Method (_AC0) {
			If (\FLVL <= 0) {
				Return (CTOK (FAN0_THRESHOLD_OFF))
			} Else {
				Return (CTOK (FAN0_THRESHOLD_ON))
			}
		}

		Method (_AC1) {
			If (\FLVL <= 1) {
				Return (CTOK (FAN1_THRESHOLD_OFF))
			} Else {
				Return (CTOK (FAN1_THRESHOLD_ON))
			}
		}

		Method (_AC2) {
			If (\FLVL <= 2) {
				Return (CTOK (FAN2_THRESHOLD_OFF))
			} Else {
				Return (CTOK (FAN2_THRESHOLD_ON))
			}
		}

		Method (_AC3) {
			If (\FLVL <= 3) {
				Return (CTOK (FAN3_THRESHOLD_OFF))
			} Else {
				Return (CTOK (FAN3_THRESHOLD_ON))
			}
		}

		Method (_AC4) {
			Return (CTOK (0))
		}

		Name (_AL0, Package () { FAN0 })
		Name (_AL1, Package () { FAN1 })
		Name (_AL2, Package () { FAN2 })
		Name (_AL3, Package () { FAN3 })
		Name (_AL4, Package () { FAN4 })

		PowerResource (FNP0, 0, 0)
		{
			Method (_STA) {
				If (\FLVL <= 0) {
					Return (1)
				} Else {
					Return (0)
				}
			}
			Method (_ON)  {
				If (!_STA ()) {
					\FLVL = 0
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN0_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
			Method (_OFF) {
				If (_STA ()) {
					\FLVL = 1
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN1_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
		}

		PowerResource (FNP1, 0, 0)
		{
			Method (_STA) {
				If (\FLVL <= 1) {
					Return (1)
				} Else {
					Return (0)
				}
			}
			Method (_ON)  {
				If (!_STA ()) {
					\FLVL = 1
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN1_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
			Method (_OFF) {
				If (_STA ()) {
					\FLVL = 2
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN2_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
		}

		PowerResource (FNP2, 0, 0)
		{
			Method (_STA) {
				If (\FLVL <= 2) {
					Return (1)
				} Else {
					Return (0)
				}
			}
			Method (_ON)  {
				If (!_STA ()) {
					\FLVL = 2
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN2_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
			Method (_OFF) {
				If (_STA ()) {
					\FLVL = 3
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN3_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
		}

		PowerResource (FNP3, 0, 0)
		{
			Method (_STA) {
				If (\FLVL <= 3) {
					Return (1)
				} Else {
					Return (0)
				}
			}
			Method (_ON)  {
				If (!_STA ()) {
					\FLVL = 3
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN3_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
			Method (_OFF) {
				If (_STA ()) {
					\FLVL = 4
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN4_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
		}

		PowerResource (FNP4, 0, 0)
		{
			Method (_STA) {
				If (\FLVL <= 4) {
					Return (1)
				} Else {
					Return (0)
				}
			}
			Method (_ON)  {
				If (!_STA ()) {
					\FLVL = 4
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN4_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
			Method (_OFF) {
				If (_STA ()) {
					\FLVL = 4
					\_SB.PCI0.LPCB.SIO.ENVC.F2PS = FAN4_PWM
					Notify (\_TZ.THRM, 0x81)
				}
			}
		}

		Device (FAN0)
		{
			Name (_HID, EISAID ("PNP0C0B"))
			Name (_UID, 0)
			Name (_PR0, Package () { FNP0 })
		}

		Device (FAN1)
		{
			Name (_HID, EISAID ("PNP0C0B"))
			Name (_UID, 1)
			Name (_PR0, Package () { FNP1 })
		}

		Device (FAN2)
		{
			Name (_HID, EISAID ("PNP0C0B"))
			Name (_UID, 2)
			Name (_PR0, Package () { FNP2 })
		}

		Device (FAN3)
		{
			Name (_HID, EISAID ("PNP0C0B"))
			Name (_UID, 3)
			Name (_PR0, Package () { FNP3 })
		}

		Device (FAN4)
		{
			Name (_HID, EISAID ("PNP0C0B"))
			Name (_UID, 4)
			Name (_PR0, Package () { FNP4 })
		}
	}
}
