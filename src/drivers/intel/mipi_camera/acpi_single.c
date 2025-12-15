/* SPDX-License-Identifier: GPL-2.0-only */

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

/*
 * This implementation assumes that any VCM or NVM for a CAM is on the same I2C bus as the CAM.
 */

 /*
 * Generate ASL DSM code for I2C device count and addresses (V2)
 *
 * Generated ASL:
 * If (LEqual (Local0, ToUUID ("5815c5c8-c47d-477b-9a8d-76173176414b"))) {
 *     If (LEqual (Arg2, Zero)) {
 *         If (LEqual (Arg1, Zero)) {
 *             Return (Buffer (One) { 0x3 })
 *         } Else {
 *             Return (Buffer (One) { 0x1 })
 *         }
 *     }
 *     If (LEqual (Arg2, One)) {
 *         Return (Buffer (52) {
 *             i2c_count, sensor_addr, [vcm_addr], [rom_addr], 0, 0, ...
 *         })
 *         // Buffer is 13 * 4 = 52 bytes: count + up to 12 device addresses
 *     }
 * }
 */
static void camera_generate_dsm_i2c_v2(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;
	if (!config)
		return;

	int i2c_count = 1 + (config->ssdb.vcm_type ? 1 : 0) + (config->ssdb.rom_type ? 1 : 0);
	int i2c_idx = 1;

	/* If (LEqual (Local0, ToUUID(UUID_DSM_I2C_V2))) */
	acpigen_write_if();
	acpigen_emit_byte(LEQUAL_OP);
	acpigen_emit_byte(LOCAL0_OP);
	acpigen_write_uuid(UUID_DSM_I2C_V2);

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

	/* Buffer is 13 * 4 = 52 bytes: count + up to 12 device addresses */
	uint32_t i2c_buffer[13] = {0};
	_Static_assert(sizeof(i2c_buffer) == 52, "i2c_buffer size must be 52 bytes");

	i2c_buffer[0] = i2c_count;
	i2c_buffer[i2c_idx++] = address_for_dev_type(dev, DEV_TYPE_SENSOR);

	if (config->ssdb.vcm_type) {
		i2c_buffer[i2c_idx++] = address_for_dev_type(dev, DEV_TYPE_VCM);
	}

	if (config->ssdb.rom_type) {
		i2c_buffer[i2c_idx] = address_for_dev_type(dev, DEV_TYPE_ROM);
	}

	acpigen_write_return_byte_buffer((uint8_t *)i2c_buffer, sizeof(i2c_buffer));

	acpigen_pop_len();	/* If Arg2=1 */

	acpigen_pop_len();	/* If uuid */
}

/*
 * Generate ASL DSM code for Computer Vision Framework (CVF)
 *
 * Generated ASL:
 * If (LEqual (Local0, ToUUID ("02f55f0c-2e63-4f05-84f3-bf1980f9af79"))) {
 *     If (LEqual (Arg2, Zero)) {
 *         Return (Buffer (One) { 0x3 })
 *     }
 *     If (LEqual (Arg2, One)) {
 *         Return (Zero)
 *     }
 * }
 */
static void camera_generate_dsm_cvf(const struct device *dev)
{
	/* If (LEqual (Local0, ToUUID (UUID_DSM_CVF))) */
	acpigen_write_if();
	acpigen_emit_byte(LEQUAL_OP);
	acpigen_emit_byte(LOCAL0_OP);
	acpigen_write_uuid(UUID_DSM_CVF);

	/* If (LEqual (Arg2, Zero)) */
	acpigen_write_if_lequal_op_int(ARG2_OP, 0);
	acpigen_write_return_singleton_buffer(0x3);
	acpigen_pop_len();

	/* If (LEqual (Arg2, One)) */
	acpigen_write_if_lequal_op_int(ARG2_OP, 1);
	acpigen_write_return_integer(0);
	acpigen_pop_len();

	acpigen_pop_len();	/* If uuid */
}

static void camera_generate_dsm(const struct device *dev)
{
	/* Method (_DSM, 4, NotSerialized) */
	acpigen_write_method("_DSM", 4);

	/* ToBuffer (Arg0, Local0) */
	acpigen_write_to_buffer(ARG0_OP, LOCAL0_OP);

	camera_generate_dsm_sensor(dev);
	camera_generate_dsm_i2c(dev);
	camera_generate_dsm_i2c_v2(dev);
	camera_generate_dsm_cvf(dev);

	/* Return (Buffer (One) { 0x0 }) */
	acpigen_write_return_singleton_buffer(0x0);

	acpigen_pop_len();      /* Method _DSM */
}

/*
 * Adds settings for a camera sensor device (typically at "\_SB.PCI0.I2Cx.CAMy"). The drivers for
 * Windows and Linux want all of these to be grouped together in the camera sensor ACPI device.
 * The OS driver can use the "_DSM" method to disambiguate the I2C resources in the camera sensor
 * ACPI device.  Drivers typically query "SSDB" for configuration information (represented as a
 * binary blob dump of struct).
 */
