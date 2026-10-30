#include "apsis_bloom_audio_runtime.h"

static void bloom_rt_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t bloom_rt_voice(uint8_t voice)
{
    return voice <= APSIS_BLOOM_GLASS ? voice : APSIS_BLOOM_VEIL;
}

static uint16_t bloom_rt_attack_ms(uint16_t attack_ms)
{
    if (attack_ms < 8u) return 8u;
    if (attack_ms > 800u) return 800u;
    return attack_ms;
}

static uint16_t bloom_rt_brightness_q15(uint16_t brightness_q15)
{
    const uint16_t lo = APSIS_Q15_MILLI(120);
    if (brightness_q15 < lo) return lo;
    if (brightness_q15 > APSIS_Q15_ONE) return APSIS_Q15_ONE;
    return brightness_q15;
}

void apsis_bloom_audio_runtime_reset(ApsisBloomAudioRuntime *audio)
{
    if (!audio) return;
    bloom_rt_zero(audio, (uint32_t)sizeof(*audio));
}

void apsis_bloom_audio_runtime_begin_process(ApsisBloomAudioRuntime *audio)
{
    if (!audio) return;
    audio->binding_count = 0u;
    bloom_rt_zero(audio->binding, (uint32_t)sizeof(audio->binding));
}

void apsis_bloom_audio_runtime_capture(ApsisBloomAudioRuntime *audio,
                                       const ApsisRuntime *runtime,
                                       const ApsisState *state)
{
    uint8_t i;
    if (!audio || !runtime || !state) return;

    for (i = 0u; i < APSIS_RUNTIME_BLOOM_SLOTS; ++i) {
        const ApsisRuntimeBloomSlot *slot = &runtime->bloom_slots[i];
        ApsisBloomAudioSnapshot *dst = &audio->snapshot[i];

        if (!slot->active) continue;
        if (dst->active && dst->token == slot->token) continue;

        bloom_rt_zero(dst, (uint32_t)sizeof(*dst));
        dst->active = 1u;
        dst->token = slot->token;
        dst->occurrence_sample = slot->occurrence_sample;
        dst->target_sample = slot->target_sample;
        apsis_bloom_audio_plan(state, slot->voice_count, &dst->acoustic);
        if (!dst->acoustic.valid || dst->acoustic.count != slot->voice_count) {
            dst->active = 0u;
            audio->capture_miss_count++;
            continue;
        }
        audio->capture_count++;
    }
}

void apsis_bloom_audio_runtime_bind(ApsisBloomAudioRuntime *audio,
                                    const ApsisRuntime *runtime,
                                    const ApsisState *state,
                                    const ApsisTransportEvent *bind_event)
{
    ApsisBloomAudioSnapshot *src;
    ApsisBloomAudioBinding *dst;
    const ApsisRuntimeBloomBinding *native;
    uint8_t ix, petal, final_petal;

    if (!audio || !runtime || !state || !bind_event ||
        bind_event->type != APSIS_EVENT_BLOOM_BIND ||
        bind_event->sub_index >= APSIS_RUNTIME_BLOOM_SLOTS)
        return;

    ix = bind_event->sub_index;
    petal = bind_event->sub_count;
    src = &audio->snapshot[ix];
    native = &runtime->last_bloom_bind;

    if (!src->active || src->token != bind_event->absolute_pulse ||
        petal >= src->acoustic.count) {
        audio->bind_miss_count++;
        return;
    }

    final_petal = (uint8_t)(petal + 1u >= src->acoustic.count);
    if (final_petal) src->active = 0u;

    if (!native->valid || native->serial != bind_event->absolute_pulse ||
        native->skipped || !native->pitch.valid)
        return;

    if (audio->binding_count >= APSIS_BLOOM_AUDIO_BINDINGS_MAX) {
        audio->binding_overflow_count++;
        return;
    }

    dst = &audio->binding[audio->binding_count++];
    bloom_rt_zero(dst, (uint32_t)sizeof(*dst));
    dst->valid = 1u;
    dst->petal_index = petal;
    dst->voice = bloom_rt_voice(state->bloom.voice);
    dst->token = src->token;
    dst->bind_sample = native->bind_sample;
    dst->target_sample = native->target_sample;
    dst->attack_ms = bloom_rt_attack_ms(state->bloom.attack_ms);
    dst->brightness_q15 = bloom_rt_brightness_q15(state->bloom.brightness_q15);
    dst->acoustic = src->acoustic.petal[petal];
    dst->pitch = native->pitch;
    audio->bind_count++;
}
