#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 10E layers only the host-proven String Attractor editor above the green
# production P10D2 Motion target. M4c, P9 dispatch/performance and Motion remain
# untouched source authorities; this generator adds a finite UI-session bit and
# consumes editor frames before those ordinary String interpreters see them.
bash ci/apply_apsis_p10d2.sh

# Re-run the focused host contract before a generated target is allowed to use
# the editor source. This keeps target binding downstream of the P10E host gate.
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Iapsis \
  apsis/apsis_string_attractor.c \
  apsis/apsis_controls_string_attractor.c \
  tests/test_controls_string_attractor.c \
  -o /tmp/test_controls_string_attractor
/tmp/test_controls_string_attractor

cp apsis/apsis_controls_string_attractor.h \
   apsis/apsis_controls_string_attractor.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_string_motion_mailbox.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P10E P10D2 header anchor missing')
s = s.replace(h_anchor,
              h_anchor + '\n#include "apsis_controls_string_attractor.h"', 1)

c_anchor = '#include "apsis_string_motion_mailbox.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P10E P10D2 source anchor missing')
s = s.replace(c_anchor,
              c_anchor + '\n#include "apsis_controls_string_attractor.c"', 1)

state_anchor = 'static uint8_t apsis_string_motion_playback_valid;'
if state_anchor not in s:
    raise SystemExit('FAIL: P10E P10D2 state anchor missing')
s = s.replace(state_anchor, state_anchor +
              '\nstatic ApsisStringAttractorEditorSession apsis_string_attractor_editor;'
              '\nstatic uint8_t apsis_string_attractor_visual_dirty;', 1)

# P9H4a/H4b consume the dispatch result after the H3 dispatcher call. When an
# ATTRACTOR frame deliberately bypasses ordinary P9 STRING dispatch, result must
# therefore be a defined all-zero no-performance result rather than stack data.
decl = '''    ApsisFm1InputFrame input;
    ApsisControlsDispatchResult result;
    ApsisStringMotionControlResult motion_control;
    uint8_t motion_control_valid = 0u;
    uint8_t motion_capture = 0u;
    uint8_t motion_manual_valid = 0u;
    uint8_t motion_manual_active = 0u;
    int32_t last_abs_step;'''
if decl not in s:
    raise SystemExit('FAIL: P10E P10D2 dispatcher declaration anchor missing')
s = s.replace(decl, '''    ApsisFm1InputFrame input;
    ApsisControlsDispatchResult result = {0};
    ApsisStringMotionControlResult motion_control;
    ApsisStringAttractorControlResult attractor_control;
    uint8_t attractor_editor_frame = 0u;
    uint8_t motion_control_valid = 0u;
    uint8_t motion_capture = 0u;
    uint8_t motion_manual_valid = 0u;
    uint8_t motion_manual_active = 0u;
    int32_t last_abs_step;''', 1)

# Context selection runs immediately after physical decoding. SEL+ENV can thus
# enter from MOTHER before ENV navigation lands on STRING. Once on STRING, an
# active editor consumes the full frame before Motion or ordinary P9 STRING can
# interpret note/REC/encoder activity. Navigation is intentionally not blocked.
take = '    apsis_p9h3_take_input(packed, &input);\n'
if take not in s:
    raise SystemExit('FAIL: P10E P10D2 input-take anchor missing')
s = s.replace(take, take + '''    (void)apsis_controls_string_attractor_context(
        &apsis_string_attractor_editor, current, &input, 0);
    if (current == SURF_STRING && apsis_string_attractor_editor.active) {
        attractor_editor_frame = apsis_controls_string_attractor_edit(
            &apsis.string, 1u, &input, &attractor_control);
        if (attractor_control.state_changed)
            apsis_string_attractor_visual_dirty = 1u;
    }
''', 1)

motion = '''    if (current == SURF_STRING) {
        motion_control_valid = apsis_controls_string_motion(
            &apsis, apsis_string_motion_runtime.record_target,
            &input, &motion_control);'''
if motion not in s:
    raise SystemExit('FAIL: P10E P10D2 Motion-decode anchor missing')
s = s.replace(motion, '''    if (current == SURF_STRING && !attractor_editor_frame) {
        motion_control_valid = apsis_controls_string_motion(
            &apsis, apsis_string_motion_runtime.record_target,
            &input, &motion_control);''', 1)

dispatch = '''    apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                            &chain_sequence, &mother_pending, current, &input,
                            last_abs_step, &result);
'''
if dispatch not in s:
    raise SystemExit('FAIL: P10E P10D2 dispatcher-call anchor missing')
s = s.replace(dispatch, '''    if (!attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current, &input,
                                last_abs_step, &result);
''', 1)

