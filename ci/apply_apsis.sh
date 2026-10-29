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
   apsis/apsis_spiral.h apsis/apsis_spiral.c \
   apsis/apsis_bloom.h apsis/apsis_bloom.c \
   apsis/apsis_keep.h apsis/apsis_keep.c \
   apsis/apsis_chain.h apsis/apsis_chain.c \
   apsis/apsis_mother.h apsis/apsis_mother.c \
   apsis/apsis_transport.h apsis/apsis_transport.c \
   apsis/apsis_late_binding.h apsis/apsis_late_binding.c \
   apsis/apsis_runtime.h apsis/apsis_runtime.c apsis/apsis_runtime_m3c.c \
   apsis/apsis_runtime_m4c.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path
p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

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
# The wrapper includes the already-validated transient implementation while
# preserving ApsisStringDsp's exact M4a layout.
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
# source. It reuses the already-proven fixed pitch/tuning conversion and adds no
# event or scheduler state.
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

# runtime_m4c.c includes the frozen M3c coordinator internally and appends only
# the external Attractor-aware scheduler/process layer. Do not include
# runtime_final.c separately or the lifecycle symbols would be duplicated.
old = '#include "apsis_runtime.c"'
new = '#include "apsis_late_binding.c"\n#include "apsis_runtime_m4c.c"'
if old not in s:
    raise SystemExit('FAIL: runtime include anchor missing from Dinghy overlay')
s = s.replace(old, new, 1)
s = s.replace('M3b instantiates the shared transport/runtime coordinator',
              'M4d retains the M4c shared coordinator and adds persistent Drift audio', 1)
s = s.replace('static volatile uint32_t apsis_runtime_probe[4];',
              'static volatile uint32_t apsis_runtime_probe[6];', 1)

# Keep persistent Drift state separate from the frozen String and runtime
# layouts. Static BSS zeroing is the correct reset state; first audio update
# initializes pitch directly at the current target and slews gain from zero.
drift_state_anchor = 'static ApsisStringDsp string_dsp;'
if drift_state_anchor not in s:
    raise SystemExit('FAIL: M4d Drift state declaration anchor missing')
s = s.replace(drift_state_anchor,
              drift_state_anchor + '\nstatic ApsisDriftDsp drift_dsp;', 1)

# Keep all M4 finite state outside the frozen ApsisRuntime layout. The
# transient pool is shared by Rhythm + Attractor; the Attractor runtime owns
# only its S&H cells/pending bind slots/telemetry.
pool_anchor = 'static volatile uint32_t apsis_runtime_probe[6];'
pool_decl = (pool_anchor +
             '\nstatic ApsisStringTransientPool string_transient_pool;'
             '\nstatic ApsisStringAttractorRuntime string_attractor_runtime;')
if pool_anchor not in s:
    raise SystemExit('FAIL: M4c String state declaration anchor missing')
s = s.replace(pool_anchor, pool_decl, 1)

# Audio order: update Drift from current live Gravity/tuning/source state,
# render persistent String, render persistent Drift, render already-active
# finite String voices, then advance the unchanged shared runtime and bind any
# future String events. Drift contributes no events and cannot perturb random
# order, queue occupancy or late-binding traces.
audio_old = '''        apsis_string_dsp_render(&string_dsp, out, n);
        apsis_runtime_process(&apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                              &mother_pending, &bloom_voicing_state, n, 0, 0);'''
audio_new = '''        ApsisRuntimeStringRhythmObservation rhythm_observation;
        uint8_t attr_ix;
        apsis_drift_dsp_update(&drift_dsp, &apsis,
                               apsis_runtime.transport.running);
        apsis_string_dsp_render(&string_dsp, out, n);
        apsis_drift_dsp_render(&drift_dsp, out, n);
        apsis_string_transient_render(&string_transient_pool, out, n);
        if (string_attractor_runtime.length ||
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
        for (attr_ix = 0u; attr_ix < string_attractor_runtime.binding_count; ++attr_ix)
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
    raise SystemExit('FAIL: M4d live audio handoff anchor missing')
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

# First boot needs no eager Attractor or Drift memset: static BSS is already
# zero. The first Drift audio update initializes pitch at target while gain
# rises with the source 25 ms time constant.
start_anchor = '    apsis_runtime_start(&apsis_runtime);'
if start_anchor not in s:
    raise SystemExit('FAIL: runtime start anchor missing')

p.write_text(s)
PY

python3 ci/patch_release_identity.py work/apsis/tools/build.py

( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) 2>&1 | tee build/evidence/apsis_host.log

printf 'PASS: APSIS overlay + host shell\n'
