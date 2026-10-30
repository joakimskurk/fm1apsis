#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_runtime_m4g.h"
#include "apsis_spiral_dsp.h"
#include "apsis_bloom_dsp.h"

typedef struct {
    ApsisSpiralAudioSnapshot frozen;
    ApsisSpiralAudioBinding binding;
    uint32_t phase_inc;
    uint32_t start_delay;
} SpiralResult;

typedef struct {
    ApsisBloomPetalAcoustic frozen_petal;
    ApsisBloomAudioBinding binding;
    uint32_t phase_inc;
    uint32_t start_delay;
} BloomResult;

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
    s->spiral.follow_frame = 1u;
    s->spiral.attractor_rate = 1u;
    s->spiral.attractor_length = 8u;
    s->spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s->spiral.attractor_gates = 0xffu;
    for (i = 0u; i < 8u; ++i)
        s->spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(820);
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

static uint8_t first_active_spiral_leg(const ApsisSpiralDsp *dsp)
{
    uint8_t i;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i)
        if (dsp->leg[i].active) return i;
    return APSIS_SPIRAL_DSP_LEG_CAP;
}

static uint16_t first_active_bloom_petal(const ApsisBloomDsp *dsp)
{
    uint16_t i;
    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i)
        if (dsp->petal[i].active) return i;
    return APSIS_BLOOM_DSP_PETAL_CAP;
}

static SpiralResult run_spiral_case(uint8_t change_gravity)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime audio;
    ApsisSpiralDsp dsp;
    SpiralResult out;
    uint8_t found_snapshot = 0u, found_bind = 0u, leg;
    uint32_t i, guard;

    memset(&out, 0, sizeof(out));
    init_all(&s, &r, &bank, &chain, &mother, &bloom, 0x50354153u);
    enable_simple_spiral(&s);
    apsis_spiral_audio_runtime_reset(&audio);
    apsis_spiral_dsp_reset(&dsp);
    apsis_runtime_start_spiral_audio(&r, &s, 0, &audio);

    (void)apsis_runtime_process_spiral_audio(&r, &s, &bank, &chain, &mother,
                                             &bloom, 256u, 0, 0, 0, &audio, 0);
    for (i = 0u; i < APSIS_RUNTIME_SPIRAL_SLOTS; ++i) {
        if (audio.snapshot[i].active && audio.snapshot[i].target_sample == 18375u) {
            out.frozen = audio.snapshot[i];
            found_snapshot = 1u;
            break;
        }
    }
    assert(found_snapshot == 1u);
    assert(out.frozen.token == 2u);

    if (change_gravity) {
        s.gravity.change_timing = APSIS_CHANGE_IMMEDIATE;
        assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u,
                                             s.gravity.voicing,
                                             r.cursor_sample, 0u) == 1u);
        assert(s.gravity.degree == 4u);
    }

    for (guard = 0u; guard < 100u && !found_bind; ++guard) {
        (void)apsis_runtime_process_spiral_audio(&r, &s, &bank, &chain, &mother,
                                                 &bloom, 256u, 0, 0, 0,
                                                 &audio, 0);
        for (i = 0u; i < audio.binding_count; ++i) {
            if (audio.binding[i].valid && audio.binding[i].token == 2u) {
                out.binding = audio.binding[i];
                found_bind = 1u;
                break;
            }
        }
    }
    assert(found_bind == 1u);
    assert(out.binding.target_sample == out.frozen.target_sample);
    assert(out.binding.acoustic.duration_samples == out.frozen.acoustic.duration_samples);
    assert(out.binding.acoustic.leg_count == out.frozen.acoustic.leg_count);
    assert(out.binding.acoustic.leg[0].gain_q15 == out.frozen.acoustic.leg[0].gain_q15);
    assert(out.binding.acoustic.width_q15 == out.frozen.acoustic.width_q15);

    assert(apsis_spiral_dsp_trigger_binding(&dsp, &s, &out.binding,
                                            out.binding.bind_sample) > 0u);
    leg = first_active_spiral_leg(&dsp);
    assert(leg < APSIS_SPIRAL_DSP_LEG_CAP);
    assert(dsp.leg[leg].osc_count > 0u);
    out.phase_inc = dsp.leg[leg].osc[0].phase_inc;
    out.start_delay = dsp.leg[leg].start_delay_samples;
    return out;
}

