#include "apsis_string_rhythm_audio.h"
#include "apsis_harmony.h"

static uint16_t rhythm_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint16_t rhythm_level(const ApsisState *s)
{
    uint16_t rhythm;
    uint16_t level;
    if (!s) return 0u;
    rhythm = s->string.rhythm_level_q15 > APSIS_Q15_ONE
        ? APSIS_Q15_ONE : s->string.rhythm_level_q15;
    level = rhythm_mul_q15(APSIS_Q15_MILLI(52), s->string.level_q15);
    return rhythm_mul_q15(level, rhythm);
}

static uint16_t rhythm_brightness_add(uint16_t value, int32_t milli)
{
    int32_t v = (int32_t)value;
    int32_t delta = (int32_t)APSIS_Q15_MILLI(milli < 0 ? -milli : milli);
    v += milli < 0 ? -delta : delta;
    if (v < (int32_t)APSIS_Q15_MILLI(150)) v = APSIS_Q15_MILLI(150);
    if (v > (int32_t)APSIS_Q15_MILLI(880)) v = APSIS_Q15_MILLI(880);
    return (uint16_t)v;
}

static uint32_t rhythm_side_frequency(const ApsisState *s,
                                      ApsisStringPitchMemory *memory,
                                      uint8_t side)
{
    int32_t step = apsis_string_target_step_milli(s, side, memory, 1u);
    return apsis_string_dsp_frequency_millihz(s, step);
}

static uint8_t rhythm_body(ApsisStringTransientPool *pool,
                           const ApsisState *s,
                           uint32_t freq,
                           uint16_t level,
                           uint16_t brightness,
                           uint32_t duration_ms,
                           int16_t pan_q15,
                           uint32_t attack_ms,
                           uint32_t start_offset_samples)
{
    return apsis_string_dsp_trigger_body(pool, s, freq, level, duration_ms,
                                         pan_q15, brightness, attack_ms,
                                         start_offset_samples);
}

static uint8_t rhythm_wire(ApsisStringTransientPool *pool,
                           const ApsisState *s,
                           uint32_t freq,
                           uint16_t level,
                           uint16_t brightness,
                           uint32_t start_offset_samples)
{
    ApsisState local;
    uint16_t root_gain;
    local = *s;
    local.string.brightness_q15 = brightness;
    root_gain = s->string.foundation ? APSIS_Q15_MILLI(50) : 0u;
    /* stringSideTransient(right) applies another .92 before classicStringNote. */
    level = rhythm_mul_q15(level, APSIS_Q15_MILLI(920));
    return apsis_string_dsp_trigger_classic_offset(pool, &local, freq, level,
                                                    660u,
                                                    (int16_t)APSIS_Q15_MILLI(300),
                                                    1u, root_gain,
                                                    start_offset_samples);
}

static uint8_t rhythm_classic_composite(ApsisStringTransientPool *pool,
                                        const ApsisState *s,
                                        ApsisStringPitchMemory *memory,
                                        uint16_t level)
{
    uint32_t freq = rhythm_side_frequency(s, memory, APSIS_SIDE_RIGHT);
    uint16_t body_level = rhythm_mul_q15(level, APSIS_Q15_MILLI(660));
    uint16_t wire_level = rhythm_mul_q15(level, APSIS_Q15_MILLI(780));
    uint16_t body_brightness = rhythm_brightness_add(s->string.brightness_q15, -100);
    uint16_t wire_brightness = rhythm_brightness_add(s->string.brightness_q15, 180);
    uint8_t accepted = 0u;
    accepted = (uint8_t)(accepted + rhythm_body(pool, s, freq, body_level,
                                                body_brightness, 1180u,
                                                (int16_t)-APSIS_Q15_MILLI(300),
                                                48u, 0u));
    accepted = (uint8_t)(accepted + rhythm_wire(pool, s, freq, wire_level,
                                                wire_brightness, 176u));
    return accepted;
}

