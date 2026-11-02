#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# APSIS on Joakim Eriksson's FM-1 emulator (fm1-emu's fm1-live), headless and FM1_STRICT: each instruction takes its
# measured cycles, the flash cache misses, the LCD takes its time at the SPI clock. A scripted performance runs for
# 20 s; the firmware's own audio-load probe (apsis_load_probe in app.c) is read back from RAM.
#   EMU=path/to/fm1-live sh tests/emu.sh build/apsis.fwsc build/apsis.elf [OUT]
# The audio interrupt has 5805 us a block (256 frames). Fails when the worst block overruns it; prints the average.
# An emulator is not the chip: real-hardware margin stays unverified until an FM-1 runs it.
set -e
PKG=${1:?package}; ELF=${2:?elf}; OUT=${3:-build/emu}
EMU=${EMU:?EMU must point at fm1-live}
OBJDUMP=${OBJDUMP:-${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN or OBJDUMP}/common/bin/objdump}
mkdir -p "$OUT"
ADDR=$("$OBJDUMP" -t "$ELF" | awk '$NF == "apsis_load_probe" { print $1 }')
[ -n "$ADDR" ] || { echo "apsis_load_probe not in $ELF"; exit 2; }

# ids: buttons 0..13 (OCT- OCT+ FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC), keys 14..40; eN:clicks encoders
# The transport runs from boot. STRING with plucks, BLOOM petals and a grow, GRAVITY moves, SPACE, the knobs.
KEYS="1000:4,1050:4^,1500:21,1600:21^,1800:25,1900:25^,2100:28,2200:28^,2500:e2:+6,3000:e3:-4"
KEYS="$KEYS,4000:2,4050:2^,4500:14,4600:14^,4700:16,4800:16^,5000:13,5100:18,5300:18^,5400:13^,5800:20,5900:20^"
KEYS="$KEYS,7000:7,7050:7^,7500:15,7600:15^,8500:19,8600:19^,9500:22,9600:22^"
KEYS="$KEYS,11000:3,11010:2,11060:2^,11070:3^,11500:e2:+8,12000:e4:+5"
KEYS="$KEYS,13000:10,13050:10^,14000:11,14050:11^,15000:8,15050:8^,16000:e5:+3"

if [ "${PROFILE:-0}" = 1 ]; then   # where the cycles go, 6..8 s into the performance (BLOOM, petals)
    FM1_STRICT=1 FM1_TPROF="$OUT/tprof.txt,6000,8000" FM1_BENCH_KEYS="$KEYS" "$EMU" --fwsc "$PKG" --shot 8100 \
        > "$OUT/emu.log" 2>&1 || { cat "$OUT/emu.log"; exit 1; }
    "$OBJDUMP" -t "$ELF" > "$OUT/symbols.txt"
    python3 "$(dirname "$0")/../tools/profile_report.py" "$OUT/tprof.txt" "$OUT/symbols.txt" 40 > "$OUT/profile.txt"
    exit 0
fi

FM1_STRICT=1 FM1_DUMP="$ADDR" FM1_BENCH_KEYS="$KEYS" "$EMU" --fwsc "$PKG" --shot 20000 > "$OUT/emu.log" 2>&1 || {
    cat "$OUT/emu.log"; echo "FAIL: the emulator stopped"; exit 1; }
cp work/panel.ppm "$OUT/panel.ppm" 2>/dev/null || true
python3 - "$OUT" <<'PY'
import json, re, sys
from pathlib import Path
out = Path(sys.argv[1])
log = (out / "emu.log").read_text(errors="replace")
m = re.search(r"^dump 0x[0-9a-f]+: ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})", log, re.M)
if not m:
    print(log); sys.exit("FAIL: no apsis_load_probe dump in the emulator's output")
last, worst, blocks, total = (int(g, 16) for g in m.groups())
BUDGET = 5805
avg = total / blocks if blocks else 0
r = {"blocks": blocks, "avg_us": round(avg), "worst_us": worst, "last_us": last, "budget_us": BUDGET,
     "avg_pct": round(100 * avg / BUDGET, 1), "worst_pct": round(100 * worst / BUDGET, 1)}
(out / "load.json").write_text(json.dumps(r, indent=1) + "\n")
print(f"audio load (FM1_STRICT, 20 s scripted): avg {r['avg_us']} us ({r['avg_pct']} %), "
      f"worst {worst} us ({r['worst_pct']} %) of {BUDGET} us, {blocks} blocks")
if blocks < 3000:
    sys.exit(f"FAIL: only {blocks} audio blocks in 20 s (expected ~3445): the audio stalled")
if worst >= BUDGET:
    sys.exit("FAIL: a block took longer than its 5.8 ms: the sound drops out")
if worst > BUDGET // 2:
    print("WARNING: the worst block is over half the budget (docs/dinghy/RULES.md); USB and the keys interrupt it too")
PY
