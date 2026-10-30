#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 6 is layered strictly above the frozen M4g/Phase-5 target generator.
# APSIS_CI_SPACE_DRY_ONLY=1 keeps the exact same split source-bus path and full
# production storage, but deliberately omits the final room render call so CI
# can prove that source splitting itself does not change frozen dry PCM.
bash ci/apply_apsis_m4g.sh

VERSION=${APSIS_VERSION:-0.1-dev}
DRY_ONLY=${APSIS_CI_SPACE_DRY_ONLY:-0}

cp apsis/apsis_space_dsp.h apsis/apsis_space_dsp.c \
   apsis/apsis_space_room.h apsis/apsis_space_room.c \
   apsis/apsis_space_target.h apsis/apsis_space_target.c \
   work/apsis/firmware/src/

# The host/reference room deliberately keeps its high-precision biquads. The
# FM-1 target has a much tighter callback instruction budget: 64-bit recursive
# filter multiplies consumed enough emulated 240-MHz cycles to miss roughly
# every second callback. Generate a target-equivalent hot path using stable
# one-pole HP/LP sections for the same guard/echo/tail roles, plus safe 32-bit
# Q15/interpolation fast paths. The shared-room topology, delays, FDN, sends,
# smoothing, compressor and NOLOAD storage remain unchanged.
python3 - <<'PY'
from pathlib import Path

room = Path('work/apsis/firmware/src/apsis_space_room.c')
s = room.read_text()

old = '''static int32_t space_room_mul_q15(int32_t a, int32_t b)
{
    return space_room_round_shift((int64_t)a * b, 15u);
}'''
new = '''static int32_t space_room_mul_q15(int32_t a, int32_t b)
{
    if (a >= -65535 && a <= 65535 && b >= -32767 && b <= 32767) {
        int32_t p = a * b;
        if (p >= 0) p += 16384;
        else p -= 16384;
        return p >> 15;
    }
    return space_room_round_shift((int64_t)a * b, 15u);
}'''
if old not in s:
    raise SystemExit('FAIL: P6 target Q15 fast-path anchor missing')
s = s.replace(old, new, 1)

# Convex interpolation of two int16 samples with Q16 weights is guaranteed to
# remain inside signed 32-bit range; avoid a 64-bit multiply in every tap read.
s = s.replace('    int64_t mixed;\n', '    int32_t mixed;\n', 1)
old = '''    mixed = (int64_t)a * (65536u - frac) + (int64_t)b * frac;
    return (int32_t)(mixed >> 16);'''
new = '''    mixed = a * (int32_t)(65536u - frac) + b * (int32_t)frac;
    return mixed >> 16;'''
if old not in s:
    raise SystemExit('FAIL: P6 target interpolation fast-path anchor missing')
s = s.replace(old, new, 1)

# Keep the full reference biquad implementation in the copied source for easy
# comparison/debugging, but mark its now-unused helpers explicitly.
s = s.replace('static int32_t space_room_biquad(',
              'static __attribute__((unused)) int32_t space_room_biquad(', 1)
s = s.replace('static const SpaceRoomBiquadCoeff *space_room_guard_lp_coeff(',
              'static __attribute__((unused)) const SpaceRoomBiquadCoeff *space_room_guard_lp_coeff(', 1)

anchor = 'static void space_room_targets(ApsisSpaceRoom *room, const ApsisSpaceState *state)'
if anchor not in s:
    raise SystemExit('FAIL: P6 target one-pole insertion anchor missing')
