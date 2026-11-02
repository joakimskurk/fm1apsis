#ifndef APSIS_Q31_H
#define APSIS_Q31_H

#include <stdint.h>

#define APSIS_Q31_ONE 0x7fffffffu

static inline __attribute__((always_inline)) uint32_t apsis_mul_q31(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + (1ull << 30)) >> 31);
}
uint32_t apsis_pow_q31(uint32_t base, uint32_t exp);

/* The per-sample multiplier m with m^n = target (an exponential decay or glide over n samples), by 24 bisection
 * steps of apsis_pow_q31. Voices start in the audio interrupt with the same few (target, n) again and again (a
 * drum's decay, a petal's stage), so the result is memoised: the same value, at the price of a lookup.
 * Audio interrupt only (the cache is not shared safely with the main loop). */
uint32_t apsis_nth_root_q31(uint32_t target, uint32_t n);

#endif
