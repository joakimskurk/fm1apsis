#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# P9H3 layers the already host-proven P9B..G dispatcher onto the exact P9A
# target. H3 binds structural/live state edits, Gravity, MOTHER pending commits,
# KEEP/CHAIN and SPACE. Direct String/Bloom transient performance remains H4.
bash ci/apply_apsis_p9.sh

cp apsis/apsis_controls_drift.h apsis/apsis_controls_drift.c \
   apsis/apsis_controls_bloom.h apsis/apsis_controls_bloom.c \
   apsis/apsis_controls_keep_chain.h apsis/apsis_controls_keep_chain.c \
   apsis/apsis_controls_mother.h apsis/apsis_controls_mother.c \
   apsis/apsis_controls_space.h apsis/apsis_controls_space.c \
   apsis/apsis_controls_apply.h apsis/apsis_controls_apply.c \
   apsis/apsis_controls_dispatch.h apsis/apsis_controls_dispatch.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path
import re

# Pinned Dinghy already returns one button edge word every main-loop pass. Keep
# the frozen idle path unchanged: only when an input event is actually emitted,
# tag that same word with an application-private sentinel. Releases are packed
# in bits 16..29, leaving lower 14 bits as the original button presses.
hp = Path('work/apsis/firmware/hal/fm1_input.h')
h = hp.read_text()
anchor = '#define FM1_NENC 7u\n'
if anchor not in h:
    raise SystemExit('FAIL: P9H3 input sentinel anchor missing')
h = h.replace(anchor, anchor +
              '#define APSIS_P9_ACTIVITY_BIT 0x80000000u\n'
              '#define APSIS_P9_RELEASE_SHIFT 16u\n', 1)

old = '''    if (note) {
        if (!on)
            fm1_in.notes_pressed |= bit;
        fm1_in.notes ^= bit;
    } else {
        fm1_in.buttons ^= bit;
        if (!on)
            fm1_in.pressed |= bit;
        else
            fm1_in.released |= bit;
    }'''
new = '''    if (note) {
        if (!on) {
            fm1_in.notes_pressed |= bit;
            fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT;
        }
        fm1_in.notes ^= bit;
    } else {
        fm1_in.buttons ^= bit;
        if (!on) {
            fm1_in.pressed |= bit | APSIS_P9_ACTIVITY_BIT;
        } else {
            fm1_in.released |= bit;
            fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT |
                              (bit << APSIS_P9_RELEASE_SHIFT);
        }
    }'''
if old not in h:
    raise SystemExit('FAIL: P9H3 button/note edge anchor missing')
h = h.replace(old, new, 1)

old = '''            n = n >= 2 ? (n + 2) / 4 : 0;
            fm1_in.enc_steps[e] = (int16_t)(fm1_in.enc_steps[e] + (*sub < 0 ? -n : n));
            *sub = 0;'''
new = '''            n = n >= 2 ? (n + 2) / 4 : 0;
            if (n) {
                fm1_in.enc_steps[e] = (int16_t)(fm1_in.enc_steps[e] +
                                               (*sub < 0 ? -n : n));
                fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT;
            }
            *sub = 0;'''
if old not in h:
    raise SystemExit('FAIL: P9H3 encoder edge anchor missing')
h = h.replace(old, new, 1)
hp.write_text(h)

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_controls.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P9H3 P9A header anchor missing')
headers = '''#include "apsis_controls.h"
#include "apsis_controls_drift.h"
#include "apsis_controls_bloom.h"
#include "apsis_controls_keep_chain.h"
#include "apsis_controls_mother.h"
#include "apsis_controls_space.h"
#include "apsis_controls_apply.h"
#include "apsis_controls_dispatch.h"'''
s = s.replace(h_anchor, headers, 1)

c_anchor = '#include "apsis_controls.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P9H3 P9A source anchor missing')
sources = '''#include "apsis_controls.c"
#include "apsis_controls_drift.c"
#include "apsis_controls_bloom.c"
#include "apsis_controls_keep_chain.c"
#include "apsis_controls_mother.c"
#include "apsis_controls_space.c"
#include "apsis_controls_apply.c"
#include "apsis_controls_dispatch.c"'''
s = s.replace(c_anchor, sources, 1)

