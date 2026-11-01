#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/apsis-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }

EMU_COMMIT=43ed5580064885e93bc8d20be931ae950d10cd4f
rm -rf work/fm1-emu work/emu-string-attractor-active work/lcd_*.bmp
mkdir -p work/emu-string-attractor-active

git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu
( cd work/fm1-emu && git checkout -q --detach "$EMU_COMMIT" )
( cd work/fm1-emu && cargo build -q --release -p fm1-emu )
EMU=work/fm1-emu/target/release/fm1-emu

OBJDUMP="${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN not set}/common/bin/objdump"
"$OBJDUMP" -t work/apsis/build/dinghy.elf > work/emu-string-attractor-active/target_symbols.txt
PROBE_ADDR=$(awk '$NF == "apsis_attractor_probe" { found="0x" $1 } END { if (found) print found }' \
  work/emu-string-attractor-active/target_symbols.txt)
[[ -n "$PROBE_ADDR" ]] || { echo "FAIL: apsis_attractor_probe symbol not found" >&2; exit 3; }

# APSIS names physical button id 3 SEL; the pinned emulator names the same
# FM-1 panel button SCL. Hold it while pressing ENV to enter ATTRACTOR context.
# Then toggle gate 0, reroll with REC, and leave STRING with HOME.
"$EMU" --fwsc "$PKG" --ms 3000 \
  --keys "300-650:SCL,400:ENV,1000:n0,1500:REC,2200:HOME" \
  --snap 850 --snap 1250 --snap 1900 --snap 2600 \
  --dump "$PROBE_ADDR" 12 \
  > work/emu-string-attractor-active/run.log 2>&1

for p in work/lcd_*.bmp; do
  [[ -f "$p" ]] && cp "$p" work/emu-string-attractor-active/
done

python3 - <<'PY'
from pathlib import Path
import hashlib, json, re

root = Path('work/emu-string-attractor-active')
log = (root / 'run.log').read_text(errors='replace')
lines = log.splitlines()
words = None
for i, line in enumerate(lines):
    if line.startswith('dump ') and '(12 words)' in line:
        got = []
        for data_line in lines[i + 1:i + 6]:
            if ':' not in data_line:
                continue
            got.extend(re.findall(r'\b[0-9a-fA-F]{8}\b', data_line.split(':', 1)[1]))
            if len(got) >= 12:
                words = [int(x, 16) for x in got[:12]]
                break
        if words is not None:
            break
if words is None:
    print(log)
    raise SystemExit('FAIL: P10E active String Attractor telemetry dump missing')

(magic, seen, violations, initial_gates, final_gates, initial_seed,
 final_seed, editor_surface, motion, health, cursor, mailbox_pending) = words

if magic != 0x41545452:
    raise SystemExit(f'FAIL: P10E active probe magic {magic:#x}')
required_seen = 0x3f
if (seen & required_seen) != required_seen:
    raise SystemExit(f'FAIL: P10E active editor lifecycle incomplete seen={seen:#04x}, need={required_seen:#04x}')
if violations != 0:
    raise SystemExit(f'FAIL: P10E editor leaked into P9/Motion paths violations={violations:#x}')

expected_gates = initial_gates ^ 0x0001
if final_gates != expected_gates:
    raise SystemExit(
        f'FAIL: P10E gate0 toggle final={final_gates:#06x} expected={expected_gates:#06x} '
        f'from initial={initial_gates:#06x}'
    )
normalized = 317 if initial_seed == 0 else min(initial_seed, 9999)
expected_seed = ((normalized - 1 + 7919) % 9999) + 1
if final_seed != expected_seed:
    raise SystemExit(
        f'FAIL: P10E reroll seed final={final_seed} expected={expected_seed} '
        f'from initial={initial_seed}'
    )

# Final UI state must be editor inactive on MOTHER (surface 0).
if editor_surface != 0:
    raise SystemExit(f'FAIL: P10E editor did not clear on HOME, packed state={editor_surface:#x}')
if motion != 0:
    raise SystemExit(f'FAIL: P10E REC/note activity leaked into Motion final state={motion:#x}')
if mailbox_pending != 0:
    raise SystemExit(f'FAIL: P10E editor left Motion mailbox pending={mailbox_pending}')

queue_high = health & 0x3ff
queue_overflow = (health >> 10) & 1
late_flags = (health >> 16) & 0xffff
if not (0 < queue_high < 512):
    raise SystemExit(f'FAIL: P10E active queue high-water {queue_high}/512')
if queue_overflow:
    raise SystemExit('FAIL: P10E active runtime queue overflowed')
if late_flags:
    raise SystemExit(f'FAIL: P10E active late-binding flags {late_flags:#x}')
if cursor < 90000:
    raise SystemExit(f'FAIL: P10E active runtime cursor unexpectedly low: {cursor}')

shots = sorted(root.glob('lcd_*.bmp'), key=lambda p: int(re.search(r'(\d+)(?=\.bmp$)', p.name).group(1)))
if len(shots) != 4:
    print(log)
    raise SystemExit(f'FAIL: P10E expected four LCD snapshots, got {len(shots)}')
hashes = [hashlib.sha256(p.read_bytes()).hexdigest() for p in shots]
# 850 ms is STRING after editor entry; 1250 ms is the same surface after gate0
# changed. They must differ, proving the P10E visual-dirty path actually caused
# the existing incremental P8 renderer to present the edited gate state.
if hashes[0] == hashes[1]:
    raise SystemExit('FAIL: P10E gate edit changed state but did not redraw STRING LCD')
# HOME must visibly leave STRING after the editor session has been cleared.
if hashes[2] == hashes[3]:
    raise SystemExit('FAIL: P10E HOME did not visibly leave STRING after editor session')

report = {
    'seen_flags': f'{seen:02x}',
    'violations': violations,
    'initial_gates': f'{initial_gates:04x}',
    'final_gates': f'{final_gates:04x}',
    'initial_seed': initial_seed,
    'final_seed': final_seed,
    'editor_surface_final': editor_surface,
    'motion_final': motion,
    'motion_mailbox_pending': mailbox_pending,
    'queue_high_water': queue_high,
    'queue_overflowed': queue_overflow,
    'late_flags': late_flags,
    'runtime_cursor': cursor,
    'screens': [p.name for p in shots],
    'screen_sha256': hashes,
}
(root / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, sort_keys=True))
print('PASS: P10E physical SEL+ENV -> gate toggle -> REC reroll -> HOME, with redraw and zero P9/Motion leakage')
PY
