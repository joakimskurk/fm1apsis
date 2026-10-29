#include "apsis_late_binding.h"
#include "apsis_harmony.h"
#include "apsis_spiral.h"

static void late_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t late_tuning_mode(uint8_t mode)
{
    return mode <= APSIS_TUNING_PYTH ? mode : APSIS_TUNING_EQUAL;
}

static uint16_t late_q15_mul(uint16_t a, uint16_t b)
{
    uint32_t x = (uint32_t)a * b + APSIS_Q15_ONE / 2u;
    return (uint16_t)(x / APSIS_Q15_ONE);
}

static uint8_t late_pc(int32_t step)
{
    int32_t pc = step % 12;
    if (pc < 0) pc += 12;
    return (uint8_t)pc;
}

static uint8_t late_bloom_tuning_mode(const ApsisState *s)
{
    if (!s || s->bloom.tuning_mode == APSIS_BLOOM_TUNING_FOLLOW)
        return s ? late_tuning_mode(s->temper.mode) : APSIS_TUNING_EQUAL;
    if (s->bloom.tuning_mode == APSIS_BLOOM_TUNING_JUST) return APSIS_TUNING_JUST;
    if (s->bloom.tuning_mode == APSIS_BLOOM_TUNING_PYTH) return APSIS_TUNING_PYTH;
    return APSIS_TUNING_EQUAL;
}

static uint16_t late_bloom_tuning_strength(const ApsisState *s)
{
    uint16_t amount, base;
    if (!s) return APSIS_Q15_ONE;
    amount = s->bloom.tuning_amount_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->bloom.tuning_amount_q15;
    base = s->bloom.tuning_mode == APSIS_BLOOM_TUNING_FOLLOW
        ? (s->temper.strength_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->temper.strength_q15)
        : APSIS_Q15_ONE;
    return late_q15_mul(base, amount);
}

ApsisBloomHarmony apsis_late_bloom_harmony_at(const ApsisState *s,
                                               const ApsisLatePendingGravity *pending,
                                               uint64_t target_sample,
                                               uint32_t sample_rate)
{
    ApsisBloomHarmony h;
    uint64_t tolerance;
    apsis_bloom_harmony_from_state(s, &h);
    if (!s || !pending || !pending->active) return h;
    if (!sample_rate) sample_rate = 44100u;
    tolerance = sample_rate / 2000u; /* Study 126: +0.0005 seconds. */
    if (pending->target_sample <= target_sample + tolerance) {
        h.degree = pending->degree > 7u ? 7u : pending->degree;
        h.voicing = pending->ring > 2u ? 2u : pending->ring;
    }
    return h;
}

uint8_t apsis_late_spiral_resolve(const ApsisState *s,
                                  uint8_t frozen_route_index,
                                  ApsisSpiralLateBinding *out)
{
    int16_t seq[APSIS_SPIRAL_ROUTE_MAX];
    int32_t step;
    uint8_t n, ix, mode, pc;
    uint16_t strength;
    if (!out) return 0u;
    late_zero(out, (uint32_t)sizeof(*out));
    if (!s) return 0u;
    n = apsis_spiral_derived_sequence(s, seq);
    if (!n) return 0u;
    ix = (uint8_t)(frozen_route_index % n);
    step = (int32_t)seq[ix] + (int32_t)s->spiral.register_oct * 12;
    step = apsis_quantize_step_to_scale(step, s->gravity.tonic, s->gravity.scale,
                                        s->gravity.scale_lock ? 1u : 0u);
    mode = late_tuning_mode(s->temper.mode);
    strength = s->temper.strength_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->temper.strength_q15;
    pc = late_pc(step);

    out->valid = 1u;
    out->route_index = ix;
    out->step = (int16_t)step;
    out->tuning_mode = mode;
    out->tuning_strength_q15 = strength;
    out->tuning_deviation_milli = apsis_tuning_deviation_milli(
        pc, s->gravity.tonic, mode, strength);
    return 1u;
}

uint8_t apsis_late_bloom_resolve(const ApsisState *s,
                                 const ApsisBloomPlan *frozen_plan,
                                 uint8_t voice_index,
                                 const ApsisLatePendingGravity *pending,
                                 uint64_t voice_target_sample,
                                 uint32_t sample_rate,
                                 ApsisBloomLateBinding *out)
{
    ApsisBloomHarmony h;
    int16_t steps[APSIS_BLOOM_VOICES_MAX];
    int32_t root_step;
    uint8_t n, i, mode, pc, ref_pc;
    uint16_t strength;
    if (!out) return 0u;
    late_zero(out, (uint32_t)sizeof(*out));
    if (!s || !frozen_plan || !frozen_plan->count) return 0u;

    h = apsis_late_bloom_harmony_at(s, pending, voice_target_sample, sample_rate);
    n = apsis_bloom_plan_steps(s, &h, frozen_plan, steps);
    if (!n) return 0u;
    i = voice_index >= n ? (uint8_t)(n - 1u) : voice_index;
    mode = late_bloom_tuning_mode(s);
    strength = late_bloom_tuning_strength(s);
    pc = late_pc(steps[i]);
    if (s->bloom.tuning_reference == APSIS_BLOOM_REF_TONIC) {
        ref_pc = (uint8_t)(h.tonic % 12u);
    } else {
        root_step = apsis_scale_at(h.tonic, h.scale, h.degree > 7u ? 7u : h.degree);
        ref_pc = late_pc(root_step);
    }

    out->valid = 1u;
    out->voice_index = i;
    out->plan_octave = i < frozen_plan->count ? frozen_plan->octave[i] : 0;
    out->step = steps[i];
    out->tuning_mode = mode;
    out->tuning_strength_q15 = strength;
    out->tuning_deviation_milli = apsis_tuning_deviation_milli(pc, ref_pc, mode, strength);
    out->harmony = h;
    return 1u;
}
