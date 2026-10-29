#include "apsis_runtime.h"

/* Keep the already-validated M3b implementation intact and build the final
 * Phase-3 coordinator around it. This makes the M3c diff auditable: state,
 * KEEP/CHAIN/MOTHER helpers and transport math remain the M3b implementation,
 * while only reset/request/process are replaced below. */
#define apsis_runtime_reset apsis_runtime_reset_m3b_base
#define apsis_runtime_request_gravity apsis_runtime_request_gravity_m3b_base
#define apsis_runtime_process apsis_runtime_process_m3b_base
#include "apsis_runtime.c"
#undef apsis_runtime_reset
#undef apsis_runtime_request_gravity
#undef apsis_runtime_process

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
    if (voicing)
        p = apsis_bloom_choose_voicing(s, &h, voicing, 1u,
            m3c_random(r, 0x424c574bu), m3c_random(r, 0x424c504bu));
    else
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