state_anchor = 'static uint8_t surface = SURF_MOTHER;'
if state_anchor not in s:
    raise SystemExit('FAIL: P9H3 surface state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisControlsSession apsis_controls_session;', 1)

init_anchor = '    apsis_mother_pending_reset(&mother_pending);'
if init_anchor not in s:
    raise SystemExit('FAIL: P9H3 session init anchor missing')
s = s.replace(init_anchor,
              init_anchor + '\n    apsis_controls_session_reset(&apsis_controls_session);', 1)

# Runtime MOTHER commits must execute the exact same structural-parameter apply
# function used by immediate physical edits. This only changes the callback
# argument; the scheduler/event heap and ApsisRuntime layout remain untouched.
pattern = re.compile(r'(&bloom_voicing_state,\s*n,\s*)0,\s*0')
s, callback_count = pattern.subn(r'\1apsis_controls_apply_pending_item, 0', s)
if callback_count < 1:
    raise SystemExit('FAIL: P9H3 found no runtime MOTHER callback sites')

router_anchor = 'static uint8_t surface_from_buttons(uint32_t b, uint8_t current)'
ri = s.find(router_anchor)
re_ = s.find('static void app_main(void)', ri)
if ri < 0 or re_ < 0:
    raise SystemExit('FAIL: P9H3 router anchors missing')

bridge = r'''static void apsis_p9h3_take_input(uint32_t packed, ApsisFm1InputFrame *input)
{
    uint8_t e;
    uint8_t *z = (uint8_t *)input;
    uint32_t n = (uint32_t)sizeof(*input);
    while (n--) *z++ = 0u;
    input->buttons_pressed = packed & ((1u << APSIS_FM1_BUTTON_COUNT) - 1u);
    input->buttons_released = (packed >> APSIS_P9_RELEASE_SHIFT) &
                              ((1u << APSIS_FM1_BUTTON_COUNT) - 1u);
    input->buttons_held = fm1_in.buttons;
    if (!(packed & APSIS_P9_ACTIVITY_BIT)) return;
    input->notes_pressed = fm1_input_note_edges();
    input->notes_held = fm1_in.notes;
    for (e = 0u; e < APSIS_FM1_ENCODER_COUNT; ++e)
        input->encoder_steps[e] = (int16_t)fm1_enc_take(e);
}

/* H3 consumes all state/runtime results except the two direct transient intents
 * reserved for H4 (String pluck and Bloom core/petals). The shared musical
 * scheduler is never bypassed for Gravity/MOTHER/CHAIN. */
static uint8_t apsis_p9h3_dispatch(uint32_t packed, uint8_t current)
{
    ApsisFm1InputFrame input;
    ApsisControlsDispatchResult result;
    int32_t last_abs_step;

    apsis_p9h3_take_input(packed, &input);
    last_abs_step = apsis_runtime.transport.absolute_pulse
        ? (int32_t)(apsis_runtime.transport.absolute_pulse - 1u) : -1;
    apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                            &chain_sequence, &mother_pending, current, &input,
                            last_abs_step, &result);

    if (result.gravity_request)
        (void)apsis_runtime_request_gravity(
            &apsis_runtime, &apsis, &mother_pending,
            result.gravity_degree, result.gravity_ring,
            apsis_runtime.cursor_sample, 0u);

    if (result.chain_transport == APSIS_CHAIN_TRANSPORT_START)
        apsis_runtime_chain_start(&apsis_runtime, &apsis, &chain_sequence);
    else if (result.chain_transport == APSIS_CHAIN_TRANSPORT_STOP)
        apsis_runtime_chain_stop(&apsis_runtime, &chain_sequence);

    /* Browser removeChainCell() immediately applies the new cell occupying the
     * active position. Dinghy builds APSIS as one translation unit, so H3 can
     * reuse the already-proven private runtime helper without duplicating its
     * RNG/morph law or adding another scheduler path. */
    if (result.chain_apply_current && chain_sequence.count &&
        chain_sequence.pos < chain_sequence.count)
        (void)runtime_apply_chain_cell(&apsis_runtime, &apsis, &keep_bank,
                                       &chain_sequence, chain_sequence.pos,
                                       apsis_runtime.cursor_sample);

    if (result.project_dirty)
        apsis_project_fm1_request_save(&project_store);

    if (result.chain_transport != APSIS_CHAIN_TRANSPORT_NONE)
        return 1u; /* SEL+PLAY belongs to CHAIN, never global transport. */
    return result.suppress_navigation;
}

static uint8_t surface_from_buttons(uint32_t b, uint8_t current)
{
    const uint32_t buttons = b & ((1u << APSIS_FM1_BUTTON_COUNT) - 1u);
    uint8_t block;

    /* Existing P8/P9A branch count is preserved in idle operation. Dispatch is
     * entered only inside a branch that already matched an actual input event. */
    if (buttons & (1u << BTN_HOME)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_MOTHER;
    }
    if (buttons & (1u << BTN_ENV)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_STRING;
    }
    if (buttons & (1u << BTN_LFO)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_DRIFT;
    }
    if (buttons & (1u << BTN_SEQ)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_CLOCK;
    }
    if (buttons & (1u << BTN_ARP)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_SPIRAL;
    }
    if (buttons & (1u << BTN_FX)) {
        block = apsis_p9h3_dispatch(b, current);
        if (block) return current;
        return (fm1_in.buttons & (1u << BTN_SEL)) ? SURF_SPACE : SURF_BLOOM;
    }
    if (buttons & (1u << BTN_GLO)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_GRAVITY;
    }
    if (buttons & (1u << BTN_EDIT)) {
        block = apsis_p9h3_dispatch(b, current);
        return block ? current : SURF_CHAIN;
    }
    if (b & ((1u << BTN_SAVE) | (1u << BTN_PLAY) | APSIS_P9_ACTIVITY_BIT)) {
        block = apsis_p9h3_dispatch(b, current);
        if (block) return current;
        if (buttons & (1u << BTN_SAVE)) return SURF_KEEP;
        if (buttons & (1u << BTN_PLAY)) {
            apsis_p9_apply_transport(buttons);
            return current;
        }
    }
    return current;
}

'''
s = s[:ri] + bridge + s[re_:]
p.write_text(s)
PY

