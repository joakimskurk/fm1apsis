#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# H3b leaves the already-proven H3 interaction/audio path intact and only
# coalesces persistent KEEP/CHAIN writes. Flash remains Phase-7 journaled and is
# still serviced outside app_audio with audio/DMA quiesced.
bash ci/apply_apsis_p9h.sh

cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Iapsis \
  apsis/apsis_controls_dirty.c tests/test_controls_dirty.c \
  -o /tmp/test_controls_dirty
/tmp/test_controls_dirty
cc -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -Iapsis \
  apsis/apsis_controls_dirty.c tests/test_controls_dirty.c \
  -o /tmp/test_controls_dirty_san
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  /tmp/test_controls_dirty_san

cp apsis/apsis_controls_dirty.h apsis/apsis_controls_dirty.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_controls_dispatch.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P9H3b dispatcher header anchor missing')
s = s.replace(h_anchor, h_anchor + '\n#include "apsis_controls_dirty.h"', 1)

c_anchor = '#include "apsis_controls_dispatch.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P9H3b dispatcher source anchor missing')
s = s.replace(c_anchor, c_anchor + '\n#include "apsis_controls_dirty.c"', 1)

state_anchor = 'static ApsisControlsSession apsis_controls_session;'
if state_anchor not in s:
    raise SystemExit('FAIL: P9H3b control-session state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisControlsProjectDirty apsis_project_dirty;', 1)

init_anchor = '    apsis_controls_session_reset(&apsis_controls_session);'
if init_anchor not in s:
    raise SystemExit('FAIL: P9H3b control-session reset anchor missing')
s = s.replace(init_anchor,
              init_anchor + '\n    apsis_controls_project_dirty_reset(&apsis_project_dirty);', 1)

# Phase 7 used SAVE-on-KEEP as a temporary explicit flash gesture before KEEP
# capture existed. P9 now owns SAVE as KEEP navigation/capture, so remove that
# obsolete double meaning. Persistence is driven only by project_dirty below.
legacy = '''        if ((pressed & (1u << BTN_SAVE)) && surface == SURF_KEEP)
            apsis_project_fm1_request_save(&project_store);
        next = surface_from_buttons(pressed, surface);'''
if legacy not in s:
    raise SystemExit('FAIL: P9H3b legacy P7 SAVE gesture anchor missing')
s = s.replace(legacy, '        next = surface_from_buttons(pressed, surface);', 1)

old = '''    if (result.project_dirty)
        apsis_project_fm1_request_save(&project_store);'''
new = '''    if (result.project_dirty)
        apsis_controls_project_dirty_mark(&apsis_project_dirty,
                                          apsis_runtime.cursor_sample);'''
if old not in s:
    raise SystemExit('FAIL: P9H3b H3 dirty-request anchor missing')
s = s.replace(old, new, 1)

service_anchor = '        if (project_store.save_pending && !apsis_runtime.transport.running) {'
if service_anchor not in s:
    raise SystemExit('FAIL: P9H3b Phase-7 save-service anchor missing')
service = '''        if (apsis_controls_project_dirty_take_due(
                &apsis_project_dirty, apsis_runtime.cursor_sample,
                apsis_runtime.transport.running))
            apsis_project_fm1_request_save(&project_store);
        if (project_store.save_pending && !apsis_runtime.transport.running) {'''
s = s.replace(service_anchor, service, 1)

p.write_text(s)
PY

grep -q '#include "apsis_controls_dirty.c"' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_project_dirty_mark(&apsis_project_dirty' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_project_dirty_take_due(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_project_fm1_request_save(&project_store)' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
legacy = 'if ((pressed & (1u << BTN_SAVE)) && surface == SURF_KEEP)'
if legacy in s:
    raise SystemExit('FAIL: P9H3b legacy SAVE-on-KEEP flash gesture survived')
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
for forbidden in ('apsis_controls_project_dirty_mark(',
                  'apsis_controls_project_dirty_take_due(',
                  'apsis_project_fm1_request_save(',
                  'apsis_project_fm1_service_save('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P9H3b persistence work leaked into app_audio: {forbidden}')
if s.count('apsis_project_fm1_request_save(&project_store)') != 1:
    raise SystemExit('FAIL: P9H3b expected exactly one coalesced save-request site')
print('PASS: P9H3b project writes are quiet-window coalesced, transport-deferred and main-loop-only')
PY

echo "PASS: P9H3b persistence debounce layered above frozen H3 target"
