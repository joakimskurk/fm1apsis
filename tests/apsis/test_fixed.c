/* apsis_fixed.h: the divide-free / 32767 against the real division, at every multiple of 32767 and its
 * neighbours up to the largest Q15 product, and on a dense sweep. */
#include <assert.h>
#include <stdio.h>
#include "apsis_fixed.h"

int main(void)
{
    uint32_t k, p;
    int32_t a, b;
    for (k = 0u; k <= 32800u; ++k) {
        uint32_t base = k * 32767u;
        for (p = base > 2u ? base - 2u : 0u; p <= base + 2u; ++p) assert(apsis_div32767(p) == p / 32767u);
        assert(apsis_div32767(base + 32766u) == (base + 32766u) / 32767u);
    }
    for (p = 0u; p < 1100000000u; p += 997u) assert(apsis_div32767(p) == p / 32767u);
    for (a = -32767; a <= 32767; a += 13)
        for (b = 0; b <= 32767; b += 509) {
            int32_t q = a * b;
            q = q >= 0 ? (q + 16383) / 32767 : (q - 16383) / 32767;
            assert(apsis_mul_q15_round(a, b) == q);
        }
    puts("PASS: divide-free Q15 scaling is exact");
    return 0;
}
