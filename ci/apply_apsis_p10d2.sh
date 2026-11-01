#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 10D2 layers String Motion strictly above the fully frozen Phase-9 H4b
# target. No historical M4 source is edited: Motion owns a new external store,
# passive Clock observer, control/mailbox bridge and P10-only persistent-pitch
# override. Idle firmware must remain bit-identical at the audio output.
bash ci/apply_apsis_p9h4b.sh

cp apsis/apsis_string_motion.h apsis/apsis_string_motion.c \
   apsis/apsis_string_motion_dsp.h apsis/apsis_string_motion_dsp.c \
   apsis/apsis_string_motion_clock.h apsis/apsis_string_motion_clock.c \
   apsis/apsis_controls_string_motion.h apsis/apsis_controls_string_motion.c \
   apsis/apsis_string_motion_mailbox.h apsis/apsis_string_motion_mailbox.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

# Motion's 2x1200-point bounded store is roughly 19.2 KiB. Keep it in the same
# hardware POOL region as Cathedral but outside Dinghy's ordinary .pool clear.
# The store is made valid by zeroing only its two count fields at boot; stale
# NOLOAD point bytes are unreachable while count==0, avoiding a large startup
# memset that could perturb the already-frozen audio boot timing.
ld = Path('work/apsis/firmware/app.ld')
ls = ld.read_text()
space = '.apsis_space_pool (NOLOAD) : { KEEP(*(.apsis_space_pool*)) . = ALIGN(4); } > POOL'
motion = '.apsis_motion_pool (NOLOAD) : { KEEP(*(.apsis_motion_pool*)) . = ALIGN(4); } > POOL'
if space not in ls:
    raise SystemExit('FAIL: P10D2 P6 Space linker anchor missing')
if motion not in ls:
    ls = ls.replace(space, motion + '\n    ' + space, 1)
ld.write_text(ls)

# P9H3 tags note presses as application activity, but a Motion manual override
# must also end when the final held key is released. Tag note release with the
# same existing sentinel; this adds no idle polling and changes no musical state.
hp = Path('work/apsis/firmware/hal/fm1_input.h')
h = hp.read_text()
old = '''    if (note) {
        if (!on) {
            fm1_in.notes_pressed |= bit;
            fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT;
        }
        fm1_in.notes ^= bit;
    } else {'''
new = '''    if (note) {
        if (!on)
            fm1_in.notes_pressed |= bit;
        fm1_in.notes ^= bit;
        fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT;
    } else {'''
if old not in h:
    raise SystemExit('FAIL: P10D2 P9H3 note activity anchor missing')
h = h.replace(old, new, 1)
hp.write_text(h)

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_bloom_manual_audio.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P10D2 P9H4b header anchor missing')
s = s.replace(h_anchor, h_anchor +
              '\n#include "apsis_string_motion.h"'
              '\n#include "apsis_string_motion_dsp.h"'
              '\n#include "apsis_string_motion_clock.h"'
              '\n#include "apsis_controls_string_motion.h"'
              '\n#include "apsis_string_motion_mailbox.h"', 1)

c_anchor = '#include "apsis_bloom_manual_audio.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P10D2 P9H4b source anchor missing')
s = s.replace(c_anchor, c_anchor +
              '\n#include "apsis_string_motion.c"'
              '\n#include "apsis_string_motion_dsp.c"'
              '\n#include "apsis_string_motion_clock.c"'
              '\n#include "apsis_controls_string_motion.c"'
              '\n#include "apsis_string_motion_mailbox.c"', 1)

state_anchor = 'static ApsisBloomManualAudio apsis_bloom_manual_audio;'
if state_anchor not in s:
    raise SystemExit('FAIL: P10D2 P9H4b state anchor missing')
