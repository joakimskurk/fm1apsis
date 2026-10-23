#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/apsis-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }

EMU_COMMIT=43ed5580064885e93bc8d20be931ae950d10cd4f
rm -rf work/fm1-emu work/emu-smoke work/lcd_*.bmp
mkdir -p work/emu-smoke

git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu
( cd work/fm1-emu && git checkout -q --detach "$EMU_COMMIT" )
( cd work/fm1-emu && cargo build -q --release -p fm1-emu )
EMU=work/fm1-emu/target/release/fm1-emu

# Phase 1 contract: APSIS boots, accepts native surface buttons and remains
# intentionally silent. SEQ must select CLOCK; HOME must return to MOTHER.
"$EMU" --fwsc "$PKG" --ms 3000 --wav work/emu-smoke/apsis.wav \
  --keys "700:SEQ,1600:HOME" \
  --snap 500 --snap 1100 --snap 2100 \
  > work/emu-smoke/run.log 2>&1

python3 - <<'PY'
from pathlib import Path
import hashlib, json, struct, wave

root = Path('work/emu-smoke')
for p in Path('work').glob('lcd_*.bmp'):
    (root / p.name).write_bytes(p.read_bytes())
shots = sorted(root.glob('lcd_*.bmp'))
if len(shots) < 3:
    print((root / 'run.log').read_text(errors='replace'))
    raise SystemExit(f'FAIL: expected 3 LCD snapshots, got {len(shots)}')
h = [hashlib.sha256(p.read_bytes()).hexdigest() for p in shots]
if h[0] == h[1]:
    raise SystemExit('FAIL: SEQ did not visibly change APSIS surface')
if h[1] == h[2]:
    raise SystemExit('FAIL: HOME did not visibly return from CLOCK surface')

wp = root / 'apsis.wav'
with wave.open(str(wp), 'rb') as w:
    raw = w.readframes(w.getnframes())
    sw = w.getsampwidth()
    ch = w.getnchannels()
    fr = w.getnframes()
if sw != 2:
    raise SystemExit(f'FAIL: unexpected WAV sample width {sw}')
vals = struct.unpack('<' + 'h' * (len(raw) // 2), raw)
peak = max((abs(v) for v in vals), default=0)
if peak != 0:
    raise SystemExit(f'FAIL: Phase 1 APSIS shell must be silent, peak={peak}')

report = {
    'screens': [p.name for p in shots],
    'screen_sha256': h,
    'wav_frames': fr,
    'wav_channels': ch,
    'wav_peak': peak,
}
(root / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, sort_keys=True))
print('PASS: APSIS emulator boot/input/silent-audio contract')
PY
