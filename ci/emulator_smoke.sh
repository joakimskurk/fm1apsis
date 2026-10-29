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

# M3c contract: APSIS boots, accepts native surface buttons, remains silent,
# advances the shared target runtime without queue/slot starvation, and keeps
# deterministic late-binding telemetry alive. Locate the passive probe in the
# real target ELF rather than guessing layout.
OBJDUMP="${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN not set}/common/bin/objdump"
"$OBJDUMP" -t work/apsis/build/dinghy.elf > work/emu-smoke/target_symbols.txt
PROBE_ADDR=$(awk '$NF == "apsis_runtime_probe" { found="0x" $1 } END { if (found) print found }' \
  work/emu-smoke/target_symbols.txt)
[[ -n "$PROBE_ADDR" ]] || { echo "FAIL: apsis_runtime_probe symbol not found" >&2; exit 3; }

"$EMU" --fwsc "$PKG" --ms 3000 --wav work/emu-smoke/apsis.wav \
  --keys "700:SEQ,1600:HOME" \
  --snap 500 --snap 1100 --snap 2100 \
  --dump "$PROBE_ADDR" 6 \
  > work/emu-smoke/run.log 2>&1

python3 - <<'PY'
from pathlib import Path
import hashlib, json, re, struct, wave

root = Path('work/emu-smoke')
for p in Path('work').glob('lcd_*.bmp'):
    (root / p.name).write_bytes(p.read_bytes())

def snap_time(path):
    m = re.search(r'(\d+)(?=\.bmp$)', path.name)
    if not m:
        raise SystemExit(f'FAIL: cannot parse snapshot timestamp: {path.name}')
    return int(m.group(1))

shots = sorted(root.glob('lcd_*.bmp'), key=snap_time)
if len(shots) < 3:
    print((root / 'run.log').read_text(errors='replace'))
    raise SystemExit(f'FAIL: expected 3 LCD snapshots, got {len(shots)}')
h = [hashlib.sha256(p.read_bytes()).hexdigest() for p in shots]
if h[0] == h[1]:
    raise SystemExit('FAIL: SEQ did not visibly change APSIS surface')
if h[1] == h[2]:
    raise SystemExit('FAIL: HOME did not visibly return from CLOCK surface')
if h[0] != h[2]:
    raise SystemExit('FAIL: HOME did not restore the original MOTHER surface')

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
    raise SystemExit(f'FAIL: M3c APSIS runtime must remain silent, peak={peak}')

log = (root / 'run.log').read_text(errors='replace')
lines = log.splitlines()
probe_words = None
for i, line in enumerate(lines):
    if line.startswith('dump ') and '(6 words)' in line:
        words = []
        for data_line in lines[i + 1:i + 5]:
            if ':' not in data_line:
                continue
            words.extend(re.findall(r'\b[0-9a-fA-F]{8}\b', data_line.split(':', 1)[1]))
            if len(words) >= 6:
                probe_words = [int(w, 16) for w in words[:6]]
                break
        if probe_words is not None:
            break
if probe_words is None:
    print(log)
    raise SystemExit('FAIL: runtime telemetry dump missing from emulator output')
magic, queue_high_water, queue_overflowed, cursor_low32, late_flags, late_trace_hash = probe_words
if magic != 0x41505349:
    raise SystemExit(f'FAIL: runtime telemetry magic {magic:#010x}')
if not (0 < queue_high_water < 512):
    raise SystemExit(f'FAIL: invalid live queue high-water {queue_high_water}/512')
if queue_overflowed != 0:
    raise SystemExit(f'FAIL: live runtime queue overflow flag={queue_overflowed}')
if late_flags != 0:
    raise SystemExit(f'FAIL: live late-binding starvation flags={late_flags:#x}')
if cursor_low32 == 0:
    raise SystemExit('FAIL: runtime sample cursor did not advance')
if late_trace_hash == 0:
    raise SystemExit('FAIL: late-binding telemetry hash is zero')

report = {
    'screens': [p.name for p in shots],
    'screen_sha256': h,
    'wav_frames': fr,
    'wav_channels': ch,
    'wav_peak': peak,
    'runtime_queue_high_water': queue_high_water,
    'runtime_queue_capacity': 512,
    'runtime_queue_overflowed': queue_overflowed,
    'runtime_late_flags': late_flags,
    'runtime_late_trace_hash': f'{late_trace_hash:08x}',
    'runtime_cursor_low32': cursor_low32,
}
(root / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, sort_keys=True))
print('PASS: APSIS emulator boot/input/silent-audio/M3c-runtime contract')
PY
