#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 6 is layered strictly above the frozen M4g/Phase-5 target generator.
# APSIS_CI_SPACE_DRY_ONLY=1 keeps the exact same split source-bus path and full
# production storage, but deliberately omits the final room render call so CI
# can prove that source splitting itself does not change frozen dry PCM.
bash ci/apply_apsis_m4g.sh

VERSION=${APSIS_VERSION:-0.1-dev}
DRY_ONLY=${APSIS_CI_SPACE_DRY_ONLY:-0}

cp apsis/apsis_space_dsp.h apsis/apsis_space_dsp.c \
   apsis/apsis_space_room.h apsis/apsis_space_room.c \
   apsis/apsis_space_target.h apsis/apsis_space_target.c \
   work/apsis/firmware/src/

APSIS_CI_SPACE_DRY_ONLY="$DRY_ONLY" python3 - <<'PY'
from pathlib import Path
import os

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()
dry_only = os.environ.get('APSIS_CI_SPACE_DRY_ONLY') == '1'

h_anchor = '#include "apsis_bloom_dsp.h"'
if '#include "apsis_space_target.h"' not in s:
    if h_anchor not in s:
        raise SystemExit('FAIL: P6 Space header anchor missing')
    s = s.replace(
        h_anchor,
        h_anchor +
        '\n#include "apsis_space_dsp.h"'
        '\n#include "apsis_space_room.h"'
        '\n#include "apsis_space_target.h"',
        1)

c_anchor = '#include "apsis_bloom_dsp.c"'
if '#include "apsis_space_target.c"' not in s:
    if c_anchor not in s:
        raise SystemExit('FAIL: P6 Space source anchor missing')
    s = s.replace(
        c_anchor,
        c_anchor +
        '\n#include "apsis_space_dsp.c"'
        '\n#include "apsis_space_room.c"'
        '\n#include "apsis_space_target.c"',
        1)

state_anchor = 'static uint8_t bloom_audio_started;'
if state_anchor not in s:
    raise SystemExit('FAIL: P6 Space state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisSpaceTarget space_target;', 1)

ready_anchor = '    if (runtime_ready) {'
if ready_anchor not in s:
    raise SystemExit('FAIL: P6 app_audio ready anchor missing')
s = s.replace(ready_anchor,
              ready_anchor + '\n        apsis_space_target_begin_block(&space_target, n);', 1)

# Preserve the exact M4g source/render order. Each source renderer now writes
# into one reusable Q15 source bus; commit_source applies Mother mix/mute to dry
# and derives the post-Mother Space send from the same contribution.
def split_render(call, send, mother_ix):
    old = call
    new = (
        '        {\n'
        '            int32_t *space_source = apsis_space_target_source_buffer(&space_target, n);\n'
        f'            {call.strip().replace("out, n", "space_source, n")}\n'
        '            apsis_space_target_commit_source(\n'
        '                &space_target, out, n, '
        f'{send}, apsis.mother.mix_q15[{mother_ix}],\n'
        f'                (uint8_t)(apsis.mother.mute_mask & (1u << {mother_ix})));\n'
        '        }'
    )
    return old, new

replacements = [
    split_render('        apsis_string_dsp_render(&string_dsp, out, n);',
                 'apsis.string.send_q15', 0),
    split_render('        apsis_drift_dsp_render(&drift_dsp, out, n);',
                 'apsis.drift.send_q15', 1),
    split_render('        apsis_clock_dsp_render(&clock_dsp, out, n);',
                 'apsis.clock.send_q15', 2),
    split_render('        apsis_string_transient_render(&string_transient_pool, out, n);',
                 'apsis.string.send_q15', 0),
    split_render('                    apsis_spiral_dsp_render(&spiral_dsp, out, n);',
                 'apsis.spiral.send_q15', 3),
    split_render('                    apsis_bloom_dsp_render(&bloom_dsp, out, n);',
                 'apsis.bloom.send_q15', 4),
    split_render('                apsis_spiral_dsp_render(&spiral_dsp, out, n);',
                 'apsis.spiral.send_q15', 3),
]
for old, new in replacements:
    if old not in s:
        raise SystemExit(f'FAIL: P6 source split anchor missing: {old.strip()}')
    s = s.replace(old, new, 1)

# Room rendering is intentionally after every current-block source contribution
# has been committed. The dry-only CI mode skips only this call; it still uses
# the split bus, Mother gain/mute and production NOLOAD storage layout.
probe_anchor = '        apsis_runtime_probe[0] = 0x41505349u; /* "APSI" */'
if probe_anchor not in s:
    raise SystemExit('FAIL: P6 room render anchor missing')
if not dry_only:
    s = s.replace(probe_anchor,
                  '        apsis_space_target_render(&space_target, &apsis.space, out, n);\n' +
                  probe_anchor, 1)

start_anchor = '    apsis_runtime_start(&apsis_runtime);'
if start_anchor not in s:
    raise SystemExit('FAIL: P6 room bind anchor missing')
s = s.replace(start_anchor,
              start_anchor + '\n    (void)apsis_space_target_bind(&space_target);', 1)

p.write_text(s)

# Reserve long-lived Space storage in a NOLOAD section immediately before the
# ordinary Dinghy .pool. _pool_end stays after both, so build accounting and the
# hardware POOL limit still cover the full allocation while Dinghy clears only
# its original pool at boot.
ld = Path('work/apsis/firmware/app.ld')
ls = ld.read_text()
old = '.pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL'
new = '.apsis_space_pool (NOLOAD) : { KEEP(*(.apsis_space_pool*)) . = ALIGN(4); } > POOL\n    ' + old
if old not in ls:
    raise SystemExit('FAIL: P6 Space linker anchor missing')
ls = ls.replace(old, new, 1)
ld.write_text(ls)
PY

# Compile the actual generated single-TU Phase-6 target before the real JieLi
# link gate. This catches target-only namespace/include regressions immediately.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

if [[ "$DRY_ONLY" == "1" ]]; then
  echo "PASS: P6 split-source dry-only overlay with production Space storage"
else
  echo "PASS: P6 shared Space/Cathedral overlay integrated into app_audio"
fi
