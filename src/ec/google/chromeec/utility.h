/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 The Chromium OS Authors.
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

/**
 * Compare [n] bytes starting at [s1] with [s2] and return 0 if they
 * match, 1 if they don't.  Returns 0 if n=0, since no bytes mismatched.
 *
 * Time taken to perform the comparison is only dependent on [n] and
 * not on the relationship of the match between [s1] and [s2].
 *
 * Note that unlike Memcmp(), this only indicates inequality, not
 * whether s1 is less than or greater than s2.
 */
int SafeMemcmp(const void *s1, const void *s2, size_t n);

long timer_us(long startTime);
