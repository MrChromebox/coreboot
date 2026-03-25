/* SPDX-License-Identifier: GPL-2.0-only */

#include <commonlib/helpers.h>
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <delay.h>
#include <option.h>
#include <stdint.h>
#include <types.h>
#include "oc_mailbox.h"

#define MSR_OC_MAILBOX				0x150

#define OC_MBOX_CMD_VOLTAGE_FREQ_OVR_READ	0x10
#define OC_MBOX_CMD_VOLTAGE_FREQ_OVR_WRITE	0x11
#define OC_MBOX_VOLTAGE_MAX_ADJUSTMENT		125
#define OC_PLANE_MAX				6

union voltage_freq_override_data {
	struct __packed {

		uint32_t max_ratio_limit :  8; /* Bits 7:0: Max OC ratio limit */
		uint32_t voltage_target  : 12; /* Bits 19:8: Voltage target in U12.2.10V format */
		uint32_t target_mode     :  1; /* Bit 20: 0=PCU adaptive, 1=override */
		int32_t voltage_offset   : 11; /* Bits 31:21: Voltage offset in S11.0.10V format */
	};
	uint32_t raw;
};

union oc_mailbox_msr {
	struct __packed {
		uint32_t data;                        /* Bits 31:0  */
		union oc_mailbox_interface interface; /* Bits 63:32 */
	};
	uint64_t raw;
	msr_t msr;
};

enum oc_mailbox_plane {
	OC_PLANE_CORE = 0,
	OC_PLANE_GPU,
	OC_PLANE_CACHE,
	OC_PLANE_SA,
	OC_PLANE_ANALOG_IO,
	OC_PLANE_DIGITAL_IO,
};

static const char *const plane_opt_names[OC_PLANE_MAX] = {
	[OC_PLANE_CORE]		= "oc_undervolt_core",
	[OC_PLANE_GPU]		= "oc_undervolt_gpu",
	[OC_PLANE_CACHE]	= "oc_undervolt_cache",
	[OC_PLANE_SA]		= "oc_undervolt_sa",
	[OC_PLANE_ANALOG_IO]	= "oc_undervolt_analog_io",
	[OC_PLANE_DIGITAL_IO]	= "oc_undervolt_digital_io",
};

static const char *const plane_log_names[OC_PLANE_MAX] = {
	[OC_PLANE_CORE]		= "core",
	[OC_PLANE_GPU]		= "gpu",
	[OC_PLANE_CACHE]	= "cache",
	[OC_PLANE_SA]		= "sa",
	[OC_PLANE_ANALOG_IO]	= "analog_io",
	[OC_PLANE_DIGITAL_IO]	= "digital_io",
};

int oc_mailbox_ready(void)
{
	int wait_count;
	const int delay_step = 10;

	wait_count = 0;
	do {
		const union oc_mailbox_msr oc_mbox = { .msr = rdmsr(MSR_OC_MAILBOX) };
		if (oc_mbox.interface.run_busy == 0)
			return 0;
		wait_count += delay_step;
		udelay(delay_step);
	} while (wait_count < 1000);

	return -1;
}

u32 oc_mailbox_read(const union oc_mailbox_interface command)
{
	if (oc_mailbox_ready() < 0) {
		printk(BIOS_ERR, "OC mailbox: timeout on wait ready\n");
		return 0;
	}

	union oc_mailbox_msr wr = {
		.interface = command,
		.data = 0,
	};
	wr.interface.run_busy = 1;
	wrmsr(MSR_OC_MAILBOX, wr.msr);

	if (oc_mailbox_ready() < 0) {
		printk(BIOS_ERR, "OC mailbox: timeout on completion\n");
		return 0;
	}

	const union oc_mailbox_msr rd = { .msr = rdmsr(MSR_OC_MAILBOX) };
	if (rd.interface.cmd_code != 0) {
		printk(BIOS_ERR, "OC mailbox: command failed with error 0x%02x\n",
			rd.interface.cmd_code);
		return 0;
	}
	return rd.data;
}

