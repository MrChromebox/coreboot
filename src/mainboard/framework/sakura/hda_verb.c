/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/azalia_device.h>
#include <device/mmio.h>
#include <timer.h>
#include <types.h>

static const u32 realtek_alc285_verbs[] = {
	/*
	 * Verb table reproduced 1:1 from the vendor reference table.
	 * The internal speaker is driven via NID 0x17 (I2S OUT) into an
	 * external ALC1320 Class-D amplifier
	 */
	AZALIA_SUBVENDOR(0, 0xf111000f),
	AZALIA_RESET(1),

	/* Pin widget configuration (matches vendor reference table) */
	/* DMIC - Internal Mic */
	AZALIA_PIN_CFG(0, 0x12, AZALIA_PIN_DESC(
		AZALIA_INTEGRATED,
		AZALIA_INTERNAL,
		AZALIA_MIC_IN,
		AZALIA_OTHER_DIGITAL,
		AZALIA_COLOR_UNKNOWN,
		AZALIA_NO_JACK_PRESENCE_DETECT,
		3, 0
	)),
	AZALIA_PIN_CFG(0, 0x13, 0x40000000), /* DMIC - NC */
	AZALIA_PIN_CFG(0, 0x14, AZALIA_PIN_CFG_NC(0)), /* Front (Port-D) - NC */
	AZALIA_PIN_CFG(0, 0x16, AZALIA_PIN_CFG_NC(0)), /* NPC */
	/* I2S OUT - Internal Speaker (ALC1320) */
	AZALIA_PIN_CFG(0, 0x17, AZALIA_PIN_DESC(
		AZALIA_INTEGRATED,
		AZALIA_INTERNAL,
		AZALIA_SPEAKER,
		AZALIA_OTHER_ANALOG,
		AZALIA_COLOR_UNKNOWN,
		AZALIA_NO_JACK_PRESENCE_DETECT,
		1, 0
	)),
	AZALIA_PIN_CFG(0, 0x18, AZALIA_PIN_CFG_NC(0)), /* I2S IN */
	AZALIA_PIN_CFG(0, 0x19, AZALIA_PIN_CFG_NC(0)), /* MIC2 (Port-F) */
	AZALIA_PIN_CFG(0, 0x1a, AZALIA_PIN_CFG_NC(0)), /* NPC */
	AZALIA_PIN_CFG(0, 0x1b, AZALIA_PIN_CFG_NC(0)), /* LINE2 (Port-E) */
	AZALIA_PIN_CFG(0, 0x1d, 0x40651b05), /* BEEP-IN */
	AZALIA_PIN_CFG(0, 0x1e, AZALIA_PIN_CFG_NC(0)), /* S/PDIF-OUT */
	/* HP1-OUT (Port-I) - Headphone Jack */
	AZALIA_PIN_CFG(0, 0x21, AZALIA_PIN_DESC(
		AZALIA_JACK,
		AZALIA_EXTERNAL_PRIMARY_CHASSIS | AZALIA_RIGHT,
		AZALIA_HP_OUT,
		AZALIA_STEREO_MONO_1_8,
		AZALIA_BLACK,
		AZALIA_JACK_PRESENCE_DETECT,
		2, 0
	)),

	/* Reset power state to D0 */
	0x00170500, 0x00170500, 0x00170500, 0x00170500,

	/* Reset all registers back to default */
	0x0205001a, 0x02048003, 0x0205001a, 0x0204c003,

	/* ClassD reset */
	0x0205003c, 0x0204f2d4, 0x0205003c, 0x0204f214,

	/* JD1 - 2port JD mode */
	0x02050009, 0x0204e003, 0x0205000a, 0x02047770,

	/* Set TRS type-1 */
	0x02050045, 0x0204c689, 0x02050049, 0x02040049,

	/* Set TRS type-2 (Turn off MIC2_Vrefo_R_L) + Set UAJ Line2 vref (ALC285 only) */
	0x0205004a, 0x0204a830, 0x02050063, 0x02040f00,

	/* NID 0x20 set class-D to 2W@4ohm (+12dB gain) + Set sine tone gain(0x34) */
	0x02050038, 0x02047909, 0x05c50000, 0x05c43482,

	/* HP-JD Enable+ For Nokia type (+power down JD2)  ( item 23) */
	0x0205004a, 0x02042010, 0x02050008, 0x02046a2c,

	/* EAPD set to verb-control */
	0x02050010, 0x02040020, 0x02050010, 0x02040020,

	/* Class D silent detection enable -84dB threshold */
	0x02050030, 0x02049000, 0x02050037, 0x0204fe15,

	/* > Reserve EAPD and Set GPIO2 as GPIO */
	0x02050040, 0x02048800, 0x05a50001, 0x05a4001f,

	/* ALC3254 pin1 I2S/OE pull down to Enable NID17 */
	0x02050035, 0x02048d6a, 0x02050035, 0x02048d6a,

	/* I2S output buffer enable */
	0x00170500, 0x01770500, 0x01770740, 0x01770740,
};

