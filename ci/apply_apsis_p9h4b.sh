#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# H4b layers manual Bloom audition strictly above the green H4a String target.
# The structural dispatcher remains H2/H3 authority; H4b only carries its
# already-frozen performance intents across main->audio ownership.
bash ci/apply_apsis_p9h4a.sh

cp apsis/apsis_bloom_manual_audio.h apsis/apsis_bloom_manual_audio.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_string_manual_audio.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P9H4b H4a header anchor missing')
s = s.replace(h_anchor, h_anchor + '\n#include "apsis_bloom_manual_audio.h"', 1)

c_anchor = '#include "apsis_string_manual_audio.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P9H4b H4a source anchor missing')
s = s.replace(c_anchor, c_anchor + '\n#include "apsis_bloom_manual_audio.c"', 1)

state_anchor = 'static ApsisControlsPerformanceMailbox apsis_controls_performance;'
if state_anchor not in s:
    raise SystemExit('FAIL: P9H4b H4a mailbox state anchor missing')
s = s.replace(state_anchor,
              state_anchor + '\nstatic ApsisBloomManualAudio apsis_bloom_manual_audio;', 1)

init_anchor = '    apsis_controls_performance_reset(&apsis_controls_performance);'
if init_anchor not in s:
    raise SystemExit('FAIL: P9H4b H4a reset anchor missing')
s = s.replace(init_anchor,
              init_anchor + '\n    apsis_bloom_manual_audio_reset(&apsis_bloom_manual_audio);', 1)

# H2 already distinguishes core vs petal. P9E guarantees REC-held note edges are
# suppressed during grow, so the current held REC bit is the authoritative way
# to classify newly-crossed grow petals without adding shadow gesture state.
producer = '''    if (result.string_pluck)
        apsis_controls_performance_queue_string(&apsis_controls_performance,
                                                result.string_side);
'''
if producer not in s:
    raise SystemExit('FAIL: P9H4b H4a producer anchor missing')
s = s.replace(producer, producer + '''    if (result.bloom_core || result.bloom_petal_mask) {
        uint8_t bloom_grow_mask =
            (input.buttons_held & (1u << BTN_REC)) ? result.bloom_petal_mask : 0u;
        /* bloom_request is a 16-bit read/modify/write shared with the audio ISR.
         * Keep the producer critical section tiny; no DSP work is permitted here. */
        fm1_irq_off();
        apsis_controls_performance_queue_bloom(&apsis_controls_performance,
                                               result.bloom_core,
                                               result.bloom_petal_mask,
                                               bloom_grow_mask);
        fm1_irq_on();
    }
''', 1)

# H4a consumes String after the shared runtime has advanced the current block.
# Extend that exact block with Bloom. The consumer therefore treats
# apsis_runtime.cursor_sample as the NEXT callback start: manual transients can
# be at most one 256-frame callback late, never early. Existing scheduled Bloom
# and every M3c event remain untouched.
old = '''        if (rhythm_observation.valid || apsis_controls_performance.string_request) {
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
new = '''        if (rhythm_observation.valid ||
            apsis_controls_performance.string_request ||
            apsis_controls_performance.bloom_request ||
            apsis_bloom_manual_pending_count(&apsis_bloom_manual_audio)) {
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
            if (apsis_controls_performance.bloom_request ||
                apsis_bloom_manual_pending_count(&apsis_bloom_manual_audio)) {
                ApsisControlsBloomRequest bloom_request;
                ApsisLatePendingGravity bloom_pending;
                uint64_t bloom_block_start = apsis_runtime.cursor_sample;
                uint8_t bloom_started = 0u;
                bloom_pending.active = apsis_runtime.gravity_pending.active;
                bloom_pending.degree = apsis_runtime.gravity_pending.degree;
                bloom_pending.ring = apsis_runtime.gravity_pending.ring;
                bloom_pending.target_sample = apsis_runtime.gravity_pending.target_sample;

                if (apsis_controls_performance_take_bloom(
                        &apsis_controls_performance, &bloom_request)) {
                    if (bloom_request.petal_mask) {
                        if (apsis_bloom_manual_trigger_petals(
                                &apsis_bloom_manual_audio, &bloom_dsp, &apsis,
                                &bloom_voicing_state, &bloom_pending,
                                bloom_request.petal_mask, bloom_request.grow_mask,
                                bloom_block_start))
                            bloom_started = 1u;
                    }
                    if (bloom_request.core && apsis.bloom.sound &&
                        apsis.bloom.level_q15) {
                        uint32_t walk_random = m3c_random(&apsis_runtime, 0x50394257u);
                        uint32_t pick_random = m3c_random(&apsis_runtime, 0x50394250u);
                        if (apsis_bloom_manual_trigger_core(
                                &apsis_bloom_manual_audio, &apsis,
                                &bloom_voicing_state, &bloom_pending,
                                bloom_block_start, walk_random, pick_random))
                            bloom_started = 1u;
                    }
                }
                /* Process after a new CORE is queued so its zero-offset first
                 * petal is already bound for the next callback. */
                if (apsis_bloom_manual_pending_count(&apsis_bloom_manual_audio) &&
                    apsis_bloom_manual_process(&apsis_bloom_manual_audio,
                                               &bloom_dsp, &apsis,
                                               &bloom_pending,
                                               bloom_block_start, n))
                    bloom_started = 1u;
                if (bloom_started)
                    bloom_audio_started = 1u;
            }
        }'''
if s.count(old) != 1:
    raise SystemExit(f'FAIL: P9H4b H4a audio consumer anchor count {s.count(old)} != 1')
s = s.replace(old, new, 1)

p.write_text(s)
PY

grep -q '#include "apsis_bloom_manual_audio.c"' work/apsis/firmware/src/dinghy.c
grep -q 'static ApsisBloomManualAudio apsis_bloom_manual_audio;' work/apsis/firmware/src/dinghy.c
grep -q 'fm1_irq_off();' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_controls_performance_queue_bloom(&apsis_controls_performance' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_bloom_manual_trigger_petals(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_bloom_manual_trigger_core(' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_bloom_manual_process(' work/apsis/firmware/src/dinghy.c
grep -q 'm3c_random(&apsis_runtime, 0x50394257u)' work/apsis/firmware/src/dinghy.c
grep -q 'm3c_random(&apsis_runtime, 0x50394250u)' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
main = s[s.index('static uint8_t apsis_p9h3_dispatch('):b]
for forbidden in ('apsis_bloom_manual_trigger_petals(',
                  'apsis_bloom_manual_trigger_core(',
                  'apsis_bloom_manual_process(',
                  'apsis_bloom_dsp_trigger_binding('):
    if forbidden in main:
        raise SystemExit(f'FAIL: P9H4b Bloom DSP/manual queue mutated from main loop: {forbidden}')
for required in ('apsis_controls_performance_take_bloom(',
                 'apsis_bloom_manual_trigger_petals(',
                 'apsis_bloom_manual_trigger_core(',
                 'apsis_bloom_manual_process('):
    if required not in audio:
        raise SystemExit(f'FAIL: P9H4b audio consumer missing: {required}')
# Historical source engines and persistence remain frozen above the P9 layer.
for required in ('project_store.save_pending && !apsis_runtime.transport.running',
                 'apsis_project_fm1_service_save('):
    if required not in s:
        raise SystemExit(f'FAIL: P9H4b disturbed P7 deferred save path: {required}')
print('PASS: P9H4b Bloom manual performance is main->ISR mailboxed, shared-RNG and audio-owned')
PY

echo "PASS: P9H4b direct Bloom performance bound above green H4a target"
