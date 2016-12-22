/* SPDX-License-Identifier: GPL-2.0-only */

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
