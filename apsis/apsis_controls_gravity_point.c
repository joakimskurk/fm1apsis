#include "apsis_controls_gravity_point.h"

#define P10G_LEVEL_MIN APSIS_Q15_MILLI(150)
#define P10G_LEVEL_MAX APSIS_Q15_MILLI(1250)
#define P10G_BRIGHT_MIN APSIS_Q15_MILLI(150)
#define P10G_BRIGHT_MAX APSIS_Q15_MILLI(780)
#define P10G_Q15_STEP APSIS_Q15_MILLI(10)
#define P10G_ATTACK_STEP_MS 4
#define P10G_RELEASE_STEP_MS 25
#define P10G_DIRECT_DEGREE_MASK 0xffu

static uint32_t p10g_button_bit(uint8_t button)
{
    return 1u << button;
}

static int32_t p10g_clamp_i32(int64_t value, int32_t lo, int32_t hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return (int32_t)value;
}

static uint16_t p10g_step_u16(uint16_t current, int32_t detents,
                              int32_t step, uint16_t lo, uint16_t hi)
{
    int64_t value = (int64_t)current + (int64_t)detents * step;
    if (value < lo) return lo;
    if (value > hi) return hi;
    return (uint16_t)value;
}

static uint8_t p10g_first_degree(uint32_t notes)
{
    uint8_t degree;
    for (degree = 0u; degree < APSIS_GRAVITY_DEGREES; ++degree)
        if (notes & (1u << degree)) return degree;
    return 0u;
}

static uint8_t p10g_ring(uint8_t ring)
{
    return ring < APSIS_GRAVITY_RINGS ? ring : 0u;
}

static uint8_t p10g_degree(uint8_t degree)
{
    return degree < APSIS_GRAVITY_DEGREES ? degree : 0u;
}

static uint8_t p10g_point_index(uint8_t degree, uint8_t ring)
{
    return (uint8_t)(p10g_degree(degree) * APSIS_GRAVITY_RINGS + p10g_ring(ring));
}

static void p10g_snapshot(const ApsisGravityState *gravity,
                          ApsisGravityPointControlResult *out)
{
    uint8_t ring, degree, index;
    const ApsisGravityPoint *point;
    if (!gravity || !out) return;
    ring = p10g_ring(gravity->selected_ring);
    degree = p10g_degree(gravity->selected_point_degree);
    index = p10g_point_index(degree, ring);
    point = &gravity->points[index];
    out->ring = ring;
    out->degree = degree;
    out->point_index = index;
    out->enabled = point->enabled ? 1u : 0u;
    out->offset = point->offset;
    out->octave = point->octave;
    out->level_q15 = point->level_q15;
    out->brightness_q15 = point->brightness_q15;
    out->attack_ms = point->attack_ms;
    out->release_ms = point->release_ms;
}

void apsis_controls_gravity_point_editor_reset(
    ApsisGravityPointEditorSession *session)
{
    if (session) session->page = APSIS_GRAVITY_PAGE_HARMONY;
}

uint8_t apsis_controls_gravity_point_context(
    ApsisGravityPointEditorSession *session,
    uint8_t current_surface,
    const ApsisFm1InputFrame *input,
    ApsisGravityPointContextResult *out)
{
    uint8_t before, next;
    int32_t d = 0;

    if (out) {
        out->page = session ? session->page : APSIS_GRAVITY_PAGE_HARMONY;
        out->changed = 0u;
        out->entered_point = 0u;
        out->exited_point = 0u;
    }
    if (!session) return 0u;

    before = session->page < APSIS_GRAVITY_PAGE_COUNT
        ? session->page : APSIS_GRAVITY_PAGE_HARMONY;
    next = before;
    if (current_surface != APSIS_SURFACE_GRAVITY) {
        next = APSIS_GRAVITY_PAGE_HARMONY;
    } else if (input) {
        d = input->encoder_steps[APSIS_FM1_ENC_ALGORITHM];
        if (d) {
            int32_t page = ((int32_t)before + d) % APSIS_GRAVITY_PAGE_COUNT;
            if (page < 0) page += APSIS_GRAVITY_PAGE_COUNT;
            next = (uint8_t)page;
        }
    }
    session->page = next;
    if (out) {
        out->page = next;
        out->changed = (uint8_t)(before != next);
        out->entered_point = (uint8_t)(before != APSIS_GRAVITY_PAGE_POINT &&
                                       next == APSIS_GRAVITY_PAGE_POINT);
        out->exited_point = (uint8_t)(before == APSIS_GRAVITY_PAGE_POINT &&
                                      next != APSIS_GRAVITY_PAGE_POINT);
    }
    return before != next;
}

