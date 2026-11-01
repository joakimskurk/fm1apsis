#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 10F layers the host-proven Spiral Attractor editor above green P10E.
# The frozen M3c/M4f Spiral planner/DSP and P9 ROUTE dispatcher remain source
# authorities. ATTRACTOR is a finite control-session intercept only.
bash ci/apply_apsis_p10e.sh

# Target binding is downstream of the green host contract.
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Iapsis \
  apsis/apsis_harmony.c \
  apsis/apsis_spiral.c \
  apsis/apsis_controls_spiral_attractor.c \
  tests/test_controls_spiral_attractor.c \
  -o /tmp/test_controls_spiral_attractor
/tmp/test_controls_spiral_attractor

cp apsis/apsis_controls_spiral_attractor.h \
   apsis/apsis_controls_spiral_attractor.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_controls_string_attractor.h"'
if s.count(h_anchor) != 1:
    raise SystemExit(f'FAIL: P10F P10E header anchor count {s.count(h_anchor)} != 1')
s = s.replace(h_anchor,
              h_anchor + '\n#include "apsis_controls_spiral_attractor.h"', 1)

c_anchor = '#include "apsis_controls_string_attractor.c"'
if s.count(c_anchor) != 1:
    raise SystemExit(f'FAIL: P10F P10E source anchor count {s.count(c_anchor)} != 1')
s = s.replace(c_anchor,
              c_anchor + '\n#include "apsis_controls_spiral_attractor.c"', 1)

state_anchor = 'static uint8_t apsis_string_attractor_visual_dirty;'
if s.count(state_anchor) != 1:
    raise SystemExit(f'FAIL: P10F P10E state anchor count {s.count(state_anchor)} != 1')
s = s.replace(state_anchor, state_anchor +
              '\nstatic ApsisSpiralAttractorEditorSession apsis_spiral_attractor_editor;'
              '\nstatic uint8_t apsis_spiral_attractor_visual_dirty;', 1)

decl = '''    ApsisStringAttractorControlResult attractor_control;
    uint8_t attractor_editor_frame = 0u;
    uint8_t motion_control_valid = 0u;'''
if s.count(decl) != 1:
    raise SystemExit(f'FAIL: P10F dispatcher declaration anchor count {s.count(decl)} != 1')
s = s.replace(decl, '''    ApsisStringAttractorControlResult attractor_control;
    ApsisSpiralAttractorControlResult spiral_attractor_control;
    uint8_t attractor_editor_frame = 0u;
    uint8_t spiral_attractor_editor_frame = 0u;
    uint8_t motion_control_valid = 0u;''', 1)

# Run the Spiral context/editor immediately after the already-proven String
# context. ALGORITHM is unused by P9 ROUTE, so entry is an unambiguous intercept.
string_block = '''    (void)apsis_controls_string_attractor_context(
        &apsis_string_attractor_editor, current, &input, 0);
    if (current == SURF_STRING && apsis_string_attractor_editor.active) {
        attractor_editor_frame = apsis_controls_string_attractor_edit(
            &apsis.string, 1u, &input, &attractor_control);
        if (attractor_control.state_changed)
            apsis_string_attractor_visual_dirty = 1u;
    }
'''
if s.count(string_block) != 1:
    raise SystemExit(f'FAIL: P10F P10E String context block count {s.count(string_block)} != 1')
s = s.replace(string_block, string_block + '''    (void)apsis_controls_spiral_attractor_context(
        &apsis_spiral_attractor_editor, current, &input, 0);
    if (current == SURF_SPIRAL && apsis_spiral_attractor_editor.active) {
        spiral_attractor_editor_frame = apsis_controls_spiral_attractor_edit(
            &apsis_spiral_attractor_editor, &apsis.spiral, &input,
            &spiral_attractor_control);
        if (spiral_attractor_control.state_changed)
            apsis_spiral_attractor_visual_dirty = 1u;
    }
''', 1)

# P10E already intercepts normal String dispatch. Extend the same one shared
# dispatcher gate to Spiral ATTRACTOR; navigation and global transport are
# outside this surface dispatcher and remain available.
dispatch = '''    if (!attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current, &input,
                                last_abs_step, &result);'''
if s.count(dispatch) != 1:
    raise SystemExit(f'FAIL: P10F P10E dispatch gate count {s.count(dispatch)} != 1')
s = s.replace(dispatch, '''    if (!attractor_editor_frame && !spiral_attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current, &input,
                                last_abs_step, &result);''', 1)

# P10E has the single navigation/redraw site. Add the equivalent Spiral session
# reset and existing-P8 redraw invalidation there; no new renderer/framebuffer.
nav = '''        if (next != SURF_STRING) {
            apsis_controls_string_attractor_editor_reset(
                &apsis_string_attractor_editor);
            apsis_string_attractor_visual_dirty = 0u;
        } else if (apsis_string_attractor_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_string_attractor_visual_dirty = 0u;
        }'''
