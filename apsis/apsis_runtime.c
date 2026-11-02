#include "apsis_runtime_m4g.h"

/* Compile the already-proven M4f coordinator unchanged into this translation
 * unit, then append one Bloom-aware process entry point. Static M3c/M4c/M4f
 * helpers stay available below without modifying their implementations. */
#include "apsis_runtime_m4f.h"

/* Compile the already-proven M4c coordinator unchanged into this translation
 * unit, then append one explicit Spiral-audio process entry point. Static M3c/
 * M4c helpers remain available below without duplicating their implementations. */
/* M4c String Attractor runtime layer.
 *
 * This repeats the tiny final M3c lifecycle/Rhythm wrapper while including the
 * validated M3c coordinator itself unchanged. The original public
 * apsis_runtime_process() therefore remains the exact M3c implementation; only
 * explicit M4c entry points use the Attractor-aware schedule/consumer path.
 */
#include "apsis_runtime.h"

/* Keep the already-validated M3b implementation intact and build the final
 * Phase-3 coordinator around it. This makes the M3c diff auditable: state,
 * KEEP/CHAIN/MOTHER helpers and transport math remain the M3b implementation,
 * while only reset/request/process are replaced below. */
/* ---- apsis_runtime.c ---- */
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

static void apsis_runtime_reset_m3b_base(ApsisRuntime *r, uint32_t sample_rate,
                         uint64_t first_sample, uint32_t random_seed)
{
    if (!r) return;
    runtime_zero(r, (uint32_t)sizeof(*r));
    apsis_transport_reset(&r->transport, sample_rate, first_sample);
    apsis_event_queue_reset(&r->queue);
    r->cursor_sample = first_sample;
    r->random_state = random_seed ? random_seed : 0x41505349u;
}



