/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DRIVERS_INTEL_MIPI_CAMERA_CAMERA_H
#define DRIVERS_INTEL_MIPI_CAMERA_CAMERA_H

#include <device/device.h>

/* Common definitions */
#define CSI2_DATA_STREAM_INTERFACE_GUID \
	GUID_INIT(0x8A395669, 0x11F7, 0x4EA9, \
	0x9C, 0x7D, 0x20, 0xEE, 0x0A, 0xB5, 0xCA, 0x40)

#define UUID_DSM_SENSOR		"822ace8f-2814-4174-a56b-5f029fe079ee"
#define UUID_DSM_I2C		"26257549-9271-4ca4-bb43-c4899d5a4881"
#define DEFAULT_ENDPOINT	0
#define DEFAULT_REMOTE_NAME	"\\_SB.PCI0.CIO2"
#define CIO2_PCI_DEV		0x14
#define CIO2_PCI_FN		0x3
#define POWER_RESOURCE_NAME	"PRIC"
#define GUARD_VARIABLE_FORMAT	"RES%1d"
#define ENABLE_METHOD_FORMAT	"ENB%1d"
#define DISABLE_METHOD_FORMAT	"DSB%1d"
#define UNKNOWN_METHOD_FORMAT	"UNK%1d"
#define CLK_ENABLE_METHOD	"MCON"
#define CLK_DISABLE_METHOD	"MCOF"

/* Common function declarations */
void apply_pld_defaults(struct drivers_intel_mipi_camera_config *config);
void camera_generate_pld(const struct device *dev);
uint32_t address_for_dev_type(const struct device *dev, uint8_t dev_type);
void camera_generate_dsm_sensor(const struct device *dev);
void camera_generate_dsm_i2c(const struct device *dev);
void camera_generate_dsm_i2c_v2(const struct device *dev);
void camera_generate_dsm_cvf(const struct device *dev);
void camera_fill_ssdb_defaults(struct drivers_intel_mipi_camera_config *config);
int get_resource_index(const struct resource_config *res_config);
void add_guarded_method_namestring(struct resource_config *res_config, int res_index);
void call_guarded_method(struct resource_config *res_config);
void add_clk_op(const struct clk_config *clk_config, enum action_type action);
void add_gpio_op(const struct gpio_config *gpio_config, enum action_type action);
void add_power_operation(const struct resource_config *res_config);
void write_guard_variable(uint8_t res_index);
void write_enable_method(struct resource_config *res_config, uint8_t res_index);
void write_disable_method(struct resource_config *res_config, uint8_t res_index);
void add_guarded_operations(const struct drivers_intel_mipi_camera_config *config,
				   const struct operation_seq *seq);
void fill_power_res_sequence(struct drivers_intel_mipi_camera_config *config,
				     struct operation_seq *seq);
void write_pci_camera_device(const struct device *dev);

#endif /* DRIVERS_INTEL_MIPI_CAMERA_CAMERA_H */
