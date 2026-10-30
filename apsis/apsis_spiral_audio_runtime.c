#include "apsis_spiral_audio_runtime.h"

static void spiral_rt_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t spiral_rt_voice(uint8_t voice)
{
    return voice <= APSIS_SPIRAL_HALO ? voice : APSIS_SPIRAL_THREAD;
}

static const ApsisTransportEvent *spiral_rt_target_event(
    const ApsisRuntime *runtime, uint64_t target_sample)
{
    uint16_t i;
    const ApsisTransportEvent *best = 0;
    if (!runtime) return 0;
    for (i = 0u; i < runtime->queue.count; ++i) {
        const ApsisTransportEvent *e = &runtime->queue.events[i];
        if (e->type != APSIS_EVENT_SPIRAL_TICK || e->sample != target_sample)
            continue;
        if (!best || e->serial < best->serial) best = e;
    }
    return best;
}

void apsis_spiral_audio_runtime_reset(ApsisSpiralAudioRuntime *audio)
{
    if (!audio) return;
    spiral_rt_zero(audio, (uint32_t)sizeof(*audio));
}

void apsis_spiral_audio_runtime_begin_process(ApsisSpiralAudioRuntime *audio)
{
    if (!audio) return;
    audio->binding_count = 0u;
    spiral_rt_zero(audio->binding, (uint32_t)sizeof(audio->binding));
}

void apsis_spiral_audio_runtime_capture(ApsisSpiralAudioRuntime *audio,
                                        const ApsisRuntime *runtime,
                                        const ApsisState *state)
{
    uint8_t i;
    if (!audio || !runtime || !state) return;

    for (i = 0u; i < APSIS_RUNTIME_SPIRAL_SLOTS; ++i) {
        const ApsisRuntimeSpiralSlot *slot = &runtime->spiral_slots[i];
        ApsisSpiralAudioSnapshot *dst = &audio->snapshot[i];
        const ApsisTransportEvent *target;

        if (!slot->active) continue;
        if (dst->active && dst->token == slot->token) continue;

        target = spiral_rt_target_event(runtime, slot->target_sample);
        if (!target) {
            audio->capture_miss_count++;
            continue;
        }

        spiral_rt_zero(dst, (uint32_t)sizeof(*dst));
        dst->active = 1u;
        dst->token = slot->token;
        dst->target_sample = slot->target_sample;
        apsis_spiral_audio_plan(state, slot->energy_q15,
                                target->interval_q32, target->sub_count,
                                &dst->acoustic);
        if (!dst->acoustic.valid) {
            dst->active = 0u;
            audio->capture_miss_count++;
            continue;
        }
        audio->capture_count++;
    }
}

void apsis_spiral_audio_runtime_bind(ApsisSpiralAudioRuntime *audio,
                                     const ApsisRuntime *runtime,
                                     const ApsisState *state,
                                     const ApsisTransportEvent *bind_event)
{
    ApsisSpiralAudioSnapshot *src;
    ApsisSpiralAudioBinding *dst;
    const ApsisRuntimeSpiralBinding *native;
    uint8_t ix;

    if (!audio || !runtime || !state || !bind_event ||
        bind_event->type != APSIS_EVENT_SPIRAL_BIND ||
        bind_event->sub_index >= APSIS_RUNTIME_SPIRAL_SLOTS)
        return;

    ix = bind_event->sub_index;
    src = &audio->snapshot[ix];
    native = &runtime->last_spiral_bind;

    if (!src->active || src->token != bind_event->absolute_pulse) {
        audio->bind_miss_count++;
        return;
    }

    /* The native slot is finished at bind regardless of whether sound was
     * muted/skipped at that instant. Mirror that lifetime exactly. */
    src->active = 0u;

    if (!native->valid || native->serial != bind_event->absolute_pulse ||
        native->skipped || !native->pitch.valid)
        return;

    if (audio->binding_count >= APSIS_SPIRAL_AUDIO_BINDINGS_MAX) {
        audio->binding_overflow_count++;
        return;
    }

    dst = &audio->binding[audio->binding_count++];
    spiral_rt_zero(dst, (uint32_t)sizeof(*dst));
    dst->valid = 1u;
    dst->cutoff_voice = spiral_rt_voice(state->spiral.voice);
    dst->token = src->token;
    dst->bind_sample = native->bind_sample;
    dst->target_sample = native->target_sample;
    dst->acoustic = src->acoustic;
    dst->pitch = native->pitch;
    audio->bind_count++;
}
