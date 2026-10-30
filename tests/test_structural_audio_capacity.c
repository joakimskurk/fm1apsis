#include <assert.h>
#include <stdio.h>

#include "apsis_runtime_m5a.h"

static uint64_t ms_samples(uint32_t ms)
{
    return (uint64_t)(44u * ms) + (uint64_t)((100u * ms + 500u) / 1000u);
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

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom, ApsisSpiralAudioRuntime *spiral,
                     ApsisBloomAudioRuntime *bloom_audio,
                     ApsisStringPitchMemory *pitch)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, 0x4d354143u);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
    apsis_spiral_audio_runtime_reset(spiral);
    apsis_bloom_audio_runtime_reset(bloom_audio);
    apsis_string_pitch_memory_reset(pitch);
}

static void process_frames(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                           ApsisChainSequence *chain, ApsisMotherPending *mother,
                           ApsisBloomVoicingState *bloom,
                           ApsisSpiralAudioRuntime *spiral,
                           ApsisBloomAudioRuntime *bloom_audio,
                           ApsisPersistentAudioQueue *queue,
                           ApsisStringPitchMemory *pitch,
                           ApsisStructuralAudioStats *stats,
                           uint32_t frames,
                           ApsisRuntimeMotherApplyFn mother_apply,
                           void *mother_user)
{
    (void)apsis_runtime_process_structural_audio(
        r, s, bank, chain, mother, bloom, frames,
        mother_apply, mother_user, 0, spiral, bloom_audio, 0,
        queue, pitch, stats);
}

static void process_to(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                       ApsisChainSequence *chain, ApsisMotherPending *mother,
                       ApsisBloomVoicingState *bloom,
                       ApsisSpiralAudioRuntime *spiral,
                       ApsisBloomAudioRuntime *bloom_audio,
                       ApsisPersistentAudioQueue *queue,
                       ApsisStringPitchMemory *pitch,
                       ApsisStructuralAudioStats *stats,
                       uint64_t target,
                       ApsisRuntimeMotherApplyFn mother_apply,
                       void *mother_user)
{
    while (r->cursor_sample < target) {
        uint64_t remain = target - r->cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        process_frames(r, s, bank, chain, mother, bloom, spiral, bloom_audio,
                       queue, pitch, stats, frames, mother_apply, mother_user);
    }
}

static void process_to_consuming(ApsisRuntime *r, ApsisState *s,
                                 ApsisKeepBank *bank,
                                 ApsisChainSequence *chain,
                                 ApsisMotherPending *mother,
                                 ApsisBloomVoicingState *bloom,
                                 ApsisSpiralAudioRuntime *spiral,
                                 ApsisBloomAudioRuntime *bloom_audio,
                                 ApsisPersistentAudioQueue *queue,
                                 ApsisStringPitchMemory *pitch,
                                 ApsisStructuralAudioStats *stats,
                                 ApsisStringDsp *string_dsp,
                                 ApsisDriftDsp *drift_dsp,
                                 uint64_t target,
                                 ApsisRuntimeMotherApplyFn mother_apply,
                                 void *mother_user)
{
    while (r->cursor_sample < target) {
        uint64_t remain = target - r->cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        process_frames(r, s, bank, chain, mother, bloom, spiral, bloom_audio,
                       queue, pitch, stats, frames, mother_apply, mother_user);
        (void)apsis_persistent_audio_apply_through(queue, r->cursor_sample,
                                                   string_dsp, drift_dsp);
    }
}

static void setup_one_pulse_chain(ApsisState *s, ApsisKeepBank *bank,
                                  ApsisChainSequence *chain)
{
    ApsisState source;
    apsis_state_default(&source);
    source.gravity.tonic = 3u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(400);
    source.string.right_split_q15 = APSIS_Q15_MILLI(600);
    source.drift.amount_q15 = APSIS_Q15_MILLI(300);
    bank->slots[0] = apsis_keep_capture(&source);

    source.gravity.tonic = 7u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(350);
    source.string.right_split_q15 = APSIS_Q15_MILLI(650);
    source.drift.amount_q15 = APSIS_Q15_MILLI(760);
    bank->slots[1] = apsis_keep_capture(&source);

    s->chain.scope_key_scale = 1u;
    s->chain.scope_tension = 1u;
    s->chain.scope_drift = 1u;
    s->chain.start_sync = APSIS_CHAIN_SYNC_PULSE;
    s->chain.loop_sync = APSIS_CHAIN_SYNC_PULSE;
    chain->count = 2u;
    chain->cells[0] = apsis_chain_default_cell(0u, &s->chain, s->clock.n);
    chain->cells[0].cycles = 0u;
    chain->cells[0].fine_pulses = 1u;
    chain->cells[0].morph_quarters = APSIS_CHAIN_MORPH_NONE;
    chain->cells[1] = apsis_chain_default_cell(1u, &s->chain, s->clock.n);
    chain->cells[1].cycles = 0u;
    chain->cells[1].fine_pulses = 1u;
    chain->cells[1].morph_quarters = APSIS_CHAIN_MORPH_NONE;
}

