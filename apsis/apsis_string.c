#include "apsis_string.h"
#include "apsis_harmony.h"

#define Q16_ONE 65536u
#define Q16_TWO 131072u
#define STRING_RAW_MAX_SEMIS_MILLI 30030
#define STRING_TWO_OCT_SEMIS_MILLI 24000
#define STRING_CLASSIC_MAX_SEMIS_MILLI 36000
#define STRING_SPAN_MIN_Q15 APSIS_Q15_MILLI(500)
#define STRING_SPAN_MAX_Q15 APSIS_Q15_MILLI(1800)

typedef struct { uint8_t n, d; } RatioPair;

static const RatioPair SIMPLE_RATIOS[] = {
    {1,1},{16,15},{9,8},{6,5},{5,4},{4,3},{7,5},
    {3,2},{8,5},{5,3},{7,4},{9,5},{15,8},{2,1}
};

static uint16_t clamp_u16(uint16_t v, uint16_t lo, uint16_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int32_t abs_i32(int32_t v)
{
    return v < 0 ? -v : v;
}

static uint8_t clamp_side(uint8_t side)
{
    return side == APSIS_SIDE_LEFT ? APSIS_SIDE_LEFT : APSIS_SIDE_RIGHT;
}

static uint8_t clamp_degree_i32(int32_t degree)
{
    if (degree < 0) return 0u;
    if (degree > 7) return 7u;
    return (uint8_t)degree;
}

static uint32_t ratio_q16_from_split(uint16_t split_q15)
{
    uint32_t s = clamp_u16(split_q15, APSIS_STRING_SPLIT_MIN_Q15, APSIS_STRING_SPLIT_MAX_Q15);
    uint32_t other = APSIS_Q15_ONE - s;
    uint32_t hi, lo;
    if (s >= APSIS_STRING_SPLIT_MID_Q15 - 1u && s <= APSIS_STRING_SPLIT_MID_Q15 + 1u)
        return Q16_ONE;
    hi = s > other ? s : other;
    lo = s > other ? other : s;
    if (!lo) return Q16_ONE;
    return (hi << 16) / lo;
}

/* Binary logarithm in Q16.16. Input is >= 1.0 in Q16.16.
 * The only 64-bit operation is multiply+shift, never division.
 */
static uint32_t log2_q16(uint32_t x)
{
    uint32_t exponent = 0u, frac = 0u, i;
    while (x >= Q16_TWO) {
        x >>= 1u;
        ++exponent;
    }
    for (i = 0u; i < 16u; ++i) {
        x = (uint32_t)(((uint64_t)x * x) >> 16);
        if (x >= Q16_TWO) {
            x >>= 1u;
            frac |= 1u << (15u - i);
        }
    }
    return (exponent << 16) | frac;
}

static int32_t raw_semis_milli(uint16_t split_q15)
{
    uint32_t lg = log2_q16(ratio_q16_from_split(split_q15));
    return (int32_t)((lg * 12000u + 32768u) >> 16);
}

static int32_t mapped_semis_milli(int32_t raw)
{
    int32_t num, den;
    if (raw <= STRING_TWO_OCT_SEMIS_MILLI) return raw;
    if (raw >= STRING_RAW_MAX_SEMIS_MILLI) return STRING_CLASSIC_MAX_SEMIS_MILLI;
    num = raw - STRING_TWO_OCT_SEMIS_MILLI;
    den = STRING_RAW_MAX_SEMIS_MILLI - STRING_TWO_OCT_SEMIS_MILLI;
    return STRING_TWO_OCT_SEMIS_MILLI +
           (num * (STRING_CLASSIC_MAX_SEMIS_MILLI - STRING_TWO_OCT_SEMIS_MILLI) + den / 2) / den;
}

void apsis_string_pitch_memory_reset(ApsisStringPitchMemory *m)
{
    if (!m) return;
    m->left_target_step = 0;
    m->right_target_step = 0;
    m->left_valid = 0u;
    m->right_valid = 0u;
}

void apsis_string_set_side_split(ApsisStringState *s, uint8_t side, uint16_t split_q15, uint8_t mirror)
{
    uint16_t v;
    if (!s) return;
    side = clamp_side(side);
    if (side == APSIS_SIDE_LEFT) {
        v = clamp_u16(split_q15, APSIS_STRING_SPLIT_MIN_Q15, APSIS_STRING_SPLIT_MID_Q15);
        s->left_split_q15 = v;
        if (s->surface_mode != APSIS_STRING_CLASSIC && s->linked && mirror)
            s->right_split_q15 = (uint16_t)(APSIS_Q15_ONE - v);
    } else {
        v = clamp_u16(split_q15, APSIS_STRING_SPLIT_MID_Q15, APSIS_STRING_SPLIT_MAX_Q15);
        s->right_split_q15 = v;
        if (s->surface_mode != APSIS_STRING_CLASSIC && s->linked && mirror)
            s->left_split_q15 = (uint16_t)(APSIS_Q15_ONE - v);
    }
    s->left_split_q15 = clamp_u16(s->left_split_q15, APSIS_STRING_SPLIT_MIN_Q15, APSIS_STRING_SPLIT_MID_Q15);
    s->right_split_q15 = clamp_u16(s->right_split_q15, APSIS_STRING_SPLIT_MID_Q15, APSIS_STRING_SPLIT_MAX_Q15);
    s->split_q15 = s->right_split_q15;
}

ApsisStringRatio apsis_string_ratio(uint16_t split_q15)
{
    ApsisStringRatio out;
    uint32_t i, best_err = 0xFFFFFFFFu, best_q16 = Q16_ONE;
    uint32_t raw = ratio_q16_from_split(split_q15);
    out.raw_ratio_q16 = raw;
    out.effective_ratio_q16 = raw;
    out.numerator = 1u;
    out.denominator = 1u;
    out.near_simple = 0u;

    for (i = 0u; i < (uint32_t)(sizeof SIMPLE_RATIOS / sizeof SIMPLE_RATIOS[0]); ++i) {
        uint32_t q = ((uint32_t)SIMPLE_RATIOS[i].n << 16) / SIMPLE_RATIOS[i].d;
        uint32_t err = raw > q ? raw - q : q - raw;
        if (err < best_err) {
            best_err = err;
            best_q16 = q;
            out.numerator = SIMPLE_RATIOS[i].n;
            out.denominator = SIMPLE_RATIOS[i].d;
        }
    }
    if (best_err * 1000u < best_q16 * 18u) {
        out.near_simple = 1u;
        out.effective_ratio_q16 = best_q16;
    }
    return out;
}

int32_t apsis_string_magnitude_milli(uint16_t split_q15, uint16_t span_q15)
{
    uint32_t mapped = (uint32_t)mapped_semis_milli(raw_semis_milli(split_q15));
    uint32_t span = clamp_u16(span_q15, STRING_SPAN_MIN_Q15, STRING_SPAN_MAX_Q15);
    return (int32_t)((mapped * span + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

int32_t apsis_string_side_degree(const ApsisState *s, uint8_t side)
{
    int32_t local;
    if (!s) return 0;
    side = clamp_side(side);
    if (s->string.surface_mode == APSIS_STRING_CLASSIC) return clamp_degree_i32(s->gravity.degree);
    if ((side == APSIS_SIDE_LEFT && s->string.left_follow_gravity) ||
        (side == APSIS_SIDE_RIGHT && s->string.right_follow_gravity))
        return clamp_degree_i32(s->gravity.degree);
    local = side == APSIS_SIDE_LEFT ? s->string.left_gravity_degree : s->string.right_gravity_degree;
    return clamp_degree_i32(local);
}

int32_t apsis_string_anchor_step(const ApsisState *s, uint8_t side)
{
    static const int8_t anchor_index[5] = {0, 2, 4, 6, 8};
    uint8_t anchor, degree;
    if (!s) return 0;
    side = clamp_side(side);
    degree = (uint8_t)apsis_string_side_degree(s, side);
    if (s->string.surface_mode == APSIS_STRING_CLASSIC && side == APSIS_SIDE_RIGHT)
        anchor = APSIS_ANCHOR_ROOT;
    else
        anchor = side == APSIS_SIDE_LEFT ? s->string.left_anchor : s->string.right_anchor;
    if (anchor > APSIS_ANCHOR_NINTH) anchor = APSIS_ANCHOR_ROOT;
    return apsis_scale_at(s->gravity.tonic, s->gravity.scale, (int32_t)degree + anchor_index[anchor]);
}

int32_t apsis_string_raw_step_milli(const ApsisState *s, uint8_t side)
{
    uint16_t split;
    if (!s) return 0;
    side = clamp_side(side);
    split = side == APSIS_SIDE_LEFT ? s->string.left_split_q15 : s->string.right_split_q15;
    return apsis_string_anchor_step(s, side) * 1000 +
           apsis_string_magnitude_milli(split, s->string.span_q15);
}

static int32_t quantize_scale_milli(const ApsisState *s, int32_t raw_milli)
{
    const int8_t *scale = apsis_scale_intervals(s->gravity.scale);
    int32_t cand, best = 0, best_err = 0x7FFFFFFF;
    uint8_t i;
    for (cand = -48; cand <= 84; ++cand) {
        int32_t pc = cand % 12;
        uint8_t allowed = 0u;
        if (pc < 0) pc += 12;
        for (i = 0u; i < 7u; ++i) {
            int32_t apc = ((int32_t)(s->gravity.tonic % 12u) + scale[i]) % 12;
            if (apc < 0) apc += 12;
            if (apc == pc) { allowed = 1u; break; }
        }
        if (allowed) {
            int32_t e = abs_i32(cand * 1000 - raw_milli);
            if (e < best_err) { best_err = e; best = cand; }
        }
    }
    return best;
}

static int32_t quantize_chord_milli(const ApsisState *s, uint8_t side, int32_t raw_milli)
{
    ApsisGravityState g;
    int16_t intervals[APSIS_GRAVITY_VOICES_MAX];
    uint8_t degree = (uint8_t)apsis_string_side_degree(s, side);
    uint8_t n, i;
    int32_t oct, best = 0, best_err = 0x7FFFFFFF;
    int32_t root = apsis_scale_at(s->gravity.tonic, s->gravity.scale, degree);

    g = s->gravity;
    g.degree = (uint8_t)(degree % 7u);
    n = apsis_gravity_ring_intervals(&g, intervals);
    for (oct = -4; oct <= 5; ++oct) {
        for (i = 0u; i < n; ++i) {
            int32_t cand = root + intervals[i] + oct * 12;
            int32_t e = abs_i32(cand * 1000 - raw_milli);
            if (e < best_err) { best_err = e; best = cand; }
        }
    }
    return best;
}

static int32_t round_step_milli(int32_t raw_milli)
{
    if (raw_milli >= 0) return (raw_milli + 500) / 1000;
    return -((-raw_milli + 499) / 1000);
}

int32_t apsis_string_target_step_milli(const ApsisState *s, uint8_t side,
                                       ApsisStringPitchMemory *m, uint8_t stable)
{
    int32_t raw, candidate, prev;
    uint8_t *valid;
    int16_t *target;
    if (!s) return 0;
    side = clamp_side(side);
    raw = apsis_string_raw_step_milli(s, side);
    if (!s->string.quantize) {
        if (m) {
            if (side == APSIS_SIDE_LEFT) m->left_valid = 0u;
            else m->right_valid = 0u;
        }
        return raw;
    }

    if (s->string.harmony == APSIS_HARMONY_CHROMATIC)
        candidate = round_step_milli(raw);
    else if (s->string.harmony == APSIS_HARMONY_CHORD)
        candidate = quantize_chord_milli(s, side, raw);
    else
        candidate = quantize_scale_milli(s, raw);

    if (!m) return candidate * 1000;
    valid = side == APSIS_SIDE_LEFT ? &m->left_valid : &m->right_valid;
    target = side == APSIS_SIDE_LEFT ? &m->left_target_step : &m->right_target_step;
    prev = *target;
    if (stable && *valid && candidate != prev &&
        abs_i32(raw - candidate * 1000) + APSIS_STRING_HYSTERESIS_MILLI >= abs_i32(raw - prev * 1000))
        candidate = prev;
    *target = (int16_t)candidate;
    *valid = 1u;
    return candidate * 1000;
}