state = '''static ApsisBloomManualAudio apsis_bloom_manual_audio;
static ApsisStringMotionStore apsis_string_motion_store
    __attribute__((section(".apsis_motion_pool"), aligned(4)));
static ApsisStringMotionRuntime apsis_string_motion_runtime;
static ApsisStringMotionMailbox apsis_string_motion_mailbox;
static ApsisStringMotionPulseBatch apsis_string_motion_pulses;
static ApsisStringMotionPlayback apsis_string_motion_playback_frame;
static uint8_t apsis_string_motion_active;
static uint8_t apsis_string_motion_playback_valid;'''
s = s.replace(state_anchor, state, 1)

# Target-only helpers keep Motion ownership explicit. The store itself is only
# written by app_audio; main publishes compact intents through D1's mailbox.
app_audio_anchor = 'static void app_audio(int32_t *out, uint32_t n)'
if app_audio_anchor not in s:
    raise SystemExit('FAIL: P10D2 app_audio anchor missing')
helpers = r'''static void apsis_p10_motion_recompute_active(void)
{
    apsis_string_motion_active =
        (uint8_t)(apsis_string_motion_runtime.armed ||
                  apsis_string_motion_runtime.recording ||
                  apsis.string.motion_loop_play);
    if (!apsis_string_motion_active)
        apsis_string_motion_playback_valid = 0u;
}

static void apsis_p10_motion_take_control(void)
{
    uint32_t packed;
    ApsisStringMotionMailboxFrame frame;

    if (!apsis_string_motion_mailbox.pending) return;
    packed = apsis_string_motion_mailbox_take(&apsis_string_motion_mailbox);
    if (!packed) return;
    apsis_string_motion_mailbox_decode(packed, &frame);

    if (frame.target_valid)
        apsis_string_motion_runtime.record_target = frame.target;

    if (frame.action == APSIS_STRING_MOTION_ACTION_ARM_TOGGLE) {
        (void)apsis_string_motion_arm(&apsis_string_motion_runtime, &apsis,
                                      apsis_string_motion_runtime.record_target);
    } else if (frame.action == APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE) {
        (void)apsis_string_motion_toggle_play(&apsis_string_motion_store,
                                              &apsis_string_motion_runtime,
                                              &apsis);
    } else if (frame.action == APSIS_STRING_MOTION_ACTION_CLEAR) {
        /* Metadata-only clear. The 19 KiB NOLOAD body is unreachable once both
         * counts are zero, so never memset it from the audio ISR. */
        apsis_string_motion_store.left.count = 0u;
        apsis_string_motion_store.right.count = 0u;
        apsis_string_motion_reset(0, &apsis_string_motion_runtime);
        apsis.string.motion_loop_play = 0u;
    }

    if (frame.manual_valid)
        apsis_string_motion_manual_override(&apsis_string_motion_runtime,
                                            frame.manual_active);
    if (frame.capture && apsis_string_motion_runtime.recording)
        (void)apsis_string_motion_capture(&apsis_string_motion_store,
                                          &apsis_string_motion_runtime,
                                          &apsis, apsis_runtime.cursor_sample, 0u);
    apsis_p10_motion_recompute_active();
}

static void apsis_p10_motion_prepare_block(uint32_t n)
{
    apsis_p10_motion_take_control();
    if (!apsis_string_motion_active) return;

    apsis_string_motion_collect_pulses(&apsis_runtime, &apsis, n,
                                       &apsis_string_motion_pulses);
    apsis_string_motion_playback_valid = apsis_string_motion_playback(
        &apsis_string_motion_store, &apsis_string_motion_runtime, &apsis,
        apsis_runtime.cursor_sample, &apsis_string_motion_playback_frame);
    if (apsis_string_motion_playback_valid) {
        /* Playback owns the visible physical String coordinates, while P10B
         * separately restores the frozen absolute pitch after M4a update. */
        apsis.string.left_split_q15 = apsis_string_motion_playback_frame.left_split_q15;
        apsis.string.right_split_q15 = apsis_string_motion_playback_frame.right_split_q15;
        apsis.string.split_q15 = apsis.string.right_split_q15;
    }
}

static void apsis_p10_motion_finish_block(void)
{
    uint8_t i;
    if (!apsis_string_motion_active) return;

    /* Pulses were observed before M3c consumed the current callback. Apply them
     * only after this block has sounded: native Motion may be <=1 block late,
     * but is never early relative to the shared Clock. */
    for (i = 0u; i < apsis_string_motion_pulses.count; ++i) {
        const ApsisStringMotionPulse *pulse = &apsis_string_motion_pulses.pulse[i];
        apsis_string_motion_pulse(&apsis_string_motion_store,
                                  &apsis_string_motion_runtime, &apsis,
                                  pulse->step, pulse->sample,
                                  pulse->interval_samples);
    }
    if (apsis_string_motion_runtime.recording)
        (void)apsis_string_motion_capture(&apsis_string_motion_store,
                                          &apsis_string_motion_runtime,
                                          &apsis, apsis_runtime.cursor_sample, 0u);
    apsis_string_motion_restore_owned(&apsis_string_motion_runtime, &apsis);
    apsis_p10_motion_recompute_active();
}

'''
s = s.replace(app_audio_anchor, helpers + app_audio_anchor, 1)

