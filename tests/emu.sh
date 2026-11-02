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
mkdir -p "$OUT" work                 # (fm1-live --shot writes work/panel.ppm)
if [ -n "${SYMBOLS:-}" ]; then        # (the symbol table made elsewhere: make emu runs objdump on the Mac)
    [ "$SYMBOLS" = "$OUT/symbols.txt" ] || cp "$SYMBOLS" "$OUT/symbols.txt"
else
    OBJDUMP=${OBJDUMP:-${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN or OBJDUMP}/common/bin/objdump}
    "$OBJDUMP" -t "$ELF" > "$OUT/symbols.txt"
fi
ADDR=$(awk '$NF == "apsis_load_probe" { print $1 }' "$OUT/symbols.txt")
[ -n "$ADDR" ] || { echo "apsis_load_probe not in $ELF"; exit 2; }

# ids: buttons 0..13 (OCT- OCT+ FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC), keys 14..40; eN:clicks encoders
# The transport runs from boot. STRING with plucks, BLOOM petals and a grow, GRAVITY moves, SPACE, the knobs.
KEYS="1000:4,1050:4^,1500:21,1600:21^,1800:25,1900:25^,2100:28,2200:28^,2500:e2:+6,3000:e3:-4"
KEYS="$KEYS,4000:2,4050:2^,4500:14,4600:14^,4700:16,4800:16^,5000:13,5100:18,5300:18^,5400:13^,5800:20,5900:20^"
KEYS="$KEYS,7000:7,7050:7^,7500:15,7600:15^,8500:19,8600:19^,9500:22,9600:22^"
KEYS="$KEYS,11000:3,11010:2,11060:2^,11070:3^,11500:e2:+8,12000:e4:+5"
KEYS="$KEYS,13000:10,13050:10^,14000:11,14050:11^,15000:8,15050:8^,16000:e5:+3"

# STRESS=1 (with a build made with DINGHY_DEFS=-DAPSIS_STRESS, every source at its densest): String plucks as fast as
# the 35 ms guard allows across the keys, then Bloom petals with grow (REC held), then plucks again.
if [ "${STRESS:-0}" = 1 ]; then
    KEYS="1000:4,1050:4^"
    t=1200; k=14
    while [ $t -lt 8000 ]; do KEYS="$KEYS,$t:$k,$((t + 20)):$k^"; t=$((t + 40)); k=$((k + 7)); [ $k -gt 40 ] && k=$((k - 27)); done
    KEYS="$KEYS,8000:2,8050:2^,8100:13"
    t=8200; k=14
    while [ $t -lt 12000 ]; do KEYS="$KEYS,$t:$k,$((t + 40)):$k^"; t=$((t + 100)); k=$((k + 1)); [ $k -gt 21 ] && k=14; done
    KEYS="$KEYS,12000:13^,12100:4,12150:4^"
    t=12300; k=40
    while [ $t -lt 19500 ]; do KEYS="$KEYS,$t:$k,$((t + 20)):$k^"; t=$((t + 40)); k=$((k - 5)); [ $k -lt 14 ] && k=$((k + 27)); done
fi

if [ "${PROFILE:-0}" = 1 ]; then   # where the cycles go, 6..8 s into the performance (BLOOM, petals)
    FM1_STRICT=1 FM1_TPROF="$OUT/tprof.txt,${PROF_FROM:-6000},${PROF_TO:-8000}" FM1_BENCH_KEYS="$KEYS" "$EMU" --fwsc "$PKG" --shot $(( ${PROF_TO:-8000} + 100 )) \
        > "$OUT/emu.log" 2>&1 || { cat "$OUT/emu.log"; exit 1; }
    python3 "$(dirname "$0")/../tools/profile_report.py" "$OUT/tprof.txt" "$OUT/symbols.txt" 40 > "$OUT/profile.txt"
    exit 0
fi

FM1_STRICT=1 FM1_DUMPRAM="$OUT/ram.bin" FM1_BENCH_KEYS="$KEYS" "$EMU" --fwsc "$PKG" --shot ${EMU_MS:-20000} > "$OUT/emu.log" 2>&1 || {
    cat "$OUT/emu.log"; echo "FAIL: the emulator stopped"; exit 1; }
cp work/panel.ppm "$OUT/panel.ppm" 2>/dev/null || true
python3 - "$OUT" "${STRESS:-0}" <<'PY'
import json, re, sys
from pathlib import Path
out = Path(sys.argv[1])
import struct
ram = (out / "ram.bin").read_bytes()
syms = {l.split()[-1]: int(l.split()[0], 16) for l in (out / "symbols.txt").read_text(errors="replace").splitlines()
        if l[:8].strip() and len(l.split()) >= 2 and all(c in "0123456789abcdef" for c in l.split()[0])}
def words(name, n):
    a = syms[name] - 0x01C00000
    return struct.unpack_from("<%dI" % n, ram, a)
