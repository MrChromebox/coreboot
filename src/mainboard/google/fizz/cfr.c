
/* SPDX-License-Identifier: GPL-2.0-only */
#include <boot/coreboot_tables.h>
#include <commonlib/coreboot_tables.h>
#include <drivers/option/cfr_frontend.h>
#include <inttypes.h>
#include <string.h>
#include <types.h>

static const struct sm_object ec_sw_sync = SM_DECLARE_BOOL({
	.opt_name	= "ec_sw_sync",
	.ui_name	= "EC Software Sync",
	.ui_helptext	= "Enable or disable updating of EC-RW firmware",
	.default_value	= true,
});

static const struct sm_object ec_rw_jump = SM_DECLARE_BOOL({
	.opt_name	= "ec_rw_jump",
	.ui_name	= "Use EC-RW Firmware",
	.ui_helptext	= "Jump to EC-RW firmware after EC SW Sync (if enabled). "
			  "If disabled, EC-RO firmware will be used instead.",
	.default_value	= true,
});

static const struct sm_object debug_level = SM_DECLARE_ENUM({
	.opt_name	= "debug_level",
	.ui_name	= "Debug Level",
	.ui_helptext	= "Set the verbosity of the debug output.",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "Emergency",		0		},
				{ "Alert",		1		},
				{ "Critical",		2		},
				{ "Error",		3		},
				{ "Warning",		4		},
				{ "Notice",		5		},
				{ "Info",		6		},
				{ "Debug",		7		},
				{ "Spew",		8		},
				SM_ENUM_VALUE_END			},
});

static const struct sm_object me_state = SM_DECLARE_ENUM({
	.opt_name	= "me_state",
	.ui_name	= "Intel Management Engine",
	.ui_helptext	= "Enable or disable the Intel Management Engine",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "Disabled",		1		},
				{ "Enabled",		0		},
				SM_ENUM_VALUE_END			},
});

static const struct sm_object me_state_counter = SM_DECLARE_NUMBER({
	.opt_name	= "me_state_counter",
	.ui_name	= "ME State Counter",
	.flags		= CFR_OPTFLAG_SUPPRESS,
	.default_value	= 0,
});

static const struct sm_object igd_dvmt = SM_DECLARE_ENUM({
	.opt_name	= "IgdDvmt50PreAlloc",
	.ui_name	= "IGD DVMT Size",
	.ui_helptext	= "Size of memory preallocated for internal graphics",
	.default_value	= 2,
	.values		= (const struct sm_enum_value[]) {
				{ "32 MB",		1		},
				{ "64 MB",		2		},
				{ "96 MB",		3		},
				{ "128 MB",		4		},
				SM_ENUM_VALUE_END			},
});

static const struct sm_object igd_aperture = SM_DECLARE_ENUM({
	.opt_name	= "ApertureSize",
	.ui_name	= "IGD Aperture Size",
	.ui_helptext	= "Select the Aperture Size",
	.default_value	= 0,
	.values		= (const struct sm_enum_value[]) {
				{ "128 MB",		0		},
				{ "256 MB",		1		},
				{ "512 MB",		2		},
				{ "1024 MB",		3		},
				SM_ENUM_VALUE_END			},
});

enum cfr_power_profile {
	PP_POWER_SAVER = 0,
	PP_BALANCED    = 1,
	PP_PERFORMANCE = 2,
};

static const struct sm_object hyperthreading = SM_DECLARE_BOOL({
	.opt_name	= "hyper_threading",
	.ui_name	= "Hyperthreading",
	.ui_helptext	= "Enable or disable CPU hyperthreading",
	.default_value	= true,
});

static const struct sm_object power_on_after_fail = SM_DECLARE_BOOL({
	.opt_name	= "power_on_after_fail",
	.ui_name	= "Power on after failure",
	.ui_helptext	= "Automatically turn on after a power failure",
	.default_value	= false,
});

static const struct sm_object power_profile = SM_DECLARE_ENUM({
	.opt_name	= "power_profile",
	.ui_name	= "Power Profile",
	.ui_helptext	= "Select whether to maximize performance, battery life or both.",
	.default_value	= 2,
	.flags		= CFR_OPTFLAG_INACTIVE,
	.values		= (const struct sm_enum_value[]) {
				{ "Power Saver",	PP_POWER_SAVER	},
				{ "Balanced",		PP_BALANCED	},
				{ "Performance",	PP_PERFORMANCE	},
				SM_ENUM_VALUE_END			},
});

static const struct sm_object sleep_mode = SM_DECLARE_ENUM({
	.opt_name	= "s0ix_enable",
	.ui_name	= "Sleep Mode",
	.ui_helptext	= "Select between legacy S3 sleep mode or S0ix (Modern Standby)",
	.default_value	= 1,
	.values		= (const struct sm_enum_value[]) {
				{ "ACPI S3",		0	},
				{ "S0ix",		1	},
				SM_ENUM_VALUE_END			},
});

static const struct sm_object wireless = SM_DECLARE_BOOL({
	.opt_name	= "wireless",
	.ui_name	= "Wireless",
	.flags		= CFR_OPTFLAG_INACTIVE,
	.ui_helptext	= "Enable or disable the built-in wireless card",
	.default_value	= true,
});

static const struct sm_object vtd = SM_DECLARE_BOOL({
	.opt_name	= "vtd",
	.ui_name	= "VT-d",
	.ui_helptext	= "Enable or disable Intel VT-d (virtualization)",
	.default_value	= true,
});

static struct sm_obj_form performance = {
	.ui_name = "Performance",
	.obj_list = (const struct sm_object *[]) {
		&power_profile,
		NULL
	},
};

static struct sm_obj_form processor = {
	.ui_name = "Processor",
	.obj_list = (const struct sm_object *[]) {
		&me_state,
		&me_state_counter,
		&vtd,
		&hyperthreading,
		&igd_dvmt,
		&igd_aperture,
		NULL
	},
};

static struct sm_obj_form power = {
	.ui_name = "Power",
	.obj_list = (const struct sm_object *[]) {
		&power_on_after_fail,
		&sleep_mode,
		NULL
	},
};

static struct sm_obj_form devices = {
	.ui_name = "Devices",
	.obj_list = (const struct sm_object *[]) {
		&wireless,
		NULL
	},
};

static struct sm_obj_form pci = {
	.ui_name  = "PCI",
	.flags    = CFR_OPTFLAG_SUPPRESS,
	.obj_list = (const struct sm_object *[]) {
		NULL
	},
};

static struct sm_obj_form coreboot = {
	.ui_name = "coreboot",
	.obj_list = (const struct sm_object *[]) {
		&debug_level,
		NULL
	},
};

static struct sm_obj_form ec = {
	.ui_name = "Embedded Controller",
	.obj_list = (const struct sm_object *[]) {
		&ec_sw_sync,
		&ec_rw_jump,
		NULL
	},
};

static struct sm_obj_form *sm_root[] = {
	&performance,
	&processor,
	&ec,
	&power,
	&devices,
	&pci,
	&coreboot,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