# Audio block start: consume only if Motion is active or a main-loop message is
# pending. Default firmware therefore pays only two tiny branch tests. Playback
# split is visible to M4a, then P10B replaces only persistent primary pitch.
update = '''        apsis_string_dsp_update(&string_dsp, &apsis, &string_pitch_memory,
                                apsis_runtime.transport.running);'''
if update not in s:
    raise SystemExit('FAIL: P10D2 frozen M4a update anchor missing')
new_update = '''        if (apsis_string_motion_active || apsis_string_motion_mailbox.pending)
            apsis_p10_motion_prepare_block(n);
        apsis_string_dsp_update(&string_dsp, &apsis, &string_pitch_memory,
                                apsis_runtime.transport.running);
        if (apsis_string_motion_playback_valid)
            apsis_string_motion_dsp_override(&string_dsp, &apsis,
                                             &apsis_string_motion_playback_frame);'''
s = s.replace(update, new_update, 1)

# Finish after all current-block runtime/audio consumers and shared Cathedral,
# immediately before the established telemetry probe. This is deliberately
# outside M3c: Motion observes shared Clock events but creates none.
probe = '        apsis_runtime_probe[0] = 0x41505349u; /* "APSI" */'
if probe not in s:
    raise SystemExit('FAIL: P10D2 final callback probe anchor missing')
s = s.replace(probe,
              '        if (apsis_string_motion_active)\n'
              '            apsis_p10_motion_finish_block();\n' + probe, 1)

# Main-loop binding. Decode C1 before ordinary P9 STRING routing so the REC+OCT-
# clear chord can suppress String-bank movement, then let P9 remain sole owner
# of actual split mutation. Publication happens after that mutation.
decl = '''    ApsisFm1InputFrame input;
    ApsisControlsDispatchResult result;
    int32_t last_abs_step;'''
if decl not in s:
    raise SystemExit('FAIL: P10D2 P9H3 dispatch declaration anchor missing')
s = s.replace(decl, '''    ApsisFm1InputFrame input;
    ApsisControlsDispatchResult result;
    ApsisStringMotionControlResult motion_control;
    uint8_t motion_control_valid = 0u;
    uint8_t motion_capture = 0u;
    uint8_t motion_manual_valid = 0u;
    uint8_t motion_manual_active = 0u;
    int32_t last_abs_step;''', 1)

take = '    apsis_p9h3_take_input(packed, &input);\n'
if take not in s:
    raise SystemExit('FAIL: P10D2 P9H3 input-take anchor missing')
s = s.replace(take, take + '''    if (current == SURF_STRING) {
        motion_control_valid = apsis_controls_string_motion(
            &apsis, apsis_string_motion_runtime.record_target,
            &input, &motion_control);
        if (motion_control.suppress_string_bank)
            input.buttons_pressed &= ~(1u << APSIS_FM1_BTN_OCT_DOWN);
        motion_capture = (uint8_t)(input.notes_pressed != 0u ||
            input.encoder_steps[APSIS_FM1_ENC_SELECT] != 0);
        /* Every real STRING input frame publishes held-note state. P10D2 tags
         * note release as activity, so manual override ends on the last key-up. */
        motion_manual_valid = 1u;
        motion_manual_active = input.notes_held ? 1u : 0u;
    }
''', 1)

