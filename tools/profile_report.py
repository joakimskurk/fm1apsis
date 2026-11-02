#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Where the cycles go: fm1-live's FM1_TPROF samples (time-weighted PCs, flash-cache misses) per function.

    python3 tools/profile_report.py TPROF_FILE SYMBOLS_FILE [TOP]

SYMBOLS_FILE is `objdump -t apsis.elf`. Prints each function's share of core 0's cycles and its flash-cache
misses, most expensive first. In a unity build static functions inline into their callers: a big share in
`app_audio` is the inlined render graph.
"""
import bisect
import re
import sys
from collections import defaultdict


def symbols(path):
    funcs = []
    for line in open(path, errors="replace"):
        m = re.match(r"^([0-9a-f]{8})\s.*\s(?:F|\.text|\.ram_text)\s+\S+\s+([0-9a-f]{8})\s+(\S+)$", line)
        if not m:
            m = re.match(r"^([0-9a-f]{8})\s+\S*\s+F\s+\S+\s+([0-9a-f]{8})\s+(\S+)$", line)
        if m and int(m.group(2), 16) > 0:
            funcs.append((int(m.group(1), 16), int(m.group(2), 16), m.group(3)))
    funcs.sort()
    return funcs


def main():
    tprof, syms = sys.argv[1], sys.argv[2]
    top = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    funcs = symbols(syms)
    starts = [f[0] for f in funcs]

    def name(pc):
        i = bisect.bisect_right(starts, pc) - 1
        if i >= 0 and pc < funcs[i][0] + funcs[i][1]:
            return funcs[i][2]
        return f"?{pc:#010x}"

    cyc, miss = defaultdict(int), defaultdict(int)
    total = 0
    for line in open(tprof):
        kind, addr, n = line.split()
        a, n = int(addr, 16), int(n)
        if kind == "0":
            cyc[name(a)] += n
            total += n
        elif kind.startswith("m"):
            miss[name(a)] += n
    print(f"{'share':>6}  {'misses':>8}  function   ({total} samples, core 0)")
    for f, n in sorted(cyc.items(), key=lambda kv: -kv[1])[:top]:
        print(f"{100 * n / total:5.1f}%  {miss.get(f, 0):8d}  {f}")


if __name__ == "__main__":
    main()
