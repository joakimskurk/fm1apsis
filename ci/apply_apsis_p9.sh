#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 9A starts from the exact frozen Phase-8 target. It adds only the global
# FM-1 panel contract: canonical navigation, SPACE chord, transport gesture and
# matching LED feedback. Per-surface parameter editing follows later.
bash ci/apply_apsis_p8.sh

cp apsis/apsis_controls.h apsis/apsis_controls.c work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path
p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_visual_p8c.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P9 P8 visual header anchor missing')
s = s.replace(h_anchor, h_anchor + '\n#include "apsis_controls.h"', 1)

c_anchor = '#include "apsis_visual_p8c.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P9 P8 visual source anchor missing')
s = s.replace(c_anchor, c_anchor + '\n#include "apsis_controls.c"', 1)

# Bind the pure P9 contract to the pinned Dinghy platform numerically. If the
# upstream panel vocabulary ever changes, target generation must fail loudly.
assert_anchor = 'static uint8_t surface = SURF_MOTHER;'
if assert_anchor not in s:
    raise SystemExit('FAIL: P9 surface declaration anchor missing')
asserts = r'''
_Static_assert(BTN_OCT_DOWN == APSIS_FM1_BTN_OCT_DOWN, "P9 BTN_OCT_DOWN id");
_Static_assert(BTN_OCT_UP == APSIS_FM1_BTN_OCT_UP, "P9 BTN_OCT_UP id");
_Static_assert(BTN_FX == APSIS_FM1_BTN_FX, "P9 BTN_FX id");
_Static_assert(BTN_SEL == APSIS_FM1_BTN_SEL, "P9 BTN_SEL id");
_Static_assert(BTN_ENV == APSIS_FM1_BTN_ENV, "P9 BTN_ENV id");
_Static_assert(BTN_LFO == APSIS_FM1_BTN_LFO, "P9 BTN_LFO id");
_Static_assert(BTN_EDIT == APSIS_FM1_BTN_EDIT, "P9 BTN_EDIT id");
_Static_assert(BTN_GLO == APSIS_FM1_BTN_GLO, "P9 BTN_GLO id");
_Static_assert(BTN_HOME == APSIS_FM1_BTN_HOME, "P9 BTN_HOME id");
_Static_assert(BTN_SAVE == APSIS_FM1_BTN_SAVE, "P9 BTN_SAVE id");
_Static_assert(BTN_ARP == APSIS_FM1_BTN_ARP, "P9 BTN_ARP id");
_Static_assert(BTN_SEQ == APSIS_FM1_BTN_SEQ, "P9 BTN_SEQ id");
_Static_assert(BTN_PLAY == APSIS_FM1_BTN_PLAY, "P9 BTN_PLAY id");
_Static_assert(BTN_REC == APSIS_FM1_BTN_REC, "P9 BTN_REC id");
_Static_assert(ENC_SELECT == APSIS_FM1_ENC_SELECT, "P9 ENC_SELECT id");
_Static_assert(ENC_ALGORITHM == APSIS_FM1_ENC_ALGORITHM, "P9 ENC_ALGORITHM id");
_Static_assert(ENC_KNOB1 == APSIS_FM1_ENC_K1, "P9 ENC_K1 id");
_Static_assert(ENC_KNOB2 == APSIS_FM1_ENC_K2, "P9 ENC_K2 id");
_Static_assert(ENC_KNOB3 == APSIS_FM1_ENC_K3, "P9 ENC_K3 id");
_Static_assert(ENC_KNOB4 == APSIS_FM1_ENC_K4, "P9 ENC_K4 id");
_Static_assert(ENC_PRESETS == APSIS_FM1_ENC_PRESETS, "P9 ENC_PRESETS id");
_Static_assert(FM1_NENC == APSIS_FM1_ENCODER_COUNT, "P9 encoder count");
_Static_assert(FM1_NKEY - APSIS_FM1_BUTTON_COUNT == APSIS_FM1_KEY_COUNT, "P9 note-key count");
_Static_assert(SURF_MOTHER == APSIS_SURFACE_MOTHER, "P9 MOTHER surface order");
_Static_assert(SURF_STRING == APSIS_SURFACE_STRING, "P9 STRING surface order");
_Static_assert(SURF_DRIFT == APSIS_SURFACE_DRIFT, "P9 DRIFT surface order");
_Static_assert(SURF_CLOCK == APSIS_SURFACE_CLOCK, "P9 CLOCK surface order");
_Static_assert(SURF_SPIRAL == APSIS_SURFACE_SPIRAL, "P9 SPIRAL surface order");
_Static_assert(SURF_BLOOM == APSIS_SURFACE_BLOOM, "P9 BLOOM surface order");
_Static_assert(SURF_GRAVITY == APSIS_SURFACE_GRAVITY, "P9 GRAVITY surface order");
_Static_assert(SURF_CHAIN == APSIS_SURFACE_CHAIN, "P9 CHAIN surface order");
_Static_assert(SURF_KEEP == APSIS_SURFACE_KEEP, "P9 KEEP surface order");
_Static_assert(SURF_SPACE == APSIS_SURFACE_SPACE, "P9 SPACE surface order");
_Static_assert(SURF_COUNT == APSIS_SURFACE_COUNT, "P9 surface count");
'''
s = s.replace(assert_anchor, assert_anchor + '\n' + asserts, 1)

