#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_runtime_m4g.h"
#include "apsis_spiral_dsp.h"

typedef struct {
    uint8_t valid;
    uint8_t trigger_pos;
    uint16_t energy_q15;
    int16_t pitch_step;
    uint32_t phase_inc;
    uint16_t peak_gain_q15;
    uint64_t bind_sample;
    uint64_t target_sample;
} StepOneResult;

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, 0x50355253u);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static uint8_t first_leg(const ApsisSpiralDsp *dsp)
{
    uint8_t i;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i)
        if (dsp->leg[i].active) return i;
    return APSIS_SPIRAL_DSP_LEG_CAP;
}

static StepOneResult run_case(uint8_t resync)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisSpiralAudioRuntime audio;
    ApsisSpiralDsp dsp;
    StepOneResult out;
    uint32_t guard, i;
    uint8_t leg;
    const uint64_t target = 11025u;

    memset(&out, 0, sizeof(out));
    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    if (!resync) assert(s.clock.step_one_resync == 0u);

    s.clock.n = 3u;
    s.clock.bounds[0] = 1u;
    s.clock.bounds[1] = 2u;
    s.clock.bounds[2] = 2u;
    s.clock.main_mask = 0x7u;
    s.clock.bpm = 180u;
    s.clock.division_num = 1u;
    s.clock.division_den = 4u;
    s.clock.swing_q15 = 0u;
    s.clock.step_one_resync = resync ? 1u : 0u;

    s.spiral.route_count = 1u;
    s.spiral.route[0] = 4u;
    s.spiral.attractor_rate = 1u;
    s.spiral.attractor_length = 8u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = 0xffu;
    for (i = 0u; i < 8u; ++i)
        s.spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(500);
    s.spiral.attractor_energy_q15[0] = APSIS_Q15_MILLI(200);
    s.spiral.attractor_energy_q15[3] = APSIS_Q15_MILLI(900);

    apsis_spiral_audio_runtime_reset(&audio);
    apsis_spiral_dsp_reset(&dsp);
    apsis_runtime_start_spiral_audio(&r, &s, 0, &audio);

    for (guard = 0u; guard < 80u && !out.valid; ++guard) {
        uint64_t block_start = r.cursor_sample;
        (void)apsis_runtime_process_spiral_audio(&r, &s, &bank, &chain, &mother,
                                                 &bloom, 256u, 0, 0, 0,
                                                 &audio, 0);
        for (i = 0u; i < audio.binding_count; ++i) {
            const ApsisSpiralAudioBinding *b = &audio.binding[i];
            uint8_t slot;
            if (!b->valid || b->target_sample != target) continue;
            out.valid = 1u;
            out.trigger_pos = b->trigger_pos;
            out.energy_q15 = b->acoustic.energy_q15;
            out.pitch_step = b->pitch.step;
            out.bind_sample = b->bind_sample;
            out.target_sample = b->target_sample;
            assert(apsis_spiral_dsp_trigger_binding(&dsp, &s, b, block_start) > 0u);
            leg = first_leg(&dsp);
            assert(leg < APSIS_SPIRAL_DSP_LEG_CAP);
            out.phase_inc = dsp.leg[leg].osc[0].phase_inc;
            out.peak_gain_q15 = dsp.leg[leg].peak_gain_q15;

            /* The external runtime snapshot and emitted binding must agree on
             * the planning-time attractor identity. */
            for (slot = 0u; slot < APSIS_RUNTIME_SPIRAL_SLOTS; ++slot) {
                if (audio.snapshot[slot].active &&
                    audio.snapshot[slot].token == b->token) {
                    assert(audio.snapshot[slot].trigger_pos == out.trigger_pos);
                    assert(audio.snapshot[slot].acoustic.energy_q15 == out.energy_q15);
                    break;
                }
            }
            break;
        }
    }

    assert(out.valid == 1u);
    assert(out.target_sample == target);
    assert(out.bind_sample == 7497u);
    return out;
}

static void test_step_one_free_run_changes_sounding_energy_not_pitch(void)
{
    StepOneResult free_run = run_case(0u);
    StepOneResult resynced = run_case(1u);

    assert(free_run.trigger_pos == 3u);
    assert(resynced.trigger_pos == 0u);
    assert(free_run.energy_q15 == APSIS_Q15_MILLI(900));
    assert(resynced.energy_q15 == APSIS_Q15_MILLI(200));

    assert(free_run.target_sample == resynced.target_sample);
    assert(free_run.bind_sample == resynced.bind_sample);
    assert(free_run.pitch_step == resynced.pitch_step);
    assert(free_run.phase_inc == resynced.phase_inc);
    assert(free_run.peak_gain_q15 != resynced.peak_gain_q15);
}

int main(void)
{
    test_step_one_free_run_changes_sounding_energy_not_pitch();
    puts("PASS: APSIS P5f step-one RESYNC OFF free-runs into sounding Spiral energy");
    return 0;
}
