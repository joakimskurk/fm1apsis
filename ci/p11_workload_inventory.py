#!/usr/bin/env python3
"""P11G read-only workload proxy inventory.

The report intentionally distinguishes deterministic static proxies from physical
FM-1 timing. It never converts instruction counts or emulator wall time into ISR
headroom or CPU percentages.
"""

from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path

KEYWORDS = {
    "if", "for", "while", "switch", "return", "sizeof", "do", "case",
}

AUDIO_REQUIRED = {
    "apsis_string_dsp_render",
    "apsis_string_transient_render",
    "apsis_drift_dsp_render",
    "apsis_clock_dsp_render",
    "apsis_spiral_dsp_render",
    "apsis_bloom_dsp_render",
    "apsis_space_target_render",
}

AUDIO_FORBIDDEN_PREFIXES = (
    "apsis_visual_", "apsis_controls_", "draw_", "cv_", "lcd_", "fm1_lcd_",
    "fm1_input", "platform_poll",
)

STATIC_FOCUS = [
    "apsis_string_dsp_render",
    "apsis_string_transient_render",
    "apsis_drift_dsp_render",
    "apsis_clock_dsp_render",
    "apsis_spiral_dsp_render",
    "apsis_bloom_dsp_render",
    "apsis_space_target_render",
    "apsis_space_room_render",
    "apsis_space_fdn_render",
    "apsis_runtime_process",
    "apsis_runtime_process_attractor",
    "apsis_runtime_process_spiral_audio",
    "apsis_runtime_process_bloom_audio",
]


