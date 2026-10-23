#include "apsis_transport.h"

static uint8_t apsis_transport_clock_n(uint8_t n)
{
    return n < 3u ? 3u : (n > 24u ? 24u : n);
}

static uint16_t apsis_transport_bpm(uint16_t bpm)
{
    return bpm < 35u ? 35u : (bpm > 180u ? 180u : bpm);
}

static uint16_t apsis_transport_swing(uint16_t swing)
{
    const uint16_t max = APSIS_Q15_MILLI(350);
    return swing > max ? max : swing;
}

static void apsis_transport_division(const ApsisClockState *c, uint8_t *num, uint8_t *den)
{
    uint8_t n = c ? c->division_num : 1u;
    uint8_t d = c ? c->division_den : 2u;
    if (n != 1u || (d != 1u && d != 2u && d != 4u)) { n = 1u; d = 2u; }
    *num = n;
    *den = d;
}

static uint64_t apsis_transport_q15_mul64(uint64_t value, uint16_t q15)
{
    uint64_t whole = value / APSIS_Q15_ONE;
    uint64_t rem = value - whole * APSIS_Q15_ONE;
    return whole * q15 + (rem * q15 + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
}

static uint64_t apsis_transport_ms_q32(uint32_t rate, uint32_t ms)
{
    uint64_t whole = (uint64_t)rate * ms;
    return ((whole << 32) + 500u) / 1000u;
}

static uint64_t apsis_transport_pair_round(uint64_t sample, uint32_t frac)
{
    return sample + (frac >= 0x80000000u ? 1u : 0u);
}

static void apsis_transport_pair_add(uint64_t *sample, uint32_t *frac, uint64_t delta_q32)
{
    uint64_t f;
    if (!sample || !frac) return;
    *sample += delta_q32 >> 32;
    f = (uint64_t)*frac + (uint32_t)delta_q32;
    *sample += f >> 32;
    *frac = (uint32_t)f;
}

static uint8_t apsis_transport_event_before(const ApsisTransportEvent *a,
                                            const ApsisTransportEvent *b)
{
    if (a->sample != b->sample) return a->sample < b->sample;
    if (a->type != b->type) return a->type < b->type;
    return a->serial < b->serial;
}

static uint8_t apsis_transport_queue_push(ApsisEventQueue *q, ApsisTransportEvent e)
{
    uint16_t pos;
    if (!q || q->count >= APSIS_TRANSPORT_QUEUE_CAP) {
        if (q) q->overflowed = 1u;
        return 0u;
    }
    e.serial = q->next_serial++;
    pos = q->count++;
    q->events[pos] = e;
    while (pos) {
        uint16_t parent = (uint16_t)((pos - 1u) / 2u);
        ApsisTransportEvent tmp;
        if (!apsis_transport_event_before(&q->events[pos], &q->events[parent])) break;
        tmp = q->events[parent]; q->events[parent] = q->events[pos]; q->events[pos] = tmp;
        pos = parent;
    }
    return 1u;
}

void apsis_event_queue_reset(ApsisEventQueue *q)
{
    if (!q) return;
    q->count = 0u;
    q->next_serial = 0u;
    q->overflowed = 0u;
}

uint16_t apsis_event_queue_count(const ApsisEventQueue *q)
{
    return q ? q->count : 0u;
}

const ApsisTransportEvent *apsis_event_queue_peek(const ApsisEventQueue *q)
{
    return q && q->count ? &q->events[0] : 0;
}

uint8_t apsis_event_queue_pop(ApsisEventQueue *q, ApsisTransportEvent *out)
{
    uint16_t pos = 0u;
    if (!q || !q->count) return 0u;
    if (out) *out = q->events[0];
    q->count--;
    if (!q->count) return 1u;
    q->events[0] = q->events[q->count];
    for (;;) {
        uint16_t left = (uint16_t)(pos * 2u + 1u), right = (uint16_t)(left + 1u), best = pos;
        ApsisTransportEvent tmp;
        if (left < q->count && apsis_transport_event_before(&q->events[left], &q->events[best])) best = left;
        if (right < q->count && apsis_transport_event_before(&q->events[right], &q->events[best])) best = right;
        if (best == pos) break;
        tmp = q->events[pos]; q->events[pos] = q->events[best]; q->events[best] = tmp;
        pos = best;
    }
    return 1u;
}

void apsis_transport_reset(ApsisTransport *t, uint32_t sample_rate, uint64_t first_sample)
{
    if (!t) return;
    t->sample_rate = sample_rate ? sample_rate : APSIS_TRANSPORT_DEFAULT_RATE;
    t->next_sample = first_sample;
    t->next_frac_q32 = 0u;
    t->absolute_pulse = 0u;
    t->melody_step = 0u;
    t->cycle_pos = 0u;
    t->running = 0u;
}

void apsis_transport_start(ApsisTransport *t) { if (t) t->running = 1u; }
void apsis_transport_stop(ApsisTransport *t) { if (t) t->running = 0u; }

uint64_t apsis_transport_interval_q32(const ApsisClockState *c,
                                      uint32_t sample_rate,
                                      uint32_t absolute_pulse)
{
    uint8_t num, den;
    uint16_t bpm, swing;
    uint64_t numerator, divisor, base, delta;
    uint32_t rate = sample_rate ? sample_rate : APSIS_TRANSPORT_DEFAULT_RATE;
    if (!c) return ((uint64_t)18375u << 32);
    apsis_transport_division(c, &num, &den);
    bpm = apsis_transport_bpm(c->bpm);
    swing = apsis_transport_swing(c->swing_q15);
    numerator = (uint64_t)rate * 60u * num;
    numerator <<= 32;
    divisor = (uint64_t)bpm * den;
    base = (numerator + divisor / 2u) / divisor;
    delta = apsis_transport_q15_mul64(base, swing);
    return (absolute_pulse & 1u) ? base + delta : base - delta;
}

uint64_t apsis_transport_round_sample(uint64_t sample_q32)
{
    return (sample_q32 >> 32) + ((uint32_t)sample_q32 >= 0x80000000u ? 1u : 0u);
}

uint8_t apsis_transport_gravity_division(uint8_t requested_parts, uint8_t clock_n)
{
    uint8_t n = apsis_transport_clock_n(clock_n);
    uint8_t want = requested_parts ? requested_parts : 2u;
    if (want <= n && n % want == 0u) return want;
    if (n & 1u) return 1u;
    if (n % 2u == 0u) return 2u;
    return 1u;
}

uint8_t apsis_transport_is_gravity_boundary(uint8_t clock_step, uint8_t clock_n,
                                            uint8_t requested_parts)
{
    uint8_t n = apsis_transport_clock_n(clock_n);
    uint8_t parts = apsis_transport_gravity_division(requested_parts, n);
    uint8_t stride = (uint8_t)(n / parts);
    uint8_t pos = (uint8_t)(clock_step % n);
    return stride && pos % stride == 0u;
}

uint64_t apsis_transport_next_gravity_boundary(const ApsisTransport *t,
                                               const ApsisClockState *c,
                                               uint8_t requested_parts,
                                               uint64_t now_sample,
                                               uint32_t min_lead_samples)
{
    uint64_t sample;
    uint32_t frac, abs;
    uint8_t pos, n, guard;
    uint64_t need = now_sample + min_lead_samples;
    if (!t || !c) return need;
    sample = t->next_sample;
    frac = t->next_frac_q32;
    abs = t->absolute_pulse;
    pos = t->cycle_pos;
    n = apsis_transport_clock_n(c->n);
    for (guard = 0u; guard < 160u; ++guard) {
        if (apsis_transport_pair_round(sample, frac) >= need &&
            apsis_transport_is_gravity_boundary(pos, n, requested_parts))
            return apsis_transport_pair_round(sample, frac);
        apsis_transport_pair_add(&sample, &frac,
            apsis_transport_interval_q32(c, t->sample_rate, abs));
        abs++;
        pos++;
        if (pos >= n) pos = 0u;
    }
    return apsis_transport_pair_round(sample, frac);
}

static uint8_t apsis_transport_mask_has(uint32_t mask, uint8_t step)
{
    return step < 24u && (mask & (1u << step)) != 0u;
}

static uint8_t apsis_transport_clock_bound(const ApsisClockState *c, uint8_t step)
{
    uint8_t i;
    if (!c) return 0u;
    for (i = 0u; i < 3u; ++i) if (c->bounds[i] == step) return 1u;
    return 0u;
}

static uint8_t apsis_transport_valid_every(uint8_t every)
{
    return every == 1u || every == 2u || every == 4u || every == 8u ? every : 2u;
}

static uint8_t apsis_transport_spiral_rate(uint8_t rate)
{
    return rate == 1u || rate == 2u || rate == 4u ? rate : 2u;
}

static uint8_t apsis_transport_bloom_rate(uint8_t rate)
{
    return rate == 1u || rate == 2u || rate == 4u ? rate : 1u;
}

static uint8_t apsis_transport_bloom_cycles(uint8_t cycles)
{
    return cycles == 0u || cycles == 1u || cycles == 2u || cycles == 4u || cycles == 8u ? cycles : 2u;
}

static uint8_t apsis_transport_bloom_point(uint8_t step, uint8_t n, uint8_t start, uint8_t rate)
{
    uint8_t k;
    for (k = 0u; k < rate; ++k) {
        uint8_t offset = (uint8_t)(((uint16_t)k * n + rate / 2u) / rate);
        if ((uint8_t)((start + offset) % n) == step) return 1u;
    }
    return 0u;
}

static uint8_t apsis_transport_ghost_hit(uint16_t ghost_q15, uint32_t random_u32)
{
    uint16_t ghost = ghost_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : ghost_q15;
    return (uint64_t)random_u32 * APSIS_Q15_ONE < ((uint64_t)ghost << 32) ? 1u : 0u;
}

static uint64_t apsis_transport_target_sample(uint64_t base_sample, uint32_t base_frac,
                                              uint64_t offset_q32)
{
    apsis_transport_pair_add(&base_sample, &base_frac, offset_q32);
    return apsis_transport_pair_round(base_sample, base_frac);
}

static uint8_t apsis_transport_local_add(ApsisTransportEvent *local, uint8_t *count,
                                         uint8_t type, uint64_t sample,
                                         uint64_t interval_q32, uint32_t abs,
                                         uint8_t clock_step, uint8_t sub_index,
                                         uint8_t sub_count, uint8_t flags)
{
    ApsisTransportEvent *e;
    if (!local || !count || *count >= APSIS_TRANSPORT_PULSE_EVENTS_MAX) return 0u;
    e = &local[(*count)++];
    e->sample = sample;
    e->interval_q32 = interval_q32;
    e->absolute_pulse = abs;
    e->serial = 0u;
    e->type = type;
    e->clock_step = clock_step;
    e->sub_index = sub_index;
    e->sub_count = sub_count;
    e->flags = flags;
    return 1u;
}

uint8_t apsis_transport_schedule_next(ApsisTransport *t,
                                      const ApsisState *s,
                                      ApsisEventQueue *q,
                                      uint32_t ghost_random_u32)
{
    ApsisTransportEvent local[APSIS_TRANSPORT_PULSE_EVENTS_MAX];
    uint8_t count = 0u, i, n, main_on, accent, flags = 0u, spiral_rate;
    uint8_t bloom_rate, bloom_cycles, bloom_step;
    uint32_t abs, bloom_cycle;
    uint64_t interval_q32, pulse_sample, half_q32;
    if (!t || !s || !q || !t->running) return 0u;

    n = apsis_transport_clock_n(s->clock.n);
    i = t->cycle_pos;
    if (i >= n) i = 0u;
    abs = t->absolute_pulse;
    interval_q32 = apsis_transport_interval_q32(&s->clock, t->sample_rate, abs);
    pulse_sample = apsis_transport_pair_round(t->next_sample, t->next_frac_q32);
    half_q32 = interval_q32 / 2u;

    if (i == 0u) flags |= APSIS_EVENT_FLAG_STEP_ONE;
    if (apsis_transport_is_gravity_boundary(i, n, s->gravity.change_grid))
        flags |= APSIS_EVENT_FLAG_GRAVITY_GRID;

    if (i == 0u && s->clock.step_one_resync)
        apsis_transport_local_add(local, &count, APSIS_EVENT_STEP_ONE_RESYNC,
            pulse_sample, interval_q32, abs, i, 0u, 1u, flags);

    apsis_transport_local_add(local, &count, APSIS_EVENT_STRING_FILTER_CLOCK,
        pulse_sample, interval_q32, abs, i, 0u, 1u, flags);
    apsis_transport_local_add(local, &count, APSIS_EVENT_PLAYHEAD,
        pulse_sample, interval_q32, abs, i, 0u, 1u, flags);
    apsis_transport_local_add(local, &count, APSIS_EVENT_MOTHER_PULSE,
        pulse_sample, interval_q32, abs, i, 0u, 1u, flags);

    main_on = apsis_transport_mask_has(s->clock.main_mask, i);
    accent = main_on && (i == 0u || apsis_transport_clock_bound(&s->clock, i));
    if (main_on)
        apsis_transport_local_add(local, &count, APSIS_EVENT_CLOCK_MAIN,
            pulse_sample, interval_q32, abs, i, 0u, 1u,
            (uint8_t)(flags | (accent ? APSIS_EVENT_FLAG_ACCENT : 0u)));

    spiral_rate = apsis_transport_spiral_rate(s->spiral.attractor_rate);
    for (i = 0u; i < spiral_rate; ++i) {
        uint64_t offset = ((interval_q32 / spiral_rate) * i) +
            (((interval_q32 % spiral_rate) * i) / spiral_rate);
        apsis_transport_local_add(local, &count, APSIS_EVENT_SPIRAL_TICK,
            apsis_transport_target_sample(t->next_sample, t->next_frac_q32, offset),
            interval_q32, abs, t->cycle_pos < n ? t->cycle_pos : 0u,
            i, spiral_rate, flags);
    }

    i = t->cycle_pos < n ? t->cycle_pos : 0u;
    if (s->string.sound && s->string.rhythm != APSIS_STRING_RHYTHM_OFF &&
        (s->string.pulse_diag == APSIS_STRING_PULSE_FULL ||
         s->string.pulse_diag == APSIS_STRING_PULSE_EXCITE)) {
        uint8_t every = apsis_transport_valid_every(s->string.rhythm_every);
        uint8_t trigger = s->string.rhythm_trigger == APSIS_STRING_TRIGGER_SEQUENCE
            ? apsis_transport_mask_has(s->string.rhythm_mask, i)
            : (uint8_t)(i % every == 0u);
        if (trigger)
            apsis_transport_local_add(local, &count, APSIS_EVENT_STRING_RHYTHM,
                apsis_transport_target_sample(t->next_sample, t->next_frac_q32,
                    apsis_transport_ms_q32(t->sample_rate, 26u)),
                interval_q32, abs, i, 0u, 1u, flags);
    }

    bloom_cycles = apsis_transport_bloom_cycles(s->bloom.cycles);
    bloom_rate = apsis_transport_bloom_rate(s->bloom.rate);
    bloom_step = s->bloom.trigger_step >= n ? (uint8_t)(n - 1u) : s->bloom.trigger_step;
    bloom_cycle = abs / n;
    if (s->bloom.sound && bloom_cycles > 0u &&
        apsis_transport_bloom_point(i, n, bloom_step, bloom_rate) &&
        bloom_cycle > 0u && bloom_cycle % bloom_cycles == 0u) {
        uint32_t delay_ms = s->bloom.delay_ms > 320u ? 320u : s->bloom.delay_ms;
        apsis_transport_local_add(local, &count, APSIS_EVENT_BLOOM,
            apsis_transport_target_sample(t->next_sample, t->next_frac_q32,
                apsis_transport_ms_q32(t->sample_rate, delay_ms)),
            interval_q32, abs, i, 0u, 1u, flags);
    }

    if (s->clock.sound) {
        if (apsis_transport_mask_has(s->clock.snap_mask, i))
            apsis_transport_local_add(local, &count, APSIS_EVENT_CLOCK_SNAP,
                apsis_transport_target_sample(t->next_sample, t->next_frac_q32,
                    apsis_transport_ms_q32(t->sample_rate, 4u)),
                interval_q32, abs, i, 0u, 1u, flags);
        if (apsis_transport_mask_has(s->clock.inner_mask, i))
            apsis_transport_local_add(local, &count, APSIS_EVENT_CLOCK_INNER,
                apsis_transport_target_sample(t->next_sample, t->next_frac_q32, half_q32),
                interval_q32, abs, i, 0u, 1u, flags);
        if (t->melody_step % 3u == 2u && apsis_transport_ghost_hit(s->clock.ghost_q15, ghost_random_u32))
            apsis_transport_local_add(local, &count, APSIS_EVENT_CLOCK_GHOST,
                apsis_transport_target_sample(t->next_sample, t->next_frac_q32, half_q32),
                interval_q32, abs, i, 0u, 1u, flags);
    }

    apsis_transport_local_add(local, &count, APSIS_EVENT_CHAIN_PULSE,
        pulse_sample, interval_q32, abs, i, 0u, 1u, flags);

    if ((uint32_t)q->count + count > APSIS_TRANSPORT_QUEUE_CAP) {
        q->overflowed = 1u;
        return 0u;
    }
    for (i = 0u; i < count; ++i)
        if (!apsis_transport_queue_push(q, local[i])) return 0u;

    t->melody_step++;
    t->absolute_pulse++;
    t->cycle_pos++;
    if (t->cycle_pos >= n) t->cycle_pos = 0u;
    apsis_transport_pair_add(&t->next_sample, &t->next_frac_q32, interval_q32);
    return count;
}