/* ALC1320 Class-D smart-amp firmware blob, programmed by alc1320_init() */
static const u32 alc1320_verbs[] = {
#include "alc1320_smartamp.inc"
};

static const u32 intel_ptl_hdmi_verbs[] = {
	AZALIA_SUBVENDOR(2, 0x80860101),
	AZALIA_PIN_CFG(2, 0x04, 0x18560010),
	AZALIA_PIN_CFG(2, 0x06, 0x18560010),
	AZALIA_PIN_CFG(2, 0x08, 0x18560010),
	AZALIA_PIN_CFG(2, 0x0a, 0x18560010),
	AZALIA_PIN_CFG(2, 0x0b, 0x18560010),
	AZALIA_PIN_CFG(2, 0x0c, 0x18560010),
	AZALIA_PIN_CFG(2, 0x0d, 0x18560010),
	AZALIA_PIN_CFG(2, 0x0e, 0x18560010),
	AZALIA_PIN_CFG(2, 0x0f, 0x18560010),
};

const u32 pc_beep_verbs[] = {};

struct azalia_codec mainboard_azalia_codecs[] = {
	{
		.name         = "Realtek ALC285",
		.vendor_id    = 0x10ec0285,
		.subsystem_id = 0xf111000f,
		.address      = 0,
		.verbs        = realtek_alc285_verbs,
		.verb_count   = ARRAY_SIZE(realtek_alc285_verbs),
	},
	{
		.name         = "Intel Panther Lake HDMI",
		.vendor_id    = 0x80862822,
		.subsystem_id = 0x80860101,
		.address      = 2,
		.verbs        = intel_ptl_hdmi_verbs,
		.verb_count   = ARRAY_SIZE(intel_ptl_hdmi_verbs),
	},
};

AZALIA_ARRAY_SIZES;

/* ALC285 vendor coefficients on NID 0x20 */
#define ALC285_SET_COEF_INDEX	0x02050000
#define ALC285_SET_COEF		0x02040000
#define ALC285_GET_COEF		0x020c0000

/*
 * The ALC285 has an I2C bridge to the ALC1320. Coefficient 0x24 holds the I2C
 * address, 0x25-0x26 the 32-bit register address and 0x27-0x28 up to four
 * data bytes, right-aligned with the first byte most significant. Writing a
 * command with bit 5 set to coefficient 0x29 starts the transfer. Bits 3:2
 * of the command hold the data length - 1 and bits 1:0 the register address
 * length - 1. Bit 14 is set while the bridge is busy. The amp
 * auto-increments the register address for each data byte.
 */
#define ALC1320_I2C_ADDR	0x24
#define ALC1320_REG_HI		0x25
#define ALC1320_REG_LO		0x26
#define ALC1320_DATA_HI		0x27
#define ALC1320_DATA_LO		0x28
#define ALC1320_BRIDGE_CTRL	0x29
#define ALC1320_BRIDGE_BUSY	BIT(14)
#define ALC1320_BRIDGE_WRITE(len)	(0xb023 | ((len) - 1) << 2)
#define ALC1320_BRIDGE_US	500
#define ALC1320_BRIDGE_TIMEOUT_MS	10
#define ALC1320_COEF_COUNT	0x30
#define ALC1320_MAX_LEN		4

struct alc1320_bridge {
	u8 *base;
	u16 coefs[ALC1320_COEF_COUNT];
	bool known[ALC1320_COEF_COUNT];
	struct stopwatch transfer;
	bool transfer_started;
};

struct alc1320_xfer {
	u16 i2c_addr;
	u32 reg;
	u32 data;
	size_t len;
};

static int alc285_verb(u8 *base, u32 verb)
{
	return azalia_program_verb_table(base, &verb, 1);
}

