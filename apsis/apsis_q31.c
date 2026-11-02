#include "apsis_q31.h"

#define Q31_ROOT_CACHE 32u

uint32_t apsis_pow_q31(uint32_t base, uint32_t exp)
{
    uint32_t out = APSIS_Q31_ONE;
    while (exp) {
        if (exp & 1u) out = apsis_mul_q31(out, base);
        exp >>= 1u;
        if (exp) base = apsis_mul_q31(base, base);
    }
    return out;
}

static uint32_t q31_nth_root(uint32_t target, uint32_t n)
{
    uint32_t lo, hi, k;
    if (!n || target >= APSIS_Q31_ONE) return APSIS_Q31_ONE;
    if (!target) return 0u;
    lo = target;
    hi = APSIS_Q31_ONE;
    for (k = 0u; k < 24u; ++k) {
        uint32_t mid = lo + ((hi - lo) >> 1u);
        uint32_t p = apsis_pow_q31(mid, n);
        if (p > target) hi = mid;
        else lo = mid;
    }
    return lo + ((hi - lo) >> 1u);
}

uint32_t apsis_nth_root_q31(uint32_t target, uint32_t n)
{
    static struct { uint32_t target, n, root; uint8_t valid; } cache[Q31_ROOT_CACHE];
    uint32_t i = ((target * 2654435761u) ^ (n * 40503u)) >> 27;   /* 32 slots */
    if (cache[i].valid && cache[i].target == target && cache[i].n == n) return cache[i].root;
    cache[i].target = target;
    cache[i].n = n;
    cache[i].root = q31_nth_root(target, n);
    cache[i].valid = 1u;
    return cache[i].root;
}
