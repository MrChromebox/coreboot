/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _CPU_INTEL_HASWELL_CFR_H_
#define _CPU_INTEL_HASWELL_CFR_H_

#include <drivers/option/cfr_frontend.h>

static const struct sm_object cpu_power_limit_lock = SM_DECLARE_BOOL({
	.opt_name	= "cpu_power_limit_lock",
	.ui_name	= "CPU power limit lock",
	.ui_helptext	= "Lock CPU package power limits after programming.\n"
			  "This prevents the power limits from being changed by the OS or runtime tools.\n",
	.default_value	= false,
});

#endif /* _CPU_INTEL_HASWELL_CFR_H_ */
