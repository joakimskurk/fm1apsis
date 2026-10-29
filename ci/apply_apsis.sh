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
   apsis/apsis_drift.h apsis/apsis_drift.c \
   apsis/apsis_clock.h apsis/apsis_clock.c \
   apsis/apsis_spiral.h apsis/apsis_spiral.c \
   apsis/apsis_bloom.h apsis/apsis_bloom.c \
   apsis/apsis_keep.h apsis/apsis_keep.c \
   apsis/apsis_chain.h apsis/apsis_chain.c \
   apsis/apsis_mother.h apsis/apsis_mother.c \
   apsis/apsis_transport.h apsis/apsis_transport.c \
   apsis/apsis_late_binding.h apsis/apsis_late_binding.c \
   apsis/apsis_runtime.h apsis/apsis_runtime.c apsis/apsis_runtime_m3c.c \
   apsis/apsis_runtime_final.c \
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

# M4b2 binds already-consumed shared Rhythm events to the finite pool. The
# event observer remains part of apsis_runtime_final.c; this layer adds no
# second scheduler or queue.
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

old = '#include "apsis_runtime.c"'
new = '#include "apsis_late_binding.c"\n#include "apsis_runtime_final.c"'
if old not in s:
    raise SystemExit('FAIL: runtime include anchor missing from Dinghy overlay')
s = s.replace(old, new, 1)
s = s.replace('M3b instantiates the shared transport/runtime coordinator',
              'M4b retains the M3c shared transport/runtime coordinator', 1)
s = s.replace('static volatile uint32_t apsis_runtime_probe[4];',
              'static volatile uint32_t apsis_runtime_probe[6];', 1)

# Reserve M4b transient BSS only after every M4a/M3c state object. This keeps
# the persistent String struct at 196 bytes and avoids relocating the proven
# runtime merely because finite voices need their own capacity.
pool_anchor = 'static volatile uint32_t apsis_runtime_probe[6];'
pool_decl = pool_anchor + '\nstatic ApsisStringTransientPool string_transient_pool;'
if pool_anchor not in s:
    raise SystemExit('FAIL: M4b transient pool declaration anchor missing')
s = s.replace(pool_anchor, pool_decl, 1)

# Audio ordering is deliberate: render M4a sustain and all transients that were
# already active at block start, then advance the frozen M3c timeline while
# passively observing one due STRING_RHYTHM event, then bind that event into the
# finite pool for the next callback. A target can therefore be at most one
# callback late and can never sound before its shared-timeline sample.
audio_old = '''        apsis_string_dsp_render(&string_dsp, out, n);
        apsis_runtime_process(&apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                              &mother_pending, &bloom_voicing_state, n, 0, 0);'''
audio_new = '''        ApsisRuntimeStringRhythmObservation rhythm_observation;
        apsis_string_dsp_render(&string_dsp, out, n);
        apsis_string_transient_render(&string_transient_pool, out, n);
        apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                       &chain_sequence, &mother_pending,
                                       &bloom_voicing_state, n, 0, 0,
                                       &rhythm_observation);
        if (rhythm_observation.valid)
            (void)apsis_string_rhythm_trigger_event(&string_transient_pool,
                                                    &apsis,
                                                    &string_pitch_memory,
                                                    &rhythm_observation.event);'''
if audio_old not in s:
    raise SystemExit('FAIL: M4b live audio handoff anchor missing')
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

# The pool is live-referenced by app_audio(), so it no longer needs an explicit
# retention write. Platform BSS zeroing gives the same reset state without
# adding a 6 kB memset to the IRQ-enabled startup path.

p.write_text(s)
PY

python3 ci/patch_release_identity.py work/apsis/tools/build.py

( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) 2>&1 | tee build/evidence/apsis_host.log

printf 'PASS: APSIS overlay + host shell\n'
