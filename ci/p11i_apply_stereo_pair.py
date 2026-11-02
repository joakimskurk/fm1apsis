#!/usr/bin/env python3
from pathlib import Path

p = Path('apsis/apsis_space_room.c')
s = p.read_text()

if 'space_room_stereo_read_pair' in s and 'static int32_t space_room_stereo_read(' not in s:
    print('PASS: P11I stereo-pair production patch already present')
    raise SystemExit(0)

old_helper = '''static int32_t space_room_stereo_read(const int16_t *storage,
                                      uint32_t capacity_frames,
                                      uint32_t write_frame,
                                      uint32_t fill_frames,
                                      uint32_t delay_q16,
                                      uint8_t channel)
{
    uint32_t needed, span_q16, write_q16, read_q16, i0, i1, frac;
    int32_t a, b;
    int64_t mixed;
    if (!storage || !capacity_frames || channel > 1u) return 0;
    needed = (delay_q16 + 65535u) >> 16;
    if (!needed || fill_frames < needed) return 0;
    span_q16 = capacity_frames << 16;
    write_q16 = write_frame << 16;
    if (write_q16 >= delay_q16) read_q16 = write_q16 - delay_q16;
    else read_q16 = span_q16 - (delay_q16 - write_q16);
    if (read_q16 >= span_q16) read_q16 -= span_q16;
    i0 = read_q16 >> 16;
    frac = read_q16 & 65535u;
    i1 = i0 + 1u;
    if (i1 >= capacity_frames) i1 = 0u;
    a = storage[i0 * 2u + channel];
    if (!frac) return a;
    b = storage[i1 * 2u + channel];
    mixed = (int64_t)a * (65536u - frac) + (int64_t)b * frac;
    return (int32_t)(mixed >> 16);
}
'''

new_helper = '''typedef struct {
    int32_t left;
    int32_t right;
} SpaceRoomStereoPair;

static SpaceRoomStereoPair space_room_stereo_read_pair(
    const int16_t *storage,
    uint32_t capacity_frames,
    uint32_t write_frame,
    uint32_t fill_frames,
    uint32_t delay_q16)
{
    SpaceRoomStereoPair out = {0, 0};
    uint32_t needed, span_q16, write_q16, read_q16, i0, i1, frac;
    int32_t a_left, a_right, b_left, b_right;
    int64_t mixed;

    if (!storage || !capacity_frames) return out;
    needed = (delay_q16 + 65535u) >> 16;
    if (!needed || fill_frames < needed) return out;
    span_q16 = capacity_frames << 16;
    write_q16 = write_frame << 16;
    if (write_q16 >= delay_q16) read_q16 = write_q16 - delay_q16;
    else read_q16 = span_q16 - (delay_q16 - write_q16);
    if (read_q16 >= span_q16) read_q16 -= span_q16;
    i0 = read_q16 >> 16;
    frac = read_q16 & 65535u;
    i1 = i0 + 1u;
    if (i1 >= capacity_frames) i1 = 0u;

    a_left = storage[i0 * 2u];
    a_right = storage[i0 * 2u + 1u];
    if (!frac) {
        out.left = a_left;
        out.right = a_right;
        return out;
    }

    b_left = storage[i1 * 2u];
    b_right = storage[i1 * 2u + 1u];
    mixed = (int64_t)a_left * (65536u - frac) + (int64_t)b_left * frac;
    out.left = (int32_t)(mixed >> 16);
    mixed = (int64_t)a_right * (65536u - frac) + (int64_t)b_right * frac;
    out.right = (int32_t)(mixed >> 16);
    return out;
}
'''

old_early = '''            for (tap = 0u; tap < APSIS_SPACE_EARLY_TAPS; ++tap) {
                early_l += space_room_mul_q15(
                    space_room_stereo_read(room->history_storage,
                        APSIS_SPACE_HISTORY_MAX_FRAMES,
                        room->history_write_frame, room->history_fill_frames,
                        ROOM_EARLY_DELAY_Q16[tap], 0u),
                    ROOM_EARLY_GAIN_Q15[tap]);
                early_r += space_room_mul_q15(
                    space_room_stereo_read(room->history_storage,
                        APSIS_SPACE_HISTORY_MAX_FRAMES,
                        room->history_write_frame, room->history_fill_frames,
                        ROOM_EARLY_DELAY_Q16[tap], 1u),
                    ROOM_EARLY_GAIN_Q15[tap]);
            }
'''
new_early = '''            for (tap = 0u; tap < APSIS_SPACE_EARLY_TAPS; ++tap) {
                const SpaceRoomStereoPair early = space_room_stereo_read_pair(
                    room->history_storage, APSIS_SPACE_HISTORY_MAX_FRAMES,
                    room->history_write_frame, room->history_fill_frames,
                    ROOM_EARLY_DELAY_Q16[tap]);
                early_l += space_room_mul_q15(early.left, ROOM_EARLY_GAIN_Q15[tap]);
                early_r += space_room_mul_q15(early.right, ROOM_EARLY_GAIN_Q15[tap]);
            }
'''

old_pred = '''            pred_l = space_room_stereo_read(room->history_storage,
                APSIS_SPACE_HISTORY_MAX_FRAMES, room->history_write_frame,
                room->history_fill_frames, room->predelay_frames_q16, 0u);
            pred_r = space_room_stereo_read(room->history_storage,
                APSIS_SPACE_HISTORY_MAX_FRAMES, room->history_write_frame,
                room->history_fill_frames, room->predelay_frames_q16, 1u);
'''
new_pred = '''            {
                const SpaceRoomStereoPair pred = space_room_stereo_read_pair(
                    room->history_storage, APSIS_SPACE_HISTORY_MAX_FRAMES,
                    room->history_write_frame, room->history_fill_frames,
                    room->predelay_frames_q16);
                pred_l = pred.left;
                pred_r = pred.right;
            }
'''

old_echo = '''            echo_l = space_room_stereo_read(room->echo_storage,
                APSIS_SPACE_ECHO_MAX_FRAMES, room->echo_write_frame,
                room->echo_fill_frames, room->echo_delay_frames_q16, 0u);
            echo_r = space_room_stereo_read(room->echo_storage,
                APSIS_SPACE_ECHO_MAX_FRAMES, room->echo_write_frame,
                room->echo_fill_frames, room->echo_delay_frames_q16, 1u);
'''
new_echo = '''            {
                const SpaceRoomStereoPair echo = space_room_stereo_read_pair(
                    room->echo_storage, APSIS_SPACE_ECHO_MAX_FRAMES,
                    room->echo_write_frame, room->echo_fill_frames,
                    room->echo_delay_frames_q16);
                echo_l = echo.left;
                echo_r = echo.right;
            }
'''

replacements = [
    ('single-channel helper', old_helper, new_helper),
    ('early reflection pair reads', old_early, new_early),
    ('predelay pair read', old_pred, new_pred),
    ('echo pair read', old_echo, new_echo),
]
for label, old, new in replacements:
    count = s.count(old)
    if count != 1:
        raise SystemExit(f'FAIL: {label} anchor count {count} != 1')
    s = s.replace(old, new, 1)

if 'static int32_t space_room_stereo_read(' in s:
    raise SystemExit('FAIL: old single-channel helper survived patch')
if s.count('space_room_stereo_read_pair(') != 4:
    raise SystemExit(f"FAIL: expected helper definition + 3 call sites, got {s.count('space_room_stereo_read_pair(')}")

p.write_text(s)
print('PASS: P11I applied exact stereo-pair Cathedral delay-read patch')
