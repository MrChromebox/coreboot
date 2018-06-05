/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

/* These devices are created at runtime */
External (\_PR.CP00, DeviceObj)
External (\_PR.CP01, DeviceObj)
External (\_PR.CP02, DeviceObj)
External (\_PR.CP03, DeviceObj)
External (\_PR.CP04, DeviceObj)
External (\_PR.CP05, DeviceObj)
External (\_PR.CP06, DeviceObj)
External (\_PR.CP07, DeviceObj)

/* Notify OS to re-read CPU tables, assuming ^2 CPU count */
Method (PNOT)
{
	If (LGreaterEqual (\PCNT, 2)) {
		Notify (\_PR.CP00, 0x81)  // _CST
		Notify (\_PR.CP01, 0x81)  // _CST
	}
	If (LGreaterEqual (\PCNT, 4)) {
		Notify (\_PR.CP02, 0x81)  // _CST
		Notify (\_PR.CP03, 0x81)  // _CST
	}
	If (LGreaterEqual (\PCNT, 8)) {
		Notify (\_PR.CP04, 0x81)  // _CST
		Notify (\_PR.CP05, 0x81)  // _CST
		Notify (\_PR.CP06, 0x81)  // _CST
		Notify (\_PR.CP07, 0x81)  // _CST
	}
}

/* Notify OS to re-read CPU _PPC limit, assuming ^2 CPU count */
Method (PPCN)
{
	If (LGreaterEqual (\PCNT, 2)) {
		Notify (\_PR.CP00, 0x80)  // _PPC
		Notify (\_PR.CP01, 0x80)  // _PPC
	}
	If (LGreaterEqual (\PCNT, 4)) {
		Notify (\_PR.CP02, 0x80)  // _PPC
		Notify (\_PR.CP03, 0x80)  // _PPC
	}
	If (LGreaterEqual (\PCNT, 8)) {
		Notify (\_PR.CP04, 0x80)  // _PPC
		Notify (\_PR.CP05, 0x80)  // _PPC
		Notify (\_PR.CP06, 0x80)  // _PPC
		Notify (\_PR.CP07, 0x80)  // _PPC
	}
}

/* Notify OS to re-read Throttle Limit tables, assuming ^2 CPU count */
Method (TNOT)
{
	If (LGreaterEqual (\PCNT, 2)) {
		Notify (\_PR.CP00, 0x82)  // _TPC
		Notify (\_PR.CP01, 0x82)  // _TPC
	}
	If (LGreaterEqual (\PCNT, 4)) {
		Notify (\_PR.CP02, 0x82)  // _TPC
		Notify (\_PR.CP03, 0x82)  // _TPC
	}
	If (LGreaterEqual (\PCNT, 8)) {
		Notify (\_PR.CP04, 0x82)  // _TPC
		Notify (\_PR.CP05, 0x82)  // _TPC
		Notify (\_PR.CP06, 0x82)  // _TPC
		Notify (\_PR.CP07, 0x82)  // _TPC
	}
}

/* Return a package containing enabled processor entries */
Method (PPKG)
{
	If (LGreaterEqual (\PCNT, 8)) {
		Return (Package()
		{
			\_PR.CP00,
			\_PR.CP01,
			\_PR.CP02,
			\_PR.CP03,
			\_PR.CP04,
			\_PR.CP05,
			\_PR.CP06,
			\_PR.CP07
		})
	} ElseIf (LGreaterEqual (\PCNT, 4)) {
		Return (Package ()
		{
			\_PR.CP00,
			\_PR.CP01,
			\_PR.CP02,
			\_PR.CP03
		})
	} ElseIf (LGreaterEqual (\PCNT, 2)) {
		Return (Package ()
		{
			\_PR.CP00,
			\_PR.CP01
		})
	} Else {
		Return (Package ()
		{
			\_PR.CP00
		})
	}
}

Scope(\)
{
	Name (GCPC, Package ()
	{
		0x15, // 21 items
		0x02, // version 2 (ver 1 vs 2 reported by OS in _OSC)

		// 0x771 is IA32_HWP_CAPABILITIES (RO)

		// Highest Performance
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x00, 0x771, 0x04,)},
		// Nominal Performance -> Guaranteed Performance
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x08, 0x771, 0x04,)},
		// Lowest Nonlinear Performance -> Most Efficient Performance
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x10, 0x771, 0x04,)},
		// Lowest Performance
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x18, 0x771, 0x04,)},
		// Guaranteed Performance Register
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x08, 0x771, 0x04,)},

		// 0x774 is IA32_HWP_REQUEST (RW)

		// Desired Performance Register
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x10, 0x774, 0x04,)},
		// Minimum Performance Register
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x00, 0x774, 0x04,)},
		// Maximum Performance Register
		ResourceTemplate(){Register(FFixedHW, 0x08, 0x08, 0x774, 0x04,)},

		// Unsupported

		// Performance Reduction Tolerance Register
		ResourceTemplate(){Register(SystemMemory, 0x00, 0x00, 0x0,,)},
		// Time Window Register
		ResourceTemplate(){Register(SystemMemory, 0x00, 0x00, 0x0,,)},
		// Counter Wraparound Time
		ResourceTemplate(){Register(SystemMemory, 0x00, 0x00, 0x0,,)},

		// 0xE7 is IA32_MPERF

		// Reference Performance Counter Register
		ResourceTemplate(){Register(FFixedHW, 0x40, 0x00, 0x0E7, 0x04,)},

		// 0xE8 is IA32_APERF

		// Delivered Performance Counter Register
		ResourceTemplate(){Register(FFixedHW, 0x40, 0x00, 0x0E8, 0x04,)},

		// 0x777 is IA32_HWP_STATUS

		// Performance Limited Register
		ResourceTemplate(){Register(FFixedHW, 0x01, 0x02, 0x777, 0x04,)},

		// 0x770 is IA32_PM_ENABLE

		// CPPC Enable Register
		ResourceTemplate(){Register(FFixedHW, 0x01, 0x00, 0x770, 0x04,)},

		0x1, // Autonomous Selection Enable

		// Unsupported

		// Autonomous Activity Window Register
		ResourceTemplate(){Register(SystemMemory, 0x00, 0x00, 0x0,,)},
		// Energy Performance Preference Register
		ResourceTemplate(){Register(SystemMemory, 0x00, 0x00, 0x0,,)},
		// Reference Performance
		ResourceTemplate(){Register(SystemMemory, 0x00, 0x00, 0x0,,)}
	})
}
