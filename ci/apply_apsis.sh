#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
[[ -d work/dinghy ]] || { echo "run ci/prepare_dinghy.sh first" >&2; exit 2; }

VERSION=${APSIS_VERSION:-0.1-dev}
rm -rf work/apsis
cp -a work/dinghy work/apsis
rm -rf work/apsis/.git work/apsis/build

cp overlay/dinghy.c work/apsis/firmware/src/dinghy.c
cp apsis/apsis_state.h apsis/apsis_state.c \
   apsis/apsis_harmony.h apsis/apsis_harmony.c \
   apsis/apsis_gravity.h apsis/apsis_gravity.c \
   apsis/apsis_string.h apsis/apsis_string.c \
   apsis/apsis_string_dsp.h apsis/apsis_string_dsp.c \
   apsis/apsis_string_transient.c apsis/apsis_string_transient_pool.c \
   apsis/apsis_string_rhythm_audio.h apsis/apsis_string_rhythm_audio.c \
   apsis/apsis_string_attractor.h apsis/apsis_string_attractor.c \
   apsis/apsis_string_attractor_audio.h apsis/apsis_string_attractor_audio.c \
   apsis/apsis_drift.h apsis/apsis_drift.c \
   apsis/apsis_drift_dsp.h apsis/apsis_drift_dsp.c \
   apsis/apsis_clock.h apsis/apsis_clock.c \
   apsis/apsis_clock_dsp.h apsis/apsis_clock_dsp.c \
   apsis/apsis_target_div64.c \
   apsis/apsis_spiral.h apsis/apsis_spiral.c \
   apsis/apsis_spiral_audio.h apsis/apsis_spiral_audio.c \
   apsis/apsis_spiral_audio_runtime.h apsis/apsis_spiral_audio_runtime.c \
   apsis/apsis_spiral_dsp.h apsis/apsis_spiral_dsp.c \
   apsis/apsis_bloom.h apsis/apsis_bloom.c \
   apsis/apsis_keep.h apsis/apsis_keep.c \
   apsis/apsis_chain.h apsis/apsis_chain.c \
   apsis/apsis_mother.h apsis/apsis_mother.c \
   apsis/apsis_transport.h apsis/apsis_transport.c \
   apsis/apsis_late_binding.h apsis/apsis_late_binding.c \
   apsis/apsis_runtime.h apsis/apsis_runtime.c apsis/apsis_runtime_m3c.c \
   apsis/apsis_runtime_m4c.c apsis/apsis_runtime_m4f.h apsis/apsis_runtime_m4f.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path
import os
p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()
drift_dsp_enabled = os.environ.get('APSIS_CI_DRIFT_DSP_OFF') != '1'
clock_dsp_enabled = os.environ.get('APSIS_CI_CLOCK_DSP_OFF') != '1'
spiral_dsp_enabled = os.environ.get('APSIS_CI_SPIRAL_DSP_OFF') != '1'

# Older checked-in overlays did not yet include M4a. Keep the generator
# backwards-safe, but never duplicate a .c include once the live M4a overlay
# owns it directly.
h_anchor = '#include "apsis_string.h"'
c_anchor = '#include "apsis_string.c"'
if '#include "apsis_string_dsp.h"' not in s:
    if h_anchor not in s:
        raise SystemExit('FAIL: String header include anchor missing')
    s = s.replace(h_anchor, h_anchor + '\n#include "apsis_string_dsp.h"', 1)
if '#include "apsis_string_dsp.c"' not in s:
    if c_anchor not in s:
        raise SystemExit('FAIL: String source include anchor missing')
    s = s.replace(c_anchor, c_anchor + '\n#include "apsis_string_dsp.c"', 1)

# M4b1 compiles the finite transient engine against a separate pool object.
transient_anchor = '#include "apsis_string_dsp.c"'
if '#include "apsis_string_transient_pool.c"' not in s:
    if transient_anchor not in s:
        raise SystemExit('FAIL: M4b transient include anchor missing')
    s = s.replace(transient_anchor,
                  transient_anchor + '\n#include "apsis_string_transient_pool.c"', 1)

# M4b2 binds already-consumed shared Rhythm events to the finite pool.
if '#include "apsis_string_rhythm_audio.h"' not in s:
    h2_anchor = '#include "apsis_string_dsp.h"'
    if h2_anchor not in s:
        raise SystemExit('FAIL: M4b Rhythm header anchor missing')
    s = s.replace(h2_anchor, h2_anchor + '\n#include "apsis_string_rhythm_audio.h"', 1)
