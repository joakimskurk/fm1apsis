#!/usr/bin/env python3
from pathlib import Path

src = Path('apsis/apsis_space_dsp.c').read_text()

hook = '''\n/* P11S test-only observer. This file is generated into /tmp; production source\n * remains byte-identical to the frozen P11R authority. */\nenum {\n    P11S_MUL_ALPHA = 0,\n    P11S_MUL_OUTPUT = 1,\n    P11S_MUL_CROSS = 2,\n    P11S_MUL_HAD_NORM = 3,\n    P11S_MUL_EXCITATION = 4,\n    P11S_MUL_FEEDBACK = 5\n};\nextern void p11s_observe_mul(unsigned site, int32_t a, int32_t b, int64_t product);\n'''
anchor = '#include "apsis_space_dsp.h"\n'
if src.count(anchor) != 1:
    raise SystemExit('FAIL: include anchor')
src = src.replace(anchor, anchor + hook, 1)

mul_def = '''static int32_t space_mul_q15(int32_t a, int32_t b)\n{\n    int64_t p = (int64_t)a * b;\n    if (p >= 0) p += 16384;\n    else p -= 16384;\n    return (int32_t)(p >> 15);\n}\n'''
site_def = mul_def + '''\nstatic int32_t p11s_mul_site(unsigned site, int32_t a, int32_t b)\n{\n    p11s_observe_mul(site, a, b, (int64_t)a * b);\n    return space_mul_q15(a, b);\n}\n'''
if src.count(mul_def) != 1:
    raise SystemExit('FAIL: space_mul_q15 definition anchor')
src = src.replace(mul_def, site_def, 1)

repls = [
    ('''line->damping_state_q15 += space_mul_q15(\n                delayed - line->damping_state_q15, alpha);''',
     '''line->damping_state_q15 += p11s_mul_site(P11S_MUL_ALPHA,\n                delayed - line->damping_state_q15, alpha);''', 1),
    ('space_mul_q15(out_l, SPACE_OUTPUT_Q15)',
     'p11s_mul_site(P11S_MUL_OUTPUT, out_l, SPACE_OUTPUT_Q15)', 1),
    ('space_mul_q15(out_r, SPACE_OUTPUT_Q15)',
     'p11s_mul_site(P11S_MUL_OUTPUT, out_r, SPACE_OUTPUT_Q15)', 1),
    ('''space_mul_q15(\n            p11s_mul_site(P11S_MUL_OUTPUT, out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha)''',
     '''p11s_mul_site(P11S_MUL_ALPHA,\n            p11s_mul_site(P11S_MUL_OUTPUT, out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha)''', 1),
    ('''space_mul_q15(\n            p11s_mul_site(P11S_MUL_OUTPUT, out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha)''',
     '''p11s_mul_site(P11S_MUL_ALPHA,\n            p11s_mul_site(P11S_MUL_OUTPUT, out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha)''', 1),
    ('space_mul_q15(fdn->post_left_q15, SPACE_CROSS_SELF_Q15)',
     'p11s_mul_site(P11S_MUL_CROSS, fdn->post_left_q15, SPACE_CROSS_SELF_Q15)', 1),
    ('space_mul_q15(fdn->post_right_q15, SPACE_CROSS_OTHER_Q15)',
     'p11s_mul_site(P11S_MUL_CROSS, fdn->post_right_q15, SPACE_CROSS_OTHER_Q15)', 1),
    ('space_mul_q15(fdn->post_right_q15, SPACE_CROSS_SELF_Q15)',
     'p11s_mul_site(P11S_MUL_CROSS, fdn->post_right_q15, SPACE_CROSS_SELF_Q15)', 1),
    ('space_mul_q15(fdn->post_left_q15, SPACE_CROSS_OTHER_Q15)',
     'p11s_mul_site(P11S_MUL_CROSS, fdn->post_left_q15, SPACE_CROSS_OTHER_Q15)', 1),
    ('space_mul_q15(mixed[k], SPACE_HAD_NORM_Q15)',
     'p11s_mul_site(P11S_MUL_HAD_NORM, mixed[k], SPACE_HAD_NORM_Q15)', 1),
    ('space_mul_q15(input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15)',
     'p11s_mul_site(P11S_MUL_EXCITATION, input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15)', 1),
    ('space_mul_q15(feedback, fdn->feedback_q15[k])',
     'p11s_mul_site(P11S_MUL_FEEDBACK, feedback, fdn->feedback_q15[k])', 1),
]
for old, new, expected in repls:
    n = src.count(old)
    if n != expected:
        raise SystemExit(f'FAIL: instrumentation anchor count {n} != {expected}: {old[:70]!r}')
    src = src.replace(old, new, expected)

# Only the original helper definition and the observer wrapper may still name it.
if src.count('space_mul_q15(') != 2:
    raise SystemExit(f'FAIL: unexpected remaining space_mul_q15 lexical count {src.count("space_mul_q15(")}')

out = Path('/tmp/p11s_apsis_space_dsp.c')
out.write_text(src)
print('PASS: generated test-only P11S instrumented Space source with 6 site families')
