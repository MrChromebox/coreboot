/* SPDX-License-Identifier: GPL-2.0-only */

Device (CREC)
{
	Name (_HID, "GOOG0004")
	Name (_UID, 1)
	Name (_DDN, "EC Command Device")
#ifdef EC_ENABLE_WAKE_PIN
	Name (_PRW, Package () { EC_ENABLE_WAKE_PIN, 0x5 })
#endif

#ifdef EC_ENABLE_MKBP_DEVICE
	Device (CKSC)
	{
		Name (_HID, "GOOG0007")
		Name (_UID, 1)
		Name (_DDN, "EC MKBP Device")
	}
#endif

#ifdef EC_ENABLE_CBAS_DEVICE
	Device (CBAS)
	{
		Name (_HID, "GOOG000B")
		Name (_UID, 1)
		Name (_DDN, "EC Base Switch Device")
	}
#endif

#ifdef EC_ENABLE_PD_MCU_DEVICE
	#include "pd.asl"
#endif

#ifdef EC_ENABLE_TBMC_DEVICE
	#include "tbmc.asl"
#endif

	Method(_STA, 0)
	{
		Return (0xF)
	}

#if CONFIG(DRIVERS_ACPI_THERMAL_ZONE)
	Method(TMP, 1)
	{
		Return(^^TSRD(Arg0))
	}
#endif
}