# Generated-target guardrails. The new dispatcher may run only on real input
# activity, remains outside app_audio, preserves P7 deferred writes and keeps H4
# direct transient performance absent for this checkpoint.
grep -q '#include "apsis_controls_dispatch.c"' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_P9_ACTIVITY_BIT 0x80000000u' work/apsis/firmware/hal/fm1_input.h
grep -q 'fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT' work/apsis/firmware/hal/fm1_input.h
grep -q 'apsis_controls_dispatch(&apsis_controls_session' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_runtime_request_gravity(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_apply_pending_item, 0' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_project_fm1_request_save(&project_store)' work/apsis/firmware/src/dinghy.c
grep -q 'runtime_apply_chain_cell(&apsis_runtime' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
for forbidden in ('apsis_controls_dispatch(', 'fm1_enc_take(',
                  'fm1_input_note_edges(', 'apsis_project_fm1_request_save('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P9H3 main-loop control work leaked into app_audio: {forbidden}')
for required in ('project_store.save_pending && !apsis_runtime.transport.running',
                 'apsis_project_fm1_service_save('):
    if required not in s:
        raise SystemExit(f'FAIL: P9H3 disturbed P7 deferred save path: {required}')
# H4 owns direct manual transient emission; H3 may only return the intents.
bridge = s[s.index('static uint8_t apsis_p9h3_dispatch('):s.index('static void app_main(void)')]
for forbidden in ('apsis_string_dsp_trigger_classic_offset(',
                  'apsis_string_dsp_trigger_body(',
                  'apsis_bloom_dsp_trigger_binding('):
    if forbidden in bridge:
        raise SystemExit(f'FAIL: P9H3 stole H4 performance path: {forbidden}')
print(f'PASS: P9H3 dispatcher target bound with {s.count("apsis_controls_apply_pending_item, 0")} runtime MOTHER callback site(s)')
PY

echo "PASS: P9H3 structural physical-control target layered over frozen P9A"
