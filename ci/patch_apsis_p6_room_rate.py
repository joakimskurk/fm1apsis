#!/usr/bin/env python3
"""Retimes only generated FM-1 Phase-6 Cathedral sources to 11.025 kHz.

The musical/source engines and scheduler remain 44.1 kHz. The host/reference
Space implementation is never rewritten. apsis_space_target.c performs the 4:1
send averaging and wet hold; this patch preserves the room's real-time delay,
filter, decay and modulation behaviour at the embedded room rate.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "work" / "apsis" / "firmware" / "src"
room_path = SRC / "apsis_space_room.c"
fdn_path = SRC / "apsis_space_dsp.c"
dinghy_path = SRC / "dinghy.c"

room = room_path.read_text()

repls = [
    (
        "#define ROOM_COMP_ATTACK_Q15 492\n#define ROOM_COMP_RELEASE_Q15 7\n"
        "#define ROOM_SMOOTH_40MS_Q15 19\n#define ROOM_SMOOTH_50MS_Q15 15\n"
        "#define ROOM_SMOOTH_80MS_Q15 9\n#define ROOM_SMOOTH_100MS_Q15 7",
        "#define ROOM_COMP_ATTACK_Q15 1968\n#define ROOM_COMP_RELEASE_Q15 28\n"
        "#define ROOM_SMOOTH_40MS_Q15 76\n#define ROOM_SMOOTH_50MS_Q15 60\n"
        "#define ROOM_SMOOTH_80MS_Q15 36\n#define ROOM_SMOOTH_100MS_Q15 28",
    ),
    (
        "    60692890u, 124275917u, 205199770u, 315024998u",
        "    15173223u, 31068979u, 51299943u, 78756250u",
    ),
    (
        "#define ROOM_FAST_GUARD_HP_LP_ALPHA_Q15 784u\n"
        "#define ROOM_FAST_TAIL_HP_LP_ALPHA_Q15 875u\n"
        "#define ROOM_FAST_ECHO_LP_ALPHA_Q15 14755u",
        "#define ROOM_FAST_GUARD_HP_LP_ALPHA_Q15 3026u\n"
        "#define ROOM_FAST_TAIL_HP_LP_ALPHA_Q15 3363u\n"
        "#define ROOM_FAST_ECHO_LP_ALPHA_Q15 29775u",
    ),
    (
        "    13699u,13969u,14235u,14497u,14755u,15010u,15261u,\n"
        "    15509u,15753u,15994u,16231u,16465u,16696u,16923u,\n"
        "    17147u,17368u,17586u,17801u,18012u",
        "    29009u,29218u,29414u,29600u,29775u,29941u,30098u,\n"
        "    30245u,30385u,30517u,30642u,30759u,30871u,30976u,\n"
        "    31075u,31169u,31257u,31341u,31420u",
    ),
]
for old, new in repls:
    if old not in room:
        raise SystemExit(f"FAIL: quarter-rate room anchor missing: {old[:72]!r}")
    room = room.replace(old, new, 1)

old = '''static uint32_t space_room_ms_q16(uint16_t ms)
{
    return (uint32_t)ms * 2883584u + (uint32_t)ms * 6553u +
           ((uint32_t)ms * 3u + 2u) / 5u;
}'''
new = '''static uint32_t space_room_ms_q16(uint16_t ms)
{
    /* 11.025-kHz target-room rate: one quarter of the 44.1-kHz Q16 frame
     * count, rounded to the nearest Q16 unit. */
    uint32_t full = (uint32_t)ms * 2883584u + (uint32_t)ms * 6553u +
                    ((uint32_t)ms * 3u + 2u) / 5u;
    return (full + 2u) >> 2;
}'''
if old not in room:
    raise SystemExit("FAIL: quarter-rate ms/Q16 anchor missing")
room = room.replace(old, new, 1)
room_path.write_text(room)

fdn = fdn_path.read_text()
fdn_repls = [
    ("#define SPACE_EXP_RATIO_Q30 100913548u", "#define SPACE_EXP_RATIO_Q30 403654192u"),
    ("#define SPACE_MOD_DEPTH_Q8 2304", "#define SPACE_MOD_DEPTH_Q8 576"),
    (
        "    1871u, 1999u, 2137u, 2281u, 2441u, 2593u, 2767u, 2953u",
        "    468u, 500u, 534u, 570u, 610u, 648u, 692u, 738u",
    ),
    (
        "    6915u, 8083u, 9447u, 10616u, 12369u, 13537u, 15290u, 17628u",
        "    27660u, 32332u, 37788u, 42464u, 49476u, 54148u, 61160u, 70512u",
    ),
    (
        "    6040u,6286u,6530u,6772u,7012u,7249u,7484u,7717u,\n"
        "    7948u,8177u,8404u,8628u,8851u,9071u,9290u,9506u,\n"
        "    9721u,9933u,10144u,10352u,10559u,10763u,10966u,11167u,\n"
        "    11366u,11564u,11759u,11953u,12145u,12335u,12523u,12710u,\n"
        "    12895u,13078u,13259u,13439u,13617u,13794u,13969u,14142u,\n"
        "    14314u,14484u,14652u,14819u,14985u,15149u,15311u",
        "    18262u,18790u,19298u,19788u,20260u,20715u,21153u,21576u,\n"
        "    21982u,22375u,22753u,23117u,23468u,23806u,24132u,24446u,\n"
        "    24748u,25040u,25321u,25592u,25853u,26104u,26346u,26580u,\n"
        "    26805u,27022u,27231u,27432u,27626u,27813u,27993u,28167u,\n"
        "    28334u,28495u,28651u,28800u,28945u,29084u,29218u,29347u,\n"
        "    29471u,29591u,29706u,29818u,29925u,30028u,30128u",
    ),
    (
        "    if (offset != APSIS_SPACE_FDN_STORAGE_SAMPLES) fdn->storage_bound = 0u;",
        "    if (offset > APSIS_SPACE_FDN_STORAGE_SAMPLES) fdn->storage_bound = 0u;",
    ),
]
for old, new in fdn_repls:
    if old not in fdn:
        raise SystemExit(f"FAIL: quarter-rate FDN anchor missing: {old[:72]!r}")
    fdn = fdn.replace(old, new, 1)
fdn_path.write_text(fdn)

# Space target scratch is NOLOAD and needs one small boot clear. Do that before
# the frozen runtime scheduler starts so initialization cannot consume the first
# 256-frame runtime interval. This changes no scheduler code or semantics.
dinghy = dinghy_path.read_text()
old = '''    apsis_runtime_start(&apsis_runtime);
    (void)apsis_space_target_bind(&space_target);'''
new = '''    (void)apsis_space_target_bind(&space_target);
    apsis_runtime_start(&apsis_runtime);'''
if old not in dinghy:
    raise SystemExit("FAIL: P6 runtime/bind ordering anchor missing")
dinghy = dinghy.replace(old, new, 1)
dinghy_path.write_text(dinghy)

print("PASS: generated P6 target room retimed to 11.025 kHz; Space init precedes runtime start")
