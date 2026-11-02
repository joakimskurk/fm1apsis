#!/usr/bin/env python3
from pathlib import Path

OLD = 'assert(sizeof(ApsisRuntime) == 21928u);'
NEW = 'assert(sizeof(ApsisRuntime) == 21032u);'
FILES = [
    Path('tests/test_bloom_audio_live.c'),
    Path('tests/test_string_attractor_runtime.c'),
    Path('tests/test_string_rhythm_observer.c'),
]

for p in FILES:
    s = p.read_text()
    count = s.count(OLD)
    if count != 1:
        raise SystemExit(f'{p}: expected exactly one old runtime-size assertion, found {count}')
    if NEW in s:
        raise SystemExit(f'{p}: new runtime-size assertion already present before replacement')
    p.write_text(s.replace(OLD, NEW, 1))

prepare = Path('ci/p11f_prepare.sh')
prepare.write_text(r'''#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# P11F permanently reduced ApsisRuntime by removing 16 unreachable Bloom slots.
# Phase-11 closure freezes the resulting 21032-byte host ABI in the three
# historical tests that intentionally assert the structural size.  Do not
# patch tests ephemerally here: ordinary `make test`, sanitizer preparation,
# and P11Z must all exercise the same checked-in contract.
verify_runtime_size_contract() {
  python3 - <<'PY'
from pathlib import Path
old = 'assert(sizeof(ApsisRuntime) == 21928u);'
new = 'assert(sizeof(ApsisRuntime) == 21032u);'
expected = {
    'tests/test_bloom_audio_live.c',
    'tests/test_string_attractor_runtime.c',
    'tests/test_string_rhythm_observer.c',
}
old_hits = []
new_hits = []
for p in sorted(Path('tests').rglob('*.c')):
    s = p.read_text()
    old_hits.extend([str(p)] * s.count(old))
    new_hits.extend([str(p)] * s.count(new))
if old_hits:
    raise SystemExit(f'FAIL: stale 21928-B ApsisRuntime assertion(s): {old_hits}')
if len(new_hits) != 3 or set(new_hits) != expected:
    raise SystemExit(
        f'FAIL: expected exactly one 21032-B ApsisRuntime assertion in each '
        f'of {sorted(expected)}, got {new_hits}'
    )
print('PASS: permanent P11F ApsisRuntime size contract is 21032 B in exactly three historical tests')
PY
}

verify_runtime_size_contract
bash ci/prepare_dinghy.sh
verify_runtime_size_contract

echo 'PASS: P11F full historical behavior/sanitizer prepare uses the permanent 21032-B runtime contract'
''')

# Prove the one-shot transformation has exactly the intended source effect.
old_hits = []
new_hits = []
for p in sorted(Path('tests').rglob('*.c')):
    s = p.read_text()
    old_hits.extend([str(p)] * s.count(OLD))
    new_hits.extend([str(p)] * s.count(NEW))
if old_hits:
    raise SystemExit(f'stale old runtime-size assertions remain: {old_hits}')
if len(new_hits) != 3 or set(new_hits) != {str(p) for p in FILES}:
    raise SystemExit(f'unexpected new runtime-size assertions: {new_hits}')

print('PASS: permanent Phase-11 runtime-size test contract prepared')
