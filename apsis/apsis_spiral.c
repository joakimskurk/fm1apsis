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
    return (uint64_t)rnd * APSIS_Q15_ONE < ((uint64_t)threshold_q15 << 32);
}

static uint16_t rnd_energy_q15(uint32_t rnd)
{
    const uint16_t lo = APSIS_Q15_MILLI(220);
    const uint16_t span = APSIS_Q15_MILLI(760);
    return (uint16_t)(lo + (uint16_t)(((uint64_t)rnd * span) >> 32));
}

/* the first scale index in -42..56 whose step is >= value (57 when none): apsis_scale_at never falls as the index
 * rises (a scale's repeated degree only holds it level), so this is a binary search */
static int32_t scale_lower_bound(const ApsisState *s, int32_t value)
{
    int32_t lo = -42, hi = 57;
    while (lo < hi) {
        int32_t mid = lo + (hi - lo) / 2;
        if (apsis_scale_at(s->gravity.tonic, s->gravity.scale, mid) >= value) hi = mid;
        else lo = mid + 1;
    }
    return lo;
}

/* the scale index nearest to step over -42..56, the lowest index on a tie: what a scan from -42 upward keeping
 * the first strictly smaller error found (and tested against that scan, test_spiral_route) */
static int32_t nearest_scale_index(const ApsisState *s, int32_t step)
{
    int32_t hi = scale_lower_bound(s, step), lo_value, lo;
    if (hi == -42) return -42;
    lo_value = apsis_scale_at(s->gravity.tonic, s->gravity.scale, hi - 1);
    lo = scale_lower_bound(s, lo_value);
    if (hi > 56) return lo;
    return step - lo_value <= apsis_scale_at(s->gravity.tonic, s->gravity.scale, hi) - step ? lo : hi;
}

static void set_default_tail(ApsisSpiralState *sp, uint8_t from)
{
    uint8_t i;
    for (i = from; i < APSIS_ATTRACTOR_STEPS; ++i) {
        if (DEFAULT_GATE[i % 16u]) sp->attractor_gates |= 1u << i;
        else sp->attractor_gates &= ~(1u << i);
        sp->attractor_energy_q15[i] = DEFAULT_ENERGY[i % 16u];
    }
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
    sp->route_count = write;
    while (write < APSIS_SPIRAL_ROUTE_MAX) sp->route[write++] = 0u;
    return sp->route_count;
}

/* one route point's step in the current frame (Study 126 spiralDerivedSequence's map) */
static int16_t derived_entry(const ApsisState *s, int32_t raw)
{
    int32_t base;
    if (!s->spiral.follow_frame)
        return (int16_t)(s->gravity.scale_lock
            ? apsis_quantize_step_to_scale(raw, s->gravity.tonic, s->gravity.scale, 1u)
            : raw);
    base = apsis_quantize_step_to_scale(raw, s->gravity.tonic, s->gravity.scale, 1u);
    return (int16_t)apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                                   nearest_scale_index(s, base) +
                                   (int32_t)(s->gravity.degree > 7u ? 7u : s->gravity.degree));
}

uint8_t apsis_spiral_derived_sequence(const ApsisState *s,
                                      int16_t out[APSIS_SPIRAL_ROUTE_MAX])
{
    uint8_t i, written = 0u, count;
    if (!s || !out) return 0u;
    count = s->spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : s->spiral.route_count;
    for (i = 0u; i < count; ++i)
        if (s->spiral.route[i] <= 35u) out[written++] = derived_entry(s, s->spiral.route[i]);
    return written;
}

uint8_t apsis_spiral_derived_count(const ApsisState *s)
{
    uint8_t i, n = 0u, count;
    if (!s) return 0u;
    count = s->spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : s->spiral.route_count;
    for (i = 0u; i < count; ++i) n = (uint8_t)(n + (s->spiral.route[i] <= 35u));
    return n;
}

uint8_t apsis_spiral_derived_step(const ApsisState *s, uint8_t index, int16_t *step)
{
    uint8_t i, n = 0u, count;
    if (!s || !step) return 0u;
    count = s->spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : s->spiral.route_count;
    for (i = 0u; i < count; ++i) {
        if (s->spiral.route[i] > 35u) continue;
        if (n++ == index) {
            *step = derived_entry(s, s->spiral.route[i]);
            return 1u;
        }
    }
    return 0u;
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
        for (i = 0u; i < len; ++i)
            sp->attractor_energy_q15[i] = clamp_q15(sp->attractor_energy_q15[i], APSIS_Q15_MILLI(120), APSIS_Q15_ONE);
        set_default_tail(sp, len);
        return;
    }

    sp->attractor_seed = (uint16_t)((((uint32_t)(sp->attractor_seed ? sp->attractor_seed : 911u) * 71u + 137u) % 9999u) + 1u);
    x = sp->attractor_seed ? sp->attractor_seed : 911u;

    for (i = 0u; i < len; ++i) {
        uint16_t bias = (i % 4u == 0u) ? APSIS_Q15_MILLI(120) : ((i % 2u == 0u) ? APSIS_Q15_MILLI(40) : 0u);
        uint32_t threshold = (uint32_t)density + bias;
        if (threshold > APSIS_Q15_MILLI(940)) threshold = APSIS_Q15_MILLI(940);
        if (rnd_below_q15(lcg_next(&x), (uint16_t)threshold)) gates |= 1u << i;
    }
    if (!gates) gates = 1u;
    sp->attractor_gates = gates;

    for (i = 0u; i < len; ++i)
        sp->attractor_energy_q15[i] = rnd_energy_q15(lcg_next(&x));
    set_default_tail(sp, len);
}