int oc_mailbox_write(const union oc_mailbox_interface command, u32 data)
{
	if (oc_mailbox_ready() < 0) {
		printk(BIOS_ERR, "OC mailbox: timeout on wait ready\n");
		return -1;
	}

	union oc_mailbox_msr wr = {
		.interface = command,
		.data = data,
	};
	wr.interface.run_busy = 1;
	wrmsr(MSR_OC_MAILBOX, wr.msr);

	if (oc_mailbox_ready() < 0) {
		printk(BIOS_ERR, "OC mailbox: timeout on completion\n");
		return -1;
	}

	const union oc_mailbox_msr rd = { .msr = rdmsr(MSR_OC_MAILBOX) };
	if (rd.interface.cmd_code != 0) {
		printk(BIOS_ERR, "OC mailbox: command failed with error 0x%02x\n",
			rd.interface.cmd_code);
		return -1;
	}

	return 0;
}

/*
 * Convert a signed voltage tweak in mV into OC_MAILBOX fixed-point units for
 * voltage_offset (S11.0.10V). Negative mV is undervolt, positive is overvolt,
 * matching common tools (intel-undervolt / ThrottleStop).
 *
 * The hardware allows roughly +/-500 mV; we cap the absolute adjustment at
 * 125 mV for policy reasons. The return value is round(|in_mv| * 1024 / 1000)
 * in those units, with the original sign (after clamping magnitude).
 */
static int encode_voltage(const int in_mv)
{
	const int sign = in_mv < 0 ? -1 : 1;
	const int abs_mv = MIN(in_mv * sign, OC_MBOX_VOLTAGE_MAX_ADJUSTMENT);

	return DIV_ROUND_CLOSEST(abs_mv * 1024, 1000) * sign;
}

static bool write_plane(uint8_t plane, unsigned int undervolt_mv)
{
	/* Program voltage offset; leave target/ratio fields at 0. */
	const union voltage_freq_override_data data = {
		.voltage_offset = encode_voltage(-(int)undervolt_mv),
	};
	const union oc_mailbox_interface command_wr = {
		.cmd_code = OC_MBOX_CMD_VOLTAGE_FREQ_OVR_WRITE,
		.param1 = plane,
	};

	if (oc_mailbox_write(command_wr, data.raw) < 0)
		return false;

	const union oc_mailbox_interface command_rd = {
		.cmd_code = OC_MBOX_CMD_VOLTAGE_FREQ_OVR_READ,
		.param1 = plane,
	};
	const uint32_t readback = oc_mailbox_read(command_rd);

	if (readback != data.raw) {
		printk(BIOS_WARNING,
		       "OC mailbox: plane %s verify failed (expected 0x%08x got 0x%08x)\n",
		       plane_log_names[plane], data.raw, readback);
		return false;
	}
	return true;
}

/* HSW/BDW expose 6 planes; SKL-CML expose 5 (no digital_io plane). */
static const size_t num_planes = CONFIG_NUM_OC_MAILBOX_PLANES;

static const char undervolt_apply_opt[] = "oc_undervolt_apply";

static void oc_mailbox_undervolt(void)
{
	unsigned int uv[OC_PLANE_MAX] = {0};
	size_t i;
	bool ok = true;

	if (!get_uint_option(undervolt_apply_opt, 0))
		return;

	for (i = 0; i < num_planes; i++)
		uv[i] = get_uint_option(plane_opt_names[i], 0);

	printk(BIOS_INFO, "OC mailbox: applying voltage offsets (mV):");
	for (i = 0; i < num_planes; i++) {
		printk(BIOS_INFO, " %s=%u",
		       plane_log_names[i],
		       uv[i]);
	}
	printk(BIOS_INFO, "\n");

	/* Program all planes */
	for (i = 0; i < num_planes; i++) {
		if (!write_plane((uint8_t)i, uv[i]))
			ok = false;
	}

	if (ok)
		printk(BIOS_INFO, "OC mailbox: all planes programmed\n");
}

void program_oc_mailbox(void)
{
	if (CONFIG(INTEL_OC_MAILBOX_ENABLE_UNDERVOLTING))
		oc_mailbox_undervolt();
}
