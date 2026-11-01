#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# P10B is deliberately a storage/link checkpoint above the fully green P9H4b
# target. It reserves the source-locked String Motion runtime in hardware POOL,
# but does not yet feed Clock/input/audio into the motion engine. The large raw
# tracks live in a dedicated NOLOAD pre-pool so neither Dinghy cstart nor
# app_main() spends a first-audio-window clearing 25 KiB of unreachable points.
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

# 25 KiB cannot live in the 96-KiB ordinary RAM budget. Keep it inside the same
# hardware POOL accounting as Cathedral, but before Dinghy's _pool_start so the
# generic startup clear does not touch it. Track bytes are unreadable while
# count==0, therefore only the small metadata surface must be reset explicitly.
state_anchor = 'static ApsisControlsPerformanceMailbox apsis_controls_performance;'
if state_anchor not in s:
    raise SystemExit('FAIL: P10B P9H4b performance state anchor missing')
state_extra = state_anchor + '''
static ApsisStringMotionRuntime apsis_string_motion
    __attribute__((section(".apsis_motion_pool")));

static void apsis_string_motion_boot_reset(ApsisStringMotionRuntime *motion)
{
    uint8_t *ctx;
    uint32_t n;
    if (!motion) return;
    motion->left.count = 0u;
    motion->right.count = 0u;
    ctx = (uint8_t *)&motion->pitch_context;
    n = (uint32_t)sizeof(motion->pitch_context);
    while (n--) *ctx++ = 0u;
    motion->armed = 0u;
    motion->recording = 0u;
    motion->record_target = APSIS_STRING_MOTION_BOTH;
    motion->preferred_target = APSIS_STRING_MOTION_BOTH;
    motion->manual_override = 0u;
    motion->just_finished = 0u;
    motion->record_cycles = 0u;
    motion->play_cycle = 0u;
    motion->record_pulse_index = 0u;
    motion->play_pulse_index = 0u;
    motion->record_pulse_sample = 0u;
    motion->play_pulse_sample = 0u;
    motion->record_pulse_interval = 0u;
    motion->play_pulse_interval = 0u;
    motion->last_capture_sample = 0u;
    motion->owned_left_q15 = 0u;
    motion->owned_right_q15 = 0u;
}
'''
s = s.replace(state_anchor, state_extra, 1)

init_anchor = '    apsis_controls_performance_reset(&apsis_controls_performance);'
if init_anchor not in s:
    raise SystemExit('FAIL: P10B P9H4b performance reset anchor missing')
s = s.replace(init_anchor,
              init_anchor + '\n    apsis_string_motion_boot_reset(&apsis_string_motion);', 1)

p.write_text(s)

# Same proven pattern as P6 Cathedral: dedicated NOLOAD storage immediately
# before ordinary .pool. The POOL region still accounts for the bytes, while
# Dinghy only clears _pool_start.._pool_end at boot.
ld = Path('work/apsis/firmware/app.ld')
ls = ld.read_text()
old = '.pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL'
new = '.apsis_motion_pool (NOLOAD) : { KEEP(*(.apsis_motion_pool*)) . = ALIGN(4); } > POOL\n    ' + old
if old not in ls:
    raise SystemExit('FAIL: P10B Motion linker anchor missing')
ls = ls.replace(old, new, 1)
ld.write_text(ls)
PY

grep -q '#include "apsis_string_motion.c"' work/apsis/firmware/src/dinghy.c
grep -q 'section(".apsis_motion_pool")' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_motion_boot_reset(&apsis_string_motion);' work/apsis/firmware/src/dinghy.c
grep -q '^    .apsis_motion_pool (NOLOAD)' work/apsis/firmware/app.ld

python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
ld = Path('work/apsis/firmware/app.ld').read_text()
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
if 'apsis_string_motion_reset(&apsis_string_motion);' in s:
    raise SystemExit('FAIL: P10B still performs full 25-KiB Motion reset at boot')
if ld.index('.apsis_motion_pool (NOLOAD)') > ld.index('.pool (NOLOAD)'):
    raise SystemExit('FAIL: P10B Motion NOLOAD section is not before Dinghy .pool')
if '_pool_start' in ld[ld.index('.apsis_motion_pool (NOLOAD)'):ld.index('.pool (NOLOAD)')]:
    raise SystemExit('FAIL: P10B Motion section accidentally extends Dinghy clear range')
print('PASS: P10B Motion is NOLOAD pre-pool with metadata-only boot reset and no main/audio activation')
PY

echo "PASS: P10B String Motion storage layered above frozen P9H4b target"
