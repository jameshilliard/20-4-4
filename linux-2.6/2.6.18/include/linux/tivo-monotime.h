#ifndef _LINUX_TIVO_MONOTIME_H
#define _LINUX_TIVO_MONOTIME_H

/*
 * Monotonic time for TiVo systems
 *
 * Copyright (C) 2002 TiVo Inc. All Rights Reserved.
 */

#include <linux/types.h>

typedef struct { u64 count; } monotonic_t;

/**
 * Read the current monotonic time, this is expected to be a full-range
 * 64-bit value (or at least not ever wrap less than 64 bits during the
 * practical system run time)
 */
extern void tivo_monotonic_get_current(monotonic_t *t);

/**
 * Number of monotonic counts per 256 microseconds (because per microsecond
 * does not generally give adequate resolution)
 */
extern unsigned long tivo_counts_per_256usec;

/**
 * Offset a monotonic time by a time value (in microseconds)
 */
static inline void tivo_monotonic_add_usecs(monotonic_t *t, s64 usecs) {
    t->count += (u64)((usecs * tivo_counts_per_256usec) >> 8);
}

static inline void tivo_monotonic_store(monotonic_t *t, u64 count) {
    t->count = count;
}

static inline u64 tivo_monotonic_retrieve(monotonic_t *t) {
    return t->count;
}

/**
 * t1 greater than t2, handles (a single) 64-bit count wrap
 */
static inline int tivo_monotonic_gt(monotonic_t *t1, monotonic_t *t2) {
    return (s64)(t1->count - t2->count) > 0;
}

/**
 * t1 less than t2, handles (a single) 64-bit count wrap
 */
static inline int tivo_monotonic_lt(monotonic_t *t1, monotonic_t *t2) {
    return (s64)(t1->count - t2->count) < 0;
}

/**
 * Test for 0 time, which may have special value. WARNING: 0 may also be a
 * perfectly valid time count, although the chances of hitting it exactly
 * are probably staggeringly small...
 */
static inline int tivo_monotonic_is_null(monotonic_t *t) {
    return t->count == 0;
}

#endif /* _LINUX_TIVO_MONOTIME_H */
