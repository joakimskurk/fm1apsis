#!/usr/bin/env python3
"""P11D read-only ELF/linker memory ownership inventory.

This script reports static ownership only. It deliberately does not infer
runtime stack high-water, ISR margin, LCD contention or any hardware timing.
"""

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path


def parse_sections(text: str):
    """Parse JieLi objdump -h: Idx Name Size Address Type..."""
    sections = []
    for raw in text.splitlines():
        parts = raw.split()
        if len(parts) < 4 or not parts[0].isdigit() or not parts[1].startswith('.'):
            continue
        try:
            size = int(parts[2], 16)
            address = int(parts[3], 16)
        except ValueError:
            continue
        sections.append({
            "name": parts[1],
            "size": size,
            "address": address,
            "type": " ".join(parts[4:]),
        })
    return sections


def parse_symbols(text: str):
    symbols = []
    hexword = re.compile(r"^[0-9A-Fa-f]+$")
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("SYMBOL TABLE"):
            continue
        parts = line.split()
        if len(parts) < 4 or not hexword.match(parts[0]):
            continue
        sec_i = None
        for i, tok in enumerate(parts[1:], 1):
            if tok.startswith(".") or tok in ("*ABS*", "*UND*", "*COM*"):
                sec_i = i
                break
        if sec_i is None or sec_i + 1 >= len(parts):
            continue
        size_tok = parts[sec_i + 1]
        if not hexword.match(size_tok):
            continue
        name = parts[-1]
        if name == parts[sec_i] or name.startswith(".debug"):
            continue
        symbols.append({
            "address": int(parts[0], 16),
            "section": parts[sec_i],
            "size": int(size_tok, 16),
            "name": name,
            "flags": " ".join(parts[1:sec_i]),
        })
    return symbols


def dedupe_symbols(symbols):
    """Avoid duplicate aliases at the exact same storage tuple."""
    chosen = {}
    for s in symbols:
        key = (s["section"], s["address"], s["size"])
        prev = chosen.get(key)
        if prev is None or (prev["name"].startswith(".") and not s["name"].startswith(".")):
            chosen[key] = s
    return list(chosen.values())


def is_ordinary_ram_section(name: str) -> bool:
    return (
        name == ".data" or name.startswith(".data.") or
        name == ".bss" or name.startswith(".bss.") or
        name == ".sdata" or name.startswith(".sdata.") or
        name == ".sbss" or name.startswith(".sbss.") or
        name == ".common" or name.startswith(".common.")
    )


def is_special_ram_section(name: str) -> bool:
    low = name.lower()
    return (
        "ram" in low or "pool" in low or "noinit" in low or
        "overlay" in low or "heap" in low
    ) and not is_ordinary_ram_section(name)


def top_symbols(symbols, predicate, limit=50):
    return sorted(
        [s for s in symbols if s["size"] > 0 and predicate(s["section"])],
        key=lambda s: (-s["size"], s["section"], s["name"]),
    )[:limit]


def interval_coverage_by_section(symbols, predicate):
    by_section = defaultdict(list)
    for s in symbols:
        if s["size"] > 0 and predicate(s["section"]):
            by_section[s["section"]].append((s["address"], s["address"] + s["size"]))

    coverage = {}
    for section, intervals in by_section.items():
        intervals.sort()
        merged = []
        for start, end in intervals:
            if not merged or start > merged[-1][1]:
                merged.append([start, end])
            elif end > merged[-1][1]:
                merged[-1][1] = end
        coverage[section] = sum(end - start for start, end in merged)
    return coverage


def stack_symbol_candidates(symbols):
    rx = re.compile(r"(?:^|[_.$])(stack|stk)(?:$|[_.$])|stack|stacksize|stack_size", re.I)
    return sorted(
        [s for s in symbols if rx.search(s["name"])],
        key=lambda s: (s["section"], s["address"], s["name"]),
    )


def linker_stack_lines(text: str):
    rx = re.compile(r"stack|\bstk\b|stack_size|stacksize|_stack|stack_|_guard|_ustack|_sstack", re.I)
    out = []
    for no, raw in enumerate(text.splitlines(), 1):
        if rx.search(raw):
            out.append({"line": no, "text": raw.rstrip()})
    return out


