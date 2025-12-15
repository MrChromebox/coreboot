/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdlib.h>
#include <acpi/acpi.h>
#include <acpi/acpi_device.h>
#include <acpi/acpigen.h>
#include <acpi/acpigen_pci.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <stdio.h>

#include "chip.h"
#include "camera.h"

static struct camera_resource_manager res_mgr;

static void resource_set_action_type(struct resource_config *res_config,
				     enum action_type action)
{
	if (res_config)
		res_config->action = action;
}

static enum action_type resource_get_action_type(const struct resource_config *res_config)
{
	return res_config ? res_config->action : UNKNOWN_ACTION;
}

static enum ctrl_type resource_get_ctrl_type(const struct resource_config *res_config)
{
	return res_config ? res_config->type : UNKNOWN_CTRL;
}

static void resource_set_clk_config(struct resource_config *res_config,
				    const struct clk_config *clk_conf)
{
	if (res_config) {
		res_config->type = IMGCLK;
		res_config->clk_conf = clk_conf;
	}
}

static const struct clk_config *resource_clk_config(const struct resource_config *res_config)
{
	return res_config ? res_config->clk_conf : NULL;
}

static void resource_set_gpio_config(struct resource_config *res_config,
				     const struct gpio_config *gpio_conf)
{
	if (res_config) {
		res_config->type = GPIO;
		res_config->gpio_conf = gpio_conf;
	}
}

static const struct gpio_config *resource_gpio_config(const struct resource_config *res_config)
{
	return res_config ? res_config->gpio_conf : NULL;
}

void apply_pld_defaults(struct drivers_intel_mipi_camera_config *config)
{
	if (!config->pld.ignore_color)
		config->pld.ignore_color = 1;

	if (!config->pld.visible)
		config->pld.visible = 1;

	if (!config->pld.vertical_offset)
		config->pld.vertical_offset = 0xffff;

	if (!config->pld.horizontal_offset)
		config->pld.horizontal_offset = 0xffff;

	/*
	 * PLD_PANEL_TOP has a value of zero, so the following will change any instance of
	 * PLD_PANEL_TOP to PLD_PANEL_FRONT.
	 */
	if (!config->pld.panel)
		config->pld.panel = PLD_PANEL_FRONT;

	/*
	 * PLD_HORIZONTAL_POSITION_LEFT has a value of zero, so the following will change any
	 * instance of that value to PLD_HORIZONTAL_POSITION_CENTER.
	 */
	if (!config->pld.horizontal_position)
		config->pld.horizontal_position = PLD_HORIZONTAL_POSITION_CENTER;

	/*
	 * The desired default for |vertical_position| is PLD_VERTICAL_POSITION_UPPER, which
	 * has a value of zero so no work is needed to set a default. The same applies for
	 * setting |shape| to PLD_SHAPE_ROUND.
	 */
}

void camera_generate_pld(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;

	apply_pld_defaults(config);

	acpigen_write_pld(&config->pld);
}

uint32_t address_for_dev_type(const struct device *dev, uint8_t dev_type)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;
	uint16_t i2c_bus = dev->upstream ? dev->upstream->secondary : 0xFFFF;
	uint16_t i2c_addr;

	switch (dev_type) {
	case DEV_TYPE_SENSOR:
		i2c_addr = dev->path.i2c.device;
		break;
	case DEV_TYPE_VCM:
		i2c_addr = config->vcm_address;
		break;
	case DEV_TYPE_ROM:
		i2c_addr = config->rom_address;
		break;
	default:
		return 0;
	}

	return (((uint32_t)i2c_bus) << 24 | ((uint32_t)i2c_addr) << 8 | dev_type);
}

/*
 * Generate ASL DSM code for Sensor Device
 */
void camera_generate_dsm_sensor(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;

	/* If (LEqual (Local0, ToUUID (UUID_DSM_SENSOR))) */
	acpigen_write_if();
	acpigen_emit_byte(LEQUAL_OP);
	acpigen_emit_byte(LOCAL0_OP);
	acpigen_write_uuid(UUID_DSM_SENSOR);

	/* If (LEqual (Arg2, Zero)) */
	acpigen_write_if_lequal_op_int(ARG2_OP, 0);
	/* If (LEqual (Arg1, Zero)) */
	acpigen_write_if_lequal_op_int(ARG1_OP, 0);
	/* Return (Buffer (One) { 0x3 }) */
	acpigen_write_return_singleton_buffer(0x3);
	/* Else */
	acpigen_write_else();
	/* Return (Buffer (One) { 0x1 }) */
	acpigen_write_return_singleton_buffer(0x1);
	acpigen_pop_len();	/* If Arg1=0 */

	acpigen_pop_len();	/* If Arg2=0 */

	/* If (LEqual (Arg2, One)) */
	acpigen_write_if_lequal_op_int(ARG2_OP, 1);
	acpigen_write_return_string(config && config->sensor_name ? config->sensor_name : "UNKNOWN");
	acpigen_pop_len();	/* If Arg2=1 */

	acpigen_pop_len();	/* If uuid */
}

