#!/usr/bin/env python3
from pathlib import Path
import argparse


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    src = Path('apsis/apsis_space_dsp.c').read_text()
    include_anchor = '#include "apsis_space_dsp.h"\n'
    if include_anchor not in src:
        raise SystemExit('P11J include anchor missing')
    src = src.replace(include_anchor,
                      include_anchor + '#include "p11j_probe.h"\n', 1)

    gain_anchor = '''static uint16_t space_feedback_gain(uint16_t delay_samples, uint16_t decay_ms)\n{\n'''
    if gain_anchor not in src:
        raise SystemExit('P11J feedback anchor missing')
    src = src.replace(gain_anchor,
                      gain_anchor + '    P11J_PROBE_FEEDBACK_GAIN(delay_samples, decay_ms);\n', 1)

    render_anchor = '''    if (!fdn || !fdn->storage_bound || !out_stereo) return;\n    alpha = space_damping_alpha(damping_hz);\n'''
    if render_anchor not in src:
        raise SystemExit('P11J render anchor missing')
    src = src.replace(render_anchor,
                      '''    if (!fdn || !fdn->storage_bound || !out_stereo) return;\n    P11J_PROBE_FDN_RENDER(decay_ms, damping_hz);\n    alpha = space_damping_alpha(damping_hz);\n''', 1)

    (out / 'apsis_space_dsp.c').write_text(src)
    print('PASS: generated test-only P11J-instrumented FDN source')


if __name__ == '__main__':
    main()