static uint8_t rhythm_dual_side(ApsisStringTransientPool *pool,
                                const ApsisState *s,
                                ApsisStringPitchMemory *memory,
                                const ApsisTransportEvent *event,
                                uint8_t mode,
                                uint16_t level)
{
    uint8_t side = (event->absolute_pulse & 1u) ? APSIS_SIDE_RIGHT : APSIS_SIDE_LEFT;
    uint16_t side_level = side == APSIS_SIDE_LEFT
        ? s->string.left_level_q15 : s->string.right_level_q15;
    uint16_t mode_scale = mode == APSIS_STRING_RHYTHM_PIZZ
        ? APSIS_Q15_MILLI(1050) : APSIS_Q15_MILLI(900);
    uint16_t voice_level = rhythm_mul_q15(rhythm_mul_q15(level, mode_scale), side_level);
    uint32_t freq = rhythm_side_frequency(s, memory, side);
    if (side == APSIS_SIDE_LEFT) {
        return rhythm_body(pool, s, freq, voice_level,
                           rhythm_brightness_add(s->string.brightness_q15, -100),
                           1180u, (int16_t)-APSIS_Q15_MILLI(300), 48u, 0u);
    }
    return rhythm_wire(pool, s, freq, voice_level,
                       rhythm_brightness_add(s->string.brightness_q15, 180), 0u);
}

static uint8_t rhythm_motion(ApsisStringTransientPool *pool,
                             const ApsisState *s,
                             const ApsisTransportEvent *event,
                             uint16_t level)
{
    int16_t chord[APSIS_GRAVITY_VOICES_MAX];
    uint8_t n = apsis_gravity_chord_steps(&s->gravity, chord);
    uint32_t pulse = event->absolute_pulse;
    int32_t root = apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                                  s->gravity.degree > 7u ? 7 : s->gravity.degree);
    int32_t step = n ? chord[pulse % n] : root;
    int32_t pan_milli = ((int32_t)(pulse % 3u) - 1) * 140;
    uint32_t freq;
    while (step > root + 16) step -= 12;
    while (step < root - 7) step += 12;
    freq = apsis_string_dsp_frequency_millihz(s, step * 1000);
    level = rhythm_mul_q15(level, APSIS_Q15_MILLI(880));
    return rhythm_body(pool, s, freq, level, s->string.brightness_q15,
                       700u, (int16_t)(pan_milli < 0
                           ? -(int32_t)APSIS_Q15_MILLI((uint32_t)(-pan_milli))
                           : (int32_t)APSIS_Q15_MILLI((uint32_t)pan_milli)),
                       46u, 0u);
}

uint8_t apsis_string_rhythm_trigger_event(ApsisStringTransientPool *pool,
                                          const ApsisState *s,
                                          ApsisStringPitchMemory *pitch_memory,
                                          const ApsisTransportEvent *event)
{
    uint16_t level;
    uint8_t mode;
    if (!pool || !s || !event || event->type != APSIS_EVENT_STRING_RHYTHM)
        return 0u;
    /* scheduleStringRhythmBound() re-checks sourceOn at fire time. It does not
     * re-check the current rhythm mode; that argument was frozen when planned. */
    if (!s->string.sound || !s->string.level_q15) return 0u;
    level = rhythm_level(s);
    if (!level) return 0u;
    mode = event->sub_index;
    if (mode < APSIS_STRING_RHYTHM_BOW || mode > APSIS_STRING_RHYTHM_MOTION)
        mode = APSIS_STRING_RHYTHM_BOW;

    if (s->string.surface_mode == APSIS_STRING_CLASSIC)
        return rhythm_classic_composite(pool, s, pitch_memory, level);
    if (mode == APSIS_STRING_RHYTHM_MOTION)
        return rhythm_motion(pool, s, event, level);
    return rhythm_dual_side(pool, s, pitch_memory, event, mode, level);
}
