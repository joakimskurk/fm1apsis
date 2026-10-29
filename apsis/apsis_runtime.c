#include "apsis_runtime.h"
#include "apsis_spiral.h"

static void runtime_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint32_t runtime_random(ApsisRuntime *r)
{
    if (!r) return 0u;
    if (!r->random_state) r->random_state = 0x41505349u;
    r->random_state = 1664525u * r->random_state + 1013904223u;
    return r->random_state;
}

static uint16_t runtime_random_q15(uint32_t rnd)
{
    return (uint16_t)(((uint64_t)rnd * APSIS_Q15_ONE) >> 32);
}

static uint8_t runtime_random_below(uint32_t rnd, uint16_t threshold_q15)
{
    uint16_t t = threshold_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : threshold_q15;
    return (uint64_t)rnd * APSIS_Q15_ONE < ((uint64_t)t << 32) ? 1u : 0u;
}

static uint8_t runtime_clock_n(uint8_t n)
{
    return n < 3u ? 3u : (n > 24u ? 24u : n);
}

static uint8_t runtime_chain_clock_n(const ApsisRuntime *r, const ApsisState *s)
{
    if (r && r->chain_meter_n) return runtime_clock_n(r->chain_meter_n);
    return s ? runtime_clock_n(s->clock.n) : 12u;
}

static uint64_t runtime_next_transport_sample(const ApsisTransport *t)
{
    if (!t) return 0u;
    return t->next_sample + (t->next_frac_q32 >= 0x80000000u ? 1u : 0u);
}

static void runtime_update_high_water(ApsisRuntime *r)
{
    uint16_t n;
    if (!r) return;
    n = apsis_event_queue_count(&r->queue);
    if (n > r->queue_high_water) r->queue_high_water = n;
    if (r->queue.overflowed) r->queue_overflowed = 1u;
}

static void runtime_apply_gravity(ApsisState *s, uint8_t degree, uint8_t ring)
{
    if (!s) return;
    s->gravity.degree = degree > 7u ? 7u : degree;
    s->gravity.voicing = ring > 2u ? 2u : ring;
    s->gravity.selected_ring = s->gravity.voicing;
    s->gravity.selected_point_degree = s->gravity.degree;
}

static void runtime_commit_due_gravity(ApsisRuntime *r, ApsisState *s, uint64_t sample)
{
    if (!r || !s || !r->gravity_pending.active) return;
    if (sample < r->gravity_pending.target_sample) return;
    runtime_apply_gravity(s, r->gravity_pending.degree, r->gravity_pending.ring);
    r->gravity_pending.active = 0u;
}

/* Chain morph spans are bounded to one source Clock cycle. Convert the
 * elapsed/span ratio to Q15 with a 15-step binary fraction so the pi32v2
 * target never needs the unavailable __udivdi3 helper. */
static uint16_t runtime_fraction_q15(uint64_t elapsed, uint64_t span)
{
    uint32_t rem, den;
    uint16_t out = 0u;
    uint8_t i;
    if (!span || elapsed >= span) return APSIS_Q15_ONE;
    while (span > 0x3fffffffu) {
        span >>= 1;
        elapsed >>= 1;
        if (elapsed >= span) elapsed = span ? span - 1u : 0u;
    }
    den = (uint32_t)span;
    rem = (uint32_t)elapsed;
    for (i = 0u; i < 15u; ++i) {
        rem <<= 1;
        out <<= 1;
        if (rem >= den) { rem -= den; out |= 1u; }
    }
    return out;
}

