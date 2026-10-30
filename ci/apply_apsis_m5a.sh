#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# M4g is the frozen Phase-4 target authority. The structural-audio bypass must
# reproduce that exact generated overlay rather than approximating it.
bash ci/apply_apsis_m4g.sh

if [[ "${APSIS_CI_STRUCTURAL_AUDIO_OFF:-0}" == "1" ]]; then
  echo "PASS: M5a structural-audio bypass uses exact frozen M4g overlay"
  exit 0
fi

VERSION=${APSIS_VERSION:-0.1-dev}

cp apsis/apsis_persistent_audio.h apsis/apsis_persistent_audio.c \
   apsis/apsis_runtime_m5a.h apsis/apsis_runtime_m5a.c \
   apsis/apsis_runtime_m5a_probe.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

# M5a is a target-boundary layer above frozen M4g. runtime_m5a.c includes
# runtime_m4g.c internally; the separate probe is deliberately cheap and does
# not insert, consume, or reschedule shared events.
h_anchor = '#include "apsis_runtime_m4g.h"'
if '#include "apsis_runtime_m5a.h"' not in s:
    if h_anchor not in s:
        raise SystemExit('FAIL: M5a runtime header anchor missing')
    s = s.replace(
        h_anchor,
        h_anchor + '\n#include "apsis_persistent_audio.h"'
                 '\n#include "apsis_runtime_m5a.h"',
        1)

c_anchor = '#include "apsis_bloom_dsp.c"'
if '#include "apsis_persistent_audio.c"' not in s:
    if c_anchor not in s:
        raise SystemExit('FAIL: M5a persistent source anchor missing')
    s = s.replace(c_anchor,
                  c_anchor + '\n#include "apsis_persistent_audio.c"', 1)

runtime_old = '#include "apsis_runtime_m4g.c"'
if runtime_old not in s:
    raise SystemExit('FAIL: M5a runtime include anchor missing')
s = s.replace(runtime_old,
              '#include "apsis_runtime_m5a_probe.c"\n#include "apsis_runtime_m5a.c"',
              1)

# Keep Phase-5 state external to the frozen ApsisRuntime and append it after the
# already-proven M4g external state so existing objects retain their layout.
state_anchor = 'static uint8_t bloom_audio_started;'
if state_anchor not in s:
    raise SystemExit('FAIL: M5a target state anchor missing')
s = s.replace(
    state_anchor,
    state_anchor +
    '\nstatic ApsisPersistentAudioQueue persistent_audio_queue;'
    '\nstatic ApsisStructuralAudioStats structural_audio_stats;',
    1)

# While a future structural target is pending, current shared state may already
# have changed at target-18/24/26 ms. Do not leak that early state through the
# old block-start String/Drift update. The persistent queue owns those two dry
# sources until the true target sample has been applied.
string_update = '''        apsis_string_dsp_update(&string_dsp, &apsis, &string_pitch_memory,
                                apsis_runtime.transport.running);'''
if string_update not in s:
    raise SystemExit('FAIL: M5a String update anchor missing')
s = s.replace(
    string_update,
    '''        if (!persistent_audio_queue.count)
            apsis_string_dsp_update(&string_dsp, &apsis, &string_pitch_memory,
                                    apsis_runtime.transport.running);''',
    1)

drift_update = '''        apsis_drift_dsp_update(&drift_dsp, &apsis,
                               apsis_runtime.transport.running);'''
if drift_update not in s:
    raise SystemExit('FAIL: M5a Drift update anchor missing')
s = s.replace(
    drift_update,
    '''        if (!persistent_audio_queue.count)
            apsis_drift_dsp_update(&drift_dsp, &apsis,
                                   apsis_runtime.transport.running);''',
    1)

persistent_render = '''        apsis_string_dsp_render(&string_dsp, out, n);
        apsis_drift_dsp_render(&drift_dsp, out, n);'''
if persistent_render not in s:
    raise SystemExit('FAIL: M5a persistent render anchor missing')
s = s.replace(
    persistent_render,
    '''        if (persistent_audio_queue.count)
            apsis_persistent_audio_render_block(&persistent_audio_queue,
                                                &string_dsp, &drift_dsp,
                                                out, n,
                                                apsis_runtime.cursor_sample);
        else {
            apsis_string_dsp_render(&string_dsp, out, n);
            apsis_drift_dsp_render(&drift_dsp, out, n);
        }''',
    1)

# One local flag decides only which already-proven coordinator entry point owns
# this callback. Existing M4g render/trigger ordering remains unchanged.
decl_anchor = '''        ApsisRuntimeStringRhythmObservation rhythm_observation;
        uint8_t attr_ix;'''
if decl_anchor not in s:
    raise SystemExit('FAIL: M5a callback declaration anchor missing')
s = s.replace(
    decl_anchor,
    decl_anchor + '\n        uint8_t structural_capture_active;',
    1)

# M4g computes its Spiral/Bloom activation latches before choosing a runtime
# path. Insert the structural probe immediately after those latches. Active
# pending state covers startup/lookahead cases where the early observation has
# not yet been inserted into the heap when the callback begins.
activation_anchor = '''        if (!bloom_audio_started &&
            apsis_runtime_bloom_occurrence_due(
                &apsis_runtime, apsis_runtime.cursor_sample + n))
            bloom_audio_started = 1u;'''
