#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_runtime_m4f.h"

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

static void enable_simple_spiral(ApsisState *s)
{
    uint8_t i;
    s->spiral.route_count = 1u;
    s->spiral.route[0] = 0u;
    s->spiral.attractor_rate = 1u;
    s->spiral.attractor_length = 8u;
    s->spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s->spiral.attractor_gates = 0xFFu;
    for (i = 0u; i < 8u; ++i)
        s->spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(820);
}

static void test_native_runtime_is_bit_identical(void)
{
    ApsisState a, b;
    ApsisRuntime ra, rb;
    ApsisKeepBank ka, kb;
    ApsisChainSequence ca, cb;
    ApsisMotherPending ma, mb;
    ApsisBloomVoicingState ba, bb;
    ApsisSpiralAudioRuntime audio;
    uint32_t blocks;

    init_all(&a, &ra, &ka, &ca, &ma, &ba, 0x4f535049u);
    init_all(&b, &rb, &kb, &cb, &mb, &bb, 0x4f535049u);
    enable_simple_spiral(&a);
    enable_simple_spiral(&b);
    apsis_spiral_audio_runtime_reset(&audio);
    apsis_runtime_start(&ra);
    apsis_runtime_start(&rb);

    for (blocks = 0u; blocks < 900u; ++blocks) {
        apsis_runtime_process_attractor(&ra, &a, &ka, &ca, &ma, &ba,
                                        256u, 0, 0, 0);
        (void)apsis_runtime_process_spiral_audio(&rb, &b, &kb, &cb, &mb, &bb,
                                                256u, 0, 0, 0, &audio, 0);
    }

    assert(memcmp(&ra, &rb, sizeof(ra)) == 0);
    assert(memcmp(&a, &b, sizeof(a)) == 0);
    assert(memcmp(&ka, &kb, sizeof(ka)) == 0);
    assert(memcmp(&ca, &cb, sizeof(ca)) == 0);
    assert(memcmp(&ma, &mb, sizeof(ma)) == 0);
    assert(audio.capture_miss_count == 0u);
    assert(audio.bind_miss_count == 0u);
    assert(audio.binding_overflow_count == 0u);
    assert(audio.capture_count > 0u);
    assert(audio.bind_count > 0u);
}

static void test_planning_snapshot_survives_late_edits(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime audio;
    ApsisSpiralAudioSnapshot frozen;
    ApsisSpiralAudioBinding bound;
    uint8_t found_snapshot = 0u, found_bind = 0u;
    uint32_t i, guard;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, 0x13579bdfu);
    enable_simple_spiral(&s);
    apsis_spiral_audio_runtime_reset(&audio);
    apsis_runtime_start_spiral_audio(&r, &s, 0, &audio);

    (void)apsis_runtime_process_spiral_audio(&r, &s, &bank, &chain, &mother,
                                             &bloom, 256u, 0, 0, 0, &audio, 0);

    /* Lookahead planned target 18,375 while the original THREAD/.78/Drift-.64
     * state was still current. Token 1 targeted sample 0 and bound immediately;
     * the next occurrence remains pending for its target-80 ms bind. */
    for (i = 0u; i < APSIS_RUNTIME_SPIRAL_SLOTS; ++i) {
        if (audio.snapshot[i].active &&
            audio.snapshot[i].target_sample == 18375u) {
            frozen = audio.snapshot[i];
            found_snapshot = 1u;
            break;
        }
    }
    assert(found_snapshot == 1u);
    assert(frozen.token == 2u);
    assert(frozen.acoustic.voice == APSIS_SPIRAL_THREAD);
    assert(frozen.acoustic.width_q15 == APSIS_Q15_MILLI(640));
    assert(frozen.acoustic.leg_count == 2u);

    /* These edits happen after planning but before token 2 bind. Source semantics
     * keep prepared topology/level/Drift data frozen while register/pitch and
     * the unusual late filter-cutoff voice family remain live. */
    s.spiral.voice = APSIS_SPIRAL_HALO;
    s.spiral.note_length_q15 = APSIS_Q15_MILLI(180);
    s.spiral.level_q15 = APSIS_Q15_MILLI(200);
    s.drift.width_q15 = APSIS_Q15_MILLI(100);
    s.spiral.register_oct = 1;

    for (guard = 0u; guard < 100u && !found_bind; ++guard) {
        (void)apsis_runtime_process_spiral_audio(&r, &s, &bank, &chain, &mother,
                                                 &bloom, 256u, 0, 0, 0,
                                                 &audio, 0);
        for (i = 0u; i < audio.binding_count; ++i) {
            if (audio.binding[i].valid && audio.binding[i].token == 2u) {
                bound = audio.binding[i];
                found_bind = 1u;
                break;
            }
        }
    }

    assert(found_bind == 1u);
    assert(bound.bind_sample == 14847u); /* 18,375 - 80 ms (3,528 samples) */
    assert(bound.target_sample == 18375u);
    assert(bound.acoustic.voice == APSIS_SPIRAL_THREAD);
    assert(bound.acoustic.width_q15 == APSIS_Q15_MILLI(640));
    assert(bound.acoustic.leg[0].gain_q15 == frozen.acoustic.leg[0].gain_q15);
    assert(bound.acoustic.duration_samples == frozen.acoustic.duration_samples);
    assert(bound.cutoff_voice == APSIS_SPIRAL_HALO);
    assert(bound.pitch.valid == 1u);
    assert(bound.pitch.step == 12); /* current +1 octave register at bind */
    assert(audio.capture_miss_count == 0u);
    assert(audio.bind_miss_count == 0u);
    assert(audio.binding_overflow_count == 0u);
}

static void test_lifecycle_clears_external_pending(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime audio;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, 7u);
    enable_simple_spiral(&s);
    apsis_runtime_start_spiral_audio(&r, &s, 0, &audio);
    (void)apsis_runtime_process_spiral_audio(&r, &s, &bank, &chain, &mother,
                                             &bloom, 256u, 0, 0, 0, &audio, 0);
    assert(audio.capture_count > 0u);
    apsis_runtime_stop_spiral_audio(&r, 0, &audio);
    assert(audio.capture_count == 0u);
    assert(audio.bind_count == 0u);
    assert(audio.binding_count == 0u);
}

int main(void)
{
    test_native_runtime_is_bit_identical();
    test_planning_snapshot_survives_late_edits();
    test_lifecycle_clears_external_pending();
    puts("PASS: APSIS M4f shared-runtime Spiral acoustic snapshot/bind bridge");
    return 0;
}
