#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Preserve the merged M4f generator as a byte-for-byte bypass authority. When
# Bloom DSP is disabled this wrapper deliberately does nothing beyond M4f.
bash ci/apply_apsis.sh

if [[ "${APSIS_CI_BLOOM_DSP_OFF:-0}" == "1" ]]; then
  echo "PASS: M4g Bloom DSP bypass uses exact frozen M4f overlay"
  exit 0
fi

VERSION=${APSIS_VERSION:-0.1-dev}

cp apsis/apsis_bloom_audio.h apsis/apsis_bloom_audio.c \
   apsis/apsis_bloom_audio_runtime.h apsis/apsis_bloom_audio_runtime.c \
   apsis/apsis_bloom_dsp.h apsis/apsis_bloom_dsp.c \
   apsis/apsis_runtime_m4g.h apsis/apsis_runtime_m4g.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

# Bloom acoustic/runtime/DSP code is layered above the already-proven M4f
# single-TU target. runtime_m4g.c includes runtime_m4f.c internally.
h_anchor = '#include "apsis_bloom.h"'
if '#include "apsis_bloom_audio.h"' not in s:
    if h_anchor not in s:
        raise SystemExit('FAIL: M4g Bloom header anchor missing')
    s = s.replace(
        h_anchor,
        h_anchor +
        '\n#include "apsis_bloom_audio.h"'
        '\n#include "apsis_bloom_audio_runtime.h"'
        '\n#include "apsis_bloom_dsp.h"'
        '\n#include "apsis_runtime_m4g.h"',
        1)

c_anchor = '#include "apsis_bloom.c"'
if '#include "apsis_bloom_audio.c"' not in s:
    if c_anchor not in s:
        raise SystemExit('FAIL: M4g Bloom source anchor missing')
    s = s.replace(
        c_anchor,
        c_anchor +
        '\n#include "apsis_bloom_audio.c"'
        '\n#include "apsis_bloom_audio_runtime.c"'
        '\n#include "apsis_bloom_dsp.c"',
        1)

runtime_old = '#include "apsis_runtime_m4f.c"'
if runtime_old not in s:
    raise SystemExit('FAIL: M4g runtime include anchor missing')
s = s.replace(runtime_old, '#include "apsis_runtime_m4g.c"', 1)

state_anchor = 'static uint8_t spiral_audio_started;'
if state_anchor not in s:
    raise SystemExit('FAIL: M4g Bloom state anchor missing')
s = s.replace(
    state_anchor,
    state_anchor +
    '\nstatic ApsisBloomAudioRuntime bloom_audio_runtime;'
    '\nstatic ApsisBloomDsp bloom_dsp;'
    '\nstatic uint8_t bloom_audio_started;',
    1)

# Keep the exact M4f callback path until an already-planned shared Bloom
# occurrence belongs to the current block. Once activated, M4g remains live so
# pending petals and finite tails survive later mute/state edits.
old = '''        if (!spiral_audio_started && apsis.spiral.sound && apsis.spiral.route_count)
            spiral_audio_started = 1u;
        if (spiral_audio_started) {
            spiral_block_start = apsis_runtime.cursor_sample;
            apsis_runtime_process_spiral_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, 0, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                &spiral_audio_runtime, &rhythm_observation);
            for (spiral_ix = 0u; spiral_ix < spiral_audio_runtime.binding_count; ++spiral_ix)
                (void)apsis_spiral_dsp_trigger_binding(
                    &spiral_dsp, &apsis, &spiral_audio_runtime.binding[spiral_ix],
                    spiral_block_start);
            if (spiral_dsp.hit_trigger_count)
                apsis_spiral_dsp_render(&spiral_dsp, out, n);
        } else if (string_attractor_runtime.length ||
                   apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR) {
            apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                     &keep_bank, &chain_sequence,
                                                     &mother_pending,
                                                     &bloom_voicing_state, n, 0, 0,
                                                     &string_attractor_runtime,
                                                     &rhythm_observation);
        } else {
            apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                           &chain_sequence, &mother_pending,
                                           &bloom_voicing_state, n, 0, 0,
                                           &rhythm_observation);
        }
'''
new = '''        if (!spiral_audio_started && apsis.spiral.sound && apsis.spiral.route_count)
            spiral_audio_started = 1u;
        if (!bloom_audio_started &&
            apsis_runtime_bloom_occurrence_due(
                &apsis_runtime, apsis_runtime.cursor_sample + n))
            bloom_audio_started = 1u;
        if (bloom_audio_started) {
            spiral_block_start = apsis_runtime.cursor_sample;
            apsis_runtime_process_bloom_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, 0, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                spiral_audio_started ? &spiral_audio_runtime : 0,
                &bloom_audio_runtime, &rhythm_observation);
            if (spiral_audio_started) {
                for (spiral_ix = 0u; spiral_ix < spiral_audio_runtime.binding_count; ++spiral_ix)
                    (void)apsis_spiral_dsp_trigger_binding(
                        &spiral_dsp, &apsis, &spiral_audio_runtime.binding[spiral_ix],
                        spiral_block_start);
                if (spiral_dsp.hit_trigger_count)
                    apsis_spiral_dsp_render(&spiral_dsp, out, n);
            }
            {
                uint8_t bloom_ix;
                for (bloom_ix = 0u; bloom_ix < bloom_audio_runtime.binding_count; ++bloom_ix)
                    (void)apsis_bloom_dsp_trigger_binding(
                        &bloom_dsp, &apsis, &bloom_audio_runtime.binding[bloom_ix],
                        spiral_block_start);
                if (bloom_dsp.trigger_count)
                    apsis_bloom_dsp_render(&bloom_dsp, out, n);
            }
        } else if (spiral_audio_started) {
            spiral_block_start = apsis_runtime.cursor_sample;
            apsis_runtime_process_spiral_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, 0, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                &spiral_audio_runtime, &rhythm_observation);
            for (spiral_ix = 0u; spiral_ix < spiral_audio_runtime.binding_count; ++spiral_ix)
                (void)apsis_spiral_dsp_trigger_binding(
                    &spiral_dsp, &apsis, &spiral_audio_runtime.binding[spiral_ix],
                    spiral_block_start);
            if (spiral_dsp.hit_trigger_count)
                apsis_spiral_dsp_render(&spiral_dsp, out, n);
        } else if (string_attractor_runtime.length ||
                   apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR) {
            apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                     &keep_bank, &chain_sequence,
                                                     &mother_pending,
                                                     &bloom_voicing_state, n, 0, 0,
                                                     &string_attractor_runtime,
                                                     &rhythm_observation);
        } else {
            apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                           &chain_sequence, &mother_pending,
                                           &bloom_voicing_state, n, 0, 0,
                                           &rhythm_observation);
        }
'''
if old not in s:
    raise SystemExit('FAIL: M4g callback handoff anchor missing')
s = s.replace(old, new, 1)

p.write_text(s)
PY

# Gate 3 must compile the actual M4g single-TU result, not only the frozen M4f
# base produced by apply_apsis.sh.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) \
  2>&1 | tee -a build/evidence/apsis_host.log

echo "PASS: M4g Bloom overlay layered above frozen M4f target"
