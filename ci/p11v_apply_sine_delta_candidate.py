#!/usr/bin/env python3
from pathlib import Path

h = Path('apsis/apsis_space_dsp.h')
s = h.read_text()
old = '    int16_t lfo_sine_b_q15;\n'
new = '    int16_t lfo_sine_delta_q15;\n'
if s.count(old) != 1:
    raise SystemExit(f'FAIL: header field anchor count {s.count(old)} != 1')
h.write_text(s.replace(old, new, 1))

c = Path('apsis/apsis_space_dsp.c')
s = c.read_text()

old_reset = '''        {
            uint8_t sine_i = (uint8_t)(line->lfo_phase >> 24);
            line->lfo_sine_a_q15 = space_sine_point(sine_i);
            line->lfo_sine_b_q15 = space_sine_point((uint8_t)(sine_i + 1u));
        }'''
new_reset = '''        {
            uint8_t sine_i = (uint8_t)(line->lfo_phase >> 24);
            int32_t sine_a = space_sine_point(sine_i);
            int32_t sine_b = space_sine_point((uint8_t)(sine_i + 1u));
            line->lfo_sine_a_q15 = (int16_t)sine_a;
            line->lfo_sine_delta_q15 = (int16_t)(sine_b - sine_a);
        }'''
if s.count(old_reset) != 1:
    raise SystemExit(f'FAIL: reset anchor count {s.count(old_reset)} != 1')
s = s.replace(old_reset, new_reset, 1)

old_render = '''            int32_t mod_q8, delayed;
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
            mod_q8 = (int32_t)(int16_t)(sine_a + (((sine_b - sine_a) * sine_frac) >> 8));'''
new_render = '''            int32_t mod_q8, delayed;
            int32_t sine_a, sine_delta;
            uint8_t sine_i, sine_frac;
            line->lfo_phase += line->lfo_phase_inc;
            sine_i = (uint8_t)(line->lfo_phase >> 24);
            if ((uint8_t)(old_phase >> 24) != sine_i) {
                int32_t next_a = space_sine_point(sine_i);
                int32_t next_b = space_sine_point((uint8_t)(sine_i + 1u));
                line->lfo_sine_a_q15 = (int16_t)next_a;
                line->lfo_sine_delta_q15 = (int16_t)(next_b - next_a);
            }
            sine_frac = (uint8_t)(line->lfo_phase >> 16);
            sine_a = line->lfo_sine_a_q15;
            sine_delta = line->lfo_sine_delta_q15;
            mod_q8 = (int32_t)(int16_t)(sine_a + ((sine_delta * sine_frac) >> 8));'''
if s.count(old_render) != 1:
    raise SystemExit(f'FAIL: render anchor count {s.count(old_render)} != 1')
s = s.replace(old_render, new_render, 1)

if 'lfo_sine_b_q15' in s:
    raise SystemExit('FAIL: stale lfo_sine_b_q15 remains in candidate source')
if s.count('lfo_sine_delta_q15') != 3:
    raise SystemExit(f'FAIL: candidate delta field source count {s.count("lfo_sine_delta_q15")} != 3')

c.write_text(s)
print('PASS: applied ephemeral P11V A+delta sine cache candidate with unchanged two-int16 state shape')
