#include <assert.h>
#include <stdio.h>

#include "apsis_runtime_m5a.h"

static uint64_t ms_samples(uint32_t ms)
{
    return (uint64_t)(44u * ms) + (uint64_t)((100u * ms + 500u) / 1000u);
}

static void process_frames(ApsisRuntime *r, ApsisState *s,
                           ApsisKeepBank *bank, ApsisChainSequence *chain,
                           ApsisMotherPending *mother,
                           ApsisBloomVoicingState *bloom,
                           ApsisPersistentAudioQueue *queue,
                           ApsisStringPitchMemory *pitch,
                           ApsisStructuralAudioStats *stats,
                           uint32_t frames)
{
    (void)apsis_runtime_process_structural_audio(
        r, s, bank, chain, mother, bloom, frames,
        0, 0, 0, 0, 0, 0, queue, pitch, stats);
}

int main(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisPersistentAudioQueue queue;
    ApsisStringPitchMemory pitch;
    ApsisStructuralAudioStats stats;
    uint64_t observe;

    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 0u, 0x4d35414fu);
    apsis_keep_bank_reset(&bank);
    apsis_chain_sequence_reset(&chain);
    apsis_mother_pending_reset(&mother);
    apsis_bloom_voicing_reset(&bloom);
    apsis_persistent_audio_queue_reset(&queue);
    apsis_string_pitch_memory_reset(&pitch);
    apsis_structural_audio_stats_reset(&stats);

    apsis_runtime_start(&r);
    process_frames(&r, &s, &bank, &chain, &mother, &bloom,
                   &queue, &pitch, &stats, 256u);

    s.gravity.change_grid = 12u;
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 5u, 1u,
                                         r.cursor_sample, 0u) == 3u);
    observe = r.gravity_pending.target_sample - ms_samples(18u);
    while (r.cursor_sample < observe) {
        uint64_t remain = observe - r.cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        process_frames(&r, &s, &bank, &chain, &mother, &bloom,
                       &queue, &pitch, &stats, frames);
    }
    process_frames(&r, &s, &bank, &chain, &mother, &bloom,
                   &queue, &pitch, &stats, 1u);

    assert(stats.gravity_captures == 1u);
    assert(stats.capture_drops == 0u);
    assert(queue.count == 1u);
    assert(queue.event[0].sample == r.gravity_pending.target_sample ||
           r.gravity_pending.active == 0u);
    assert(queue.overflowed == 0u);
    puts("PASS: APSIS M5a structural capture works with Bloom audio observer absent");
    return 0;
}
