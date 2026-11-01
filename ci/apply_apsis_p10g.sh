#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 10G layers the host-proven Gravity selected-point editor above green
# P10F. It edits the existing 24-point grid and filters only SEL+degree audition
# from the frozen P9 Gravity dispatcher; no scheduler/DSP/pending shadow exists.
bash ci/apply_apsis_p10f.sh

cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Iapsis \
  apsis/apsis_controls_gravity_point.c \
  tests/test_controls_gravity_point.c \
  -o /tmp/test_controls_gravity_point
/tmp/test_controls_gravity_point

cp apsis/apsis_controls_gravity_point.h \
   apsis/apsis_controls_gravity_point.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_controls_spiral_attractor.h"'
if s.count(h_anchor) != 1:
    raise SystemExit(f'FAIL: P10G P10F header anchor count {s.count(h_anchor)} != 1')
s = s.replace(h_anchor,
              h_anchor + '\n#include "apsis_controls_gravity_point.h"', 1)

c_anchor = '#include "apsis_controls_spiral_attractor.c"'
if s.count(c_anchor) != 1:
    raise SystemExit(f'FAIL: P10G P10F source anchor count {s.count(c_anchor)} != 1')
s = s.replace(c_anchor,
              c_anchor + '\n#include "apsis_controls_gravity_point.c"', 1)

state_anchor = 'static uint8_t apsis_spiral_attractor_visual_dirty;'
if s.count(state_anchor) != 1:
    raise SystemExit(f'FAIL: P10G P10F state anchor count {s.count(state_anchor)} != 1')
s = s.replace(state_anchor, state_anchor +
              '\nstatic ApsisGravityPointEditorSession apsis_gravity_point_editor;'
              '\nstatic uint8_t apsis_gravity_point_visual_dirty;', 1)

decl = '''    ApsisSpiralAttractorControlResult spiral_attractor_control;
    uint8_t attractor_editor_frame = 0u;
    uint8_t spiral_attractor_editor_frame = 0u;
    uint8_t motion_control_valid = 0u;'''
if s.count(decl) != 1:
    raise SystemExit(f'FAIL: P10G dispatcher declaration anchor count {s.count(decl)} != 1')
s = s.replace(decl, '''    ApsisSpiralAttractorControlResult spiral_attractor_control;
    ApsisGravityPointContextResult gravity_point_context;
    ApsisGravityPointControlResult gravity_point_control = {0};
    ApsisFm1InputFrame gravity_dispatch_input;
    const ApsisFm1InputFrame *dispatch_input = &input;
    uint8_t attractor_editor_frame = 0u;
    uint8_t spiral_attractor_editor_frame = 0u;
    uint8_t motion_control_valid = 0u;''', 1)

spiral_block = '''    (void)apsis_controls_spiral_attractor_context(
        &apsis_spiral_attractor_editor, current, &input, 0);
    if (current == SURF_SPIRAL && apsis_spiral_attractor_editor.active) {
        spiral_attractor_editor_frame = apsis_controls_spiral_attractor_edit(
            &apsis_spiral_attractor_editor, &apsis.spiral, &input,
            &spiral_attractor_control);
        if (spiral_attractor_control.state_changed)
            apsis_spiral_attractor_visual_dirty = 1u;
    }
'''
if s.count(spiral_block) != 1:
    raise SystemExit(f'FAIL: P10G P10F Spiral block count {s.count(spiral_block)} != 1')
s = s.replace(spiral_block, spiral_block + '''    (void)apsis_controls_gravity_point_context(
        &apsis_gravity_point_editor, current, &input, &gravity_point_context);
    if (gravity_point_context.changed)
        apsis_gravity_point_visual_dirty = 1u;
    if (current == SURF_GRAVITY &&
        apsis_gravity_point_editor.page == APSIS_GRAVITY_PAGE_POINT) {
        (void)apsis_controls_gravity_point_edit(
            &apsis_gravity_point_editor, &apsis.gravity, &input,
            &gravity_point_control);
        if (gravity_point_control.state_changed)
            apsis_gravity_point_visual_dirty = 1u;
        if (gravity_point_control.suppress_degree_request) {
            gravity_dispatch_input = input;
            gravity_dispatch_input.notes_pressed &= ~0xffu;
            dispatch_input = &gravity_dispatch_input;
        }
    }
''', 1)

dispatch = '''    if (!attractor_editor_frame && !spiral_attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current, &input,
                                last_abs_step, &result);'''
if s.count(dispatch) != 1:
    raise SystemExit(f'FAIL: P10G P10F dispatch gate count {s.count(dispatch)} != 1')
