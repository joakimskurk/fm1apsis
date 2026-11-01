#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Test-only telemetry above production P10F. It observes physical editor routing,
# source-authority reroll, route isolation and runtime health; no production
# musical state, scheduler, DSP or persistence contract is changed.
bash ci/apply_apsis_p10f.sh

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

state_anchor = 'static ApsisSpiralAttractorEditorSession apsis_spiral_attractor_editor;'
if s.count(state_anchor) != 1:
    raise SystemExit(f'FAIL: P10F probe session anchor count {s.count(state_anchor)} != 1')
s = s.replace(state_anchor, state_anchor + r'''
/* P10F active-emulator telemetry only. */
static volatile uint32_t apsis_spiral_attractor_probe[14];
static uint32_t apsis_spiral_attractor_probe_seen;
static uint32_t apsis_spiral_attractor_probe_violation;
static uint32_t apsis_spiral_attractor_probe_initial_gates;
static uint16_t apsis_spiral_attractor_probe_initial_seed;
static uint8_t apsis_spiral_attractor_probe_initial_route_count;
static uint32_t apsis_spiral_attractor_probe_initial_route_hash;

enum {
    APSIS_SPAT_SEEN_ENTER = 1u << 0,
    APSIS_SPAT_SEEN_REC = 1u << 1,
    APSIS_SPAT_SEEN_REROLL = 1u << 2,
    APSIS_SPAT_SEEN_NOTE = 1u << 3,
    APSIS_SPAT_SEEN_GATE = 1u << 4,
    APSIS_SPAT_SEEN_EXIT = 1u << 5
};

enum {
    APSIS_SPAT_VIOL_ROUTE = 1u << 0
};''', 1)

helper_anchor = 'static uint8_t apsis_p9h3_dispatch('
if helper_anchor not in s:
    raise SystemExit('FAIL: P10F probe dispatcher anchor missing')
helper = r'''static uint32_t apsis_p10f_route_hash(void)
{
    uint32_t h = 2166136261u;
    uint8_t count = apsis.spiral.route_count > APSIS_SPIRAL_ROUTE_MAX
        ? APSIS_SPIRAL_ROUTE_MAX : apsis.spiral.route_count;
    uint8_t i;
    h = (h ^ count) * 16777619u;
    for (i = 0u; i < count; ++i)
        h = (h ^ apsis.spiral.route[i]) * 16777619u;
    return h;
}

static void apsis_p10f_spiral_attractor_probe_update(uint8_t current)
{
    uint32_t health;
    uint32_t route_hash = apsis_p10f_route_hash();
    if (apsis.spiral.route_count != apsis_spiral_attractor_probe_initial_route_count ||
        route_hash != apsis_spiral_attractor_probe_initial_route_hash)
        apsis_spiral_attractor_probe_violation |= APSIS_SPAT_VIOL_ROUTE;

    health = (apsis_runtime_probe[1] & 0x3ffu) |
             ((apsis_runtime_probe[2] ? 1u : 0u) << 10) |
             ((apsis_runtime_probe[4] & 0xffffu) << 16);
    apsis_spiral_attractor_probe[0] = 0x53504154u; /* "SPAT" */
    apsis_spiral_attractor_probe[1] = apsis_spiral_attractor_probe_seen;
    apsis_spiral_attractor_probe[2] = apsis_spiral_attractor_probe_violation;
    apsis_spiral_attractor_probe[3] = apsis_spiral_attractor_probe_initial_gates;
    apsis_spiral_attractor_probe[4] = apsis.spiral.attractor_gates;
    apsis_spiral_attractor_probe[5] = apsis_spiral_attractor_probe_initial_seed;
    apsis_spiral_attractor_probe[6] = apsis.spiral.attractor_seed;
    apsis_spiral_attractor_probe[7] =
        (uint32_t)apsis_spiral_attractor_probe_initial_route_count |
        ((uint32_t)apsis.spiral.route_count << 16);
    apsis_spiral_attractor_probe[8] = apsis_spiral_attractor_probe_initial_route_hash;
    apsis_spiral_attractor_probe[9] = route_hash;
    apsis_spiral_attractor_probe[10] =
        (apsis_spiral_attractor_editor.active ? 1u : 0u) |
        ((uint32_t)current << 8);
    apsis_spiral_attractor_probe[11] = health;
    apsis_spiral_attractor_probe[12] = (uint32_t)apsis_runtime.cursor_sample;
    apsis_spiral_attractor_probe[13] =
        (uint32_t)apsis_spiral_attractor_editor.selected_cell |
        ((uint32_t)apsis.spiral.attractor_length << 8) |
        ((uint32_t)apsis.spiral.attractor_rate << 16);
}

'''
s = s.replace(helper_anchor, helper + helper_anchor, 1)

