#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Test-only telemetry above production P10G. It proves filtered SEL+degree,
# selected-point mutation, preserved plain degree request and runtime health.
bash ci/apply_apsis_p10g.sh

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

state_anchor = 'static ApsisGravityPointEditorSession apsis_gravity_point_editor;'
if s.count(state_anchor) != 1:
    raise SystemExit(f'FAIL: P10G probe session anchor count {s.count(state_anchor)} != 1')
s = s.replace(state_anchor, state_anchor + r'''
/* P10G active-emulator telemetry only. Target point is degree 4 / anchor ring 0. */
static volatile uint32_t apsis_gravity_point_probe[18];
static uint32_t apsis_gravity_point_probe_seen;
static uint32_t apsis_gravity_point_probe_violation;
static uint32_t apsis_gravity_point_probe_initial_other_hash;
static uint32_t apsis_gravity_point_probe_initial_attrs;
static uint16_t apsis_gravity_point_probe_initial_level;
static uint16_t apsis_gravity_point_probe_initial_brightness;
static uint16_t apsis_gravity_point_probe_initial_attack;
static uint16_t apsis_gravity_point_probe_initial_release;
static uint8_t apsis_gravity_point_probe_plain_degree;

enum {
    APSIS_GRPT_SEEN_POINT_PAGE = 1u << 0,
    APSIS_GRPT_SEEN_SELECT = 1u << 1,
    APSIS_GRPT_SEEN_SUPPRESS = 1u << 2,
    APSIS_GRPT_SEEN_EDIT = 1u << 3,
    APSIS_GRPT_SEEN_REC = 1u << 4,
    APSIS_GRPT_SEEN_PLAIN_REQUEST = 1u << 5,
    APSIS_GRPT_SEEN_EXIT = 1u << 6
};

enum {
    APSIS_GRPT_VIOL_SELECT_REQUEST = 1u << 0,
    APSIS_GRPT_VIOL_OTHER_POINT = 1u << 1,
    APSIS_GRPT_VIOL_PLAIN_DEGREE = 1u << 2
};''', 1)

helper_anchor = 'static uint8_t apsis_p9h3_dispatch('
if helper_anchor not in s:
    raise SystemExit('FAIL: P10G probe dispatcher anchor missing')
