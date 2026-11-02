#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p11_workload_inventory as inv


def function_lines(text: str, name: str) -> list[str]:
    lines = text.splitlines()
    gnu = re.compile(r"^\s*[0-9a-fA-F]+\s+<" + re.escape(name) + r">:\s*$")
    jieli = re.compile(r"^" + re.escape(name) + r":\s*$")
    any_gnu = re.compile(r"^\s*[0-9a-fA-F]+\s+<[^>]+>:\s*$")
    any_jieli = re.compile(r"^[A-Za-z_.$][A-Za-z0-9_.$@-]*:\s*$")
    start = None
    for i, line in enumerate(lines):
        if gnu.match(line) or jieli.match(line):
            start = i + 1
            break
    if start is None:
        return []
    out = []
    for line in lines[start:]:
        if any_gnu.match(line) or any_jieli.match(line):
            break
        if re.match(r"^\s*[0-9a-fA-F]+:\s+", line):
            out.append(line)
    return out


def opcode(line: str) -> str | None:
    # JieLi/GNU variants differ in byte grouping. Strip address, then consume
    # leading hexadecimal byte/word fields until the first non-hex token.
    m = re.match(r"^\s*[0-9a-fA-F]+:\s+(.*)$", line)
    if not m:
        return None
    toks = m.group(1).split()
    i = 0
    while i < len(toks) and re.fullmatch(r"[0-9a-fA-F]{2,8}", toks[i]):
        i += 1
    if i >= len(toks):
        return None
    return toks[i]


def main() -> None:
    if len(sys.argv) != 5:
        raise SystemExit("usage: p11u_target_residual_inventory.py SYMBOLS DISASM TARGET_JSON OUT_JSON")
    symbols_text = Path(sys.argv[1]).read_text(errors="replace")
    disasm_text = Path(sys.argv[2]).read_text(errors="replace")
    target = json.loads(Path(sys.argv[3]).read_text())
    symbols = inv.parse_symbols(symbols_text)
    insns = inv.parse_disassembly(disasm_text)

    helpers = [
        "apsis_space_fdn_render",
        "space_mul_q15",
        "space_sine_point",
        "space_feedback_gain",
        "space_line_read",
        "space_hadamard8",
        "space_clamp_q15",
        "space_damping_alpha",
    ]
    helper_shape = {
        n: {"text_bytes": symbols.get(n), "static_instruction_lines": insns.get(n)}
        for n in helpers
    }

    render = function_lines(disasm_text, "apsis_space_fdn_render")
    if len(render) != 366:
        raise SystemExit(f"FAIL: P11R render instruction extraction {len(render)} != 366")
    hist: dict[str, int] = {}
    for line in render:
        op = opcode(line)
        if op:
            hist[op] = hist.get(op, 0) + 1
    top_ops = sorted(hist.items(), key=lambda kv: (-kv[1], kv[0]))[:20]

    call_refs = {
        name: sum(1 for line in render if name in line)
        for name in helpers[1:]
    }
    mul_like = [line.strip() for line in render if re.search(r"\bmul", line, re.I)]
    div_like = [line.strip() for line in render if re.search(r"\bdiv", line, re.I)]
    call_like = [line.strip() for line in render if re.search(r"\b(call|bl|jal)\b", line, re.I)]

    assert symbols.get("apsis_space_fdn_render") == 1098
    assert insns.get("apsis_space_fdn_render") == 366
    assert symbols.get("space_mul_q15") == 88
    assert insns.get("space_mul_q15") == 27
    assert symbols.get("space_sine_point") == 48
    assert insns.get("space_sine_point") == 14

    report = {
        "phase": "P11U",
        "physical_timing_proven": False,
        "target": target,
        "helper_shape": helper_shape,
        "render": {
            "text_bytes": symbols.get("apsis_space_fdn_render"),
            "static_instruction_lines": len(render),
            "top_mnemonics": top_ops,
            "symbol_references": call_refs,
            "mul_like_line_count": len(mul_like),
            "div_like_line_count": len(div_like),
            "call_like_line_count": len(call_like),
            "mul_like_lines": mul_like,
            "call_like_lines": call_like,
        },
    }
    Path(sys.argv[4]).write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    print("P11U target helper_shape", json.dumps(helper_shape, sort_keys=True))
    print("P11U target render_top_mnemonics", json.dumps(top_ops))
    print("P11U target render_symbol_references", json.dumps(call_refs, sort_keys=True))
    print("P11U target render_mul_like_line_count=", len(mul_like))
    print("P11U target render_call_like_line_count=", len(call_like))
    print("P11U target render_div_like_line_count=", len(div_like))
    for line in mul_like:
        print("P11U MUL", line)
    for line in call_like:
        print("P11U CALL", line)
    print("PASS: P11U exact P11R residual target code-shape inventory")


if __name__ == "__main__":
    main()
