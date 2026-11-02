#!/usr/bin/env python3
from pathlib import Path

p = Path('apsis/apsis_space_dsp.c')
s = p.read_text()

wide = '''static int32_t space_mul_q15(int32_t a, int32_t b)
{
    int64_t p = (int64_t)a * b;
    if (p >= 0) p += 16384;
    else p -= 16384;
    return (int32_t)(p >> 15);
}
'''
narrow = wide + '''
/* P11T candidate: use only at call-site families with P11S structural bounds
 * proving product plus the exact rounding term fits signed 32-bit. */
static int32_t space_mul_q15_i32(int32_t a, int32_t b)
{
    int32_t p = a * b;
    if (p >= 0) p += 16384;
    else p -= 16384;
    return p >> 15;
}
'''
if s.count(wide) != 1:
    raise SystemExit(f'FAIL: wide helper anchor count {s.count(wide)} != 1')
s = s.replace(wide, narrow, 1)

repls = [
    ('''line->damping_state_q15 += space_mul_q15(
                delayed - line->damping_state_q15, alpha);''',
     '''line->damping_state_q15 += space_mul_q15_i32(
                delayed - line->damping_state_q15, alpha);'''),
    ('''fdn->post_left_q15 += space_mul_q15(
            space_mul_q15(out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha);''',
     '''fdn->post_left_q15 += space_mul_q15_i32(
            space_mul_q15_i32(out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha);'''),
    ('''fdn->post_right_q15 += space_mul_q15(
            space_mul_q15(out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha);''',
     '''fdn->post_right_q15 += space_mul_q15_i32(
            space_mul_q15_i32(out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha);'''),
    ('space_mul_q15(fdn->post_left_q15, SPACE_CROSS_SELF_Q15)',
     'space_mul_q15_i32(fdn->post_left_q15, SPACE_CROSS_SELF_Q15)'),
    ('space_mul_q15(fdn->post_right_q15, SPACE_CROSS_OTHER_Q15)',
     'space_mul_q15_i32(fdn->post_right_q15, SPACE_CROSS_OTHER_Q15)'),
    ('space_mul_q15(fdn->post_right_q15, SPACE_CROSS_SELF_Q15)',
     'space_mul_q15_i32(fdn->post_right_q15, SPACE_CROSS_SELF_Q15)'),
    ('space_mul_q15(fdn->post_left_q15, SPACE_CROSS_OTHER_Q15)',
     'space_mul_q15_i32(fdn->post_left_q15, SPACE_CROSS_OTHER_Q15)'),
    ('''int32_t excitation = space_mul_q15(input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15);''',
     '''int32_t excitation = space_mul_q15_i32(input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15);'''),
]
for old, new in repls:
    n = s.count(old)
    if n != 1:
        raise SystemExit(f'FAIL: candidate anchor count {n} != 1: {old[:90]!r}')
    s = s.replace(old, new, 1)

# Wide authority must remain at exactly the two P11S-vetoed dynamic families,
# plus its own definition. Narrow helper has one definition + ten lexical sites.
if s.count('space_mul_q15(') != 3:
    raise SystemExit(f'FAIL: remaining wide lexical count {s.count("space_mul_q15(")} != 3')
if s.count('space_mul_q15_i32(') != 11:
    raise SystemExit(f'FAIL: narrow lexical count {s.count("space_mul_q15_i32(")} != 11')
if 'int32_t feedback = space_mul_q15(mixed[k], SPACE_HAD_NORM_Q15);' not in s:
    raise SystemExit('FAIL: Hadamard normalization no longer uses wide helper')
if 'feedback = space_mul_q15(feedback, fdn->feedback_q15[k]);' not in s:
    raise SystemExit('FAIL: feedback gain no longer uses wide helper')

p.write_text(s)
print('PASS: applied ephemeral P11T selective Q15 narrow candidate; 24 dynamic calls narrow, 16 remain wide')
