#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 7 is persistence-only. Generate the exact frozen Phase-6 target first,
# then add project storage without changing audio/scheduler source files.
bash ci/apply_apsis_p6.sh

cp apsis/apsis_project_store.h apsis/apsis_project_store.c \
   apsis/apsis_project_fm1.h apsis/apsis_project_fm1.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

# Pinned Dinghy narrows the underlying FM-1 flash driver to OTA + one settings
# sector. Permit exactly APSIS's two 4 KiB project sectors as a narrow backport.
p = Path('work/apsis/firmware/src/platform.c')
s = p.read_text()
old = '#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u))'
new = '''#define APSIS_PROJECT_FLASH_LO 0x000F4000u
#define APSIS_PROJECT_FLASH_HI 0x000F6000u
#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || \\
                             FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u) || \\
                             FL_IN(off, n, APSIS_PROJECT_FLASH_LO, APSIS_PROJECT_FLASH_HI))'''
if old not in s:
    raise SystemExit('FAIL: P7 pinned Dinghy flash allow-list anchor missing')
s = s.replace(old, new, 1)

# Project NOR reads must happen before ALNK0 starts. Pinned Dinghy enters
# app_main() only after audio/DMA has been enabled, so add one narrow pre-audio
# application hook. The hook does read-only prefetch; all APSIS decode remains
# in app_main after defaults are installed.
decl = 'static void app_audio(int32_t *out, uint32_t n);\nstatic void app_main(void);'
if decl not in s:
    raise SystemExit('FAIL: P7 pre-audio declaration anchor missing')
s = s.replace(decl,
              'static void app_audio(int32_t *out, uint32_t n);\n'
              'static void app_pre_audio_init(void);\n'
              'static void app_main(void);', 1)
start = '    fm1_adc_init();\n    fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);'
if start not in s:
    raise SystemExit('FAIL: P7 pre-audio call anchor missing')
s = s.replace(start,
              '    fm1_adc_init();\n'
              '    app_pre_audio_init();\n'
              '    fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);', 1)
p.write_text(s)

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_space_target.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P7 Phase-6 header anchor missing')
s = s.replace(h_anchor,
              h_anchor + '\n#include "apsis_project_store.h"\n#include "apsis_project_fm1.h"', 1)

c_anchor = '#include "apsis_space_target.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P7 Phase-6 source anchor missing')

# Dinghy intentionally builds the application as one translation unit by
# textually including module .c files. Namespace every private helper in the
# project-format implementation so generic names (for example clamp_q15) can
# never collide with helpers from Spiral or another frozen APSIS module.
helper_names = (
    'project_zero', 'clamp_u8', 'clamp_i8', 'clamp_q15',
    'put_u16', 'put_u32', 'get_u16', 'get_u32', 'put_i8', 'get_i8',
    'normalize_keep', 'encode_keep', 'decode_keep',
    'normalize_cell', 'encode_cell', 'decode_cell',
    'normalize_chain_settings', 'encode_chain_settings',
    'decode_chain_settings',
)
wrapped = [c_anchor]
for name in helper_names:
    wrapped.append(f'#define {name} apsis_p7_{name}')
wrapped.append('#include "apsis_project_store.c"')
for name in reversed(helper_names):
    wrapped.append(f'#undef {name}')
wrapped.append('#include "apsis_project_fm1.c"')
s = s.replace(c_anchor, '\n'.join(wrapped), 1)

state_anchor = 'static ApsisSpaceTarget space_target;'
if state_anchor not in s:
    raise SystemExit('FAIL: P7 Phase-6 state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisProjectFm1 project_store;', 1)

# The platform hook runs before ALNK0/DMA. It only discovers and buffers the
# newest valid project record; no APSIS musical state is touched yet.
pre_anchor = 'static ApsisProjectFm1 project_store;'
pre_fn = '''static ApsisProjectFm1 project_store;

static void app_pre_audio_init(void)
{
    apsis_project_fm1_reset(&project_store);
    (void)apsis_project_fm1_prefetch(&project_store);
}'''
s = s.replace(pre_anchor, pre_fn, 1)

# Once app_main has installed the normal defaults and reset KEEP/CHAIN, apply
# only the already-prefetched RAM record. Empty flash leaves defaults untouched.
load_anchor = '    apsis_chain_sequence_reset(&chain_sequence);'
if load_anchor not in s:
    raise SystemExit('FAIL: P7 project-apply anchor missing')
s = s.replace(
    load_anchor,
    load_anchor +
    '\n    (void)apsis_project_fm1_apply_prefetched('
    '\n        &project_store, &keep_bank, &chain_sequence, &apsis.chain);',
    1)

# Prototype save gesture: once already on KEEP, pressing SAVE again requests a
# project commit. The request itself never touches flash.
input_anchor = '        next = surface_from_buttons(pressed, surface);'
if input_anchor not in s:
    raise SystemExit('FAIL: P7 save-request input anchor missing')
s = s.replace(
    input_anchor,
    '        if ((pressed & (1u << BTN_SAVE)) && surface == SURF_KEEP)\n'
    '            apsis_project_fm1_request_save(&project_store);\n' + input_anchor,
    1)

# NOR erase/program can mask interrupts while XIP is unavailable. Service only
# from app_main and only while transport is stopped. Quiesce DMA around the
# complete transaction, then restart the existing audio engine. No flash write
# path is reachable from app_audio().
service_anchor = '        if (next != surface) surface = next;'
if service_anchor not in s:
    raise SystemExit('FAIL: P7 deferred-save service anchor missing')
service = '''        if (next != surface) surface = next;
        if (project_store.save_pending && !apsis_runtime.transport.running) {
            uint32_t i;
            runtime_ready = 0u;
            fm1_audio_stop();
            (void)apsis_project_fm1_service_save(
                &project_store, &keep_bank, &chain_sequence, &apsis.chain);
            for (i = 0u; i < (2u * HALF_WORDS); ++i) abuf[i] = 0;
            fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);
            runtime_ready = 1u;
        }'''
s = s.replace(service_anchor, service, 1)
p.write_text(s)
PY

# Static guardrails: the generated target must own exactly the intended sectors,
# prefetch before audio starts, and keep all erase/program calls out of app_audio.
grep -q 'APSIS_PROJECT_FLASH_LO 0x000F4000u' work/apsis/firmware/src/platform.c
grep -q 'APSIS_PROJECT_FLASH_HI 0x000F6000u' work/apsis/firmware/src/platform.c
grep -q 'app_pre_audio_init();' work/apsis/firmware/src/platform.c
grep -q 'apsis_project_fm1_prefetch' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_project_fm1_apply_prefetched' work/apsis/firmware/src/dinghy.c
grep -q 'project_store.save_pending && !apsis_runtime.transport.running' work/apsis/firmware/src/dinghy.c
grep -q '#define clamp_q15 apsis_p7_clamp_q15' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
for forbidden in ('fl_erase4k(', 'fl_write(', 'apsis_project_fm1_service_save('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P7 flash write leaked into app_audio: {forbidden}')
print('PASS: P7 project reads prefetch before audio; writes stay outside app_audio')
PY