helper = r'''static uint32_t apsis_p10g_other_point_hash(void)
{
    uint32_t h = 2166136261u;
    uint8_t i;
    for (i = 0u; i < APSIS_GRAVITY_POINTS; ++i) {
        const ApsisGravityPoint *p;
        if (i == 12u) continue; /* degree 4 * 3 + anchor ring 0 */
        p = &apsis.gravity.points[i];
        h = (h ^ p->enabled) * 16777619u;
        h = (h ^ (uint8_t)p->offset) * 16777619u;
        h = (h ^ (uint8_t)p->octave) * 16777619u;
        h = (h ^ p->level_q15) * 16777619u;
        h = (h ^ p->brightness_q15) * 16777619u;
        h = (h ^ p->attack_ms) * 16777619u;
        h = (h ^ p->release_ms) * 16777619u;
    }
    return h;
}

static uint32_t apsis_p10g_attrs(const ApsisGravityPoint *p)
{
    return (p->enabled ? 1u : 0u) |
           ((uint32_t)(uint8_t)p->offset << 8) |
           ((uint32_t)(uint8_t)p->octave << 16);
}

static void apsis_p10g_gravity_point_probe_update(uint8_t current)
{
    const ApsisGravityPoint *p = &apsis.gravity.points[12u];
    uint32_t health = (apsis_runtime_probe[1] & 0x3ffu) |
                      ((apsis_runtime_probe[2] ? 1u : 0u) << 10) |
                      ((apsis_runtime_probe[4] & 0xffffu) << 16);
    uint32_t other_hash = apsis_p10g_other_point_hash();
    if (other_hash != apsis_gravity_point_probe_initial_other_hash)
        apsis_gravity_point_probe_violation |= APSIS_GRPT_VIOL_OTHER_POINT;

    apsis_gravity_point_probe[0] = 0x47525054u; /* GRPT */
    apsis_gravity_point_probe[1] = apsis_gravity_point_probe_seen;
    apsis_gravity_point_probe[2] = apsis_gravity_point_probe_violation;
    apsis_gravity_point_probe[3] = apsis_gravity_point_probe_initial_attrs;
    apsis_gravity_point_probe[4] = apsis_gravity_point_probe_initial_level;
    apsis_gravity_point_probe[5] = apsis_gravity_point_probe_initial_brightness;
    apsis_gravity_point_probe[6] = (uint32_t)apsis_gravity_point_probe_initial_attack |
                                   ((uint32_t)apsis_gravity_point_probe_initial_release << 16);
    apsis_gravity_point_probe[7] = apsis_p10g_attrs(p);
    apsis_gravity_point_probe[8] = p->level_q15;
    apsis_gravity_point_probe[9] = p->brightness_q15;
    apsis_gravity_point_probe[10] = (uint32_t)p->attack_ms |
                                    ((uint32_t)p->release_ms << 16);
    apsis_gravity_point_probe[11] =
        (uint32_t)apsis.gravity.selected_point_degree |
        ((uint32_t)apsis.gravity.selected_ring << 8) |
        ((uint32_t)apsis_gravity_point_editor.page << 16) |
        ((uint32_t)current << 24);
    apsis_gravity_point_probe[12] = apsis_gravity_point_probe_plain_degree;
    apsis_gravity_point_probe[13] = health;
    apsis_gravity_point_probe[14] = (uint32_t)apsis_runtime.cursor_sample;
    apsis_gravity_point_probe[15] = apsis_gravity_point_probe_initial_other_hash;
    apsis_gravity_point_probe[16] = other_hash;
    apsis_gravity_point_probe[17] =
        (uint32_t)apsis.gravity.degree |
        ((uint32_t)(apsis_runtime.gravity_pending.active ? 1u : 0u) << 8) |
        ((uint32_t)apsis_runtime.gravity_pending.degree << 16);
}

'''
s = s.replace(helper_anchor, helper + helper_anchor, 1)

block = '''    (void)apsis_controls_gravity_point_context(
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
'''
if s.count(block) != 1:
    raise SystemExit(f'FAIL: P10G probe editor block count {s.count(block)} != 1')
