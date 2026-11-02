#!/usr/bin/env python3
import argparse
import json
import re
from collections import Counter
from pathlib import Path

HEADER_GNU = re.compile(r'^\s*[0-9a-fA-F]+\s+<([^>]+)>:\s*$')
HEADER_JIELI = re.compile(r'^([A-Za-z_.$][A-Za-z0-9_.$@-]*):\s*$')
INSN = re.compile(r'^\s*[0-9a-fA-F]+:\s+(.*)$')
HEX = re.compile(r'^[0-9a-fA-F]{2,8}$')


def extract_function_lines(text: str, target: str):
    current = None
    lines = []
    for line in text.splitlines():
        m = HEADER_GNU.match(line) or HEADER_JIELI.match(line)
        if m:
            current = m.group(1)
            continue
        if current == target and INSN.match(line):
            lines.append(line)
    if not lines:
        raise SystemExit(f'FAIL: no disassembly lines for {target}')
    return lines


def mnemonic(line: str):
    m = INSN.match(line)
    if not m:
        return None
    tokens = m.group(1).strip().split()
    for tok in tokens:
        clean = tok.rstrip(':')
        if HEX.fullmatch(clean):
            continue
        # Some objdump variants join bytes into a wider all-hex word.
        if re.fullmatch(r'[0-9a-fA-F]{8,16}', clean):
            continue
        return clean.lower()
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--disassembly', type=Path, required=True)
    ap.add_argument('--out-json', type=Path, required=True)
    ap.add_argument('--out-text', type=Path, required=True)
    args = ap.parse_args()

    text = args.disassembly.read_text(errors='replace')
    lines = extract_function_lines(text, 'apsis_space_fdn_render')
    opcodes = Counter()
    unknown = 0
    for line in lines:
        op = mnemonic(line)
        if op is None:
            unknown += 1
        else:
            opcodes[op] += 1

    multiply_like = {k: v for k, v in sorted(opcodes.items())
                     if 'mul' in k or 'mac' in k or 'mla' in k}
    report = {
        'function': 'apsis_space_fdn_render',
        'instruction_lines': len(lines),
        'parsed_opcode_lines': sum(opcodes.values()),
        'unknown_lines': unknown,
        'opcode_histogram': dict(opcodes.most_common()),
        'multiply_like_opcodes': multiply_like,
        'physical_timing_proven': False,
    }
    if report['parsed_opcode_lines'] < len(lines) * 9 // 10:
        raise SystemExit(f"FAIL: opcode parser coverage too low: {report}")

    args.out_json.parent.mkdir(parents=True, exist_ok=True)
    args.out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    args.out_text.write_text('\n'.join(lines) + '\n')
    print(json.dumps(report, sort_keys=True))
    print('PASS: P11X real-target FDN render opcode inventory captured')


if __name__ == '__main__':
    main()
