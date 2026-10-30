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

# Single-TU target: include the CI-only pool storage after the final dry DSP
# source. The file has no renderer and therefore cannot affect app_audio().
c_anchor = '#include "apsis_bloom_dsp.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P6 pool-probe include anchor missing')
if '#include "apsis_space_pool_probe.c"' not in s:
    s = s.replace(c_anchor, c_anchor + '\n#include "apsis_space_pool_probe.c"', 1)
p.write_text(s)

# This gate must prove memory placement without executing any new startup code.
# Dinghy's production linker collects .pool with a wildcard but does not KEEP
# otherwise-unreferenced CI-only sections. Patch only the copied target linker
# script so the exact selected FDN arrays are retained entirely at link time.
ld = Path('work/apsis/firmware/app.ld')
ls = ld.read_text()
old = '.pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL'
new = '.pool (NOLOAD) : { _pool_start = .; KEEP(*(.pool*)) . = ALIGN(4); _pool_end = .; } > POOL'
if old not in ls:
    raise SystemExit('FAIL: P6 pool-probe linker anchor missing')
ls = ls.replace(old, new, 1)
ld.write_text(ls)
PY

# Compile the generated single-TU shell before the real target gate.
VERSION=${APSIS_VERSION:-0.1-dev}
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

echo "PASS: P6 selected FDN storage retained silently in Dinghy .pool"