/*
 * Generate ASL DSM code for I2C device count and addresses
 */
void camera_generate_dsm_i2c(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;
	int i2c_dev_count = 1 + (config->ssdb.vcm_type ? 1 : 0) + (config->ssdb.rom_type ? 1 : 0);
	int i2c_dev_idx = 1;

	/* If (LEqual (Local0, ToUUID (UUID_DSM_I2C))) */
	acpigen_write_if();
	acpigen_emit_byte(LEQUAL_OP);
	acpigen_emit_byte(LOCAL0_OP);
	acpigen_write_uuid(UUID_DSM_I2C);
	/* ToInteger (Arg2, Local1) */
	acpigen_write_to_integer(ARG2_OP, LOCAL1_OP);

	/* If (LEqual (Local1, 1)) */
	acpigen_write_if_lequal_op_int(LOCAL1_OP, i2c_dev_idx++);
	acpigen_write_return_integer(i2c_dev_count);
	acpigen_pop_len();	/* If Arg2=1 */

	/* If (LEqual (Local1, 2)) */
	acpigen_write_if_lequal_op_int(LOCAL1_OP, i2c_dev_idx++);
	acpigen_write_return_integer(address_for_dev_type(dev, DEV_TYPE_SENSOR));
	acpigen_pop_len();	/* If Arg2=2 */

	if (config->ssdb.vcm_type) {
		/* If (LEqual (Local1, 3)) */
		acpigen_write_if_lequal_op_int(LOCAL1_OP, i2c_dev_idx++);
		acpigen_write_return_integer(address_for_dev_type(dev, DEV_TYPE_VCM));
		acpigen_pop_len();      /* If Arg2=3 */
	}

	if (config->ssdb.rom_type) {
		/* If (LEqual (Local1, 3 or 4)) */
		acpigen_write_if_lequal_op_int(LOCAL1_OP, i2c_dev_idx);
		acpigen_write_return_integer(address_for_dev_type(dev, DEV_TYPE_ROM));
		acpigen_pop_len();      /* If Arg2=3 or 4 */
	}

	acpigen_pop_len();      /* If uuid */
}

static void camera_fill_ssdb_defaults(struct drivers_intel_mipi_camera_config *config)
{
	config->ssdb.version = 1;

	if (!config->ssdb.sensor_card_sku.card_type)
		config->ssdb.sensor_card_sku.card_type = SKU_CRD_D;

	guidcpy(&config->ssdb.csi2_data_stream_interface, &CSI2_DATA_STREAM_INTERFACE_GUID);

	if (!config->ssdb.bdf_value)
		config->ssdb.bdf_value = PCI_DEVFN(CIO2_PCI_DEV, CIO2_PCI_FN);

	if (!config->ssdb.flash_support)
		config->ssdb.flash_support = FLASH_DISABLE;

	if (!config->ssdb.privacy_led)
		config->ssdb.privacy_led = PRIVACY_LED_A_16mA;

	if (!config->ssdb.mipi_define)
		config->ssdb.mipi_define = MIPI_INFO_ACPI_DEFINED;

	if (!config->ssdb.mclk_speed)
		config->ssdb.mclk_speed = CLK_FREQ_19_2MHZ;
}

int get_resource_index(const struct resource_config *res_config)
{
	enum ctrl_type type = resource_get_ctrl_type(res_config);
	const struct clk_config *clk_config;
	const struct gpio_config *gpio_config;
	unsigned int i;
	uint8_t res_id;

	switch (type) {
	case IMGCLK:
		clk_config = resource_clk_config(res_config);
		res_id = clk_config->clknum;
		break;
	case GPIO:
		gpio_config = resource_gpio_config(res_config);
		res_id = gpio_config->gpio_num;
		break;
	default:
		printk(BIOS_ERR, "Unsupported power operation: %x\n"
				 "OS camera driver will likely not work", type);
		return -1;
	}

	for (i = 0; i < res_mgr.cnt; i++)
		if (res_mgr.resource[i].type == type && res_mgr.resource[i].id == res_id)
			return i;

	return -1;
}

void add_guarded_method_namestring(struct resource_config *res_config, int res_index)
{
	char method_name[ACPI_NAME_BUFFER_SIZE];
	enum action_type action = resource_get_action_type(res_config);

	switch (action) {
	case ENABLE:
		snprintf(method_name, sizeof(method_name), ENABLE_METHOD_FORMAT, res_index);
		break;
	case DISABLE:
		snprintf(method_name, sizeof(method_name), DISABLE_METHOD_FORMAT, res_index);
		break;
	default:
		snprintf(method_name, sizeof(method_name), UNKNOWN_METHOD_FORMAT, res_index);
		printk(BIOS_ERR, "Unsupported resource action: %x\n", action);
	}

	acpigen_emit_namestring(method_name);
}

