#include "apsis_visual_p8c.h"

static uint16_t apsis_p8c_clamp_q15(uint16_t value)
{
    return value > APSIS_Q15_ONE ? APSIS_Q15_ONE : value;
}

static void apsis_p8c_identity(ApsisVisualRaster *raster, uint16_t colour)
{
    apsis_visual_hline(raster, 106, 134, 8, APSIS_VISUAL_DIM);
    apsis_visual_hline(raster, 114, 126, 8, colour);
}

static uint8_t apsis_p8c_angle(uint8_t index, uint8_t count)
{
    if (!count) count = 1u;
    return (uint8_t)(48u + ((uint32_t)index * APSIS_VISUAL_ANGLE_STEPS) / count);
}

static uint8_t apsis_p8c_chain_source_slot(const ApsisChainSequence *seq,
                                            uint8_t *slot)
{
    uint8_t pos;
    if (!seq || !slot || !seq->count) return 0u;
    pos = seq->pos < seq->count ? seq->pos : 0u;
    if (pos >= APSIS_CHAIN_MAX) return 0u;
    *slot = seq->cells[pos].slot < APSIS_KEEP_SLOTS ? seq->cells[pos].slot :
                                                     (APSIS_KEEP_SLOTS - 1u);
    return (uint8_t)(seq->playing || seq->armed);
}

void apsis_visual_render_keep(ApsisVisualRaster *raster,
                              const ApsisVisualContext *context,
                              const ApsisKeepBank *keep_bank,
                              const ApsisChainSequence *chain_sequence)
{
    uint8_t i, chain_slot = 0u;
    uint8_t has_chain_slot;
    if (!raster || !context) return;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8c_identity(raster, APSIS_VISUAL_FG);

    has_chain_slot = apsis_p8c_chain_source_slot(chain_sequence, &chain_slot);
    apsis_visual_circle(raster, 120, 124, 76, APSIS_VISUAL_DIM);
    apsis_visual_circle(raster, 120, 124, 45, APSIS_VISUAL_DIM);

    for (i = 0u; i < APSIS_KEEP_SLOTS; ++i) {
        int16_t x, y;
        uint8_t valid = keep_bank ? keep_bank->slots[i].valid : 0u;
        uint8_t selected = keep_bank && keep_bank->selected_slot == (int8_t)i;
        uint8_t chain_source = has_chain_slot && chain_slot == i;
        uint8_t angle = apsis_p8c_angle(i, APSIS_KEEP_SLOTS);
        apsis_visual_polar(120, 124, 76, angle, &x, &y);
        apsis_visual_circle(raster, x, y, valid ? 9 : 6,
                            valid ? APSIS_VISUAL_MOTHER : APSIS_VISUAL_DIM);
        if (valid)
            apsis_visual_disc(raster, x, y, 3, APSIS_VISUAL_MOTHER);
        if (selected)
            apsis_visual_circle(raster, x, y, 13, APSIS_VISUAL_FG);
        if (chain_source)
            apsis_visual_circle(raster, x, y, 17, APSIS_VISUAL_CHAIN);
        if (valid) {
            int16_t ix, iy;
            apsis_visual_polar(120, 124, 45, angle, &ix, &iy);
            apsis_visual_line(raster, ix, iy, x, y,
                              chain_source ? APSIS_VISUAL_CHAIN : APSIS_VISUAL_DIM);
        }
    }

    /* Central memory witness: selected slot is direct, chain source is curved
     * around it instead of turning KEEP into a preset list. */
    apsis_visual_circle(raster, 120, 124, 18, APSIS_VISUAL_MOTHER);
    apsis_visual_disc(raster, 120, 124, 5, APSIS_VISUAL_FG);
    if (has_chain_slot)
        apsis_visual_arc(raster, 120, 124, 27, 48, 42, APSIS_VISUAL_CHAIN);
}

