#ifndef APSIS_MOTHER_H
#define APSIS_MOTHER_H

#include <stdint.h>
#include "apsis_state.h"

/* The browser uses a JS object keyed by param/toggle/control path. 128 fixed
 * entries exceed the number of current structural controls while keeping the
 * FM-1 implementation allocation-free and deterministic. */
#define APSIS_MOTHER_PENDING_MAX 128u

typedef enum {
    APSIS_MOTHER_DOMAIN_STRUCTURAL = 0,
    APSIS_MOTHER_DOMAIN_MOTHER,
    APSIS_MOTHER_DOMAIN_OUTPUT
} ApsisMotherDomain;

typedef enum {
    APSIS_MOTHER_PENDING_PARAM = 0,
    APSIS_MOTHER_PENDING_TOGGLE,
    APSIS_MOTHER_PENDING_CONTROL
} ApsisMotherPendingKind;

typedef struct {
    uint8_t kind;
    uint16_t key;
    int32_t value;
} ApsisMotherPendingItem;

typedef struct {
    ApsisMotherPendingItem items[APSIS_MOTHER_PENDING_MAX];
    uint8_t count;
    uint8_t gravity_pending;
    uint8_t gravity_degree;
    uint8_t gravity_ring;
    int32_t pending_start_abs;
    uint8_t take_next;
    uint8_t applying;
} ApsisMotherPending;

void apsis_mother_pending_reset(ApsisMotherPending *pending);
uint16_t apsis_mother_pending_count(const ApsisMotherPending *pending);
uint8_t apsis_mother_has_pending(const ApsisMotherPending *pending);
uint8_t apsis_mother_should_hold(const ApsisMotherState *mother,
                                 const ApsisMotherPending *pending,
                                 uint8_t domain);
uint8_t apsis_mother_queue_param(ApsisMotherPending *pending, uint16_t key,
                                 int32_t value, int32_t last_abs_step);
uint8_t apsis_mother_queue_toggle(ApsisMotherPending *pending, uint16_t key,
                                  int32_t last_abs_step);
uint8_t apsis_mother_queue_control(ApsisMotherPending *pending, uint16_t key,
                                   int32_t value, int32_t last_abs_step);
void apsis_mother_queue_gravity(ApsisMotherPending *pending, uint8_t degree,
                                uint8_t ring, int32_t last_abs_step);
const ApsisMotherPendingItem *apsis_mother_pending_at(const ApsisMotherPending *pending,
                                                      uint8_t index);
void apsis_mother_clear_pending(ApsisMotherPending *pending);
void apsis_mother_set_sync(ApsisMotherState *mother, ApsisMotherPending *pending,
                           uint8_t on);
void apsis_mother_set_horizon(ApsisMotherState *mother, ApsisMotherPending *pending,
                              uint8_t horizon, int32_t last_abs_step);
uint8_t apsis_mother_should_commit(const ApsisMotherState *mother,
                                   ApsisMotherPending *pending,
                                   int32_t abs_step, uint8_t clock_step);
void apsis_mother_begin_commit(ApsisMotherPending *pending);
void apsis_mother_finish_commit(ApsisMotherState *mother, ApsisMotherPending *pending);

#endif