static BloomResult run_bloom_case(uint8_t change_gravity)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState voicing;
    ApsisSpiralAudioRuntime spiral_audio;
    ApsisBloomAudioRuntime audio;
    ApsisBloomDsp dsp;
    BloomResult out;
    ApsisBloomAudioBinding first;
    uint8_t got_first = 0u, got_second = 0u;
    uint32_t block = 0u, i;
    uint16_t petal;

    memset(&out, 0, sizeof(out));
    init_all(&s, &r, &bank, &chain, &mother, &voicing, 0x5035424cu);
    fast_bloom(&s);
    apsis_bloom_dsp_reset(&dsp);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral_audio, &audio);

    for (; block < 80u && !got_first; ++block) {
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
    assert(first.target_sample == 11025u);

    /* Petal 0 binding proves the occurrence has happened and the token's full
     * acoustic plan is frozen. Petal 1 remains late-bound. */
    for (i = 0u; i < APSIS_RUNTIME_BLOOM_SLOTS; ++i) {
        if (audio.snapshot[i].active && audio.snapshot[i].token == first.token) {
            assert(audio.snapshot[i].acoustic.count > 1u);
            out.frozen_petal = audio.snapshot[i].acoustic.petal[1];
            break;
        }
    }
    assert(i < APSIS_RUNTIME_BLOOM_SLOTS);

    if (change_gravity) {
        s.gravity.change_timing = APSIS_CHANGE_IMMEDIATE;
        assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u,
                                             s.gravity.voicing,
                                             r.cursor_sample, 0u) == 1u);
        assert(s.gravity.degree == 4u);
    }

    for (; block < 110u && !got_second; ++block) {
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &audio, 0);
        for (i = 0u; i < audio.binding_count; ++i) {
            if (audio.binding[i].valid &&
                audio.binding[i].token == first.token &&
                audio.binding[i].petal_index == 1u) {
                out.binding = audio.binding[i];
                got_second = 1u;
                break;
            }
        }
    }
    assert(got_second == 1u);
    assert(out.binding.acoustic.target_offset_samples == out.frozen_petal.target_offset_samples);
    assert(out.binding.acoustic.duration_samples == out.frozen_petal.duration_samples);
    assert(out.binding.acoustic.amp_q15 == out.frozen_petal.amp_q15);
    assert(out.binding.acoustic.pan_q15 == out.frozen_petal.pan_q15);

    assert(apsis_bloom_dsp_trigger_binding(&dsp, &s, &out.binding,
                                           out.binding.bind_sample) == 1u);
    petal = first_active_bloom_petal(&dsp);
    assert(petal < APSIS_BLOOM_DSP_PETAL_CAP);
    assert(dsp.petal[petal].osc_count > 0u);
    out.phase_inc = dsp.petal[petal].osc[0].phase_inc;
    out.start_delay = dsp.petal[petal].start_delay_samples;
    return out;
}

static void test_spiral_immediate_gravity_reaches_sounding_voice(void)
{
    SpiralResult base = run_spiral_case(0u);
    SpiralResult changed = run_spiral_case(1u);

    assert(base.frozen.target_sample == changed.frozen.target_sample);
    assert(base.binding.target_sample == changed.binding.target_sample);
    assert(base.binding.bind_sample == changed.binding.bind_sample);
    assert(base.binding.pitch.step != changed.binding.pitch.step);
    assert(base.phase_inc != changed.phase_inc);
    assert(base.start_delay == changed.start_delay);
    assert(base.start_delay == 3528u); /* target - target−80 ms bind */
}

static void test_bloom_immediate_gravity_reaches_later_sounding_petal(void)
{
    BloomResult base = run_bloom_case(0u);
    BloomResult changed = run_bloom_case(1u);

    assert(base.binding.token == changed.binding.token);
    assert(base.binding.petal_index == 1u && changed.binding.petal_index == 1u);
    assert(base.binding.target_sample == changed.binding.target_sample);
    assert(base.binding.bind_sample == changed.binding.bind_sample);
    assert(base.binding.pitch.step != changed.binding.pitch.step);
    assert(base.phase_inc != changed.phase_inc);
    assert(base.start_delay == changed.start_delay);
    assert(base.start_delay ==
           (uint32_t)(base.binding.target_sample - base.binding.bind_sample));
}

int main(void)
{
    test_spiral_immediate_gravity_reaches_sounding_voice();
    test_bloom_immediate_gravity_reaches_later_sounding_petal();
    puts("PASS: APSIS P5a structural late binding reaches sounding Spiral/Bloom DSP");
    return 0;
}
