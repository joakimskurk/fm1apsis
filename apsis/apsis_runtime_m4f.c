#include "apsis_runtime_m4f.h"

/* Compile the already-proven M4c coordinator unchanged into this translation
 * unit, then append one explicit Spiral-audio process entry point. Static M3c/
 * M4c helpers remain available below without duplicating their implementations. */
#include "apsis_runtime_m4c.c"

void apsis_runtime_start_spiral_audio(ApsisRuntime *r,
                                      ApsisState *s,
                                      ApsisStringAttractorRuntime *a,
                                      ApsisSpiralAudioRuntime *spiral_audio)
{
    if (spiral_audio) apsis_spiral_audio_runtime_reset(spiral_audio);
    apsis_runtime_start_attractor(r, s, a);
}

void apsis_runtime_stop_spiral_audio(ApsisRuntime *r,
                                     ApsisStringAttractorRuntime *a,
                                     ApsisSpiralAudioRuntime *spiral_audio)
{
    if (spiral_audio) apsis_spiral_audio_runtime_reset(spiral_audio);
    apsis_runtime_stop_attractor(r, a);
}

uint8_t apsis_runtime_process_spiral_audio(
                                      ApsisRuntime *r,
                                      ApsisState *s,
                                      ApsisKeepBank *bank,
                                      ApsisChainSequence *chain,
                                      ApsisMotherPending *mother_pending,
                                      ApsisBloomVoicingState *bloom_voicing,
                                      uint32_t frames,
                                      ApsisRuntimeMotherApplyFn mother_apply,
                                      void *mother_user,
                                      ApsisStringAttractorRuntime *a,
                                      ApsisSpiralAudioRuntime *spiral_audio,
                                      ApsisRuntimeStringRhythmObservation *observation)
{
    ApsisTransportEvent rhythm_due;
    uint32_t rhythm_before = 0u;
    uint64_t end, horizon, lookahead;
    uint8_t have_rhythm = 0u;

    if (!spiral_audio) {
        return apsis_runtime_process_observed_attractor(
            r, s, bank, chain, mother_pending, bloom_voicing, frames,
            mother_apply, mother_user, a, observation);
    }

    if (observation) runtime_zero(observation, (uint32_t)sizeof(*observation));
    if (!r || !s || !frames) return 0u;

    rhythm_before = r->event_count[APSIS_EVENT_STRING_RHYTHM];
    end = r->cursor_sample + frames;
    if (end < r->cursor_sample) end = ~(uint64_t)0;
    have_rhythm = final_due_string_rhythm(r, end, &rhythm_due);

    if (a) apsis_string_attractor_begin_process(a);
    apsis_spiral_audio_runtime_begin_process(spiral_audio);

    lookahead = m3c_ms_samples(r->transport.sample_rate, APSIS_RUNTIME_LOOKAHEAD_MS);
    horizon = end + lookahead;
    if (horizon < end) horizon = ~(uint64_t)0;
    runtime_apply_chain_morph(r, s, r->cursor_sample);
    r->scheduler_starved = 0u;

    while (r->transport.running && runtime_next_transport_sample(&r->transport) < horizon) {
        uint8_t added = a ? m4c_schedule_next_pulse(r, s, a)
                          : m3c_schedule_next_pulse(r, s);
        if (!added) break;
        /* m4c_schedule_next_pulse can mutate Attractor cells but does not apply
         * structural/Chain state. Capturing here therefore sees the exact same
         * acoustic state that Study 126 used to prepare this occurrence. */
        apsis_spiral_audio_runtime_capture(spiral_audio, r, s);
    }

    for (;;) {
        const ApsisTransportEvent *peek = apsis_event_queue_peek(&r->queue);
        ApsisTransportEvent e;
        if (!peek || peek->sample >= end) break;
        apsis_event_queue_pop(&r->queue, &e);
        runtime_apply_chain_morph(r, s, e.sample);
        if (e.type < APSIS_RUNTIME_EVENT_TYPES) r->event_count[e.type]++;
        r->last_event_sample = e.sample;
        switch (e.type) {
            case APSIS_EVENT_STEP_ONE_RESYNC:
                break;
            case APSIS_EVENT_MOTHER_PULSE:
            case APSIS_EVENT_CHAIN_PULSE:
            case APSIS_EVENT_SPIRAL_TICK:
            case APSIS_EVENT_BLOOM:
                break;
            case APSIS_EVENT_GRAVITY_COMMIT:
                m3c_handle_gravity_commit(r, s, &e);
                break;
            case APSIS_EVENT_MOTHER_COMMIT_OBSERVE:
                runtime_commit_mother(r, s, mother_pending, &e,
                                      mother_apply, mother_user);
                break;
            case APSIS_EVENT_CHAIN_APPLY_OBSERVE:
                m3c_handle_chain_observe(r, s, bank, chain, &e);
                break;
            case APSIS_EVENT_SPIRAL_BIND:
                m3c_handle_spiral_bind(r, s, &e);
                apsis_spiral_audio_runtime_bind(spiral_audio, r, s, &e);
                break;
            case APSIS_EVENT_BLOOM_OCCURRENCE:
                m3c_handle_bloom_occurrence(r, s, bloom_voicing, &e);
                break;
            case APSIS_EVENT_BLOOM_BIND:
                m3c_handle_bloom_bind(r, s, &e);
                break;
            case APSIS_EVENT_STRING_ATTRACTOR_BIND:
                if (a)
                    (void)apsis_string_attractor_pending_bind(a, e.sub_index,
                                                              e.absolute_pulse,
                                                              e.sample);
                break;
            default:
                break;
        }
    }

    runtime_apply_chain_morph(r, s, end);
    runtime_update_high_water(r);
    r->cursor_sample = end;

    if (have_rhythm &&
        r->event_count[APSIS_EVENT_STRING_RHYTHM] != rhythm_before) {
        if (observation) {
            observation->valid = 1u;
            observation->event = rhythm_due;
        }
        return 1u;
    }
    return 0u;
}
