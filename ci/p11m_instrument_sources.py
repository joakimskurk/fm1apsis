#!/usr/bin/env python3
from pathlib import Path
import argparse


def replace_once(src: str, old: str, new: str, label: str) -> str:
    if src.count(old) != 1:
        raise SystemExit(f'P11M {label} anchor count={src.count(old)}')
    return src.replace(old, new, 1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    src = Path('apsis/apsis_space_dsp.c').read_text()
    src = replace_once(
        src,
        '#include "apsis_space_dsp.h"\n',
        '#include "apsis_space_dsp.h"\n#include "p11m_probe.h"\n',
        'include')

    src = replace_once(
        src,
        '''    uint8_t i = (uint8_t)(phase >> 24);\n    uint8_t frac = (uint8_t)(phase >> 16);\n''',
        '''    uint8_t i = (uint8_t)(phase >> 24);\n    uint8_t frac = (uint8_t)(phase >> 16);\n    P11M_PROBE_SINE(frac);\n''',
        'sine')

    src = replace_once(
        src,
        '''    if (!fdn || !line || !fdn->storage_bound) return 0;\n    delay_q8 = (int32_t)line->base_delay * 256 + modulation_q8;\n    if (delay_q8 < 256) delay_q8 = 256;\n    needed = (uint32_t)(delay_q8 + 255) >> 8;\n    if (line->fill_count < needed) return 0;\n\n    span_q8 = (int32_t)line->capacity * 256;\n    read_q8 = (int32_t)line->write_pos * 256 - delay_q8;\n''',
        '''    if (!fdn || !line || !fdn->storage_bound) return 0;\n    P11M_PROBE_LINE_CALL(modulation_q8);\n    delay_q8 = (int32_t)line->base_delay * 256 + modulation_q8;\n    P11M_PROBE_DELAY(delay_q8);\n    if (delay_q8 < 256) delay_q8 = 256;\n    needed = (uint32_t)(delay_q8 + 255) >> 8;\n    P11M_PROBE_READINESS(line->fill_count, line->capacity, needed);\n    if (line->fill_count < needed) return 0;\n\n    span_q8 = (int32_t)line->capacity * 256;\n    read_q8 = (int32_t)line->write_pos * 256 - delay_q8;\n    P11M_PROBE_WRAP(read_q8, span_q8);\n''',
        'line-read setup')

    src = replace_once(
        src,
        '''    frac = (uint32_t)read_q8 & 255u;\n    i1 = i0 + 1u;\n    if (i1 >= line->capacity) i1 = 0u;\n''',
        '''    frac = (uint32_t)read_q8 & 255u;\n    i1 = i0 + 1u;\n    P11M_PROBE_INTERP(frac, i1, line->capacity);\n    if (i1 >= line->capacity) i1 = 0u;\n''',
        'interpolation')

    (out / 'apsis_space_dsp.c').write_text(src)
    print('PASS: generated test-only P11M-instrumented FDN source')


if __name__ == '__main__':
    main()
