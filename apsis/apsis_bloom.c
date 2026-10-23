#include "apsis_bloom.h"
#include "apsis_harmony.h"

typedef struct {
    ApsisBloomPlan plan;
    int16_t steps[APSIS_BLOOM_VOICES_MAX];
    uint8_t inv;
    int32_t score;
} BloomCandidate;

static int32_t bloom_abs_i32(int32_t v) { return v < 0 ? -v : v; }
static int16_t bloom_clamp_i16(int32_t v, int16_t lo, int16_t hi) { return (int16_t)(v < lo ? lo : (v > hi ? hi : v)); }
static int8_t bloom_clamp_i8(int32_t v, int8_t lo, int8_t hi) { return (int8_t)(v < lo ? lo : (v > hi ? hi : v)); }
static uint8_t bloom_clamp_u8(uint32_t v, uint8_t lo, uint8_t hi) { return (uint8_t)(v < lo ? lo : (v > hi ? hi : v)); }

static uint16_t bloom_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static void bloom_sort_i16(int16_t *v, uint8_t n)
{
    uint8_t i;
    for (i = 1u; i < n; ++i) {
        int16_t x = v[i];
        uint8_t j = i;
        while (j && v[j - 1u] > x) { v[j] = v[j - 1u]; --j; }
        v[j] = x;
    }
}

void apsis_bloom_harmony_from_state(const ApsisState *s, ApsisBloomHarmony *h)
{
    if (!h) return;
    if (!s) {
        h->tonic = 0u; h->scale = APSIS_SCALE_DORIAN; h->degree = 0u;
        h->voicing = APSIS_GRAVITY_COLOUR; h->inversion = 0u; h->voices = 4u;
        return;
    }
    h->tonic = (uint8_t)(s->gravity.tonic % 12u);
    h->scale = s->gravity.scale < APSIS_SCALE_COUNT ? s->gravity.scale : APSIS_SCALE_DORIAN;
    h->degree = s->gravity.degree > 7u ? 7u : s->gravity.degree;
    h->voicing = s->gravity.voicing > 2u ? 2u : s->gravity.voicing;
    h->voices = bloom_clamp_u8(s->gravity.voices, 3u, 5u);
    h->inversion = s->gravity.inversion >= h->voices ? (uint8_t)(h->voices - 1u) : s->gravity.inversion;
}

uint8_t apsis_bloom_voice_count(const ApsisState *s, const ApsisBloomHarmony *h)
{
    if (!s) return h ? bloom_clamp_u8(h->voices, 3u, 5u) : 4u;
    if (s->bloom.depth == APSIS_BLOOM_DEPTH_TRIAD) return 3u;
    if (s->bloom.depth == APSIS_BLOOM_DEPTH_SEVENTH) return 4u;
    if (s->bloom.depth == APSIS_BLOOM_DEPTH_NINTH) return 5u;
    return h ? bloom_clamp_u8(h->voices, 3u, 5u) : bloom_clamp_u8(s->gravity.voices, 3u, 5u);
}

uint8_t apsis_bloom_base_chord_steps(const ApsisState *s, const ApsisBloomHarmony *h,
                                     int16_t out[APSIS_BLOOM_VOICES_MAX])
{
    static const int8_t profile[3][5] = {
        {0,2,4,7,11}, {0,2,4,6,8}, {4,8,9,10,13}
    };
    ApsisBloomHarmony local;
    uint8_t n, ring, i;
    if (!out) return 0u;
    if (!h) { apsis_bloom_harmony_from_state(s, &local); h = &local; }
    n = apsis_bloom_voice_count(s, h);
    ring = h->voicing > 2u ? 2u : h->voicing;
    for (i = 0u; i < n; ++i)
        out[i] = (int16_t)apsis_scale_at(h->tonic, h->scale,
                                         (int32_t)(h->degree > 7u ? 7u : h->degree) + profile[ring][i]);
    return n;
}

