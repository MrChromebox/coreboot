/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef THERMAL_H
#define THERMAL_H

/* Fan is at default speed */
#define FAN4_PWM		0x47

/* Fan is at LOW speed */
#define FAN3_THRESHOLD_OFF	50
#define FAN3_THRESHOLD_ON	60
#define FAN3_PWM		0x62

/* Fan is at MEDIUM speed */
#define FAN2_THRESHOLD_OFF	65
#define FAN2_THRESHOLD_ON	77
#define FAN2_PWM		0x86

/* Fan is at HIGH speed */
#define FAN1_THRESHOLD_OFF	77
#define FAN1_THRESHOLD_ON	85
#define FAN1_PWM		0xa8

/* Fan is at FULL speed */
#define FAN0_THRESHOLD_OFF	85
#define FAN0_THRESHOLD_ON	90
#define FAN0_PWM		0xdc

/* Temperature which OS will shutdown at */
#define CRITICAL_TEMPERATURE	98

/* Temperature which OS will throttle CPU */
#define PASSIVE_TEMPERATURE	95

/* Tj_max value for calculating PECI CPU temperature */
#define MAX_TEMPERATURE		100

#endif