fast = r'''/* FM-1 target hot-path one-poles at 44.1 kHz, Q15. The four roles are
 * unchanged from the high-precision reference room: 170-Hz guard HP,
 * 3.8..5.6-kHz guard LP, 4.2-kHz echo LP and 190-Hz tail HP. */
#define ROOM_FAST_GUARD_HP_LP_ALPHA_Q15 784u
#define ROOM_FAST_TAIL_HP_LP_ALPHA_Q15 875u
#define ROOM_FAST_ECHO_LP_ALPHA_Q15 14755u

static const uint16_t ROOM_FAST_GUARD_LP_ALPHA[19] = {
    13699u,13969u,14235u,14497u,14755u,15010u,15261u,
    15509u,15753u,15994u,16231u,16465u,16696u,16923u,
    17147u,17368u,17586u,17801u,18012u
};

static uint16_t space_room_fast_guard_alpha(int32_t hz_q16)
{
    uint32_t hz, index;
    if (hz_q16 <= 0) return ROOM_FAST_GUARD_LP_ALPHA[0];
    hz = ((uint32_t)hz_q16 + 32768u) >> 16;
    if (hz <= ROOM_GUARD_LP_MIN_HZ) return ROOM_FAST_GUARD_LP_ALPHA[0];
    if (hz >= ROOM_GUARD_LP_MAX_HZ) return ROOM_FAST_GUARD_LP_ALPHA[18];
    index = (hz - ROOM_GUARD_LP_MIN_HZ + 50u) / 100u;
    if (index > 18u) index = 18u;
    return ROOM_FAST_GUARD_LP_ALPHA[index];
}

static int32_t space_room_fast_lp(ApsisSpaceBiquadState *state,
                                  int32_t x_q15, uint16_t alpha_q15)
{
    int32_t cur, delta, product, step;
    if (!state) return x_q15;
    cur = (int32_t)state->z1_q60;
    delta = x_q15 - cur;
    product = delta * (int32_t)alpha_q15;
    if (product >= 0) product += 16384;
    else product -= 16384;
    step = product >> 15;
    if (!step && delta) step = delta > 0 ? 1 : -1;
    cur += step;
    cur = space_room_clamp_q15(cur);
    state->z1_q60 = cur;
    return cur;
}

static int32_t space_room_fast_hp(ApsisSpaceBiquadState *state,
                                  int32_t x_q15, uint16_t low_alpha_q15)
{
    int32_t low = space_room_fast_lp(state, x_q15, low_alpha_q15);
    return space_room_clamp_q15((int64_t)x_q15 - low);
}

'''
s = s.replace(anchor, fast + anchor, 1)

# Keep the coefficient tables referenced so -Wall/-Werror host-shell builds do
# not warn about the reference constants after target substitution.
target_anchor = '    if (!room || !state) return;\n'
keep = '''    if (!room || !state) return;
    (void)&ROOM_GUARD_HP_170;
    (void)&ROOM_TAIL_HP_190;
    (void)&ROOM_ECHO_LP_4200;
    (void)&ROOM_GUARD_LP[0];
'''
if target_anchor not in s:
    raise SystemExit('FAIL: P6 target coefficient-reference anchor missing')
s = s.replace(target_anchor, keep, 1)

if '            const SpaceRoomBiquadCoeff *guard_lp;\n' not in s:
    raise SystemExit('FAIL: P6 target guard declaration anchor missing')
s = s.replace('            const SpaceRoomBiquadCoeff *guard_lp;\n',
              '            uint16_t guard_lp_alpha;\n', 1)

if '            guard_lp = space_room_guard_lp_coeff(room->guard_lp_hz_q16);\n' not in s:
    raise SystemExit('FAIL: P6 target guard lookup anchor missing')
s = s.replace('            guard_lp = space_room_guard_lp_coeff(room->guard_lp_hz_q16);\n',
              '            guard_lp_alpha = space_room_fast_guard_alpha(room->guard_lp_hz_q16);\n', 1)

old = '''            g_l = space_room_mul_q15(in_l, ROOM_TRIM_Q15);
            g_r = space_room_mul_q15(in_r, ROOM_TRIM_Q15);
            g_l = space_room_biquad(&room->guard_hp[0], &ROOM_GUARD_HP_170, g_l);
            g_r = space_room_biquad(&room->guard_hp[1], &ROOM_GUARD_HP_170, g_r);
            g_l = space_room_biquad(&room->guard_lp[0], guard_lp, g_l);
            g_r = space_room_biquad(&room->guard_lp[1], guard_lp, g_r);'''
new = '''            g_l = space_room_clamp_q15(space_room_mul_q15(in_l, ROOM_TRIM_Q15));
            g_r = space_room_clamp_q15(space_room_mul_q15(in_r, ROOM_TRIM_Q15));
            g_l = space_room_fast_hp(&room->guard_hp[0], g_l,
                                     ROOM_FAST_GUARD_HP_LP_ALPHA_Q15);
            g_r = space_room_fast_hp(&room->guard_hp[1], g_r,
                                     ROOM_FAST_GUARD_HP_LP_ALPHA_Q15);
            g_l = space_room_fast_lp(&room->guard_lp[0], g_l, guard_lp_alpha);
            g_r = space_room_fast_lp(&room->guard_lp[1], g_r, guard_lp_alpha);'''
