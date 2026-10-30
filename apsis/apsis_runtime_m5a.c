#include "apsis_runtime_m5a.h"

/* Keep M4g byte-for-byte available in this translation unit. M5a adds a new
 * process entry point rather than editing the frozen coordinator. */
#include "apsis_runtime_m4g.c"

static void m5a_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

void apsis_structural_audio_stats_reset(ApsisStructuralAudioStats *stats)
{
    if (stats) m5a_zero(stats, (uint32_t)sizeof(*stats));
}

static uint8_t m5a_capture_targets(ApsisRuntime *r,
                                   ApsisState *s,
                                   ApsisPersistentAudioQueue *queue,
                                   ApsisStringPitchMemory *pitch_memory,
                                   uint64_t target_sample)
{
    ApsisPersistentAudioTargets targets;
    if (!r || !s || !queue) return 0u;
    apsis_persistent_audio_capture(s, pitch_memory,
                                   r->transport.running ? 1u : 0u,
                                   &targets);
    return apsis_persistent_audio_queue_push(queue, target_sample, &targets);
}

uint8_t apsis_runtime_process_structural_audio(
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
                                     ApsisBloomAudioRuntime *bloom_audio,
                                     ApsisRuntimeStringRhythmObservation *observation,
                                     ApsisPersistentAudioQueue *persistent_queue,
                                     ApsisStringPitchMemory *pitch_memory,
                                     ApsisStructuralAudioStats *stats)
{
    ApsisTransportEvent rhythm_due;
    uint32_t rhythm_before = 0u;
    uint64_t end, horizon, lookahead;
    uint8_t have_rhythm = 0u;

    if (!persistent_queue) {
        return apsis_runtime_process_bloom_audio(
            r, s, bank, chain, mother_pending, bloom_voicing, frames,
            mother_apply, mother_user, a, spiral_audio, bloom_audio, observation);
    }

    if (observation) runtime_zero(observation, (uint32_t)sizeof(*observation));
    if (!r || !s || !frames) return 0u;

    rhythm_before = r->event_count[APSIS_EVENT_STRING_RHYTHM];
    end = r->cursor_sample + frames;
    if (end < r->cursor_sample) end = ~(uint64_t)0;
    have_rhythm = final_due_string_rhythm(r, end, &rhythm_due);

    if (a) apsis_string_attractor_begin_process(a);
    if (spiral_audio) apsis_spiral_audio_runtime_begin_process(spiral_audio);
    if (bloom_audio) apsis_bloom_audio_runtime_begin_process(bloom_audio);

    lookahead = m3c_ms_samples(r->transport.sample_rate, APSIS_RUNTIME_LOOKAHEAD_MS);
    horizon = end + lookahead;
    if (horizon < end) horizon = ~(uint64_t)0;
    runtime_apply_chain_morph(r, s, r->cursor_sample);
    r->scheduler_starved = 0u;

    while (r->transport.running && runtime_next_transport_sample(&r->transport) < horizon) {
        uint8_t added = a ? m4c_schedule_next_pulse(r, s, a)
                          : m3c_schedule_next_pulse(r, s);
        if (!added) break;
        if (spiral_audio)
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
            case APSIS_EVENT_GRAVITY_COMMIT: {
                uint8_t should_capture = (uint8_t)(
                    r->gravity_pending.active &&
                    e.absolute_pulse == r->gravity_pending.token);
                m3c_handle_gravity_commit(r, s, &e);
                if (should_capture && !r->gravity_pending.active) {
                    if (m5a_capture_targets(r, s, persistent_queue, pitch_memory,
                                            e.interval_q32)) {
                        if (stats) stats->gravity_captures++;
                    } else if (stats) stats->capture_drops++;
                }
                break;
            }
            case APSIS_EVENT_MOTHER_COMMIT_OBSERVE: {
                uint16_t before = mother_pending
                    ? apsis_mother_pending_count(mother_pending) : 0u;
                runtime_commit_mother(r, s, mother_pending, &e,
                                      mother_apply, mother_user);
                if (before && mother_pending &&
                    apsis_mother_pending_count(mother_pending) == 0u) {
                    if (m5a_capture_targets(r, s, persistent_queue, pitch_memory,
                                            e.interval_q32)) {
                        if (stats) stats->mother_captures++;
                    } else if (stats) stats->capture_drops++;
                }
                break;
            }
            case APSIS_EVENT_CHAIN_APPLY_OBSERVE: {
                uint32_t random_before = r->random_state;
                m3c_handle_chain_observe(r, s, bank, chain, &e);
                if (chain && r->random_state != random_before && chain->last_applied) {
                    if (m5a_capture_targets(r, s, persistent_queue, pitch_memory,
                                            e.interval_q32)) {
                        if (stats) stats->chain_captures++;
                    } else if (stats) stats->capture_drops++;
                }
                break;
            }
            case APSIS_EVENT_SPIRAL_BIND:
                m3c_handle_spiral_bind(r, s, &e);
                if (spiral_audio)
                    apsis_spiral_audio_runtime_bind(spiral_audio, r, s, &e);
                break;
            case APSIS_EVENT_BLOOM_OCCURRENCE:
                m3c_handle_bloom_occurrence(r, s, bloom_voicing, &e);
                if (bloom_audio)
                    apsis_bloom_audio_runtime_capture(bloom_audio, r, s);
                break;
            case APSIS_EVENT_BLOOM_BIND:
                m3c_handle_bloom_bind(r, s, &e);
                if (bloom_audio)
                    apsis_bloom_audio_runtime_bind(bloom_audio, r, s, &e);
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