def strip_c_noise(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    text = re.sub(r'"(?:\\.|[^"\\])*"', '""', text)
    text = re.sub(r"'(?:\\.|[^'\\])*'", "''", text)
    return text


def extract_function(text: str, name: str) -> str:
    # Function definitions only; prototypes terminate with ';' before a body.
    m = re.search(r"\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.S)
    if not m:
        raise SystemExit(f"FAIL: generated target source has no {name}() body")
    start = text.find("{", m.start())
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[m.start(): i + 1]
    raise SystemExit(f"FAIL: unterminated {name}() body")


def lexical_calls(body: str) -> dict[str, int]:
    clean = strip_c_noise(body)
    counts: dict[str, int] = {}
    for name in re.findall(r"\b([A-Za-z_]\w*)\s*\(", clean):
        if name in KEYWORDS:
            continue
        counts[name] = counts.get(name, 0) + 1
    return dict(sorted(counts.items()))


def parse_symbols(text: str) -> dict[str, int]:
    out: dict[str, int] = {}
    # Typical JieLi objdump -t:
    # 02004154 g     F .text  00000382 apsis_string_dsp_render
    rx = re.compile(
        r"^\s*[0-9a-fA-F]+\s+\S+\s+F\s+\.text\s+([0-9a-fA-F]+)\s+(\S+)\s*$",
        re.M,
    )
    for size_hex, name in rx.findall(text):
        out[name] = int(size_hex, 16)
    if not out:
        raise SystemExit("FAIL: could not parse any .text function symbols")
    return out


def parse_disassembly(text: str) -> dict[str, int]:
    counts: dict[str, int] = {}
    current: str | None = None
    header = re.compile(r"^\s*[0-9a-fA-F]+\s+<([^>]+)>:\s*$")
    insn = re.compile(r"^\s*[0-9a-fA-F]+:\s+")
    for line in text.splitlines():
        m = header.match(line)
        if m:
            current = m.group(1)
            counts.setdefault(current, 0)
            continue
        if current is not None and insn.match(line):
            counts[current] += 1
    if not counts or max(counts.values(), default=0) == 0:
        raise SystemExit("FAIL: could not parse target disassembly instruction lines")
    return counts


def define_u32(text: str, name: str) -> int:
    m = re.search(r"^\s*#define\s+" + re.escape(name) + r"\s+(\d+)u?\b", text, re.M)
    if not m:
        raise SystemExit(f"FAIL: missing numeric define {name}")
    return int(m.group(1))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dinghy", type=Path, required=True)
    ap.add_argument("--visual-header", type=Path, required=True)
    ap.add_argument("--symbols", type=Path, required=True)
    ap.add_argument("--disassembly", type=Path, required=True)
    ap.add_argument("--target", type=Path, required=True)
    ap.add_argument("--out-json", type=Path, required=True)
    ap.add_argument("--out-md", type=Path, required=True)
    a = ap.parse_args()

    dinghy = a.dinghy.read_text(errors="replace")
    visual_h = a.visual_header.read_text(errors="replace")
    symbols = parse_symbols(a.symbols.read_text(errors="replace"))
    insns = parse_disassembly(a.disassembly.read_text(errors="replace"))
    target = json.loads(a.target.read_text())

    audio = extract_function(dinghy, "app_audio")
    audio_calls = lexical_calls(audio)
    missing = sorted(AUDIO_REQUIRED - set(audio_calls))
    if missing:
        raise SystemExit(f"FAIL: generated app_audio lost expected final render calls: {missing}")
    forbidden = sorted(
        name for name in audio_calls
        if any(name.startswith(prefix) for prefix in AUDIO_FORBIDDEN_PREFIXES)
    )
    if forbidden:
        raise SystemExit(f"FAIL: UI/control/display call leaked into app_audio: {forbidden}")

    draw_step = extract_function(dinghy, "draw_native_visual_step")
    draw_calls = lexical_calls(draw_step)
    for required in ("apsis_visual_render_p8c", "cv_begin", "cv_blit", "apsis_lcd_dma_ready"):
        if required not in draw_calls:
            raise SystemExit(f"FAIL: native draw step lost {required}()")
    lcd_ready = extract_function(dinghy, "apsis_lcd_dma_ready")
    if "fm1_lcd_done" not in lexical_calls(lcd_ready):
        raise SystemExit("FAIL: native visual path is no longer HAL-gated by fm1_lcd_done()")

    strip_rows = define_u32(dinghy, "APSIS_NATIVE_STRIP_ROWS")
    start_sample = define_u32(dinghy, "APSIS_NATIVE_START_SAMPLE")
    width = define_u32(visual_h, "APSIS_VISUAL_WIDTH")
    height = define_u32(visual_h, "APSIS_VISUAL_HEIGHT")
    if strip_rows <= 0 or strip_rows > height:
        raise SystemExit(f"FAIL: nonsensical native strip rows {strip_rows}")
    strips = math.ceil(height / strip_rows)
    max_strip_pixels = width * strip_rows
    max_strip_payload_bytes = max_strip_pixels * 2
    full_payload_bytes = width * height * 2

    apsis_sizes = {k: v for k, v in symbols.items() if k.startswith("apsis_")}
    apsis_insns = {k: v for k, v in insns.items() if k.startswith("apsis_")}
    if len(apsis_sizes) < 20 or len(apsis_insns) < 20:
        raise SystemExit(
            f"FAIL: target census too small: symbols={len(apsis_sizes)} disassembled={len(apsis_insns)}"
        )

    focus = {}
    for name in STATIC_FOCUS:
        if name in symbols or name in insns:
            focus[name] = {
                "text_bytes": symbols.get(name),
                "static_instruction_lines": insns.get(name),
            }
    required_focus = AUDIO_REQUIRED | {
        "apsis_runtime_process_spiral_audio", "apsis_runtime_process_bloom_audio"
    }
    absent_focus = sorted(name for name in required_focus if name not in focus)
    if absent_focus:
        raise SystemExit(
            f"FAIL: target static census lost expected audio/runtime functions: {absent_focus}"
        )

    ranked = sorted(
        (
            {
                "name": name,
                "static_instruction_lines": count,
                "text_bytes": symbols.get(name),
            }
            for name, count in apsis_insns.items()
        ),
        key=lambda x: (x["static_instruction_lines"], x["text_bytes"] or 0, x["name"]),
        reverse=True,
    )[:25]

    report = {
        "phase": "P11G",
        "scope": "read-only deterministic target workload proxy inventory",
        "physical_timing_proven": False,
        "target": target,
        "audio_source_proxy": {
            "app_audio_source_lines": len(audio.splitlines()),
            "lexical_calls": audio_calls,
            "forbidden_ui_control_calls": forbidden,
        },
        "display_proxy": {
            "width": width,
            "height": height,
            "strip_rows": strip_rows,
            "strips_per_full_redraw": strips,
            "max_pixels_per_strip": max_strip_pixels,
            "max_pixel_payload_bytes_per_strip": max_strip_payload_bytes,
            "pixel_payload_bytes_per_full_redraw": full_payload_bytes,
            "native_takeover_sample": start_sample,
            "hal_completion_gate": "fm1_lcd_done",
            "one_cv_blit_call_in_step": draw_calls.get("cv_blit", 0) == 1,
        },
        "target_static_proxy": {
            "apsis_text_function_count": len(apsis_sizes),
            "apsis_disassembled_function_count": len(apsis_insns),
            "focus": focus,
            "top_25_by_static_instruction_lines": ranked,
            "warning": (
                "Static instruction-line and text-size counts are compiler/code-shape proxies only. "
                "They do not represent dynamic instruction counts, cycles, cache effects or ISR headroom."
            ),
        },
        "unproven": [
            "physical FM-1 audio ISR execution time/headroom",
            "runtime stack high-water",
            "LCD/SPI bus contention with audio on hardware",
            "cache/memory wait-state effects",
            "DAC/listening quality",
        ],
    }

    a.out_json.parent.mkdir(parents=True, exist_ok=True)
    a.out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    rows = []
    for name in STATIC_FOCUS:
        item = focus.get(name)
        if not item:
            continue
        rows.append(
            f"| `{name}` | {item['text_bytes'] if item['text_bytes'] is not None else 'n/a'} | "
            f"{item['static_instruction_lines'] if item['static_instruction_lines'] is not None else 'n/a'} |"
        )

    md = [
        "# APSIS Phase 11G workload proxy inventory",
        "",
        "P11G changes no production firmware behavior or capacity. This report is a deterministic source/target-code inventory, **not a physical timing measurement**.",
        "",
        "## Audio callback boundary",
        "",
        f"Generated `app_audio()` contains {len(audio.splitlines())} source lines and {sum(audio_calls.values())} lexical call sites ({len(audio_calls)} unique call names).",
        "",
        "No visual, control-dispatch, LCD, input-poll or canvas call is present in `app_audio()`.",
        "",
        "## Native display workload boundary",
        "",
        f"- native strip height: **{strip_rows} rows**;",
        f"- display: **{width}×{height}** RGB565;",
        f"- full redraw: **{strips} strips**;",
        f"- maximum pixel payload per strip: **{max_strip_payload_bytes:,} B**;",
        f"- pixel payload per complete redraw: **{full_payload_bytes:,} B** (SPI/protocol overhead excluded);",
        f"- native takeover remains sample **{start_sample}**;",
        "- every native strip is gated through `apsis_lcd_dma_ready()` / `fm1_lcd_done()` and performs one `cv_blit()` call.",
        "",
        "The pixel payload is a deterministic work-volume proxy. It is not a measured LCD bus duration.",
        "",
        "## JieLi target static audio/runtime proxy",
        "",
        "| Function | `.text` bytes | static disassembly instruction lines |",
        "|---|---:|---:|",
        *rows,
        "",
        "Largest APSIS functions by static instruction-line count are preserved in `workload_inventory.json` together with the complete lexical `app_audio()` call census.",
        "",
        "These counts describe compiler-emitted code shape only. Loop trip counts, active voice counts, cache behavior, memory waits and hardware ISR scheduling are not encoded by the static count.",
        "",
        "## Decision boundary",
        "",
        "P11G may use this inventory to select a later falsifiable optimisation target, but it does **not** authorize an optimisation by itself. A candidate must first be tied to a dynamic workload scenario or an independently bounded repeated operation.",
        "",
        "Still unproven without physical hardware: ISR headroom, runtime stack high-water, LCD/audio bus contention, memory wait-state effects and DAC/listening quality.",
        "",
    ]
    a.out_md.write_text("\n".join(md))

    print(json.dumps({
        "audio_unique_calls": len(audio_calls),
        "audio_call_sites": sum(audio_calls.values()),
        "display_strips": strips,
        "display_full_payload_bytes": full_payload_bytes,
        "apsis_text_functions": len(apsis_sizes),
        "apsis_disassembled_functions": len(apsis_insns),
    }, sort_keys=True))
    print("PASS: P11G deterministic workload proxy inventory generated; no physical timing claim")


if __name__ == "__main__":
    main()
