#include "apsis_harmony.h"

static const int8_t SCALES[APSIS_SCALE_COUNT][APSIS_SCALE_DEGREES] = {
    {0, 2, 4, 5, 7, 9, 11}, /* major */
    {0, 2, 3, 5, 7, 8, 10}, /* naturalMinor */
    {0, 2, 3, 5, 7, 8, 11}, /* harmonicMinor */
    {0, 2, 3, 5, 7, 9, 11}, /* melodicMinor */
    {0, 2, 3, 5, 7, 9, 10}, /* dorian */
    {0, 1, 3, 5, 7, 8, 10}, /* phrygian */
    {0, 2, 4, 6, 7, 9, 11}, /* lydian */
    {0, 2, 4, 5, 7, 9, 10}  /* mixolydian */
};

/* Browser tunings in thousandths of a cent. */
static const int32_t TUNING_EQUAL[12] = {
    0, 100000, 200000, 300000, 400000, 500000,
    600000, 700000, 800000, 900000, 1000000, 1100000
};
static const int32_t TUNING_JUST[12] = {
    0, 111731, 203910, 315641, 386314, 498045,
    590224, 701955, 813686, 884359, 1017596, 1088269
};
static const int32_t TUNING_PYTH[12] = {
    0, 113685, 203910, 317595, 407820, 521505,
    611730, 701955, 815640, 905865, 1019550, 1109775
};

static uint8_t norm_pc(int32_t v)
{
    int32_t r = v % 12;
    if (r < 0) r += 12;
    return (uint8_t)r;
}

static uint8_t norm_scale(uint8_t scale_id)
{
    return scale_id < APSIS_SCALE_COUNT ? scale_id : APSIS_SCALE_DORIAN;
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

const int8_t *apsis_scale_intervals(uint8_t scale_id)
{
    return SCALES[norm_scale(scale_id)];
}

int32_t apsis_scale_at(uint8_t tonic, uint8_t scale_id, int32_t index)
{
    const int8_t *sc = apsis_scale_intervals(scale_id);
    int32_t oct = index / 7;
    int32_t ix = index % 7;
    if (ix < 0) {
        ix += 7;
        --oct;
    }
    return (int32_t)(tonic % 12u) + (int32_t)sc[ix] + oct * 12;
}

static uint8_t allowed_pitch_class(uint8_t pc, uint8_t tonic, uint8_t scale_id)
{
    const int8_t *sc = apsis_scale_intervals(scale_id);
    uint32_t i;
    for (i = 0; i < APSIS_SCALE_DEGREES; ++i)
        if (norm_pc((int32_t)(tonic % 12u) + sc[i]) == pc) return 1u;
    return 0u;
}

int32_t apsis_quantize_step_to_scale(int32_t step, uint8_t tonic, uint8_t scale_id, uint8_t scale_lock)
{
    int32_t k, best = step, err = 999;
    if (!scale_lock) return step;
    for (k = -12; k <= 12; ++k) {
        int32_t cand = step + k;
        if (allowed_pitch_class(norm_pc(cand), tonic, scale_id)) {
            int32_t e = k < 0 ? -k : k;
            if (e < err) {
                err = e;
                best = cand;
            }
        }
    }
    return best;
}

static uint8_t clamp_voice_count(uint8_t voices)
{
    if (voices < 3u) return 3u;
    if (voices > 5u) return 5u;
    return voices;
}

static uint8_t clamp_ring(uint8_t ring)
{
    return ring > APSIS_GRAVITY_AIR ? APSIS_GRAVITY_AIR : ring;
}

uint8_t apsis_gravity_ring_intervals(const ApsisGravityState *g, int16_t out[APSIS_GRAVITY_VOICES_MAX])
{
    static const int8_t profile[3][5] = {
        {0, 2, 4, 7, 11},
        {0, 2, 4, 6, 8},
        {4, 8, 9, 10, 13}
    };
    uint8_t ring, voices, i, degree;
    int32_t base;
    if (!g || !out) return 0u;
    ring = clamp_ring(g->voicing);
    voices = clamp_voice_count(g->voices);
    degree = (uint8_t)(g->degree % 7u);
    base = apsis_scale_at(g->tonic, g->scale, degree);
    for (i = 0; i < voices; ++i)
        out[i] = (int16_t)(apsis_scale_at(g->tonic, g->scale, (int32_t)degree + profile[ring][i]) - base);
    return voices;
}

static void sort_i16(int16_t *v, uint8_t n)
{
    uint8_t i;
    for (i = 1u; i < n; ++i) {
        int16_t x = v[i];
        uint8_t j = i;
        while (j && v[j - 1u] > x) {
            v[j] = v[j - 1u];
            --j;
        }
        v[j] = x;
    }
}

uint8_t apsis_gravity_chord_steps(const ApsisGravityState *g, int16_t out[APSIS_GRAVITY_VOICES_MAX])
{
    int16_t intervals[APSIS_GRAVITY_VOICES_MAX];
    uint8_t voices, ring, inv, i;
    int32_t root;
    if (!g || !out) return 0u;
    voices = apsis_gravity_ring_intervals(g, intervals);
    ring = clamp_ring(g->voicing);
    root = apsis_scale_at(g->tonic, g->scale, g->degree > 7u ? 7u : g->degree);
    for (i = 0u; i < voices; ++i) out[i] = (int16_t)(root + intervals[i]);

    inv = g->inversion;
    if (voices && inv >= voices) inv = (uint8_t)(voices - 1u);
    if (ring == APSIS_GRAVITY_ANCHOR && inv > 1u) inv = 1u;
    for (i = 0u; i < inv; ++i) out[i] = (int16_t)(out[i] + 12);
    sort_i16(out, voices);

    if (ring == APSIS_GRAVITY_COLOUR) {
        for (i = 2u; i < voices; ++i) out[i] = (int16_t)(out[i] + 12);
    } else if (ring == APSIS_GRAVITY_AIR) {
        for (i = 1u; i < voices; ++i) out[i] = (int16_t)(out[i] + (int16_t)((i - 1u) * 5u));
        sort_i16(out, voices);
    }
    return voices;
}

static const int32_t *tuning_table(uint8_t mode)
{
    if (mode == APSIS_TUNING_JUST) return TUNING_JUST;
    if (mode == APSIS_TUNING_PYTH) return TUNING_PYTH;
    return TUNING_EQUAL;
}

int32_t apsis_tuning_cents_milli(uint8_t pitch_class, uint8_t tonic, uint8_t mode, uint16_t strength_q15)
{
    uint8_t rel = norm_pc((int32_t)(pitch_class % 12u) - (int32_t)(tonic % 12u));
    const int32_t *target = tuning_table(mode);
    int32_t eq = TUNING_EQUAL[rel];
    int32_t diff = target[rel] - eq;
    int32_t strength = clamp_i32(strength_q15, 0, APSIS_Q15_ONE);
    int32_t scaled;
    if (diff >= 0)
        scaled = (diff * strength + (APSIS_Q15_ONE / 2)) / APSIS_Q15_ONE;
    else
        scaled = -(((-diff) * strength + (APSIS_Q15_ONE / 2)) / APSIS_Q15_ONE);
    return eq + scaled;
}

int32_t apsis_tuning_deviation_milli(uint8_t pitch_class, uint8_t tonic, uint8_t mode, uint16_t strength_q15)
{
    uint8_t rel = norm_pc((int32_t)(pitch_class % 12u) - (int32_t)(tonic % 12u));
    return apsis_tuning_cents_milli(pitch_class, tonic, mode, strength_q15) - TUNING_EQUAL[rel];
}
