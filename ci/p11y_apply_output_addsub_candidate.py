#!/usr/bin/env python3
from pathlib import Path
import argparse


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source', default='work/apsis/firmware/src/apsis_space_dsp.c')
    args = ap.parse_args()
    path = Path(args.source)
    s = path.read_text()

    arrays = '''static const int8_t SPACE_OUT_LEFT[APSIS_SPACE_FDN_LINES] = {
    1, 1, -1, 1, -1, 1, 1, -1
};
static const int8_t SPACE_OUT_RIGHT[APSIS_SPACE_FDN_LINES] = {
    1, -1, 1, 1, 1, -1, 1, -1
};

'''
    if s.count(arrays) != 1:
        raise SystemExit('FAIL: P11Y frozen sign-vector anchor mismatch')
    s = s.replace(arrays, '', 1)

    decl = '        int32_t out_l = 0, out_r = 0;\n'
    if s.count(decl) != 1:
        raise SystemExit('FAIL: P11Y output accumulator declaration anchor mismatch')
    s = s.replace(decl, '        int32_t out_l, out_r;\n', 1)

    old = '''            mixed[k] = line->damping_state_q15;
            out_l += (int32_t)SPACE_OUT_LEFT[k] * mixed[k];
            out_r += (int32_t)SPACE_OUT_RIGHT[k] * mixed[k];
        }

        fdn->post_left_q15 += space_mul_q15(
'''
    new = '''            mixed[k] = line->damping_state_q15;
        }

        out_l = mixed[0];
        out_r = mixed[0];
        out_l += mixed[1];
        out_r -= mixed[1];
        out_l -= mixed[2];
        out_r += mixed[2];
        out_l += mixed[3];
        out_r += mixed[3];
        out_l -= mixed[4];
        out_r += mixed[4];
        out_l += mixed[5];
        out_r -= mixed[5];
        out_l += mixed[6];
        out_r += mixed[6];
        out_l -= mixed[7];
        out_r -= mixed[7];

        fdn->post_left_q15 += space_mul_q15(
'''
    if s.count(old) != 1:
        raise SystemExit('FAIL: P11Y output mix anchor mismatch')
    s = s.replace(old, new, 1)

    if 'SPACE_OUT_LEFT' in s or 'SPACE_OUT_RIGHT' in s:
        raise SystemExit('FAIL: P11Y sign vectors still referenced after transform')
    path.write_text(s)
    print(f'PASS: applied ephemeral P11Y fixed output add/sub candidate to {path}')


if __name__ == '__main__':
    main()
