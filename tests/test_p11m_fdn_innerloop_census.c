#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apsis_space_dsp.h"

typedef struct {
    uint64_t sine_calls;
    uint64_t sine_frac_zero;
    uint64_t line_calls;
    uint64_t modulation_multiple_256;
    int32_t modulation_min;
    int32_t modulation_max;
    uint64_t delay_clamp_opportunities;
    int32_t delay_min;
    int32_t delay_max;
    uint64_t readiness_checks;
    uint64_t not_ready;
    uint64_t full_line_calls;
    uint64_t wrap_checks;
    uint64_t negative_wrap;
    uint64_t upper_wrap;
    int32_t raw_read_min;
    int32_t raw_read_max;
    int32_t span_min;
    int32_t span_max;
    uint64_t interp_checks;
    uint64_t integer_reads;
    uint64_t interpolated_reads;
    uint64_t i1_wraps;
} P11mProbe;

static P11mProbe g_probe;

static void probe_reset(void)
{
    memset(&g_probe, 0, sizeof(g_probe));
    g_probe.modulation_min = INT32_MAX;
    g_probe.modulation_max = INT32_MIN;
    g_probe.delay_min = INT32_MAX;
    g_probe.delay_max = INT32_MIN;
    g_probe.raw_read_min = INT32_MAX;
    g_probe.raw_read_max = INT32_MIN;
    g_probe.span_min = INT32_MAX;
    g_probe.span_max = INT32_MIN;
}

void p11m_probe_sine(uint8_t phase_frac)
{
    g_probe.sine_calls++;
    if (phase_frac == 0u) g_probe.sine_frac_zero++;
}

void p11m_probe_line_call(int32_t modulation_q8)
{
    g_probe.line_calls++;
    if ((modulation_q8 & 255) == 0) g_probe.modulation_multiple_256++;
    if (modulation_q8 < g_probe.modulation_min) g_probe.modulation_min = modulation_q8;
    if (modulation_q8 > g_probe.modulation_max) g_probe.modulation_max = modulation_q8;
}

void p11m_probe_delay(int32_t delay_q8)
{
    if (delay_q8 < 256) g_probe.delay_clamp_opportunities++;
    if (delay_q8 < g_probe.delay_min) g_probe.delay_min = delay_q8;
    if (delay_q8 > g_probe.delay_max) g_probe.delay_max = delay_q8;
}

void p11m_probe_readiness(uint16_t fill_count, uint16_t capacity, uint32_t needed)
{
    g_probe.readiness_checks++;
    if (fill_count < needed) g_probe.not_ready++;
    if (fill_count == capacity) g_probe.full_line_calls++;
}

void p11m_probe_wrap(int32_t raw_read_q8, int32_t span_q8)
{
    g_probe.wrap_checks++;
    if (raw_read_q8 < 0) g_probe.negative_wrap++;
    if (raw_read_q8 >= span_q8) g_probe.upper_wrap++;
    if (raw_read_q8 < g_probe.raw_read_min) g_probe.raw_read_min = raw_read_q8;
    if (raw_read_q8 > g_probe.raw_read_max) g_probe.raw_read_max = raw_read_q8;
    if (span_q8 < g_probe.span_min) g_probe.span_min = span_q8;
    if (span_q8 > g_probe.span_max) g_probe.span_max = span_q8;
}

void p11m_probe_interp(uint32_t frac, uint32_t i1, uint16_t capacity)
{
    g_probe.interp_checks++;
    if (frac == 0u) g_probe.integer_reads++;
    else g_probe.interpolated_reads++;
    if (i1 >= capacity) g_probe.i1_wraps++;
}

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void render_frames(ApsisSpaceFdn *fdn, uint32_t frames_total, uint32_t *rng)
{
    int32_t input[64];
    int32_t output[128];
    uint32_t done = 0u;
    while (done < frames_total) {
        uint32_t n = frames_total - done;
        uint32_t i;
        if (n > 64u) n = 64u;
        for (i = 0u; i < n; ++i)
            input[i] = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 4;
        memset(output, 0, sizeof(output));
        apsis_space_fdn_render(fdn, input, output, n, 8000u, 4500u);
        done += n;
    }
}