if '#include "apsis_string_rhythm_audio.c"' not in s:
    c2_anchor = '#include "apsis_string_transient_pool.c"'
    if c2_anchor not in s:
        raise SystemExit('FAIL: M4b Rhythm source anchor missing')
    s = s.replace(c2_anchor, c2_anchor + '\n#include "apsis_string_rhythm_audio.c"', 1)

# M4c adds external String Attractor planner/bind state and a late-bind audio
# consumer. It still uses the same global finite transient pool.
if '#include "apsis_string_attractor.h"' not in s:
    ha_anchor = '#include "apsis_string_rhythm_audio.h"'
    if ha_anchor not in s:
        raise SystemExit('FAIL: M4c Attractor header anchor missing')
    s = s.replace(ha_anchor, ha_anchor + '\n#include "apsis_string_attractor.h"\n#include "apsis_string_attractor_audio.h"', 1)
if '#include "apsis_string_attractor.c"' not in s:
    ca_anchor = '#include "apsis_string_rhythm_audio.c"'
    if ca_anchor not in s:
        raise SystemExit('FAIL: M4c Attractor source anchor missing')
    s = s.replace(ca_anchor,
                  ca_anchor + '\n#include "apsis_string_attractor.c"\n#include "apsis_string_attractor_audio.c"', 1)

# M4d restores the identity-bearing persistent Drift pair as a separate dry
# source. A CI bypass can deliberately omit only this DSP layer.
if drift_dsp_enabled:
    if '#include "apsis_drift_dsp.h"' not in s:
        hd_anchor = '#include "apsis_drift.h"'
        if hd_anchor not in s:
            raise SystemExit('FAIL: M4d Drift header anchor missing')
        s = s.replace(hd_anchor, hd_anchor + '\n#include "apsis_drift_dsp.h"', 1)
    if '#include "apsis_drift_dsp.c"' not in s:
        cd_anchor = '#include "apsis_drift.c"'
        if cd_anchor not in s:
            raise SystemExit('FAIL: M4d Drift source anchor missing')
        s = s.replace(cd_anchor, cd_anchor + '\n#include "apsis_drift_dsp.c"', 1)

# M4e makes the already-frozen shared Clock events audible. The Clock DSP is a
# passive finite consumer: no scheduler, queue, or musical RNG lives here.
# JieLi's pi32v2 target omits libgcc DI-mode division. Clock owns the shim when
# it is enabled; M4f inserts the same one before Spiral only when Clock is off.
if clock_dsp_enabled:
    if '#include "apsis_clock_dsp.h"' not in s:
        hc_anchor = '#include "apsis_clock.h"'
        if hc_anchor not in s:
            raise SystemExit('FAIL: M4e Clock header anchor missing')
        s = s.replace(hc_anchor, hc_anchor + '\n#include "apsis_clock_dsp.h"', 1)
    if '#include "apsis_clock_dsp.c"' not in s:
        cc_anchor = '#include "apsis_clock.c"'
        if cc_anchor not in s:
            raise SystemExit('FAIL: M4e Clock source anchor missing')
        s = s.replace(cc_anchor,
                      cc_anchor + '\n#include "apsis_target_div64.c"\n#include "apsis_clock_dsp.c"', 1)

# M4f consumes the already-frozen M3c Spiral slot/bind lifecycle. The acoustic
# snapshot/runtime and finite renderer are external to ApsisRuntime. No new
# scheduler event is created; +8 ms shadow timing remains DSP-local.
if spiral_dsp_enabled:
    if '#include "apsis_spiral_audio.h"' not in s:
        hs_anchor = '#include "apsis_spiral.h"'
        if hs_anchor not in s:
            raise SystemExit('FAIL: M4f Spiral header anchor missing')
        s = s.replace(hs_anchor,
                      hs_anchor + '\n#include "apsis_spiral_audio.h"\n#include "apsis_spiral_audio_runtime.h"\n#include "apsis_spiral_dsp.h"\n#include "apsis_runtime_m4f.h"', 1)
    if '#include "apsis_spiral_audio.c"' not in s:
        cs_anchor = '#include "apsis_spiral.c"'
        if cs_anchor not in s:
            raise SystemExit('FAIL: M4f Spiral source anchor missing')
        div64 = '' if clock_dsp_enabled else '\n#include "apsis_target_div64.c"'
        s = s.replace(cs_anchor,
                      cs_anchor + div64 +
                      '\n#include "apsis_spiral_audio.c"'
                      '\n#include "apsis_spiral_audio_runtime.c"'
                      '\n#include "apsis_spiral_dsp.c"', 1)