static void camera_fill_sensor(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;

	camera_generate_pld(dev);

	camera_fill_ssdb_defaults(config);

	/* _DSM */
	camera_generate_dsm(dev);

	acpigen_write_method_serialized("SSDB", 0);
	acpigen_write_return_byte_buffer((uint8_t *)&config->ssdb, sizeof(config->ssdb));
	acpigen_pop_len(); /* Method */
}


static void write_i2c_camera_device(const struct device *dev, const char *scope)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;
	struct acpi_i2c i2c = {
		.address = dev->path.i2c.device,
		.mode_10bit = dev->path.i2c.mode_10bit,
		.speed = I2C_SPEED_FAST,
		.resource = scope,
	};

	acpigen_write_device(acpi_device_name(dev));

	/* add power resource */
	if (config->has_power_resource) {
		acpigen_write_power_res(POWER_RESOURCE_NAME, 0, 0, NULL, 0);
		acpigen_write_name_integer("STA", 0);
		acpigen_write_STA_ext("STA");

		acpigen_write_method_serialized("_ON", 0);
		acpigen_write_if();
		acpigen_emit_byte(LEQUAL_OP);
		acpigen_emit_namestring("STA");
		acpigen_write_integer(0);

		fill_power_res_sequence(config, &config->on_seq);

		acpigen_write_store_op_to_namestr(1, "STA");
		acpigen_pop_len(); /* if */
		acpigen_pop_len(); /* _ON */

		/* _OFF operations */
		acpigen_write_method_serialized("_OFF", 0);
		acpigen_write_if();
		acpigen_emit_byte(LEQUAL_OP);
		acpigen_emit_namestring("STA");
		acpigen_write_integer(1);

		fill_power_res_sequence(config, &config->off_seq);

		acpigen_write_store_op_to_namestr(0, "STA");
		acpigen_pop_len(); /* if */
		acpigen_pop_len(); /* _ON */

		acpigen_pop_len(); /* Power Resource */
	}

	acpigen_write_name_string("_HID", config->acpi_hid);
	acpigen_write_name_integer("_UID", config->acpi_uid);
	acpigen_write_name_string("_DDN", config->sensor_name);
	acpigen_write_STA(acpi_device_status(dev));
	acpigen_write_method("_DSC", 0);
	acpigen_write_return_integer(config->max_dstate_for_probe);
	acpigen_pop_len(); /* Method _DSC */

	/* Resources */
	acpigen_write_name("_CRS");
	acpigen_write_resourcetemplate_header();
	acpi_device_write_i2c(&i2c);

	/*
	 * The optional vcm/nvram devices are presumed to be on the same I2C bus as the camera
	 * sensor.
	 */
	if (config->ssdb.vcm_type && config->vcm_address) {
		struct acpi_i2c i2c_vcm = i2c;
		i2c_vcm.address = config->vcm_address;
		acpi_device_write_i2c(&i2c_vcm);
	}

	if (config->ssdb.rom_type && config->rom_address) {
		struct acpi_i2c i2c_rom = i2c;
		i2c_rom.address = config->rom_address;
		acpi_device_write_i2c(&i2c_rom);
	}

	acpigen_write_resourcetemplate_footer();
}

static void write_camera_device_common(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;

	if (config->pr0 || config->has_power_resource) {
		acpigen_write_name("_PR0");
		acpigen_write_package(1);
		if (config->pr0)
			acpigen_emit_namestring(config->pr0); /* External power resource */
		else
			acpigen_emit_namestring(POWER_RESOURCE_NAME);

		acpigen_pop_len(); /* _PR0 */
	}

	camera_fill_sensor(dev);
}

static void camera_fill_ssdt(const struct device *dev)
{
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;
	const char *scope = NULL;
	const struct device *pdev = dev->upstream->dev;

	/* Only generate SSDT for an i2c-attached sensor device */
	if (dev->path.type != DEVICE_PATH_I2C || config->device_type != INTEL_ACPI_CAMERA_SENSOR)
		return;

	scope = acpi_device_scope(dev);
	if (!scope) {
		printk(BIOS_ERR, "Failed to get scope for device %s\n", dev_path(dev));
		return;
	}

	acpigen_write_scope(scope);

	if (config->has_power_resource && pdev && pdev->enabled) {
		add_guarded_operations(config, &config->on_seq);
		add_guarded_operations(config, &config->off_seq);
	}

	write_i2c_camera_device(dev, scope);
	write_camera_device_common(dev);

	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */

	printk(BIOS_INFO, "%s: %s at I2C 0x%02x\n", acpi_device_path(dev),
	       dev->chip_ops->name, dev->path.i2c.device);

}

static const char *camera_acpi_name(const struct device *dev)
{
	static char name[ACPI_NAME_BUFFER_SIZE];
	struct drivers_intel_mipi_camera_config *config = dev->chip_info;

	if (config->acpi_name)
		return config->acpi_name;

	snprintf(name, sizeof(name), "CAM%1u", config->acpi_uid);
	return name;
}