if s.count(nav) != 1:
    raise SystemExit(f'FAIL: P10F P10E navigation block count {s.count(nav)} != 1')
s = s.replace(nav, nav + '''
        if (next != SURF_SPIRAL) {
            apsis_controls_spiral_attractor_editor_reset(
                &apsis_spiral_attractor_editor);
            apsis_spiral_attractor_visual_dirty = 0u;
        } else if (apsis_spiral_attractor_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_spiral_attractor_visual_dirty = 0u;
        }''', 1)

boot = '''    apsis_controls_string_attractor_editor_reset(
        &apsis_string_attractor_editor);
    apsis_string_attractor_visual_dirty = 0u;'''
if s.count(boot) != 1:
    raise SystemExit(f'FAIL: P10F P10E boot anchor count {s.count(boot)} != 1')
s = s.replace(boot, boot + '''
    apsis_controls_spiral_attractor_editor_reset(
        &apsis_spiral_attractor_editor);
    apsis_spiral_attractor_visual_dirty = 0u;''', 1)

p.write_text(s)
PY

# Generated-target guardrails.
grep -q '#include "apsis_controls_spiral_attractor.c"' work/apsis/firmware/src/dinghy.c
grep -q 'static ApsisSpiralAttractorEditorSession apsis_spiral_attractor_editor;' work/apsis/firmware/src/dinghy.c
grep -q 'static uint8_t apsis_spiral_attractor_visual_dirty;' work/apsis/firmware/src/dinghy.c
grep -q '!attractor_editor_frame && !spiral_attractor_editor_frame' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_spiral_attractor_edit(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_spiral_attractor_visual_dirty = 1u;' work/apsis/firmware/src/dinghy.c
grep -q 'spiral->attractor_gates' work/apsis/firmware/src/apsis_visual_p8b.c
grep -q 'context->runtime->spiral_attractor_pos' work/apsis/firmware/src/apsis_visual_p8b.c
grep -q 'apsis_controls_spiral_append(' work/apsis/firmware/src/apsis_controls_dispatch.c
grep -q 'apsis_controls_spiral_insert(' work/apsis/firmware/src/apsis_controls_dispatch.c
grep -q 'apsis_controls_spiral_move_selected(' work/apsis/firmware/src/apsis_controls_dispatch.c
grep -q 'apsis_controls_spiral_delete_selected(' work/apsis/firmware/src/apsis_controls_dispatch.c

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
control = s[:a] + s[b:]

for forbidden in ('apsis_controls_spiral_attractor_context(',
                  'apsis_controls_spiral_attractor_edit(',
                  'apsis_spiral_attractor_visual_dirty'):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P10F editor/UI leaked into app_audio: {forbidden}')
for required in ('apsis_controls_spiral_attractor_context(',
                 'apsis_controls_spiral_attractor_edit(',
                 '!attractor_editor_frame && !spiral_attractor_editor_frame',
                 'apsis_controls_spiral_attractor_editor_reset('):
    if required not in control:
        raise SystemExit(f'FAIL: P10F target binding missing outside audio: {required}')
if s.count('apsis_controls_spiral_attractor_editor_reset(') != 2:
    raise SystemExit('FAIL: P10F expected boot + leave-surface Spiral editor resets only')
if s.count('apsis_spiral_attractor_visual_dirty = 1u;') != 1:
    raise SystemExit('FAIL: P10F expected exactly one Spiral visual-dirty producer')
renderer = Path('work/apsis/firmware/src/apsis_visual_p8b.c').read_text()
for required in ('spiral->attractor_gates',
                 'context->runtime->spiral_attractor_pos'):
    if required not in renderer:
        raise SystemExit(f'FAIL: P10F existing P8 Spiral renderer lost attractor authority: {required}')
# P9 ROUTE lives in the copied dispatch source, not textually inside dinghy.c.
# Keep that source as the authority guard so the check survives include order.
route_dispatch = Path('work/apsis/firmware/src/apsis_controls_dispatch.c').read_text()
for required in ('apsis_controls_spiral_append(',
                 'apsis_controls_spiral_insert(',
                 'apsis_controls_spiral_move_selected(',
                 'apsis_controls_spiral_delete_selected('):
    if required not in route_dispatch:
        raise SystemExit(f'FAIL: P10F disturbed P9 ROUTE authority: {required}')
for required in ('apsis_string_motion_mailbox_publish(',
                 'apsis_controls_performance_queue_string(',
                 'apsis_controls_performance_queue_bloom('):
    if required not in control:
        raise SystemExit(f'FAIL: P10F disturbed existing control path: {required}')
if 'static ApsisRuntime apsis_runtime;' not in s:
    raise SystemExit('FAIL: P10F disturbed shared runtime declaration')
print('PASS: P10F intercepts Spiral ATTRACTOR before P9 ROUTE and reuses existing P8 visual/runtime ownership')
PY

echo "PASS: P10F Spiral Attractor target layered above green P10E"
