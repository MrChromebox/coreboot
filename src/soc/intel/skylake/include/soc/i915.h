/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Google Inc.
 * Copyright (C) 2018 Intel Corporation.
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

#ifndef INTEL_I915_H
#define INTEL_I915_H

#define I915_MAX_DISPLAYS	5

struct i915_gpu_controller_info {
	int ndid;
	u32 did[I915_MAX_DISPLAYS];
	const char *names[I915_MAX_DISPLAYS];
};

void intel_igd_displays_ssdt_generate(struct i915_gpu_controller_info *conf);
void gma_ssdt(struct device *dev);
struct i915_gpu_controller_info *
intel_igd_get_controller_info(void);

#endif
