/* Final Phase-3 runtime entry point.
 *
 * M3c deliberately layers lifecycle parity on top of the coordinator rather
 * than rewriting the validated M3b base. Study 126 audio.stop() cancels the
 * future scheduler/Spiral/Bloom/Chain/Mother timers and resets Spiral planner
 * state, while the separate framePending Gravity timer is not cancelled.
 */
#define apsis_runtime_start apsis_runtime_start_m3c_base
#define apsis_runtime_stop apsis_runtime_stop_m3c_base
#include "apsis_runtime_m3c.c"
#undef apsis_runtime_start
#undef apsis_runtime_stop

static void final_clear_late_slots(ApsisRuntime *r)
{
    uint8_t i;
    if (!r) return;
    for (i = 0u; i < APSIS_RUNTIME_SPIRAL_SLOTS; ++i)
        runtime_zero(&r->spiral_slots[i], (uint32_t)sizeof(r->spiral_slots[i]));
    for (i = 0u; i < APSIS_RUNTIME_BLOOM_SLOTS; ++i)
        runtime_zero(&r->bloom_slots[i], (uint32_t)sizeof(r->bloom_slots[i]));
}

static void final_requeue_pending_gravity(ApsisRuntime *r)
{
    ApsisTransportEvent e;
    uint64_t commit;
    if (!r || !r->gravity_pending.active) return;
    commit = m3c_sub_ms(r->gravity_pending.target_sample,
                        r->transport.sample_rate,
                        APSIS_RUNTIME_GRAVITY_COMMIT_LEAD_MS);
    if (commit < r->cursor_sample) commit = r->cursor_sample;
    e = m3c_event(APSIS_EVENT_GRAVITY_COMMIT, commit,
                  r->gravity_pending.target_sample);
    e.absolute_pulse = r->gravity_pending.token;
    if (!m3c_queue_push(r, e)) r->late_slot_overflowed = 1u;
}

void apsis_runtime_start(ApsisRuntime *r)
{
    uint32_t rate;
    if (!r || r->transport.running) return;
    rate = r->transport.sample_rate ? r->transport.sample_rate
                                    : APSIS_TRANSPORT_DEFAULT_RATE;
    /* Source startScheduler() begins a fresh shared transport/planner. Native
     * keeps the already-established zero-latency transport origin rather than
     * emulating the browser's 50 ms WebAudio startup safety pad. */
    apsis_transport_reset(&r->transport, rate, r->cursor_sample);
    apsis_event_queue_reset(&r->queue);
    final_clear_late_slots(r);
    r->spiral_attractor_pos = 0u;
    r->spiral_step = 0u;
    r->chain_morph_active = 0u;
    r->scheduler_starved = 0u;
    final_requeue_pending_gravity(r);
    apsis_transport_start(&r->transport);
}

void apsis_runtime_stop(ApsisRuntime *r)
{
    if (!r || !r->transport.running) return;
    apsis_transport_stop(&r->transport);
    apsis_event_queue_reset(&r->queue);
    final_clear_late_slots(r);
    r->spiral_attractor_pos = 0u;
    r->spiral_step = 0u;
    r->last_spiral.valid = 0u;
    r->last_spiral_bind.valid = 0u;
    r->last_bloom_bind.valid = 0u;
    r->chain_morph_active = 0u;
    r->scheduler_starved = 0u;
    /* Normal quantized Gravity is intentionally independent of audio.stop()
     * in Study 126. Keep exactly that one future callback alive. */
    final_requeue_pending_gravity(r);
}

static uint8_t final_due_string_rhythm(const ApsisRuntime *r, uint64_t end,
                                       ApsisTransportEvent *out)
{
    uint16_t i;
    uint8_t found = 0u;
    if (!r || !out) return 0u;
    for (i = 0u; i < r->queue.count; ++i) {
        const ApsisTransportEvent *e = &r->queue.events[i];
        if (e->type != APSIS_EVENT_STRING_RHYTHM || e->sample >= end) continue;
        if (!found || m3c_event_before(e, out)) {
            *out = *e;
            found = 1u;
        }
    }
    return found;
}

uint8_t apsis_runtime_process_observed(ApsisRuntime *r,
                                      ApsisState *s,
                                      ApsisKeepBank *bank,
                                      ApsisChainSequence *chain,
                                      ApsisMotherPending *mother_pending,
                                      ApsisBloomVoicingState *bloom_voicing,
                                      uint32_t frames,
                                      ApsisRuntimeMotherApplyFn mother_apply,
                                      void *mother_user,
                                      ApsisRuntimeStringRhythmObservation *observation)
{
    ApsisTransportEvent due;
    uint32_t before = 0u;
    uint64_t end = 0u;
    uint8_t have_due = 0u;

    if (observation) runtime_zero(observation, (uint32_t)sizeof(*observation));
    if (r && frames) {
        before = r->event_count[APSIS_EVENT_STRING_RHYTHM];
        end = r->cursor_sample + frames;
        if (end < r->cursor_sample) end = ~(uint64_t)0;
        have_due = final_due_string_rhythm(r, end, &due);
    }

    apsis_runtime_process(r, s, bank, chain, mother_pending, bloom_voicing,
                          frames, mother_apply, mother_user);

    if (r && have_due &&
        r->event_count[APSIS_EVENT_STRING_RHYTHM] != before) {
        if (observation) {
            observation->valid = 1u;
            observation->event = due;
        }
        return 1u;
    }
    return 0u;
}
