#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "apsis_runtime.h"
#include "apsis_string_attractor_audio.h"

static void zero_audio(int32_t *out, uint32_t frames)
{
    uint32_t i;
    for (i = 0u; i < frames * 2u; ++i) out[i] = 0;
}

static ApsisStringTransientVoice *newest_body(ApsisStringTransientPool *pool)
{
    uint16_t i;
    ApsisStringTransientVoice *best = 0;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &pool->transient[i];
        if (!v->active || v->waveform != APSIS_STRING_DSP_WAVE_BODY) continue;
        if (!best || v->serial > best->serial) best = v;
    }
    return best;
}

int main(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisStringAttractorRuntime attractor;
    ApsisStringPitchMemory pitch;
    ApsisStringTransientPool pool;
    ApsisRuntimeStringRhythmObservation rhythm;
    int32_t out[256u * 2u];
    uint64_t frozen_target = 0u;
    uint64_t render_origin = 0u;
    uint32_t guard = 0u;
    uint8_t i, saw_frozen_left = 0u;
    ApsisStringTransientVoice *voice = 0;

    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 0u, 0x4c495645u);
    apsis_keep_bank_reset(&bank);
    apsis_chain_sequence_reset(&chain);
    apsis_mother_pending_reset(&mother);
    apsis_bloom_voicing_reset(&bloom);
    apsis_string_pitch_memory_reset(&pitch);
    apsis_string_transient_pool_reset(&pool);

    s.string.rhythm = APSIS_STRING_RHYTHM_OFF;
    s.string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_2;
    s.string.filter_attractor_lock_q15 = APSIS_Q15_ONE;
    s.string.filter_attractor_gates = 0xffffu;
    s.string.filter_attractor_strum_q15 = APSIS_Q15_ONE;
    s.string.filter_attractor_strum_level_q15 = APSIS_Q15_ONE;
    s.string.attractor_target = APSIS_STRING_ATTRACTOR_LEFT;
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.foundation = 0u;

    apsis_runtime_start_attractor(&r, &s, &attractor);

    /* Target-like callback order: render already-allocated finite voices first,
     * then advance the shared runtime, then bind newly-due Attractor callbacks. */
    zero_audio(out, 256u);
    apsis_string_transient_render(&pool, out, 256u);
    (void)apsis_runtime_process_observed_attractor(
        &r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0,
        &attractor, &rhythm);
    for (i = 0u; i < attractor.binding_count; ++i)
        (void)apsis_string_attractor_trigger_binding(
            &pool, &s, &pitch, &attractor.binding[i], r.cursor_sample);

    /* The 1.20 s lookahead already planned the later step-2 occurrence as LEFT.
     * Mutate live surface/target/pitch/level now: route must remain LEFT while
     * pitch/level are deliberately late-read when its bind callback arrives. */
    for (i = 0u; i < APSIS_STRING_ATTRACTOR_PENDING_CAP; ++i) {
        if (attractor.pending[i].active && attractor.pending[i].target_sample > 10000u &&
            attractor.pending[i].route == APSIS_STRING_ATTRACTOR_ROUTE_LEFT) {
            frozen_target = attractor.pending[i].target_sample;
            saw_frozen_left = 1u;
            break;
        }
    }
    assert(saw_frozen_left && frozen_target > 10000u);
    s.string.attractor_target = APSIS_STRING_ATTRACTOR_RIGHT;
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    s.string.left_split_q15 = APSIS_Q15_MILLI(350);
    s.string.left_level_q15 = APSIS_Q15_MILLI(410);

    while (!voice && guard++ < 256u) {
        zero_audio(out, 256u);
        apsis_string_transient_render(&pool, out, 256u);
        (void)apsis_runtime_process_observed_attractor(
            &r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0,
            &attractor, &rhythm);
        render_origin = r.cursor_sample;
        for (i = 0u; i < attractor.binding_count; ++i) {
            const ApsisStringAttractorBinding *b = &attractor.binding[i];
            if (b->target_sample == frozen_target) {
                uint16_t before = apsis_string_dsp_transient_logical_active(&pool);
                assert(b->route == APSIS_STRING_ATTRACTOR_ROUTE_LEFT);
                assert(apsis_string_attractor_trigger_binding(
                    &pool, &s, &pitch, b, render_origin) == 1u);
                assert(apsis_string_dsp_transient_logical_active(&pool) == before + 1u);
                voice = newest_body(&pool);
                break;
            }
            (void)apsis_string_attractor_trigger_binding(
                &pool, &s, &pitch, b, render_origin);
        }
    }

    assert(voice);
    assert(frozen_target > render_origin);
    assert(voice->start_delay_samples == (uint32_t)(frozen_target - render_origin));
    assert(voice->age_samples == 0u);

    /* Exact native target proof: rendering precisely the remaining lead consumes
     * only start-delay samples. The voice ages for the first time on the frozen
     * target sample itself, never before it. */
    {
        uint32_t lead = voice->start_delay_samples;
        int32_t chunk[256u * 2u];
        while (lead) {
            uint32_t frames = lead > 256u ? 256u : lead;
            zero_audio(chunk, frames);
            apsis_string_transient_render(&pool, chunk, frames);
            lead -= frames;
        }
        assert(voice->start_delay_samples == 0u);
        assert(voice->age_samples == 0u);
        zero_audio(chunk, 1u);
        apsis_string_transient_render(&pool, chunk, 1u);
        assert(voice->age_samples == 1u);
    }

    assert(attractor.pending_overflowed == 0u);
    assert(attractor.binding_overflowed == 0u);
    assert(pool.transient_drop_count == 0u);
    printf("PASS: M4c live Attractor plan->bind->pool target=%lu bind-block-end=%lu lead=%lu\n",
           (unsigned long)frozen_target, (unsigned long)render_origin,
           (unsigned long)(frozen_target - render_origin));
    return 0;
}
