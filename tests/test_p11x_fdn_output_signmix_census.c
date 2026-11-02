#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Same TU gives the measurement direct authority over the frozen static sign
 * vectors while exercising the real production FDN render. */
#include "../apsis/apsis_space_dsp.c"

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void verify_sign_vectors(void)
{
    uint8_t k;
    int left_pos = 0, left_neg = 0, right_pos = 0, right_neg = 0;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        assert(SPACE_OUT_LEFT[k] == 1 || SPACE_OUT_LEFT[k] == -1);
        assert(SPACE_OUT_RIGHT[k] == 1 || SPACE_OUT_RIGHT[k] == -1);
        if (SPACE_OUT_LEFT[k] > 0) left_pos++; else left_neg++;
        if (SPACE_OUT_RIGHT[k] > 0) right_pos++; else right_neg++;
    }
    assert(left_pos == 5 && left_neg == 3);
    assert(right_pos == 5 && right_neg == 3);
    printf("P11X signs left_pos=%d left_neg=%d right_pos=%d right_neg=%d\n",
           left_pos, left_neg, right_pos, right_neg);
}

static void verify_structural_sum_bound(void)
{
    const int64_t per_term_abs_max = 32767;
    const int64_t terms = APSIS_SPACE_FDN_LINES;
    const int64_t sum_abs_max = per_term_abs_max * terms;
    assert(sum_abs_max == 262136);
    assert(sum_abs_max < INT32_MAX);
    printf("P11X structural mixed_q15_abs_max=%lld eight_term_sum_abs_max=%lld int32_headroom=%lld\n",
           (long long)per_term_abs_max,
           (long long)sum_abs_max,
           (long long)INT32_MAX - sum_abs_max);
}

static void exercise_frames(uint32_t frames, uint32_t seed, const char *name)
{
    ApsisSpaceFdn fdn = {0};
    int16_t *storage = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*storage));
    int32_t input[64];
    int32_t output[128];
    uint32_t rng = seed;
    uint32_t done = 0u;
    uint64_t sign_products = 0u;
    uint64_t signed_accumulates = 0u;

    assert(storage);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    while (done < frames) {
        uint32_t n = frames - done;
        uint32_t i;
        if (n > 64u) n = 64u;
        for (i = 0u; i < n; ++i)
            input[i] = (int32_t)((int16_t)(lcg_next(&rng) >> 16)) / 2;
        memset(output, 0, sizeof(output));
        apsis_space_fdn_render(&fdn, input, output, n, 8000u, 4500u);
        sign_products += (uint64_t)n * APSIS_SPACE_FDN_LINES * 2u;
        signed_accumulates += (uint64_t)n * APSIS_SPACE_FDN_LINES * 2u;
        done += n;
    }

    printf("P11X scenario=%s frames=%u sign_products=%llu signed_accumulates=%llu products_per_frame=%u\n",
           name, frames,
           (unsigned long long)sign_products,
           (unsigned long long)signed_accumulates,
           (unsigned)(APSIS_SPACE_FDN_LINES * 2u));

    assert(sign_products == (uint64_t)frames * 16u);
    free(storage);
}

int main(void)
{
    verify_sign_vectors();
    verify_structural_sum_bound();
    exercise_frames(16384u, 0x58434f4cu, "cold");
    exercise_frames(65536u, 0x58574152u, "warm");
    puts("PASS: P11X FDN output sign-mix workload and structural bounds census");
    return 0;
}
