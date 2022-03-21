/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/variants.h>
#include <bootmode.h>
#include <chip.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <ec/google/chromeec/ec.h>
#include <gpio.h>
#include <soc/pci_devs.h>
#include <timer.h>

#define GPIO_HDMI_HPD		GPP_E13
#define GPIO_DP_HPD		GPP_E14

/* TODO: This can be moved to common directory */
static void wait_for_hpd(gpio_t gpio, long timeout)
{
	struct stopwatch sw;

	printk(BIOS_INFO, "Waiting for HPD\n");
	stopwatch_init_msecs_expire(&sw, timeout);
	while (!gpio_get(gpio)) {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_WARNING,
			       "HPD not ready after %ldms. Abort.\n", timeout);
			return;
		}
		mdelay(200);
	}
	printk(BIOS_INFO, "HPD ready after %lu ms\n",
	       stopwatch_duration_msecs(&sw));
}

void variant_ramstage_init(void)
{
	static const long display_timeout_ms = 3000;
	struct soc_power_limits_config *soc_config;
	config_t *conf = config_of_soc();

	/* This is reconfigured back to whatever FSP-S expects by gpio_configure_pads. */
	gpio_input(GPIO_HDMI_HPD);
	gpio_input(GPIO_DP_HPD);
	if (display_init_required()
		&& !gpio_get(GPIO_HDMI_HPD)
		&& !gpio_get(GPIO_DP_HPD)) {
		/* This has to be done before FSP-S runs. */
		if (google_chromeec_wait_for_displayport(display_timeout_ms))
			wait_for_hpd(GPIO_DP_HPD, display_timeout_ms);
	}
}
