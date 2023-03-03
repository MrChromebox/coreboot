/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef THERMAL_H
#define THERMAL_H

/* Fan is at default speed: ~2600 rpm */
#define FAN4_PWM		0x4b

/* Fan is at LOW speed: ~3000 rpm */
#define FAN3_THRESHOLD_OFF	30
#define FAN3_THRESHOLD_ON	40
#define FAN3_PWM		0x5B

/* Fan is at MEDIUM speed: ~4000 rpm */
#define FAN2_THRESHOLD_OFF	40
#define FAN2_THRESHOLD_ON	50
#define FAN2_PWM		0x87

/* Fan is at HIGH speed: ~5000 rpm */
#define FAN1_THRESHOLD_OFF	50
#define FAN1_THRESHOLD_ON	60
#define FAN1_PWM		0xc3

/* Fan is at FULL speed: ~6000 rpm */
#define FAN0_THRESHOLD_OFF	60
#define FAN0_THRESHOLD_ON	70
#define FAN0_PWM		0xff

/* Temperature which OS will shutdown at */
#define CRITICAL_TEMPERATURE	98

/* Temperature which OS will throttle CPU */
#define PASSIVE_TEMPERATURE	95

/* Tj_max value for calculating PECI CPU temperature */
#define MAX_TEMPERATURE		100

#endif
