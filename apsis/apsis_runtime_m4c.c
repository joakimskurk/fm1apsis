/* M4c String Attractor runtime layer.
 *
 * This repeats the tiny final M3c lifecycle/Rhythm wrapper while including the
 * validated M3c coordinator itself unchanged. The original public
 * apsis_runtime_process() therefore remains the exact M3c implementation; only
 * explicit M4c entry points use the Attractor-aware schedule/consumer path.
 */
#define apsis_runtime_start apsis_runtime_start_m3c_base
#define apsis_runtime_stop apsis_runtime_stop_m3c_base
#include "apsis_runtime_m3c.c"
#undef apsis_runtime_start
#undef apsis_runtime_stop

#define M4C_ATTRACTOR_BIND_LEAD_MS 45u
#define M4C_ATTRACTOR_MAX_BINDS_PER_PULSE 4u
#define M4C_PULSE_RESERVE (M3C_PULSE_RESERVE + M4C_ATTRACTOR_MAX_BINDS_PER_PULSE)

/* Frozen final-M3c lifecycle helpers, kept byte-for-byte in behavior. */
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

/* M4c shared-heap extension. */
static uint32_t m4c_attractor_random(void *user, uint32_t tag)
{
    return m3c_random((ApsisRuntime *)user, tag);
}

static uint64_t m4c_target_with_delta(const ApsisTransport *t, uint64_t delta_q32)
{
    uint64_t sample, frac;
    uint32_t f;
    if (!t) return 0u;
    sample = t->next_sample + (delta_q32 >> 32);
    f = t->next_frac_q32;
    frac = (uint64_t)f + (uint32_t)delta_q32;
    sample += frac >> 32;
    f = (uint32_t)frac;
    return sample + (f >= 0x80000000u ? 1u : 0u);
}

static void m4c_attractor_targets(const ApsisRuntime *r, const ApsisState *s,
                                  uint64_t target[2], uint8_t *clock_step)
{
    uint8_t n, step;
    uint64_t interval_q32;
    if (!target || !clock_step) return;
    target[0] = 0u;
    target[1] = 0u;
    *clock_step = 0u;
    if (!r || !s) return;
    n = runtime_clock_n(s->clock.n);
    step = r->transport.cycle_pos;
    if (step >= n) step = 0u;
    interval_q32 = apsis_transport_interval_q32(&s->clock,
                                                 r->transport.sample_rate,
                                                 r->transport.absolute_pulse);
    target[0] = runtime_next_transport_sample(&r->transport);
    target[1] = m4c_target_with_delta(&r->transport, interval_q32 >> 1);
    *clock_step = step;
}

static uint8_t m4c_schedule_attractor_route(ApsisRuntime *r,
                                             ApsisStringAttractorRuntime *a,
                                             const ApsisStringAttractorDecision *d,
                                             uint8_t route,
                                             uint8_t both_scale)
{
    ApsisTransportEvent e;
    uint64_t bind_sample;
    uint32_t token;
    uint8_t slot;
    if (!r || !a || !d) return 0u;
    bind_sample = m3c_sub_ms(d->target_sample, r->transport.sample_rate,
                             M4C_ATTRACTOR_BIND_LEAD_MS);
    slot = apsis_string_attractor_pending_alloc(a, route, d->pos, both_scale,
                                                 d->value_q15,
                                                 d->base_level_q15,
                                                 bind_sample,
                                                 d->target_sample,
                                                 &token);
    if (slot >= APSIS_STRING_ATTRACTOR_PENDING_CAP) return 0u;
    e = m3c_event(APSIS_EVENT_STRING_ATTRACTOR_BIND, bind_sample,
                  d->target_sample);
    e.absolute_pulse = token;
    e.sub_index = slot;
    e.clock_step = d->pos;
    if (!m3c_queue_push(r, e)) {
        a->pending[slot].active = 0u;
        a->pending_overflowed = 1u;
        return 0u;
    }
    return 1u;
}

static void m4c_schedule_attractor_decisions(ApsisRuntime *r,
                                              ApsisStringAttractorRuntime *a,
                                              const ApsisStringAttractorDecision d[2],
                                              uint8_t count)
{
    uint8_t i;
    if (!r || !a || !d) return;
    for (i = 0u; i < count; ++i) {
        uint8_t both;
        if (!d[i].excite) continue;
        both = (uint8_t)((d[i].route_mask &
                         (APSIS_STRING_ATTRACTOR_ROUTE_LEFT |
                          APSIS_STRING_ATTRACTOR_ROUTE_RIGHT)) ==
                        (APSIS_STRING_ATTRACTOR_ROUTE_LEFT |
                         APSIS_STRING_ATTRACTOR_ROUTE_RIGHT));
        if (d[i].route_mask & APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC) {
            (void)m4c_schedule_attractor_route(r, a, &d[i],
                                               APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC,
                                               0u);
        } else {
            if (d[i].route_mask & APSIS_STRING_ATTRACTOR_ROUTE_LEFT)
                (void)m4c_schedule_attractor_route(r, a, &d[i],
                                                   APSIS_STRING_ATTRACTOR_ROUTE_LEFT,
                                                   both);
            if (d[i].route_mask & APSIS_STRING_ATTRACTOR_ROUTE_RIGHT)
                (void)m4c_schedule_attractor_route(r, a, &d[i],
                                                   APSIS_STRING_ATTRACTOR_ROUTE_RIGHT,
                                                   both);
        }
    }
}