# M4f includes the proven M4c coordinator internally. With the Spiral DSP CI
# bypass, keep the exact M4e runtime include path so Gate 5b can prove isolation.
old = '#include "apsis_runtime.c"'
new = ('#include "apsis_late_binding.c"\n#include "apsis_runtime_m4f.c"'
       if spiral_dsp_enabled else
       '#include "apsis_late_binding.c"\n#include "apsis_runtime_m4c.c"')
if old not in s:
    raise SystemExit('FAIL: runtime include anchor missing from Dinghy overlay')
s = s.replace(old, new, 1)
s = s.replace('M3b instantiates the shared transport/runtime coordinator',
              'M4f retains the shared coordinator and adds dry Drift/Clock/Spiral audio', 1)
s = s.replace('static volatile uint32_t apsis_runtime_probe[4];',
              'static volatile uint32_t apsis_runtime_probe[6];', 1)

# Keep persistent dry-engine state separate from frozen String/runtime layouts.
state_anchor = 'static ApsisStringDsp string_dsp;'
if state_anchor not in s:
    raise SystemExit('FAIL: M4 dry-state declaration anchor missing')
state_extra = state_anchor
if drift_dsp_enabled:
    state_extra += '\nstatic ApsisDriftDsp drift_dsp;'
if clock_dsp_enabled:
    state_extra += '\nstatic ApsisClockDsp clock_dsp;'
if spiral_dsp_enabled:
    state_extra += '\nstatic ApsisSpiralDsp spiral_dsp;'
s = s.replace(state_anchor, state_extra, 1)

# Keep all M4 finite state outside the frozen ApsisRuntime layout. String owns
# its 80+32 transient pool; Spiral owns an external snapshot/bind runtime and
# the separately-frozen 32-leg dry pool. The one-byte latch keeps the exact M4e
# callback path until Spiral can actually produce audio; once set it remains set
# so pending binds and finite tails survive later route/mute edits.
pool_anchor = 'static volatile uint32_t apsis_runtime_probe[6];'
pool_decl = (pool_anchor +
             '\nstatic ApsisStringTransientPool string_transient_pool;'
             '\nstatic ApsisStringAttractorRuntime string_attractor_runtime;')
if spiral_dsp_enabled:
    pool_decl += ('\nstatic ApsisSpiralAudioRuntime spiral_audio_runtime;'
                  '\nstatic uint8_t spiral_audio_started;')
if pool_anchor not in s:
    raise SystemExit('FAIL: M4 finite-state declaration anchor missing')
s = s.replace(pool_anchor, pool_decl, 1)

# Shared callback handoff. Clock observes due shared events before Phase-3
# consumes them. Spiral uses the existing target-80 ms M3c bind. While the
# source-default route is empty, use the exact frozen M4e runtime path and do no
# Spiral renderer work; this preserves the proven idle callback budget.
audio_old = '''        apsis_string_dsp_render(&string_dsp, out, n);
        apsis_runtime_process(&apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                              &mother_pending, &bloom_voicing_state, n, 0, 0);'''
