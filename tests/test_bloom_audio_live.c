#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_runtime_m4g.h"

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom, uint32_t seed)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, seed);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static void fast_bloom(ApsisState *s)
{
    s->clock.n = 3u;
    s->clock.bounds[0] = 1u;
    s->clock.bounds[1] = 2u;
    s->clock.bounds[2] = 2u;
    s->clock.main_mask = 0x7u;
    s->clock.bpm = 180u;
    s->clock.division_num = 1u;
    s->clock.division_den = 4u;
    s->clock.swing_q15 = 0u;
    s->bloom.cycles = 1u;
    s->bloom.rate = 4u;
    s->bloom.trigger_step = 0u;
    s->bloom.delay_ms = 0u;
    s->bloom.sound = 1u;
}

static void test_native_runtime_is_bit_identical(void)
{
    ApsisState a, b;
    ApsisRuntime ra, rb;
    ApsisKeepBank ka, kb;
    ApsisChainSequence ca, cb;
    ApsisMotherPending ma, mb;
    ApsisBloomVoicingState va, vb;
    ApsisSpiralAudioRuntime sa, sb;
    ApsisBloomAudioRuntime bloom_audio;
    uint32_t block;

    init_all(&a, &ra, &ka, &ca, &ma, &va, 0x424c4f4fu);
    init_all(&b, &rb, &kb, &cb, &mb, &vb, 0x424c4f4fu);
    fast_bloom(&a);
    fast_bloom(&b);
    apsis_spiral_audio_runtime_reset(&sa);
    apsis_spiral_audio_runtime_reset(&sb);
    apsis_bloom_audio_runtime_reset(&bloom_audio);
    apsis_runtime_start_spiral_audio(&ra, &a, 0, &sa);
    apsis_runtime_start_bloom_audio(&rb, &b, 0, &sb, &bloom_audio);

    for (block = 0u; block < 180u; ++block) {
        (void)apsis_runtime_process_spiral_audio(&ra, &a, &ka, &ca, &ma, &va,
                                                 256u, 0, 0, 0, &sa, 0);
        (void)apsis_runtime_process_bloom_audio(&rb, &b, &kb, &cb, &mb, &vb,
                                                256u, 0, 0, 0, &sb,
                                                &bloom_audio, 0);
    }

    assert(sizeof(ApsisRuntime) == 21928u);
    assert(memcmp(&ra, &rb, sizeof(ra)) == 0);
    assert(memcmp(&a, &b, sizeof(a)) == 0);
    assert(memcmp(&ka, &kb, sizeof(ka)) == 0);
    assert(memcmp(&ca, &cb, sizeof(ca)) == 0);
    assert(memcmp(&ma, &mb, sizeof(ma)) == 0);
    assert(memcmp(&va, &vb, sizeof(va)) == 0);
    assert(bloom_audio.capture_miss_count == 0u);
    assert(bloom_audio.bind_miss_count == 0u);
    assert(bloom_audio.binding_overflow_count == 0u);
    assert(bloom_audio.capture_count > 0u);
    assert(bloom_audio.bind_count > 0u);
}

static void test_occurrence_freeze_and_bind_late_state(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState voicing;
    ApsisSpiralAudioRuntime spiral_audio;
    ApsisBloomAudioRuntime audio;
    ApsisBloomAudioBinding first, second;
    uint8_t got_first = 0u, got_second = 0u;
    uint32_t block, i;

    init_all(&s, &r, &bank, &chain, &mother, &voicing, 0x13572468u);
    fast_bloom(&s);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral_audio, &audio);

    for (block = 0u; block < 80u && !got_first; ++block) {
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &audio, 0);
        for (i = 0u; i < audio.binding_count; ++i) {
            if (audio.binding[i].valid && audio.binding[i].petal_index == 0u) {
                first = audio.binding[i];
                got_first = 1u;
                break;
            }
        }
    }

    assert(got_first == 1u);
    assert(first.bind_sample == 9040u);   /* target 11,025 - occurrence lead 1,985 */
    assert(first.target_sample == 11025u);
    assert(first.voice == APSIS_BLOOM_VEIL);
    assert(first.attack_ms == 18u);
    assert(first.brightness_q15 == APSIS_Q15_MILLI(720));
    assert(first.acoustic.duration_samples == 83790u);
    assert(first.acoustic.amp_q15 == 190u);
    assert(first.acoustic.pan_q15 == -17694);

    /* Occurrence-frozen values change now, but token 1 must keep the old plan. */
    s.bloom.length_ms = 6000u;
    s.bloom.unfurl_ms = 3600u;
    s.bloom.width_q15 = 0u;
    s.bloom.level_q15 = 0u;
    s.bloom.depth = APSIS_BLOOM_DEPTH_TRIAD;

    /* Bind-late acoustic values must be observed by the next petal. */
    s.bloom.voice = APSIS_BLOOM_GLASS;
    s.bloom.attack_ms = 250u;
    s.bloom.brightness_q15 = APSIS_Q15_MILLI(300);

    for (; block < 100u && !got_second; ++block) {
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &audio, 0);
        for (i = 0u; i < audio.binding_count; ++i) {
            if (audio.binding[i].valid &&
                audio.binding[i].token == first.token &&
                audio.binding[i].petal_index == 1u) {
                second = audio.binding[i];
                got_second = 1u;
                break;
            }
        }
    }

    assert(got_second == 1u);
    assert(second.bind_sample == 14994u);
    assert(second.target_sample == 17199u);
    assert(second.voice == APSIS_BLOOM_GLASS);
    assert(second.attack_ms == 250u);
    assert(second.brightness_q15 == APSIS_Q15_MILLI(300));
    assert(second.acoustic.target_offset_samples == 6174u);
    assert(second.acoustic.duration_samples == 86436u);
    assert(second.acoustic.amp_q15 == 178u);
    assert(second.acoustic.pan_q15 == -5898);
    assert(second.pitch.valid == 1u);
    assert(audio.capture_miss_count == 0u);
    assert(audio.bind_miss_count == 0u);
    assert(audio.binding_overflow_count == 0u);
}

static void test_stop_clears_external_bloom_audio_state(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState voicing;
    ApsisSpiralAudioRuntime spiral_audio;
    ApsisBloomAudioRuntime audio;
    uint32_t block;

    init_all(&s, &r, &bank, &chain, &mother, &voicing, 9u);
    fast_bloom(&s);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral_audio, &audio);
    for (block = 0u; block < 50u && audio.capture_count == 0u; ++block)
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &audio, 0);
    assert(audio.capture_count > 0u);

    apsis_runtime_stop_bloom_audio(&r, 0, &spiral_audio, &audio);
    assert(audio.capture_count == 0u);
    assert(audio.bind_count == 0u);
    assert(audio.binding_count == 0u);
}

int main(void)
{
    test_native_runtime_is_bit_identical();
    test_occurrence_freeze_and_bind_late_state();
    test_stop_clears_external_bloom_audio_state();
    puts("PASS: APSIS M4g shared-runtime Bloom acoustic handoff");
    return 0;
}