def find_abs(symbols, name: str):
    matches = [s for s in symbols if s["name"] == name and s["section"] == "*ABS*"]
    if len(matches) != 1:
        raise SystemExit(f"FAIL: expected one absolute symbol {name}, got {len(matches)}")
    return matches[0]["address"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sections", type=Path, required=True)
    ap.add_argument("--symbols", type=Path, required=True)
    ap.add_argument("--linker", type=Path, required=True)
    ap.add_argument("--target", type=Path, required=True)
    ap.add_argument("--out-json", type=Path, required=True)
    ap.add_argument("--out-md", type=Path, required=True)
    a = ap.parse_args()

    target = json.loads(a.target.read_text())
    sections = parse_sections(a.sections.read_text(errors="replace"))
    raw_symbols = parse_symbols(a.symbols.read_text(errors="replace"))
    symbols = dedupe_symbols(raw_symbols)
    linker_text = a.linker.read_text(errors="replace")

    if not sections:
        raise SystemExit("FAIL: no ELF sections parsed from JieLi objdump -h")
    if not symbols:
        raise SystemExit("FAIL: no ELF symbols parsed from objdump -t")

    section_by_name = {s["name"]: s for s in sections}
    data = section_by_name.get(".data")
    bss = section_by_name.get(".bss")
    if not data or not bss:
        raise SystemExit("FAIL: .data/.bss sections missing from real target ELF")
    if bss["address"] < data["address"] + data["size"]:
        raise SystemExit("FAIL: unexpected overlapping .data/.bss target layout")

    section_bytes = data["size"] + bss["size"]
    alignment_gap = bss["address"] - (data["address"] + data["size"])
    ram_span = bss["address"] + bss["size"] - data["address"]
    if ram_span != target["ram"]:
        raise SystemExit(
            f"FAIL: target RAM metric {target['ram']} != ELF .data→.bss-end span {ram_span}"
        )

    motion = [s for s in symbols if s["name"] == "apsis_string_motion_store"]
    if len(motion) != 1:
        raise SystemExit(f"FAIL: expected one apsis_string_motion_store symbol, got {len(motion)}")
    if motion[0]["section"] != ".apsis_motion_pool" or motion[0]["size"] != 9644:
        raise SystemExit(f"FAIL: Motion store ownership mismatch: {motion[0]}")

    ordinary = top_symbols(symbols, is_ordinary_ram_section)
    special = top_symbols(symbols, is_special_ram_section)
    stack_syms = stack_symbol_candidates(raw_symbols)
    stack_lines = linker_stack_lines(linker_text)

    interesting_sections = [
        s for s in sections
        if is_ordinary_ram_section(s["name"]) or is_special_ram_section(s["name"])
    ]
    interesting_sections.sort(key=lambda s: (-s["size"], s["name"]))

    coverage_by_section = interval_coverage_by_section(symbols, is_ordinary_ram_section)
    covered_ordinary = sum(coverage_by_section.values())
    symbol_coverage_gap = target["ram"] - covered_ordinary

    by_section_naive = defaultdict(int)
    for s in symbols:
        if s["size"] > 0:
            by_section_naive[s["section"]] += s["size"]

    ustack_lo = find_abs(raw_symbols, "_ustack_lo")
    ustack_top = find_abs(raw_symbols, "_ustack_top")
    sstack_lo = find_abs(raw_symbols, "_sstack_lo")
    sstack_top = find_abs(raw_symbols, "_sstack_top")
    guard0 = find_abs(raw_symbols, "_guard0")
    guard1 = find_abs(raw_symbols, "_guard1")
    if not (guard0 <= ustack_lo < ustack_top == guard1 <= sstack_lo < sstack_top):
        raise SystemExit("FAIL: unexpected configured stack/guard ordering")

    stack_static = {
        "ustack_lo": ustack_lo,
        "ustack_top": ustack_top,
        "ustack_configured_bytes": ustack_top - ustack_lo,
        "guard0": guard0,
        "guard0_to_ustack_bytes": ustack_lo - guard0,
        "guard1": guard1,
        "guard1_to_sstack_bytes": sstack_lo - guard1,
        "sstack_lo": sstack_lo,
        "sstack_top": sstack_top,
        "sstack_configured_bytes": sstack_top - sstack_lo,
        "symbol_candidates": stack_syms,
        "linker_lines": stack_lines,
        "interpretation": (
            "Configured/static stack ranges only. These values do not measure runtime "
            "stack high-water and do not prove physical FM-1 stack headroom."
        ),
    }

    report = {
        "phase": "P11D",
        "scope": "static target ELF/linker ownership inventory only",
        "target": target,
        "elf": {
            "section_count": len(sections),
            "parsed_symbol_count_raw": len(raw_symbols),
            "parsed_storage_tuples": len(symbols),
            "interesting_sections": interesting_sections,
            "naive_symbol_bytes_by_section": dict(sorted(by_section_naive.items())),
        },
        "motion_store": motion[0],
        "ordinary_ram": {
            "target_report_data_plus_bss_span_bytes": target["ram"],
            "data_section_bytes": data["size"],
            "bss_section_bytes": bss["size"],
            "section_payload_bytes": section_bytes,
            "data_bss_alignment_gap_bytes": alignment_gap,
            "data_start_to_bss_end_span_bytes": ram_span,
            "covered_unique_symbol_intervals_bytes": covered_ordinary,
            "covered_unique_symbol_intervals_by_section": dict(sorted(coverage_by_section.items())),
            "symbol_coverage_gap_vs_target_span_bytes": symbol_coverage_gap,
            "largest_symbols": ordinary,
        },
        "special_ram": {
            "largest_symbols": special,
            "ram_text_section": section_by_name.get(".ram_text"),
            "apsis_motion_pool_section": section_by_name.get(".apsis_motion_pool"),
            "apsis_space_pool_section": section_by_name.get(".apsis_space_pool"),
            "pool_section": section_by_name.get(".pool"),
            "noinit_section": section_by_name.get(".noinit"),
        },
        "stack_static_inventory": stack_static,
        "not_proven": [
            "runtime stack high-water",
            "physical app_audio ISR/callback cycle margin",
            "physical LCD DMA contention",
            "physical NOR timing/power-interruption behavior",
            "DAC/listening quality",
        ],
    }

    a.out_json.parent.mkdir(parents=True, exist_ok=True)
    a.out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    md = [
        "# APSIS Phase 11D target memory ownership inventory",
        "",
        "Read-only inventory from the real JieLi production ELF/linker output. No firmware memory size is changed by P11D.",
        "",
        "## Aggregate P11C target baseline",
        "",
        f"- image: **{target['image']:,} / {target['image_max']:,} B**",
        f"- ordinary RAM (`.data` start → `.bss` end build span): **{target['ram']:,} / {target['ram_max']:,} B**",
        f"- POOL: **{target['pool']:,} / {target['pool_max']:,} B**",
        f"- package SHA-256 `{target['package_sha256']}`",
        "",
        "## Ordinary RAM accounting",
        "",
        f"- `.data` payload: **{data['size']:,} B**",
        f"- `.bss` payload: **{bss['size']:,} B**",
        f"- linker alignment gap between them: **{alignment_gap:,} B**",
        f"- `.data` start → `.bss` end span: **{ram_span:,} B** (exactly the Dinghy RAM metric)",
        f"- unique covered sized-symbol intervals: **{covered_ordinary:,} B**",
        f"- remaining span not directly attributed to sized symbol intervals: **{symbol_coverage_gap:,} B**",
        "",
        "## Largest ordinary RAM symbols",
        "",
        "| Bytes | Section | Symbol |",
        "|---:|---|---|",
    ]
    for s in ordinary[:30]:
        md.append(f"| {s['size']:,} | `{s['section']}` | `{s['name']}` |")
    if not ordinary:
        md.append("| 0 | — | no sized ordinary-RAM symbols parsed |")

    md += [
        "",
        "## Special / dedicated RAM ownership",
        "",
        "| Bytes | Section | Symbol |",
        "|---:|---|---|",
    ]
    for s in special[:30]:
        md.append(f"| {s['size']:,} | `{s['section']}` | `{s['name']}` |")
    if not special:
        md.append("| 0 | — | no sized special-RAM symbols parsed |")

    md += [
        "",
        "## Section inventory",
        "",
        "| Bytes | Section | Address | Type |",
        "|---:|---|---:|---|",
    ]
    for s in interesting_sections:
        md.append(f"| {s['size']:,} | `{s['name']}` | `0x{s['address']:08x}` | {s['type'] or '—'} |")

    md += [
        "",
        "## Configured stack ranges — static only",
        "",
        f"- user stack `_ustack_lo.._ustack_top`: **{stack_static['ustack_configured_bytes']:,} B** configured",
        f"- user-stack guard before `_ustack_lo`: **{stack_static['guard0_to_ustack_bytes']:,} B**",
        f"- system stack `_sstack_lo.._sstack_top`: **{stack_static['sstack_configured_bytes']:,} B** configured",
        f"- system-stack guard before `_sstack_lo`: **{stack_static['guard1_to_sstack_bytes']:,} B**",
        f"- stack-named ELF symbol candidates: **{len(stack_syms)}**",
        f"- stack/guard-related linker-script lines: **{len(stack_lines)}**",
        "",
        "> These are configured/static address ranges. They are **not runtime stack high-water** and do not establish physical stack headroom.",
        "",
        "## Attribution note",
        "",
        "The report uses merged address intervals rather than naively summing symbol sizes, because linker aliases and merged globals can overlap. "
        "The remaining symbol-coverage gap can contain padding, linker-generated bytes, partially named aggregate storage, or other bytes not represented by unique sized symbols. It is not free RAM.",
        "",
        "## Not proven by this inventory",
        "",
    ]
    md.extend(f"- {x}" for x in report["not_proven"])
    md.append("")
    a.out_md.write_text("\n".join(md))

    print(json.dumps({
        "motion_store": motion[0],
        "ordinary_ram_top5": ordinary[:5],
        "special_ram_top5": special[:5],
        "ordinary_symbol_interval_coverage": covered_ordinary,
        "ordinary_symbol_coverage_gap": symbol_coverage_gap,
        "data_bss_alignment_gap": alignment_gap,
        "ustack_configured_bytes": stack_static["ustack_configured_bytes"],
        "sstack_configured_bytes": stack_static["sstack_configured_bytes"],
        "target_ram": target["ram"],
    }, sort_keys=True))
    print("PASS: P11D static target memory ownership inventory generated; no runtime stack claim")


if __name__ == "__main__":
    main()