void apsis_visual_render_chain(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context,
                               const ApsisKeepBank *keep_bank,
                               const ApsisChainSequence *chain_sequence)
{
    uint8_t count, i;
    int16_t prev_x = 0, prev_y = 0;
    uint8_t have_prev = 0u;
    uint32_t progress_q15 = 0u;
    (void)keep_bank;
    if (!raster || !context) return;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8c_identity(raster, APSIS_VISUAL_CHAIN);

    count = chain_sequence ? chain_sequence->count : 0u;
    if (count > APSIS_CHAIN_MAX) count = APSIS_CHAIN_MAX;
    apsis_visual_circle(raster, 120, 124, 62, APSIS_VISUAL_DIM);
    apsis_visual_circle(raster, 120, 124, 94, APSIS_VISUAL_DIM);

    if (!count) {
        for (i = 0u; i < APSIS_KEEP_SLOTS; ++i) {
            int16_t x, y;
            apsis_visual_polar(120, 124, 78, apsis_p8c_angle(i, APSIS_KEEP_SLOTS), &x, &y);
            apsis_visual_circle(raster, x, y, 4, APSIS_VISUAL_DIM);
        }
        return;
    }

    for (i = 0u; i < count; ++i) {
        const ApsisChainCell *cell = &chain_sequence->cells[i];
        uint8_t slot = cell->slot < APSIS_KEEP_SLOTS ? cell->slot : (APSIS_KEEP_SLOTS - 1u);
        uint8_t angle = apsis_p8c_angle(i, count);
        int16_t radius = (int16_t)(64 + slot * 4);
        int16_t x, y;
        uint16_t node_colour = cell->chance_q15 < (APSIS_Q15_ONE / 2u) ?
                               APSIS_VISUAL_MUTED : APSIS_VISUAL_CHAIN;
        apsis_visual_polar(120, 124, radius, angle, &x, &y);
        if (have_prev)
            apsis_visual_line(raster, prev_x, prev_y, x, y, APSIS_VISUAL_CHAIN);
        apsis_visual_circle(raster, x, y, 4, node_colour);
        if (cell->chance_q15)
            apsis_visual_disc(raster, x, y, 1, node_colour);
        if (chain_sequence->selected == (int8_t)i)
            apsis_visual_circle(raster, x, y, 8, APSIS_VISUAL_FG);
        if ((chain_sequence->playing || chain_sequence->armed) &&
            chain_sequence->pos == i)
            apsis_visual_circle(raster, x, y, 11, APSIS_VISUAL_CLOCK);
        if (cell->transpose > 0)
            apsis_visual_line(raster, x, (int16_t)(y - 7), x, (int16_t)(y - 12), APSIS_VISUAL_FG);
        else if (cell->transpose < 0)
            apsis_visual_line(raster, x, (int16_t)(y + 7), x, (int16_t)(y + 12), APSIS_VISUAL_FG);
        prev_x = x;
        prev_y = y;
        have_prev = 1u;
    }

    if (count > 1u) {
        const ApsisChainCell *first = &chain_sequence->cells[0];
        uint8_t slot = first->slot < APSIS_KEEP_SLOTS ? first->slot : (APSIS_KEEP_SLOTS - 1u);
        int16_t x, y;
        apsis_visual_polar(120, 124, (int16_t)(64 + slot * 4), 48, &x, &y);
        apsis_visual_line(raster, prev_x, prev_y, x, y, APSIS_VISUAL_DIM);
    }

    if (chain_sequence->master_pulses) {
        uint32_t pulse = chain_sequence->phrase_pulse % chain_sequence->master_pulses;
        progress_q15 = (pulse * APSIS_Q15_ONE) / chain_sequence->master_pulses;
    }
    if (progress_q15) {
        uint8_t extent = (uint8_t)((progress_q15 * APSIS_VISUAL_ANGLE_STEPS) / APSIS_Q15_ONE);
        if (extent) apsis_visual_arc(raster, 120, 124, 106, 48, extent, APSIS_VISUAL_CHAIN);
    }
    apsis_visual_disc(raster, 120, 124, chain_sequence->playing ? 5 : 3,
                      chain_sequence->playing ? APSIS_VISUAL_CLOCK : APSIS_VISUAL_CHAIN);
}

static uint16_t apsis_p8c_space_send(const ApsisState *state, uint8_t index)
{
    if (!state) return 0u;
    switch (index) {
    case 0u: return state->string.send_q15;
    case 1u: return state->drift.send_q15;
    case 2u: return state->clock.send_q15;
    case 3u: return state->spiral.send_q15;
    default: return state->bloom.send_q15;
    }
}

