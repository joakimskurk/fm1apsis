#ifndef APSIS_P11M_PROBE_H
#define APSIS_P11M_PROBE_H

#include <stdint.h>

void p11m_probe_sine(uint8_t phase_frac);
void p11m_probe_line_call(int32_t modulation_q8);
void p11m_probe_delay(int32_t delay_q8);
void p11m_probe_readiness(uint16_t fill_count, uint16_t capacity, uint32_t needed);
void p11m_probe_wrap(int32_t raw_read_q8, int32_t span_q8);
void p11m_probe_interp(uint32_t frac, uint32_t i1, uint16_t capacity);

#define P11M_PROBE_SINE(frac) \
    p11m_probe_sine((frac))
#define P11M_PROBE_LINE_CALL(modulation_q8) \
    p11m_probe_line_call((modulation_q8))
#define P11M_PROBE_DELAY(delay_q8) \
    p11m_probe_delay((delay_q8))
#define P11M_PROBE_READINESS(fill_count, capacity, needed) \
    p11m_probe_readiness((fill_count), (capacity), (needed))
#define P11M_PROBE_WRAP(raw_read_q8, span_q8) \
    p11m_probe_wrap((raw_read_q8), (span_q8))
#define P11M_PROBE_INTERP(frac, i1, capacity) \
    p11m_probe_interp((frac), (i1), (capacity))

#endif