start = s.find('static uint8_t surface_from_buttons(')
end = s.find('static void app_main(', start)
if start < 0 or end < 0:
    raise SystemExit('FAIL: P9 input-router anchors missing')
# Preserve any functions between surface_from_buttons and app_main only if there
# are any; in the frozen P8 target this function is the final helper before main.
old_router = s[start:end]
router_end = old_router.find('\n}\n')
if router_end < 0:
    raise SystemExit('FAIL: P9 surface_from_buttons body end missing')
router_end += 3
rest = old_router[router_end:]
router = r'''static uint8_t surface_from_buttons(uint32_t pressed, uint32_t held,
                                          uint8_t current)
{
    ApsisFm1InputFrame input = {0};
    input.buttons_pressed = pressed;
    input.buttons_held = held;
    return apsis_controls_surface_from_fm1(&input, current);
}

static void apsis_p9_apply_global(uint32_t pressed, uint32_t released,
                                  uint32_t held)
{
    ApsisFm1InputFrame input = {0};
    input.buttons_pressed = pressed;
    input.buttons_released = released;
    input.buttons_held = held;
    if (apsis_controls_global_action(&input) != APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE)
        return;
    if (apsis_runtime.transport.running)
        apsis_runtime_stop_attractor(&apsis_runtime, &string_attractor_runtime);
    else
        apsis_runtime_start_attractor(&apsis_runtime, &apsis,
                                      &string_attractor_runtime);
}

'''
s = s[:start] + router + rest + s[end:]

# SPACE shares FX's source-family landmark but also lights SEL to show that the
# user is on the secondary shared-room surface. PLAY reflects shared transport.
old_fx_led = '    fm1_led_key(BTN_FX, surface == SURF_BLOOM);'
if old_fx_led not in s:
    raise SystemExit('FAIL: P9 FX LED anchor missing')
s = s.replace(old_fx_led,
              '    fm1_led_key(BTN_FX, surface == SURF_BLOOM || surface == SURF_SPACE);\n'
              '    fm1_led_key(BTN_SEL, surface == SURF_SPACE);', 1)
led_tail = '    fm1_led_key(BTN_SAVE, surface == SURF_KEEP);\n}'
if led_tail not in s:
    raise SystemExit('FAIL: P9 LED tail anchor missing')
s = s.replace(led_tail,
              '    fm1_led_key(BTN_SAVE, surface == SURF_KEEP);\n'
              '    fm1_led_key(BTN_PLAY, apsis_runtime.transport.running);\n}', 1)

# P7 deliberately inserts project-save request/service code around the original
# input block, and P8 deliberately replaces only the draw tail. Do not match or
# rewrite that larger block here. Patch the single P8 navigation call and append
# the P9 global action immediately after it, preserving both earlier phases.
input_anchor = '        next = surface_from_buttons(pressed, surface);'
if s.count(input_anchor) != 1:
    raise SystemExit(f'FAIL: P9 navigation-call anchor count {s.count(input_anchor)} != 1')
s = s.replace(
    input_anchor,
    '        next = surface_from_buttons(pressed, fm1_in.buttons, surface);\n'
    '        apsis_p9_apply_global(pressed, released, fm1_in.buttons);',
    1)
p.write_text(s)
PY

# Generated-target structural guardrails.
grep -q '#include "apsis_controls.c"' work/apsis/firmware/src/dinghy.c
grep -q '_Static_assert(BTN_HOME == APSIS_FM1_BTN_HOME' work/apsis/firmware/src/dinghy.c
grep -q '_Static_assert(ENC_PRESETS == APSIS_FM1_ENC_PRESETS' work/apsis/firmware/src/dinghy.c
grep -q '_Static_assert(SURF_SPACE == APSIS_SURFACE_SPACE' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_surface_from_fm1(&input, current)' work/apsis/firmware/src/dinghy.c
grep -q 'surface_from_buttons(pressed, fm1_in.buttons, surface)' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_global_action(&input)' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_runtime_stop_attractor(&apsis_runtime, &string_attractor_runtime)' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_runtime_start_attractor(&apsis_runtime, &apsis' work/apsis/firmware/src/dinghy.c
grep -q 'surface == SURF_BLOOM || surface == SURF_SPACE' work/apsis/firmware/src/dinghy.c
grep -q 'fm1_led_key(BTN_SEL, surface == SURF_SPACE)' work/apsis/firmware/src/dinghy.c
grep -q 'fm1_led_key(BTN_PLAY, apsis_runtime.transport.running)' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
for forbidden in ('apsis_controls_surface_from_fm1', 'apsis_controls_global_action',
                  'fm1_enc_take(', 'fm1_input_edges('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P9 control work leaked into app_audio: {forbidden}')
# P7 save request and deferred service must survive P9 patching intact.
for required in ('apsis_project_fm1_request_save(&project_store)',
                 'project_store.save_pending && !apsis_runtime.transport.running',
                 'apsis_project_fm1_service_save('):
    if required not in s:
        raise SystemExit(f'FAIL: P9 disturbed P7 project path: {required}')
print('PASS: P9A target binds pinned panel ids, global navigation/transport and LEDs outside app_audio')
PY
