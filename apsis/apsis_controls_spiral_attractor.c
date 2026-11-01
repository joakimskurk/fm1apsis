#include "apsis_controls_spiral_attractor.h"

#define P10F_LOCK_STEP APSIS_Q15_MILLI(10)
#define P10F_DENSITY_STEP APSIS_Q15_MILLI(10)
#define P10F_ENERGY_STEP APSIS_Q15_MILLI(10)
#define P10F_DENSITY_MIN APSIS_Q15_MILLI(150)
#define P10F_DENSITY_MAX APSIS_Q15_MILLI(900)
#define P10F_ENERGY_MIN APSIS_Q15_MILLI(120)
#define P10F_ENERGY_MAX APSIS_Q15_ONE

static uint32_t p10f_button_bit(uint8_t button)
{
    return 1u << button;
}

static uint16_t p10f_step_q15(uint16_t current, int32_t detents,
                              uint16_t step, uint16_t lo, uint16_t hi)
{
    int64_t next = (int64_t)current + (int64_t)detents * step;
    if (next < (int64_t)lo) return lo;
    if (next > (int64_t)hi) return hi;
    return (uint16_t)next;
}

static uint8_t p10f_rate(uint8_t rate)
{
    if (rate == 1u || rate == 2u || rate == 4u) return rate;
    return 2u;
}

static uint8_t p10f_rate_step(uint8_t rate, int32_t detents)
{
    int32_t index;
    rate = p10f_rate(rate);
    index = rate == 1u ? 0 : (rate == 2u ? 1 : 2);
    index += detents;
    if (index < 0) index = 0;
    if (index > 2) index = 2;
    return index == 0 ? 1u : (index == 1 ? 2u : 4u);
}

static uint32_t p10f_gate_mask(uint8_t length)
{
    length = apsis_spiral_attractor_length(length);
    return length >= 32u ? 0xffffffffu : ((1u << length) - 1u);
}

static uint8_t p10f_first_cell(uint32_t bits, uint8_t length)
{
    uint8_t i;
    for (i = 0u; i < length; ++i)
        if (bits & (1u << i)) return i;
    return 0u;
}

static void p10f_result_snapshot(const ApsisSpiralAttractorEditorSession *session,
                                 const ApsisSpiralState *spiral,
                                 ApsisSpiralAttractorControlResult *out)
{
    uint8_t length, selected;
    if (!out || !spiral) return;
    length = apsis_spiral_attractor_length(spiral->attractor_length);
    selected = session ? session->selected_cell : 0u;
    if (selected >= length) selected = 0u;
    out->selected_cell = selected;
    out->rate = p10f_rate(spiral->attractor_rate);
    out->length = length;
    out->lock_q15 = spiral->attractor_lock_q15 > APSIS_Q15_ONE
        ? APSIS_Q15_ONE : spiral->attractor_lock_q15;
    out->density_q15 = spiral->attractor_density_q15 < P10F_DENSITY_MIN
        ? P10F_DENSITY_MIN
        : (spiral->attractor_density_q15 > P10F_DENSITY_MAX
            ? P10F_DENSITY_MAX : spiral->attractor_density_q15);
    out->energy_q15 = spiral->attractor_energy_q15[selected] < P10F_ENERGY_MIN
        ? P10F_ENERGY_MIN
        : (spiral->attractor_energy_q15[selected] > P10F_ENERGY_MAX
            ? P10F_ENERGY_MAX : spiral->attractor_energy_q15[selected]);
}

void apsis_controls_spiral_attractor_editor_reset(
    ApsisSpiralAttractorEditorSession *session)
{
    if (!session) return;
    session->active = 0u;
    session->selected_cell = 0u;
}

uint8_t apsis_controls_spiral_attractor_context(
    ApsisSpiralAttractorEditorSession *session,
    uint8_t current_surface,
    const ApsisFm1InputFrame *input,
    ApsisSpiralAttractorContextResult *out)
{
    uint8_t before, next;
    int32_t algorithm = 0;

    if (out) {
        out->active = session ? session->active : 0u;
        out->entered = 0u;
        out->exited = 0u;
    }
    if (!session) return 0u;

    before = session->active ? 1u : 0u;
    next = before;
    if (current_surface != APSIS_SURFACE_SPIRAL) {
        next = 0u;
    } else if (input) {
        algorithm = input->encoder_steps[APSIS_FM1_ENC_ALGORITHM];
        if ((algorithm % 2) != 0) next = (uint8_t)!before;
    }

    if (next != before) session->selected_cell = 0u;
    session->active = next;
    if (out) {
        out->active = next;
        out->entered = (uint8_t)(!before && next);
        out->exited = (uint8_t)(before && !next);
    }
    return before != next;
}

