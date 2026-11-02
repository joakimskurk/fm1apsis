#include "apsis_string98.h"
#include "apsis_harmony.h"

static uint16_t s98_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static int16_t s98_signed_milli(int32_t milli)
{
    return milli < 0 ? (int16_t)-(int32_t)APSIS_Q15_MILLI((uint32_t)-milli)
                     : (int16_t)APSIS_Q15_MILLI((uint32_t)milli);
}

/* sourceLevel('string') clamps to 1.25; the Q15 state never exceeds 1, so the level is the plain product */
static uint16_t s98_level(const ApsisState *s, uint16_t base_q15)
{
    if (!s || !s->string.sound) return 0u;
    return s98_mul_q15(base_q15, s->string.level_q15);
}

static uint16_t s98_root_gain(const ApsisState *s, uint32_t milli)
{
    return s->string.foundation ? APSIS_Q15_MILLI(milli) : 0u;
}

uint32_t apsis_string98_tension_millihz(const ApsisState *s, ApsisStringPitchMemory *memory)
{
    int32_t step = apsis_string_target_step_milli(s, APSIS_SIDE_RIGHT, memory, 1u);
    return apsis_string_dsp_frequency_millihz(s, step);
}

uint8_t apsis_string98_pluck(ApsisStringTransientPool *pool, const ApsisState *s,
                             ApsisStringPitchMemory *memory, uint64_t now_sample)
{
    uint16_t level;
    int32_t pan_milli;
    if (!pool || !s || !memory) return 0u;
    level = s98_level(s, APSIS_STRING98_PLUCK_LEVEL_Q15);
    if (!level) return 0u;
    if (pool->pluck_seen && now_sample - pool->last_pluck_sample < APSIS_STRING98_PLUCK_GUARD_SAMPLES)
        return 0u;
    pool->pluck_seen = 1u;
    pool->last_pluck_sample = now_sample;
    /* pan = (position - .5) * 1.24, within +-.62; position: where the string is held */
    pan_milli = ((int32_t)s->string.right_split_q15 * 1000 + (int32_t)APSIS_Q15_ONE / 2) / (int32_t)APSIS_Q15_ONE;
    pan_milli = (pan_milli - 500) * 124 / 100;
    if (pan_milli > 620) pan_milli = 620;
    if (pan_milli < -620) pan_milli = -620;
    return apsis_string_dsp_trigger_classic(pool, s, apsis_string98_tension_millihz(s, memory), level, 1050u,
                                            s98_signed_milli(pan_milli), 0u, s98_root_gain(s, 280u));
}

uint8_t apsis_string98_rhythm(ApsisStringTransientPool *pool, const ApsisState *s,
                              ApsisStringPitchMemory *memory, const ApsisTransportEvent *event)
{
    uint16_t level, rhythm;
    uint8_t mode;
    if (!pool || !s || !memory || !event || event->type != APSIS_EVENT_STRING_RHYTHM) return 0u;
    /* lvl = .052 * sourceLevel * rhythmLevel; the mode was frozen when the event was planned */
    rhythm = s->string.rhythm_level_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->string.rhythm_level_q15;
    level = s98_mul_q15(s98_level(s, APSIS_Q15_MILLI(52)), rhythm);
    if (!level) return 0u;
    mode = event->sub_index;
    if (mode == APSIS_STRING_RHYTHM_MOTION) {
        int16_t chord[APSIS_GRAVITY_VOICES_MAX];
        uint8_t n = apsis_gravity_chord_steps(&s->gravity, chord);
        uint32_t pulse = event->absolute_pulse;
        int32_t root = apsis_scale_at(s->gravity.tonic, s->gravity.scale, s->gravity.degree > 7u ? 7 : s->gravity.degree);
        int32_t step = n ? chord[pulse % n] : root;
        while (step > root + 16) step -= 12;
        while (step < root - 7) step += 12;
        return apsis_string_dsp_trigger_classic(pool, s, apsis_string_dsp_frequency_millihz(s, step * 1000),
                                                s98_mul_q15(level, APSIS_Q15_MILLI(920)), 720u,
                                                s98_signed_milli(((int32_t)(pulse % 3u) - 1) * 140), 0u, 0u);
    }
    if (mode == APSIS_STRING_RHYTHM_PIZZ) {
        uint32_t lvl = (uint32_t)level * 115u / 100u;   /* lvl * 1.15 */
        return apsis_string_dsp_trigger_classic(pool, s, apsis_string98_tension_millihz(s, memory),
                                                (uint16_t)(lvl > APSIS_Q15_ONE ? APSIS_Q15_ONE : lvl), 340u, 0, 1u,
                                                s98_root_gain(s, 180u));
    }
    /* bow: stringSolo(attack .055, release .72); CLASSIC plays it as a plain classicStringNote of .70 s */
    return apsis_string_dsp_trigger_classic(pool, s, apsis_string98_tension_millihz(s, memory), level, 700u, 0, 0u,
                                            0u);
}

uint8_t apsis_string98_strum(ApsisStringTransientPool *pool, const ApsisState *s,
                             ApsisStringPitchMemory *memory, const ApsisStringAttractorBinding *binding,
                             uint64_t render_origin_sample)
{
    uint16_t norm, amount, user, level;
    uint32_t base_milli, start, dur_ms;
    if (!pool || !s || !memory || !binding || !binding->valid) return 0u;
    amount = s->string.filter_attractor_strum_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->string.filter_attractor_strum_q15;
    user = s->string.filter_attractor_strum_level_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE
                                                                       : s->string.filter_attractor_strum_level_q15;
    if (!amount) return 0u;
    norm = binding->value_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : binding->value_q15;
    /* level = (.055 + .105 * norm) * amount * userLevel * sourceLevel */
    base_milli = 55u + (105u * norm + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    level = s98_level(s, s98_mul_q15(s98_mul_q15(APSIS_Q15_MILLI(base_milli), amount), user));
    if (!level) return 0u;
    /* dur = .26 + .34 * (1 - norm * .25) */
    dur_ms = 600u - (85u * norm + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    start = binding->target_sample <= render_origin_sample ? 0u
          : (binding->target_sample - render_origin_sample > 0xffffffffu
                 ? 0xffffffffu : (uint32_t)(binding->target_sample - render_origin_sample));
    return apsis_string_dsp_trigger_classic_offset(pool, s, apsis_string98_tension_millihz(s, memory), level,
                                                   dur_ms, s98_signed_milli(((int32_t)(binding->pos % 5u) - 2) * 45),
                                                   1u, s98_root_gain(s, 120u), start);
}
