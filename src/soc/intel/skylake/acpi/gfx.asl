/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

Device (GFX0)
{
	Name (_ADR, 0x00020000)

	OperationRegion (GFXC, PCI_Config, 0x00, 0x0100)
	Field(GFXC, AnyAcc, NoLock, Preserve)
	{
		Offset(0xfc),
		ASLB, 32,
	}

	OperationRegion(IGDM, SystemMemory, ASLB, 0x2000)
	Field(IGDM, AnyAcc, NoLock, Preserve)
	{
		/* OpRegion Header */
		SIGN, 128, // Signature-"IntelGraphicsMem"
		SIZE, 32,  // OpRegion Size
		OVER, 32,  // OpRegion Version
		SVER, 256, // System BIOS Version
		VVER, 128, // VBIOS Version
		GVER, 128, // Driver version
		MBOX, 32,  // Mailboxes supported
		DMOD, 32,  // Driver Model
		PCON, 32,  // Platform Configuration
		DVER, 64,  // GOP Version

		/* OpRegion Mailbox 1 (Public ACPI Methods) */
		Offset(0x100),
		DRDY, 32, // Driver readiness (ACPI notification)
		CSTS, 32, // Notification status
		CEVT, 32, // Current event
		Offset(0x120),
		DIDL, 32, // Supported display device ID list
		DDL2, 32, // Allows for 8 devices
		DDL3, 32,
		DDL4, 32,
		DDL5, 32,
		DDL6, 32,
		DDL7, 32,
		DDL8, 32,
		CPDL, 32, // Currently present display list
		CPL2, 32, // Allows for 8 devices
		CPL3, 32,
		CPL4, 32,
		CPL5, 32,
		CPL6, 32,
		CPL7, 32,
		CPL8, 32,
		CADL, 32, // Currently active display list
		CAL2, 32, // Allows for 8 devices
		CAL3, 32,
		CAL4, 32,
		CAL5, 32,
		CAL6, 32,
		CAL7, 32,
		CAL8, 32,
		NADL, 32, // Next active display list
		NDL2, 32, // Allows for 8 devices
		NDL3, 32,
		NDL4, 32,
		NDL5, 32,
		NDL6, 32,
		NDL7, 32,
		NDL8, 32,
		ASLP, 32, // ASL sleep timeout
		TIDX, 32, // Toggle table index
		CHPD, 32, // Current hot plug enable indicator
		CLID, 32, // Current lid state indicator
		CDCK, 32, // Current docking state indicator
		SXSW, 32, // Display switch notify on resume
		EVTS, 32, // Events supported by ASL (diag only)
		CNOT, 32, // Current OS notifications (diag only)
		NRDY, 32,

		/* Extended DIDL list */

		DDL9, 32,
		DD10, 32,
		DD11, 32,
		DD12, 32,
		DD13, 32,
		DD14, 32,
		DD15, 32,

		/* Extended Currently attached Display Device ListCPD2 */

		CPL9, 32,
		CP10, 32,
		CP11, 32,
		CP12, 32,
		CP13, 32,
		CP14, 32,
		CP15, 32,

		/* OpRegion Mailbox 2 (Software SCI Interface) */

		Offset(0x200), // SCIC
		SCIE, 1, // SCI entry bit (1=call unserviced)
		GEFC, 4, // Entry function code
		GXFC, 3, // Exit result
		GESF, 8, // Entry/exit sub-function/parameter
		, 16,// SCIC[31:16] reserved
		Offset(0x204), // PARM
		PARM, 32,// PARM register (extra parameters)
		DSLP,32, // Driver sleep time out

		/* OpRegion Mailbox 3 (BIOS to Driver Notification) */
		Offset(0x300),
		ARDY, 32,// Driver readiness (power conservation)
		ASLC, 32,// ASLE interrupt command/status
		TCHE, 32,// Technology enabled indicator
		ALSI, 32,// Current ALS illuminance reading
		BCLP, 32,// Backlight brightness
		PFIT, 32,// Panel fitting state or request
		CBLV, 32,// Current brightness level
		BCLM, 320, // Backlight brightness level duty cycle mapping table
		CPFM, 32,// Current panel fitting mode
		EPFM, 32,// Enabled panel fitting modes
		PLUT, 592, // Optional. 74-byte Panel LUT Table
		PFMB, 32,// Optional. PWM Frequency and Minimum Brightness
		CCDV, 32,// Optional. Gamma, Brightness, Contrast values.
		PCFT, 32,// Optional. Power Conservation Features
		SROT, 32,// Supported rotation angle.
		IUER, 32,// Optional. Intel Ultrabook Event Register.
		FDSP, 64,// Optional. FFS Display Physical address
		FDSS, 32,// Optional. FFS Display Size
		STAT, 32,// State Indicator

		/* OpRegion Mailbox 4 (VBT) */
		Offset(0x400),
		GVD1, 0xC000,// 6K bytes maximum VBT image

		/* OpRegion Mailbox 5 (BIOS to Driver Notification Extension) */
		Offset(0x1C00),
		PHED, 32,// Panel Header
		BDDC, 2048,// Panel EDID (Max 256 bytes)
	}

	OperationRegion(IGDP, PCI_Config, 0x40, 0xC0)
	Field(IGDP, AnyAcc, NoLock, Preserve)
	{
		Offset(0x10), //Offset(16),
		, 1,
		GIVD, 1,
		, 2,
		GUMA, 3,
		Offset(0x12), //Offset(18),
		Offset(0x14), //Offset(20),
		, 4,
		GMFN, 1,
		Offset(0x18), //Offset(24),
		Offset(0xA4), //Offset(164),
		ASLE, 8,
		Offset(0xA8), //Offset(168),
		GSSE, 1,
		GSSB, 14,
		GSES, 1,
		Offset(0xB0), //Offset(176),
		Offset(0xB1), //Offset(177),
		, 4,
		CDVL, 1,
		, 3,
		Offset(0xB5), //Offset(181),
		LBPC, 8,
		Offset(0xBC), //Offset(188),
		ASLS, 32,
	}

	Method (XBCM, 1, NotSerialized)
	{
		If(LAnd(LGreaterEqual(Arg0,0),LLessEqual(Arg0,100)))
		{
			\_SB.PCI0.GFX0.AINT(1, Arg0) // Generate ASLE backlight brightness
			Store(Arg0,BRTL) // Store Brightness Level.
		}
	}

	Method (XBQC, 0, NotSerialized)
	{
		Return(BRTL)
	}

	Method (XBCL, 0, NotSerialized)
	{
		Return(Package(){80, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100})
	}

	Method (_DOS, 1)
	{
		Store (And(Arg0, 7), DSEN)
	}

	/* Check if the driver is ready to handle ASLE interrupts
	 * generate by the system BIOS.
	 * Input: none
	 * Output: Returns 0 = success, 1 = failure. */
	Method(PARD)
	{
		If(LNot(ARDY))
		{
		    /* Sleep for ASLP milliseconds if the driver is not ready. */
		    Sleep(ASLP)
		}

		/* If ARDY is clear, the driver is not ready. If the return value is !=0
		 *  do not generate the ASLE interrupt. */
		Return(LNot(ARDY))
	}

	/* Call the appropriate methods to generate an ASLE interrupt.
	 * This process includes ensuring the graphics driver is ready
	 * to process the interrupt, and passing information about the event
	 * to the graphics driver.
	 * Input:
	 * 	Arg0 = ASLE command function code
	 * 	0 = Set ALS illuminance
	 * 	1 = Set backlight brightness
	 * Output: Returns 0 = success, 1 = failure */

	Method(AINT, 2)
	{
		/* Return failure if the requested feature is not supported by the driver. */

		If(LNot(And(TCHE, ShiftLeft(1, Arg0))))
		{
			Return(0x1)
		}

		/* Return failure if the driver is not ready to handle an ASLE interrupt. */
		If(PARD())
		{
			Return(0x1)
		}
		If(LEqual(Arg0, 1)) // Arg0=1, so set the backlight brightness.
		{
			Store(Divide(Multiply(Arg1, 255), 100), BCLP) // Convert from percent to 0-255.
			Or(BCLP, 0x80000000, BCLP) // Set the valid bit.
			Store(2, ASLC) // Store "Backlight control event" to ASLC[31:1]
		}
		Else
		{
			If(LEqual(Arg0, 0)) // Arg0=0, so set the ALS illuminace
			{
				Store(Arg1, ALSI)
				Store(1, ASLC) // Store "ALS event" to ASLC[31:1]
			}
			Else
			{
				Return(0x1) // Unsupported function
			}
		}

		Store(0x01, ASLE) // Generate ASLE interrupt
		Return(0x0) // Return success
	}
}