static void test_three_structural_sources_share_one_target(void)
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
    ApsisClockState fastest;
    uint64_t target, mother_observe, chain_observe, gravity_observe, min_pulse;
    uint32_t mother_calls = 0u;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio, &pitch);
    apsis_persistent_audio_queue_reset(&queue);
    apsis_structural_audio_stats_reset(&stats);
    apsis_string_dsp_reset(&string_dsp);
    apsis_drift_dsp_reset(&drift_dsp);
    setup_one_pulse_chain(&s, &bank, &chain);

    /* Source/legal bound: even the shortest Clock pulse (180 BPM, 1/4,
     * 35% swing short leg) is longer than the largest structural pre-target
     * observation lead (Mother 26 ms). Therefore entries from adjacent pulse
     * targets cannot overlap in this queue. */
    fastest = s.clock;
    fastest.bpm = 180u;
    fastest.division_num = 1u;
    fastest.division_den = 4u;
    fastest.swing_q15 = APSIS_Q15_MILLI(350);
    min_pulse = apsis_transport_round_sample(
        apsis_transport_interval_q32(&fastest, 44100u, 0u));
    assert(min_pulse > ms_samples(26u));

    apsis_runtime_chain_start(&r, &s, &chain);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral, &bloom_audio);

    /* First callback fills the 1.20 s lookahead through absolute pulse 2 and
     * consumes Chain target 0. The scheduler frontier is therefore pulse 3. */
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 256u, 0, 0);
    assert(stats.chain_captures == 1u);
    assert(queue.count == 1u && queue.event[0].sample == 0u);
    assert(apsis_persistent_audio_apply_through(&queue, 0u,
                                                &string_dsp, &drift_dsp) == 1u);

    /* Gravity is requested while Mother hold is OFF. With change_grid=1 only
     * Clock step 1 / cycle boundaries are legal Gravity targets, so the
     * scheduler-frontier search lands on absolute pulse 12. */
    s.gravity.change_grid = 1u;
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u, 2u,
                                         r.cursor_sample, 0u) == 3u);
    target = r.gravity_pending.target_sample;
    assert(target > r.cursor_sample);

    /* Arm Mother after the ordinary Gravity request. Horizon 8 from audible
     * absolute step 0 cannot commit until a Clock-step-1 boundary; the first
     * such boundary satisfying elapsed>=8 is the same absolute pulse 12.
     * Chain is one-cell-per-pulse, so it also applies at that target. */
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_mother_set_horizon(&s.mother, &mother, 8u, 0);
    assert(apsis_mother_queue_param(&mother, 10u, APSIS_Q15_MILLI(777), 0));

    mother_observe = target - ms_samples(26u);
    chain_observe = target - ms_samples(24u);
    gravity_observe = target - ms_samples(18u);
    assert(mother_observe < chain_observe && chain_observe < gravity_observe);

    /* Model the real audio side while approaching the cluster: previously
     * queued Chain targets are consumed at their true target samples. Stop
     * immediately before Mother target-26 ms so the queue is empty and the
     * cluster high-water can be measured independently. */
    process_to_consuming(&r, &s, &bank, &chain, &mother, &bloom,
                         &spiral, &bloom_audio, &queue, &pitch, &stats,
                         &string_dsp, &drift_dsp, mother_observe,
                         mother_apply_test, &mother_calls);
    assert(queue.count == 0u);
    apsis_structural_audio_stats_reset(&stats);
    mother_calls = 0u;
    queue.high_water = 0u;
    queue.overflowed = 0u;

    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 1u,
                   mother_apply_test, &mother_calls);
    assert(queue.count == 1u && queue.event[0].sample == target);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
               &queue, &pitch, &stats, chain_observe,
               mother_apply_test, &mother_calls);
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 1u,
                   mother_apply_test, &mother_calls);
    assert(queue.count == 2u);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
               &queue, &pitch, &stats, gravity_observe,
               mother_apply_test, &mother_calls);
    process_frames(&r, &s, &bank, &chain, &mother, &bloom, &spiral, &bloom_audio,
                   &queue, &pitch, &stats, 1u,
                   mother_apply_test, &mother_calls);

    assert(stats.mother_captures == 1u);
    assert(stats.chain_captures == 1u);
    assert(stats.gravity_captures == 1u);
    assert(stats.capture_drops == 0u);
    assert(mother_calls == 1u);
    assert(queue.overflowed == 0u);
    assert(queue.count == 3u);
    assert(queue.high_water == 3u);
    assert(queue.event[0].sample == target);
    assert(queue.event[1].sample == target);
    assert(queue.event[2].sample == target);

    /* Same-target stable serial order applies all three at the target. */
    assert(apsis_persistent_audio_apply_through(&queue, target - 1u,
                                                &string_dsp, &drift_dsp) == 0u);
    assert(apsis_persistent_audio_apply_through(&queue, target,
                                                &string_dsp, &drift_dsp) == 3u);
    assert(queue.count == 0u);

    printf("M5A_CAPACITY high_water=%u provisional_cap=%u target=%llu min_pulse=%llu lead26=%llu captures=%u/%u/%u drops=%u\n",
           (unsigned)queue.high_water,
           (unsigned)APSIS_PERSISTENT_AUDIO_QUEUE_CAP,
           (unsigned long long)target,
           (unsigned long long)min_pulse,
           (unsigned long long)ms_samples(26u),
           (unsigned)stats.mother_captures,
           (unsigned)stats.chain_captures,
           (unsigned)stats.gravity_captures,
           (unsigned)stats.capture_drops);
}

int main(void)
{
    test_three_structural_sources_share_one_target();
    puts("PASS: APSIS M5a legal persistent-target queue capacity bound");
    return 0;
}
