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

#endif /* CHROMEEC_CFR_H */
