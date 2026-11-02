#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Measurement-only translation unit. Production source is included unchanged
 * so the frozen static sine/LFO helpers are the measurement authority. */
#include "../apsis/apsis_space_dsp.c"

typedef struct {
    uint64_t calls;
    uint64_t point_lookups;
    uint64_t frac_zero;
    uint64_t frac_nonzero;
    uint64_t boundary_crossings;
    uint64_t endpoint_refreshes;
    uint64_t endpoint_reuses;
    uint64_t equal_endpoint_pairs;
    uint64_t quadrant_a[4];
    uint64_t quadrant_b[4];
    uint8_t seen_index[256];
    uint8_t seen_frac[256];
    uint32_t distinct_index;
    uint32_t distinct_frac;
    uint64_t line_refreshes[APSIS_SPACE_FDN_LINES];
} P11pStats;

typedef struct {
    uint8_t valid[APSIS_SPACE_FDN_LINES];
    uint8_t index[APSIS_SPACE_FDN_LINES];
} P11pEndpointCacheModel;

static void mark_seen(uint8_t seen[256], uint32_t *count, uint8_t v)
{
    if (!seen[v]) {
        seen[v] = 1u;
        (*count)++;
    }
}

static void count_phase(P11pStats *s,
                        P11pEndpointCacheModel *cache,
                        uint8_t line_index,
                        uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    uint8_t next = (uint8_t)(i + 1u);
    int32_t a = space_sine_point(i);
    int32_t b = space_sine_point(next);
    int16_t authority = space_sine_q15(phase);
    int16_t reconstructed = (int16_t)(a + (((b - a) * frac) >> 8));

    assert(line_index < APSIS_SPACE_FDN_LINES);
    assert(authority == reconstructed);

    s->calls++;
    s->point_lookups += 2u;
    if (frac == 0u) s->frac_zero++;
    else s->frac_nonzero++;
    if ((i & 63u) == 63u) s->boundary_crossings++;
    if (a == b) s->equal_endpoint_pairs++;
    s->quadrant_a[i >> 6]++;
    s->quadrant_b[next >> 6]++;
    mark_seen(s->seen_index, &s->distinct_index, i);
    mark_seen(s->seen_frac, &s->distinct_frac, frac);

    if (!cache->valid[line_index] || cache->index[line_index] != i) {
        cache->valid[line_index] = 1u;
        cache->index[line_index] = i;
        s->endpoint_refreshes++;
        s->line_refreshes[line_index]++;
    } else {
        s->endpoint_reuses++;
    }
}

static void run_natural_segment(ApsisSpaceFdn *fdn,
                                uint32_t frames,
                                P11pStats *stats,
                                P11pEndpointCacheModel *cache)
{
    uint32_t frame;
    uint8_t k;
    memset(stats, 0, sizeof(*stats));
    for (frame = 0u; frame < frames; ++frame) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            fdn->line[k].lfo_phase += fdn->line[k].lfo_phase_inc;
            count_phase(stats, cache, k, fdn->line[k].lfo_phase);
        }
    }
}

static void print_segment(const char *name, const P11pStats *s)
{
    uint64_t avoided = 2u * (s->calls - s->endpoint_refreshes);
    printf("P11P %s calls=%llu point_lookups=%llu frac_zero=%llu frac_nonzero=%llu "
           "boundary_crossings=%llu endpoint_refreshes=%llu endpoint_reuses=%llu "
           "avoided_point_evals_if_cached=%llu distinct_index=%u distinct_frac=%u\n",
           name,
           (unsigned long long)s->calls,
           (unsigned long long)s->point_lookups,
           (unsigned long long)s->frac_zero,
           (unsigned long long)s->frac_nonzero,
           (unsigned long long)s->boundary_crossings,
           (unsigned long long)s->endpoint_refreshes,
           (unsigned long long)s->endpoint_reuses,
           (unsigned long long)avoided,
           s->distinct_index,
           s->distinct_frac);
    printf("P11P %s quadrants_a=%llu,%llu,%llu,%llu quadrants_b=%llu,%llu,%llu,%llu\n",
           name,
           (unsigned long long)s->quadrant_a[0],
           (unsigned long long)s->quadrant_a[1],
           (unsigned long long)s->quadrant_a[2],
           (unsigned long long)s->quadrant_a[3],
           (unsigned long long)s->quadrant_b[0],
           (unsigned long long)s->quadrant_b[1],
           (unsigned long long)s->quadrant_b[2],
           (unsigned long long)s->quadrant_b[3]);
    printf("P11P %s line_refreshes=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu equal_pairs=%llu\n",
           name,
           (unsigned long long)s->line_refreshes[0],
           (unsigned long long)s->line_refreshes[1],
           (unsigned long long)s->line_refreshes[2],
           (unsigned long long)s->line_refreshes[3],
           (unsigned long long)s->line_refreshes[4],
           (unsigned long long)s->line_refreshes[5],
           (unsigned long long)s->line_refreshes[6],
           (unsigned long long)s->line_refreshes[7],
           (unsigned long long)s->equal_endpoint_pairs);
}