if old not in s:
    raise SystemExit('FAIL: P6 target guard filter anchors missing')
s = s.replace(old, new, 1)

old = '''            echo_fb_l = space_room_biquad(&room->echo_lp[0], &ROOM_ECHO_LP_4200, echo_l);
            echo_fb_r = space_room_biquad(&room->echo_lp[1], &ROOM_ECHO_LP_4200, echo_r);'''
new = '''            echo_fb_l = space_room_fast_lp(&room->echo_lp[0], echo_l,
                                           ROOM_FAST_ECHO_LP_ALPHA_Q15);
            echo_fb_r = space_room_fast_lp(&room->echo_lp[1], echo_r,
                                           ROOM_FAST_ECHO_LP_ALPHA_Q15);'''
if old not in s:
    raise SystemExit('FAIL: P6 target echo filter anchors missing')
s = s.replace(old, new, 1)

old = '''            tail_l = space_room_biquad(&room->tail_hp[0], &ROOM_TAIL_HP_190,
                                       pred_l + echo_l);
            tail_r = space_room_biquad(&room->tail_hp[1], &ROOM_TAIL_HP_190,
                                       pred_r + echo_r);
            tail_mono[i] = space_room_clamp_q15(((int64_t)tail_l + tail_r) / 2);'''
new = '''            tail_l = space_room_fast_hp(&room->tail_hp[0],
                                         space_room_clamp_q15((int64_t)pred_l + echo_l),
                                         ROOM_FAST_TAIL_HP_LP_ALPHA_Q15);
            tail_r = space_room_fast_hp(&room->tail_hp[1],
                                         space_room_clamp_q15((int64_t)pred_r + echo_r),
                                         ROOM_FAST_TAIL_HP_LP_ALPHA_Q15);
            tail_mono[i] = (tail_l + tail_r) / 2;'''
if old not in s:
    raise SystemExit('FAIL: P6 target tail filter anchors missing')
s = s.replace(old, new, 1)

room.write_text(s)

# Most FDN Q15 multiplies are also safely bounded to 32-bit; retain the exact
# 64-bit fallback for the rare Hadamard intermediate that can exceed it.
fdn = Path('work/apsis/firmware/src/apsis_space_dsp.c')
f = fdn.read_text()
old = '''static int32_t space_mul_q15(int32_t a, int32_t b)
{
    int64_t p = (int64_t)a * b;
    if (p >= 0) p += 16384;
    else p -= 16384;
    return (int32_t)(p >> 15);
}'''
new = '''static int32_t space_mul_q15(int32_t a, int32_t b)
{
    if (a >= -65535 && a <= 65535 && b >= -32767 && b <= 32767) {
        int32_t p = a * b;
        if (p >= 0) p += 16384;
        else p -= 16384;
        return p >> 15;
    }
    {
        int64_t p = (int64_t)a * b;
        if (p >= 0) p += 16384;
        else p -= 16384;
        return (int32_t)(p >> 15);
    }
}'''
if old not in f:
    raise SystemExit('FAIL: P6 target FDN Q15 fast-path anchor missing')
f = f.replace(old, new, 1)
fdn.write_text(f)
PY

APSIS_CI_SPACE_DRY_ONLY="$DRY_ONLY" python3 - <<'PY'
from pathlib import Path
import os

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()
dry_only = os.environ.get('APSIS_CI_SPACE_DRY_ONLY') == '1'

h_anchor = '#include "apsis_bloom_dsp.h"'
if '#include "apsis_space_target.h"' not in s:
    if h_anchor not in s:
        raise SystemExit('FAIL: P6 Space header anchor missing')
    s = s.replace(
        h_anchor,
        h_anchor +
        '\n#include "apsis_space_dsp.h"'
        '\n#include "apsis_space_room.h"'
        '\n#include "apsis_space_target.h"',
        1)

c_anchor = '#include "apsis_bloom_dsp.c"'
if '#include "apsis_space_target.c"' not in s:
    if c_anchor not in s:
        raise SystemExit('FAIL: P6 Space source anchor missing')
    s = s.replace(
        c_anchor,
        c_anchor +
        '\n#include "apsis_space_dsp.c"'
        '\n#include "apsis_space_room.c"'
        '\n#include "apsis_space_target.c"',
        1)

