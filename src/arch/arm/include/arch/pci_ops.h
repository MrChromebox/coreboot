/*
 * This file is part of the coreboot project.
 *
 * Copyright 2013 Google Inc.
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

#if CONFIG_INTEL_LYNXPOINT_LP
#include "microcode-M7240650_ffff000a.h"
#include "microcode-M7240651_0000001c.h"
#else
#include "microcode-M32306c1_ffff000d.h"
#include "microcode-M32306c2_ffff0003.h"
#include "microcode-M32306c3_0000001c.h"
#include "microcode-M3240660_ffff000b.h"
#endif
	/*  Dummy terminator  */
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