uint8_t apsis_controls_gravity_point_edit(
    ApsisGravityPointEditorSession *session,
    ApsisGravityState *gravity,
    const ApsisFm1InputFrame *input,
    ApsisGravityPointControlResult *out)
{
    ApsisGravityPoint *point;
    uint32_t direct_notes;
    uint8_t ring, degree, index;
    int32_t d;

    if (out) {
        out->handled = 0u;
        out->state_changed = 0u;
        out->selection_changed = 0u;
        out->suppress_degree_request = 0u;
        out->ring = 0u;
        out->degree = 0u;
        out->point_index = 0u;
        out->enabled = 0u;
        out->offset = 0;
        out->octave = 0;
        out->level_q15 = 0u;
        out->brightness_q15 = 0u;
        out->attack_ms = 0u;
        out->release_ms = 0u;
    }
    if (!session || !gravity || !input ||
        session->page != APSIS_GRAVITY_PAGE_POINT) return 0u;

    if (out) out->handled = 1u;
    direct_notes = input->notes_pressed & P10G_DIRECT_DEGREE_MASK;
    if (direct_notes &&
        (input->buttons_held & p10g_button_bit(APSIS_FM1_BTN_SEL))) {
        uint8_t selected = p10g_first_degree(direct_notes);
        if (gravity->selected_point_degree != selected) {
            gravity->selected_point_degree = selected;
            if (out) {
                out->state_changed = 1u;
                out->selection_changed = 1u;
            }
        }
        if (out) out->suppress_degree_request = 1u;
    }

    ring = p10g_ring(gravity->selected_ring);
    degree = p10g_degree(gravity->selected_point_degree);
    index = p10g_point_index(degree, ring);
    point = &gravity->points[index];

    if (input->buttons_pressed & p10g_button_bit(APSIS_FM1_BTN_REC)) {
        point->enabled = point->enabled ? 0u : 1u;
        if (out) out->state_changed = 1u;
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) {
        int8_t next = (int8_t)p10g_clamp_i32((int64_t)point->offset + d, -2, 2);
        if (next != point->offset) {
            point->offset = next;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) {
        int8_t next = (int8_t)p10g_clamp_i32((int64_t)point->octave + d, -1, 1);
        if (next != point->octave) {
            point->octave = next;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) {
        uint16_t next = p10g_step_u16(point->level_q15, d, P10G_Q15_STEP,
                                      P10G_LEVEL_MIN, P10G_LEVEL_MAX);
        if (next != point->level_q15) {
            point->level_q15 = next;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K4];
    if (d) {
        uint16_t next = p10g_step_u16(point->brightness_q15, d, P10G_Q15_STEP,
                                      P10G_BRIGHT_MIN, P10G_BRIGHT_MAX);
        if (next != point->brightness_q15) {
            point->brightness_q15 = next;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_SELECT];
    if (d) {
        uint16_t next = p10g_step_u16(point->attack_ms, d, P10G_ATTACK_STEP_MS,
                                      12u, 220u);
        if (next != point->attack_ms) {
            point->attack_ms = next;
            if (out) out->state_changed = 1u;
        }
    }

    d = input->encoder_steps[APSIS_FM1_ENC_PRESETS];
    if (d) {
        uint16_t next = p10g_step_u16(point->release_ms, d, P10G_RELEASE_STEP_MS,
                                      250u, 3000u);
        if (next != point->release_ms) {
            point->release_ms = next;
            if (out) out->state_changed = 1u;
        }
    }

    p10g_snapshot(gravity, out);
    return 1u;
}
