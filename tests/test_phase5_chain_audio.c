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
    uint8_t chain_pos;
} ChainPoint;

typedef struct {
    ChainPoint spiral_boundary;
    ChainPoint bloom_boundary;
    ChainPoint spiral_after;
    ChainPoint bloom_after;
} ChainResult;

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

static void configure_audio(ApsisState *s)
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

static ChainPoint spiral_point(const ApsisState *s,
                               const ApsisChainSequence *chain,
                               const ApsisSpiralAudioBinding *binding,
                               uint64_t block_start)
{
    ApsisSpiralDsp dsp;
    ChainPoint p;
    uint8_t leg;
    memset(&p, 0, sizeof(p));
    apsis_spiral_dsp_reset(&dsp);
    assert(apsis_spiral_dsp_trigger_binding(&dsp, s, binding, block_start) > 0u);
    leg = first_spiral_leg(&dsp);
    assert(leg < APSIS_SPIRAL_DSP_LEG_CAP);
    p.valid = 1u;
    p.pitch_step = binding->pitch.step;
    p.phase_inc = dsp.leg[leg].osc[0].phase_inc;
    p.bind_sample = binding->bind_sample;
    p.target_sample = binding->target_sample;
    p.state_degree = s->gravity.degree;
    p.chain_pos = chain->pos;
    return p;
}

static ChainPoint bloom_point(const ApsisState *s,
                              const ApsisChainSequence *chain,
                              const ApsisBloomAudioBinding *binding,
                              uint64_t block_start)
{
    ApsisBloomDsp dsp;
    ChainPoint p;
    uint16_t petal;
    memset(&p, 0, sizeof(p));
    apsis_bloom_dsp_reset(&dsp);
    assert(apsis_bloom_dsp_trigger_binding(&dsp, s, binding, block_start) == 1u);
    petal = first_bloom_petal(&dsp);
    assert(petal < APSIS_BLOOM_DSP_PETAL_CAP);
    p.valid = 1u;
    p.pitch_step = binding->pitch.step;
    p.phase_inc = dsp.petal[petal].osc[0].phase_inc;
    p.bind_sample = binding->bind_sample;
    p.target_sample = binding->target_sample;
    p.state_degree = s->gravity.degree;
    p.chain_pos = chain->pos;
    return p;
}

static void configure_chain(ApsisState *s, ApsisKeepBank *bank,
                            ApsisChainSequence *chain, uint8_t second_degree)
{
    ApsisState source;
    apsis_state_default(&source);
    source.gravity.degree = 0u;
    bank->slots[0] = apsis_keep_capture(&source);
    source.gravity.degree = second_degree;
    bank->slots[1] = apsis_keep_capture(&source);

    s->chain.scope_chord = 1u;
    s->chain.scope_key_scale = 0u;
    s->chain.scope_tension = 0u;
    s->chain.scope_drift = 0u;
    s->chain.start_sync = APSIS_CHAIN_SYNC_ONE;
    s->chain.loop_sync = APSIS_CHAIN_SYNC_ONE;

    chain->count = 2u;
    chain->cells[0] = apsis_chain_default_cell(0u, &s->chain, s->clock.n);
    chain->cells[0].cycles = 0u;
    chain->cells[0].fine_pulses = 3u;
    chain->cells[0].morph_quarters = APSIS_CHAIN_MORPH_NONE;
    chain->cells[1] = apsis_chain_default_cell(1u, &s->chain, s->clock.n);
    chain->cells[1].cycles = 0u;
    chain->cells[1].fine_pulses = 3u;
    chain->cells[1].morph_quarters = APSIS_CHAIN_MORPH_NONE;
}

