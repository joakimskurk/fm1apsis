#include "apsis_spiral_audio.h"
#include "apsis_drift.h"

#define SPIRAL_Q16_ONE 65536u

static void spiral_audio_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint16_t spiral_audio_clamp_q15(uint16_t v, uint16_t hi)
{
    return v > hi ? hi : v;
}

static uint32_t spiral_audio_mul_q15_u32(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint32_t spiral_audio_round_q16(uint64_t q16)
{
    return (uint32_t)((q16 + (SPIRAL_Q16_ONE / 2u)) >> 16);
}

static uint32_t spiral_audio_ms_samples(uint32_t ms)
{
    return (APSIS_SPIRAL_AUDIO_RATE * ms + 500u) / 1000u;
}

static uint8_t spiral_audio_rate(uint8_t rate)
{
    return rate == 1u || rate == 2u || rate == 4u ? rate : 2u;
}

static uint8_t spiral_audio_voice(uint8_t voice)
{
    return voice <= APSIS_SPIRAL_HALO ? voice : APSIS_SPIRAL_THREAD;
}

void apsis_spiral_audio_plan(const ApsisState *s,
                             uint16_t energy_q15,
                             uint64_t interval_q32,
                             uint8_t attractor_rate,
                             ApsisSpiralAcousticPlan *out)
{
    uint64_t sub_q16, raw_q16, dur_q16;
    uint32_t factor_q15, amp_q15, level, gain;
    uint16_t energy, note_length, width, pan;
    uint16_t delta;
    uint8_t rate;

    if (!out) return;
    spiral_audio_zero(out, (uint32_t)sizeof(*out));
    if (!s || !interval_q32 || !s->spiral.sound) return;

    rate = spiral_audio_rate(attractor_rate);
    energy = energy_q15;
    if (energy < APSIS_Q15_MILLI(120)) energy = APSIS_Q15_MILLI(120);
    if (energy > APSIS_Q15_ONE) energy = APSIS_Q15_ONE;
    note_length = s->spiral.note_length_q15;
    if (note_length < APSIS_Q15_MILLI(180)) note_length = APSIS_Q15_MILLI(180);
    if (note_length > APSIS_Q15_MILLI(1350)) note_length = APSIS_Q15_MILLI(1350);

    /* Work in sample-Q16 here. It preserves far more than sample precision and
     * avoids overflowing uint64_t at the legal slowest Clock / 1.35x length. */
    sub_q16 = (interval_q32 + 0x8000u) >> 16;
    if (rate == 2u) sub_q16 >>= 1u;
    else if (rate == 4u) sub_q16 >>= 2u;

    factor_q15 = APSIS_Q15_MILLI(480) +
        spiral_audio_mul_q15_u32(APSIS_Q15_MILLI(360), energy);
    raw_q16 = (sub_q16 * factor_q15 + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    if (raw_q16 < ((uint64_t)spiral_audio_ms_samples(110u) << 16))
        raw_q16 = (uint64_t)spiral_audio_ms_samples(110u) << 16;
    dur_q16 = (raw_q16 * note_length + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;

    level = s->spiral.level_q15;
    if (level > APSIS_Q15_MILLI(1250)) level = APSIS_Q15_MILLI(1250);
    amp_q15 = APSIS_Q15_MILLI(30) +
        spiral_audio_mul_q15_u32(APSIS_Q15_MILLI(36), energy);
    gain = spiral_audio_mul_q15_u32(amp_q15, level);

    delta = apsis_drift_audible_delta_millihz(&s->drift);
    width = spiral_audio_clamp_q15(s->drift.width_q15, APSIS_Q15_ONE);
    pan = (uint16_t)spiral_audio_mul_q15_u32(width, APSIS_Q15_MILLI(340));

    out->valid = 1u;
    out->voice = spiral_audio_voice(s->spiral.voice);
    out->energy_q15 = energy;
    out->drift_delta_millihz = delta;
    out->width_q15 = width;
    out->duration_samples = spiral_audio_round_q16(dur_q16);

    if (delta > 1u) {
        out->leg_count = 2u;

        out->leg[0].start_offset_samples = 0u;
        out->leg[0].duration_samples = out->duration_samples;
        out->leg[0].gain_q15 = (uint16_t)gain;
        out->leg[0].pan_q15 = -(int16_t)pan;
        out->leg[0].drift_sign = -1;
        out->leg[0].shadow = 0u;

        out->leg[1].start_offset_samples = spiral_audio_ms_samples(8u);
        out->leg[1].duration_samples =
            (uint32_t)(((uint64_t)out->duration_samples * APSIS_Q15_MILLI(880) +
                        APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
        out->leg[1].gain_q15 = (uint16_t)spiral_audio_mul_q15_u32(
            gain, APSIS_Q15_MILLI(580));
        out->leg[1].pan_q15 = (int16_t)pan;
        out->leg[1].drift_sign = 1;
        out->leg[1].shadow = 1u;
    } else {
        out->leg_count = 1u;
        out->leg[0].start_offset_samples = 0u;
        out->leg[0].duration_samples = out->duration_samples;
        out->leg[0].gain_q15 = (uint16_t)spiral_audio_mul_q15_u32(
            gain, APSIS_Q15_MILLI(1080));
        out->leg[0].pan_q15 = 0;
        out->leg[0].drift_sign = 0;
        out->leg[0].shadow = 0u;
    }
}
