#include "apsis_mother.h"

static void apsis_mother_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t apsis_mother_horizon_valid(uint8_t h)
{
    return h == 1u || h == 2u || h == 4u || h == 8u || h == 16u || h == 32u;
}

static int16_t apsis_mother_find(const ApsisMotherPending *p, uint8_t kind, uint16_t key)
{
    uint8_t i;
    if (!p) return -1;
    for (i = 0u; i < p->count; ++i)
        if (p->items[i].kind == kind && p->items[i].key == key) return (int16_t)i;
    return -1;
}

static void apsis_mother_arm_if_needed(ApsisMotherPending *p, int32_t last_abs_step)
{
    if (!p || !apsis_mother_has_pending(p)) return;
    if (p->pending_start_abs < 0 && last_abs_step >= 0) p->pending_start_abs = last_abs_step;
}

static uint8_t apsis_mother_queue_value(ApsisMotherPending *p, uint8_t kind, uint16_t key,
                                        int32_t value, int32_t last_abs_step)
{
    int16_t found;
    if (!p) return 0u;
    found = apsis_mother_find(p, kind, key);
    if (found >= 0) {
        p->items[(uint8_t)found].value = value;
        apsis_mother_arm_if_needed(p, last_abs_step);
        return 1u;
    }
    if (p->count >= APSIS_MOTHER_PENDING_MAX) return 0u;
    p->items[p->count].kind = kind;
    p->items[p->count].key = key;
    p->items[p->count].value = value;
    p->count++;
    apsis_mother_arm_if_needed(p, last_abs_step);
    return 1u;
}

void apsis_mother_pending_reset(ApsisMotherPending *p)
{
    if (!p) return;
    apsis_mother_zero(p, (uint32_t)sizeof(*p));
    p->pending_start_abs = -1;
}

uint16_t apsis_mother_pending_count(const ApsisMotherPending *p)
{
    if (!p) return 0u;
    return (uint16_t)p->count + (p->gravity_pending ? 1u : 0u);
}

uint8_t apsis_mother_has_pending(const ApsisMotherPending *p)
{
    return apsis_mother_pending_count(p) ? 1u : 0u;
}

uint8_t apsis_mother_should_hold(const ApsisMotherState *mother,
                                 const ApsisMotherPending *pending,
                                 uint8_t domain)
{
    if (!mother || !mother->sync) return 0u;
    if (pending && pending->applying) return 0u;
    return domain == APSIS_MOTHER_DOMAIN_STRUCTURAL ? 1u : 0u;
}

uint8_t apsis_mother_queue_param(ApsisMotherPending *p, uint16_t key,
                                 int32_t value, int32_t last_abs_step)
{
    return apsis_mother_queue_value(p, APSIS_MOTHER_PENDING_PARAM, key, value, last_abs_step);
}

uint8_t apsis_mother_queue_control(ApsisMotherPending *p, uint16_t key,
                                   int32_t value, int32_t last_abs_step)
{
    return apsis_mother_queue_value(p, APSIS_MOTHER_PENDING_CONTROL, key, value, last_abs_step);
}

uint8_t apsis_mother_queue_toggle(ApsisMotherPending *p, uint16_t key,
                                  int32_t last_abs_step)
{
    int16_t found;
    uint8_t i;
    if (!p) return 0u;
    found = apsis_mother_find(p, APSIS_MOTHER_PENDING_TOGGLE, key);
    if (found >= 0) {
        for (i = (uint8_t)found; i + 1u < p->count; ++i) p->items[i] = p->items[i + 1u];
        if (p->count) p->count--;
        if (apsis_mother_has_pending(p)) apsis_mother_arm_if_needed(p, last_abs_step);
        else apsis_mother_clear_pending(p);
        return 1u;
    }
    return apsis_mother_queue_value(p, APSIS_MOTHER_PENDING_TOGGLE, key, 1, last_abs_step);
}

void apsis_mother_queue_gravity(ApsisMotherPending *p, uint8_t degree,
                                uint8_t ring, int32_t last_abs_step)
{
    if (!p) return;
    p->gravity_pending = 1u;
    p->gravity_degree = degree > 7u ? 7u : degree;
    p->gravity_ring = ring > 2u ? 2u : ring;
    apsis_mother_arm_if_needed(p, last_abs_step);
}

const ApsisMotherPendingItem *apsis_mother_pending_at(const ApsisMotherPending *p,
                                                      uint8_t index)
{
    if (!p || index >= p->count) return 0;
    return &p->items[index];
}

void apsis_mother_clear_pending(ApsisMotherPending *p)
{
    if (!p) return;
    p->count = 0u;
    p->gravity_pending = 0u;
    p->gravity_degree = 0u;
    p->gravity_ring = 0u;
    p->pending_start_abs = -1;
    p->take_next = 0u;
}

void apsis_mother_set_sync(ApsisMotherState *mother, ApsisMotherPending *p,
                           uint8_t on)
{
    uint8_t next;
    if (!mother || !p) return;
    next = on ? 1u : 0u;
    if (!next && apsis_mother_has_pending(p)) apsis_mother_clear_pending(p);
    mother->sync = next;
    p->pending_start_abs = -1;
}

void apsis_mother_set_horizon(ApsisMotherState *mother, ApsisMotherPending *p,
                              uint8_t horizon, int32_t last_abs_step)
{
    if (!mother || !p) return;
    mother->horizon = apsis_mother_horizon_valid(horizon) ? horizon : 8u;
    if (apsis_mother_has_pending(p))
        p->pending_start_abs = last_abs_step >= 0 ? last_abs_step : -1;
}

uint8_t apsis_mother_should_commit(const ApsisMotherState *mother,
                                   ApsisMotherPending *p,
                                   int32_t abs_step, uint8_t clock_step)
{
    int32_t elapsed;
    uint8_t h;
    if (!mother || !p || !mother->sync || !apsis_mother_has_pending(p)) return 0u;
    if (abs_step < 0) return 0u;
    if (p->pending_start_abs < 0) p->pending_start_abs = abs_step > 0 ? abs_step - 1 : -1;
    elapsed = p->pending_start_abs < 0 ? abs_step + 1 : abs_step - p->pending_start_abs;
    if (elapsed < 0) elapsed = 0;
    h = apsis_mother_horizon_valid(mother->horizon) ? mother->horizon : 8u;
    return elapsed >= h && clock_step == 0u ? 1u : 0u;
}

void apsis_mother_begin_commit(ApsisMotherPending *p)
{
    if (p) p->applying = 1u;
}

void apsis_mother_finish_commit(ApsisMotherState *mother, ApsisMotherPending *p)
{
    if (!mother || !p) return;
    p->applying = 0u;
    apsis_mother_clear_pending(p);
    mother->sync = 0u;
}
