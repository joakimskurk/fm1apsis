#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Test-only layer above the green P10D2 firmware. This does not alter the
# production Motion model or store; it adds a compact telemetry probe so the
# pinned emulator can prove the complete physical lifecycle.
bash ci/apply_apsis_p10d2.sh

python3 - <<'PY'
from pathlib import Path
import re

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

state_anchor = 'static uint8_t apsis_string_motion_playback_valid;'
if state_anchor not in s:
    raise SystemExit('FAIL: P10D3 P10D2 state anchor missing')
s = s.replace(state_anchor, state_anchor + r'''
/* P10D3 CI-only lifecycle telemetry. Not part of the production state graph. */
static volatile uint32_t apsis_motion_probe[10];
static uint32_t apsis_motion_probe_seen;
static uint8_t apsis_motion_probe_prev_recording;
static uint8_t apsis_motion_probe_prev_manual;
static uint32_t apsis_motion_probe_record_start;
static uint32_t apsis_motion_probe_record_finish;
static uint32_t apsis_motion_probe_manual_start;
static uint32_t apsis_motion_probe_manual_resume;''', 1)

app_audio_anchor = 'static void app_audio(int32_t *out, uint32_t n)'
if app_audio_anchor not in s:
    raise SystemExit('FAIL: P10D3 app_audio anchor missing')
probe_helper = r'''enum {
    APSIS_MOTION_SEEN_ARMED = 1u << 0,
    APSIS_MOTION_SEEN_RECORDING = 1u << 1,
    APSIS_MOTION_SEEN_FINISHED_PLAY = 1u << 2,
    APSIS_MOTION_SEEN_PLAY = 1u << 3,
    APSIS_MOTION_SEEN_MANUAL = 1u << 4,
    APSIS_MOTION_SEEN_RESUME = 1u << 5,
    APSIS_MOTION_SEEN_POINTS = 1u << 6,
    APSIS_MOTION_SEEN_PLAYBACK = 1u << 7
};

static void apsis_p10_motion_probe_update(void)
{
    uint8_t recording = apsis_string_motion_runtime.recording ? 1u : 0u;
    uint8_t manual = apsis_string_motion_runtime.manual_override ? 1u : 0u;
    uint8_t play = apsis.string.motion_loop_play ? 1u : 0u;
    uint32_t cursor = (uint32_t)apsis_runtime.cursor_sample;
    uint32_t current = 0u;
    uint32_t health;

    if (apsis_string_motion_runtime.armed)
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_ARMED;
    if (recording) {
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_RECORDING;
        if (!apsis_motion_probe_prev_recording && !apsis_motion_probe_record_start)
            apsis_motion_probe_record_start = cursor;
    }
    if (apsis_motion_probe_prev_recording && !recording && play) {
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_FINISHED_PLAY;
        if (!apsis_motion_probe_record_finish)
            apsis_motion_probe_record_finish = cursor;
    }
    if (play) apsis_motion_probe_seen |= APSIS_MOTION_SEEN_PLAY;
    if (manual) {
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_MANUAL;
        if (!apsis_motion_probe_prev_manual && !apsis_motion_probe_manual_start)
            apsis_motion_probe_manual_start = cursor;
    }
    if (apsis_motion_probe_prev_manual && !manual && play) {
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_RESUME;
        if (!apsis_motion_probe_manual_resume)
            apsis_motion_probe_manual_resume = cursor;
    }
    if (apsis_string_motion_store.left.count > 1u ||
        apsis_string_motion_store.right.count > 1u)
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_POINTS;
    if (apsis_string_motion_playback_valid)
        apsis_motion_probe_seen |= APSIS_MOTION_SEEN_PLAYBACK;

    if (apsis_string_motion_runtime.armed) current |= 1u << 0;
    if (recording) current |= 1u << 1;
    if (play) current |= 1u << 2;
    if (manual) current |= 1u << 3;
    if (apsis_string_motion_playback_valid) current |= 1u << 4;

    health = (apsis_runtime_probe[1] & 0x3ffu) |
             ((apsis_runtime_probe[2] ? 1u : 0u) << 10) |
             ((apsis_runtime_probe[4] & 0xffffu) << 16);
    apsis_motion_probe[0] = 0x4d4f544eu; /* "MOTN" */
    apsis_motion_probe[1] = apsis_motion_probe_seen;
    apsis_motion_probe[2] = current;
    apsis_motion_probe[3] = (uint32_t)apsis_string_motion_store.left.count |
                            ((uint32_t)apsis_string_motion_store.right.count << 16);
    apsis_motion_probe[4] = apsis_motion_probe_record_start;
    apsis_motion_probe[5] = apsis_motion_probe_record_finish;
    apsis_motion_probe[6] = apsis_motion_probe_manual_start;
    apsis_motion_probe[7] = apsis_motion_probe_manual_resume;
    apsis_motion_probe[8] = health;
    apsis_motion_probe[9] = cursor;

    apsis_motion_probe_prev_recording = recording;
    apsis_motion_probe_prev_manual = manual;
}

'''
s = s.replace(app_audio_anchor, probe_helper + app_audio_anchor, 1)

# D2 retains two established callback telemetry paths. Instrument both after
# probe[5] is written so the D3 helper observes whichever path actually ran,
# rather than inventing a third callback path or disturbing M3c.
pattern = r'(?m)^([ \t]*apsis_runtime_probe\[5\][ \t]*=[^;\n]+;)$'
matches = list(re.finditer(pattern, s))
if len(matches) != 2:
    raise SystemExit(f'FAIL: P10D3 expected two runtime probe[5] assignments, got {len(matches)}')
for m in reversed(matches):
    indent = re.match(r'[ \t]*', m.group(1)).group(0)
    s = s[:m.end()] + '\n' + indent + 'apsis_p10_motion_probe_update();' + s[m.end():]

# Explicitly initialize the CI probe only; no Motion store memset is added.
init_anchor = '    apsis_string_motion_playback_valid = 0u;'
if init_anchor not in s:
    raise SystemExit('FAIL: P10D3 P10D2 init anchor missing')
s = s.replace(init_anchor, init_anchor + r'''
    apsis_motion_probe[0] = 0x4d4f544eu;
    apsis_motion_probe_seen = 0u;
    apsis_motion_probe_prev_recording = 0u;
    apsis_motion_probe_prev_manual = 0u;
    apsis_motion_probe_record_start = 0u;
    apsis_motion_probe_record_finish = 0u;
    apsis_motion_probe_manual_start = 0u;
    apsis_motion_probe_manual_resume = 0u;''', 1)

p.write_text(s)
PY

grep -q 'static volatile uint32_t apsis_motion_probe\[10\]' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_MOTION_SEEN_FINISHED_PLAY' work/apsis/firmware/src/dinghy.c
[[ $(grep -c 'apsis_p10_motion_probe_update();' work/apsis/firmware/src/dinghy.c) -eq 2 ]]

echo "PASS: P10D3 active Motion lifecycle telemetry layered test-only above green D2"
