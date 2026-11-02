#include <assert.h>
#include <stdio.h>

#include "apsis_runtime_m4g.h"
#include "apsis_drift_dsp.h"

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, 0x5035454du);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static void process_to(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                       ApsisChainSequence *chain, ApsisMotherPending *mother,
                       ApsisBloomVoicingState *bloom, uint64_t target)
{
    while (r->cursor_sample < target) {
        uint64_t remain = target - r->cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        apsis_runtime_process(r, s, bank, chain, mother, bloom, frames, 0, 0);
    }
}

static void configure_clock(ApsisState *s)
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
}

static void configure_chain(ApsisState *s, ApsisKeepBank *bank,
                            ApsisChainSequence *chain,
                            uint16_t old_amount, uint16_t new_amount)
{
    ApsisState source;
    apsis_state_default(&source);
    source.drift.amount_q15 = old_amount;
    bank->slots[0] = apsis_keep_capture(&source);
    source.drift.amount_q15 = new_amount;
    bank->slots[1] = apsis_keep_capture(&source);

    s->chain.scope_key_scale = 0u;
    s->chain.scope_chord = 0u;
    s->chain.scope_tension = 0u;
    s->chain.scope_drift = 1u;
    s->chain.start_sync = APSIS_CHAIN_SYNC_ONE;
    s->chain.loop_sync = APSIS_CHAIN_SYNC_ONE;

    chain->count = 2u;
    chain->cells[0] = apsis_chain_default_cell(0u, &s->chain, s->clock.n);
    chain->cells[0].cycles = 0u;
    chain->cells[0].fine_pulses = 1u;
    chain->cells[0].morph_quarters = APSIS_CHAIN_MORPH_NONE;
    chain->cells[1] = apsis_chain_default_cell(1u, &s->chain, s->clock.n);
    chain->cells[1].cycles = 0u;
    chain->cells[1].fine_pulses = 3u;
    chain->cells[1].morph_quarters = APSIS_CHAIN_MORPH_QUARTER;
}

static void test_chain_morph_starts_on_audio_target(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisDriftDsp dsp;
    const uint16_t old_amount = APSIS_Q15_MILLI(200);
    const uint16_t new_amount = APSIS_Q15_MILLI(800);
    const uint64_t target = 3675u;
    const uint64_t observe = 2617u; /* target - nearest 24 ms */
    uint64_t morph_end, mid;
    uint32_t old_target_inc, mid_target_inc, before_render_inc, after_render_inc;
    int32_t scratch[256u * 2u] = {0};

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    configure_clock(&s);
    configure_chain(&s, &bank, &chain, old_amount, new_amount);
    apsis_runtime_chain_start(&r, &s, &chain);
    apsis_runtime_start(&r);

    /* Startup Clock-1 applies cell 0. */
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(chain.pos == 0u);
    assert(s.drift.amount_q15 == old_amount);

    apsis_drift_dsp_reset(&dsp);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    old_target_inc = dsp.voice[APSIS_DRIFT_DSP_LEFT].target_phase_inc;
    assert(dsp.voice[APSIS_DRIFT_DSP_LEFT].phase_inc == old_target_inc);

    /* The discrete callback selects cell 1 early but must not move the
     * continuous lane. */
    process_to(&r, &s, &bank, &chain, &mother, &bloom, observe);
    assert(chain.pos == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(chain.pos == 1u);
    assert(s.drift.amount_q15 == old_amount);
    assert(r.chain_morph_active == 1u);
    assert(r.chain_morph_start_sample == target);
    morph_end = r.chain_morph_end_sample;
    assert(morph_end > target);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    assert(dsp.voice[APSIS_DRIFT_DSP_LEFT].target_phase_inc == old_target_inc);

    /* Exactly at target, t=0: still no audible morph advance. */
    process_to(&r, &s, &bank, &chain, &mother, &bloom, target);
    assert(s.drift.amount_q15 == old_amount);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    assert(dsp.voice[APSIS_DRIFT_DSP_LEFT].target_phase_inc == old_target_inc);

    /* Mid-morph state and actual oscillator target have moved, but the
     * smoothed sounding oscillator has not jumped instantaneously. */
    mid = target + (morph_end - target) / 2u;
    process_to(&r, &s, &bank, &chain, &mother, &bloom, mid);
    assert(s.drift.amount_q15 > old_amount);
    assert(s.drift.amount_q15 < new_amount);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    mid_target_inc = dsp.voice[APSIS_DRIFT_DSP_LEFT].target_phase_inc;
    assert(mid_target_inc != old_target_inc);
    before_render_inc = dsp.voice[APSIS_DRIFT_DSP_LEFT].phase_inc;
    assert(before_render_inc == old_target_inc);
    apsis_drift_dsp_render(&dsp, scratch, 256u);
    after_render_inc = dsp.voice[APSIS_DRIFT_DSP_LEFT].phase_inc;
    assert(after_render_inc != old_target_inc);
    if (mid_target_inc > old_target_inc) {
        assert(after_render_inc > old_target_inc);
        assert(after_render_inc <= mid_target_inc);
    } else {
        assert(after_render_inc < old_target_inc);
        assert(after_render_inc >= mid_target_inc);
    }

    process_to(&r, &s, &bank, &chain, &mother, &bloom, morph_end);
    assert(s.drift.amount_q15 == new_amount);
    assert(r.chain_morph_active == 0u);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    assert(dsp.voice[APSIS_DRIFT_DSP_LEFT].target_phase_inc != old_target_inc);
}

int main(void)
{
    test_chain_morph_starts_on_audio_target();
    puts("PASS: APSIS P5e Chain continuous Drift morph begins on target and reaches sounding DSP");
    return 0;
}
