#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_drift_dsp.h"
#include "apsis_persistent_audio.h"

static void init_pair(ApsisState *old_state, ApsisState *new_state,
                      ApsisStringPitchMemory *old_mem,
                      ApsisStringPitchMemory *new_mem,
                      ApsisStringDsp *a_string, ApsisStringDsp *b_string,
                      ApsisDriftDsp *a_drift, ApsisDriftDsp *b_drift,
                      ApsisPersistentAudioTargets *future)
{
    apsis_state_default(old_state);
    *new_state = *old_state;
    new_state->gravity.degree = 4u;
    new_state->gravity.voicing = 2u;
    new_state->drift.amount_q15 = APSIS_Q15_MILLI(820);

    apsis_string_pitch_memory_reset(old_mem);
    apsis_string_pitch_memory_reset(new_mem);
    apsis_string_dsp_reset(a_string);
    apsis_string_dsp_reset(b_string);
    apsis_drift_dsp_reset(a_drift);
    apsis_drift_dsp_reset(b_drift);

    apsis_string_dsp_update(a_string, old_state, old_mem, 1u);
    apsis_drift_dsp_update(a_drift, old_state, 1u);
    apsis_string_dsp_update(b_string, old_state, new_mem, 1u);
    apsis_drift_dsp_update(b_drift, old_state, 1u);

    /* Use a separate pitch-memory snapshot for the future state; this mirrors
     * the early structural callback computing a future WebAudio target. */
    apsis_persistent_audio_capture(new_state, new_mem, 1u, future);
}

static void test_segmented_render_matches_manual_boundary(void)
{
    ApsisState old_state, new_state;
    ApsisStringPitchMemory old_mem, new_mem;
    ApsisStringDsp auto_string, manual_string;
    ApsisDriftDsp auto_drift, manual_drift;
    ApsisPersistentAudioTargets future;
    ApsisPersistentAudioQueue queue;
    int32_t automatic[256 * 2];
    int32_t manual[256 * 2];
    const uint64_t block_start = 1000u;
    const uint64_t boundary = 1123u;
    const uint32_t first = (uint32_t)(boundary - block_start);

    init_pair(&old_state, &new_state, &old_mem, &new_mem,
              &auto_string, &manual_string, &auto_drift, &manual_drift, &future);
    apsis_persistent_audio_queue_reset(&queue);
    assert(apsis_persistent_audio_queue_push(&queue, boundary, &future));
    memset(automatic, 0, sizeof(automatic));
    memset(manual, 0, sizeof(manual));

    apsis_persistent_audio_render_block(&queue, &auto_string, &auto_drift,
                                        automatic, 256u, block_start);

    apsis_string_dsp_render(&manual_string, manual, first);
    apsis_drift_dsp_render(&manual_drift, manual, first);
    apsis_persistent_audio_apply(&manual_string, &manual_drift, &future);
    apsis_string_dsp_render(&manual_string, manual + first * 2u, 256u - first);
    apsis_drift_dsp_render(&manual_drift, manual + first * 2u, 256u - first);

    assert(queue.count == 0u);
    assert(memcmp(automatic, manual, sizeof(automatic)) == 0);
    assert(memcmp(&auto_string, &manual_string, sizeof(auto_string)) == 0);
    assert(memcmp(&auto_drift, &manual_drift, sizeof(auto_drift)) == 0);
}

static void test_callback_end_event_is_not_early(void)
{
    ApsisState old_state, new_state;
    ApsisStringPitchMemory old_mem, new_mem;
    ApsisStringDsp auto_string, reference_string;
    ApsisDriftDsp auto_drift, reference_drift;
    ApsisPersistentAudioTargets future;
    ApsisPersistentAudioQueue queue;
    int32_t automatic[256 * 2];
    int32_t reference[256 * 2];
    const uint64_t block_start = 4096u;
    const uint64_t block_end = block_start + 256u;

    init_pair(&old_state, &new_state, &old_mem, &new_mem,
              &auto_string, &reference_string, &auto_drift, &reference_drift, &future);
    apsis_persistent_audio_queue_reset(&queue);
    assert(apsis_persistent_audio_queue_push(&queue, block_end, &future));
    memset(automatic, 0, sizeof(automatic));
    memset(reference, 0, sizeof(reference));

    apsis_persistent_audio_render_block(&queue, &auto_string, &auto_drift,
                                        automatic, 256u, block_start);
    apsis_string_dsp_render(&reference_string, reference, 256u);
    apsis_drift_dsp_render(&reference_drift, reference, 256u);

    assert(queue.count == 1u);
    assert(memcmp(automatic, reference, sizeof(automatic)) == 0);
    assert(memcmp(&auto_string, &reference_string, sizeof(auto_string)) == 0);
    assert(memcmp(&auto_drift, &reference_drift, sizeof(auto_drift)) == 0);

    /* It becomes due at the start of the next callback. */
    memset(automatic, 0, sizeof(automatic));
    apsis_persistent_audio_render_block(&queue, &auto_string, &auto_drift,
                                        automatic, 1u, block_end);
    assert(queue.count == 0u);
}

int main(void)
{
    test_segmented_render_matches_manual_boundary();
    test_callback_end_event_is_not_early();
    puts("PASS: APSIS M5a exact segmented persistent render boundaries");
    return 0;
}