static uint16_t runtime_smooth_q15(uint64_t sample, uint64_t start, uint64_t end)
{
    uint32_t x, xx, factor, e;
    if (sample <= start) return 0u;
    if (sample >= end || end <= start) return APSIS_Q15_ONE;
    x = runtime_fraction_q15(sample - start, end - start);
    xx = (x * x + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    factor = (uint32_t)3u * APSIS_Q15_ONE - 2u * x;
    e = (xx * factor + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    return (uint16_t)(e > APSIS_Q15_ONE ? APSIS_Q15_ONE : e);
}

static uint16_t runtime_lerp_q15(uint16_t from, uint16_t to, uint16_t t)
{
    int32_t delta = (int32_t)to - (int32_t)from;
    int32_t scaled = (delta * (int32_t)t) / (int32_t)APSIS_Q15_ONE;
    int32_t value = (int32_t)from + scaled;
    if (value < 0) value = 0;
    if (value > (int32_t)APSIS_Q15_ONE) value = APSIS_Q15_ONE;
    return (uint16_t)value;
}

static void runtime_apply_chain_morph(ApsisRuntime *r, ApsisState *s, uint64_t sample)
{
    uint16_t t;
    if (!r || !s || !r->chain_morph_active) return;
    t = runtime_smooth_q15(sample, r->chain_morph_start_sample, r->chain_morph_end_sample);
    if (r->chain_morph.has_tension) {
        s->string.left_split_q15 = runtime_lerp_q15(r->chain_morph_from_left_q15,
                                                    r->chain_morph.target_left_split_q15, t);
        s->string.right_split_q15 = runtime_lerp_q15(r->chain_morph_from_right_q15,
                                                     r->chain_morph.target_right_split_q15, t);
        s->string.split_q15 = s->string.right_split_q15;
    }
    if (r->chain_morph.has_drift)
        s->drift.amount_q15 = runtime_lerp_q15(r->chain_morph_from_drift_q15,
                                               r->chain_morph.target_drift_amount_q15, t);
    if (t == APSIS_Q15_ONE) r->chain_morph_active = 0u;
}

static uint64_t runtime_chain_morph_samples(const ApsisRuntime *r, const ApsisState *s,
                                            uint8_t morph_quarters)
{
    ApsisClockState clock;
    uint64_t pulse_q32, cycle_q32, duration_q32;
    uint8_t n, q;
    if (!r || !s || !morph_quarters) return 0u;
    q = morph_quarters > 4u ? 4u : morph_quarters;
    clock = s->clock;
    clock.swing_q15 = 0u; /* browser clockCycleMs() is nominal, without swing */
    n = runtime_chain_clock_n(r, s);
    pulse_q32 = apsis_transport_interval_q32(&clock, r->transport.sample_rate, 0u);
    cycle_q32 = pulse_q32 * n;
    duration_q32 = (cycle_q32 * q) >> 2;
    return apsis_transport_round_sample(duration_q32);
}

static void runtime_begin_chain_morph(ApsisRuntime *r, ApsisState *s,
                                      const ApsisChainMorphTargets *mt,
                                      uint64_t sample,
                                      uint16_t from_left, uint16_t from_right,
                                      uint16_t from_drift)
{
    uint64_t duration;
    if (!r || !s || !mt || !mt->morph_quarters || (!mt->has_tension && !mt->has_drift)) return;
    duration = runtime_chain_morph_samples(r, s, mt->morph_quarters);
    if (!duration) return;
    r->chain_morph = *mt;
    r->chain_morph_start_sample = sample;
    r->chain_morph_end_sample = sample + duration;
    r->chain_morph_from_left_q15 = from_left;
    r->chain_morph_from_right_q15 = from_right;
    r->chain_morph_from_drift_q15 = from_drift;
    r->chain_morph_active = 1u;
}

static uint8_t runtime_apply_chain_cell(ApsisRuntime *r, ApsisState *s,
                                        ApsisKeepBank *bank, ApsisChainSequence *seq,
                                        uint8_t index, uint64_t sample)
{
    ApsisChainMorphTargets mt;
    uint16_t from_left, from_right, from_drift;
    uint8_t applied;
    if (!r || !s || !bank || !seq || index >= seq->count) return 0u;
    runtime_apply_chain_morph(r, s, sample);
    from_left = s->string.left_split_q15;
    from_right = s->string.right_split_q15;
    from_drift = s->drift.amount_q15;
    applied = apsis_chain_apply_cell(s, bank, &s->chain, &seq->cells[index],
                                     runtime_random(r), &mt);
    seq->last_applied = applied;
    if (applied)
        runtime_begin_chain_morph(r, s, &mt, sample, from_left, from_right, from_drift);
    return applied;
}

static void runtime_restart_chain(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                                  ApsisChainSequence *seq, uint64_t sample)
{
    uint8_t n;
    if (!r || !s || !bank || !seq || !seq->count) return;
    n = runtime_chain_clock_n(r, s);
    seq->pos = 0u;
    seq->pulse = 0u;
    seq->phrase_pulse = 0u;
    seq->hold_tail = 0u;
    seq->master_pulses = apsis_chain_master_pulses(seq, &s->chain, n);
    runtime_apply_chain_cell(r, s, bank, seq, 0u, sample);
}

static void runtime_advance_chain(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                                  ApsisChainSequence *seq, const ApsisTransportEvent *e)
{
    const ApsisChainCell *cell;
    uint16_t total;
    uint8_t on_one, n;
    if (!r || !s || !bank || !seq || !e || !seq->playing || !seq->count) return;
    n = runtime_chain_clock_n(r, s);
    on_one = e->clock_step == 0u;
    if (seq->armed) {
        if (s->chain.start_sync == APSIS_CHAIN_SYNC_ONE && !on_one) return;
        seq->armed = 0u;
        seq->pos = 0u;
        seq->pulse = 0u;
        seq->phrase_pulse = 0u;
        seq->hold_tail = 0u;
        runtime_apply_chain_cell(r, s, bank, seq, 0u, e->sample);
        return;
    }
    if (seq->hold_tail) {
        seq->phrase_pulse++;
        if (s->chain.loop_sync == APSIS_CHAIN_SYNC_PULSE || on_one)
            runtime_restart_chain(r, s, bank, seq, e->sample);
        return;
    }
    if (seq->pos >= seq->count) return;
    cell = &seq->cells[seq->pos];
    total = apsis_chain_cell_pulses(cell, &s->chain, n);
    seq->pulse++;
    seq->phrase_pulse++;
    if (seq->pulse < total) return;
    seq->pulse = 0u;
    if (seq->pos + 1u < seq->count) {
        seq->pos++;
        runtime_apply_chain_cell(r, s, bank, seq, seq->pos, e->sample);
        return;
    }
    if (s->chain.loop_sync == APSIS_CHAIN_SYNC_PULSE || on_one)
        runtime_restart_chain(r, s, bank, seq, e->sample);
    else
        seq->hold_tail = 1u;
}

static void runtime_advance_spiral(ApsisRuntime *r, ApsisState *s,
                                   const ApsisTransportEvent *e)
{
    uint8_t len, pos, gate, route_count;
    uint16_t energy, lock, density;
    uint32_t rnd;
    if (!r || !s || !e) return;
    len = apsis_spiral_attractor_length(s->spiral.attractor_length);
    pos = (uint8_t)(r->spiral_attractor_pos % len);
    lock = s->spiral.attractor_lock_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->spiral.attractor_lock_q15;
    density = s->spiral.attractor_density_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : s->spiral.attractor_density_q15;
    rnd = runtime_random(r);
    if (runtime_random_q15(rnd) > lock) {
        uint32_t gate_rnd = runtime_random(r);
        uint32_t energy_rnd = runtime_random(r);
        if (runtime_random_below(gate_rnd, density)) s->spiral.attractor_gates |= (1u << pos);
        else s->spiral.attractor_gates &= ~(1u << pos);
        s->spiral.attractor_energy_q15[pos] = (uint16_t)(APSIS_Q15_MILLI(200) +
            (uint16_t)(((uint64_t)energy_rnd * APSIS_Q15_MILLI(800)) >> 32));
    }
    gate = (s->spiral.attractor_gates & (1u << pos)) ? 1u : 0u;
    energy = s->spiral.attractor_energy_q15[pos];
    if (energy < APSIS_Q15_MILLI(120)) energy = APSIS_Q15_MILLI(120);
    if (energy > APSIS_Q15_ONE) energy = APSIS_Q15_ONE;
    r->spiral_attractor_pos = (uint8_t)((pos + 1u) % len);

    route_count = s->spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : s->spiral.route_count;
    if (gate && route_count) {
        uint8_t index = (uint8_t)(r->spiral_step % route_count);
        r->last_spiral.valid = 1u;
        r->last_spiral.sample = e->sample;
        r->last_spiral.serial++;
        r->last_spiral.route_index = index;
        r->last_spiral.route_point = s->spiral.route[index] <= 35u ? s->spiral.route[index] : 0u;
        r->last_spiral.trigger_pos = pos;
        r->last_spiral.energy_q15 = energy;
        r->spiral_step++;
        r->spiral_occurrence_count++;
    }
}

static void runtime_advance_bloom(ApsisRuntime *r, ApsisState *s,
                                  ApsisBloomVoicingState *voicing,
                                  const ApsisTransportEvent *e)
{
    ApsisBloomHarmony h;
    ApsisBloomPlan p;
    if (!r || !s || !e) return;
    apsis_bloom_harmony_from_state(s, &h);
    if (voicing)
        p = apsis_bloom_choose_voicing(s, &h, voicing, 1u,
                                       runtime_random(r), runtime_random(r));
    else
        p = apsis_bloom_legacy_plan(s, &h);
    r->last_bloom.valid = 1u;
    r->last_bloom.sample = e->sample;
    r->last_bloom.serial++;
    r->last_bloom.harmony = h;
    r->last_bloom.plan = p;
    r->bloom_occurrence_count++;
}

static void runtime_commit_mother(ApsisRuntime *r, ApsisState *s,
                                  ApsisMotherPending *pending,
                                  const ApsisTransportEvent *e,
                                  ApsisRuntimeMotherApplyFn apply,
                                  void *user)
{
    uint8_t i;
    if (!r || !s || !pending || !e) return;
    if (!apsis_mother_should_commit(&s->mother, pending, (int32_t)e->absolute_pulse, e->clock_step)) return;
    /* A missing/failing dispatcher preserves the pending queue for a later
     * Clock-1 retry. This flag describes the current commit attempt, so a
     * previous block must not poison a later valid retry. */
    r->mother_apply_blocked = 0u;
    if (pending->count && !apply) {
        r->mother_apply_blocked = 1u;
        return;
    }
    apsis_mother_begin_commit(pending);
    for (i = 0u; i < pending->count; ++i) {
        const ApsisMotherPendingItem *item = apsis_mother_pending_at(pending, i);
        if (item && apply && !apply(s, item, user)) r->mother_apply_blocked = 1u;
    }
    if (r->mother_apply_blocked) {
        pending->applying = 0u;
        return;
    }
    if (pending->gravity_pending)
        runtime_apply_gravity(s, pending->gravity_degree, pending->gravity_ring);
    apsis_mother_finish_commit(&s->mother, pending);
}

void apsis_runtime_reset(ApsisRuntime *r, uint32_t sample_rate,
                         uint64_t first_sample, uint32_t random_seed)
{
    if (!r) return;
    runtime_zero(r, (uint32_t)sizeof(*r));
    apsis_transport_reset(&r->transport, sample_rate, first_sample);
    apsis_event_queue_reset(&r->queue);
    r->cursor_sample = first_sample;
    r->random_state = random_seed ? random_seed : 0x41505349u;
}

void apsis_runtime_start(ApsisRuntime *r)
{
    if (r) apsis_transport_start(&r->transport);
}

void apsis_runtime_stop(ApsisRuntime *r)
{
    if (r) apsis_transport_stop(&r->transport);
}

uint8_t apsis_runtime_request_gravity(ApsisRuntime *r,
                                      ApsisState *s,
                                      ApsisMotherPending *mother_pending,
                                      uint8_t degree, uint8_t ring,
                                      uint64_t now_sample,
                                      uint32_t min_lead_samples)
{
    int32_t last_abs;
    if (!r || !s) return 0u;
    degree = degree > 7u ? 7u : degree;
    ring = ring > 2u ? 2u : ring;
    last_abs = r->transport.absolute_pulse ? (int32_t)(r->transport.absolute_pulse - 1u) : -1;
    if (mother_pending && apsis_mother_should_hold(&s->mother, mother_pending,
                                                    APSIS_MOTHER_DOMAIN_STRUCTURAL)) {
        r->gravity_pending.active = 0u;
        apsis_mother_queue_gravity(mother_pending, degree, ring, last_abs);
        return 2u;
    }
    if (s->gravity.change_timing == APSIS_CHANGE_IMMEDIATE || !r->transport.running) {
        r->gravity_pending.active = 0u;
        runtime_apply_gravity(s, degree, ring);
        return 1u;
    }
    s->gravity.change_grid = apsis_transport_gravity_division(s->gravity.change_grid, s->clock.n);
    r->gravity_pending.active = 1u;
    r->gravity_pending.degree = degree;
    r->gravity_pending.ring = ring;
    r->gravity_pending.target_sample = apsis_transport_next_gravity_boundary(
        &r->transport, &s->clock, s->gravity.change_grid, now_sample, min_lead_samples);
    return 3u;
}

void apsis_runtime_chain_start(ApsisRuntime *r, ApsisState *s,
                               ApsisChainSequence *seq)
{
    uint8_t n;
    if (!r || !s || !seq || !seq->count) return;
    n = runtime_clock_n(s->clock.n);
    r->chain_meter_n = n;
    seq->playing = 1u;
    seq->armed = 1u;
    seq->pos = 0u;
    seq->pulse = 0u;
    seq->phrase_pulse = 0u;
    seq->hold_tail = 0u;
    seq->last_applied = 1u;
    seq->master_pulses = apsis_chain_master_pulses(seq, &s->chain, n);
    r->chain_morph_active = 0u;
}

void apsis_runtime_chain_stop(ApsisRuntime *r, ApsisChainSequence *seq)
{
    if (!r || !seq) return;
    seq->playing = 0u;
    seq->armed = 0u;
    seq->pulse = 0u;
    seq->phrase_pulse = 0u;
    seq->hold_tail = 0u;
    seq->master_pulses = 0u;
    seq->last_applied = 1u;
    r->chain_meter_n = 0u;
    r->chain_morph_active = 0u;
}

void apsis_runtime_process(ApsisRuntime *r,
                           ApsisState *s,
                           ApsisKeepBank *bank,
                           ApsisChainSequence *chain,
                           ApsisMotherPending *mother_pending,
                           ApsisBloomVoicingState *bloom_voicing,
                           uint32_t frames,
                           ApsisRuntimeMotherApplyFn mother_apply,
                           void *mother_user)
{
    uint64_t end;
    if (!r || !s || !frames) return;
    end = r->cursor_sample + frames;
    runtime_apply_chain_morph(r, s, r->cursor_sample);

    while (r->transport.running && runtime_next_transport_sample(&r->transport) < end) {
        uint8_t added = apsis_transport_schedule_next(&r->transport, s, &r->queue, runtime_random(r));
        runtime_update_high_water(r);
        if (!added) break;
    }

    for (;;) {
        const ApsisTransportEvent *peek = apsis_event_queue_peek(&r->queue);
        ApsisTransportEvent e;
        if (!peek || peek->sample >= end) break;
        apsis_event_queue_pop(&r->queue, &e);
        runtime_apply_chain_morph(r, s, e.sample);
        runtime_commit_due_gravity(r, s, e.sample);
        if (e.type < APSIS_RUNTIME_EVENT_TYPES) r->event_count[e.type]++;
        r->last_event_sample = e.sample;
        switch (e.type) {
            case APSIS_EVENT_STEP_ONE_RESYNC:
                r->spiral_attractor_pos = 0u;
                break;
            case APSIS_EVENT_MOTHER_PULSE:
                runtime_commit_mother(r, s, mother_pending, &e, mother_apply, mother_user);
                break;
            case APSIS_EVENT_SPIRAL_TICK:
                runtime_advance_spiral(r, s, &e);
                break;
            case APSIS_EVENT_BLOOM:
                runtime_advance_bloom(r, s, bloom_voicing, &e);
                break;
            case APSIS_EVENT_CHAIN_PULSE:
                runtime_advance_chain(r, s, bank, chain, &e);
                break;
            default:
                break;
        }
    }

    runtime_apply_chain_morph(r, s, end);
    runtime_update_high_water(r);
    r->cursor_sample = end;
}