static uint8_t m4c_schedule_next_pulse(ApsisRuntime *r, ApsisState *s,
                                       ApsisStringAttractorRuntime *a)
{
    M3cSpiralDecision spiral[4];
    ApsisStringAttractorDecision attractor[2];
    uint64_t attractor_target[2];
    uint32_t first_serial, end_serial, ghost = 0u;
    uint8_t added, attr_count, clock_step, current_step;
    if (!r || !s || !r->transport.running) return 0u;

    current_step = r->transport.cycle_pos < runtime_clock_n(s->clock.n)
        ? r->transport.cycle_pos : 0u;
    /* NULL/inactive/skipped pulses take the exact frozen M3c function. */
    if (!a || !apsis_string_attractor_advances_for_pulse(s, current_step))
        return m3c_schedule_next_pulse(r, s);

    if ((uint32_t)r->queue.count + M4C_PULSE_RESERVE > APSIS_TRANSPORT_QUEUE_CAP) {
        r->scheduler_starved = 1u;
        return 0u;
    }

    /* Study 126 order: String Attractor, Spiral, then Clock ghost. */
    m4c_attractor_targets(r, s, attractor_target, &clock_step);
    attr_count = apsis_string_attractor_plan_pulse(a, s, clock_step,
                                                    attractor_target,
                                                    m4c_attractor_random, r,
                                                    attractor);
    m3c_prepare_spiral(r, s, spiral);
    if (s->clock.sound && (r->transport.melody_step % 3u) == 2u)
        ghost = m3c_random(r, 0x47484f53u);

    first_serial = r->queue.next_serial;
    added = apsis_transport_schedule_next(&r->transport, s, &r->queue, ghost);
    runtime_update_high_water(r);
    if (!added) return 0u;
    end_serial = r->queue.next_serial;
    m3c_attach_transport_events(r, s, first_serial, end_serial, spiral);
    m4c_schedule_attractor_decisions(r, a, attractor, attr_count);
    return added;
}

void apsis_runtime_start_attractor(ApsisRuntime *r,
                                   ApsisState *s,
                                   ApsisStringAttractorRuntime *a)
{
    if (a) apsis_string_attractor_reset(a, s);
    apsis_runtime_start(r);
}

void apsis_runtime_stop_attractor(ApsisRuntime *r,
                                  ApsisStringAttractorRuntime *a)
{
    if (a) apsis_string_attractor_stop(a);
    apsis_runtime_stop(r);
}

void apsis_runtime_process_attractor(ApsisRuntime *r,
                                     ApsisState *s,
                                     ApsisKeepBank *bank,
                                     ApsisChainSequence *chain,
                                     ApsisMotherPending *mother_pending,
                                     ApsisBloomVoicingState *bloom_voicing,
                                     uint32_t frames,
                                     ApsisRuntimeMotherApplyFn mother_apply,
                                     void *mother_user,
                                     ApsisStringAttractorRuntime *a)
{
    uint64_t end, horizon, lookahead;
    if (!a) {
        apsis_runtime_process(r, s, bank, chain, mother_pending,
                              bloom_voicing, frames, mother_apply, mother_user);
        return;
    }
    if (!r || !s || !frames) return;
    apsis_string_attractor_begin_process(a);
    end = r->cursor_sample + frames;
    lookahead = m3c_ms_samples(r->transport.sample_rate, APSIS_RUNTIME_LOOKAHEAD_MS);
    horizon = end + lookahead;
    if (horizon < end) horizon = ~(uint64_t)0;
    runtime_apply_chain_morph(r, s, r->cursor_sample);
    r->scheduler_starved = 0u;

    while (r->transport.running && runtime_next_transport_sample(&r->transport) < horizon) {
        if (!m4c_schedule_next_pulse(r, s, a)) break;
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
                break;
            case APSIS_EVENT_BLOOM_OCCURRENCE:
                m3c_handle_bloom_occurrence(r, s, bloom_voicing, &e);
                break;
            case APSIS_EVENT_BLOOM_BIND:
                m3c_handle_bloom_bind(r, s, &e);
                break;
            case APSIS_EVENT_STRING_ATTRACTOR_BIND:
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
}

uint8_t apsis_runtime_process_observed_attractor(
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

    apsis_runtime_process_attractor(r, s, bank, chain, mother_pending,
                                    bloom_voicing, frames, mother_apply,
                                    mother_user, a);

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
