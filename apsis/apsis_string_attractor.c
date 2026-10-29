#include "apsis_string_attractor.h"

#define ATTR_LOCK_TAG_BASE 0x53410000u
#define ATTR_MUTATE_TAG_BASE 0x53410100u
#define ATTR_STRUM_EPS_Q15 3u

static void attr_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint16_t attr_clamp_q15(uint16_t v)
{
    return v > APSIS_Q15_ONE ? APSIS_Q15_ONE : v;
}

static uint16_t attr_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint16_t attr_draw_q15(uint32_t draw)
{
    return (uint16_t)(((uint64_t)draw * APSIS_Q15_ONE) >> 32);
}

static uint8_t attr_length(const ApsisState *s)
{
    uint8_t n = s ? s->string.filter_attractor_length : 8u;
    if (n < 4u) n = 4u;
    if (n > APSIS_STRING_ATTRACTOR_CELLS_MAX) n = APSIS_STRING_ATTRACTOR_CELLS_MAX;
    return n;
}

static uint16_t attr_seed(const ApsisState *s)
{
    uint16_t seed = s ? s->string.filter_attractor_seed : 317u;
    if (!seed) seed = 317u;
    if (seed > 9999u) seed = 9999u;
    return seed;
}

static uint8_t attr_allow_filter(const ApsisState *s)
{
    if (!s) return 0u;
    return s->string.pulse_diag == APSIS_STRING_PULSE_FULL ||
           s->string.pulse_diag == APSIS_STRING_PULSE_FILTER;
}

static uint8_t attr_allow_excite(const ApsisState *s)
{
    if (!s) return 0u;
    return s->string.pulse_diag == APSIS_STRING_PULSE_FULL ||
           s->string.pulse_diag == APSIS_STRING_PULSE_EXCITE;
}

static uint8_t attr_every(const ApsisState *s)
{
    uint8_t e = s ? s->string.filter_attractor_every : APSIS_STRING_ATTRACTOR_1;
    if (e > APSIS_STRING_ATTRACTOR_8) e = APSIS_STRING_ATTRACTOR_1;
    return e;
}

static uint8_t attr_route(const ApsisState *s, uint8_t pos)
{
    uint8_t target;
    if (!s) return APSIS_STRING_ATTRACTOR_ROUTE_RIGHT;
    if (s->string.surface_mode == APSIS_STRING_CLASSIC)
        return APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC;
    target = s->string.attractor_target;
    if (target > APSIS_STRING_ATTRACTOR_BOTH)
        target = APSIS_STRING_ATTRACTOR_ALTERNATE;
    if (target == APSIS_STRING_ATTRACTOR_LEFT)
        return APSIS_STRING_ATTRACTOR_ROUTE_LEFT;
    if (target == APSIS_STRING_ATTRACTOR_RIGHT)
        return APSIS_STRING_ATTRACTOR_ROUTE_RIGHT;
    if (target == APSIS_STRING_ATTRACTOR_BOTH)
        return APSIS_STRING_ATTRACTOR_ROUTE_LEFT | APSIS_STRING_ATTRACTOR_ROUTE_RIGHT;
    return (pos & 1u) ? APSIS_STRING_ATTRACTOR_ROUTE_RIGHT
                      : APSIS_STRING_ATTRACTOR_ROUTE_LEFT;
}

static uint8_t attr_should_mutate(uint32_t draw, uint16_t lock_q15)
{
    uint16_t lock = attr_clamp_q15(lock_q15);
    /* Source test is Math.random() > lock. Compare in the original 32-bit
     * random domain rather than quantizing the draw before the threshold. */
    return (uint64_t)draw * APSIS_Q15_ONE > ((uint64_t)lock << 32) ? 1u : 0u;
}

static uint16_t attr_base_level(const ApsisState *s, uint16_t value_q15)
{
    uint16_t shape, amount, user;
    if (!s) return 0u;
    amount = attr_clamp_q15(s->string.filter_attractor_strum_q15);
    user = attr_clamp_q15(s->string.filter_attractor_strum_level_q15);
    shape = (uint16_t)(APSIS_Q15_MILLI(50) +
        attr_mul_q15(APSIS_Q15_MILLI(95), attr_clamp_q15(value_q15)));
    if (shape > APSIS_Q15_ONE) shape = APSIS_Q15_ONE;
    return attr_mul_q15(attr_mul_q15(shape, amount), user);
}

