/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef STUMPY_THERMAL_H
#define STUMPY_THERMAL_H

/* Fan is at default speed */
#define FAN4_PWM		0x90

/* Fan is at LOW speed */
#define FAN3_THRESHOLD_OFF	48
#define FAN3_THRESHOLD_ON	55
#define FAN3_PWM		0xA0

/* Fan is at MEDIUM speed */
#define FAN2_THRESHOLD_OFF	52
#define FAN2_THRESHOLD_ON	64
#define FAN2_PWM		0xB0

/* Fan is at HIGH speed */
#define FAN1_THRESHOLD_OFF	60
#define FAN1_THRESHOLD_ON	68
#define FAN1_PWM		0xC0

/* Fan is at FULL speed */
#define FAN0_THRESHOLD_OFF	66
#define FAN0_THRESHOLD_ON	78
#define FAN0_PWM		0xff

/* Temperature which OS will shutdown at */
#define CRITICAL_TEMPERATURE	100

/* Temperature which OS will throttle CPU */
#define PASSIVE_TEMPERATURE	90

/* Tj_max value for calculating PECI CPU temperature */
#define MAX_TEMPERATURE		100

#endif