state_anchor = 'static uint8_t bloom_audio_started;'
if state_anchor not in s:
    raise SystemExit('FAIL: P6 Space state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisSpaceTarget space_target;', 1)

ready_anchor = '    if (runtime_ready) {'
if ready_anchor not in s:
    raise SystemExit('FAIL: P6 app_audio ready anchor missing')
s = s.replace(ready_anchor,
              ready_anchor + '\n        apsis_space_target_begin_block(&space_target, n);', 1)

# Preserve the exact M4g source/render order. Each source renderer now writes
# into one reusable Q15 source bus; commit_source applies Mother mix/mute to dry
# and derives the post-Mother Space send from the same contribution.
def split_render(call, send, mother_ix):
    old = call
    new = (
        '        {\n'
        '            int32_t *space_source = apsis_space_target_source_buffer(&space_target, n);\n'
        f'            {call.strip().replace("out, n", "space_source, n")}\n'
        '            apsis_space_target_commit_source(\n'
        '                &space_target, out, n, '
        f'{send}, apsis.mother.mix_q15[{mother_ix}],\n'
        f'                (uint8_t)(apsis.mother.mute_mask & (1u << {mother_ix})));\n'
        '        }'
    )
    return old, new

replacements = [
    split_render('        apsis_string_dsp_render(&string_dsp, out, n);',
                 'apsis.string.send_q15', 0),
    split_render('        apsis_drift_dsp_render(&drift_dsp, out, n);',
                 'apsis.drift.send_q15', 1),
    split_render('        apsis_clock_dsp_render(&clock_dsp, out, n);',
                 'apsis.clock.send_q15', 2),
    split_render('        apsis_string_transient_render(&string_transient_pool, out, n);',
                 'apsis.string.send_q15', 0),
    split_render('                    apsis_spiral_dsp_render(&spiral_dsp, out, n);',
                 'apsis.spiral.send_q15', 3),
    split_render('                    apsis_bloom_dsp_render(&bloom_dsp, out, n);',
                 'apsis.bloom.send_q15', 4),
    split_render('                apsis_spiral_dsp_render(&spiral_dsp, out, n);',
                 'apsis.spiral.send_q15', 3),
]
for old, new in replacements:
    if old not in s:
        raise SystemExit(f'FAIL: P6 source split anchor missing: {old.strip()}')
    s = s.replace(old, new, 1)

# Room rendering is intentionally after every current-block source contribution
# has been committed. The dry-only CI mode skips only this call; it still uses
# the split bus, Mother gain/mute and production NOLOAD storage layout.
probe_anchor = '        apsis_runtime_probe[0] = 0x41505349u; /* "APSI" */'
if probe_anchor not in s:
    raise SystemExit('FAIL: P6 room render anchor missing')
if not dry_only:
    s = s.replace(probe_anchor,
                  '        apsis_space_target_render(&space_target, &apsis.space, out, n);\n' +
                  probe_anchor, 1)

start_anchor = '    apsis_runtime_start(&apsis_runtime);'
if start_anchor not in s:
    raise SystemExit('FAIL: P6 room bind anchor missing')
s = s.replace(start_anchor,
              start_anchor + '\n    (void)apsis_space_target_bind(&space_target);', 1)

p.write_text(s)

# Reserve long-lived Space storage in a NOLOAD section immediately before the
# ordinary Dinghy .pool. _pool_end stays after both, so build accounting and the
# hardware POOL limit still cover the full allocation while Dinghy clears only
# its original pool at boot.
ld = Path('work/apsis/firmware/app.ld')
ls = ld.read_text()
old = '.pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL'
new = '.apsis_space_pool (NOLOAD) : { KEEP(*(.apsis_space_pool*)) . = ALIGN(4); } > POOL\n    ' + old
if old not in ls:
    raise SystemExit('FAIL: P6 Space linker anchor missing')
ls = ls.replace(old, new, 1)
ld.write_text(ls)
PY

# Compile the actual generated single-TU Phase-6 target before the real JieLi
# link gate. This catches target-only namespace/include regressions immediately.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

if [[ "$DRY_ONLY" == "1" ]]; then
  echo "PASS: P6 split-source dry-only overlay with production Space storage"
else
  echo "PASS: P6 shared Space/Cathedral overlay integrated into app_audio"
fi
