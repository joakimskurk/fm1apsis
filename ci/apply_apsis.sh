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
   work/apsis/firmware/src/

# The checked-in overlay remains the stable UI shell. M3c swaps only the
# generated target translation-unit tail: late binding is compiled before the
# coordinator, and the coordinator includes the validated M3b runtime as its
# base implementation. No second runtime object or module clock is introduced.
# The passive emulator probe is extended here as M3c-only validation telemetry.
python3 - <<'PY'
from pathlib import Path
p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()
old = '#include "apsis_runtime.c"'
new = '#include "apsis_late_binding.c"\n#include "apsis_runtime_m3c.c"'
if old not in s:
    raise SystemExit('FAIL: runtime include anchor missing from Dinghy overlay')
s = s.replace(old, new, 1)
s = s.replace('M3b instantiates the shared transport/runtime coordinator',
              'M3c instantiates the shared transport/runtime coordinator', 1)
s = s.replace('static volatile uint32_t apsis_runtime_probe[4];',
              'static volatile uint32_t apsis_runtime_probe[6];', 1)
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
p.write_text(s)
PY

python3 ci/patch_release_identity.py work/apsis/tools/build.py

# Host compilation is a structural gate for the single-translation-unit FM-1 app.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) 2>&1 | tee build/evidence/apsis_host.log

printf 'PASS: APSIS overlay + host shell\n'
