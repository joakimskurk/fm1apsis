#ifndef APSIS_CLOCK_H
#define APSIS_CLOCK_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_CLOCK_GROUPS_MAX 4u

typedef struct {
    uint8_t step;
    uint8_t n;
    uint8_t main_on;
    uint8_t accent;
    uint8_t inner_on;
    uint8_t snap_on;
    uint8_t step_one_resync;
    uint32_t interval_us;
} ApsisClockPulse;

uint8_t apsis_clock_groups(const ApsisClockState *clock, uint8_t out[APSIS_CLOCK_GROUPS_MAX]);
uint32_t apsis_clock_base_interval_us(const ApsisClockState *clock);
uint32_t apsis_clock_interval_us(const ApsisClockState *clock, uint32_t absolute_step);
ApsisClockPulse apsis_clock_pulse(const ApsisClockState *clock, uint8_t cycle_step, uint32_t absolute_step);
void apsis_clock_set_length(ApsisClockState *clock, uint8_t n);

#endif