# Leaving STRING must clear the UI-only editor bit immediately, not merely on a
# later input frame. Entry from another surface is retained because ENV resolves
# to STRING before this post-router check. A changed editor frame invalidates
# only the existing P8 native STRING draw; it adds no framebuffer or renderer.
nav = '        next = surface_from_buttons(pressed, surface);'
if s.count(nav) != 1:
    raise SystemExit(f'FAIL: P10E expected one main navigation site, got {s.count(nav)}')
s = s.replace(nav, nav + '''
        if (next != SURF_STRING) {
            apsis_controls_string_attractor_editor_reset(
                &apsis_string_attractor_editor);
            apsis_string_attractor_visual_dirty = 0u;
        } else if (apsis_string_attractor_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_string_attractor_visual_dirty = 0u;
        }''', 1)

# Boot initialization: anchor the unique Motion mailbox sequence so we do not
# repeat the P10D3 generic-assignment mistake.
boot = '''    apsis_string_motion_mailbox_reset(&apsis_string_motion_mailbox);
    apsis_string_motion_active = 0u;
    apsis_string_motion_playback_valid = 0u;'''
if s.count(boot) != 1:
    raise SystemExit(f'FAIL: P10E unique Motion boot anchor count {s.count(boot)} != 1')
s = s.replace(boot, boot + '''
    apsis_controls_string_attractor_editor_reset(
        &apsis_string_attractor_editor);
    apsis_string_attractor_visual_dirty = 0u;''', 1)

p.write_text(s)
PY

# Generated-target guardrails.
grep -q '#include "apsis_controls_string_attractor.c"' work/apsis/firmware/src/dinghy.c
grep -q 'static ApsisStringAttractorEditorSession apsis_string_attractor_editor;' work/apsis/firmware/src/dinghy.c
grep -q 'static uint8_t apsis_string_attractor_visual_dirty;' work/apsis/firmware/src/dinghy.c
grep -q 'ApsisControlsDispatchResult result = {0};' work/apsis/firmware/src/dinghy.c
grep -q 'current == SURF_STRING && !attractor_editor_frame' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_string_attractor_edit(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_string_attractor_editor_reset(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_attractor_visual_dirty = 1u;' work/apsis/firmware/src/dinghy.c
# P8 compiles the renderer by including its copied source file from dinghy.c;
# validate both the include and the actual gate-read expression in that source.
grep -q '#include "apsis_visual_p8b.c"' work/apsis/firmware/src/dinghy.c
grep -q 'filter_attractor_gates >> i' work/apsis/firmware/src/apsis_visual_p8b.c

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
# P9 layers place some control helpers after surface_name, while app_main and
# dispatcher code are generated in a different source order. Validate the whole
# non-audio translation unit instead of assuming those helpers precede b.
control = s[:a] + s[b:]

for forbidden in ('apsis_controls_string_attractor_context(',
                  'apsis_controls_string_attractor_edit(',
                  'apsis_string_attractor_visual_dirty'):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P10E editor/UI work leaked into app_audio: {forbidden}')
for required in ('apsis_controls_string_attractor_context(',
                 'apsis_controls_string_attractor_edit(',
                 'if (!attractor_editor_frame)',
                 'current == SURF_STRING && !attractor_editor_frame'):
    if required not in control:
        raise SystemExit(f'FAIL: P10E target binding missing outside app_audio: {required}')
if s.count('apsis_controls_string_attractor_editor_reset(') != 2:
    raise SystemExit('FAIL: P10E expected boot + leave-surface editor resets only')
if s.count('apsis_string_attractor_visual_dirty = 1u;') != 1:
    raise SystemExit('FAIL: P10E expected exactly one editor visual-dirty producer')
for required in ('drawn = 0xFFu;',
                 'apsis_native_draw_active = 0u;',
                 'apsis_native_draw_ui_surface = 0xFFu;'):
    if required not in control:
        raise SystemExit(f'FAIL: P10E missing existing-P8 redraw invalidation outside audio: {required}')
renderer = Path('work/apsis/firmware/src/apsis_visual_p8b.c').read_text()
if 'filter_attractor_gates >> i' not in renderer:
    raise SystemExit('FAIL: P10E existing P8 STRING renderer no longer reads attractor gates')
for required in ('apsis_string_motion_mailbox_publish(',
                 'apsis_controls_performance_queue_string(',
                 'apsis_controls_performance_queue_bloom('):
    if required not in control:
        raise SystemExit(f'FAIL: P10E disturbed existing non-audio control path: {required}')
for required in ('project_store.save_pending && !apsis_runtime.transport.running',
                 'apsis_project_fm1_service_save('):
    if required not in control:
        raise SystemExit(f'FAIL: P10E disturbed P7 deferred save path: {required}')
if 'static ApsisRuntime apsis_runtime;' not in s:
    raise SystemExit('FAIL: P10E disturbed shared runtime declaration')
print('PASS: P10E editor consumes STRING frames before P9/Motion and invalidates only the existing P8 visual path')
PY

echo "PASS: P10E String Attractor editor target layered above green P10D2"