editor = '''    (void)apsis_controls_spiral_attractor_context(
        &apsis_spiral_attractor_editor, current, &input, 0);
    if (current == SURF_SPIRAL && apsis_spiral_attractor_editor.active) {
        spiral_attractor_editor_frame = apsis_controls_spiral_attractor_edit(
            &apsis_spiral_attractor_editor, &apsis.spiral, &input,
            &spiral_attractor_control);
        if (spiral_attractor_control.state_changed)
            apsis_spiral_attractor_visual_dirty = 1u;
    }
'''
if s.count(editor) != 1:
    raise SystemExit(f'FAIL: P10F probe editor block count {s.count(editor)} != 1')
s = s.replace(editor, '''    {
        uint8_t editor_before = apsis_spiral_attractor_editor.active;
        (void)apsis_controls_spiral_attractor_context(
            &apsis_spiral_attractor_editor, current, &input, 0);
        if (!editor_before && apsis_spiral_attractor_editor.active)
            apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_ENTER;
        if (editor_before && !apsis_spiral_attractor_editor.active)
            apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_EXIT;
        if (current == SURF_SPIRAL && apsis_spiral_attractor_editor.active) {
            spiral_attractor_editor_frame = apsis_controls_spiral_attractor_edit(
                &apsis_spiral_attractor_editor, &apsis.spiral, &input,
                &spiral_attractor_control);
            if (input.buttons_pressed & (1u << APSIS_FM1_BTN_REC))
                apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_REC;
            if (spiral_attractor_control.rerolled)
                apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_REROLL;
            if (input.notes_pressed)
                apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_NOTE;
            if (spiral_attractor_control.toggled_gates)
                apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_GATE;
            if (spiral_attractor_control.state_changed)
                apsis_spiral_attractor_visual_dirty = 1u;
        }
    }
''', 1)

dispatch = '''    if (!attractor_editor_frame && !spiral_attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                                &chain_sequence, &mother_pending, current, &input,
                                last_abs_step, &result);'''
if s.count(dispatch) != 1:
    raise SystemExit(f'FAIL: P10F probe dispatch gate count {s.count(dispatch)} != 1')
s = s.replace(dispatch, dispatch + '''
    apsis_p10f_spiral_attractor_probe_update(current);''', 1)

nav = '''        if (next != SURF_SPIRAL) {
            apsis_controls_spiral_attractor_editor_reset(
                &apsis_spiral_attractor_editor);
            apsis_spiral_attractor_visual_dirty = 0u;
        } else if (apsis_spiral_attractor_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_spiral_attractor_visual_dirty = 0u;
        }'''
if s.count(nav) != 1:
    raise SystemExit(f'FAIL: P10F probe navigation block count {s.count(nav)} != 1')
s = s.replace(nav, '''        if (next != SURF_SPIRAL) {
            if (apsis_spiral_attractor_editor.active)
                apsis_spiral_attractor_probe_seen |= APSIS_SPAT_SEEN_EXIT;
            apsis_controls_spiral_attractor_editor_reset(
                &apsis_spiral_attractor_editor);
            apsis_spiral_attractor_visual_dirty = 0u;
        } else if (apsis_spiral_attractor_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_spiral_attractor_visual_dirty = 0u;
        }
        apsis_p10f_spiral_attractor_probe_update(next);''', 1)

boot = '''    apsis_controls_spiral_attractor_editor_reset(
        &apsis_spiral_attractor_editor);
    apsis_spiral_attractor_visual_dirty = 0u;'''
if s.count(boot) != 1:
    raise SystemExit(f'FAIL: P10F probe boot anchor count {s.count(boot)} != 1')
s = s.replace(boot, boot + r'''
    apsis_spiral_attractor_probe_seen = 0u;
    apsis_spiral_attractor_probe_violation = 0u;
    apsis_spiral_attractor_probe_initial_gates = apsis.spiral.attractor_gates;
    apsis_spiral_attractor_probe_initial_seed = apsis.spiral.attractor_seed;
    apsis_spiral_attractor_probe_initial_route_count = apsis.spiral.route_count;
    apsis_spiral_attractor_probe_initial_route_hash = apsis_p10f_route_hash();
    apsis_p10f_spiral_attractor_probe_update(SURF_MOTHER);''', 1)

p.write_text(s)
PY

grep -q 'static volatile uint32_t apsis_spiral_attractor_probe\[14\]' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_SPAT_SEEN_REROLL' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_SPAT_VIOL_ROUTE' work/apsis/firmware/src/dinghy.c
[[ $(grep -c 'apsis_spiral_attractor_probe_seen = 0u;' work/apsis/firmware/src/dinghy.c) -eq 1 ]]

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
if 'apsis_p10f_spiral_attractor_probe_update(' in s[a:b]:
    raise SystemExit('FAIL: P10F active probe leaked into app_audio')
if s.count('apsis_spiral_attractor_probe_seen = 0u;') != 1:
    raise SystemExit('FAIL: P10F active probe has multiple history-reset sites')
print('PASS: P10F active telemetry is test-only, route-observing and main/control-side')
PY

echo "PASS: P10F active Spiral Attractor telemetry layered test-only above production target"
