#include "apsis_chain.h"

static void apsis_chain_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t apsis_chain_clock_n(uint8_t n)
{
    return n < 3u ? 3u : (n > 24u ? 24u : n);
}

static int8_t apsis_chain_clamp_transpose(int8_t v)
{
    return v < -12 ? -12 : (v > 12 ? 12 : v);
}

uint8_t apsis_chain_morph_valid(uint8_t q)
{
    return q == APSIS_CHAIN_MORPH_NONE || q == APSIS_CHAIN_MORPH_QUARTER ||
           q == APSIS_CHAIN_MORPH_HALF || q == APSIS_CHAIN_MORPH_FULL;
}

void apsis_chain_sequence_reset(ApsisChainSequence *seq)
{
    if (!seq) return;
    apsis_chain_zero(seq, (uint32_t)sizeof(*seq));
    seq->selected = -1;
    seq->last_applied = 1u;
}

ApsisChainCell apsis_chain_default_cell(uint8_t slot, const ApsisChainState *settings,
                                        uint8_t clock_n)
{
    ApsisChainCell c;
    uint8_t n = apsis_chain_clock_n(clock_n);
    c.slot = slot > 7u ? 7u : slot;
    c.cycles = settings ? settings->default_cycles : 4u;
    c.fine_pulses = settings ? settings->default_fine_pulses : 0u;
    if (c.cycles > 32u) c.cycles = 32u;
    if (c.fine_pulses >= n) c.fine_pulses = (uint8_t)(n - 1u);
    if (c.cycles == 0u && c.fine_pulses == 0u) c.cycles = 1u;
    c.chance_q15 = APSIS_Q15_ONE;
    c.transpose = 0;
    c.morph_quarters = settings && apsis_chain_morph_valid(settings->default_morph_quarters)
        ? settings->default_morph_quarters : APSIS_CHAIN_MORPH_NONE;
    return c;
}

ApsisChainCell apsis_chain_normalize_cell(const ApsisChainCell *cell,
                                          const ApsisChainState *settings,
                                          uint8_t clock_n)
{
    ApsisChainCell c = cell ? *cell : apsis_chain_default_cell(0u, settings, clock_n);
    uint8_t n = apsis_chain_clock_n(clock_n);
    uint8_t fallback_morph = settings && apsis_chain_morph_valid(settings->default_morph_quarters)
        ? settings->default_morph_quarters : APSIS_CHAIN_MORPH_NONE;

    if (c.slot > 7u) c.slot = 7u;
    if (c.cycles > 32u) c.cycles = 32u;
    if (c.fine_pulses >= n) c.fine_pulses = (uint8_t)(n - 1u);
    if (c.cycles == 0u && c.fine_pulses == 0u) c.cycles = 1u;
    if (c.chance_q15 > APSIS_Q15_ONE) c.chance_q15 = APSIS_Q15_ONE;
    c.transpose = apsis_chain_clamp_transpose(c.transpose);
    if (!apsis_chain_morph_valid(c.morph_quarters)) c.morph_quarters = fallback_morph;
    return c;
}

uint16_t apsis_chain_cell_pulses(const ApsisChainCell *cell,
                                 const ApsisChainState *settings,
                                 uint8_t clock_n)
{
    ApsisChainCell c = apsis_chain_normalize_cell(cell, settings, clock_n);
    uint16_t n = apsis_chain_clock_n(clock_n);
    uint16_t total = (uint16_t)((uint16_t)c.cycles * n + c.fine_pulses);
    return total ? total : 1u;
}

uint32_t apsis_chain_content_pulses(const ApsisChainSequence *seq,
                                    const ApsisChainState *settings,
                                    uint8_t clock_n)
{
    uint8_t i, count;
    uint32_t total = 0u;
    if (!seq) return 0u;
    count = seq->count > APSIS_CHAIN_MAX ? APSIS_CHAIN_MAX : seq->count;
    for (i = 0u; i < count; ++i)
        total += apsis_chain_cell_pulses(&seq->cells[i], settings, clock_n);
    return total;
}

uint32_t apsis_chain_master_pulses(const ApsisChainSequence *seq,
                                   const ApsisChainState *settings,
                                   uint8_t clock_n)
{
    uint32_t content = apsis_chain_content_pulses(seq, settings, clock_n);
    uint32_t n = apsis_chain_clock_n(clock_n);
    uint8_t sync_one = !settings || settings->loop_sync == APSIS_CHAIN_SYNC_ONE;
    if (!content) content = 1u;
    if (sync_one) return ((content + n - 1u) / n) * n;
    return content;
}

uint8_t apsis_chain_should_apply(uint16_t chance_q15, uint32_t random_u32)
{
    uint16_t chance = chance_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : chance_q15;
    return ((uint64_t)random_u32 * APSIS_Q15_ONE) <= ((uint64_t)chance << 32) ? 1u : 0u;
}

void apsis_chain_morph_targets(const ApsisKeepSnapshot *snap,
                               const ApsisChainState *settings,
                               uint8_t morph_quarters,
                               ApsisChainMorphTargets *out)
{
    if (!out) return;
    apsis_chain_zero(out, (uint32_t)sizeof(*out));
    if (!snap || !snap->valid || !settings || !apsis_chain_morph_valid(morph_quarters) ||
        morph_quarters == APSIS_CHAIN_MORPH_NONE) return;

    out->morph_quarters = morph_quarters;
    if (settings->scope_tension) {
        out->has_tension = 1u;
        out->target_left_split_q15 = snap->left_split_q15;
        out->target_right_split_q15 = snap->right_split_q15;
    }
    if (settings->scope_drift) {
        out->has_drift = 1u;
        out->target_drift_amount_q15 = snap->drift_amount_q15;
    }
}

uint8_t apsis_chain_apply_cell(ApsisState *state, const ApsisKeepBank *bank,
                               const ApsisChainState *settings,
                               const ApsisChainCell *cell, uint32_t random_u32,
                               ApsisChainMorphTargets *morph_targets)
{
    ApsisChainCell c;
    ApsisKeepApplyOptions o;
    const ApsisKeepSnapshot *snap;
    if (morph_targets) apsis_chain_zero(morph_targets, (uint32_t)sizeof(*morph_targets));
    if (!state || !bank || !settings || !cell) return 0u;

    c = apsis_chain_normalize_cell(cell, settings, state->clock.n);
    snap = &bank->slots[c.slot];
    if (!snap->valid) return 0u;
    if (!apsis_chain_should_apply(c.chance_q15, random_u32)) return 0u;

    if (morph_targets) apsis_chain_morph_targets(snap, settings, c.morph_quarters, morph_targets);
    o.from_chain = 1u;
    o.skip_continuous = c.morph_quarters != APSIS_CHAIN_MORPH_NONE;
    o.transpose = c.transpose;
    o.lane_mask = apsis_keep_lane_mask_from_chain(settings);
    return apsis_keep_apply(state, snap, &o);
}
