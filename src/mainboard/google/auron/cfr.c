/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <drivers/option/cfr_frontend.h>
#include <ec/google/chromeec/cfr.h>
#include <soc/cfr.h>

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
		NULL
	},
};

static struct sm_obj_form ec = {
	.ui_name = "ChromeEC Embedded Controller",
	.obj_list = (const struct sm_object *[]) {
		&ec_sw_sync,
		&ec_rw_jump,
		&ec_kb_backlight,
		NULL
	},
};
static struct sm_obj_form *sm_root[] = {
	&system,
	&power,
	&ec,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
