#include "apsis_persistent_audio.h"

static void pa_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint32_t pa_string_phase_inc(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint32_t pa_drift_phase_inc(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint16_t pa_string_alpha_for_ms(uint32_t ms)
{
    uint32_t samples, a;
    if (ms < 1u) ms = 1u;
    samples = (APSIS_STRING_DSP_RATE * ms + 500u) / 1000u;
    if (!samples) samples = 1u;
    if (samples <= 1u) return APSIS_Q15_ONE;
    a = (APSIS_Q15_ONE + samples / 2u) / samples;
    return (uint16_t)(a ? a : 1u);
}

static void pa_string_apply_timbre(ApsisStringDspVoice *v)
{
    v->waveform = v->desired_waveform;
    v->lowpass_alpha_q15 = v->desired_lowpass_alpha_q15;
    v->mix_left_q15 = v->desired_mix_left_q15;
    v->mix_right_q15 = v->desired_mix_right_q15;
    v->lowpass_q15 = 0;
    v->target_gain_q15 = v->desired_gain_q15;
    v->timbre_transition = 0u;
}

void apsis_persistent_audio_capture(const ApsisState *state,
                                    ApsisStringPitchMemory *pitch_memory,
                                    uint8_t transport_running,
                                    ApsisPersistentAudioTargets *out)
{
    if (!out) return;
    pa_zero(out, (uint32_t)sizeof(*out));
    if (!state) return;
    apsis_string_dsp_targets(state, pitch_memory, transport_running,
                             &out->string_targets);
    apsis_drift_dsp_targets(state, transport_running, &out->drift_targets);
    out->string_glide_q15 = state->string.glide_q15;
    out->string_surface_mode = state->string.surface_mode;
    out->transport_running = transport_running ? 1u : 0u;
}

void apsis_persistent_audio_apply(ApsisStringDsp *string_dsp,
                                  ApsisDriftDsp *drift_dsp,
                                  const ApsisPersistentAudioTargets *targets)
{
    uint32_t glide_ms, wire_ms;
    uint8_t i;
    if (!targets) return;

    if (string_dsp) {
        glide_ms = ((uint32_t)targets->string_glide_q15 * 1000u +
                    APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
        if (glide_ms < 25u) glide_ms = 25u;
        if (glide_ms > 800u) glide_ms = 800u;
        wire_ms = glide_ms > 45u ? 45u : glide_ms;

        string_dsp->transport_running = targets->transport_running ? 1u : 0u;
        string_dsp->voice[0].pitch_alpha_q15 = pa_string_alpha_for_ms(12u);
        string_dsp->voice[1].pitch_alpha_q15 = pa_string_alpha_for_ms(12u);
        string_dsp->voice[2].pitch_alpha_q15 = pa_string_alpha_for_ms(glide_ms);
        string_dsp->voice[3].pitch_alpha_q15 = pa_string_alpha_for_ms(
            targets->string_surface_mode == APSIS_STRING_CLASSIC ? glide_ms : wire_ms);

        for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
            ApsisStringDspVoice *v = &string_dsp->voice[i];
            const ApsisStringDspTargets *t = &targets->string_targets;
            uint8_t metadata_changed;
            v->target_phase_inc = pa_string_phase_inc(t->freq_millihz[i]);
            v->desired_gain_q15 = t->gain_q15[i];
            v->desired_waveform = t->waveform[i];
            v->desired_lowpass_alpha_q15 = t->lowpass_alpha_q15[i];
            v->desired_mix_left_q15 = t->mix_left_q15[i];
            v->desired_mix_right_q15 = t->mix_right_q15[i];
            if (!v->pitch_initialized && v->target_phase_inc) {
                v->phase_inc = v->target_phase_inc;
                v->pitch_initialized = 1u;
            }
            metadata_changed = (uint8_t)(
                v->waveform != v->desired_waveform ||
                v->lowpass_alpha_q15 != v->desired_lowpass_alpha_q15 ||
                v->mix_left_q15 != v->desired_mix_left_q15 ||
                v->mix_right_q15 != v->desired_mix_right_q15);
            if (metadata_changed) {
                if (v->gain_q15 <= 1) pa_string_apply_timbre(v);
                else {
                    v->target_gain_q15 = 0;
                    v->timbre_transition = 1u;
                }
            } else if (!v->timbre_transition) {
                v->target_gain_q15 = v->desired_gain_q15;
            }
        }
    }

    if (drift_dsp) {
        drift_dsp->transport_running = targets->transport_running ? 1u : 0u;
        for (i = 0u; i < APSIS_DRIFT_DSP_VOICES; ++i) {
            ApsisDriftDspVoice *v = &drift_dsp->voice[i];
            const ApsisDriftDspTargets *t = &targets->drift_targets;
            uint32_t inc = pa_drift_phase_inc(t->freq_millihz[i]);
            if (!v->pitch_initialized) {
                v->phase_inc = inc;
                v->mix_left_q15 = t->mix_left_q15[i];
                v->mix_right_q15 = t->mix_right_q15[i];
                v->pitch_initialized = 1u;
            }
            v->target_phase_inc = inc;
            v->target_gain_q23 = (int32_t)t->gain_q15[i] << 8;
            v->target_mix_left_q15 = t->mix_left_q15[i];
            v->target_mix_right_q15 = t->mix_right_q15[i];
        }
    }
}

void apsis_persistent_audio_queue_reset(ApsisPersistentAudioQueue *queue)
{
    if (!queue) return;
    pa_zero(queue, (uint32_t)sizeof(*queue));
}

uint8_t apsis_persistent_audio_queue_push(ApsisPersistentAudioQueue *queue,
                                          uint64_t sample,
                                          const ApsisPersistentAudioTargets *targets)
{
    ApsisPersistentAudioEvent e;
    uint8_t pos;
    if (!queue || !targets) return 0u;
    if (queue->count >= APSIS_PERSISTENT_AUDIO_QUEUE_CAP) {
        queue->overflowed = 1u;
        return 0u;
    }
    e.sample = sample;
    e.serial = queue->next_serial++;
    e.targets = *targets;
    pos = queue->count++;
    while (pos) {
        const ApsisPersistentAudioEvent *prev = &queue->event[pos - 1u];
        if (prev->sample < e.sample ||
            (prev->sample == e.sample && prev->serial < e.serial)) break;
        queue->event[pos] = queue->event[pos - 1u];
        --pos;
    }
    queue->event[pos] = e;
    if (queue->count > queue->high_water) queue->high_water = queue->count;
    return 1u;
}

const ApsisPersistentAudioEvent *apsis_persistent_audio_queue_peek(
                                          const ApsisPersistentAudioQueue *queue)
{
    if (!queue || !queue->count) return 0;
    return &queue->event[0];
}

uint8_t apsis_persistent_audio_queue_pop(ApsisPersistentAudioQueue *queue,
                                         ApsisPersistentAudioEvent *out)
{
    uint8_t i;
    if (!queue || !queue->count) return 0u;
    if (out) *out = queue->event[0];
    for (i = 1u; i < queue->count; ++i) queue->event[i - 1u] = queue->event[i];
    queue->count--;
    return 1u;
}

uint8_t apsis_persistent_audio_next_boundary(const ApsisPersistentAudioQueue *queue,
                                             uint64_t block_start,
                                             uint64_t block_end,
                                             uint64_t *sample_out)
{
    const ApsisPersistentAudioEvent *e = apsis_persistent_audio_queue_peek(queue);
    uint64_t sample;
    if (!e || block_end <= block_start) return 0u;
    sample = e->sample < block_start ? block_start : e->sample;
    if (sample >= block_end) return 0u;
    if (sample_out) *sample_out = sample;
    return 1u;
}

uint8_t apsis_persistent_audio_apply_through(ApsisPersistentAudioQueue *queue,
                                             uint64_t sample,
                                             ApsisStringDsp *string_dsp,
                                             ApsisDriftDsp *drift_dsp)
{
    uint8_t applied = 0u;
    while (queue && queue->count && queue->event[0].sample <= sample) {
        ApsisPersistentAudioEvent e;
        (void)apsis_persistent_audio_queue_pop(queue, &e);
        apsis_persistent_audio_apply(string_dsp, drift_dsp, &e.targets);
        applied++;
    }
    return applied;
}
