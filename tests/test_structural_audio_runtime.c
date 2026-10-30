#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_runtime_m5a.h"

static uint64_t ms_samples(uint32_t ms)
{
    return (uint64_t)(44u * ms) + (uint64_t)((100u * ms + 500u) / 1000u);
}

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom, ApsisSpiralAudioRuntime *spiral,
                     ApsisBloomAudioRuntime *bloom_audio,
                     ApsisStringPitchMemory *pitch_memory, uint32_t seed)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, seed);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
    apsis_spiral_audio_runtime_reset(spiral);
    apsis_bloom_audio_runtime_reset(bloom_audio);
    apsis_string_pitch_memory_reset(pitch_memory);
}

static uint8_t mother_apply_test(ApsisState *s,
                                 const ApsisMotherPendingItem *item,
                                 void *user)
{
    uint32_t *calls = (uint32_t *)user;
    if (calls) (*calls)++;
    if (!s || !item) return 0u;
    if (item->kind == APSIS_MOTHER_PENDING_PARAM && item->key == 10u) {
        s->drift.amount_q15 = (uint16_t)item->value;
        return 1u;
    }
    return 0u;
}

static void process_frames(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                           ApsisChainSequence *chain, ApsisMotherPending *mother,
                           ApsisBloomVoicingState *bloom,
                           ApsisSpiralAudioRuntime *spiral,
                           ApsisBloomAudioRuntime *bloom_audio,
                           ApsisPersistentAudioQueue *queue,
                           ApsisStringPitchMemory *pitch_memory,
                           ApsisStructuralAudioStats *stats,
                           uint32_t frames,
                           ApsisRuntimeMotherApplyFn mother_apply,
                           void *mother_user)
{
    (void)apsis_runtime_process_structural_audio(
        r, s, bank, chain, mother, bloom, frames,
        mother_apply, mother_user, 0, spiral, bloom_audio, 0,
        queue, pitch_memory, stats);
}

static void process_to(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                       ApsisChainSequence *chain, ApsisMotherPending *mother,
                       ApsisBloomVoicingState *bloom,
                       ApsisSpiralAudioRuntime *spiral,
                       ApsisBloomAudioRuntime *bloom_audio,
                       ApsisPersistentAudioQueue *queue,
                       ApsisStringPitchMemory *pitch_memory,
                       ApsisStructuralAudioStats *stats,
                       uint64_t target,
                       ApsisRuntimeMotherApplyFn mother_apply,
                       void *mother_user)
{
    while (r->cursor_sample < target) {
        uint64_t remain = target - r->cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        process_frames(r, s, bank, chain, mother, bloom, spiral, bloom_audio,
                       queue, pitch_memory, stats, frames,
                       mother_apply, mother_user);
    }
}

static void test_null_queue_is_exact_m4g(void)
{
    ApsisState a, b;
    ApsisRuntime ra, rb;
    ApsisKeepBank ka, kb;
    ApsisChainSequence ca, cb;
    ApsisMotherPending ma, mb;
    ApsisBloomVoicingState va, vb;
    ApsisSpiralAudioRuntime sa, sb;
    ApsisBloomAudioRuntime ba, bb;
    ApsisStringPitchMemory pitch;
    uint32_t i;

    init_all(&a, &ra, &ka, &ca, &ma, &va, &sa, &ba, &pitch, 0x53545255u);
    init_all(&b, &rb, &kb, &cb, &mb, &vb, &sb, &bb, &pitch, 0x53545255u);
    apsis_runtime_start_bloom_audio(&ra, &a, 0, &sa, &ba);
    apsis_runtime_start_bloom_audio(&rb, &b, 0, &sb, &bb);

    for (i = 0u; i < 64u; ++i) {
        (void)apsis_runtime_process_bloom_audio(&ra, &a, &ka, &ca, &ma, &va,
                                                256u, 0, 0, 0, &sa, &ba, 0);
        (void)apsis_runtime_process_structural_audio(&rb, &b, &kb, &cb, &mb, &vb,
                                                     256u, 0, 0, 0, &sb, &bb, 0,
                                                     0, &pitch, 0);
    }

    assert(sizeof(ApsisRuntime) == 21928u);
    assert(memcmp(&ra, &rb, sizeof(ra)) == 0);
    assert(memcmp(&a, &b, sizeof(a)) == 0);
    assert(memcmp(&ka, &kb, sizeof(ka)) == 0);
    assert(memcmp(&ca, &cb, sizeof(ca)) == 0);
    assert(memcmp(&ma, &mb, sizeof(ma)) == 0);
    assert(memcmp(&va, &vb, sizeof(va)) == 0);
    assert(memcmp(&sa, &sb, sizeof(sa)) == 0);
    assert(memcmp(&ba, &bb, sizeof(ba)) == 0);
}

static void test_gravity_state_early_audio_target_future(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime spiral;
    ApsisBloomAudioRuntime bloom_audio;
    ApsisStringPitchMemory pitch;
    ApsisPersistentAudioQueue queue;
    ApsisStructuralAudioStats stats;
    ApsisStringDsp string_dsp;
    ApsisDriftDsp drift_dsp;
    const ApsisPersistentAudioEvent *audio_event;
    uint64_t target, observe;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
             &pitch, 0x47524156u);
    apsis_persistent_audio_queue_reset(&queue);
    apsis_structural_audio_stats_reset(&stats);
    apsis_string_dsp_reset(&string_dsp);
    apsis_drift_dsp_reset(&drift_dsp);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral, &bloom_audio);
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 256u, 0, 0);
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u, 2u,
                                         r.cursor_sample, 0u) == 3u);
    target = r.gravity_pending.target_sample;
    observe = target - ms_samples(18u);
    assert(observe > r.cursor_sample);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
               &queue, &pitch, &stats, observe, 0, 0);
    assert(s.gravity.degree == 0u);
    assert(queue.count == 0u);

    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 1u, 0, 0);
    assert(s.gravity.degree == 4u && s.gravity.voicing == 2u);
    assert(stats.gravity_captures == 1u && stats.capture_drops == 0u);
    assert(queue.count == 1u);
    audio_event = apsis_persistent_audio_queue_peek(&queue);
    assert(audio_event && audio_event->sample == target);
    assert(r.cursor_sample < target);
    assert(apsis_persistent_audio_apply_through(&queue, target - 1u,
                                                &string_dsp, &drift_dsp) == 0u);
    assert(queue.count == 1u);
    assert(apsis_persistent_audio_apply_through(&queue, target,
                                                &string_dsp, &drift_dsp) == 1u);
    assert(queue.count == 0u);
}

