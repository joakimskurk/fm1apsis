#include <stdint.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string_rhythm_audio.h"
#include "apsis_runtime.h"

#define FRAMES 256u
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static void clear_audio(int32_t *out)
{
    uint32_t i;
    for (i = 0u; i < FRAMES * 2u; ++i) out[i] = 0;
}

static uint64_t audio_energy(const int32_t *out)
{
    uint32_t i;
    uint64_t e = 0u;
    for (i = 0u; i < FRAMES * 2u; ++i) {
        int64_t v = out[i];
        e += (uint64_t)(v < 0 ? -v : v);
    }
    return e;
}

static uint8_t live_block(ApsisRuntime *runtime,
                          ApsisState *state,
                          ApsisKeepBank *bank,
                          ApsisChainSequence *chain,
                          ApsisMotherPending *mother,
                          ApsisBloomVoicingState *bloom,
                          ApsisStringPitchMemory *pitch_memory,
                          ApsisStringTransientPool *pool,
                          int32_t *out,
                          ApsisRuntimeStringRhythmObservation *observation)
{
    uint8_t accepted = 0u;
    clear_audio(out);

    /* This is the target app_audio() ordering. Already-active transients render
     * before the shared timeline advances. A newly consumed event is therefore
     * bound only after this callback's samples have been written. */
    apsis_string_transient_render(pool, out, FRAMES);
    apsis_runtime_process_observed(runtime, state, bank, chain, mother, bloom,
                                   FRAMES, 0, 0, observation);
    if (observation->valid)
        accepted = apsis_string_rhythm_trigger_event(pool, state, pitch_memory,
                                                      &observation->event);
    return accepted;
}

int main(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisStringPitchMemory pitch_memory;
    ApsisStringTransientPool pool;
    ApsisRuntimeStringRhythmObservation observation;
    int32_t out[FRAMES * 2u];
    uint32_t block;
    uint8_t accepted;

    apsis_state_default(&state);
    state.string.surface_mode = APSIS_STRING_DUAL;
    state.string.rhythm = APSIS_STRING_RHYTHM_BOW;
    state.string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    state.string.rhythm_every = 1u;
    state.string.pulse_diag = APSIS_STRING_PULSE_FULL;

    apsis_keep_bank_reset(&bank);
    apsis_chain_sequence_reset(&chain);
    apsis_mother_pending_reset(&mother);
    apsis_bloom_voicing_reset(&bloom);
    apsis_string_pitch_memory_reset(&pitch_memory);
    apsis_string_transient_pool_reset(&pool);
    apsis_runtime_reset(&runtime, 44100u, 0u, 0x41505349u);
    apsis_runtime_start(&runtime);

    /* The first Rhythm target is Clock-0 + 26 ms = sample 1147. Four complete
     * callbacks end at sample 1024, so neither observation nor audio is legal
     * before the fifth callback. */
    for (block = 0u; block < 4u; ++block) {
        accepted = live_block(&runtime, &state, &bank, &chain, &mother, &bloom,
                              &pitch_memory, &pool, out, &observation);
        CHECK(accepted == 0u);
        CHECK(observation.valid == 0u);
        CHECK(audio_energy(out) == 0u);
        CHECK(apsis_string_dsp_transient_physical_active(&pool) == 0u);
    }
    CHECK(runtime.cursor_sample == 1024u);

    /* Study 126 freezes the Rhythm mode when scheduling. Turn the live mode off
     * after the event is already queued; the pending BOW event must still bind.
     * Current source/surface/pitch state remains late-read by the audio binder. */
    state.string.rhythm = APSIS_STRING_RHYTHM_OFF;
    accepted = live_block(&runtime, &state, &bank, &chain, &mother, &bloom,
                          &pitch_memory, &pool, out, &observation);
    CHECK(audio_energy(out) == 0u); /* never early inside the target callback */
    CHECK(observation.valid == 1u);
    CHECK(observation.event.type == APSIS_EVENT_STRING_RHYTHM);
    CHECK(observation.event.sample == 1147u);
    CHECK(observation.event.absolute_pulse == 0u);
    CHECK(observation.event.clock_step == 0u);
    CHECK(observation.event.sub_index == APSIS_STRING_RHYTHM_BOW);
    CHECK(accepted == 1u); /* pulse 0 -> Dual left/BODY */
    CHECK(apsis_string_dsp_transient_physical_active(&pool) == 1u);
    CHECK(runtime.event_count[APSIS_EVENT_STRING_RHYTHM] == 1u);
    CHECK(runtime.cursor_sample == 1280u);

    /* The first audible transient block begins at sample 1280, 133 samples
     * after the source target. This is within the explicit M4 callback-boundary
     * policy (<=256 samples late) and cannot precede the target. */
    accepted = live_block(&runtime, &state, &bank, &chain, &mother, &bloom,
                          &pitch_memory, &pool, out, &observation);
    CHECK(accepted == 0u);
    CHECK(observation.valid == 0u);
    CHECK(audio_energy(out) > 0u);
    CHECK(1280u - 1147u < FRAMES);

    CHECK(runtime.queue_overflowed == 0u);
    CHECK(runtime.late_slot_overflowed == 0u);
    CHECK(runtime.scheduler_starved == 0u);
    CHECK(pool.transient_drop_count == 0u);

    puts("PASS: APSIS M4b live Rhythm shared-timeline -> transient handoff");
    return 0;
}
