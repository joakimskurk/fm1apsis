#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/apsis-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }

EMU_COMMIT=43ed5580064885e93bc8d20be931ae950d10cd4f
rm -rf work/fm1-emu work/emu-gravity-point-active work/lcd_*.bmp
mkdir -p work/emu-gravity-point-active

git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu
( cd work/fm1-emu && git checkout -q --detach "$EMU_COMMIT" )
( cd work/fm1-emu && cargo build -q --release -p fm1-emu )
EMU=work/fm1-emu/target/release/fm1-emu

OBJDUMP="${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN not set}/common/bin/objdump"
"$OBJDUMP" -t work/apsis/build/dinghy.elf > work/emu-gravity-point-active/target_symbols.txt
PROBE_ADDR=$(awk '$NF == "apsis_gravity_point_probe" { found="0x" $1 } END { if (found) print found }' \
  work/emu-gravity-point-active/target_symbols.txt)
[[ -n "$PROBE_ADDR" ]] || { echo "FAIL: apsis_gravity_point_probe symbol not found" >&2; exit 3; }

# GRAVITY -> SELECTED POINT; SEL+n4 selects degree4 without audition. Then edit
# all six numeric fields and enabled, prove plain n6 still reaches P9 Gravity,
# and HOME resets the UI-only page.
"$EMU" --fwsc "$PKG" --ms 4200 \
  --keys "300:GLO,700:ALGO:+2,1000-1350:SCL,1100:n4,1500:K1:+1,1650:K2:+1,1800:K3:+2,1950:K4:-1,2100:SELECT:+2,2250:PRESET:+2,2400:REC,2800:n6,3500:HOME" \
  --snap 900 --snap 1300 --snap 2600 --snap 3900 \
  --dump "$PROBE_ADDR" 18 \
  > work/emu-gravity-point-active/run.log 2>&1

for p in work/lcd_*.bmp; do
  [[ -f "$p" ]] && cp "$p" work/emu-gravity-point-active/
done

python3 - <<'PY'
from pathlib import Path
import hashlib, json, re

root = Path('work/emu-gravity-point-active')
log = (root / 'run.log').read_text(errors='replace')
lines = log.splitlines()
words = None
for i, line in enumerate(lines):
    if line.startswith('dump ') and '(18 words)' in line:
        got = []
        for data_line in lines[i + 1:i + 8]:
            if ':' not in data_line:
                continue
            got.extend(re.findall(r'\b[0-9a-fA-F]{8}\b', data_line.split(':', 1)[1]))
            if len(got) >= 18:
                words = [int(x, 16) for x in got[:18]]
                break
        if words is not None:
            break
if words is None:
    print(log)
    raise SystemExit('FAIL: P10G active Gravity-point telemetry dump missing')

(magic, seen, violations, initial_attrs, initial_level, initial_bright,
 initial_times, final_attrs, final_level, final_bright, final_times,
 selection, plain_degree, health, cursor, initial_other_hash,
 final_other_hash, gravity_runtime) = words

if magic != 0x47525054:
    raise SystemExit(f'FAIL: P10G active probe magic {magic:#x}')
required_seen = 0x7f
if (seen & required_seen) != required_seen:
    raise SystemExit(f'FAIL: P10G lifecycle incomplete seen={seen:#04x}, need={required_seen:#04x}')
if violations != 0:
    raise SystemExit(f'FAIL: P10G active violations={violations:#x}')
if plain_degree != 6:
    raise SystemExit(f'FAIL: P10G plain degree request recorded {plain_degree}, expected 6')
if initial_other_hash != final_other_hash:
    raise SystemExit(f'FAIL: P10G neighboring points changed {initial_other_hash:#x}->{final_other_hash:#x}')

# Packed attrs: enabled bit0; signed offset byte at bits8..15; signed octave byte16..23.
def s8(v):
    v &= 0xff
    return v - 256 if v & 0x80 else v
initial_enabled = initial_attrs & 1
initial_offset = s8(initial_attrs >> 8)
initial_octave = s8(initial_attrs >> 16)
final_enabled = final_attrs & 1
final_offset = s8(final_attrs >> 8)
final_octave = s8(final_attrs >> 16)
if final_enabled != (initial_enabled ^ 1):
    raise SystemExit(f'FAIL: P10G enabled {initial_enabled}->{final_enabled}')