static void test_mother_commit_captures_true_target(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime spiral;
    ApsisBloomAudioRuntime bloom_audio;
    ApsisStringPitchMemory pitch;
    ApsisPersistentAudioQueue queue;
    ApsisStructuralAudioStats stats;
    const uint64_t target = 220500u;
    const uint64_t observe = target - 1147u;
    uint32_t calls = 0u;
    const ApsisPersistentAudioEvent *e;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
             &pitch, 0x4d4f5448u);
    apsis_persistent_audio_queue_reset(&queue);
    apsis_structural_audio_stats_reset(&stats);
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_mother_set_horizon(&s.mother, &mother, 1u, -1);
    assert(apsis_mother_queue_param(&mother, 10u, APSIS_Q15_MILLI(777), -1));
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral, &bloom_audio);

    /* Preserve the M3c recovery scenario: first target has no dispatcher. */
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 256u, 0, 0);
    assert(apsis_mother_pending_count(&mother) == 1u);
    assert(queue.count == 0u);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
               &queue, &pitch, &stats, observe, mother_apply_test, &calls);
    assert(s.drift.amount_q15 != APSIS_Q15_MILLI(777));
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 1u, mother_apply_test, &calls);
    assert(calls == 1u);
    assert(s.drift.amount_q15 == APSIS_Q15_MILLI(777));
    assert(apsis_mother_pending_count(&mother) == 0u);
    assert(stats.mother_captures == 1u && stats.capture_drops == 0u);
    assert(queue.count == 1u);
    e = apsis_persistent_audio_queue_peek(&queue);
    assert(e && e->sample == target);
    assert(r.cursor_sample < target);
}

static void test_chain_discrete_capture_targets_clock_sample(void)
{
    ApsisState s, source;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime spiral;
    ApsisBloomAudioRuntime bloom_audio;
    ApsisStringPitchMemory pitch;
    ApsisPersistentAudioQueue queue;
    ApsisStructuralAudioStats stats;
    const uint64_t target = 18375u;
    const uint64_t observe = target - 1058u;
    const ApsisPersistentAudioEvent *e;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
             &pitch, 0x43484149u);
    apsis_persistent_audio_queue_reset(&queue);
    apsis_structural_audio_stats_reset(&stats);

    apsis_state_default(&source);
    source.gravity.tonic = 3u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(400);
    source.string.right_split_q15 = APSIS_Q15_MILLI(600);
    source.drift.amount_q15 = APSIS_Q15_MILLI(300);
    bank.slots[0] = apsis_keep_capture(&source);
    source.gravity.tonic = 7u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(350);
    source.string.right_split_q15 = APSIS_Q15_MILLI(650);
    source.drift.amount_q15 = APSIS_Q15_MILLI(760);
    bank.slots[1] = apsis_keep_capture(&source);

    s.chain.scope_key_scale = 1u;
    s.chain.scope_tension = 1u;
    s.chain.scope_drift = 1u;
    chain.count = 2u;
    chain.cells[0] = apsis_chain_default_cell(0u, &s.chain, s.clock.n);
    chain.cells[0].cycles = 0u;
    chain.cells[0].fine_pulses = 1u;
    chain.cells[0].morph_quarters = APSIS_CHAIN_MORPH_NONE;
    chain.cells[1] = apsis_chain_default_cell(1u, &s.chain, s.clock.n);
    chain.cells[1].cycles = 0u;
    chain.cells[1].fine_pulses = 1u;
    chain.cells[1].morph_quarters = APSIS_CHAIN_MORPH_NONE;

    apsis_runtime_chain_start(&r, &s, &chain);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral, &bloom_audio);
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 256u, 0, 0);
    assert(chain.pos == 0u && s.gravity.tonic == 3u);
    assert(stats.chain_captures == 1u);
    assert(queue.count == 1u && queue.event[0].sample == 0u);
    /* Simulate the audio side having consumed the startup target. */
    assert(apsis_persistent_audio_queue_pop(&queue, 0));
    apsis_structural_audio_stats_reset(&stats);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
               &queue, &pitch, &stats, observe, 0, 0);
    assert(chain.pos == 0u && s.gravity.tonic == 3u);
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 1u, 0, 0);
    assert(chain.pos == 1u && s.gravity.tonic == 7u);
    assert(s.drift.amount_q15 == APSIS_Q15_MILLI(760));
    assert(stats.chain_captures == 1u && stats.capture_drops == 0u);
    assert(queue.count == 1u);
    e = apsis_persistent_audio_queue_peek(&queue);
    assert(e && e->sample == target);
    assert(r.cursor_sample < target);
}

int main(void)
{
    test_null_queue_is_exact_m4g();
    test_gravity_state_early_audio_target_future();
    test_mother_commit_captures_true_target();
    test_chain_discrete_capture_targets_clock_sample();
    puts("PASS: APSIS M5a structural runtime persistent-target capture");
    return 0;
}
