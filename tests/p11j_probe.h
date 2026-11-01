#ifndef APSIS_P11J_PROBE_H
#define APSIS_P11J_PROBE_H

#include <stdint.h>

void p11j_probe_feedback_gain(uint16_t delay_samples, uint16_t decay_ms);
void p11j_probe_fdn_render(uint16_t decay_ms, uint16_t damping_hz);

#define P11J_PROBE_FEEDBACK_GAIN(delay_samples, decay_ms) \
    p11j_probe_feedback_gain((delay_samples), (decay_ms))
#define P11J_PROBE_FDN_RENDER(decay_ms, damping_hz) \
    p11j_probe_fdn_render((decay_ms), (damping_hz))

#endif
