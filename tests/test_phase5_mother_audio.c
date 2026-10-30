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
    uint64_t bind_sample;
    uint64_t target_sample;
    uint8_t state_degree;
    uint8_t runtime_pending;
    uint8_t mother_gravity_pending;
    uint8_t mother_sync;
} MotherPoint;

typedef struct {
    MotherPoint spiral_boundary;
    MotherPoint bloom_boundary;
    MotherPoint spiral_after;
    MotherPoint bloom_after;
} MotherResult;

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

static MotherPoint spiral_point(const ApsisState *s, const ApsisRuntime *r,
                                const ApsisMotherPending *mother,
                                const ApsisSpiralAudioBinding *binding,
                                uint64_t block_start)
{
    ApsisSpiralDsp dsp;
    MotherPoint p;
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
    p.bind_sample = binding->bind_sample;
    p.target_sample = binding->target_sample;
    p.state_degree = s->gravity.degree;
    p.runtime_pending = r->gravity_pending.active;
    p.mother_gravity_pending = mother->gravity_pending;
    p.mother_sync = s->mother.sync;
    return p;
}

static MotherPoint bloom_point(const ApsisState *s, const ApsisRuntime *r,
                               const ApsisMotherPending *mother,
                               const ApsisBloomAudioBinding *binding,
                               uint64_t block_start)
{
    ApsisBloomDsp dsp;
    MotherPoint p;
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
    p.bind_sample = binding->bind_sample;
    p.target_sample = binding->target_sample;
    p.state_degree = s->gravity.degree;
    p.runtime_pending = r->gravity_pending.active;
    p.mother_gravity_pending = mother->gravity_pending;
    p.mother_sync = s->mother.sync;
    return p;
}

static MotherResult run_case(uint8_t hold_gravity)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState voicing;
    ApsisSpiralAudioRuntime spiral_audio;
    ApsisBloomAudioRuntime bloom_audio;
    MotherResult out;
    const uint64_t boundary = 55125u;
    const uint64_t spiral_after = 58800u;
    const uint64_t bloom_after = 66150u;
    uint32_t guard, i;

    memset(&out, 0, sizeof(out));
    init_all(&s, &r, &bank, &chain, &mother, &voicing, 0x50354d4fu);
    configure_aligned_case(&s);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral_audio, &bloom_audio);
    (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                            &voicing, 256u, 0, 0, 0,
                                            &spiral_audio, &bloom_audio, 0);
    assert(r.transport.next_sample == boundary);

    if (hold_gravity) {
        apsis_mother_set_sync(&s.mother, &mother, 1u);
        apsis_mother_set_horizon(&s.mother, &mother, 1u,
                                 (int32_t)r.transport.absolute_pulse - 1);
        assert(apsis_mother_should_hold(&s.mother, &mother,
                                        APSIS_MOTHER_DOMAIN_STRUCTURAL) == 1u);
        assert(apsis_mother_should_hold(&s.mother, &mother,
                                        APSIS_MOTHER_DOMAIN_MOTHER) == 0u);
        assert(apsis_mother_should_hold(&s.mother, &mother,
                                        APSIS_MOTHER_DOMAIN_OUTPUT) == 0u);
        assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u,
                                             s.gravity.voicing,
                                             r.cursor_sample, 0u) == 2u);
        assert(r.gravity_pending.active == 0u);
        assert(mother.gravity_pending == 1u);
        assert(mother.pending_start_abs == 14);
        assert(s.gravity.degree == 0u);
    }

    for (guard = 0u; guard < 320u; ++guard) {
        uint64_t block_start = r.cursor_sample;
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &bloom_audio, 0);
        for (i = 0u; i < spiral_audio.binding_count; ++i) {
            const ApsisSpiralAudioBinding *b = &spiral_audio.binding[i];
            if (!b->valid) continue;
            if (!out.spiral_boundary.valid && b->target_sample == boundary)
                out.spiral_boundary = spiral_point(&s, &r, &mother, b, block_start);
            if (!out.spiral_after.valid && b->target_sample == spiral_after)
                out.spiral_after = spiral_point(&s, &r, &mother, b, block_start);
        }
        for (i = 0u; i < bloom_audio.binding_count; ++i) {
            const ApsisBloomAudioBinding *b = &bloom_audio.binding[i];
            if (!b->valid || b->petal_index != 0u) continue;
            if (!out.bloom_boundary.valid && b->target_sample == boundary)
                out.bloom_boundary = bloom_point(&s, &r, &mother, b, block_start);
            if (!out.bloom_after.valid && b->target_sample == bloom_after)
                out.bloom_after = bloom_point(&s, &r, &mother, b, block_start);
        }
        if (out.spiral_boundary.valid && out.bloom_boundary.valid &&
            out.spiral_after.valid && out.bloom_after.valid)
            break;
    }

    assert(out.spiral_boundary.valid == 1u);
    assert(out.bloom_boundary.valid == 1u);
    assert(out.spiral_after.valid == 1u);
    assert(out.bloom_after.valid == 1u);
    assert(out.spiral_boundary.bind_sample == 51597u);
    assert(out.bloom_boundary.bind_sample == 53140u);
    assert(out.spiral_after.bind_sample == 55272u);
    return out;
}

static void test_mother_hold_reaches_sounding_sources_only_after_commit(void)
{
    MotherResult control = run_case(0u);
    MotherResult held = run_case(1u);

    /* Mother-held Gravity is neither committed nor exposed as ordinary pending
     * state at the same-boundary Spiral/Bloom observation points. */
    assert(held.spiral_boundary.state_degree == 0u);
    assert(held.bloom_boundary.state_degree == 0u);
    assert(held.spiral_boundary.runtime_pending == 0u);
    assert(held.bloom_boundary.runtime_pending == 0u);
    assert(held.spiral_boundary.mother_gravity_pending == 1u);
    assert(held.bloom_boundary.mother_gravity_pending == 1u);
    assert(held.spiral_boundary.mother_sync == 1u);
    assert(held.bloom_boundary.mother_sync == 1u);

    assert(held.spiral_boundary.pitch_step == control.spiral_boundary.pitch_step);
    assert(held.spiral_boundary.phase_inc == control.spiral_boundary.phase_inc);
    assert(held.bloom_boundary.pitch_step == control.bloom_boundary.pitch_step);
    assert(held.bloom_boundary.phase_inc == control.bloom_boundary.phase_inc);

    /* Mother commits at Clock-1 target−26 ms = 53,978. Both later sounding
     * sources must then use the new degree, and the Mother hold clears. */
    assert(held.spiral_after.state_degree == 4u);
    assert(held.spiral_after.runtime_pending == 0u);
    assert(held.spiral_after.mother_gravity_pending == 0u);
    assert(held.spiral_after.mother_sync == 0u);
    assert(held.spiral_after.pitch_step != control.spiral_after.pitch_step);
    assert(held.spiral_after.phase_inc != control.spiral_after.phase_inc);

    assert(held.bloom_after.state_degree == 4u);
    assert(held.bloom_after.mother_gravity_pending == 0u);
    assert(held.bloom_after.mother_sync == 0u);
    assert(held.bloom_after.pitch_step != control.bloom_after.pitch_step);
    assert(held.bloom_after.phase_inc != control.bloom_after.phase_inc);
}

int main(void)
{
    test_mother_hold_reaches_sounding_sources_only_after_commit();
    puts("PASS: APSIS P5c Mother held Gravity reaches sounding sources only after ->1 commit");
    return 0;
}