void call_guarded_method(struct resource_config *res_config)
{
	int res_index;

	if (res_config == NULL)
		return;

	res_index = get_resource_index(res_config);

	if (res_index != -1)
		add_guarded_method_namestring(res_config, res_index);
}

void add_clk_op(const struct clk_config *clk_config, enum action_type action)
{
	if (clk_config == NULL)
		return;

	switch (action) {
	case ENABLE:
		acpigen_write_if();
		acpigen_emit_ext_op(COND_REFOF_OP);
		acpigen_emit_string(CLK_ENABLE_METHOD);
		acpigen_emit_namestring(CLK_ENABLE_METHOD);
		acpigen_write_integer(clk_config->clknum);
		acpigen_write_integer(clk_config->freq);
		acpigen_pop_len(); /* CondRefOf */
		break;
	case DISABLE:
		acpigen_write_if();
		acpigen_emit_ext_op(COND_REFOF_OP);
		acpigen_emit_string(CLK_DISABLE_METHOD);
		acpigen_emit_namestring(CLK_DISABLE_METHOD);
		acpigen_write_integer(clk_config->clknum);
		acpigen_pop_len(); /* CondRefOf */
		break;
	default:
		acpigen_write_debug_string("Unsupported clock action");
		printk(BIOS_ERR, "Unsupported clock action: %x\n"
				 "OS camera driver will likely not work", action);
	}
}

void add_gpio_op(const struct gpio_config *gpio_config, enum action_type action)
{
	if (gpio_config == NULL)
		return;

	switch (action) {
	case ENABLE:
		acpigen_soc_set_tx_gpio(gpio_config->gpio_num);
		break;
	case DISABLE:
		acpigen_soc_clear_tx_gpio(gpio_config->gpio_num);
		break;
	default:
		acpigen_write_debug_string("Unsupported GPIO action");
		printk(BIOS_ERR, "Unsupported GPIO action: %x\n"
				 "OS camera driver will likely not work\n", action);
	}
}

void add_power_operation(const struct resource_config *res_config)
{
	const struct clk_config *clk_config;
	const struct gpio_config *gpio_config;
	enum ctrl_type type = resource_get_ctrl_type(res_config);
	enum action_type action = resource_get_action_type(res_config);

	if (res_config == NULL)
		return;

	switch (type) {
	case IMGCLK:
		clk_config = resource_clk_config(res_config);
		add_clk_op(clk_config, action);
		break;
	case GPIO:
		gpio_config = resource_gpio_config(res_config);
		add_gpio_op(gpio_config, action);
		break;
	default:
		printk(BIOS_ERR, "Unsupported power operation: %x\n"
				 "OS camera driver will likely not work\n", type);
		break;
	}
}

void write_guard_variable(uint8_t res_index)
{
	char varname[ACPI_NAME_BUFFER_SIZE];

	snprintf(varname, sizeof(varname), GUARD_VARIABLE_FORMAT, res_index);
	acpigen_write_name_integer(varname, 0);
}

void write_enable_method(struct resource_config *res_config, uint8_t res_index)
{
	char method_name[ACPI_NAME_BUFFER_SIZE];
	char varname[ACPI_NAME_BUFFER_SIZE];

	snprintf(varname, sizeof(varname), GUARD_VARIABLE_FORMAT, res_index);

	snprintf(method_name, sizeof(method_name), ENABLE_METHOD_FORMAT, res_index);

	acpigen_write_method_serialized(method_name, 0);
	acpigen_write_if_lequal_namestr_int(varname, 0);
	resource_set_action_type(res_config, ENABLE);
	add_power_operation(res_config);
	acpigen_pop_len(); /* if */

	acpigen_emit_byte(INCREMENT_OP);
	acpigen_emit_namestring(varname);
	acpigen_pop_len(); /* method_name */
}

void write_disable_method(struct resource_config *res_config, uint8_t res_index)
{
	char method_name[ACPI_NAME_BUFFER_SIZE];
	char varname[ACPI_NAME_BUFFER_SIZE];

	snprintf(varname, sizeof(varname), GUARD_VARIABLE_FORMAT, res_index);

	snprintf(method_name, sizeof(method_name), DISABLE_METHOD_FORMAT, res_index);

	acpigen_write_method_serialized(method_name, 0);
	acpigen_write_if();
	acpigen_emit_byte(LGREATER_OP);
	acpigen_emit_namestring(varname);
	acpigen_write_integer(0x0);
	acpigen_emit_byte(DECREMENT_OP);
	acpigen_emit_namestring(varname);
	acpigen_pop_len(); /* if */

	acpigen_write_if_lequal_namestr_int(varname, 0);
	resource_set_action_type(res_config, DISABLE);
	add_power_operation(res_config);
	acpigen_pop_len(); /* if */
	acpigen_pop_len(); /* method_name */
}

