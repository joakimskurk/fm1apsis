#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "apsis_runtime.h"

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, 0x12345678u);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static uint8_t process_observed(ApsisRuntime *r, ApsisState *s,
                                ApsisKeepBank *bank, ApsisChainSequence *chain,
                                ApsisMotherPending *mother,
                                ApsisBloomVoicingState *bloom,
                                uint32_t frames,
                                ApsisRuntimeStringRhythmObservation *obs)
{
    return apsis_runtime_process_observed(r, s, bank, chain, mother, bloom,
                                          frames, 0, 0, obs);
}

static void process_to_no_rhythm(ApsisRuntime *r, ApsisState *s,
                                 ApsisKeepBank *bank, ApsisChainSequence *chain,
                                 ApsisMotherPending *mother,
                                 ApsisBloomVoicingState *bloom,
                                 uint64_t target)
{
    while (r->cursor_sample < target) {
        ApsisRuntimeStringRhythmObservation obs;
        uint64_t remain = target - r->cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        assert(process_observed(r, s, bank, chain, mother, bloom, frames, &obs) == 0u);
        assert(obs.valid == 0u);
    }
}

static void test_exact_default_events(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisRuntimeStringRhythmObservation obs;
    const uint64_t first = 1147u;              /* pulse 0 + nearest 26 ms */
    const uint64_t third = 36750u + 1147u;     /* every=2 => pulse 2 */

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.string.rhythm = APSIS_STRING_RHYTHM_BOW;
    s.string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s.string.rhythm_every = 2u;
    apsis_runtime_start(&r);

    process_to_no_rhythm(&r, &s, &bank, &chain, &mother, &bloom, first);
    assert(r.cursor_sample == first);
    assert(r.event_count[APSIS_EVENT_STRING_RHYTHM] == 0u);
    assert(process_observed(&r, &s, &bank, &chain, &mother, &bloom, 1u, &obs) == 1u);
    assert(obs.valid == 1u);
    assert(obs.event.type == APSIS_EVENT_STRING_RHYTHM);
    assert(obs.event.sample == first);
    assert(obs.event.absolute_pulse == 0u);
    assert(obs.event.clock_step == 0u);
    assert((obs.event.flags & APSIS_EVENT_FLAG_STEP_ONE) != 0u);
    assert(r.event_count[APSIS_EVENT_STRING_RHYTHM] == 1u);

    process_to_no_rhythm(&r, &s, &bank, &chain, &mother, &bloom, third);
    assert(process_observed(&r, &s, &bank, &chain, &mother, &bloom, 1u, &obs) == 1u);
    assert(obs.valid == 1u);
    assert(obs.event.sample == third);
    assert(obs.event.absolute_pulse == 2u);
    assert(obs.event.clock_step == 2u);
    assert(r.event_count[APSIS_EVENT_STRING_RHYTHM] == 2u);
}

static void test_at_most_one_per_audio_block(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint32_t block, observed = 0u;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.string.rhythm = APSIS_STRING_RHYTHM_BOW;
    s.string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s.string.rhythm_every = 1u;
    s.clock.bpm = 180u;
    s.clock.division_num = 1u;
    s.clock.division_den = 4u;
    s.clock.swing_q15 = APSIS_Q15_MILLI(350);
    apsis_runtime_start(&r);

    for (block = 0u; block < 1000u; ++block) {
        ApsisRuntimeStringRhythmObservation obs;
        uint32_t before = r.event_count[APSIS_EVENT_STRING_RHYTHM];
        uint8_t got = process_observed(&r, &s, &bank, &chain, &mother, &bloom,
                                       256u, &obs);
        uint32_t delta = r.event_count[APSIS_EVENT_STRING_RHYTHM] - before;
        assert(delta <= 1u);
        assert((got ? 1u : 0u) == delta);
        assert((obs.valid ? 1u : 0u) == delta);
        if (got) {
            observed++;
            assert(obs.event.type == APSIS_EVENT_STRING_RHYTHM);
            assert(obs.event.sample < r.cursor_sample);
        }
    }
    assert(observed > 50u);
    assert(r.queue_overflowed == 0u);
    assert(r.scheduler_starved == 0u);
}

int main(void)
{
    assert(sizeof(ApsisRuntime) == 21928u);
    test_exact_default_events();
    test_at_most_one_per_audio_block();
    puts("PASS: M4b shared-timeline String Rhythm observation without private scheduling");
    return 0;
}