s = s.replace(block, '''    (void)apsis_controls_gravity_point_context(
        &apsis_gravity_point_editor, current, &input, &gravity_point_context);
    if (gravity_point_context.entered_point)
        apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_POINT_PAGE;
    if (gravity_point_context.changed)
        apsis_gravity_point_visual_dirty = 1u;
    if (current == SURF_GRAVITY &&
        apsis_gravity_point_editor.page == APSIS_GRAVITY_PAGE_POINT) {
        (void)apsis_controls_gravity_point_edit(
            &apsis_gravity_point_editor, &apsis.gravity, &input,
            &gravity_point_control);
        if (gravity_point_control.selection_changed)
            apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_SELECT;
        if (gravity_point_control.suppress_degree_request)
            apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_SUPPRESS;
        if (gravity_point_control.state_changed &&
            !gravity_point_control.selection_changed)
            apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_EDIT;
        if (input.buttons_pressed & (1u << APSIS_FM1_BTN_REC))
            apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_REC;
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
                                &chain_sequence, &mother_pending, current,
                                dispatch_input, last_abs_step, &result);'''
if s.count(dispatch) != 1:
    raise SystemExit(f'FAIL: P10G probe dispatch count {s.count(dispatch)} != 1')
s = s.replace(dispatch, dispatch + '''
    if (gravity_point_control.suppress_degree_request && result.gravity_request)
        apsis_gravity_point_probe_violation |= APSIS_GRPT_VIOL_SELECT_REQUEST;
    if (current == SURF_GRAVITY &&
        apsis_gravity_point_editor.page == APSIS_GRAVITY_PAGE_POINT &&
        (input.notes_pressed & 0xffu) &&
        !(input.buttons_held & (1u << APSIS_FM1_BTN_SEL)) &&
        result.gravity_request) {
        apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_PLAIN_REQUEST;
        apsis_gravity_point_probe_plain_degree = result.gravity_degree;
        if (result.gravity_degree != 6u)
            apsis_gravity_point_probe_violation |= APSIS_GRPT_VIOL_PLAIN_DEGREE;
    }
    apsis_p10g_gravity_point_probe_update(current);''', 1)

nav = '''        if (next != SURF_GRAVITY) {
            apsis_controls_gravity_point_editor_reset(
                &apsis_gravity_point_editor);
            apsis_gravity_point_visual_dirty = 0u;
        } else if (apsis_gravity_point_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_gravity_point_visual_dirty = 0u;
        }'''
if s.count(nav) != 1:
    raise SystemExit(f'FAIL: P10G probe navigation count {s.count(nav)} != 1')
s = s.replace(nav, '''        if (next != SURF_GRAVITY) {
            if (apsis_gravity_point_editor.page == APSIS_GRAVITY_PAGE_POINT)
                apsis_gravity_point_probe_seen |= APSIS_GRPT_SEEN_EXIT;
            apsis_controls_gravity_point_editor_reset(
                &apsis_gravity_point_editor);
            apsis_gravity_point_visual_dirty = 0u;
        } else if (apsis_gravity_point_visual_dirty) {
            drawn = 0xFFu;
            apsis_native_draw_active = 0u;
            apsis_native_draw_ui_surface = 0xFFu;
            apsis_gravity_point_visual_dirty = 0u;
        }
        apsis_p10g_gravity_point_probe_update(next);''', 1)

boot = '''    apsis_controls_gravity_point_editor_reset(
        &apsis_gravity_point_editor);
    apsis_gravity_point_visual_dirty = 0u;'''
if s.count(boot) != 1:
    raise SystemExit(f'FAIL: P10G probe boot count {s.count(boot)} != 1')
s = s.replace(boot, boot + r'''
    {
        const ApsisGravityPoint *p = &apsis.gravity.points[12u];
        apsis_gravity_point_probe_seen = 0u;
        apsis_gravity_point_probe_violation = 0u;
        apsis_gravity_point_probe_initial_attrs = apsis_p10g_attrs(p);
        apsis_gravity_point_probe_initial_level = p->level_q15;
        apsis_gravity_point_probe_initial_brightness = p->brightness_q15;
        apsis_gravity_point_probe_initial_attack = p->attack_ms;
        apsis_gravity_point_probe_initial_release = p->release_ms;
        apsis_gravity_point_probe_plain_degree = 0xffu;
        apsis_gravity_point_probe_initial_other_hash = apsis_p10g_other_point_hash();
        apsis_p10g_gravity_point_probe_update(SURF_MOTHER);
    }''', 1)

p.write_text(s)
PY

grep -q 'static volatile uint32_t apsis_gravity_point_probe\[18\]' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_GRPT_VIOL_SELECT_REQUEST' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_GRPT_SEEN_PLAIN_REQUEST' work/apsis/firmware/src/dinghy.c
[[ $(grep -c 'apsis_gravity_point_probe_seen = 0u;' work/apsis/firmware/src/dinghy.c) -eq 1 ]]

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
if 'apsis_p10g_gravity_point_probe_update(' in s[a:b]:
    raise SystemExit('FAIL: P10G active probe leaked into app_audio')
if s.count('apsis_gravity_point_probe_seen = 0u;') != 1:
    raise SystemExit('FAIL: P10G active probe has multiple history reset sites')
print('PASS: P10G active telemetry is test-only, point-isolating and control-side')
PY

echo "PASS: P10G active Gravity-point telemetry layered above production target"