static void test_natural_trajectory(void)
{
    ApsisSpaceFdn fdn;
    P11pStats cold, warm;
    P11pEndpointCacheModel cache;
    int16_t *storage = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*storage));
    assert(storage);
    memset(&fdn, 0, sizeof(fdn));
    memset(&cache, 0, sizeof(cache));
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    run_natural_segment(&fdn, 16384u, &cold, &cache);
    assert(cold.calls == 131072u);
    assert(cold.point_lookups == 262144u);
    assert(cold.frac_zero == 535u);
    assert(cold.frac_nonzero == 130537u);
    assert(cold.boundary_crossings == 0u);
    assert(cold.endpoint_refreshes == 96u);
    assert(cold.endpoint_reuses == 130976u);
    assert(cold.distinct_index == 96u);
    assert(cold.distinct_frac == 256u);
    assert(cold.equal_endpoint_pairs == 0u);
    assert(cold.line_refreshes[0] == 7u);
    assert(cold.line_refreshes[1] == 8u);
    assert(cold.line_refreshes[2] == 10u);
    assert(cold.line_refreshes[3] == 11u);
    assert(cold.line_refreshes[4] == 13u);
    assert(cold.line_refreshes[5] == 14u);
    assert(cold.line_refreshes[6] == 15u);
    assert(cold.line_refreshes[7] == 18u);
    print_segment("cold", &cold);

    /* Cache model intentionally persists across the segment boundary: this is
     * the actual refresh count a per-line endpoint cache would see. */
    run_natural_segment(&fdn, 65536u, &warm, &cache);
    assert(warm.calls == 524288u);
    assert(warm.point_lookups == 1048576u);
    assert(warm.frac_zero == 2050u);
    assert(warm.frac_nonzero == 522238u);
    assert(warm.boundary_crossings == 6944u);
    assert(warm.endpoint_refreshes == 367u);
    assert(warm.endpoint_reuses == 523921u);
    assert(warm.distinct_index == 255u);
    assert(warm.distinct_frac == 256u);
    assert(warm.equal_endpoint_pairs == 0u);
    assert(warm.line_refreshes[0] == 27u);
    assert(warm.line_refreshes[1] == 32u);
    assert(warm.line_refreshes[2] == 37u);
    assert(warm.line_refreshes[3] == 41u);
    assert(warm.line_refreshes[4] == 48u);
    assert(warm.line_refreshes[5] == 53u);
    assert(warm.line_refreshes[6] == 60u);
    assert(warm.line_refreshes[7] == 69u);
    print_segment("warm", &warm);

    printf("P11P combined calls=655360 endpoint_refreshes=463 endpoint_reuses=654897 "
           "avoided_point_evals_if_cached=1309794\n");
    free(storage);
}

static void test_exhaustive_top16_domain(void)
{
    P11pStats s;
    P11pEndpointCacheModel cache;
    uint32_t i, frac;
    memset(&s, 0, sizeof(s));
    memset(&cache, 0, sizeof(cache));

    /* Lower 16 phase bits are ignored by the frozen sine helper, so all 65,536
     * top-16 combinations exhaust its complete input partition. Use line 0
     * only for cache bookkeeping; cache counts are not used as authority here. */
    for (i = 0u; i < 256u; ++i) {
        for (frac = 0u; frac < 256u; ++frac) {
            uint32_t phase = (i << 24) | (frac << 16);
            count_phase(&s, &cache, 0u, phase);
        }
    }
    assert(s.calls == 65536u);
    assert(s.point_lookups == 131072u);
    assert(s.frac_zero == 256u);
    assert(s.frac_nonzero == 65280u);
    assert(s.boundary_crossings == 1024u);
    assert(s.equal_endpoint_pairs == 0u);
    assert(s.distinct_index == 256u);
    assert(s.distinct_frac == 256u);
    assert(s.quadrant_a[0] == 16384u && s.quadrant_a[1] == 16384u &&
           s.quadrant_a[2] == 16384u && s.quadrant_a[3] == 16384u);
    assert(s.quadrant_b[0] == 16384u && s.quadrant_b[1] == 16384u &&
           s.quadrant_b[2] == 16384u && s.quadrant_b[3] == 16384u);
    printf("P11P exhaustive calls=65536 point_lookups=131072 frac_zero=256 "
           "frac_nonzero=65280 boundary_crossings=1024 distinct_index=256 "
           "distinct_frac=256 equal_pairs=0\n");
}

int main(void)
{
    test_natural_trajectory();
    test_exhaustive_top16_domain();
    puts("PASS: P11P FDN LFO/sine workload census");
    return 0;
}