static ChainResult run_case(uint8_t second_degree)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState voicing;
    ApsisSpiralAudioRuntime spiral_audio;
    ApsisBloomAudioRuntime bloom_audio;
    ChainResult out;
    const uint64_t boundary = 11025u;
    const uint64_t spiral_after = 14700u;
    const uint64_t bloom_after = 22050u;
    uint32_t guard, i;

    memset(&out, 0, sizeof(out));
    init_all(&s, &r, &bank, &chain, &mother, &voicing, 0x50354348u);
    configure_audio(&s);
    configure_chain(&s, &bank, &chain, second_degree);
    apsis_runtime_chain_start(&r, &s, &chain);
    apsis_runtime_start_bloom_audio(&r, &s, 0, &spiral_audio, &bloom_audio);

    for (guard = 0u; guard < 180u; ++guard) {
        uint64_t block_start = r.cursor_sample;
        (void)apsis_runtime_process_bloom_audio(&r, &s, &bank, &chain, &mother,
                                                &voicing, 256u, 0, 0, 0,
                                                &spiral_audio, &bloom_audio, 0);
        for (i = 0u; i < spiral_audio.binding_count; ++i) {
            const ApsisSpiralAudioBinding *b = &spiral_audio.binding[i];
            if (!b->valid) continue;
            if (!out.spiral_boundary.valid && b->target_sample == boundary)
                out.spiral_boundary = spiral_point(&s, &chain, b, block_start);
            if (!out.spiral_after.valid && b->target_sample == spiral_after)
                out.spiral_after = spiral_point(&s, &chain, b, block_start);
        }
        for (i = 0u; i < bloom_audio.binding_count; ++i) {
            const ApsisBloomAudioBinding *b = &bloom_audio.binding[i];
            if (!b->valid || b->petal_index != 0u) continue;
            if (!out.bloom_boundary.valid && b->target_sample == boundary)
                out.bloom_boundary = bloom_point(&s, &chain, b, block_start);
            if (!out.bloom_after.valid && b->target_sample == bloom_after)
                out.bloom_after = bloom_point(&s, &chain, b, block_start);
        }
        if (out.spiral_boundary.valid && out.bloom_boundary.valid &&
            out.spiral_after.valid && out.bloom_after.valid)
            break;
    }

    assert(out.spiral_boundary.valid == 1u);
    assert(out.bloom_boundary.valid == 1u);
    assert(out.spiral_after.valid == 1u);
    assert(out.bloom_after.valid == 1u);
    assert(out.spiral_boundary.bind_sample == 7497u);
    assert(out.bloom_boundary.bind_sample == 9040u);
    assert(out.spiral_after.bind_sample == 11172u);
    return out;
}

static void test_chain_discrete_apply_respects_sounding_bind_order(void)
{
    ChainResult control = run_case(0u);
    ChainResult changed = run_case(4u);

    /* The target-11,025 voices bind before Chain applies cell 1 at 9,967. */
    assert(changed.spiral_boundary.state_degree == 0u);
    assert(changed.bloom_boundary.state_degree == 0u);
    assert(changed.spiral_boundary.chain_pos == 0u);
    assert(changed.bloom_boundary.chain_pos == 0u);
    assert(changed.spiral_boundary.pitch_step == control.spiral_boundary.pitch_step);
    assert(changed.spiral_boundary.phase_inc == control.spiral_boundary.phase_inc);
    assert(changed.bloom_boundary.pitch_step == control.bloom_boundary.pitch_step);
    assert(changed.bloom_boundary.phase_inc == control.bloom_boundary.phase_inc);

    /* Cell 1 has applied before the next source-defined bind points. */
    assert(changed.spiral_after.state_degree == 4u);
    assert(changed.spiral_after.chain_pos == 1u);
    assert(changed.spiral_after.pitch_step != control.spiral_after.pitch_step);
    assert(changed.spiral_after.phase_inc != control.spiral_after.phase_inc);

    assert(changed.bloom_after.state_degree == 4u);
    assert(changed.bloom_after.pitch_step != control.bloom_after.pitch_step);
    assert(changed.bloom_after.phase_inc != control.bloom_after.phase_inc);

    assert(changed.spiral_boundary.target_sample == control.spiral_boundary.target_sample);
    assert(changed.bloom_boundary.target_sample == control.bloom_boundary.target_sample);
    assert(changed.spiral_after.target_sample == control.spiral_after.target_sample);
    assert(changed.bloom_after.target_sample == control.bloom_after.target_sample);
}

int main(void)
{
    test_chain_discrete_apply_respects_sounding_bind_order();
    puts("PASS: APSIS P5d Chain discrete boundary reaches sounding Spiral/Bloom in source order");
    return 0;
}