void apsis_string_attractor_reset(ApsisStringAttractorRuntime *r,
                                  const ApsisState *s)
{
    uint32_t x;
    uint8_t i, n;
    if (!r) return;
    attr_zero(r, (uint32_t)sizeof(*r));
    r->seed = attr_seed(s);
    r->length = attr_length(s);
    n = r->length;
    x = r->seed;
    for (i = 0u; i < n; ++i) {
        x = x * 1664525u + 1013904223u;
        r->cell_q15[i] = attr_draw_q15(x);
    }
}

void apsis_string_attractor_sync(ApsisStringAttractorRuntime *r,
                                 const ApsisState *s)
{
    if (!r) return;
    if (!r->length || r->seed != attr_seed(s) || r->length != attr_length(s))
        apsis_string_attractor_reset(r, s);
}

uint8_t apsis_string_attractor_advances_for_pulse(const ApsisState *s,
                                                  uint8_t clock_step)
{
    uint8_t e, stride;
    if (!s || s->string.filter_mode != APSIS_STRING_FILTER_ATTRACTOR) return 0u;
    if (!attr_allow_filter(s) && !attr_allow_excite(s)) return 0u;
    e = attr_every(s);
    if (e == APSIS_STRING_ATTRACTOR_HALF) return 2u;
    stride = e == APSIS_STRING_ATTRACTOR_1 ? 1u :
             e == APSIS_STRING_ATTRACTOR_2 ? 2u :
             e == APSIS_STRING_ATTRACTOR_4 ? 4u : 8u;
    return (clock_step % stride) == 0u ? 1u : 0u;
}

static void attr_advance(ApsisStringAttractorRuntime *r,
                         const ApsisState *s,
                         uint64_t target_sample,
                         ApsisStringAttractorRandomFn random_fn,
                         void *random_user,
                         ApsisStringAttractorDecision *out)
{
    uint8_t pos, gate, route, excite;
    uint16_t lock, value, base;
    uint32_t draw, mutation;
    if (!r || !s || !out || !r->length) return;
    pos = (uint8_t)(r->pos % r->length);
    lock = attr_clamp_q15(s->string.filter_attractor_lock_q15);
    draw = random_fn ? random_fn(random_user, ATTR_LOCK_TAG_BASE | pos) : 0u;
    if (attr_should_mutate(draw, lock)) {
        mutation = random_fn ? random_fn(random_user, ATTR_MUTATE_TAG_BASE | pos) : 0u;
        r->cell_q15[pos] = attr_draw_q15(mutation);
        r->mutation_count++;
    }
    value = r->cell_q15[pos];
    r->pos = (uint8_t)((pos + 1u) % r->length);
    r->advance_count++;

    gate = (s->string.filter_attractor_gates & (uint16_t)(1u << pos)) ? 1u : 0u;
    route = attr_route(s, pos);
    base = attr_base_level(s, value);
    excite = attr_allow_excite(s) && s->string.sound && gate &&
             s->string.filter_attractor_strum_q15 > ATTR_STRUM_EPS_Q15;
    if (excite) r->excitation_count++;

    out->target_sample = target_sample;
    out->value_q15 = value;
    out->base_level_q15 = base;
    out->pos = pos;
    out->route_mask = route;
    out->gate = gate;
    out->excite = excite;
}

uint8_t apsis_string_attractor_plan_pulse(ApsisStringAttractorRuntime *r,
                                          const ApsisState *s,
                                          uint8_t clock_step,
                                          const uint64_t target_sample[2],
                                          ApsisStringAttractorRandomFn random_fn,
                                          void *random_user,
                                          ApsisStringAttractorDecision out[2])
{
    uint8_t count, i;
    if (!r || !s || !target_sample || !out) return 0u;
    attr_zero(out, (uint32_t)(sizeof(*out) * APSIS_STRING_ATTRACTOR_DECISIONS_MAX));
    apsis_string_attractor_sync(r, s);
    count = apsis_string_attractor_advances_for_pulse(s, clock_step);
    if (!count) return 0u;
    if (clock_step == 0u && s->clock.step_one_resync) r->pos = 0u;
    for (i = 0u; i < count; ++i)
        attr_advance(r, s, target_sample[i], random_fn, random_user, &out[i]);
    return count;
}
