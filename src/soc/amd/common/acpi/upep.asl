/* SPDX-License-Identifier: GPL-2.0-only */

#define PEPD_DSM_UUID				"e3f32452-febc-43ce-9039-932122d37721"
#define  PEPD_DSM_LPI_ENUM_FUNCTIONS		0
#define   PEPD_DSM_LPI_ADDITIONAL_FUNCTIONS	1
#define  PEPD_DSM_LPI_GET_DEVICE_CONSTRAINTS	1

#define PEPD_DSM_NOTIFICATIONS_UUID			"11e00d56-ce64-47ce-837b-1f898f9aa461"
#define  PEPD_DSM_NOTIFICATION_ENUM_FUNCTIONS		0
#define   PEPD_DSM_NOTIFICATION_ADDITIONAL_FUNCTIONS	1
#define  PEPD_DSM_NOTIFICATION_DISPLAY_OFF		3
#define  PEPD_DSM_NOTIFICATION_DISPLAY_ON		4
#define  PEPD_DSM_NOTIFICATION_LOW_POWER_ENTRY		5
#define  PEPD_DSM_NOTIFICATION_LOW_POWER_EXIT		6
#define  PEPD_DSM_NOTIFICATION_MODERN_STANDBY_ENTRY	7
#define  PEPD_DSM_NOTIFICATION_MODERN_STANDBY_EXIT	8

External(\_SB.ESDX, MethodObj)
External(\_SB.MS0X, MethodObj)
External(\_SB.PCI0.LPCB.EC0.ESDX, MethodObj)
External(\_SB.PCI0.LPCB.EC0.S0IX, MethodObj)

/*
 * Power Engine Plug-in Device
 *
 * References:
 *   * Intel Low Power S0 Idle
 *   * AMD Modern Standby BIOS Implementation Guide - #56358
 *   * Linux Kernel: drivers/acpi/x86/s2idle.c
 *   * https://docs.microsoft.com/en-us/windows-hardware/design/device-experiences/modern-standby-firmware-notifications
 */
Scope (\_SB) {
	Device (PEP) {
		Name (_HID, "AMDI0005")
		Name (_CID, EisaId ("PNP0D80"))
		Name (_UID, 1)

		Method (_STA, 0, NotSerialized) {
			Return (0x0F)
		}

		/*
		 * Device constraints for low power states (may be used for debugging).
		 * For now there is only one disabled dummy device, because Windows
		 * expects at least one device and crashes without it with a bluescreen
		 * (`INTERNAL_POWER_ERROR`). Returning an empty package does not work.
		 */
		Name (DEVL, Package() {
			0,
			1,	/* Only 1 dummy device for now */

			Package() {
				Package() {
					0,		/* Disabled */
					"\\DUMY",	/* \DUMY - not existent */
					0,		/* Function States */
					0		/* Minimum D-state */
				}
			}
		})

		/*
		 * PEPD_DSM_UUID Helper method
		 *
		 * Arg0: Function Index
		 */
		Method (DSM0, 1, Serialized) {
			Switch (ToInteger(Arg0)) {
				/*
				 * Return a bit field of the supported functions for
				 * this UUID.
				 */
				Case (PEPD_DSM_LPI_ENUM_FUNCTIONS) {
					Local0 = Buffer { 0x00 }
					CreateByteField(Local0, 0x00, SUPP)

					SUPP = PEPD_DSM_LPI_ADDITIONAL_FUNCTIONS
					SUPP |= 1 << PEPD_DSM_LPI_GET_DEVICE_CONSTRAINTS

					Return (Local0)
				}
				Case (PEPD_DSM_LPI_GET_DEVICE_CONSTRAINTS) {
					Return (DEVL)
				}
				Default {
					/* Unknown function */
					Return (0)
				}
			}
		}

		/*
		 * PEPD_DSM_NOTIFICATIONS_UUID Helper method
		 *
		 * Arg0: Function Index
		 */
		Method (DSM1, 1, Serialized) {
			Switch (ToInteger(Arg0)) {
				/*
				 * Return a bit field of the supported functions for
				 * this UUID.
				 */
				Case (PEPD_DSM_NOTIFICATION_ENUM_FUNCTIONS) {
					Local0 = Buffer { 0x00 }
					CreateByteField(Local0, 0x00, SUPP)

					SUPP = PEPD_DSM_NOTIFICATION_ADDITIONAL_FUNCTIONS
					SUPP |= 1 << PEPD_DSM_NOTIFICATION_DISPLAY_OFF
					SUPP |= 1 << PEPD_DSM_NOTIFICATION_DISPLAY_ON
					SUPP |= 1 << PEPD_DSM_NOTIFICATION_LOW_POWER_ENTRY
					SUPP |= 1 << PEPD_DSM_NOTIFICATION_LOW_POWER_EXIT

					Return (Local0)
				}
				Case (PEPD_DSM_NOTIFICATION_DISPLAY_OFF) {
					/* Inform the EC */
					If (CondRefOf (\_SB.PCI0.LPCB.EC0.ESDX)){
						\_SB.PCI0.LPCB.EC0.ESDX(0)
					}
					/* provide board level hook */
					If (CondRefOf (\_SB.ESDX)) {
						\_SB.ESDX(0)
					}
					Return (0)
				}
				Case (PEPD_DSM_NOTIFICATION_DISPLAY_ON) {
					/* Inform the EC */
					If (CondRefOf (\_SB.PCI0.LPCB.EC0.ESDX)){
						\_SB.PCI0.LPCB.EC0.ESDX(1)
					}
					/* provide board level hook */
					If (CondRefOf (\_SB.ESDX)) {
						\_SB.ESDX(1)
					}
					Return (0)
				}
				Case (PEPD_DSM_NOTIFICATION_LOW_POWER_ENTRY) {
					/* Inform the EC */
					If (CondRefOf (\_SB.PCI0.LPCB.EC0.S0IX)){
						\_SB.PCI0.LPCB.EC0.S0IX(1)
					}
					/* provide board level S0ix hook */
					If (CondRefOf (\_SB.MS0X)) {
						\_SB.MS0X(1)
					}
					Return (0)
				}
				Case (PEPD_DSM_NOTIFICATION_LOW_POWER_EXIT) {
					/* Inform the EC */
					If (CondRefOf (\_SB.PCI0.LPCB.EC0.S0IX)){
						\_SB.PCI0.LPCB.EC0.S0IX(0)
					}
					/* provide board level S0ix hook */
					If (CondRefOf (\_SB.MS0X)) {
						\_SB.MS0X(0)
					}
					Return (0)
				}
				Default {
					/* Unknown function */
					Return (0)
				}
			}
		}

		/*
		 * Device Specific Method
		 *
		 * Arg0: UUID
		 * Arg1: Revision Id
		 * Arg2: Function Index
		 */
		Method (_DSM, 4, Serialized)  {
			If (Arg0 == ToUUID(PEPD_DSM_UUID)){
				/* Unsupported Revision */
				If (ToInteger(Arg1) != 0) {
					Return (Buffer() { 0x00 })
				}

				Return (DSM0(Arg2))
			}
			If (Arg0 == ToUUID(PEPD_DSM_NOTIFICATIONS_UUID)){
				/* Unsupported Revision */
				If (ToInteger(Arg1) != 0) {
					Return (Buffer() { 0x00 })
				}

				Return (DSM1(Arg2))
			}
			Return (Buffer { 0x00 })
		}
	}
}
