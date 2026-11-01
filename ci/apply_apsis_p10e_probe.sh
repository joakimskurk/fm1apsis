#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Test-only telemetry layered above the production P10E target. The probe
# observes physical editor routing and leakage; it does not alter musical state,
# scheduling, DSP, persistence or the production P10E contract.
bash ci/apply_apsis_p10e.sh

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

state_anchor = 'static ApsisStringAttractorEditorSession apsis_string_attractor_editor;'
if s.count(state_anchor) != 1:
    raise SystemExit(f'FAIL: P10E probe editor-session anchor count {s.count(state_anchor)} != 1')
s = s.replace(state_anchor, state_anchor + r'''
/* P10E active-emulator telemetry only. */
static volatile uint32_t apsis_attractor_probe[12];
static uint32_t apsis_attractor_probe_seen;
static uint32_t apsis_attractor_probe_violation;
static uint16_t apsis_attractor_probe_initial_gates;
static uint16_t apsis_attractor_probe_initial_seed;

enum {
    APSIS_ATTR_SEEN_ENTER = 1u << 0,
    APSIS_ATTR_SEEN_NOTE = 1u << 1,
    APSIS_ATTR_SEEN_GATE = 1u << 2,
    APSIS_ATTR_SEEN_REC = 1u << 3,
    APSIS_ATTR_SEEN_REROLL = 1u << 4,
    APSIS_ATTR_SEEN_EXIT = 1u << 5
};

enum {
    APSIS_ATTR_VIOL_P9_PLUCK = 1u << 0,
    APSIS_ATTR_VIOL_MOTION_DECODE = 1u << 1,
    APSIS_ATTR_VIOL_MOTION_STATE = 1u << 2,
    APSIS_ATTR_VIOL_MOTION_MAILBOX = 1u << 3
};''', 1)

# Keep the telemetry formatter in main/control territory. It only reads current
# state plus established runtime health counters.
dispatch_anchor = 'static uint8_t apsis_p9h3_dispatch('
if dispatch_anchor not in s:
    raise SystemExit('FAIL: P10E probe dispatcher anchor missing')
helper = r'''static void apsis_p10e_attractor_probe_update(uint8_t current)
{
    uint32_t motion = 0u;
    uint32_t health;
    if (apsis_string_motion_runtime.armed) motion |= 1u << 0;
    if (apsis_string_motion_runtime.recording) motion |= 1u << 1;
    if (apsis.string.motion_loop_play) motion |= 1u << 2;
    if (apsis_string_motion_runtime.manual_override) motion |= 1u << 3;
    if (apsis_string_motion_playback_valid) motion |= 1u << 4;
    health = (apsis_runtime.queue_high_water & 0x3ffu) |
             ((apsis_runtime.queue_overflowed ? 1u : 0u) << 10) |
             ((apsis_runtime.late_flags & 0xffffu) << 16);

    apsis_attractor_probe[0] = 0x41545452u; /* "ATTR" */
    apsis_attractor_probe[1] = apsis_attractor_probe_seen;
    apsis_attractor_probe[2] = apsis_attractor_probe_violation;
    apsis_attractor_probe[3] = apsis_attractor_probe_initial_gates;
    apsis_attractor_probe[4] = apsis.string.filter_attractor_gates;
    apsis_attractor_probe[5] = apsis_attractor_probe_initial_seed;
    apsis_attractor_probe[6] = apsis.string.filter_attractor_seed;
    apsis_attractor_probe[7] = (apsis_string_attractor_editor.active ? 1u : 0u) |
                              ((uint32_t)current << 8);
    apsis_attractor_probe[8] = motion;
    apsis_attractor_probe[9] = health;
    apsis_attractor_probe[10] = (uint32_t)apsis_runtime.cursor_sample;
    apsis_attractor_probe[11] = apsis_string_motion_mailbox.pending ? 1u : 0u;
}

'''
s = s.replace(dispatch_anchor, helper + dispatch_anchor, 1)

# Replace the exact production context/edit block with an observational version.
editor = '''    (void)apsis_controls_string_attractor_context(
        &apsis_string_attractor_editor, current, &input, 0);
    if (current == SURF_STRING && apsis_string_attractor_editor.active)
        attractor_editor_frame = apsis_controls_string_attractor_edit(
            &apsis.string, 1u, &input, &attractor_control);
'''
if s.count(editor) != 1:
    raise SystemExit(f'FAIL: P10E probe editor block count {s.count(editor)} != 1')
