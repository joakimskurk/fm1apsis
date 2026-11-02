#ifndef APSIS_SINE_H
#define APSIS_SINE_H

#include <stdint.h>

#define APSIS_INLINE static inline __attribute__((always_inline))

extern int16_t apsis_sine_table[257];

/* the table value at index i (0..255): what each source's quarter-table sine_point(i) returned */
APSIS_INLINE int16_t apsis_sine_point(uint8_t i)
{
    return apsis_sine_table[i];
}

/* a full cycle over 2^32, linearly interpolated between 256 points: bit for bit the old per-source sine_q15 */
APSIS_INLINE int16_t apsis_sine_q15(uint32_t phase)
{
    uint32_t i = phase >> 24;
    int32_t a = apsis_sine_table[i];
    int32_t b = apsis_sine_table[i + 1u];
    return (int16_t)(a + (((b - a) * (int32_t)(uint8_t)(phase >> 16)) >> 8));
}

#endif
