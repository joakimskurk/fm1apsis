/* M4e target compatibility for JieLi pi32v2.
 *
 * The Clock DSP intentionally uses exact integer division while deriving
 * fixed-point filter/envelope coefficients. The AC79 target linker does not
 * provide libgcc's DI-mode division helpers, although the compiler emits calls
 * to them for 64-bit division. Keep the source arithmetic unchanged and supply
 * the standard helper ABI using only shifts, compares and subtraction.
 *
 * This file is included only by the generated Dinghy overlay; native host core
 * tests continue to exercise the normal compiler arithmetic. No audio state,
 * RNG state, scheduler state or timing is touched here.
 */
#include <stdint.h>

static uint64_t apsis_target_udiv64(uint64_t numerator, uint64_t denominator)
{
    uint64_t quotient = 0u;
    uint64_t bit = 1u;

    if (!denominator) return ~(uint64_t)0;
    if (numerator < denominator) return 0u;

    /* Align denominator with the highest useful quotient bit without ever
     * shifting it through bit 63 (which would overflow on the next shift). */
    while (denominator < numerator && !(denominator & (1ull << 63))) {
        denominator <<= 1u;
        bit <<= 1u;
    }

    while (bit) {
        if (numerator >= denominator) {
            numerator -= denominator;
            quotient |= bit;
        }
        denominator >>= 1u;
        bit >>= 1u;
    }
    return quotient;
}

uint64_t __udivdi3(uint64_t numerator, uint64_t denominator)
{
    return apsis_target_udiv64(numerator, denominator);
}

int64_t __divdi3(int64_t numerator, int64_t denominator)
{
    uint64_t un = (uint64_t)numerator;
    uint64_t ud = (uint64_t)denominator;
    uint64_t q;
    uint8_t negative;

    if (!denominator) return numerator < 0 ? (int64_t)0x8000000000000000ull
                                           : (int64_t)0x7fffffffffffffffull;

    negative = (uint8_t)((numerator < 0) != (denominator < 0));
    if (numerator < 0) un = ~un + 1u;
    if (denominator < 0) ud = ~ud + 1u;
    q = apsis_target_udiv64(un, ud);
    if (negative) q = ~q + 1u;
    return (int64_t)q;
}
