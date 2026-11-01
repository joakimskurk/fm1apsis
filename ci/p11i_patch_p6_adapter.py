#!/usr/bin/env python3
from pathlib import Path

p = Path('ci/apply_apsis_p6.sh')
s = p.read_text()

marker = "P11I keeps this target-only optimization backward-compatible"
if marker in s:
    print('PASS: P6 target adapter already supports P11I stereo-pair reader')
    raise SystemExit(0)

start_marker = "# Convex interpolation of two int16 samples with Q16 weights is guaranteed to\n"
end_marker = "# Keep the full reference biquad implementation in the copied source for easy\n"
start = s.find(start_marker)
end = s.find(end_marker, start)
if start < 0 or end < 0 or end <= start:
    raise SystemExit('FAIL: historical P6 interpolation adapter block not found')

old_block = s[start:end]
for required in (
    "s = s.replace('    int64_t mixed;\\n', '    int32_t mixed;\\n', 1)",
    "mixed = (int64_t)a * (65536u - frac) + (int64_t)b * frac;",
    "raise SystemExit('FAIL: P6 target interpolation fast-path anchor missing')",
):
    if required not in old_block:
        raise SystemExit(f'FAIL: historical P6 interpolation adapter lost anchor: {required}')

new_block = """# Convex interpolation of two int16 samples with Q16 weights is guaranteed to
# remain inside signed 32-bit range; avoid 64-bit multiplies in every tap read.
# P11I keeps this target-only optimization backward-compatible with both the
# historical single-channel reader and the proven stereo-pair reader.
if 'static SpaceRoomStereoPair space_room_stereo_read_pair(' in s:
    s = s.replace('    int64_t mixed;\\n', '    int32_t mixed;\\n', 1)
    old_pair = '''    mixed = (int64_t)a_left * (65536u - frac) + (int64_t)b_left * frac;
    out.left = (int32_t)(mixed >> 16);
    mixed = (int64_t)a_right * (65536u - frac) + (int64_t)b_right * frac;
    out.right = (int32_t)(mixed >> 16);'''
    new_pair = '''    mixed = a_left * (int32_t)(65536u - frac) + b_left * (int32_t)frac;
    out.left = mixed >> 16;
    mixed = a_right * (int32_t)(65536u - frac) + b_right * (int32_t)frac;
    out.right = mixed >> 16;'''
    if old_pair not in s:
        raise SystemExit('FAIL: P6 target stereo-pair interpolation fast-path anchor missing')
    s = s.replace(old_pair, new_pair, 1)
else:
    s = s.replace('    int64_t mixed;\\n', '    int32_t mixed;\\n', 1)
    old_single = '''    mixed = (int64_t)a * (65536u - frac) + (int64_t)b * frac;
    return (int32_t)(mixed >> 16);'''
    new_single = '''    mixed = a * (int32_t)(65536u - frac) + b * (int32_t)frac;
    return mixed >> 16;'''
    if old_single not in s:
        raise SystemExit('FAIL: P6 target interpolation fast-path anchor missing')
    s = s.replace(old_single, new_single, 1)

"""

s = s[:start] + new_block + s[end:]
p.write_text(s)
print('PASS: P6 target interpolation adapter supports historical single and P11I pair readers')
