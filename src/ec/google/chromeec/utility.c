/* SPDX-License-Identifier: GPL-2.0-only */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <timer.h>
#include <console/console.h>
#include "utility.h"

int SafeMemcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *us1 = s1;
	const unsigned char *us2 = s2;
	int result = 0;

	if (0 == n)
		return 0;

	/*
	 * Code snippet without data-dependent branch due to Nate Lawson
	 * (nate@root.org) of Root Labs.
	 */
	while (n--)
		result |= *us1++ ^ *us2++;

	return result != 0;
}

long timer_us(long startTime)
{
	struct mono_time now;
	timer_monotonic_get(&now);
	return (now.microseconds - startTime);
}
