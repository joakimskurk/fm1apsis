#include "apsis_spiral.h"
#include "apsis_harmony.h"

static const uint8_t DEFAULT_GATE[16] = {
    1u,0u,1u,1u,0u,1u,0u,0u,1u,0u,1u,1u,0u,1u,0u,0u
};
static const uint16_t DEFAULT_ENERGY[16] = {
    APSIS_Q15_MILLI(820), APSIS_Q15_MILLI(340), APSIS_Q15_MILLI(680), APSIS_Q15_MILLI(930),
    APSIS_Q15_MILLI(280), APSIS_Q15_MILLI(760), APSIS_Q15_MILLI(420), APSIS_Q15_MILLI(220),
    APSIS_Q15_MILLI(880), APSIS_Q15_MILLI(310), APSIS_Q15_MILLI(640), APSIS_Q15_MILLI(790),
    APSIS_Q15_MILLI(270), APSIS_Q15_MILLI(710), APSIS_Q15_MILLI(390), APSIS_Q15_MILLI(240)
};

static uint16_t clamp_q15(uint16_t value, uint16_t lo, uint16_t hi)
{
    return value < lo ? lo : (value > hi ? hi : value);
}

static uint32_t lcg_next(uint32_t *x)
{
    *x = 1664525u * *x + 1013904223u;
    return *x;
}

static uint8_t rnd_below_q15(uint32_t rnd, uint16_t threshold_q15)
{
    /* rnd / 2^32 < threshold / 32767, compared without division. */
    return (uint64_t)rnd * APSIS_Q15_ONE < ((uint64_t)threshold_q15 << 32);
}

static uint16_t rnd_energy_q15(uint32_t rnd)
{
    const uint16_t lo = APSIS_Q15_MILLI(220);
    const uint16_t span = APSIS_Q15_MILLI(760);
    return (uint16_t)(lo + (uint16_t)(((uint64_t)rnd * span) >> 32));
}

static int32_t nearest_scale_index(const ApsisState *s, int32_t step)
{
    int32_t i, best = 0, err = 999;
    for (i = -42; i <= 56; ++i) {
        int32_t cand = apsis_scale_at(s->gravity.tonic, s->gravity.scale, i);
        int32_t e = cand > step ? cand - step : step - cand;
        if (e < err) {
            err = e;
            best = i;
        }
    }
    return best;
}

uint8_t apsis_spiral_sanitize_route(ApsisSpiralState *sp)
{
    uint8_t read, write = 0u, count;
    if (!sp) return 0u;
    count = sp->route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : sp->route_count;
    for (read = 0u; read < count; ++read) {
        uint8_t v = sp->route[read];
        if (v <= 35u) sp->route[write++] = v;
    }
    while (write < APSIS_SPIRAL_ROUTE_MAX) sp->route[write++] = 0u;
    sp->route_count = 0u;
    for (read = 0u; read < count; ++read)
        if (sp->route[read] <= 35u) ++sp->route_count;
    return sp->route_count;
}

uint8_t apsis_spiral_derived_sequence(const ApsisState *s,
                                      int16_t out[APSIS_SPIRAL_ROUTE_MAX])
{
    uint8_t i, count;
    if (!s || !out) return 0u;
    count = s->spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : s->spiral.route_count;
    for (i = 0u; i < count; ++i) {
        int32_t raw = s->spiral.route[i];
        int32_t base;
        if (raw > 35) continue;
        if (!s->spiral.follow_frame) {
            out[i] = (int16_t)(s->gravity.scale_lock
                ? apsis_quantize_step_to_scale(raw, s->gravity.tonic, s->gravity.scale, 1u)
                : raw);
        } else {
            base = apsis_quantize_step_to_scale(raw, s->gravity.tonic, s->gravity.scale, 1u);
            out[i] = (int16_t)apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                                             nearest_scale_index(s, base) +
                                             (int32_t)(s->gravity.degree > 7u ? 7u : s->gravity.degree));
        }
    }
    return count;
}

uint8_t apsis_spiral_attractor_length(uint8_t length)
{
    return (length == 8u || length == 12u || length == 16u || length == 24u) ? length : 16u;
}

void apsis_spiral_reset_attractor(ApsisSpiralState *sp, uint8_t regenerate)
{
    uint8_t len, i;
    uint16_t density;
    uint32_t x, gates = 0u;
    if (!sp) return;

    len = apsis_spiral_attractor_length(sp->attractor_length);
    density = clamp_q15(sp->attractor_density_q15, APSIS_Q15_MILLI(150), APSIS_Q15_MILLI(900));
    sp->attractor_length = len;
    sp->attractor_density_q15 = density;

    if (!regenerate) {
        uint32_t mask = len == 24u ? 0x00FFFFFFu : ((1u << len) - 1u);
        sp->attractor_gates &= mask;
        for (i = 0u; i < len; ++i)
            sp->attractor_energy_q15[i] = clamp_q15(sp->attractor_energy_q15[i], APSIS_Q15_MILLI(120), APSIS_Q15_ONE);
        return;
    }

    sp->attractor_seed = (uint16_t)((((uint32_t)(sp->attractor_seed ? sp->attractor_seed : 911u) * 71u + 137u) % 9999u) + 1u);
    x = sp->attractor_seed ? sp->attractor_seed : 911u;

    for (i = 0u; i < len; ++i) {
        uint16_t threshold = density;
        uint16_t bias = (i % 4u == 0u) ? APSIS_Q15_MILLI(120) : ((i % 2u == 0u) ? APSIS_Q15_MILLI(40) : 0u);
        uint32_t t = (uint32_t)threshold + bias;
        if (t > APSIS_Q15_MILLI(940)) t = APSIS_Q15_MILLI(940);
        if (rnd_below_q15(lcg_next(&x), (uint16_t)t)) gates |= 1u << i;
    }
    if (!gates) gates = 1u;
    sp->attractor_gates = gates;

    for (i = 0u; i < len; ++i)
        sp->attractor_energy_q15[i] = rnd_energy_q15(lcg_next(&x));

    /* Browser arrays only have active length. Fixed native capacity is reset to
     * source defaults beyond it so a later length increase extends identically. */
    for (; i < APSIS_ATTRACTOR_STEPS; ++i) {
        if (DEFAULT_GATE[i % 16u]) sp->attractor_gates |= 1u << i;
        else sp->attractor_gates &= ~(1u << i);
        sp->attractor_energy_q15[i] = DEFAULT_ENERGY[i % 16u];
    }
}
