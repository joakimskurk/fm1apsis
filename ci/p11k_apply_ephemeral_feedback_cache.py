#!/usr/bin/env python3
from pathlib import Path

HEADER = Path('work/apsis/firmware/src/apsis_space_dsp.h')
SOURCE = Path('work/apsis/firmware/src/apsis_space_dsp.c')

h = HEADER.read_text()
old = '''typedef struct {
    int16_t *storage;
    uint32_t storage_samples;
    ApsisSpaceFdnLine line[APSIS_SPACE_FDN_LINES];
    int32_t post_left_q15;
    int32_t post_right_q15;
    uint8_t storage_bound;
} ApsisSpaceFdn;'''
new = '''typedef struct {
    int16_t *storage;
    uint32_t storage_samples;
    ApsisSpaceFdnLine line[APSIS_SPACE_FDN_LINES];
    int32_t post_left_q15;
    int32_t post_right_q15;
    uint16_t feedback_q15[APSIS_SPACE_FDN_LINES];
    uint16_t feedback_decay_ms;
    uint8_t feedback_cache_valid;
    uint8_t storage_bound;
} ApsisSpaceFdn;'''
if old not in h:
    raise SystemExit('FAIL: P11K target header anchor missing')
h = h.replace(old, new, 1)
HEADER.write_text(h)

s = SOURCE.read_text()
reset_anchor = '''    fdn->post_left_q15 = 0;
    fdn->post_right_q15 = 0;
    for (i = 0u; i < APSIS_SPACE_FDN_LINES; ++i) {'''
reset_new = '''    fdn->post_left_q15 = 0;
    fdn->post_right_q15 = 0;
    fdn->feedback_decay_ms = 0u;
    fdn->feedback_cache_valid = 0u;
    for (i = 0u; i < APSIS_SPACE_FDN_LINES; ++i) {'''
if reset_anchor not in s:
    raise SystemExit('FAIL: P11K target reset anchor missing')
s = s.replace(reset_anchor, reset_new, 1)

old_decl = '''    uint16_t feedback_q15[APSIS_SPACE_FDN_LINES];
    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    if (!fdn || !fdn->storage_bound || !out_stereo) return;
    alpha = space_damping_alpha(damping_hz);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        feedback_q15[k] = space_feedback_gain(fdn->line[k].base_delay, decay_ms);
'''
new_decl = '''    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    if (!fdn || !fdn->storage_bound || !out_stereo) return;
    alpha = space_damping_alpha(damping_hz);
    if (!fdn->feedback_cache_valid || fdn->feedback_decay_ms != decay_ms) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
            fdn->feedback_q15[k] = space_feedback_gain(fdn->line[k].base_delay, decay_ms);
        fdn->feedback_decay_ms = decay_ms;
        fdn->feedback_cache_valid = 1u;
    }
'''
if old_decl not in s:
    raise SystemExit('FAIL: P11K target render setup anchor missing')
s = s.replace(old_decl, new_decl, 1)

old_use = '            feedback = space_mul_q15(feedback, feedback_q15[k]);'
new_use = '            feedback = space_mul_q15(feedback, fdn->feedback_q15[k]);'
if old_use not in s:
    raise SystemExit('FAIL: P11K target feedback use anchor missing')
s = s.replace(old_use, new_use, 1)
SOURCE.write_text(s)

print('PASS: applied ephemeral P11K FDN feedback-gain cache candidate to generated target only')
