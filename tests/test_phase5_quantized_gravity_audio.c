#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_runtime_m4g.h"
#include "apsis_spiral_dsp.h"
#include "apsis_bloom_dsp.h"

typedef struct {
    uint8_t valid;
    int16_t pitch_step;
    uint32_t phase_inc;
    uint32_t start_delay;
    uint64_t bind_sample;
    uint64_t target_sample;
    uint8_t state_degree;
    uint8_t pending_active;
} SoundingPoint;

typedef struct {
    SoundingPoint spiral_boundary;
    SoundingPoint bloom_boundary;
    SoundingPoint spiral_after_commit;
    uint64_t pending_target;
    uint64_t commit_sample;
} QuantResult;

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

static void configure_aligned_case(ApsisState *s)
{
    uint8_t i;
    s->clock.n = 3u;
    s->clock.bounds[0] = 1u;
    s->clock.bounds[1] = 2u;
    s->clock.bounds[2] = 2u;
    s->clock.main_mask = 0x7u;
    s->clock.bpm = 180u;
    s->clock.division_num = 1u;
    s->clock.division_den = 4u;
    s->clock.swing_q15 = 0u;

    s->gravity.degree = 0u;
    s->gravity.change_timing = APSIS_CHANGE_PULSE;
    s->gravity.change_grid = 1u;

    s->spiral.route_count = 1u;
    s->spiral.route[0] = 0u;
    s->spiral.follow_frame = 1u;
    s->spiral.attractor_rate = 1u;
    s->spiral.attractor_length = 8u;
    s->spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s->spiral.attractor_gates = 0xffu;
    for (i = 0u; i < 8u; ++i)
        s->spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(820);

    s->bloom.cycles = 1u;
    s->bloom.rate = 1u;
    s->bloom.trigger_step = 0u;
    s->bloom.delay_ms = 0u;
    s->bloom.sound = 1u;
}

static uint8_t first_spiral_leg(const ApsisSpiralDsp *dsp)
{
    uint8_t i;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i)
        if (dsp->leg[i].active) return i;
    return APSIS_SPIRAL_DSP_LEG_CAP;
}

static uint16_t first_bloom_petal(const ApsisBloomDsp *dsp)
{
    uint16_t i;
    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i)
        if (dsp->petal[i].active) return i;
    return APSIS_BLOOM_DSP_PETAL_CAP;
}

static SoundingPoint spiral_point(const ApsisState *s,
                                   const ApsisRuntime *r,
                                   const ApsisSpiralAudioBinding *binding,
                                   uint64_t block_start)
{
    ApsisSpiralDsp dsp;
    SoundingPoint p;
    uint8_t leg;
    memset(&p, 0, sizeof(p));
    apsis_spiral_dsp_reset(&dsp);
    assert(apsis_spiral_dsp_trigger_binding(&dsp, s, binding, block_start) > 0u);
    leg = first_spiral_leg(&dsp);
    assert(leg < APSIS_SPIRAL_DSP_LEG_CAP);
    assert(dsp.leg[leg].osc_count > 0u);
    p.valid = 1u;
    p.pitch_step = binding->pitch.step;
    p.phase_inc = dsp.leg[leg].osc[0].phase_inc;
    p.start_delay = dsp.leg[leg].start_delay_samples;
    p.bind_sample = binding->bind_sample;
    p.target_sample = binding->target_sample;
    p.state_degree = s->gravity.degree;
    p.pending_active = r->gravity_pending.active;
    return p;
}

static SoundingPoint bloom_point(const ApsisState *s,
                                  const ApsisRuntime *r,
                                  const ApsisBloomAudioBinding *binding,
                                  uint64_t block_start)
{
    ApsisBloomDsp dsp;
    SoundingPoint p;
    uint16_t petal;
    memset(&p, 0, sizeof(p));
    apsis_bloom_dsp_reset(&dsp);
    assert(apsis_bloom_dsp_trigger_binding(&dsp, s, binding, block_start) == 1u);
    petal = first_bloom_petal(&dsp);
    assert(petal < APSIS_BLOOM_DSP_PETAL_CAP);
    assert(dsp.petal[petal].osc_count > 0u);
    p.valid = 1u;
    p.pitch_step = binding->pitch.step;
    p.phase_inc = dsp.petal[petal].osc[0].phase_inc;
    p.start_delay = dsp.petal[petal].start_delay_samples;
    p.bind_sample = binding->bind_sample;
    p.target_sample = binding->target_sample;
    p.state_degree = s->gravity.degree;
    p.pending_active = r->gravity_pending.active;
    return p;
}

