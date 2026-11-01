#!/usr/bin/env python3
from pathlib import Path

h = Path('apsis/apsis_space_dsp.h')
s = h.read_text()
old = '''    uint32_t lfo_phase;
    uint32_t lfo_phase_inc;
    int32_t damping_state_q15;
} ApsisSpaceFdnLine;'''
new = '''    uint32_t lfo_phase;
    uint32_t lfo_phase_inc;
    int32_t damping_state_q15;
    int16_t lfo_sine_a_q15;
    int16_t lfo_sine_b_q15;
} ApsisSpaceFdnLine;'''
if s.count(old) != 1:
    raise SystemExit(f'header candidate anchor count {s.count(old)} != 1')
h.write_text(s.replace(old, new, 1))

c = Path('apsis/apsis_space_dsp.c')
s = c.read_text()
old_sine = '''static int16_t space_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = space_sine_point(i);
    int32_t b = space_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

'''
if s.count(old_sine) != 1:
    raise SystemExit(f'sine helper removal anchor count {s.count(old_sine)} != 1')
s = s.replace(old_sine, '', 1)
old_reset = '''        line->lfo_phase = (uint32_t)i * 0x20000000u;
        line->lfo_phase_inc = SPACE_LFO_INC[i];
        line->damping_state_q15 = 0;
        offset += line->capacity;'''
new_reset = '''        line->lfo_phase = (uint32_t)i * 0x20000000u;
        line->lfo_phase_inc = SPACE_LFO_INC[i];
        line->damping_state_q15 = 0;
        {
            uint8_t sine_i = (uint8_t)(line->lfo_phase >> 24);
            line->lfo_sine_a_q15 = space_sine_point(sine_i);
            line->lfo_sine_b_q15 = space_sine_point((uint8_t)(sine_i + 1u));
        }
        offset += line->capacity;'''
if s.count(old_reset) != 1:
    raise SystemExit(f'reset candidate anchor count {s.count(old_reset)} != 1')
s = s.replace(old_reset, new_reset, 1)
old_render = '''        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int32_t mod_q8, delayed;
            line->lfo_phase += line->lfo_phase_inc;
            mod_q8 = ((int32_t)space_sine_q15(line->lfo_phase) * SPACE_MOD_DEPTH_Q8) >> 15;
            delayed = space_line_read(fdn, line, mod_q8);'''
new_render = '''        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            uint32_t old_phase = line->lfo_phase;
            int32_t mod_q8, delayed;
            int32_t sine_a, sine_b;
            uint8_t sine_i, sine_frac;
            line->lfo_phase += line->lfo_phase_inc;
            sine_i = (uint8_t)(line->lfo_phase >> 24);
            if ((uint8_t)(old_phase >> 24) != sine_i) {
                line->lfo_sine_a_q15 = space_sine_point(sine_i);
                line->lfo_sine_b_q15 = space_sine_point((uint8_t)(sine_i + 1u));
            }
            sine_frac = (uint8_t)(line->lfo_phase >> 16);
            sine_a = line->lfo_sine_a_q15;
            sine_b = line->lfo_sine_b_q15;
            mod_q8 = (int32_t)(int16_t)(sine_a + (((sine_b - sine_a) * sine_frac) >> 8));
            mod_q8 = (mod_q8 * SPACE_MOD_DEPTH_Q8) >> 15;
            delayed = space_line_read(fdn, line, mod_q8);'''
if s.count(old_render) != 1:
    raise SystemExit(f'render candidate anchor count {s.count(old_render)} != 1')
c.write_text(s.replace(old_render, new_render, 1))
print('PASS: applied ephemeral P11Q sine-endpoint cache candidate')