void add_guarded_operations(const struct drivers_intel_mipi_camera_config *config,
				   const struct operation_seq *seq)
{
	unsigned int i;
	uint8_t index;
	uint8_t res_id;
	struct resource_config res_config;
	int res_index;

	for (i = 0; i < seq->ops_cnt && i < MAX_PWR_OPS; i++) {
		index = seq->ops[i].index;
		switch (seq->ops[i].type) {
		case IMGCLK:
			res_id = config->clk_panel.clks[index].clknum;
			resource_set_clk_config(&res_config, &config->clk_panel.clks[index]);
			break;
		case GPIO:
			res_id = config->gpio_panel.gpio[index].gpio_num;
			resource_set_gpio_config(&res_config, &config->gpio_panel.gpio[index]);
			break;
		default:
			printk(BIOS_ERR, "Unsupported power operation: %x\n"
					 "OS camera driver will likely not work\n",
					 seq->ops[i].type);
			return;
		}

		res_index = get_resource_index(&res_config);

		if (res_index == -1) {
			if (res_mgr.cnt >= MAX_GUARDED_RESOURCES) {
				printk(BIOS_ERR, "Unable to add guarded camera resource\n"
						 "OS camera driver will likely not work\n");
				return;
			}

			res_mgr.resource[res_mgr.cnt].id = res_id;
			res_mgr.resource[res_mgr.cnt].type = seq->ops[i].type;

			write_guard_variable(res_mgr.cnt);
			write_enable_method(&res_config, res_mgr.cnt);
			write_disable_method(&res_config, res_mgr.cnt);

			res_mgr.cnt++;
		}
	}
}

void fill_power_res_sequence(struct drivers_intel_mipi_camera_config *config,
				    struct operation_seq *seq)
{
	struct resource_config res_config;
	unsigned int i;
	uint8_t index;

	for (i = 0; i < seq->ops_cnt && i < MAX_PWR_OPS; i++) {
		index = seq->ops[i].index;

		switch (seq->ops[i].type) {
		case IMGCLK:
			resource_set_clk_config(&res_config, &config->clk_panel.clks[index]);
			break;
		case GPIO:
			resource_set_gpio_config(&res_config, &config->gpio_panel.gpio[index]);
			break;
		default:
			printk(BIOS_ERR, "Unsupported power operation: %x\n"
					 "OS camera driver will likely not work\n",
					 seq->ops[i].type);
			return;
		}

		resource_set_action_type(&res_config, seq->ops[i].action);
		call_guarded_method(&res_config);
		if (seq->ops[i].delay_ms)
			acpigen_write_sleep(seq->ops[i].delay_ms);
	}
}

void write_pci_camera_device(const struct device *dev)
{
	if (dev->path.type != DEVICE_PATH_PCI) {
		printk(BIOS_ERR, "CIO2/IMGU devices require PCI\n");
		return;
	}

	acpigen_write_device(acpi_device_name(dev));
	acpigen_write_ADR_pci_device(dev);
	acpigen_write_name_string("_DDN", "Camera and Imaging Subsystem");
}

static struct device_operations camera_ops = {
	.read_resources		= noop_read_resources,
	.set_resources		= noop_set_resources,
	.acpi_name		= camera_acpi_name,
	.acpi_fill_ssdt		= camera_fill_ssdt,
};

static void camera_enable(struct device *dev)
{
	//Validate Camera Parameters
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;
	bool params_error = false;

	if (!config->ssdb.lanes_used) {
		printk(BIOS_ERR, "MIPI camera: SSDB lanes_used not set\n");
		params_error = true;
	}

	if (!config->ssdb.platform) {
		printk(BIOS_ERR, "MIPI camera: SSDB platform not set\n");
		params_error = true;
	}

	if (config->ssdb.rom_type && !config->rom_address) {
		printk(BIOS_ERR, "MIPI camera: ROM address not set\n");
		params_error = true;
	}

	if (config->ssdb.vcm_type && !config->vcm_address) {
		printk(BIOS_ERR, "MIPI camera: VCM address not set\n");
		params_error = true;
	}

	if (params_error) {
		printk(BIOS_ERR, "MIPI camera: parameters missing; ACPI device(s) will not be created.\n");
		printk(BIOS_ERR, "MIPI camera: fix devicetree configuration.\n");
		return;
	}

	dev->ops = &camera_ops;
}

struct chip_operations drivers_intel_mipi_camera_ops = {
	.name = "Intel MIPI Camera Device",
	.enable_dev = camera_enable
};
