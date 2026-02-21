
/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <drivers/option/cfr_frontend.h>
#include <ec/google/chromeec/cfr.h>
#include <intelblocks/cfr.h>
#include <soc/cfr.h>

#if CONFIG(BOARD_GOOGLE_TANIKS) || CONFIG(BOARD_GOOGLE_TAEKO)

enum storage_device {
	STORAGE_NVME = 0,
	STORAGE_EMMC = 1,
};

static const struct sm_object storage_device_opt = SM_DECLARE_ENUM({
	.opt_name	= "storage_device",
	.ui_name	= "Storage Device",
	.ui_helptext	= "Select which storage device to use (NVMe SSD or eMMC)",
	.default_value	= STORAGE_NVME,
	.values		= (const struct sm_enum_value[]) {
				{ "NVMe SSD",		STORAGE_NVME	},
				{ "eMMC",		STORAGE_EMMC	},
				SM_ENUM_VALUE_END			},
});
static struct sm_obj_form devices = {
	.ui_name = "Devices",
	.obj_list = (const struct sm_object *[]) {
		&storage_device_opt,
		NULL
	},
};
#endif

static struct sm_obj_form system = {
	.ui_name = "System",
	.obj_list = (const struct sm_object *[]) {
		&hyper_threading,
		&igd_dvmt,
		&igd_aperture,
		&legacy_8254_timer,
		&me_state,
		&me_state_counter,
		&me_heci1,
		&pciexp_aspm,
		&pciexp_clk_pm,
		&pciexp_l1ss,
		&pciexp_speed,
		&s0ix_enable,
		&vtd,
		NULL
	},
};

static struct sm_obj_form ec = {
	.ui_name = "ChromeEC Embedded Controller",
	.obj_list = (const struct sm_object *[]) {
		&auto_fan_control,
		&ec_kb_backlight,
		&ec_rgb_kb_color,
		&ec_sw_sync,
		&ec_rw_jump,
		NULL
	},
};

static struct sm_obj_form *sm_root[] = {
	&system,
#if CONFIG(BOARD_GOOGLE_TANIKS) || CONFIG(BOARD_GOOGLE_TAEKO)
	&devices,
#endif
	&ec,
	NULL
};

void mb_cfr_setup_menu(struct lb_cfr *cfr_root)
{
	cfr_write_setup_menu(cfr_root, sm_root);
}
