/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
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

Device (GFX0)
{
	Name (_ADR, 0x00020000)

	OperationRegion (GFXC, PCI_Config, 0x00, 0x0100)
	Field (GFXC, DWordAcc, NoLock, Preserve)
	{
		Offset (0x10),
		BAR0, 64,
		Offset (0xe4),
		ASLE, 32,
		Offset (0xfc),
		ASLS, 32,
	}

	OperationRegion (GFRG, SystemMemory, And (BAR0, 0xfffffffffffffff0), 0x400000)
	Field (GFRG, DWordAcc, NoLock, Preserve)
	{
		Offset (0x48254),
		BCLV, 16,
		Offset (0xc8256),
		BCLM, 16
	}

	OperationRegion (IGDM, SystemMemory, ASLB, 0x2000)
	Field (IGDM, AnyAcc, NoLock, Preserve)
	{
		SIGN,   128,
		SIZE,   32,
		OVER,   32,
		SVER,   256,
		VVER,   128,
		GVER,   128,
		MBOX,   32,
		DMOD,   32,
		PCON,   32,
		DVER,   256,
		Offset (0x100),
		DRDY,   32,
		CSTS,   32,
		CEVT,   32,
		Offset (0x120),
		DIDL,   32,
		DDL2,   32,
		DDL3,   32,
		DDL4,   32,
		DDL5,   32,
		DDL6,   32,
		DDL7,   32,
		DDL8,   32,
		CPDL,   32,
		CPL2,   32,
		CPL3,   32,
		CPL4,   32,
		CPL5,   32,
		CPL6,   32,
		CPL7,   32,
		CPL8,   32,
		CADL,   32,
		CAL2,   32,
		CAL3,   32,
		CAL4,   32,
		CAL5,   32,
		CAL6,   32,
		CAL7,   32,
		CAL8,   32,
		NADL,   32,
		NDL2,   32,
		NDL3,   32,
		NDL4,   32,
		NDL5,   32,
		NDL6,   32,
		NDL7,   32,
		NDL8,   32,
		ASLP,   32,
		TIDX,   32,
		CHPD,   32,
		CLID,   32,
		CDCK,   32,
		SXSW,   32,
		EVTS,   32,
		CNOT,   32,
		NRDY,   32,
		DDL9,   32,
		DD10,   32,
		DD11,   32,
		DD12,   32,
		DD13,   32,
		DD14,   32,
		DD15,   32,
		CPL9,   32,
		CP10,   32,
		CP11,   32,
		CP12,   32,
		CP13,   32,
		CP14,   32,
		CP15,   32,
		Offset (0x200),
		SCIE,   1,
		GEFC,   4,
		GXFC,   3,
		GESF,   8,
		Offset (0x204),
		PARM,   32,
		DSLP,   32,
		Offset (0x300),
		ARDY,   32,
		ASLC,   32,
		TCHE,   32,
		ALSI,   32,
		BCLP,   32,
		PFIT,   32,
		CBLV,   32,
		BCLT,   320,
		CPFM,   32,
		EPFM,   32,
		PLUT,   592,
		PFMB,   32,
		CCDV,   32,
		PCFT,   32,
		SROT,   32,
		IUER,   32,
		FDSP,   64,
		FDSS,   32,
		STAT,   32,
		Offset (0x400),
		GVD1,   49152,
		PHED,   32,
		BDDC,   2048
	}

#include "configure_brightness_levels.asl"
#include "common.asl"
}