dispatch = '''    apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                            &chain_sequence, &mother_pending, current, &input,
                            last_abs_step, &result);
'''
if dispatch not in s:
    raise SystemExit('FAIL: P10D2 P9H3 dispatcher call anchor missing')
s = s.replace(dispatch, dispatch + '''
    if (current == SURF_STRING &&
        (motion_control_valid || motion_capture || motion_manual_valid)) {
        /* D1 producer is a one-word RMW shared with app_audio. Keep the same
         * short critical-section discipline used by P9 performance mailboxes. */
        fm1_irq_off();
        apsis_string_motion_mailbox_publish(
            &apsis_string_motion_mailbox,
            motion_control_valid ? &motion_control : 0,
            motion_capture, motion_manual_valid, motion_manual_active);
        fm1_irq_on();
    }
''', 1)

# Boot initialization: no full-store clear. Count==0 makes stale NOLOAD bytes
# unreachable, while runtime/mailbox state is initialized normally.
init = '    apsis_bloom_manual_audio_reset(&apsis_bloom_manual_audio);'
if init not in s:
    raise SystemExit('FAIL: P10D2 P9H4b init anchor missing')
s = s.replace(init, init + '''
    apsis_string_motion_store.left.count = 0u;
    apsis_string_motion_store.right.count = 0u;
    apsis_string_motion_reset(0, &apsis_string_motion_runtime);
    apsis_string_motion_mailbox_reset(&apsis_string_motion_mailbox);
    apsis_string_motion_active = 0u;
    apsis_string_motion_playback_valid = 0u;''', 1)

p.write_text(s)
PY

# Generated-target invariants.
grep -q '#include "apsis_string_motion.c"' work/apsis/firmware/src/dinghy.c
grep -q '#include "apsis_string_motion_dsp.c"' work/apsis/firmware/src/dinghy.c
grep -q '#include "apsis_string_motion_clock.c"' work/apsis/firmware/src/dinghy.c
grep -q '#include "apsis_controls_string_motion.c"' work/apsis/firmware/src/dinghy.c
grep -q '#include "apsis_string_motion_mailbox.c"' work/apsis/firmware/src/dinghy.c
grep -q 'section(".apsis_motion_pool")' work/apsis/firmware/src/dinghy.c
grep -q '.apsis_motion_pool (NOLOAD)' work/apsis/firmware/app.ld
grep -q 'apsis_string_motion_collect_pulses(&apsis_runtime' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_motion_dsp_override(&string_dsp' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_motion_mailbox_publish(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_motion_mailbox_take(' work/apsis/firmware/src/dinghy.c
grep -q 'fm1_in.pressed |= APSIS_P9_ACTIVITY_BIT;' work/apsis/firmware/hal/fm1_input.h

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
main = s[s.index('static uint8_t apsis_p9h3_dispatch('):b]
for forbidden in ('apsis_string_motion_mailbox_publish(',
                  'apsis_controls_string_motion('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P10D2 main producer leaked into app_audio: {forbidden}')
for forbidden in ('apsis_string_motion_pulse(',
                  'apsis_string_motion_dsp_override(',
                  'apsis_string_motion_mailbox_take('):
    if forbidden in main:
        raise SystemExit(f'FAIL: P10D2 audio/store ownership leaked into main loop: {forbidden}')
for required in ('project_store.save_pending && !apsis_runtime.transport.running',
                 'apsis_project_fm1_service_save('):
    if required not in s:
        raise SystemExit(f'FAIL: P10D2 disturbed P7 deferred save path: {required}')
if 'static ApsisRuntime apsis_runtime;' not in s:
    raise SystemExit('FAIL: P10D2 disturbed shared runtime declaration')
print('PASS: P10D2 Motion is POOL-backed, mailboxed, Clock-observing and audio-owned')
PY

echo "PASS: P10D2 String Motion target layered above frozen Phase 9 H4b"
