/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi_gnvs.h>
#include <device/device.h>
#include <soc/nvs.h>
#include <southbridge/intel/lynxpoint/pch.h>
#include <variant/thermal.h>

void mainboard_fill_gnvs(struct global_nvs *gnvs)
{
	/* TPM Present */
	gnvs->tpmp = 1;

	gnvs->f4pw = FAN4_PWM;

	gnvs->f3of = FAN3_THRESHOLD_OFF;
	gnvs->f3on = FAN3_THRESHOLD_ON;
	gnvs->f3pw = FAN3_PWM;

	gnvs->f2of = FAN2_THRESHOLD_OFF;
	gnvs->f2on = FAN2_THRESHOLD_ON;
	gnvs->f2pw = FAN2_PWM;

	gnvs->f1of = FAN1_THRESHOLD_OFF;
	gnvs->f1on = FAN1_THRESHOLD_ON;
	gnvs->f1pw = FAN1_PWM;

	gnvs->f0of = FAN0_THRESHOLD_OFF;
	gnvs->f0on = FAN0_THRESHOLD_ON;
	gnvs->f0pw = FAN0_PWM;

	gnvs->tcrt = CRITICAL_TEMPERATURE;
	gnvs->tpsv = PASSIVE_TEMPERATURE;
	gnvs->tmax = MAX_TEMPERATURE;
	gnvs->flvl = 5;
}
