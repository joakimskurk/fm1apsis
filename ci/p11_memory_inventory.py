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
    sections = []
    rx = re.compile(
        r"^\s*\d+\s+(\S+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)",
        re.M,
    )
    for m in rx.finditer(text):
        name, size, vma, lma, off = m.groups()
        sections.append({
            "name": name,
            "size": int(size, 16),
            "vma": int(vma, 16),
            "lma": int(lma, 16),
            "file_offset": int(off, 16),
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
    """Avoid double-counting aliases at the exact same storage tuple."""
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


def stack_symbol_candidates(symbols):
    rx = re.compile(r"(?:^|[_.$])(stack|stk)(?:$|[_.$])|stack|stacksize|stack_size", re.I)
    return sorted(
        [s for s in symbols if rx.search(s["name"])],
        key=lambda s: (s["section"], s["address"], s["name"]),
    )


def linker_stack_lines(text: str):
    rx = re.compile(r"stack|\bstk\b|stack_size|stacksize|_stack|stack_", re.I)
    out = []
    for no, raw in enumerate(text.splitlines(), 1):
        if rx.search(raw):
            out.append({"line": no, "text": raw.rstrip()})
    return out


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
        raise SystemExit("FAIL: no ELF sections parsed from objdump -h")
    if not symbols:
        raise SystemExit("FAIL: no ELF symbols parsed from objdump -t")

    motion = [s for s in symbols if s["name"] == "apsis_string_motion_store"]
    if len(motion) != 1:
        raise SystemExit(f"FAIL: expected one apsis_string_motion_store symbol, got {len(motion)}")
    if motion[0]["section"] != ".apsis_motion_pool" or motion[0]["size"] != 9644:
        raise SystemExit(f"FAIL: Motion store ownership mismatch: {motion[0]}")

    ordinary = top_symbols(symbols, is_ordinary_ram_section)
    special = top_symbols(symbols, is_special_ram_section)
    stack_syms = stack_symbol_candidates(symbols)
    stack_lines = linker_stack_lines(linker_text)

    section_by_name = {s["name"]: s for s in sections}
    interesting_sections = [
        s for s in sections
        if is_ordinary_ram_section(s["name"]) or is_special_ram_section(s["name"])
    ]
    interesting_sections.sort(key=lambda s: (-s["size"], s["name"]))

    unique_ordinary = [
        s for s in symbols if s["size"] > 0 and is_ordinary_ram_section(s["section"])
    ]
    attributed_ordinary = sum(s["size"] for s in unique_ordinary)

    by_section = defaultdict(int)
    for s in symbols:
        if s["size"] > 0:
            by_section[s["section"]] += s["size"]

    report = {
        "phase": "P11D",
        "scope": "static target ELF/linker ownership inventory only",
        "target": target,
        "elf": {
            "section_count": len(sections),
            "parsed_symbol_count_raw": len(raw_symbols),
            "parsed_storage_tuples": len(symbols),
            "interesting_sections": interesting_sections,
            "symbol_bytes_by_section": dict(sorted(by_section.items())),
        },
        "motion_store": motion[0],
        "ordinary_ram": {
            "target_report_data_plus_bss_bytes": target["ram"],
            "attributed_unique_symbol_bytes": attributed_ordinary,
            "attribution_delta_vs_target_report": target["ram"] - attributed_ordinary,
            "largest_symbols": ordinary,
        },
        "special_ram": {
            "largest_symbols": special,
            "apsis_motion_pool_section": section_by_name.get(".apsis_motion_pool"),
            "apsis_space_pool_section": section_by_name.get(".apsis_space_pool"),
            "pool_section": section_by_name.get(".pool"),
        },
        "stack_static_inventory": {
            "symbol_candidates": stack_syms,
            "linker_lines": stack_lines,
            "interpretation": (
                "These are static/configured stack clues only. They do not measure runtime "
                "stack high-water or prove physical FM-1 stack headroom."
            ),
        },
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
        f"- ordinary RAM (`.data + .bss` build metric): **{target['ram']:,} / {target['ram_max']:,} B**",
        f"- POOL: **{target['pool']:,} / {target['pool_max']:,} B**",
        f"- package SHA-256 `{target['package_sha256']}`",
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
        "| Bytes | Section | VMA |",
        "|---:|---|---:|",
    ]
    for s in interesting_sections:
        md.append(f"| {s['size']:,} | `{s['name']}` | `0x{s['vma']:08x}` |")

    md += [
        "",
        "## Static stack clues",
        "",
        f"- stack-named ELF symbol candidates: **{len(stack_syms)}**",
        f"- stack-related linker-script lines: **{len(stack_lines)}**",
    ]
    for s in stack_syms[:20]:
        md.append(
            f"- symbol `{s['name']}`: section `{s['section']}`, address `0x{s['address']:x}`, size {s['size']:,} B"
        )
    for item in stack_lines[:20]:
        md.append(f"- linker line {item['line']}: `{item['text'].strip()}`")

    md += [
        "",
        "> Static stack symbols/reservations are not runtime stack high-water. P11D makes no physical stack-margin claim.",
        "",
        "## Attribution note",
        "",
        f"Unique sized ordinary-RAM symbols account for **{attributed_ordinary:,} B** versus the build metric of **{target['ram']:,} B**. "
        "Any delta can include padding, linker-generated storage, aliases omitted by de-duplication, or sections counted differently by the build summary; it is evidence for follow-up, not free RAM.",
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
        "stack_symbol_candidates": len(stack_syms),
        "stack_linker_lines": len(stack_lines),
        "attributed_ordinary": attributed_ordinary,
        "target_ram": target["ram"],
    }, sort_keys=True))
    print("PASS: P11D static target memory ownership inventory generated; no runtime stack claim")


if __name__ == "__main__":
    main()