s = s.replace(editor, '''    {
        uint8_t editor_before = apsis_string_attractor_editor.active;
        (void)apsis_controls_string_attractor_context(
            &apsis_string_attractor_editor, current, &input, 0);
        if (!editor_before && apsis_string_attractor_editor.active)
            apsis_attractor_probe_seen |= APSIS_ATTR_SEEN_ENTER;
        if (current == SURF_STRING && apsis_string_attractor_editor.active) {
            attractor_editor_frame = apsis_controls_string_attractor_edit(
                &apsis.string, 1u, &input, &attractor_control);
            if (input.notes_pressed)
                apsis_attractor_probe_seen |= APSIS_ATTR_SEEN_NOTE;
            if (attractor_control.toggled_gates)
                apsis_attractor_probe_seen |= APSIS_ATTR_SEEN_GATE;
            if (input.buttons_pressed & (1u << APSIS_FM1_BTN_REC))
                apsis_attractor_probe_seen |= APSIS_ATTR_SEEN_REC;
            if (attractor_control.rerolled)
                apsis_attractor_probe_seen |= APSIS_ATTR_SEEN_REROLL;
        }
    }
''', 1)

# Immediately after the production dispatcher gate, prove an editor-consumed
# frame has not populated any P9 String performance result or Motion producer
# intent. A pending Motion mailbox on such a frame is also a routing violation.
dispatch = '''    if (!attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current, &input,
                                last_abs_step, &result);
'''
if s.count(dispatch) != 1:
    raise SystemExit(f'FAIL: P10E probe dispatch gate count {s.count(dispatch)} != 1')
s = s.replace(dispatch, dispatch + '''    if (attractor_editor_frame) {
        if (result.string_pluck)
            apsis_attractor_probe_violation |= APSIS_ATTR_VIOL_P9_PLUCK;
        if (motion_control_valid || motion_capture || motion_manual_valid ||
            motion_manual_active)
            apsis_attractor_probe_violation |= APSIS_ATTR_VIOL_MOTION_DECODE;
        if (apsis_string_motion_runtime.armed ||
            apsis_string_motion_runtime.recording)
            apsis_attractor_probe_violation |= APSIS_ATTR_VIOL_MOTION_STATE;
        if (apsis_string_motion_mailbox.pending)
            apsis_attractor_probe_violation |= APSIS_ATTR_VIOL_MOTION_MAILBOX;
    }
    apsis_p10e_attractor_probe_update(current);
''', 1)

# Observe the production leave-STRING reset without changing its semantics.
nav = '''        if (next != SURF_STRING)
            apsis_controls_string_attractor_editor_reset(
                &apsis_string_attractor_editor);'''
if s.count(nav) != 1:
    raise SystemExit(f'FAIL: P10E probe leave-STRING anchor count {s.count(nav)} != 1')
s = s.replace(nav, '''        if (next != SURF_STRING) {
            if (apsis_string_attractor_editor.active)
                apsis_attractor_probe_seen |= APSIS_ATTR_SEEN_EXIT;
            apsis_controls_string_attractor_editor_reset(
                &apsis_string_attractor_editor);
            apsis_p10e_attractor_probe_update(next);
        }''', 1)

# Initialize only at the unique production boot reset, after ApsisState defaults
# already exist. This is deliberately an exact boot anchor, never a generic
# assignment in a runtime helper (the P10D3 lesson).
boot = '''    apsis_controls_string_attractor_editor_reset(
        &apsis_string_attractor_editor);'''
if s.count(boot) != 1:
    raise SystemExit(f'FAIL: P10E probe boot editor-reset count {s.count(boot)} != 1')
s = s.replace(boot, boot + r'''
    apsis_attractor_probe_seen = 0u;
    apsis_attractor_probe_violation = 0u;
    apsis_attractor_probe_initial_gates = apsis.string.filter_attractor_gates;
    apsis_attractor_probe_initial_seed = apsis.string.filter_attractor_seed;
    apsis_p10e_attractor_probe_update(SURF_MOTHER);''', 1)

p.write_text(s)
PY

grep -q 'static volatile uint32_t apsis_attractor_probe\[12\]' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_ATTR_SEEN_REROLL' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_ATTR_VIOL_P9_PLUCK' work/apsis/firmware/src/dinghy.c
[[ $(grep -c 'apsis_attractor_probe_seen = 0u;' work/apsis/firmware/src/dinghy.c) -eq 1 ]]

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
if 'apsis_p10e_attractor_probe_update(' in s[a:b]:
    raise SystemExit('FAIL: P10E active probe leaked into app_audio')
if s.count('apsis_attractor_probe_seen = 0u;') != 1:
    raise SystemExit('FAIL: P10E active probe has multiple history-reset sites')
print('PASS: P10E active editor telemetry is test-only and main/control-side')
PY

echo "PASS: P10E active String Attractor telemetry layered test-only above production target"
