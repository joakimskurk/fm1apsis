#!/usr/bin/env python3
"""Retimes only the generated FM-1 Space/Cathedral room to 22.05 kHz.

The source/host reference engine remains 44.1 kHz and is never rewritten.
apsis_space_target.c performs the 2:1 send downsample / wet hold; this patch
keeps acoustic times, filter cutoffs, LFO rates and decay law approximately
constant when the generated embedded room advances once per two source frames.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "work" / "apsis" / "firmware" / "src"
room_path = SRC / "apsis_space_room.c"
fdn_path = SRC / "apsis_space_dsp.c"

room = room_path.read_text()

repls = [
    (
        "#define ROOM_COMP_ATTACK_Q15 492\n#define ROOM_COMP_RELEASE_Q15 7\n"
        "#define ROOM_SMOOTH_40MS_Q15 19\n#define ROOM_SMOOTH_50MS_Q15 15\n"
        "#define ROOM_SMOOTH_80MS_Q15 9\n#define ROOM_SMOOTH_100MS_Q15 7",
        "#define ROOM_COMP_ATTACK_Q15 984\n#define ROOM_COMP_RELEASE_Q15 14\n"
        "#define ROOM_SMOOTH_40MS_Q15 38\n#define ROOM_SMOOTH_50MS_Q15 30\n"
        "#define ROOM_SMOOTH_80MS_Q15 18\n#define ROOM_SMOOTH_100MS_Q15 14",
    ),
    (
        "    60692890u, 124275917u, 205199770u, 315024998u",
        "    30346445u, 62137959u, 102599885u, 157512499u",
    ),
    (
        "#define ROOM_FAST_GUARD_HP_LP_ALPHA_Q15 784u\n"
        "#define ROOM_FAST_TAIL_HP_LP_ALPHA_Q15 875u\n"
        "#define ROOM_FAST_ECHO_LP_ALPHA_Q15 14755u",
        "#define ROOM_FAST_GUARD_HP_LP_ALPHA_Q15 1549u\n"
        "#define ROOM_FAST_TAIL_HP_LP_ALPHA_Q15 1727u\n"
        "#define ROOM_FAST_ECHO_LP_ALPHA_Q15 22866u",
    ),
    (
        "    13699u,13969u,14235u,14497u,14755u,15010u,15261u,\n"
        "    15509u,15753u,15994u,16231u,16465u,16696u,16923u,\n"
        "    17147u,17368u,17586u,17801u,18012u",
        "    21671u,21982u,22285u,22580u,22866u,23144u,23415u,\n"
        "    23677u,23933u,24181u,24422u,24657u,24884u,25106u,\n"
        "    25321u,25530u,25734u,25931u,26123u",
    ),
]
for old, new in repls:
    if old not in room:
        raise SystemExit(f"FAIL: half-rate room anchor missing: {old[:72]!r}")
    room = room.replace(old, new, 1)

old = '''static uint32_t space_room_ms_q16(uint16_t ms)
{
    return (uint32_t)ms * 2883584u + (uint32_t)ms * 6553u +
           ((uint32_t)ms * 3u + 2u) / 5u;
}'''
new = '''static uint32_t space_room_ms_q16(uint16_t ms)
{
    /* 22.05 kHz target-room rate: exactly one half of the 44.1-kHz Q16
     * frame count, rounded to the nearest Q16 unit. */
    uint32_t full = (uint32_t)ms * 2883584u + (uint32_t)ms * 6553u +
                    ((uint32_t)ms * 3u + 2u) / 5u;
    return (full + 1u) >> 1;
}'''
if old not in room:
    raise SystemExit("FAIL: half-rate ms/Q16 anchor missing")
room = room.replace(old, new, 1)
room_path.write_text(room)

fdn = fdn_path.read_text()
fdn_repls = [
    ("#define SPACE_EXP_RATIO_Q30 100913548u", "#define SPACE_EXP_RATIO_Q30 201827096u"),
    ("#define SPACE_MOD_DEPTH_Q8 2304", "#define SPACE_MOD_DEPTH_Q8 1152"),
    (
        "    1871u, 1999u, 2137u, 2281u, 2441u, 2593u, 2767u, 2953u",
        "    936u, 1000u, 1069u, 1141u, 1221u, 1297u, 1384u, 1477u",
    ),
    (
        "    6915u, 8083u, 9447u, 10616u, 12369u, 13537u, 15290u, 17628u",
        "    13830u, 16166u, 18894u, 21232u, 24738u, 27074u, 30580u, 35256u",
    ),
    (
        "    6040u,6286u,6530u,6772u,7012u,7249u,7484u,7717u,\n"
        "    7948u,8177u,8404u,8628u,8851u,9071u,9290u,9506u,\n"
        "    9721u,9933u,10144u,10352u,10559u,10763u,10966u,11167u,\n"
        "    11366u,11564u,11759u,11953u,12145u,12335u,12523u,12710u,\n"
        "    12895u,13078u,13259u,13439u,13617u,13794u,13969u,14142u,\n"
        "    14314u,14484u,14652u,14819u,14985u,15149u,15311u",
        "    10966u,11366u,11759u,12145u,12523u,12895u,13259u,13617u,\n"
        "    13969u,14314u,14652u,14985u,15311u,15631u,15946u,16255u,\n"
        "    16558u,16855u,17147u,17434u,17715u,17991u,18262u,18529u,\n"
        "    18790u,19046u,19298u,19545u,19788u,20026u,20260u,20490u,\n"
        "    20715u,20936u,21153u,21366u,21576u,21781u,21982u,22180u,\n"
        "    22375u,22565u,22753u,22936u,23117u,23294u,23468u",
    ),
    (
        "    if (offset != APSIS_SPACE_FDN_STORAGE_SAMPLES) fdn->storage_bound = 0u;",
        "    if (offset > APSIS_SPACE_FDN_STORAGE_SAMPLES) fdn->storage_bound = 0u;",
    ),
]
for old, new in fdn_repls:
    if old not in fdn:
        raise SystemExit(f"FAIL: half-rate FDN anchor missing: {old[:72]!r}")
    fdn = fdn.replace(old, new, 1)
fdn_path.write_text(fdn)

print("PASS: generated P6 target room retimed to 22.05 kHz")
