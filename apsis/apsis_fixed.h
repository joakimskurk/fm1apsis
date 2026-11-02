#ifndef APSIS_FIXED_H
#define APSIS_FIXED_H

#include <stdint.h>

/* p / 32767 without a divide (the chip's divide takes many cycles; this is one 32x32->64 multiply):
 * 2^47 / 32767 rounded up is 2^32 + 131077, and for p <= 1.1e9 (any Q15 * Q15 product plus rounding)
 * (p * (2^32 + 131077)) >> 47 == p / 32767 exactly (tests/apsis/test_fixed.c). */
static inline uint32_t apsis_div32767(uint32_t p)
{
    return (p + (uint32_t)(((uint64_t)p * 131077u) >> 32)) >> 15;
}

/* floor(x / 32767) for x < 2^46 (the quotient fits 32 bits) without a 64-bit division: q0 = x >> 15 undershoots by (x - 32767 q0) / 32767, and
 * that remainder, (x & 0x7FFF) + q0, fits 32 bits */
static inline uint32_t apsis_div32767_u64(uint64_t x)
{
    uint32_t q0 = (uint32_t)(x >> 15);
    return q0 + ((uint32_t)(x & 0x7FFFu) + q0) / 32767u;
}

/* (a * b) / 32767 rounded half away from zero, for |a|, |b| <= 32767: Q15 * Q15 on the state's 32767 scale */
static inline int32_t apsis_mul_q15_round(int32_t a, int32_t b)
{
    int32_t p = a * b;
    return p >= 0 ? (int32_t)apsis_div32767((uint32_t)p + 16383u)
                  : -(int32_t)apsis_div32767((uint32_t)-p + 16383u);
}

/* clamp to +-32767 (the buses' Q15 range) */
static inline int32_t apsis_sat_q15(int32_t v)
{
    return v > 32767 ? 32767 : v < -32767 ? -32767 : v;
}

#endif
