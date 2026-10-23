#include "apsis_clock.h"

static uint8_t clamp_n(uint8_t n)
{
    if (n < 3u) return 3u;
    if (n > APSIS_CLOCK_STEPS_MAX) return APSIS_CLOCK_STEPS_MAX;
    return n;
}

static uint16_t clamp_bpm(uint16_t bpm)
{
    if (bpm < 35u) return 35u;
    if (bpm > 180u) return 180u;
    return bpm;
}

static uint16_t clamp_swing(uint16_t swing_q15)
{
    uint16_t max = APSIS_Q15_MILLI(350);
    return swing_q15 > max ? max : swing_q15;
}

static void division(const ApsisClockState *c, uint8_t *num, uint8_t *den)
{
    uint8_t n = c ? c->division_num : 1u;
    uint8_t d = c ? c->division_den : 2u;
    if (n != 1u || (d != 1u && d != 2u && d != 4u)) {
        n = 1u;
        d = 2u;
    }
    *num = n;
    *den = d;
}

static uint32_t valid_mask(uint8_t n)
{
    return n >= 32u ? 0xFFFFFFFFu : ((1u << n) - 1u);
}

uint8_t apsis_clock_groups(const ApsisClockState *c, uint8_t out[APSIS_CLOCK_GROUPS_MAX])
{
    uint8_t b[3], count = 0u, i, j, n, prev = 0u;
    if (!c || !out) return 0u;
    n = clamp_n(c->n);
    for (i = 0u; i < 3u; ++i) {
        uint8_t v = c->bounds[i];
        if (v > 0u && v < n) b[count++] = v;
    }
    for (i = 1u; i < count; ++i) {
        uint8_t x = b[i];
        j = i;
        while (j && b[j - 1u] > x) { b[j] = b[j - 1u]; --j; }
        b[j] = x;
    }
    for (i = 0u; i < count; ++i) {
        out[i] = (uint8_t)(b[i] - prev);
        prev = b[i];
    }
    out[count] = (uint8_t)(n - prev);
    return (uint8_t)(count + 1u);
}

uint32_t apsis_clock_base_interval_us(const ApsisClockState *c)
{
    uint8_t num, den;
    uint16_t bpm;
    uint32_t quarter;
    if (!c) return 416667u;
    bpm = clamp_bpm(c->bpm);
    division(c, &num, &den);
    quarter = (60000000u + bpm / 2u) / bpm;
    return (quarter * num + den / 2u) / den;
}

uint32_t apsis_clock_interval_us(const ApsisClockState *c, uint32_t absolute_step)
{
    uint32_t base = apsis_clock_base_interval_us(c);
    uint32_t delta;
    uint16_t swing = c ? clamp_swing(c->swing_q15) : 0u;
    delta = (base * swing + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    return (absolute_step & 1u) ? base + delta : base - delta;
}

ApsisClockPulse apsis_clock_pulse(const ApsisClockState *c, uint8_t cycle_step, uint32_t absolute_step)
{
    ApsisClockPulse p = {0};
    uint8_t n, i, k;
    uint32_t bit;
    if (!c) return p;
    n = clamp_n(c->n);
    i = (uint8_t)(cycle_step % n);
    bit = 1u << i;
    p.step = i;
    p.n = n;
    p.main_on = (c->main_mask & bit) != 0u;
    p.inner_on = (c->inner_mask & bit) != 0u;
    p.snap_on = (c->snap_mask & bit) != 0u;
    p.accent = p.main_on && i == 0u;
    if (p.main_on && !p.accent) {
        for (k = 0u; k < 3u; ++k)
            if (c->bounds[k] == i) { p.accent = 1u; break; }
    }
    p.step_one_resync = i == 0u && c->step_one_resync != 0u;
    p.interval_us = apsis_clock_interval_us(c, absolute_step);
    return p;
}

void apsis_clock_set_length(ApsisClockState *c, uint8_t requested_n)
{
    uint8_t old_n, n, i;
    uint32_t mask;
    if (!c) return;
    old_n = clamp_n(c->n);
    n = clamp_n(requested_n);
    if (n > old_n) {
        for (i = old_n; i < n; ++i) c->main_mask |= 1u << i;
    }
    c->n = n;
    mask = valid_mask(n);
    c->main_mask &= mask;
    c->inner_mask &= mask;
    c->snap_mask &= mask;
    for (i = 0u; i < 3u; ++i)
        if (c->bounds[i] == 0u || c->bounds[i] >= n) c->bounds[i] = 0u;
}
