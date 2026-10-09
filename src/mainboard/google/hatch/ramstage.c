/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <baseboard/variants.h>
#include <bootmode.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <drivers/i2c/generic/chip.h>
#include <ec/ec.h>
#include <ec/google/chromeec/ec.h>
#include <intelblocks/gpio.h>
#include <option.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include <static.h>
#include <timer.h>
#include <variant/gpio.h>

#define TYPEC_DP_TIMEOUT_MS	2000

void mainboard_silicon_init_params(FSPS_UPD *supd)
{
	variant_devtree_update();
}

void __weak variant_devtree_update(void)
{
	/* Override dev tree settings per board */
}

void __weak variant_ramstage_init(void)
{
	/* Default weak implementation */
}

void __weak variant_mainboard_enable(struct device *dev)
{
	/* Override mainboard settings per board */
}

static void update_touchpad_wake_config(void)
{
	const struct device *touchpad = DEV_PTR(touchpad);
	if (!touchpad || !touchpad->chip_info)
		return;

	/* Check CFR option for touchpad wake */
	unsigned int touchpad_wake_enabled = get_uint_option("touchpad_wake", 0);

	printk(BIOS_DEBUG, "Touchpad wake: %s\n", touchpad_wake_enabled?"enabled":"disabled");

	/* Modify devicetree config directly */
	struct drivers_i2c_generic_config *config =
		(struct drivers_i2c_generic_config *)touchpad->chip_info;
	if (!touchpad_wake_enabled) {
		/* Disable wake GPE and wake flag in IRQ descriptor */
		config->wake = 0;
		config->irq.wake = 0;

		/* Reconfigure GPIO pad to remove wake capability */
		const struct pad_config touchpad_gpio[] = {
			PAD_CFG_GPI_APIC(GPP_A21, NONE, DEEP, LEVEL, INVERT),
		};
		gpio_configure_pads(touchpad_gpio, ARRAY_SIZE(touchpad_gpio));
	}
}

/*
 * The EC exits DP alt mode while the AP is off and only re-enters it after
 * power-on, which may not be done yet by the time the display is probed.
 * Wait on ports where we are DFP with a partner attached (mux in USB mode)
 * until the EC has switched the mux to DP.
 */
static void wait_for_typec_dp(void)
{
	struct usbc_mux_info info;
	struct stopwatch sw;
	unsigned int num_ports, port;
	uint32_t pending;

	if (CONFIG(NO_GFX_INIT) || acpi_is_wakeup_s3() || !display_init_required())
		return;

	if (google_chromeec_get_num_pd_ports(&num_ports))
		return;

	stopwatch_init_msecs_expire(&sw, TYPEC_DP_TIMEOUT_MS);
	do {
		pending = 0;
		for (port = 0; port < num_ports; port++) {
			if (google_chromeec_get_usbc_mux_info(port, &info))
				continue;
			if (info.usb && !info.dp && !info.ufp)
				pending |= BIT(port);
		}
		if (!pending)
			break;
		mdelay(20);
	} while (!stopwatch_expired(&sw));

	if (pending)
		printk(BIOS_INFO, "USB-C DP not entered after %d ms (ports 0x%x)\n",
		       TYPEC_DP_TIMEOUT_MS, pending);
	else if (stopwatch_duration_msecs(&sw))
		printk(BIOS_INFO, "USB-C DP ready after %lld ms\n",
		       stopwatch_duration_msecs(&sw));
}

static void mainboard_init(struct device *dev)
{
	mainboard_ec_init();
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->init = mainboard_init;
	variant_mainboard_enable(dev);
	update_touchpad_wake_config();
}

static void mainboard_chip_init(void *chip_info)
{
	const struct pad_config *base_table;
	const struct pad_config *override_table;
	size_t base_gpios;
	size_t override_gpios;

	base_table = base_gpio_table(&base_gpios);
	override_table = override_gpio_table(&override_gpios);

	gpio_configure_pads_with_override(base_table,
					base_gpios,
					override_table,
					override_gpios);

	wait_for_typec_dp();
	variant_ramstage_init();
}

void __weak variant_final(void)
{
}

static void mainboard_final(void *chip_info)
{
	const struct pad_config *variant_finalize;
	size_t variant_gpios;
	variant_finalize = variant_finalize_gpio_table(&variant_gpios);
	gpio_configure_pads(variant_finalize, variant_gpios);

	variant_final();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_chip_init,
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};
