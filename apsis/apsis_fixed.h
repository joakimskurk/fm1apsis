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

/* (a * b) / 32767 rounded half away from zero, for |a|, |b| <= 32767: Q15 * Q15 on the state's 32767 scale */
static inline int32_t apsis_mul_q15_round(int32_t a, int32_t b)
{
    int32_t p = a * b;
    return p >= 0 ? (int32_t)apsis_div32767((uint32_t)p + 16383u)
                  : -(int32_t)apsis_div32767((uint32_t)-p + 16383u);
}

#endif
