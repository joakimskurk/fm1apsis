#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Build the exact current M4g/Phase-5 overlay first. Phase 6's memory probe is
# layered above it only for the dedicated CI target gate.
bash ci/apply_apsis_m4g.sh

if [[ "${APSIS_CI_SPACE_POOL_PROBE:-0}" != "1" ]]; then
  echo "PASS: P6 probe wrapper left exact M4g overlay unchanged"
  exit 0
fi

cp apsis/apsis_space_pool_probe.c work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

# Single-TU target: include the CI-only Space storage after the final dry DSP
# source. The file has no renderer and therefore cannot affect app_audio().
c_anchor = '#include "apsis_bloom_dsp.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P6 pool-probe include anchor missing')
if '#include "apsis_space_pool_probe.c"' not in s:
    s = s.replace(c_anchor, c_anchor + '\n#include "apsis_space_pool_probe.c"', 1)
p.write_text(s)

# Dinghy zeroes [_pool_start, _pool_end) in fm1_cstart(). Making the new Space
# arrays part of that range costs enough boot time to shift the emulator by one
# 256-frame callback even though no Space code runs. Production Space will own
# and initialise its long-lived delay memory itself, so reserve it in a separate
# NOLOAD section immediately BEFORE Dinghy's ordinary .pool. _pool_end remains
# after both sections, therefore the existing build report still accounts the
# full POOL-region footprint and the linker still enforces the hardware limit;
# _pool_start begins after Space, so Dinghy only clears its original pool.
ld = Path('work/apsis/firmware/app.ld')
ls = ld.read_text()
old = '.pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL'
new = '.apsis_space_pool (NOLOAD) : { KEEP(*(.apsis_space_pool*)) . = ALIGN(4); } > POOL\n    ' + old
if old not in ls:
    raise SystemExit('FAIL: P6 pool-probe linker anchor missing')
ls = ls.replace(old, new, 1)
ld.write_text(ls)
PY

# Compile the generated single-TU shell before the real target gate.
VERSION=${APSIS_VERSION:-0.1-dev}
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

echo "PASS: P6 selected FDN storage reserved before Dinghy zeroed .pool"