uint8_t apsis_bloom_plan_steps(const ApsisState *s, const ApsisBloomHarmony *h,
                               const ApsisBloomPlan *plan, int16_t out[APSIS_BLOOM_VOICES_MAX])
{
    int16_t base[APSIS_BLOOM_VOICES_MAX];
    uint8_t n, i, tone;
    if (!out) return 0u;
    n = apsis_bloom_base_chord_steps(s, h, base);
    if (!plan || plan->count == 0u) {
        for (i = 0u; i < n; ++i) out[i] = base[i];
        return n;
    }
    n = plan->count < n ? plan->count : n;
    for (i = 0u; i < n; ++i) {
        tone = (uint8_t)(plan->tone[i] % (n ? n : 1u));
        out[i] = (int16_t)(base[tone] + bloom_clamp_i8(plan->octave[i], -5, 6) * 12);
    }
    bloom_sort_i16(out, n);
    return n;
}

ApsisBloomPlan apsis_bloom_legacy_plan(const ApsisState *s, const ApsisBloomHarmony *h)
{
    ApsisBloomHarmony local;
    ApsisBloomPlan p = {0};
    uint8_t n, inv, v, i, tone;
    int8_t reg;
    if (!h) { apsis_bloom_harmony_from_state(s, &local); h = &local; }
    n = apsis_bloom_voice_count(s, h);
    inv = s && !s->bloom.inversion_follow ? s->bloom.inversion : h->inversion;
    if (inv >= n) inv = (uint8_t)(n - 1u);
    v = s && s->bloom.voicing_mode != APSIS_BLOOM_VOICING_FOLLOW
        ? (s->bloom.voicing_mode == APSIS_BLOOM_VOICING_CLOSE ? 0u :
           s->bloom.voicing_mode == APSIS_BLOOM_VOICING_OPEN ? 1u : 2u)
        : (h->voicing > 2u ? 2u : h->voicing);
    reg = s ? bloom_clamp_i8(s->bloom.register_oct, -1, 1) : 0;
    p.count = n;
    for (i = 0u; i < n; ++i) {
        tone = (uint8_t)((inv + i) % n);
        p.tone[i] = tone;
        p.octave[i] = (int8_t)((tone < inv ? 1 : 0) + reg);
    }
    if (v == 1u) {
        for (i = 2u; i < n; ++i) p.octave[i]++;
    } else if (v == 2u) {
        for (i = 0u; i < n; ++i) p.octave[i] = (int8_t)(p.octave[i] + (int8_t)i - 1);
    }
    return p;
}

uint16_t apsis_bloom_spread_target_q15(const ApsisState *s, const ApsisBloomHarmony *h)
{
    uint16_t x = s ? s->bloom.voicing_spread_q15 : APSIS_Q15_MILLI(480);
    uint8_t mode = s ? s->bloom.voicing_mode : APSIS_BLOOM_VOICING_FOLLOW;
    uint8_t ring = h ? (h->voicing > 2u ? 2u : h->voicing) : APSIS_GRAVITY_COLOUR;
    if (x > APSIS_Q15_ONE) x = APSIS_Q15_ONE;
    if (mode == APSIS_BLOOM_VOICING_FOLLOW) {
        if (ring == 0u) x = bloom_mul_q15(x, APSIS_Q15_MILLI(340));
        else if (ring == 1u) x = (uint16_t)(APSIS_Q15_MILLI(300) + bloom_mul_q15(x, APSIS_Q15_MILLI(480)));
        else x = (uint16_t)(APSIS_Q15_MILLI(680) + bloom_mul_q15(x, APSIS_Q15_MILLI(300)));
    } else if (mode == APSIS_BLOOM_VOICING_CLOSE) {
        x = bloom_mul_q15(x, APSIS_Q15_MILLI(280));
    } else if (mode == APSIS_BLOOM_VOICING_OPEN) {
        x = (uint16_t)(APSIS_Q15_MILLI(380) + bloom_mul_q15(x, APSIS_Q15_MILLI(380)));
    } else {
        x = (uint16_t)(APSIS_Q15_MILLI(720) + bloom_mul_q15(x, APSIS_Q15_MILLI(280)));
    }
    return x > APSIS_Q15_ONE ? APSIS_Q15_ONE : x;
}

