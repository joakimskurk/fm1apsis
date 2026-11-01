#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/apsis-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }

EMU_COMMIT=43ed5580064885e93bc8d20be931ae950d10cd4f
rm -rf work/fm1-emu work/emu-spiral-attractor-active work/lcd_*.bmp
mkdir -p work/emu-spiral-attractor-active

git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu
( cd work/fm1-emu && git checkout -q --detach "$EMU_COMMIT" )
( cd work/fm1-emu && cargo build -q --release -p fm1-emu )
EMU=work/fm1-emu/target/release/fm1-emu

OBJDUMP="${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN not set}/common/bin/objdump"
"$OBJDUMP" -t work/apsis/build/dinghy.elf > work/emu-spiral-attractor-active/target_symbols.txt
PROBE_ADDR=$(awk '$NF == "apsis_spiral_attractor_probe" { found="0x" $1 } END { if (found) print found }' \
  work/emu-spiral-attractor-active/target_symbols.txt)
[[ -n "$PROBE_ADDR" ]] || { echo "FAIL: apsis_spiral_attractor_probe symbol not found" >&2; exit 3; }

# Physical lifecycle: enter SPIRAL with ARP, ALGORITHM switches ROUTE->ATTRACTOR,
# REC delegates deterministic reroll to frozen M4f, n0 toggles gate0, ALGORITHM
# returns to ROUTE, HOME exits. Default route is empty and must stay empty.
"$EMU" --fwsc "$PKG" --ms 3400 \
  --keys "300:ARP,700:ALGO:+1,1100:REC,1600:n0,2200:ALGO:+1,2700:HOME" \
  --snap 900 --snap 1350 --snap 1900 --snap 3050 \
  --dump "$PROBE_ADDR" 14 \
  > work/emu-spiral-attractor-active/run.log 2>&1

for p in work/lcd_*.bmp; do
  [[ -f "$p" ]] && cp "$p" work/emu-spiral-attractor-active/
done

python3 - <<'PY'
from pathlib import Path
import hashlib, json, re

root = Path('work/emu-spiral-attractor-active')
log = (root / 'run.log').read_text(errors='replace')
lines = log.splitlines()
words = None
for i, line in enumerate(lines):
    if line.startswith('dump ') and '(14 words)' in line:
        got = []
        for data_line in lines[i + 1:i + 7]:
            if ':' not in data_line:
                continue
            got.extend(re.findall(r'\b[0-9a-fA-F]{8}\b', data_line.split(':', 1)[1]))
            if len(got) >= 14:
                words = [int(x, 16) for x in got[:14]]
                break
        if words is not None:
            break
if words is None:
    print(log)
    raise SystemExit('FAIL: P10F active Spiral Attractor telemetry dump missing')

(magic, seen, violations, initial_gates, final_gates, initial_seed,
 final_seed, route_counts, initial_route_hash, final_route_hash,
 editor_surface, health, cursor, selected_length_rate) = words

if magic != 0x53504154:
    raise SystemExit(f'FAIL: P10F active probe magic {magic:#x}')
required_seen = 0x3f
if (seen & required_seen) != required_seen:
    raise SystemExit(f'FAIL: P10F active lifecycle incomplete seen={seen:#04x}, need={required_seen:#04x}')
if violations != 0:
    raise SystemExit(f'FAIL: P10F ATTRACTOR mutated P9 route violations={violations:#x}')
if initial_seed != 911 or final_seed != 4825:
    raise SystemExit(f'FAIL: P10F frozen reroll seed {initial_seed}->{final_seed}, expected 911->4825')
# Default length is 16. Frozen M4f reroll produces low mask A5FD; the later
# physical n0 edge must toggle gate0 to A5FC.
if final_gates != 0x0000a5fc:
    raise SystemExit(f'FAIL: P10F reroll+gate final={final_gates:#010x}, expected 0x0000a5fc')
initial_route_count = route_counts & 0xffff
final_route_count = (route_counts >> 16) & 0xffff
if initial_route_count != 0 or final_route_count != 0:
    raise SystemExit(f'FAIL: P10F route count changed {initial_route_count}->{final_route_count}')
if initial_route_hash != final_route_hash:
    raise SystemExit(f'FAIL: P10F route hash changed {initial_route_hash:#x}->{final_route_hash:#x}')
if editor_surface != 0:
    raise SystemExit(f'FAIL: P10F editor did not clear on HOME packed={editor_surface:#x}')
selected = selected_length_rate & 0xff
length = (selected_length_rate >> 8) & 0xff
rate = (selected_length_rate >> 16) & 0xff
if selected != 0 or length != 16 or rate != 2:
    raise SystemExit(f'FAIL: P10F final editor snapshot selected={selected} length={length} rate={rate}')

queue_high = health & 0x3ff
queue_overflow = (health >> 10) & 1
late_flags = (health >> 16) & 0xffff
if not (0 < queue_high < 512):
    raise SystemExit(f'FAIL: P10F active queue high-water {queue_high}/512')
if queue_overflow:
    raise SystemExit('FAIL: P10F active runtime queue overflowed')
if late_flags:
    raise SystemExit(f'FAIL: P10F active late-binding flags {late_flags:#x}')
if cursor < 100000:
    raise SystemExit(f'FAIL: P10F active runtime cursor unexpectedly low: {cursor}')

shots = sorted(root.glob('lcd_*.bmp'), key=lambda p: int(re.search(r'(\d+)(?=\.bmp$)', p.name).group(1)))
if len(shots) != 4:
    print(log)
    raise SystemExit(f'FAIL: P10F expected four LCD snapshots, got {len(shots)}')
hashes = [hashlib.sha256(p.read_bytes()).hexdigest() for p in shots]
if hashes[0] == hashes[1]:
    raise SystemExit('FAIL: P10F reroll changed state but did not redraw SPIRAL LCD')
if hashes[1] == hashes[2]:
    raise SystemExit('FAIL: P10F gate edit changed state but did not redraw SPIRAL LCD')
if hashes[2] == hashes[3]:
    raise SystemExit('FAIL: P10F HOME did not visibly leave SPIRAL')

report = {
    'seen_flags': f'{seen:02x}',
    'violations': violations,
    'initial_gates': f'{initial_gates:08x}',
    'final_gates': f'{final_gates:08x}',
    'initial_seed': initial_seed,
    'final_seed': final_seed,
    'initial_route_count': initial_route_count,
    'final_route_count': final_route_count,
    'route_hash': f'{final_route_hash:08x}',
    'editor_surface_final': editor_surface,
    'selected_cell_final': selected,
    'length_final': length,
    'rate_final': rate,
    'queue_high_water': queue_high,
    'queue_overflowed': queue_overflow,
    'late_flags': late_flags,
    'runtime_cursor': cursor,
    'screens': [p.name for p in shots],
    'screen_sha256': hashes,
}
(root / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, sort_keys=True))
print('PASS: P10F ARP -> ATTRACTOR -> source reroll -> gate -> ROUTE -> HOME with route isolation and redraw')
PY
