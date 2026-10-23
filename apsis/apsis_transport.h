#ifndef APSIS_TRANSPORT_H
#define APSIS_TRANSPORT_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_TRANSPORT_DEFAULT_RATE 44100u
#define APSIS_TRANSPORT_QUEUE_CAP 256u
#define APSIS_TRANSPORT_PULSE_EVENTS_MAX 16u

typedef enum {
    /* Numeric order is the browser tick() scheduling order for events that
     * share one target sample. It is not a DSP voice priority. */
    APSIS_EVENT_STEP_ONE_RESYNC = 0,
    APSIS_EVENT_STRING_FILTER_CLOCK,
    APSIS_EVENT_PLAYHEAD,
    APSIS_EVENT_MOTHER_PULSE,
    APSIS_EVENT_CLOCK_MAIN,
    APSIS_EVENT_SPIRAL_TICK,
    APSIS_EVENT_STRING_RHYTHM,
    APSIS_EVENT_BLOOM,
    APSIS_EVENT_CLOCK_SNAP,
    APSIS_EVENT_CLOCK_INNER,
    APSIS_EVENT_CLOCK_GHOST,
    APSIS_EVENT_CHAIN_PULSE
} ApsisEventType;

typedef struct {
    uint64_t sample;
    uint64_t interval_q32;
    uint32_t absolute_pulse;
    uint16_t serial;
    uint8_t type;
    uint8_t clock_step;
    uint8_t sub_index;
    uint8_t sub_count;
    uint8_t flags;
} ApsisTransportEvent;

#define APSIS_EVENT_FLAG_ACCENT       (1u << 0)
#define APSIS_EVENT_FLAG_STEP_ONE     (1u << 1)
#define APSIS_EVENT_FLAG_GRAVITY_GRID (1u << 2)

typedef struct {
    ApsisTransportEvent events[APSIS_TRANSPORT_QUEUE_CAP];
    uint16_t count;
    uint16_t next_serial;
    uint8_t overflowed;
} ApsisEventQueue;

typedef struct {
    uint32_t sample_rate;
    uint64_t next_sample_q32;
    uint32_t absolute_pulse;
    uint32_t melody_step;
    uint8_t cycle_pos;
    uint8_t running;
} ApsisTransport;

void apsis_event_queue_reset(ApsisEventQueue *queue);
uint16_t apsis_event_queue_count(const ApsisEventQueue *queue);
const ApsisTransportEvent *apsis_event_queue_peek(const ApsisEventQueue *queue);
uint8_t apsis_event_queue_pop(ApsisEventQueue *queue, ApsisTransportEvent *out);

void apsis_transport_reset(ApsisTransport *transport, uint32_t sample_rate,
                           uint64_t first_sample);
void apsis_transport_start(ApsisTransport *transport);
void apsis_transport_stop(ApsisTransport *transport);
uint64_t apsis_transport_interval_q32(const ApsisClockState *clock,
                                      uint32_t sample_rate,
                                      uint32_t absolute_pulse);
uint64_t apsis_transport_round_sample(uint64_t sample_q32);
uint8_t apsis_transport_gravity_division(uint8_t requested_parts, uint8_t clock_n);
uint8_t apsis_transport_is_gravity_boundary(uint8_t clock_step, uint8_t clock_n,
                                            uint8_t requested_parts);
uint64_t apsis_transport_next_gravity_boundary(const ApsisTransport *transport,
                                               const ApsisClockState *clock,
                                               uint8_t requested_parts,
                                               uint64_t now_sample,
                                               uint32_t min_lead_samples);

/* Schedule exactly one browser-equivalent Clock pulse and advance transport.
 * ghost_random_u32 is an injected deterministic [0,2^32) draw for the only
 * stochastic Clock event in tick(); it is ignored when no ghost draw is due.
 * Returns number of events added, or 0 if stopped/invalid/queue lacks room. */
uint8_t apsis_transport_schedule_next(ApsisTransport *transport,
                                      const ApsisState *state,
                                      ApsisEventQueue *queue,
                                      uint32_t ghost_random_u32);

#endif