static QuantResult run_case(uint8_t request_pending)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState voicing;
    ApsisSpiralAudioRuntime spiral_audio;
    ApsisBloomAudioRuntime bloom_audio;
    QuantResult out;
    const uint64_t aligned_target = 55125u;
    const uint64_t next_spiral_target = 58800u;
    uint32_t guard, i;

    memset(&out, 0, sizeof(out));
    init_all(&s, &r, &bank, &chain, &mother, &voicing, 0x50354247u);
    configure_aligned_case(&s);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral_audio, &bloom_audio);

    /* Initial 256-frame callback fills the 1.20 s lookahead through target
     * 51,450; the next Clock-1 / Gravity boundary is 55,125. */
    (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                            &voicing, 256u, 0, 0, 0,
                                            &spiral_audio, &bloom_audio, 0);
    assert(r.transport.next_sample == aligned_target);
    assert(r.cursor_sample == 256u);

    if (request_pending) {
        assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u,
                                             s.gravity.voicing,
                                             r.cursor_sample, 0u) == 3u);
        assert(r.gravity_pending.active == 1u);
        assert(r.gravity_pending.target_sample == aligned_target);
        assert(s.gravity.degree == 0u);
        out.pending_target = r.gravity_pending.target_sample;
        out.commit_sample = aligned_target - 794u; /* nearest 18 ms at 44.1 kHz */
    } else {
        out.pending_target = aligned_target;
        out.commit_sample = aligned_target - 794u;
    }

    for (guard = 0u; guard < 260u; ++guard) {
        uint64_t block_start = r.cursor_sample;
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &bloom_audio, 0);

        for (i = 0u; i < spiral_audio.binding_count; ++i) {
            const ApsisSpiralAudioBinding *b = &spiral_audio.binding[i];
            if (!b->valid) continue;
            if (!out.spiral_boundary.valid && b->target_sample == aligned_target)
                out.spiral_boundary = spiral_point(&s, &r, b, block_start);
            if (!out.spiral_after_commit.valid && b->target_sample == next_spiral_target)
                out.spiral_after_commit = spiral_point(&s, &r, b, block_start);
        }

        for (i = 0u; i < bloom_audio.binding_count; ++i) {
            const ApsisBloomAudioBinding *b = &bloom_audio.binding[i];
            if (!b->valid) continue;
            if (!out.bloom_boundary.valid && b->petal_index == 0u &&
                b->target_sample == aligned_target)
                out.bloom_boundary = bloom_point(&s, &r, b, block_start);
        }

        if (out.spiral_boundary.valid && out.bloom_boundary.valid &&
            out.spiral_after_commit.valid)
            break;
    }

    assert(out.spiral_boundary.valid == 1u);
    assert(out.bloom_boundary.valid == 1u);
    assert(out.spiral_after_commit.valid == 1u);
    assert(out.spiral_boundary.bind_sample == 51597u);
    assert(out.bloom_boundary.bind_sample == 53140u);
    assert(out.spiral_after_commit.bind_sample == 55272u);
    return out;
}

static void test_quantized_pending_asymmetry_reaches_actual_dsp(void)
{
    QuantResult control = run_case(0u);
    QuantResult pending = run_case(1u);

    assert(pending.pending_target == 55125u);
    assert(pending.commit_sample == 54331u);

    /* Both pre-commit observations happen while the committed shared state is
     * still degree 0 and the ordinary candidate is visibly pending. */
    assert(pending.spiral_boundary.state_degree == 0u);
    assert(pending.bloom_boundary.state_degree == 0u);
    assert(pending.spiral_boundary.pending_active == 1u);
    assert(pending.bloom_boundary.pending_active == 1u);

    /* Spiral deliberately ignores ordinary future pending Gravity. */
    assert(pending.spiral_boundary.target_sample == control.spiral_boundary.target_sample);
    assert(pending.spiral_boundary.bind_sample == control.spiral_boundary.bind_sample);
    assert(pending.spiral_boundary.pitch_step == control.spiral_boundary.pitch_step);
    assert(pending.spiral_boundary.phase_inc == control.spiral_boundary.phase_inc);
    assert(pending.spiral_boundary.start_delay == control.spiral_boundary.start_delay);

    /* Bloom deliberately projects the same ordinary pending candidate against
     * the petal target before shared state commits. */
    assert(pending.bloom_boundary.target_sample == control.bloom_boundary.target_sample);
    assert(pending.bloom_boundary.bind_sample == control.bloom_boundary.bind_sample);
    assert(pending.bloom_boundary.pitch_step != control.bloom_boundary.pitch_step);
    assert(pending.bloom_boundary.phase_inc != control.bloom_boundary.phase_inc);
    assert(pending.bloom_boundary.start_delay == control.bloom_boundary.start_delay);

    /* The target−18 ms commit has happened before the next Spiral bind, so the
     * next actual finite Spiral voice must now follow the new shared Gravity. */
    assert(pending.spiral_after_commit.state_degree == 4u);
    assert(pending.spiral_after_commit.pending_active == 0u);
    assert(pending.spiral_after_commit.pitch_step != control.spiral_after_commit.pitch_step);
    assert(pending.spiral_after_commit.phase_inc != control.spiral_after_commit.phase_inc);
    assert(pending.spiral_after_commit.target_sample == control.spiral_after_commit.target_sample);
}

int main(void)
{
    test_quantized_pending_asymmetry_reaches_actual_dsp();
    puts("PASS: APSIS P5b quantized Gravity asymmetry reaches sounding Spiral/Bloom DSP");
    return 0;
}
