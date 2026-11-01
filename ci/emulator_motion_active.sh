#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/apsis-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }

EMU_COMMIT=43ed5580064885e93bc8d20be931ae950d10cd4f
rm -rf work/fm1-emu work/emu-motion-active
mkdir -p work/emu-motion-active

git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu
( cd work/fm1-emu && git checkout -q --detach "$EMU_COMMIT" )
( cd work/fm1-emu && cargo build -q --release -p fm1-emu )
EMU=work/fm1-emu/target/release/fm1-emu

OBJDUMP="${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN not set}/common/bin/objdump"
"$OBJDUMP" -t work/apsis/build/dinghy.elf > work/emu-motion-active/target_symbols.txt
PROBE_ADDR=$(awk '$NF == "apsis_motion_probe" { found="0x" $1 } END { if (found) print found }' \
  work/emu-motion-active/target_symbols.txt)
[[ -n "$PROBE_ADDR" ]] || { echo "FAIL: apsis_motion_probe symbol not found" >&2; exit 3; }

# Default APSIS Clock is 12 steps, 72 BPM, 1/2 division => one full cycle is
# 220,500 samples (5 s). Arm after boot, so capture must begin at the next
# visible step 1 near runtime sample 220,500 and finish one cycle later.
# After finish, hold one String key long enough to enter manualOverride, then
# release it so loop playback must resume at the current phase.
"$EMU" --fwsc "$PKG" --ms 12000 --wav work/emu-motion-active/apsis-motion.wav \
  --keys "300:ENV,600:REC,5600:n0,6600:n4,7600:n8,8600:n2,9600:n7,10600-11300:n5" \
  --dump "$PROBE_ADDR" 10 \
  > work/emu-motion-active/run.log 2>&1

python3 - <<'PY'
from pathlib import Path
import json, math, re, struct, wave

root = Path('work/emu-motion-active')
log = (root / 'run.log').read_text(errors='replace')
lines = log.splitlines()
words = None
for i, line in enumerate(lines):
    if line.startswith('dump ') and '(10 words)' in line:
        got = []
        for data_line in lines[i + 1:i + 5]:
            if ':' not in data_line:
                continue
            got.extend(re.findall(r'\b[0-9a-fA-F]{8}\b', data_line.split(':', 1)[1]))
            if len(got) >= 10:
                words = [int(x, 16) for x in got[:10]]
                break
        if words is not None:
            break
if words is None:
    print(log)
    raise SystemExit('FAIL: P10D3 Motion telemetry dump missing')

magic, seen, current, counts, rec_start, rec_finish, man_start, man_resume, health, cursor = words
if magic != 0x4d4f544e:
    raise SystemExit(f'FAIL: P10D3 Motion magic {magic:#x}')
required_seen = 0xff
if (seen & required_seen) != required_seen:
    raise SystemExit(f'FAIL: P10D3 lifecycle incomplete seen={seen:#04x}, need={required_seen:#04x}')
# Final state: loop is playing and playback produced a frame; not armed,
# recording or manually overridden after key release.
if (current & 0x14) != 0x14 or (current & 0x0b):
    raise SystemExit(f'FAIL: P10D3 final Motion state current={current:#04x}')
left = counts & 0xffff
right = (counts >> 16) & 0xffff
if left <= 1 or right <= 1:
    raise SystemExit(f'FAIL: P10D3 Dual+LINK recording missing points L={left} R={right}')
# Capture is step-1 quantized, not immediate at the 600 ms REC gesture.
if not (210000 <= rec_start <= 232000):
    raise SystemExit(f'FAIL: P10D3 record start {rec_start} not near next step-1 sample 220500')
cycle = rec_finish - rec_start
if not (218000 <= cycle <= 223000):
    raise SystemExit(f'FAIL: P10D3 recorded cycle {cycle} samples, expected ~220500')
if not (rec_finish < man_start < man_resume < cursor):
    raise SystemExit(
        f'FAIL: P10D3 lifecycle order finish={rec_finish} manual={man_start} '
        f'resume={man_resume} cursor={cursor}'
    )
if man_resume - man_start < 15000:
    raise SystemExit(f'FAIL: P10D3 manual override too short: {man_resume-man_start} samples')
queue_high = health & 0x3ff
queue_overflow = (health >> 10) & 1
late_flags = (health >> 16) & 0xffff
if not (0 < queue_high < 512):
    raise SystemExit(f'FAIL: P10D3 queue high-water {queue_high}/512')
if queue_overflow:
    raise SystemExit('FAIL: P10D3 runtime queue overflowed')
if late_flags:
    raise SystemExit(f'FAIL: P10D3 late-binding flags {late_flags:#x}')
if cursor < 480000:
    raise SystemExit(f'FAIL: P10D3 runtime cursor too low for 12 s scenario: {cursor}')

wp = root / 'apsis-motion.wav'
with wave.open(str(wp), 'rb') as w:
    raw = w.readframes(w.getnframes())
    vals = struct.unpack('<' + 'h' * (len(raw)//2), raw)
    frames = w.getnframes()
peak = max((abs(v) for v in vals), default=0)
rms = math.isqrt(sum(v*v for v in vals)//max(1, len(vals)))
if not (100 <= peak <= 8192) or rms < 20:
    raise SystemExit(f'FAIL: P10D3 active audio bounds peak={peak} rms={rms}')

report = {
    'seen_flags': f'{seen:02x}',
    'final_flags': f'{current:02x}',
    'left_points': left,
    'right_points': right,
    'record_start_sample': rec_start,
    'record_finish_sample': rec_finish,
    'record_cycle_samples': cycle,
    'manual_start_sample': man_start,
    'manual_resume_sample': man_resume,
    'queue_high_water': queue_high,
    'queue_overflowed': queue_overflow,
    'late_flags': late_flags,
    'runtime_cursor': cursor,
    'wav_frames': frames,
    'wav_peak': peak,
    'wav_rms': rms,
}
(root / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, sort_keys=True))
print('PASS: P10D3 active String Motion arm -> step-1 record -> loop -> manual override -> resume')
PY
