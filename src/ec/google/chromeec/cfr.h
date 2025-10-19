/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * CFR enums and structs which are used to control EC settings.
 */

#ifndef CHROMEEC_CFR_H
#define CHROMEEC_CFR_H

#include <drivers/option/cfr_frontend.h>
#include "ec.h"

static const struct sm_object ec_sw_sync = SM_DECLARE_BOOL({
	.opt_name	= "ec_sw_sync",
	.ui_name	= "EC Software Sync",
	.ui_helptext	= "Enable or disable updating of EC-RW firmware",
	.default_value	= true,
});

static const struct sm_object ec_rw_jump = SM_DECLARE_BOOL({
	.opt_name	= "ec_rw_jump",
	.ui_name	= "Use EC-RW Firmware",
	.ui_helptext	= "Jump to EC-RW firmware after EC SW Sync. "
			  "Warning: Disabling EC-RW may result in undefined behavior, ranging"
			  "from USB-C/TBT accessories not working to the keyboard not working."
			  "If disabled, EC-RO firmware will be used instead.",
	.default_value	= true,
});

static void update_fan_control(const struct sm_object *obj, struct sm_object *new)
{
	if (!CONFIG(EC_GOOGLE_CHROMEEC_AUTO_FAN_CTRL) && !google_chromeec_has_fan()) {
		new->sm_bool.flags = CFR_OPTFLAG_SUPPRESS;
		new->sm_bool.default_value = 0;
	}
}

static const struct sm_object auto_fan_control = SM_DECLARE_ENUM({
	.opt_name       = "auto_fan_control",
	.ui_name        = "Automatic Fan Control",
	.ui_helptext    = "Enable or disable automatic fan control.",
	.default_value  = CONFIG(EC_GOOGLE_CHROMEEC_AUTO_FAN_CTRL),
	.values         = (struct sm_enum_value[]) {
		{ "Enabled", 1 },
		{ "Disabled", 0 },
		SM_ENUM_VALUE_END,
	},
}, WITH_CALLBACK(update_fan_control));

static const struct sm_enum_value ec_backlight_values[] = {
	{ "0%", 0 },
	{ "25%", 25 },
	{ "50%", 50 },
	{ "100%", 100 },
	SM_ENUM_VALUE_END,
};

static void update_kb_backlight(const struct sm_object *obj, struct sm_object *new)
{
	if (!google_chromeec_has_kbbacklight()) {
		new->sm_bool.flags = CFR_OPTFLAG_SUPPRESS;
		new->sm_bool.default_value = -1;
	}
}

static const struct sm_object ec_kb_backlight = SM_DECLARE_ENUM({
	.opt_name       = "ec_kb_backlight",
	.ui_name        = "Keyboard Backlight Level At Boot",
	.ui_helptext    = "Specify the level the keyboard backlight should be at boot."
			" Set to 0% to disable the backlight.",
	.default_value  = 50,
	.values         = ec_backlight_values,
}, WITH_CALLBACK(update_kb_backlight));

#endif /* CHROMEEC_CFR_H */
