#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# H4a binds only the already-host-proven direct String transient intent. Bloom
# manual audition remains deliberately deferred to H4b so each performance path
# has an isolated target/emulator checkpoint.
bash ci/apply_apsis_p9h3b.sh

# Phase 9 must not contaminate historical M4b/M4f authority. These are the
# exact pre-H4a blobs from parent 9bead093 and are also exercised by staged CI.
[[ "$(git hash-object apsis/apsis_string_rhythm_audio.c)" == "a3e58f2744917a5d97d2763832ed52b9b104e578" ]] || {
  echo "FAIL: P9H4a modified frozen M4b apsis_string_rhythm_audio.c" >&2; exit 2; }
[[ "$(git hash-object apsis/apsis_string_rhythm_audio.h)" == "b9b7506242cd0733c26149cd9a81f5c22aa3e381" ]] || {
  echo "FAIL: P9H4a modified frozen M4b apsis_string_rhythm_audio.h" >&2; exit 2; }
[[ "$(git hash-object tests/test_string_rhythm_audio.c)" == "76cad7635b2474690597b2ffe1408d3ae4bf2664" ]] || {
  echo "FAIL: P9H4a modified frozen M4b String Rhythm test" >&2; exit 2; }

cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Iapsis \
  apsis/apsis_controls_performance.c tests/test_controls_performance.c \
  -o /tmp/test_controls_performance
/tmp/test_controls_performance
cc -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -Iapsis \
  apsis/apsis_controls_performance.c tests/test_controls_performance.c \
  -o /tmp/test_controls_performance_san
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  /tmp/test_controls_performance_san

# P9C explicitly maps FM-1 keys onto Study-126 keyboardStringPress(), so its
# manual acoustic path is P9-only and freezes the browser keyboard level .080.
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Iapsis \
  apsis/apsis_state.c apsis/apsis_harmony.c apsis/apsis_gravity.c \
  apsis/apsis_string.c apsis/apsis_string_dsp.c \
  apsis/apsis_string_transient_pool.c apsis/apsis_string_manual_audio.c \
  tests/test_string_manual_audio.c -o /tmp/test_string_manual_audio
/tmp/test_string_manual_audio
cc -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -Iapsis \
  apsis/apsis_state.c apsis/apsis_harmony.c apsis/apsis_gravity.c \
  apsis/apsis_string.c apsis/apsis_string_dsp.c \
  apsis/apsis_string_transient_pool.c apsis/apsis_string_manual_audio.c \
  tests/test_string_manual_audio.c -o /tmp/test_string_manual_audio_san
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  /tmp/test_string_manual_audio_san

cp apsis/apsis_controls_performance.h apsis/apsis_controls_performance.c \
   apsis/apsis_string_manual_audio.h apsis/apsis_string_manual_audio.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_controls_dirty.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P9H4a H3b header anchor missing')
s = s.replace(h_anchor, h_anchor +
              '\n#include "apsis_controls_performance.h"'
              '\n#include "apsis_string_manual_audio.h"', 1)

c_anchor = '#include "apsis_controls_dirty.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P9H4a H3b source anchor missing')
s = s.replace(c_anchor, c_anchor +
              '\n#include "apsis_controls_performance.c"'
              '\n#include "apsis_string_manual_audio.c"', 1)

state_anchor = 'static ApsisControlsProjectDirty apsis_project_dirty;'
if state_anchor not in s:
    raise SystemExit('FAIL: P9H4a H3b state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisControlsPerformanceMailbox apsis_controls_performance;', 1)

init_anchor = '    apsis_controls_project_dirty_reset(&apsis_project_dirty);'
if init_anchor not in s:
    raise SystemExit('FAIL: P9H4a H3b reset anchor missing')
s = s.replace(init_anchor,
              init_anchor + '\n    apsis_controls_performance_reset(&apsis_controls_performance);', 1)

# Main loop owns physical decoding. It publishes only the one-byte selected side
# after the dispatcher has already updated the shared String split/state.
dispatch_anchor = '''    apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank,
                            &chain_sequence, &mother_pending, current, &input,
                            last_abs_step, &result);
'''
if dispatch_anchor not in s:
    raise SystemExit('FAIL: P9H4a dispatcher result anchor missing')
s = s.replace(dispatch_anchor, dispatch_anchor + '''
    if (result.string_pluck)
        apsis_controls_performance_queue_string(&apsis_controls_performance,
                                                result.string_side);
''', 1)

# The M4b transient pool is audio-owned. Consume the main->audio byte only at a
# callback boundary, after the current block's shared runtime has advanced. The
# new transient therefore begins on the next callback: at most one block late,
# never early. Fold the pending check into the already-existing Rhythm-observer
# branch to minimize idle callback overhead.
old = '''        if (rhythm_observation.valid)
            (void)apsis_string_rhythm_trigger_event(&string_transient_pool,
                                                    &apsis,
                                                    &string_pitch_memory,
                                                    &rhythm_observation.event);'''
new = '''        if (rhythm_observation.valid || apsis_controls_performance.string_request) {
            if (rhythm_observation.valid)
                (void)apsis_string_rhythm_trigger_event(&string_transient_pool,
                                                        &apsis,
                                                        &string_pitch_memory,
                                                        &rhythm_observation.event);
            if (apsis_controls_performance.string_request) {
                uint8_t manual_side;
                if (apsis_controls_performance_take_string(
                        &apsis_controls_performance, apsis_runtime.cursor_sample,
                        &manual_side))
                    (void)apsis_string_manual_trigger(&string_transient_pool,
                                                      &apsis,
                                                      &string_pitch_memory,
                                                      manual_side);
            }
        }'''
if s.count(old) != 1:
    raise SystemExit(f'FAIL: P9H4a String audio handoff anchor count {s.count(old)} != 1')
s = s.replace(old, new, 1)

p.write_text(s)
PY

grep -q '#include "apsis_controls_performance.c"' work/apsis/firmware/src/dinghy.c
grep -q '#include "apsis_string_manual_audio.c"' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_performance_queue_string(&apsis_controls_performance' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_string_manual_trigger(&string_transient_pool' work/apsis/firmware/src/dinghy.c
grep -q 'APSIS_CONTROLS_STRING_MANUAL_DEBOUNCE_SAMPLES 1323u' work/apsis/firmware/src/apsis_controls_performance.h
grep -q 'APSIS_STRING_MANUAL_KEY_LEVEL_Q15 APSIS_Q15_MILLI(80)' work/apsis/firmware/src/apsis_string_manual_audio.h
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
main = s[s.index('static uint8_t apsis_p9h3_dispatch('):b]
if 'apsis_controls_performance_queue_string(' in audio:
    raise SystemExit('FAIL: P9H4a main-loop producer leaked into app_audio')
if 'apsis_string_manual_trigger(' in main:
    raise SystemExit('FAIL: P9H4a transient pool mutated from main-loop dispatcher')
for required in ('apsis_controls_performance_take_string(',
                 'apsis_string_manual_trigger(&string_transient_pool'):
    if required not in audio:
        raise SystemExit(f'FAIL: P9H4a audio consumer missing: {required}')
if 'apsis_bloom_dsp_trigger_binding(' in main:
    raise SystemExit('FAIL: P9H4a stole H4b Bloom manual audition')
print('PASS: P9H4a String intent crosses one-byte main->audio mailbox and P9-only manual audio reuses the frozen transient pool')
PY

echo "PASS: P9H4a direct String performance bound above frozen H3b target"
