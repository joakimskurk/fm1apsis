#ifndef APSIS_P11H_PROBE_H
#define APSIS_P11H_PROBE_H

#include <stdint.h>

typedef struct {
    uint64_t target_render_calls;
    uint64_t output_frames;
    uint64_t room_frames;
    uint64_t room_render_calls;
    uint64_t room_frame_iterations;
    uint64_t smooth_param_steps;
    uint64_t delay_read_calls;
    uint64_t stereo_write_calls;
    uint64_t room_biquad_calls;
    uint64_t room_mul_q15_calls;
    uint64_t fdn_render_calls;
    uint64_t fdn_frames;
    uint64_t fdn_line_read_calls;
    uint64_t fdn_line_write_calls;
    uint64_t fdn_mul_q15_calls;
    uint64_t transport_schedule_calls;
    uint64_t transport_events_added;
} ApsisP11hProbe;

extern ApsisP11hProbe apsis_p11h_probe;

#endif
