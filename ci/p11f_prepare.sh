#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# P11F intentionally changes sizeof(ApsisRuntime) by removing 16 unreachable
# Bloom slots. The historical M4c test contains one structural-size assertion
# in addition to its behavioural assertions. Patch only that one expected size
# in the ephemeral CI workspace, run the unchanged full prepare/sanitizer gate,
# then restore the checked-in historical source byte-for-byte.
TEST=tests/test_string_attractor_runtime.c
BACKUP=$(mktemp)
cp "$TEST" "$BACKUP"
restore() {
  cp "$BACKUP" "$TEST"
  rm -f "$BACKUP"
}
trap restore EXIT

python3 - <<'PY'
from pathlib import Path
p = Path('tests/test_string_attractor_runtime.c')
s = p.read_text()
old = 'assert(sizeof(ApsisRuntime) == 21928u);'
new = 'assert(sizeof(ApsisRuntime) == 21032u);'
if s.count(old) != 1:
    raise SystemExit(f'FAIL: expected one frozen M4c runtime-size assertion, got {s.count(old)}')
s = s.replace(old, new, 1)
p.write_text(s)
PY

bash ci/prepare_dinghy.sh

grep -q 'assert(sizeof(ApsisRuntime) == 21032u);' "$TEST"
echo 'PASS: P11F full historical behavior/sanitizer prepare with intentional runtime size 21032 B'
