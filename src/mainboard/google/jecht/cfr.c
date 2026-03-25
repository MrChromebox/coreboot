/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <cpu/intel/haswell/cfr.h>
#include <drivers/option/cfr_frontend.h>
#include <soc/cfr.h>

static const struct sm_object tdp_pl1_override = SM_DECLARE_NUMBER({
	.opt_name	= "tdp_pl1_override",
	.ui_name	= "CPU PL1 power limit (W)",
	.ui_helptext	= "Long-duration CPU package power limit.\n"
			  "Default: 15 W. Range: 15-25 W.",
	.default_value	= 15,
	.min		= 15,
	.max		= 25,
	.step		= 1,
	.display_flags	= 0,
});

static const struct sm_object tdp_pl2_override = SM_DECLARE_NUMBER({
	.opt_name	= "tdp_pl2_override",
	.ui_name	= "CPU PL2 power limit (W)",
	.ui_helptext	= "Short-duration CPU package power limit.\n"
			  "Default: 18 W. Range: 15-30 W.\n"
			  "Must be >= PL1 (enforced at boot).",
	.default_value	= 18,
	.min		= 15,
	.max		= 30,
	.step		= 1,
	.display_flags	= 0,
});

static struct sm_obj_form system = {
	.ui_name = "System",
	.obj_list = (const struct sm_object *[]) {
		&me_disable,
		NULL
	},
};

static struct sm_obj_form power = {
	.ui_name = "Power",
	.obj_list = (const struct sm_object *[]) {
		&power_on_after_fail,
		&tdp_pl1_override,
		&tdp_pl2_override,
		&cpu_power_limit_lock,
		NULL
	},
};

static struct sm_obj_form *sm_root[] = {
	&system,
	&power,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
