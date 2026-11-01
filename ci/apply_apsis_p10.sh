#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# P10B is deliberately a storage/link checkpoint above the fully green P9H4b
# target. It reserves the source-locked String Motion runtime in Dinghy POOL and
# resets it at boot, but does not yet feed Clock/input/audio into the motion
# engine. This proves the real FM-1 memory budget without changing idle work.
bash ci/apply_apsis_p9h4b.sh

cp apsis/apsis_string_motion.h apsis/apsis_string_motion.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path
p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_string_manual_audio.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P10B P9H4b String header anchor missing')
if '#include "apsis_string_motion.h"' not in s:
    s = s.replace(h_anchor, h_anchor + '\n#include "apsis_string_motion.h"', 1)

c_anchor = '#include "apsis_string_manual_audio.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P10B P9H4b String source anchor missing')
if '#include "apsis_string_motion.c"' not in s:
    s = s.replace(c_anchor, c_anchor + '\n#include "apsis_string_motion.c"', 1)

# 25 KiB cannot live in the 96-KiB ordinary RAM budget. Dinghy's .pool is the
# documented home for large zero-initialised buffers and is cleared at boot.
state_anchor = 'static ApsisControlsPerformanceMailbox apsis_controls_performance;'
if state_anchor not in s:
    raise SystemExit('FAIL: P10B P9H4b performance state anchor missing')
s = s.replace(
    state_anchor,
    state_anchor +
    '\nstatic ApsisStringMotionRuntime apsis_string_motion '
    '__attribute__((section(".pool")));',
    1)

init_anchor = '    apsis_controls_performance_reset(&apsis_controls_performance);'
if init_anchor not in s:
    raise SystemExit('FAIL: P10B P9H4b performance reset anchor missing')
s = s.replace(init_anchor,
              init_anchor + '\n    apsis_string_motion_reset(&apsis_string_motion);', 1)

p.write_text(s)
PY

grep -q '#include "apsis_string_motion.c"' work/apsis/firmware/src/dinghy.c
grep -q 'ApsisStringMotionRuntime apsis_string_motion __attribute__((section(".pool")))' \
  work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_motion_reset(&apsis_string_motion);' work/apsis/firmware/src/dinghy.c

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
main = s[s.index('static uint8_t apsis_p9h3_dispatch('):b]
# P10B must be allocation/link only. No motion work may enter either hot path.
for forbidden in ('apsis_string_motion_pulse(',
                  'apsis_string_motion_capture(',
                  'apsis_string_motion_playback(',
                  'apsis_string_motion_manual_split('):
    if forbidden in audio or forbidden in main:
        raise SystemExit(f'FAIL: P10B activated motion before P10C: {forbidden}')
# Frozen runtime layout is never enlarged by Motion.
if 'static ApsisStringMotionRuntime apsis_string_motion __attribute__((section(".pool")));' not in s:
    raise SystemExit('FAIL: P10B Motion runtime not in Dinghy POOL')
print('PASS: P10B reserves/reset String Motion in POOL with no main/audio activation')
PY

echo "PASS: P10B String Motion storage layered above frozen P9H4b target"