static int alc285_write_coef(u8 *base, u8 index, u16 value)
{
	if (alc285_verb(base, ALC285_SET_COEF_INDEX | index) < 0 ||
	    alc285_verb(base, ALC285_SET_COEF | value) < 0)
		return -1;
	return 0;
}

static int alc1320_wait_bridge_idle(u8 *base)
{
	struct stopwatch sw;

	stopwatch_init_msecs_expire(&sw, ALC1320_BRIDGE_TIMEOUT_MS);
	do {
		if (alc285_verb(base, ALC285_SET_COEF_INDEX | ALC1320_BRIDGE_CTRL) < 0 ||
		    alc285_verb(base, ALC285_GET_COEF) < 0)
			return -1;
		if (!(read32(base + HDA_IR_REG) & ALC1320_BRIDGE_BUSY))
			return 0;
	} while (!stopwatch_expired(&sw));

	return -1;
}

/* Skips writes that don't change the coefficient's current value */
static int alc1320_set_coef(struct alc1320_bridge *bridge, u8 index, u16 value)
{
	if (bridge->known[index] && bridge->coefs[index] == value)
		return 0;
	if (alc285_write_coef(bridge->base, index, value) < 0)
		return -1;
	bridge->coefs[index] = value;
	bridge->known[index] = true;
	return 0;
}

static int alc1320_flush(struct alc1320_bridge *bridge, const struct alc1320_xfer *w)
{
	if (alc1320_set_coef(bridge, ALC1320_I2C_ADDR, w->i2c_addr) < 0 ||
	    alc1320_set_coef(bridge, ALC1320_REG_HI, w->reg >> 16) < 0 ||
	    alc1320_set_coef(bridge, ALC1320_REG_LO, w->reg & 0xffff) < 0 ||
	    (w->len > 2 && alc1320_set_coef(bridge, ALC1320_DATA_HI, w->data >> 16) < 0) ||
	    alc1320_set_coef(bridge, ALC1320_DATA_LO, w->data & 0xffff) < 0)
		return -1;

	/* The coefficients are only latched when a transfer starts */
	if (bridge->transfer_started && !stopwatch_expired(&bridge->transfer) &&
	    alc1320_wait_bridge_idle(bridge->base) < 0)
		return -1;

	if (alc285_write_coef(bridge->base, ALC1320_BRIDGE_CTRL,
			      ALC1320_BRIDGE_WRITE(w->len)) < 0)
		return -1;
	stopwatch_init_usecs_expire(&bridge->transfer, ALC1320_BRIDGE_US);
	bridge->transfer_started = true;
	return 0;
}

/*
 * The vendor sequence writes one byte per transfer and rewrites every
 * coefficient each time. Merge writes to consecutive registers into transfers
 * of up to four bytes instead.
 */
static void alc1320_init(u8 *base)
{
	struct alc1320_bridge bridge = { .base = base };
	struct alc1320_xfer w = { 0 };
	u16 coefs[ALC1320_COEF_COUNT] = { 0 };

	for (size_t i = 0; i + 1 < ARRAY_SIZE(alc1320_verbs); i += 2) {
		const u8 index = alc1320_verbs[i] & 0xff;
		u32 reg;

		if (index >= ALC1320_COEF_COUNT)
			continue;
		coefs[index] = alc1320_verbs[i + 1] & 0xffff;
		if (index != ALC1320_BRIDGE_CTRL)
			continue;

		reg = (u32)coefs[ALC1320_REG_HI] << 16 | coefs[ALC1320_REG_LO];
		if (w.len && w.len < ALC1320_MAX_LEN &&
		    w.i2c_addr == coefs[ALC1320_I2C_ADDR] && w.reg + w.len == reg) {
			w.data = w.data << 8 | (coefs[ALC1320_DATA_LO] & 0xff);
			w.len++;
			continue;
		}

		if (w.len && alc1320_flush(&bridge, &w) < 0)
			goto fail;
		w.i2c_addr = coefs[ALC1320_I2C_ADDR];
		w.reg = reg;
		w.data = coefs[ALC1320_DATA_LO] & 0xff;
		w.len = 1;
	}

	if (w.len && alc1320_flush(&bridge, &w) < 0)
		goto fail;
	if (alc1320_wait_bridge_idle(base) < 0)
		goto fail;
	return;

fail:
	printk(BIOS_ERR, "ALC1320: smart amp initialization failed\n");
}

void mainboard_azalia_program_runtime_verbs(u8 *base, u32 viddid)
{
	if (viddid == 0x10ec0285)
		alc1320_init(base);
}
