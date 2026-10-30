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

# Keep runtime_ready low while the tiny retention probe touches both ends of
# the pool arrays. No room DSP is called and no musical state is changed.
ready_anchor = '    runtime_ready = 1u;'
if ready_anchor not in s:
    raise SystemExit('FAIL: P6 pool-probe init anchor missing')
s = s.replace(ready_anchor,
              '    (void)apsis_space_pool_probe();\n' + ready_anchor,
              1)

p.write_text(s)
PY

# Compile the generated single-TU shell before the real target gate.
VERSION=${APSIS_VERSION:-0.1-dev}
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

echo "PASS: P6 selected FDN storage reserved silently in Dinghy .pool"