s = s.replace(dispatch, '''    if (!attractor_editor_frame && !spiral_attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current,
                                dispatch_input, last_abs_step, &result);''', 1)

nav_anchor = '''        if (next != SURF_SPIRAL) {
            apsis_controls_spiral_attractor_editor_reset(
                &apsis_spiral_attractor_editor);
            apsis_spiral_attractor_visual_dirty = 0u;
        } else if (apsis_spiral_attractor_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_spiral_attractor_visual_dirty = 0u;
        }'''
if s.count(nav_anchor) != 1:
    raise SystemExit(f'FAIL: P10G P10F navigation anchor count {s.count(nav_anchor)} != 1')
s = s.replace(nav_anchor, nav_anchor + '''
        if (next != SURF_GRAVITY) {
            apsis_controls_gravity_point_editor_reset(
                &apsis_gravity_point_editor);
            apsis_gravity_point_visual_dirty = 0u;
        } else if (apsis_gravity_point_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_gravity_point_visual_dirty = 0u;
        }''', 1)

boot_anchor = '''    apsis_controls_spiral_attractor_editor_reset(
        &apsis_spiral_attractor_editor);
    apsis_spiral_attractor_visual_dirty = 0u;'''
if s.count(boot_anchor) != 1:
    raise SystemExit(f'FAIL: P10G P10F boot anchor count {s.count(boot_anchor)} != 1')
s = s.replace(boot_anchor, boot_anchor + '''
    apsis_controls_gravity_point_editor_reset(
        &apsis_gravity_point_editor);
    apsis_gravity_point_visual_dirty = 0u;''', 1)

p.write_text(s)
PY

grep -q '#include "apsis_controls_gravity_point.c"' work/apsis/firmware/src/dinghy.c
grep -q 'static ApsisGravityPointEditorSession apsis_gravity_point_editor;' work/apsis/firmware/src/dinghy.c
grep -q 'const ApsisFm1InputFrame \*dispatch_input = &input;' work/apsis/firmware/src/dinghy.c
grep -q 'gravity_dispatch_input.notes_pressed &= ~0xffu;' work/apsis/firmware/src/dinghy.c
grep -q 'dispatch_input, last_abs_step, &result' work/apsis/firmware/src/dinghy.c
grep -q 'degree \* APSIS_GRAVITY_RINGS + ring' work/apsis/firmware/src/apsis_visual.c
grep -q 'gravity->selected_point_degree' work/apsis/firmware/src/apsis_visual.c

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
control = s[:a] + s[b:]
for forbidden in ('apsis_controls_gravity_point_context(',
                  'apsis_controls_gravity_point_edit(',
                  'apsis_gravity_point_visual_dirty'):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P10G editor/UI leaked into app_audio: {forbidden}')
for required in ('apsis_controls_gravity_point_context(',
                 'apsis_controls_gravity_point_edit(',
                 'gravity_dispatch_input.notes_pressed &= ~0xffu;',
                 'dispatch_input, last_abs_step, &result'):
    if required not in control:
        raise SystemExit(f'FAIL: P10G target binding missing outside audio: {required}')
if s.count('apsis_controls_gravity_point_editor_reset(') != 2:
    raise SystemExit('FAIL: P10G expected boot + leave-surface Gravity page resets only')
if s.count('apsis_gravity_point_visual_dirty = 1u;') != 2:
    raise SystemExit('FAIL: P10G expected context + point visual-dirty producers')
renderer = Path('work/apsis/firmware/src/apsis_visual.c').read_text()
for required in ('degree * APSIS_GRAVITY_RINGS + ring',
                 'gravity->selected_point_degree',
                 'point->enabled',
                 'context->runtime->gravity_pending.active'):
    if required not in renderer:
        raise SystemExit(f'FAIL: P10G existing Gravity visual lost authority: {required}')
dispatch_source = Path('work/apsis/firmware/src/apsis_controls_dispatch.c').read_text()
for required in ('apsis_controls_gravity_ring_from_fm1(',
                 'apsis_controls_gravity_degree_from_fm1(',
                 'result->gravity_request = 1u;'):
    if required not in dispatch_source:
        raise SystemExit(f'FAIL: P10G disturbed frozen P9 Gravity dispatcher: {required}')
if 'static ApsisRuntime apsis_runtime;' not in s:
    raise SystemExit('FAIL: P10G disturbed shared runtime declaration')
print('PASS: P10G filters only SEL+degree audition and reuses existing Gravity grid/visual/runtime')
PY

echo "PASS: P10G Gravity selected-point target layered above green P10F"