if activation_anchor not in s:
    raise SystemExit('FAIL: M5a activation anchor missing')
s = s.replace(
    activation_anchor,
    activation_anchor + '''
        structural_capture_active = (uint8_t)(
            apsis_runtime.gravity_pending.active ||
            apsis_mother_pending_count(&mother_pending) ||
            chain_sequence.playing ||
            apsis_runtime_structural_audio_due(
                &apsis_runtime, &mother_pending, &chain_sequence,
                apsis_runtime.cursor_sample + n));''',
    1)

# Replace only the process call in each M4g branch. The surrounding finite
# Spiral/Bloom binding and rendering remains byte-for-byte the proven M4g code.
bloom_call = '''            apsis_runtime_process_bloom_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, 0, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                spiral_audio_started ? &spiral_audio_runtime : 0,
                &bloom_audio_runtime, &rhythm_observation);'''
bloom_repl = '''            if (structural_capture_active)
                apsis_runtime_process_structural_audio(
                    &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                    &mother_pending, &bloom_voicing_state, n, 0, 0,
                    (string_attractor_runtime.length ||
                     apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                        ? &string_attractor_runtime : 0,
                    spiral_audio_started ? &spiral_audio_runtime : 0,
                    &bloom_audio_runtime, &rhythm_observation,
                    &persistent_audio_queue, &string_pitch_memory,
                    &structural_audio_stats);
            else
                apsis_runtime_process_bloom_audio(
                    &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                    &mother_pending, &bloom_voicing_state, n, 0, 0,
                    (string_attractor_runtime.length ||
                     apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                        ? &string_attractor_runtime : 0,
                    spiral_audio_started ? &spiral_audio_runtime : 0,
                    &bloom_audio_runtime, &rhythm_observation);'''
if bloom_call not in s:
    raise SystemExit('FAIL: M5a Bloom runtime call anchor missing')
s = s.replace(bloom_call, bloom_repl, 1)

spiral_call = '''            apsis_runtime_process_spiral_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, 0, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                &spiral_audio_runtime, &rhythm_observation);'''
spiral_repl = '''            if (structural_capture_active)
                apsis_runtime_process_structural_audio(
                    &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                    &mother_pending, &bloom_voicing_state, n, 0, 0,
                    (string_attractor_runtime.length ||
                     apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                        ? &string_attractor_runtime : 0,
                    &spiral_audio_runtime, 0, &rhythm_observation,
                    &persistent_audio_queue, &string_pitch_memory,
                    &structural_audio_stats);
            else
                apsis_runtime_process_spiral_audio(
                    &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                    &mother_pending, &bloom_voicing_state, n, 0, 0,
                    (string_attractor_runtime.length ||
                     apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                        ? &string_attractor_runtime : 0,
                    &spiral_audio_runtime, &rhythm_observation);'''
if spiral_call not in s:
    raise SystemExit('FAIL: M5a Spiral runtime call anchor missing')
s = s.replace(spiral_call, spiral_repl, 1)

attractor_call = '''            apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                     &keep_bank, &chain_sequence,
                                                     &mother_pending,
                                                     &bloom_voicing_state, n, 0, 0,
                                                     &string_attractor_runtime,
                                                     &rhythm_observation);'''
attractor_repl = '''            if (structural_capture_active)
                apsis_runtime_process_structural_audio(
                    &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                    &mother_pending, &bloom_voicing_state, n, 0, 0,
                    &string_attractor_runtime, 0, 0, &rhythm_observation,
                    &persistent_audio_queue, &string_pitch_memory,
                    &structural_audio_stats);
            else
                apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                         &keep_bank, &chain_sequence,
                                                         &mother_pending,
                                                         &bloom_voicing_state, n, 0, 0,
                                                         &string_attractor_runtime,
                                                         &rhythm_observation);'''
if attractor_call not in s:
    raise SystemExit('FAIL: M5a Attractor runtime call anchor missing')
s = s.replace(attractor_call, attractor_repl, 1)

base_call = '''            apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                           &chain_sequence, &mother_pending,
                                           &bloom_voicing_state, n, 0, 0,
                                           &rhythm_observation);'''
base_repl = '''            if (structural_capture_active)
                apsis_runtime_process_structural_audio(
                    &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                    &mother_pending, &bloom_voicing_state, n, 0, 0,
                    0, 0, 0, &rhythm_observation,
                    &persistent_audio_queue, &string_pitch_memory,
                    &structural_audio_stats);
            else
                apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                               &chain_sequence, &mother_pending,
                                               &bloom_voicing_state, n, 0, 0,
                                               &rhythm_observation);'''
if base_call not in s:
    raise SystemExit('FAIL: M5a base runtime call anchor missing')
s = s.replace(base_call, base_repl, 1)

p.write_text(s)
PY

# Gate 3 compiles the actual M5a single-TU result. The bypass exits above before
# this second host build and remains exact M4g.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

echo "PASS: M5a structural-audio overlay layered above frozen M4g target"