drift_lines = '''        apsis_drift_dsp_update(&drift_dsp, &apsis,
                               apsis_runtime.transport.running);
''' if drift_dsp_enabled else ''
drift_render = '        apsis_drift_dsp_render(&drift_dsp, out, n);\n' if drift_dsp_enabled else ''
clock_decl = '''        ApsisClockEventBatch clock_batch;
        uint8_t clock_ix;
        uint64_t clock_block_start = apsis_runtime.cursor_sample;
''' if clock_dsp_enabled else ''
clock_collect = '''        apsis_clock_collect_block(&apsis_runtime, &apsis, n, &clock_batch);
        for (clock_ix = 0u; clock_ix < clock_batch.count; ++clock_ix)
            (void)apsis_clock_dsp_trigger_event(&clock_dsp, &apsis,
                                                &clock_batch.event[clock_ix],
                                                clock_block_start);
''' if clock_dsp_enabled else ''
clock_render = '        apsis_clock_dsp_render(&clock_dsp, out, n);\n' if clock_dsp_enabled else ''
spiral_decl = '''        uint8_t spiral_ix;
        uint64_t spiral_block_start;
''' if spiral_dsp_enabled else ''
if spiral_dsp_enabled:
    runtime_process = '''        if (!spiral_audio_started && apsis.spiral.sound && apsis.spiral.route_count)
            spiral_audio_started = 1u;
        if (spiral_audio_started) {
            spiral_block_start = apsis_runtime.cursor_sample;
            apsis_runtime_process_spiral_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, 0, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                &spiral_audio_runtime, &rhythm_observation);
            for (spiral_ix = 0u; spiral_ix < spiral_audio_runtime.binding_count; ++spiral_ix)
                (void)apsis_spiral_dsp_trigger_binding(
                    &spiral_dsp, &apsis, &spiral_audio_runtime.binding[spiral_ix],
                    spiral_block_start);
            if (spiral_dsp.hit_trigger_count)
                apsis_spiral_dsp_render(&spiral_dsp, out, n);
        } else if (string_attractor_runtime.length ||
                   apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR) {
            apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                     &keep_bank, &chain_sequence,
                                                     &mother_pending,
                                                     &bloom_voicing_state, n, 0, 0,
                                                     &string_attractor_runtime,
                                                     &rhythm_observation);
        } else {
            apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                           &chain_sequence, &mother_pending,
                                           &bloom_voicing_state, n, 0, 0,
                                           &rhythm_observation);
        }
'''
else:
    runtime_process = '''        if (string_attractor_runtime.length ||
            apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR) {
            apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                     &keep_bank, &chain_sequence,
                                                     &mother_pending,
                                                     &bloom_voicing_state, n, 0, 0,
                                                     &string_attractor_runtime,
                                                     &rhythm_observation);
        } else {
            apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                           &chain_sequence, &mother_pending,
                                           &bloom_voicing_state, n, 0, 0,
                                           &rhythm_observation);
        }
'''
audio_new = '''        ApsisRuntimeStringRhythmObservation rhythm_observation;
        uint8_t attr_ix;
''' + clock_decl + spiral_decl + drift_lines + clock_collect + '''        apsis_string_dsp_render(&string_dsp, out, n);
''' + drift_render + clock_render + '''        apsis_string_transient_render(&string_transient_pool, out, n);
''' + runtime_process + '''        for (attr_ix = 0u; attr_ix < string_attractor_runtime.binding_count; ++attr_ix)
            (void)apsis_string_attractor_trigger_binding(
                &string_transient_pool, &apsis, &string_pitch_memory,
                &string_attractor_runtime.binding[attr_ix],
                apsis_runtime.cursor_sample);
        if (rhythm_observation.valid)
            (void)apsis_string_rhythm_trigger_event(&string_transient_pool,
                                                    &apsis,
                                                    &string_pitch_memory,
                                                    &rhythm_observation.event);'''
if audio_old not in s:
    raise SystemExit('FAIL: M4f live audio handoff anchor missing')
s = s.replace(audio_old, audio_new, 1)

probe_anchor = '        apsis_runtime_probe[3] = (uint32_t)apsis_runtime.cursor_sample;'
probe_extra = probe_anchor + '\n        apsis_runtime_probe[4] = (uint32_t)apsis_runtime.late_slot_overflowed | ((uint32_t)apsis_runtime.scheduler_starved << 1);\n        apsis_runtime_probe[5] = apsis_runtime.late_trace_hash;'
if probe_anchor not in s:
    raise SystemExit('FAIL: runtime telemetry write anchor missing')
s = s.replace(probe_anchor, probe_extra, 1)
init_anchor = '    apsis_runtime_probe[3] = 0u;'
init_extra = init_anchor + '\n    apsis_runtime_probe[4] = 0u;\n    apsis_runtime_probe[5] = 0u;'
if init_anchor not in s:
    raise SystemExit('FAIL: runtime telemetry init anchor missing')
s = s.replace(init_anchor, init_extra, 1)

# The FM-1 CRT zeroes .bss before audio init/IRQ enable. Spiral external state
# therefore starts clean without spending a pre-runtime audio callback on an
# explicit reset. Reset APIs remain regression-tested for lifecycle use.
start_anchor = '    apsis_runtime_start(&apsis_runtime);'
if start_anchor not in s:
    raise SystemExit('FAIL: runtime start anchor missing')

p.write_text(s)
PY

python3 ci/patch_release_identity.py work/apsis/tools/build.py

( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) 2>&1 | tee build/evidence/apsis_host.log

printf 'PASS: APSIS overlay + host shell%s%s%s\n' \
  "${APSIS_CI_DRIFT_DSP_OFF:+ (Drift DSP bypass)}" \
  "${APSIS_CI_CLOCK_DSP_OFF:+ (Clock DSP bypass)}" \
  "${APSIS_CI_SPIRAL_DSP_OFF:+ (Spiral DSP bypass)}"