uint8_t apsis_controls_spiral_attractor_edit(
    ApsisSpiralAttractorEditorSession *session,
    ApsisSpiralState *spiral,
    const ApsisFm1InputFrame *input,
    ApsisSpiralAttractorControlResult *out)
{
    uint8_t length, selected;
    uint32_t toggled;
    uint8_t next_rate;
    uint16_t next_q15;
    int32_t d;

    if (out) {
        out->handled = 0u;
        out->state_changed = 0u;
        out->rerolled = 0u;
        out->suppress_route_dispatch = 0u;
        out->selected_cell = 0u;
        out->rate = 0u;
        out->length = 0u;
        out->lock_q15 = 0u;
        out->density_q15 = 0u;
        out->energy_q15 = 0u;
        out->toggled_gates = 0u;
    }
    if (!session || !spiral || !input || !session->active) return 0u;

    if (out) {
        out->handled = 1u;
        out->suppress_route_dispatch = 1u;
    }

    length = apsis_spiral_attractor_length(spiral->attractor_length);
    if (session->selected_cell >= length) session->selected_cell = 0u;

    /* 1. Continuous/discrete attractor parameters. */
    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) {
        next_rate = p10f_rate_step(spiral->attractor_rate, d);
        if (next_rate != spiral->attractor_rate) {
            spiral->attractor_rate = next_rate;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) {
        next_q15 = p10f_step_q15(spiral->attractor_lock_q15, d,
                                 P10F_LOCK_STEP, 0u, APSIS_Q15_ONE);
        if (next_q15 != spiral->attractor_lock_q15) {
            spiral->attractor_lock_q15 = next_q15;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) {
        next_q15 = p10f_step_q15(spiral->attractor_density_q15, d,
                                 P10F_DENSITY_STEP,
                                 P10F_DENSITY_MIN, P10F_DENSITY_MAX);
        if (next_q15 != spiral->attractor_density_q15) {
            spiral->attractor_density_q15 = next_q15;
            if (out) out->state_changed = 1u;
        }
    }

    /* 2. Source-authority reroll, after density edit so a same-frame density
     * detent participates in deterministic regeneration. */
    if (input->buttons_pressed & p10f_button_bit(APSIS_FM1_BTN_REC)) {
        apsis_spiral_reset_attractor(spiral, 1u);
        length = apsis_spiral_attractor_length(spiral->attractor_length);
        if (session->selected_cell >= length) session->selected_cell = 0u;
        if (out) {
            out->state_changed = 1u;
            out->rerolled = 1u;
        }
    }

    /* 3. Direct gates. Keys outside the active source length are consumed but
     * never mutate hidden cells. Lowest simultaneous cell is deterministic. */
    toggled = input->notes_pressed & p10f_gate_mask(length);
    if (toggled) {
        spiral->attractor_gates ^= toggled;
        session->selected_cell = p10f_first_cell(toggled, length);
        if (out) {
            out->state_changed = 1u;
            out->toggled_gates = toggled;
        }
    }

    /* 4. Selected-cell energy, after direct selection and reroll. */
    selected = session->selected_cell < length ? session->selected_cell : 0u;
    d = input->encoder_steps[APSIS_FM1_ENC_K4];
    if (d) {
        next_q15 = p10f_step_q15(spiral->attractor_energy_q15[selected], d,
                                 P10F_ENERGY_STEP,
                                 P10F_ENERGY_MIN, P10F_ENERGY_MAX);
        if (next_q15 != spiral->attractor_energy_q15[selected]) {
            spiral->attractor_energy_q15[selected] = next_q15;
            if (out) out->state_changed = 1u;
        }
    }

    p10f_result_snapshot(session, spiral, out);
    return 1u;
}