void apsis_bloom_voicing_reset(ApsisBloomVoicingState *v)
{
    uint8_t i;
    if (!v) return;
    v->plan.count = 0u;
    v->last_count = 0u;
    v->drift_milli = 0;
    v->serial = 0u;
    for (i = 0u; i < APSIS_BLOOM_VOICES_MAX; ++i) {
        v->plan.tone[i] = 0u; v->plan.octave[i] = 0; v->last_steps[i] = 0;
    }
}

static uint8_t bloom_pitch_class(int16_t step)
{
    int16_t pc = (int16_t)(step % 12);
    if (pc < 0) pc += 12;
    return (uint8_t)pc;
}

static int32_t bloom_candidate_cost(const ApsisState *s, const ApsisBloomHarmony *h,
                                    const ApsisBloomVoicingState *vs, const BloomCandidate *c)
{
    int16_t base[APSIS_BLOOM_VOICES_MAX];
    uint8_t n = c->plan.count, i, pi, base_n;
    int16_t min = c->steps[0], max = c->steps[n - 1u];
    uint16_t spread = apsis_bloom_spread_target_q15(s, h);
    int32_t span_milli = (int32_t)(max - min) * 1000;
    int32_t target_span = 10000 + (int32_t)(((uint32_t)spread * 24000u + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
    int32_t center_milli = (int32_t)(min + max) * 500;
    int32_t base_center, desired, cost;
    int8_t reg = s ? bloom_clamp_i8(s->bloom.register_oct, -1, 1) : 0;
    uint8_t grav_inv = h ? (h->inversion >= n ? (uint8_t)(n - 1u) : h->inversion) : 0u;

    base_n = apsis_bloom_base_chord_steps(s, h, base);
    base_center = base_n ? (int32_t)(base[0] + base[base_n - 1u]) * 500 + reg * 12000 : 0;
    desired = base_center + (vs ? vs->drift_milli : 0);
    cost = (bloom_abs_i32(span_milli - target_span) * 420 + 500) / 1000;
    cost += (bloom_abs_i32(center_milli - desired) * 300 + 500) / 1000;

    if (vs && vs->last_count) {
        for (i = 0u; i < n; ++i) {
            uint8_t denom = n > 1u ? (uint8_t)(n - 1u) : 1u;
            pi = (uint8_t)(((uint32_t)i * (vs->last_count - 1u) + denom / 2u) / denom);
            {
                int32_t move = bloom_abs_i32((int32_t)c->steps[i] - vs->last_steps[pi]);
                cost += move * (move > 7 ? 1180 : 720);
                if (move > 12) cost += (move - 12) * 1800;
            }
        }
        for (i = 0u; i < n; ++i) {
            uint8_t j;
            for (j = 0u; j < vs->last_count; ++j) {
                if (bloom_pitch_class(c->steps[i]) == bloom_pitch_class(vs->last_steps[j])) { cost -= 1150; break; }
            }
        }
        if (vs->last_count == n) {
            uint8_t same = 1u;
            for (i = 0u; i < n; ++i) if (c->steps[i] != vs->last_steps[i]) { same = 0u; break; }
            if (same) cost += 9500 + (int32_t)(((uint32_t)spread * 4500u + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
        }
    }
    if (n) {
        uint8_t d = c->inv > grav_inv ? (uint8_t)(c->inv - grav_inv) : (uint8_t)(grav_inv - c->inv);
        uint8_t circular = (uint8_t)(n - d);
        if (circular < d) d = circular;
        cost += d * 550;
    }
    return cost;
}

static void bloom_top_insert(BloomCandidate top[5], uint8_t *count, const BloomCandidate *candidate)
{
    uint8_t i, pos;
    if (*count < 5u) {
        pos = (*count)++;
        top[pos] = *candidate;
    } else {
        if (candidate->score >= top[4].score) return;
        pos = 4u;
        top[pos] = *candidate;
    }
    while (pos && top[pos].score < top[pos - 1u].score) {
        BloomCandidate tmp = top[pos - 1u];
        top[pos - 1u] = top[pos];
        top[pos] = tmp;
        --pos;
    }
    for (i = *count; i < 5u; ++i) top[i].score = 0x7FFFFFFF;
}

static void bloom_advance_drift(const ApsisState *s, const ApsisBloomHarmony *h,
                                ApsisBloomVoicingState *vs, uint32_t random)
{
    uint16_t spread = apsis_bloom_spread_target_q15(s, h);
    int32_t amplitude = 1250 + (int32_t)(((uint32_t)spread * 2100u + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
    int64_t centered = (int64_t)random * 2 - ((int64_t)1 << 32);
    int32_t step;
    if (centered >= 0) step = (int32_t)(((uint64_t)centered * (uint32_t)amplitude) >> 32);
    else step = -(int32_t)(((uint64_t)(-centered) * (uint32_t)amplitude) >> 32);
    vs->drift_milli = bloom_clamp_i16((int32_t)vs->drift_milli - vs->drift_milli / 10 + step, -11000, 11000);
}

ApsisBloomPlan apsis_bloom_choose_voicing(const ApsisState *s, const ApsisBloomHarmony *h,
                                          ApsisBloomVoicingState *vs, uint8_t advance,
                                          uint32_t walk_random, uint32_t pick_random)
{
    static const int8_t extras[6][5] = {
        {0,0,0,0,0}, {0,0,0,1,1}, {0,0,1,1,1},
        {0,0,1,1,2}, {0,1,1,2,2}, {0,1,2,2,3}
    };
    ApsisBloomHarmony local;
    BloomCandidate top[5];
    ApsisBloomPlan fallback;
    uint8_t top_count = 0u, n, inv, pattern, whole_ix, i, pool_count, pick;
    int8_t reg;
    if (!h) { apsis_bloom_harmony_from_state(s, &local); h = &local; }
    fallback = apsis_bloom_legacy_plan(s, h);
    if (!s || !vs) return fallback;
    n = apsis_bloom_voice_count(s, h);
    reg = bloom_clamp_i8(s->bloom.register_oct, -1, 1);
    if (advance) bloom_advance_drift(s, h, vs, walk_random);

    for (inv = 0u; inv < n; ++inv) {
        for (pattern = 0u; pattern < 6u; ++pattern) {
            for (whole_ix = 0u; whole_ix < 3u; ++whole_ix) {
                BloomCandidate c;
                int8_t whole = (int8_t)whole_ix - 1;
                int16_t min, max;
                c.plan.count = n;
                c.inv = inv;
                for (i = 0u; i < n; ++i) {
                    uint8_t tone = (uint8_t)((inv + i) % n);
                    c.plan.tone[i] = tone;
                    c.plan.octave[i] = (int8_t)((tone < inv ? 1 : 0) + extras[pattern][i] + reg + whole);
                }
                apsis_bloom_plan_steps(s, h, &c.plan, c.steps);
                min = c.steps[0]; max = c.steps[n - 1u];
                if ((max - min) < 7 || (max - min) > 43) continue;
                c.score = bloom_candidate_cost(s, h, vs, &c);
                bloom_top_insert(top, &top_count, &c);
            }
        }
    }
    if (!top_count) return fallback;
    pool_count = 0u;
    for (i = 0u; i < top_count; ++i) {
        if (top[i].score <= top[0].score + 2400) ++pool_count;
        else break;
    }
    pick = 0u;
    if (advance && pool_count > 1u)
        pick = (uint8_t)(((uint64_t)pick_random * pool_count) >> 32);

    if (advance) {
        vs->plan = top[pick].plan;
        vs->last_count = n;
        for (i = 0u; i < n; ++i) vs->last_steps[i] = top[pick].steps[i];
        vs->serial++;
    }
    return top[pick].plan;
}

uint8_t apsis_bloom_chord_steps(const ApsisState *s, const ApsisBloomHarmony *h,
                                const ApsisBloomVoicingState *vs,
                                int16_t out[APSIS_BLOOM_VOICES_MAX])
{
    ApsisBloomPlan legacy;
    uint8_t n = apsis_bloom_voice_count(s, h);
    if (vs && vs->plan.count == n)
        return apsis_bloom_plan_steps(s, h, &vs->plan, out);
    legacy = apsis_bloom_legacy_plan(s, h);
    return apsis_bloom_plan_steps(s, h, &legacy, out);
}