void apsis_visual_render_space(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context)
{
    static const uint16_t source_colour[5] = {
        APSIS_VISUAL_STRING, APSIS_VISUAL_DRIFT, APSIS_VISUAL_CLOCK,
        APSIS_VISUAL_SPIRAL, APSIS_VISUAL_BLOOM
    };
    static const uint8_t source_angle[5] = { 42u, 55u, 0u, 9u, 22u };
    uint8_t i;
    int16_t wet_radius;
    if (!raster || !context || !context->state) return;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8c_identity(raster, APSIS_VISUAL_MOTHER);

    /* One common cathedral. Decay changes the depth of its nested arches;
     * individual source sends only change how far each coloured path enters. */
    apsis_visual_arc(raster, 120, 139, 72, 40, 48, APSIS_VISUAL_MOTHER);
    apsis_visual_arc(raster, 120, 139, 53, 40, 48, APSIS_VISUAL_MUTED);
    apsis_visual_arc(raster, 120, 139, 35, 40, 48, APSIS_VISUAL_DIM);
    apsis_visual_line(raster, 48, 139, 48, 202, APSIS_VISUAL_MOTHER);
    apsis_visual_line(raster, 192, 139, 192, 202, APSIS_VISUAL_MOTHER);
    apsis_visual_hline(raster, 48, 192, 202, APSIS_VISUAL_DIM);

    for (i = 0u; i < 5u; ++i) {
        uint16_t send = apsis_p8c_clamp_q15(apsis_p8c_space_send(context->state, i));
        int16_t sx, sy, ex, ey;
        int16_t inner = (int16_t)(96 - ((uint32_t)send * 70u) / APSIS_Q15_ONE);
        apsis_visual_polar(120, 126, 101, source_angle[i], &sx, &sy);
        apsis_visual_polar(120, 126, inner, source_angle[i], &ex, &ey);
        apsis_visual_circle(raster, sx, sy, 5, source_colour[i]);
        apsis_visual_disc(raster, sx, sy, 2, source_colour[i]);
        apsis_visual_line(raster, sx, sy, ex, ey, source_colour[i]);
        if (send > (APSIS_Q15_ONE / 2u))
            apsis_visual_disc(raster, ex, ey, 2, source_colour[i]);
    }

    wet_radius = (int16_t)(5 + ((uint32_t)apsis_p8c_clamp_q15(context->state->space.mix_q15) * 10u) /
                              APSIS_Q15_ONE);
    apsis_visual_circle(raster, 120, 139, wet_radius, APSIS_VISUAL_MOTHER);
    apsis_visual_disc(raster, 120, 139, 3, APSIS_VISUAL_FG);

    /* Echo is a single shared reflection witness; it is deliberately neutral,
     * not a sixth coloured source or five per-source echo indicators. */
    if (context->state->space.echo_feedback_q15) {
        int16_t echo_r = (int16_t)(18 + ((uint32_t)apsis_p8c_clamp_q15(context->state->space.echo_feedback_q15) * 20u) /
                                      APSIS_Q15_ONE);
        apsis_visual_arc(raster, 120, 139, echo_r, 7, 18, APSIS_VISUAL_MUTED);
    }
}

void apsis_visual_render_p8c(ApsisVisualRaster *raster,
                             const ApsisVisualContext *context,
                             const ApsisKeepBank *keep_bank,
                             const ApsisChainSequence *chain_sequence)
{
    if (!raster || !context) return;
    switch (context->surface) {
    case APSIS_VISUAL_SURFACE_KEEP:
        apsis_visual_render_keep(raster, context, keep_bank, chain_sequence);
        break;
    case APSIS_VISUAL_SURFACE_CHAIN:
        apsis_visual_render_chain(raster, context, keep_bank, chain_sequence);
        break;
    case APSIS_VISUAL_SURFACE_SPACE:
        apsis_visual_render_space(raster, context);
        break;
    default:
        apsis_visual_render_p8(raster, context);
        break;
    }
}