static void assert_common(uint32_t frames)
{
    uint64_t expected = (uint64_t)frames * APSIS_SPACE_FDN_LINES;
    assert(g_probe.sine_calls == expected);
    assert(g_probe.line_calls == expected);
    assert(g_probe.readiness_checks == expected);
    assert(g_probe.modulation_min >= -2304);
    assert(g_probe.modulation_max <= 2304);
    assert(g_probe.delay_clamp_opportunities == 0u);
    assert(g_probe.delay_min > 256);
    assert(g_probe.wrap_checks + g_probe.not_ready == g_probe.line_calls);
    assert(g_probe.interp_checks == g_probe.wrap_checks);
    assert(g_probe.integer_reads + g_probe.interpolated_reads == g_probe.interp_checks);
    assert(g_probe.upper_wrap == 0u);
    assert(g_probe.raw_read_min > -g_probe.span_max);
    assert(g_probe.raw_read_max < g_probe.span_max);
}

static void print_probe(const char *scenario, uint32_t frames)
{
    printf(
        "P11M scenario=%s frames=%u sine_calls=%llu sine_frac_zero=%llu "
        "line_calls=%llu mod_min=%ld mod_max=%ld mod_mul256=%llu "
        "delay_min=%ld delay_max=%ld clamp_opportunities=%llu "
        "not_ready=%llu full_line_calls=%llu ready_reads=%llu "
        "negative_wrap=%llu upper_wrap=%llu raw_min=%ld raw_max=%ld "
        "integer_reads=%llu interpolated_reads=%llu i1_wraps=%llu\n",
        scenario,
        frames,
        (unsigned long long)g_probe.sine_calls,
        (unsigned long long)g_probe.sine_frac_zero,
        (unsigned long long)g_probe.line_calls,
        (long)g_probe.modulation_min,
        (long)g_probe.modulation_max,
        (unsigned long long)g_probe.modulation_multiple_256,
        (long)g_probe.delay_min,
        (long)g_probe.delay_max,
        (unsigned long long)g_probe.delay_clamp_opportunities,
        (unsigned long long)g_probe.not_ready,
        (unsigned long long)g_probe.full_line_calls,
        (unsigned long long)g_probe.wrap_checks,
        (unsigned long long)g_probe.negative_wrap,
        (unsigned long long)g_probe.upper_wrap,
        (long)g_probe.raw_read_min,
        (long)g_probe.raw_read_max,
        (unsigned long long)g_probe.integer_reads,
        (unsigned long long)g_probe.interpolated_reads,
        (unsigned long long)g_probe.i1_wraps);
}

static void run_cold(void)
{
    enum { FRAMES = 16384 };
    ApsisSpaceFdn fdn = {0};
    int16_t *storage = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*storage));
    uint32_t rng = 0x434f4c44u;
    assert(storage);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    probe_reset();
    render_frames(&fdn, FRAMES, &rng);
    assert_common(FRAMES);
    assert(g_probe.not_ready > 0u);
    assert(g_probe.full_line_calls > 0u);
    assert(g_probe.negative_wrap > 0u);
    assert(g_probe.interpolated_reads > 0u);
    print_probe("cold", FRAMES);
    free(storage);
}

static void run_warm(void)
{
    enum { PREFILL_FRAMES = 4096, FRAMES = 65536 };
    ApsisSpaceFdn fdn = {0};
    int16_t *storage = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*storage));
    uint32_t rng = 0x5741524du;
    uint8_t k;
    assert(storage);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    probe_reset();
    render_frames(&fdn, PREFILL_FRAMES, &rng);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        assert(fdn.line[k].fill_count == fdn.line[k].capacity);

    probe_reset();
    render_frames(&fdn, FRAMES, &rng);
    assert_common(FRAMES);
    assert(g_probe.not_ready == 0u);
    assert(g_probe.full_line_calls == g_probe.line_calls);
    assert(g_probe.wrap_checks == g_probe.line_calls);
    assert(g_probe.negative_wrap > 0u);
    assert(g_probe.integer_reads > 0u);
    assert(g_probe.interpolated_reads > 0u);
    print_probe("warm", FRAMES);
    free(storage);
}

int main(void)
{
    run_cold();
    run_warm();
    puts("PASS: P11M FDN inner-loop branch census");
    return 0;
}