if final_offset != initial_offset + 1:
    raise SystemExit(f'FAIL: P10G offset {initial_offset}->{final_offset}')
if final_octave != initial_octave + 1:
    raise SystemExit(f'FAIL: P10G octave {initial_octave}->{final_octave}')
qstep = (10 * 32767 + 500) // 1000
if final_level != initial_level + 2 * qstep:
    raise SystemExit(f'FAIL: P10G level {initial_level}->{final_level}, qstep={qstep}')
if final_bright != initial_bright - qstep:
    raise SystemExit(f'FAIL: P10G brightness {initial_bright}->{final_bright}, qstep={qstep}')
initial_attack = initial_times & 0xffff
initial_release = (initial_times >> 16) & 0xffff
final_attack = final_times & 0xffff
final_release = (final_times >> 16) & 0xffff
if final_attack != initial_attack + 8:
    raise SystemExit(f'FAIL: P10G attack {initial_attack}->{final_attack}')
if final_release != initial_release + 50:
    raise SystemExit(f'FAIL: P10G release {initial_release}->{final_release}')

selected_degree = selection & 0xff
selected_ring = (selection >> 8) & 0xff
page = (selection >> 16) & 0xff
surface = (selection >> 24) & 0xff
if selected_degree != 4 or selected_ring != 0:
    raise SystemExit(f'FAIL: P10G selected point degree={selected_degree} ring={selected_ring}')
if page != 0 or surface != 0:
    raise SystemExit(f'FAIL: P10G HOME did not reset page/surface packed={selection:#x}')

queue_high = health & 0x3ff
queue_overflow = (health >> 10) & 1
late_flags = (health >> 16) & 0xffff
if not (0 < queue_high < 512):
    raise SystemExit(f'FAIL: P10G queue high-water {queue_high}/512')
if queue_overflow:
    raise SystemExit('FAIL: P10G runtime queue overflowed')
if late_flags:
    raise SystemExit(f'FAIL: P10G late flags {late_flags:#x}')
if cursor < 130000:
    raise SystemExit(f'FAIL: P10G runtime cursor too low {cursor}')

shots = sorted(root.glob('lcd_*.bmp'), key=lambda p: int(re.search(r'(\d+)(?=\.bmp$)', p.name).group(1)))
if len(shots) != 4:
    print(log)
    raise SystemExit(f'FAIL: P10G expected four LCD snapshots, got {len(shots)}')
hashes = [hashlib.sha256(p.read_bytes()).hexdigest() for p in shots]
if hashes[0] == hashes[1]:
    raise SystemExit('FAIL: P10G SEL+degree selection did not redraw Gravity LCD')
if hashes[1] == hashes[2]:
    raise SystemExit('FAIL: P10G point enabled/edit sequence did not redraw Gravity LCD')
if hashes[2] == hashes[3]:
    raise SystemExit('FAIL: P10G HOME did not visibly leave Gravity')

report = {
    'seen_flags': f'{seen:02x}',
    'violations': violations,
    'plain_request_degree': plain_degree,
    'selected_degree': selected_degree,
    'selected_ring': selected_ring,
    'initial_enabled': initial_enabled,
    'final_enabled': final_enabled,
    'initial_offset': initial_offset,
    'final_offset': final_offset,
    'initial_octave': initial_octave,
    'final_octave': final_octave,
    'initial_level_q15': initial_level,
    'final_level_q15': final_level,
    'initial_brightness_q15': initial_bright,
    'final_brightness_q15': final_bright,
    'initial_attack_ms': initial_attack,
    'final_attack_ms': final_attack,
    'initial_release_ms': initial_release,
    'final_release_ms': final_release,
    'other_points_hash': f'{final_other_hash:08x}',
    'gravity_runtime_packed': f'{gravity_runtime:08x}',
    'queue_high_water': queue_high,
    'queue_overflowed': queue_overflow,
    'late_flags': late_flags,
    'runtime_cursor': cursor,
    'screen_sha256': hashes,
}
(root / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, sort_keys=True))
print('PASS: P10G point select-without-audition -> six-field edit -> plain Gravity request -> HOME')
PY