static uint8_t apsis_runtime_request_gravity_m3b_base(ApsisRuntime *r,
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


/* ---- apsis_runtime_m3c.c ---- */

#define M3C_HASH_OFFSET 2166136261u
#define M3C_HASH_PRIME 16777619u
#define M3C_PULSE_RESERVE (APSIS_TRANSPORT_PULSE_EVENTS_MAX + 7u)

typedef struct {
    uint8_t valid;
    uint8_t route_index;
    uint8_t trigger_pos;
    uint16_t energy_q15;
} M3cSpiralDecision;

static uint32_t m3c_hash_u32(uint32_t hash, uint32_t value)
{
    hash ^= value;
    return hash * M3C_HASH_PRIME;
}

static void m3c_trace(ApsisRuntime *r, uint32_t tag, uint64_t sample,
                      uint32_t a, uint32_t b)
{
    uint32_t h;
    if (!r) return;
    h = r->late_trace_hash ? r->late_trace_hash : M3C_HASH_OFFSET;
    h = m3c_hash_u32(h, tag);
    h = m3c_hash_u32(h, (uint32_t)sample);
    h = m3c_hash_u32(h, (uint32_t)(sample >> 32));
    h = m3c_hash_u32(h, a);
    h = m3c_hash_u32(h, b);
    r->late_trace_hash = h;
}

static uint32_t m3c_random(ApsisRuntime *r, uint32_t tag)
{
    uint32_t v = runtime_random(r);
    uint32_t h;
    if (!r) return v;
    r->random_draw_count++;
    h = r->random_trace_hash ? r->random_trace_hash : M3C_HASH_OFFSET;
    h = m3c_hash_u32(h, tag);
    h = m3c_hash_u32(h, v);
    r->random_trace_hash = h;
    return v;
}

static uint64_t m3c_ms_samples(uint32_t sample_rate, uint32_t ms)
{
    uint32_t whole_khz, rem_hz, rem_samples;
    if (!sample_rate) sample_rate = APSIS_TRANSPORT_DEFAULT_RATE;
    whole_khz = sample_rate / 1000u;
    rem_hz = sample_rate % 1000u;
    rem_samples = (rem_hz * ms + 500u) / 1000u;
    return (uint64_t)whole_khz * ms + rem_samples;
}

static uint64_t m3c_sub_ms(uint64_t sample, uint32_t sample_rate, uint32_t ms)
{
    uint64_t delta = m3c_ms_samples(sample_rate, ms);
    return sample > delta ? sample - delta : 0u;
}

static uint8_t m3c_event_before(const ApsisTransportEvent *a,
                                const ApsisTransportEvent *b)
{
    if (a->sample != b->sample) return a->sample < b->sample;
    if (a->type != b->type) return a->type < b->type;
    return a->serial < b->serial;
}

static uint8_t m3c_queue_push(ApsisRuntime *r, ApsisTransportEvent e)
{
    ApsisEventQueue *q;
    uint16_t pos;
    if (!r) return 0u;
    q = &r->queue;
    if (q->count >= APSIS_TRANSPORT_QUEUE_CAP) {
        q->overflowed = 1u;
        r->queue_overflowed = 1u;
        return 0u;
    }
    e.serial = q->next_serial++;
    pos = q->count++;
    q->events[pos] = e;
    q->events[pos].reserved[0] = q->events[pos].reserved[1] = q->events[pos].reserved[2] = 0u;
    while (pos) {
        uint16_t parent = (uint16_t)((pos - 1u) / 2u);
        ApsisTransportEvent tmp;
        if (!m3c_event_before(&q->events[pos], &q->events[parent])) break;
        tmp = q->events[parent];
        q->events[parent] = q->events[pos];
        q->events[pos] = tmp;
        pos = parent;
    }
    runtime_update_high_water(r);
    return 1u;
}

static ApsisTransportEvent m3c_event(uint8_t type, uint64_t sample,
                                     uint64_t target_sample)
{
    ApsisTransportEvent e;
    runtime_zero(&e, (uint32_t)sizeof(e));
    e.type = type;
    e.sample = sample;
    e.interval_q32 = target_sample; /* runtime-only events use this as payload */
    return e;
}

static ApsisLatePendingGravity m3c_pending_gravity(const ApsisRuntime *r)
{
    ApsisLatePendingGravity p;
    runtime_zero(&p, (uint32_t)sizeof(p));
    if (!r) return p;
    p.active = r->gravity_pending.active;
    p.degree = r->gravity_pending.degree;
    p.ring = r->gravity_pending.ring;
    p.target_sample = r->gravity_pending.target_sample;
    return p;
}

static uint8_t m3c_spiral_slot_alloc(ApsisRuntime *r, uint32_t token,
                                     uint64_t target, uint8_t route_index,
                                     uint8_t trigger_pos, uint16_t energy)
{
    uint8_t i, free_ix = APSIS_RUNTIME_SPIRAL_SLOTS;
    uint16_t active = 0u;
    if (!r) return APSIS_RUNTIME_SPIRAL_SLOTS;
    for (i = 0u; i < APSIS_RUNTIME_SPIRAL_SLOTS; ++i) {
        if (r->spiral_slots[i].active) active++;
        else if (free_ix == APSIS_RUNTIME_SPIRAL_SLOTS) free_ix = i;
    }
    if (free_ix == APSIS_RUNTIME_SPIRAL_SLOTS) {
        r->late_slot_overflowed = 1u;
        return free_ix;
    }
    r->spiral_slots[free_ix].active = 1u;
    r->spiral_slots[free_ix].token = token;
    r->spiral_slots[free_ix].target_sample = target;
    r->spiral_slots[free_ix].route_index = route_index;
    r->spiral_slots[free_ix].trigger_pos = trigger_pos;
    r->spiral_slots[free_ix].energy_q15 = energy;
    active++;
    if (active > r->spiral_slot_high_water) r->spiral_slot_high_water = active;
    return free_ix;
}

static uint8_t m3c_bloom_slot_alloc(ApsisRuntime *r, uint32_t token,
                                    uint64_t occurrence, uint64_t target,
                                    uint16_t unfurl_ms,
                                    const ApsisBloomHarmony *h,
                                    const ApsisBloomPlan *plan)
{
    uint8_t i, free_ix = APSIS_RUNTIME_BLOOM_SLOTS;
    uint16_t active = 0u;
    ApsisRuntimeBloomSlot *slot;
    if (!r || !h || !plan || !plan->count) return APSIS_RUNTIME_BLOOM_SLOTS;
    for (i = 0u; i < APSIS_RUNTIME_BLOOM_SLOTS; ++i) {
        if (r->bloom_slots[i].active) active++;
        else if (free_ix == APSIS_RUNTIME_BLOOM_SLOTS) free_ix = i;
    }
    if (free_ix == APSIS_RUNTIME_BLOOM_SLOTS) {
        r->late_slot_overflowed = 1u;
        return free_ix;
    }
    slot = &r->bloom_slots[free_ix];
    runtime_zero(slot, (uint32_t)sizeof(*slot));
    slot->active = 1u;
    slot->token = token;
    slot->occurrence_sample = occurrence;
    slot->target_sample = target;
    slot->unfurl_ms = unfurl_ms > 3600u ? 3600u : unfurl_ms;
    slot->harmony = *h;
    slot->plan = *plan;
    slot->voice_count = plan->count > APSIS_BLOOM_VOICES_MAX
        ? APSIS_BLOOM_VOICES_MAX : plan->count;
    active++;
    if (active > r->bloom_slot_high_water) r->bloom_slot_high_water = active;
    return free_ix;
}

static uint64_t m3c_bloom_voice_target(const ApsisRuntime *r,
                                       const ApsisRuntimeBloomSlot *slot,
                                       uint8_t voice)
{
    uint32_t total, offset, den;
    if (!r || !slot || slot->voice_count <= 1u) return slot ? slot->target_sample : 0u;
    total = (uint32_t)m3c_ms_samples(r->transport.sample_rate, slot->unfurl_ms);
    den = (uint32_t)slot->voice_count - 1u;
    offset = (total * voice + den / 2u) / den;
    return slot->target_sample + offset;
}

static uint8_t m3c_schedule_bloom_bind(ApsisRuntime *r, uint8_t slot_ix,
                                       uint8_t voice)
{
    ApsisRuntimeBloomSlot *slot;
    ApsisTransportEvent e;
    uint64_t target, bind;
    if (!r || slot_ix >= APSIS_RUNTIME_BLOOM_SLOTS) return 0u;
    slot = &r->bloom_slots[slot_ix];
    if (!slot->active || voice >= slot->voice_count) return 0u;
    target = m3c_bloom_voice_target(r, slot, voice);
    bind = apsis_late_bloom_bind_sample(slot->occurrence_sample, target,
                                        r->transport.sample_rate);
    e = m3c_event(APSIS_EVENT_BLOOM_BIND, bind, target);
    e.absolute_pulse = slot->token;
    e.sub_index = slot_ix;
    e.sub_count = voice;
    if (!m3c_queue_push(r, e)) return 0u;
    slot->next_voice = (uint8_t)(voice + 1u);
    return 1u;
}

static uint8_t m3c_spiral_rate(uint8_t rate)
{
    return (rate == 1u || rate == 2u || rate == 4u) ? rate : 2u;
}

static uint8_t m3c_prepare_spiral(ApsisRuntime *r, ApsisState *s,
                                  M3cSpiralDecision out[4])
{
    int16_t seq[APSIS_SPIRAL_ROUTE_MAX];
    uint8_t seq_n, rate, len, sub;
    if (!r || !s || !out) return 0u;
    runtime_zero(out, (uint32_t)(sizeof(M3cSpiralDecision) * 4u));
    if (r->transport.cycle_pos == 0u && s->clock.step_one_resync)
        r->spiral_attractor_pos = 0u;
    seq_n = apsis_spiral_derived_sequence(s, seq);
    rate = m3c_spiral_rate(s->spiral.attractor_rate);
    len = apsis_spiral_attractor_length(s->spiral.attractor_length);

    for (sub = 0u; sub < rate; ++sub) {
        uint8_t pos = (uint8_t)(r->spiral_attractor_pos % len);
        uint16_t lock = s->spiral.attractor_lock_q15 > APSIS_Q15_ONE
            ? APSIS_Q15_ONE : s->spiral.attractor_lock_q15;
        uint16_t density = s->spiral.attractor_density_q15 > APSIS_Q15_ONE
            ? APSIS_Q15_ONE : s->spiral.attractor_density_q15;
        uint32_t rnd = m3c_random(r, 0x53000000u | pos);
        uint8_t gate;
        uint16_t energy;
        if (runtime_random_q15(rnd) > lock) {
            uint32_t gate_rnd = m3c_random(r, 0x53100000u | pos);
            uint32_t energy_rnd = m3c_random(r, 0x53200000u | pos);
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
        if (gate && seq_n) {
            out[sub].valid = 1u;
            out[sub].route_index = (uint8_t)(r->spiral_step % seq_n);
            out[sub].trigger_pos = pos;
            out[sub].energy_q15 = energy;
            r->spiral_step++;
        }
    }
    return rate;
}

static void m3c_schedule_spiral_occurrence(ApsisRuntime *r, ApsisState *s,
                                           const ApsisTransportEvent *target,
                                           const M3cSpiralDecision *d)
{
    ApsisTransportEvent bind;
    uint32_t token;
    uint8_t slot_ix;
    if (!r || !s || !target || !d || !d->valid) return;
    token = r->last_spiral.serial + 1u;
    r->last_spiral.valid = 1u;
    r->last_spiral.sample = target->sample;
    r->last_spiral.serial = token;
    r->last_spiral.route_index = d->route_index;
    r->last_spiral.route_point = d->route_index < s->spiral.route_count
        ? s->spiral.route[d->route_index] : 0u;
    r->last_spiral.trigger_pos = d->trigger_pos;
    r->last_spiral.energy_q15 = d->energy_q15;
    r->spiral_occurrence_count++;
    m3c_trace(r, 0x53504f43u, target->sample, token,
              ((uint32_t)d->route_index << 16) | d->energy_q15);

    /* tick() still creates/advances the musical occurrence while muted, but
     * scheduleSpiralBoundVoice() is only called when the source is on. */
    if (!s->spiral.sound) return;
    slot_ix = m3c_spiral_slot_alloc(r, token, target->sample, d->route_index,
                                    d->trigger_pos, d->energy_q15);
    if (slot_ix >= APSIS_RUNTIME_SPIRAL_SLOTS) return;
    bind = m3c_event(APSIS_EVENT_SPIRAL_BIND,
                     apsis_late_spiral_bind_sample(target->sample,
                                                   r->transport.sample_rate),
                     target->sample);
    bind.absolute_pulse = token;
    bind.sub_index = slot_ix;
    m3c_queue_push(r, bind);
}

static void m3c_attach_transport_events(ApsisRuntime *r, ApsisState *s,
                                        uint32_t first_serial, uint32_t end_serial,
                                        const M3cSpiralDecision spiral[4])
{
    uint16_t i, count;
    if (!r || !s) return;
    count = r->queue.count;
    for (i = 0u; i < count; ++i) {
        const ApsisTransportEvent *src = &r->queue.events[i];
        ApsisTransportEvent e;
        if (src->serial < first_serial || src->serial >= end_serial) continue;
        if (src->type == APSIS_EVENT_SPIRAL_TICK && src->sub_index < 4u) {
            m3c_schedule_spiral_occurrence(r, s, src, &spiral[src->sub_index]);
        } else if (src->type == APSIS_EVENT_BLOOM) {
            uint64_t occurrence = apsis_late_bloom_occurrence_sample(
                src->sample, r->transport.sample_rate);
            e = m3c_event(APSIS_EVENT_BLOOM_OCCURRENCE, occurrence, src->sample);
            e.absolute_pulse = src->absolute_pulse;
            e.clock_step = src->clock_step;
            m3c_queue_push(r, e);
        } else if (src->type == APSIS_EVENT_MOTHER_PULSE) {
            e = m3c_event(APSIS_EVENT_MOTHER_COMMIT_OBSERVE,
                m3c_sub_ms(src->sample, r->transport.sample_rate,
                           APSIS_RUNTIME_MOTHER_COMMIT_LEAD_MS), src->sample);
            e.absolute_pulse = src->absolute_pulse;
            e.clock_step = src->clock_step;
            m3c_queue_push(r, e);
        } else if (src->type == APSIS_EVENT_CHAIN_PULSE) {
            e = m3c_event(APSIS_EVENT_CHAIN_APPLY_OBSERVE,
                m3c_sub_ms(src->sample, r->transport.sample_rate,
                           APSIS_RUNTIME_CHAIN_APPLY_LEAD_MS), src->sample);
            e.absolute_pulse = src->absolute_pulse;
            e.clock_step = src->clock_step;
            m3c_queue_push(r, e);
        }
    }
}

static uint8_t m3c_schedule_next_pulse(ApsisRuntime *r, ApsisState *s)
{
    M3cSpiralDecision spiral[4];
    uint32_t first_serial, end_serial, ghost = 0u;
    uint8_t added;
    if (!r || !s || !r->transport.running) return 0u;
    if ((uint32_t)r->queue.count + M3C_PULSE_RESERVE > APSIS_TRANSPORT_QUEUE_CAP) {
        r->scheduler_starved = 1u;
        return 0u;
    }

    m3c_prepare_spiral(r, s, spiral);
    /* Source order: Spiral attractor random draws happen before the occasional
     * Clock ghost draw near the end of tick(). */
    if (s->clock.sound && (r->transport.melody_step % 3u) == 2u)
        ghost = m3c_random(r, 0x47484f53u);

    first_serial = r->queue.next_serial;
    added = apsis_transport_schedule_next(&r->transport, s, &r->queue, ghost);
    runtime_update_high_water(r);
    if (!added) return 0u;
    end_serial = r->queue.next_serial;
    m3c_attach_transport_events(r, s, first_serial, end_serial, spiral);
    return added;
}

static void m3c_handle_gravity_commit(ApsisRuntime *r, ApsisState *s,
                                      const ApsisTransportEvent *e)
{
    if (!r || !s || !e || !r->gravity_pending.active) return;
    if (e->absolute_pulse != r->gravity_pending.token) return;
    runtime_apply_gravity(s, r->gravity_pending.degree, r->gravity_pending.ring);
    r->gravity_pending.active = 0u;
    m3c_trace(r, 0x47524156u, e->sample, e->absolute_pulse,
              ((uint32_t)s->gravity.degree << 8) | s->gravity.voicing);
}

static void m3c_handle_spiral_bind(ApsisRuntime *r, ApsisState *s,
                                   const ApsisTransportEvent *e)
{
    ApsisRuntimeSpiralSlot *slot;
    ApsisRuntimeSpiralBinding *last;
    uint8_t ok = 0u;
    if (!r || !s || !e || e->sub_index >= APSIS_RUNTIME_SPIRAL_SLOTS) return;
    slot = &r->spiral_slots[e->sub_index];
    if (!slot->active || slot->token != e->absolute_pulse) return;
    last = &r->last_spiral_bind;
    runtime_zero(last, (uint32_t)sizeof(*last));
    last->valid = 1u;
    last->serial = slot->token;
    last->bind_sample = e->sample;
    last->target_sample = slot->target_sample;
    last->trigger_pos = slot->trigger_pos;
    last->energy_q15 = slot->energy_q15;
    if (s->spiral.sound)
        ok = apsis_late_spiral_resolve(s, slot->route_index, &last->pitch);
    if (!ok) {
        last->skipped = 1u;
        r->spiral_bind_skipped++;
    }
    r->spiral_bind_count++;
    m3c_trace(r, 0x53504244u, e->sample, slot->token,
              ok ? ((uint32_t)(uint16_t)last->pitch.step << 16) |
                   last->pitch.tuning_mode : 0xffffffffu);
    slot->active = 0u;
}

static void m3c_handle_bloom_occurrence(ApsisRuntime *r, ApsisState *s,
                                        ApsisBloomVoicingState *voicing,
                                        const ApsisTransportEvent *e)
{
    ApsisLatePendingGravity pending;
    ApsisBloomHarmony h;
    ApsisBloomPlan p;
    ApsisTransportEvent bind;
    uint32_t token;
    uint16_t unfurl;
    uint8_t slot_ix;
    (void)bind;
    if (!r || !s || !e || !s->bloom.sound) return;
    pending = m3c_pending_gravity(r);
    h = apsis_late_bloom_harmony_at(s, &pending, e->interval_q32,
                                    r->transport.sample_rate);
    if (voicing) {
        /* walk, then pick (bloomChooseVoicingPlan); never two draws in one argument list, whose order C leaves
         * open (gcc took pick first, clang walk first) */
        uint32_t walk_random = m3c_random(r, 0x424c574bu);
        uint32_t pick_random = m3c_random(r, 0x424c504bu);
        p = apsis_bloom_choose_voicing(s, &h, voicing, 1u, walk_random, pick_random);
    } else
        p = apsis_bloom_legacy_plan(s, &h);
    if (!p.count) return;
    token = r->last_bloom.serial + 1u;
    r->last_bloom.valid = 1u;
    r->last_bloom.sample = e->interval_q32; /* musical Bloom target */
    r->last_bloom.serial = token;
    r->last_bloom.harmony = h;
    r->last_bloom.plan = p;
    r->bloom_occurrence_count++;
    unfurl = s->bloom.unfurl_ms > 3600u ? 3600u : s->bloom.unfurl_ms;
    slot_ix = m3c_bloom_slot_alloc(r, token, e->sample, e->interval_q32,
                                   unfurl, &h, &p);
    m3c_trace(r, 0x424c4f43u, e->sample, token,
              ((uint32_t)h.degree << 24) | ((uint32_t)h.voicing << 16) |
              ((uint32_t)p.count << 8) | (unfurl > 255u ? 255u : unfurl));
    if (slot_ix < APSIS_RUNTIME_BLOOM_SLOTS)
        m3c_schedule_bloom_bind(r, slot_ix, 0u);
}

static void m3c_handle_bloom_bind(ApsisRuntime *r, ApsisState *s,
                                  const ApsisTransportEvent *e)
{
    ApsisRuntimeBloomSlot *slot;
    ApsisRuntimeBloomBinding *last;
    ApsisLatePendingGravity pending;
    uint8_t voice, ok = 0u;
    if (!r || !s || !e || e->sub_index >= APSIS_RUNTIME_BLOOM_SLOTS) return;
    slot = &r->bloom_slots[e->sub_index];
    if (!slot->active || slot->token != e->absolute_pulse) return;
    voice = e->sub_count;
    if (voice >= slot->voice_count) return;
    last = &r->last_bloom_bind;
    runtime_zero(last, (uint32_t)sizeof(*last));
    last->valid = 1u;
    last->serial = slot->token;
    last->bind_sample = e->sample;
    last->target_sample = e->interval_q32;
    pending = m3c_pending_gravity(r);
    if (s->bloom.sound)
        ok = apsis_late_bloom_resolve(s, &slot->plan, voice, &pending,
                                      e->interval_q32, r->transport.sample_rate,
                                      &last->pitch);
    if (!ok) {
        last->skipped = 1u;
        r->bloom_bind_skipped++;
    }
    r->bloom_bind_count++;
    m3c_trace(r, 0x424c4244u, e->sample, slot->token,
              ok ? ((uint32_t)voice << 24) |
                   ((uint32_t)(uint16_t)last->pitch.step << 8) |
                   last->pitch.tuning_mode : 0xffffffffu);

    if ((uint8_t)(voice + 1u) < slot->voice_count) {
        if (!m3c_schedule_bloom_bind(r, e->sub_index, (uint8_t)(voice + 1u)))
            r->late_slot_overflowed = 1u;
    } else {
        slot->active = 0u;
    }
}

static void m3c_handle_chain_observe(ApsisRuntime *r, ApsisState *s,
                                     ApsisKeepBank *bank, ApsisChainSequence *chain,
                                     const ApsisTransportEvent *e)
{
    ApsisTransportEvent target;
    uint64_t old_start, old_end;
    uint32_t random_before;
    if (!r || !s || !e) return;
    target = *e;
    old_start = r->chain_morph_start_sample;
    old_end = r->chain_morph_end_sample;
    random_before = r->random_state;
    runtime_advance_chain(r, s, bank, chain, e);
    /* If this early callback started a new continuous morph, Study 126 still
     * starts morphValue() on the audio target supplied to applyChainCell(). */
    if (r->chain_morph_active && r->chain_morph_start_sample == e->sample &&
        (r->chain_morph_start_sample != old_start || r->chain_morph_end_sample != old_end)) {
        uint64_t delta = target.interval_q32 > e->sample
            ? target.interval_q32 - e->sample : 0u;
        r->chain_morph_start_sample += delta;
        r->chain_morph_end_sample += delta;
    }
    if (r->random_state != random_before) {
        uint32_t h = r->random_trace_hash ? r->random_trace_hash : M3C_HASH_OFFSET;
        r->random_draw_count++;
        h = m3c_hash_u32(h, 0x4348414eu);
        h = m3c_hash_u32(h, r->random_state);
        r->random_trace_hash = h;
    }
}

void apsis_runtime_reset(ApsisRuntime *r, uint32_t sample_rate,
                         uint64_t first_sample, uint32_t random_seed)
{
    apsis_runtime_reset_m3b_base(r, sample_rate, first_sample, random_seed);
    if (!r) return;
    r->random_trace_hash = M3C_HASH_OFFSET;
    r->late_trace_hash = M3C_HASH_OFFSET;
}

uint8_t apsis_runtime_request_gravity(ApsisRuntime *r,
                                      ApsisState *s,
                                      ApsisMotherPending *mother_pending,
                                      uint8_t degree, uint8_t ring,
                                      uint64_t now_sample,
                                      uint32_t min_lead_samples)
{
    ApsisTransportEvent e;
    uint32_t token;
    uint8_t result;
    if (!r || !s) return 0u;
    token = r->gravity_pending.token + 1u;
    result = apsis_runtime_request_gravity_m3b_base(r, s, mother_pending,
        degree, ring, now_sample, min_lead_samples);
    r->gravity_pending.token = token; /* invalidates any older queued callback */
    if (result == 3u && r->gravity_pending.active) {
        uint64_t commit = m3c_sub_ms(r->gravity_pending.target_sample,
            r->transport.sample_rate, APSIS_RUNTIME_GRAVITY_COMMIT_LEAD_MS);
        if (commit < r->cursor_sample) commit = r->cursor_sample;
        e = m3c_event(APSIS_EVENT_GRAVITY_COMMIT, commit,
                      r->gravity_pending.target_sample);
        e.absolute_pulse = token;
        if (!m3c_queue_push(r, e)) r->late_slot_overflowed = 1u;
    }
    return result;
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
    uint64_t end, horizon, lookahead;
    if (!r || !s || !frames) return;
    end = r->cursor_sample + frames;
    lookahead = m3c_ms_samples(r->transport.sample_rate, APSIS_RUNTIME_LOOKAHEAD_MS);
    horizon = end + lookahead;
    if (horizon < end) horizon = ~(uint64_t)0;
    runtime_apply_chain_morph(r, s, r->cursor_sample);
    r->scheduler_starved = 0u;

    while (r->transport.running && runtime_next_transport_sample(&r->transport) < horizon) {
        if (!m3c_schedule_next_pulse(r, s)) break;
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
                /* The source resets the Spiral planner during tick() lookahead,
                 * not when the target pulse becomes audible. */
                break;
            case APSIS_EVENT_MOTHER_PULSE:
            case APSIS_EVENT_CHAIN_PULSE:
            case APSIS_EVENT_SPIRAL_TICK:
            case APSIS_EVENT_BLOOM:
                /* M3b target markers remain useful trace points. M3c performs
                 * their source callbacks at the explicit pre-target events. */
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
            default:
                break;
        }
    }

    runtime_apply_chain_morph(r, s, end);
    runtime_update_high_water(r);
    r->cursor_sample = end;
}

/* ---- apsis_runtime_m4c.c ---- */

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

/* ---- apsis_runtime_m4f.c ---- */

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

/* ---- apsis_runtime_m4g.c ---- */

void apsis_runtime_start_bloom_audio(ApsisRuntime *r,
                                     ApsisState *s,
                                     ApsisStringAttractorRuntime *a,
                                     ApsisSpiralAudioRuntime *spiral_audio,
                                     ApsisBloomAudioRuntime *bloom_audio)
{
    if (bloom_audio) apsis_bloom_audio_runtime_reset(bloom_audio);
    apsis_runtime_start_spiral_audio(r, s, a, spiral_audio);
}

void apsis_runtime_stop_bloom_audio(ApsisRuntime *r,
                                    ApsisStringAttractorRuntime *a,
                                    ApsisSpiralAudioRuntime *spiral_audio,
                                    ApsisBloomAudioRuntime *bloom_audio)
{
    if (bloom_audio) apsis_bloom_audio_runtime_reset(bloom_audio);
    apsis_runtime_stop_spiral_audio(r, a, spiral_audio);
}

uint8_t apsis_runtime_bloom_occurrence_due(const ApsisRuntime *r,
                                           uint64_t end_sample)
{
    uint16_t i;
    if (!r) return 0u;
    for (i = 0u; i < r->queue.count; ++i) {
        const ApsisTransportEvent *e = &r->queue.events[i];
        if (e->type == APSIS_EVENT_BLOOM_OCCURRENCE && e->sample < end_sample)
            return 1u;
    }
    return 0u;
}

uint8_t apsis_runtime_process_bloom_audio(
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
                                     ApsisRuntimeStringRhythmObservation *observation)
{
    ApsisTransportEvent rhythm_due;
    uint32_t rhythm_before = 0u;
    uint64_t end, horizon, lookahead;
    uint8_t have_rhythm = 0u;

    if (!bloom_audio) {
        return apsis_runtime_process_spiral_audio(
            r, s, bank, chain, mother_pending, bloom_voicing, frames,
            mother_apply, mother_user, a, spiral_audio, observation);
    }

    if (observation) runtime_zero(observation, (uint32_t)sizeof(*observation));
    if (!r || !s || !frames) return 0u;

    rhythm_before = r->event_count[APSIS_EVENT_STRING_RHYTHM];
    end = r->cursor_sample + frames;
    if (end < r->cursor_sample) end = ~(uint64_t)0;
    have_rhythm = final_due_string_rhythm(r, end, &rhythm_due);

    if (a) apsis_string_attractor_begin_process(a);
    if (spiral_audio) apsis_spiral_audio_runtime_begin_process(spiral_audio);
    apsis_bloom_audio_runtime_begin_process(bloom_audio);

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
                if (spiral_audio)
                    apsis_spiral_audio_runtime_bind(spiral_audio, r, s, &e);
                break;
            case APSIS_EVENT_BLOOM_OCCURRENCE:
                m3c_handle_bloom_occurrence(r, s, bloom_voicing, &e);
                apsis_bloom_audio_runtime_capture(bloom_audio, r, s);
                break;
            case APSIS_EVENT_BLOOM_BIND:
                m3c_handle_bloom_bind(r, s, &e);
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