last, worst, blocks, total = words("apsis_load_probe", 4)
PARTS = ["updates+Clock triggers", "String drone", "Drift", "Clock", "String notes", "planning",
         "triggers", "room+Motion", "Spiral+Bloom sound"]
parts = words("apsis_worst_probe", 9)
part_max = words("apsis_part_max_probe", 9)
part_at = words("apsis_part_max_block_probe", 9) if "apsis_part_max_block_probe" in syms else (0,) * 9
boot = words("apsis_boot_worst_probe", 1)[0] if "apsis_boot_worst_probe" in syms else 0
stress = sys.argv[2] == "1"
over = words("apsis_over_probe", 2) if "apsis_over_probe" in syms else (0, 0)
BUDGET = 5805
avg = total / blocks if blocks else 0
r = {"blocks": blocks, "avg_us": round(avg), "worst_us": worst, "last_us": last, "budget_us": BUDGET,
     "avg_pct": round(100 * avg / BUDGET, 1), "worst_pct": round(100 * worst / BUDGET, 1),
     "worst_parts_us": dict(zip(PARTS, parts)), "part_max_us": dict(zip(PARTS, part_max)), "blocks_over_80pct": over[0], "blocks_over_100pct": over[1],
     "part_max_at_s": dict(zip(PARTS, (round(b * 256 / 44100, 2) for b in part_at))), "boot_worst_us": boot,
     "bound_us": sum(part_max), "bound_pct": round(100 * sum(part_max) / BUDGET, 1), "stress": stress}
(out / "load.json").write_text(json.dumps(r, indent=1) + "\n")
print(f"audio load (FM1_STRICT, 20 s {'STRESS' if stress else 'scripted'}): avg {r['avg_us']} us ({r['avg_pct']} %), "
      f"worst {worst} us ({r['worst_pct']} %) of {BUDGET} us, {blocks} blocks")
print("worst block by part (us): " + ", ".join(f"{k} {v}" for k, v in zip(PARTS, parts)))
print("each part's own worst (us, at s): " + ", ".join(f"{k} {v} @{b * 256 / 44100:.1f}" for k, v, b in zip(PARTS, part_max, part_at)))
print(f"the first 16 blocks after boot (kept apart): worst {boot} us")
print(f"blocks over 80 %: {over[0]}, over 100 %: {over[1]}")
print(f"the bound (each part's own worst, added up): {r['bound_us']} us ({r['bound_pct']} %); "
      "docs/MARGIN_PLAN.md aims for 50 % or less")
if "apsis_voice_probe" in syms:
    vp = words("apsis_voice_probe", 4)
    r["voices"] = {"string_logical": vp[0], "string_physical": vp[1], "bloom_petals": vp[2]}
    (out / "load.json").write_text(json.dumps(r, indent=1) + "\n")
    print(f"voices high water: String notes {vp[0]} logical / {vp[1]} physical, Bloom petals {vp[2]}")
if "apsis_cost_probe" in syms:
    c = words("apsis_cost_probe", 8)
    r["voice_cost_ns"] = {k: {"ns_per_voice_block": c[2 * i], "voices": c[2 * i + 1]}
                          for i, k in enumerate(["String partial", "Spiral oscillator", "Bloom oscillator", "Clock voice"])}
    (out / "load.json").write_text(json.dumps(r, indent=1) + "\n")
    print("one voice's worst cost per block (us, at that many voices): " + ", ".join(
        f"{k} {v['ns_per_voice_block'] / 1000:.1f} ({v['voices']})" for k, v in r["voice_cost_ns"].items()))
if "apsis_bench_probe" in syms:
    b = words("apsis_bench_probe", 6)
    r["string_partial_bench"] = {"fixed_us_per_block": b[0], "cycles_per_partial_sample": {
        "decay mix": b[1] / 10, "sine": b[2] / 10, "triangle+LP": b[3] / 10, "attack+stagger": b[4] / 10,
        "steal fade": b[5] / 10}}
    (out / "load.json").write_text(json.dumps(r, indent=1) + "\n")
    print(f"String partial bench: fixed {b[0]} us a block; cycles per partial per sample: decay mix {b[1] / 10}, "
          f"sine {b[2] / 10}, triangle+LP {b[3] / 10}, attack+stagger {b[4] / 10}, steal fade {b[5] / 10}")
if blocks < 3000:
    sys.exit(f"FAIL: only {blocks} audio blocks in 20 s (expected ~3445): the audio stalled")
if worst >= BUDGET:
    sys.exit("FAIL: a block took longer than its 5.8 ms: the sound drops out")
if r["bound_us"] >= BUDGET:
    print("WARNING: the bound is over the budget: blocks where the parts' worsts meet would drop out")
if worst > BUDGET // 2:
    print("WARNING: the worst block is over half the budget (docs/dinghy/RULES.md); USB and the keys interrupt it too")
PY
