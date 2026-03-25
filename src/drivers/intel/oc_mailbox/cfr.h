/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * CFR objects and form for drivers/intel/oc_mailbox
 */

#ifndef DRIVERS_INTEL_OC_MAILBOX_CFR_H
#define DRIVERS_INTEL_OC_MAILBOX_CFR_H

#include <drivers/option/cfr_frontend.h>

static const struct sm_object oc_undervolt_apply = SM_DECLARE_BOOL({
	.opt_name	= "oc_undervolt_apply",
	.ui_name	= "Apply OC mailbox undervolt",
	.ui_helptext	= "Program MSR OC_MAILBOX offsets at boot.\n"
			  "Use with caution; may cause instability.\n"
			  "Turn off to use platform defaults.",
	.default_value	= false,
});

static const struct sm_object oc_undervolt_core = SM_DECLARE_NUMBER({
	.opt_name	= "oc_undervolt_core",
	.ui_name	= "CPU core undervolt (mV)",
	.ui_helptext	= "MSR plane 0. 0 = no undervolt. Range: 0-125 mV.",
	.default_value	= 0,
	.min		= 0,
	.max		= 125,
	.step		= 1,
	.display_flags	= 0,
}, WITH_DEP_VALUES(&oc_undervolt_apply, true));

static const struct sm_object oc_undervolt_gpu = SM_DECLARE_NUMBER({
	.opt_name	= "oc_undervolt_gpu",
	.ui_name	= "Intel GPU undervolt (mV)",
	.ui_helptext	= "MSR plane 1 (GT). Range: 0-125 mV.",
	.default_value	= 0,
	.min		= 0,
	.max		= 125,
	.step		= 1,
	.display_flags	= 0,
}, WITH_DEP_VALUES(&oc_undervolt_apply, true));

static const struct sm_object oc_undervolt_cache = SM_DECLARE_NUMBER({
	.opt_name	= "oc_undervolt_cache",
	.ui_name	= "CPU Cache undervolt (mV)",
	.ui_helptext	= "MSR plane 2. Range: 0-125 mV.",
	.default_value	= 0,
	.min		= 0,
	.max		= 125,
	.step		= 1,
	.display_flags	= 0,
}, WITH_DEP_VALUES(&oc_undervolt_apply, true));

static const struct sm_object oc_undervolt_sa = SM_DECLARE_NUMBER({
	.opt_name	= "oc_undervolt_sa",
	.ui_name	= "System Agent undervolt (mV)",
	.ui_helptext	= "MSR plane 3. Range: 0-125 mV.",
	.default_value	= 0,
	.min		= 0,
	.max		= 125,
	.step		= 1,
	.display_flags	= 0,
}, WITH_DEP_VALUES(&oc_undervolt_apply, true));

static const struct sm_object oc_undervolt_analog_io = SM_DECLARE_NUMBER({
	.opt_name	= "oc_undervolt_analog_io",
	.ui_name	= "Analog I/O undervolt (mV)",
	.ui_helptext	= "MSR plane 4. Range: 0-125 mV.",
	.default_value	= 0,
	.min		= 0,
	.max		= 125,
	.step		= 1,
	.display_flags	= 0,
}, WITH_DEP_VALUES(&oc_undervolt_apply, true));

static const struct sm_object oc_undervolt_digital_io = SM_DECLARE_NUMBER({
	.opt_name	= "oc_undervolt_digital_io",
	.ui_name	= "Digital I/O undervolt (mV)",
	.ui_helptext	= "MSR plane 5. Range: 0-125 mV.",
	.default_value	= 0,
	.min		= 0,
	.max		= 125,
	.step		= 1,
	.display_flags	= 0,
}, WITH_DEP_VALUES(&oc_undervolt_apply, true));

static struct sm_obj_form cpu_voltage = {
	.ui_name = "CPU voltage (OC mailbox)",
	.obj_list = (const struct sm_object *[]) {
		&oc_undervolt_apply,
		&oc_undervolt_core,
		&oc_undervolt_gpu,
		&oc_undervolt_cache,
		&oc_undervolt_sa,
		&oc_undervolt_analog_io,
#if CONFIG(HAS_DIGITAL_IO_POWER_DOMAIN)
		&oc_undervolt_digital_io,
#endif
		NULL
	},
};

#endif /* DRIVERS_INTEL_OC_MAILBOX_CFR_H */
