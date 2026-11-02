#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

/* Read-only measurement: include the frozen P11R implementation so the exact
 * sine table/helpers/constants are the authority. No production source edit. */
#include "../apsis/apsis_space_dsp.c"

typedef struct {
    uint64_t frames;
    uint64_t sine_calls;
    uint64_t endpoint_refreshes;
    uint64_t endpoint_reuses;
    uint64_t delta_recomputes_current;
    int32_t min_delta;
    int32_t max_delta;
} ResidualSineCensus;

static void census_window(uint32_t phase[APSIS_SPACE_FDN_LINES],
                          uint32_t frames,
                          ResidualSineCensus *c)
{
    uint32_t f;
    uint8_t k;
    for (f = 0u; f < frames; ++f) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            uint32_t old_phase = phase[k];
            uint8_t i;
            int32_t a, b, delta;
            phase[k] += SPACE_LFO_INC[k];
            i = (uint8_t)(phase[k] >> 24);
            if ((uint8_t)(old_phase >> 24) != i) ++c->endpoint_refreshes;
            else ++c->endpoint_reuses;
            a = space_sine_point(i);
            b = space_sine_point((uint8_t)(i + 1u));
            delta = b - a;
            if (delta < c->min_delta) c->min_delta = delta;
            if (delta > c->max_delta) c->max_delta = delta;
            ++c->sine_calls;
            ++c->delta_recomputes_current;
        }
    }
    c->frames += frames;
}

static void exhaustive_delta_domain(void)
{
    int32_t min_delta = INT32_MAX;
    int32_t max_delta = INT32_MIN;
    uint32_t max_abs = 0u;
    uint32_t positive = 0u, negative = 0u, zero = 0u;
    uint32_t i;
    for (i = 0u; i < 256u; ++i) {
        int32_t a = space_sine_point((uint8_t)i);
        int32_t b = space_sine_point((uint8_t)(i + 1u));
        int32_t d = b - a;
        uint32_t ad = (uint32_t)(d < 0 ? -d : d);
        if (d < min_delta) min_delta = d;
        if (d > max_delta) max_delta = d;
        if (ad > max_abs) max_abs = ad;
        if (d > 0) ++positive;
        else if (d < 0) ++negative;
        else ++zero;
    }
    assert(min_delta >= INT16_MIN && max_delta <= INT16_MAX);
    assert(zero == 0u);
    printf("P11U exhaustive_sine_delta indices=256 min=%" PRId32
           " max=%" PRId32 " max_abs=%" PRIu32
           " positive=%" PRIu32 " negative=%" PRIu32 " zero=%" PRIu32
           " fits_int16=true\n",
           min_delta, max_delta, max_abs, positive, negative, zero);
}

static void print_fixed_workload(void)
{
    /* These are direct structural counts from the frozen render body. They are
     * not cycle estimates. They deliberately retain P11S's Q15-family split. */
    printf("P11U fixed_per_room_frame line_reads=8 line_writes=8 sine_interpolations=8 "
           "sine_delta_subtractions=8 output_sign_projections=16 hadamard_addsub=24 "
           "clamp_calls=8 q15_total=40 q15_structural_safe32=24 q15_structural_wide=16\n");
    printf("P11U q15_per_room_frame alpha=10 output=2 cross=4 had_norm_wide=8 "
           "excitation=8 feedback_wide=8\n");
}

int main(void)
{
    uint32_t phase[APSIS_SPACE_FDN_LINES];
    ResidualSineCensus cold = {0}, warm = {0};
    uint64_t combined_calls, combined_dynamic_refreshes, refreshes_including_reset;
    uint64_t reusable_delta_subtractions;
    uint8_t k;

    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        phase[k] = (uint32_t)k * 0x20000000u;

    cold.min_delta = INT32_MAX;
    cold.max_delta = INT32_MIN;
    census_window(phase, 16384u, &cold);
    assert(cold.sine_calls == 131072u);
    assert(cold.endpoint_refreshes == 88u);
    assert(cold.endpoint_reuses == 130984u);
    printf("P11U cold frames=%" PRIu64 " sine_calls=%" PRIu64
           " endpoint_refreshes=%" PRIu64 " endpoint_reuses=%" PRIu64
           " delta_recomputes=%" PRIu64 " delta=[%" PRId32 ",%" PRId32 "]\n",
           cold.frames, cold.sine_calls, cold.endpoint_refreshes,
           cold.endpoint_reuses, cold.delta_recomputes_current,
           cold.min_delta, cold.max_delta);

    warm.min_delta = INT32_MAX;
    warm.max_delta = INT32_MIN;
    census_window(phase, 65536u, &warm);
    assert(warm.sine_calls == 524288u);
    assert(warm.endpoint_refreshes == 367u);
    assert(warm.endpoint_reuses == 523921u);
    printf("P11U warm frames=%" PRIu64 " sine_calls=%" PRIu64
           " endpoint_refreshes=%" PRIu64 " endpoint_reuses=%" PRIu64
           " delta_recomputes=%" PRIu64 " delta=[%" PRId32 ",%" PRId32 "]\n",
           warm.frames, warm.sine_calls, warm.endpoint_refreshes,
           warm.endpoint_reuses, warm.delta_recomputes_current,
           warm.min_delta, warm.max_delta);

    combined_calls = cold.sine_calls + warm.sine_calls;
    combined_dynamic_refreshes = cold.endpoint_refreshes + warm.endpoint_refreshes;
    /* P11R reset computes two endpoints once per line. A hypothetical A+delta
     * representation would compute eight deltas there, then only on transitions. */
    refreshes_including_reset = combined_dynamic_refreshes + APSIS_SPACE_FDN_LINES;
    reusable_delta_subtractions = combined_calls - combined_dynamic_refreshes;
    assert(combined_calls == 655360u);
    assert(combined_dynamic_refreshes == 455u);
    assert(refreshes_including_reset == 463u);
    assert(reusable_delta_subtractions == 654905u);
    printf("P11U combined sine_calls=%" PRIu64
           " current_delta_subtractions=%" PRIu64
           " dynamic_endpoint_transitions=%" PRIu64
           " delta_evals_if_cached_including_reset=%" PRIu64
           " render_delta_subtractions_avoidable=%" PRIu64 "\n",
           combined_calls, combined_calls, combined_dynamic_refreshes,
           refreshes_including_reset, reusable_delta_subtractions);

    exhaustive_delta_domain();
    print_fixed_workload();
    puts("PASS: P11U residual FDN workload inventory");
    return 0;
}
