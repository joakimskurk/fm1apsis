#include "apsis_visual_p8b.h"

static uint16_t apsis_p8b_clamp_q15(uint16_t value)
{
    return value > APSIS_Q15_ONE ? APSIS_Q15_ONE : value;
}

static int16_t apsis_p8b_map_q15(uint16_t value, int16_t lo, int16_t hi)
{
    uint32_t q = apsis_p8b_clamp_q15(value);
    return (int16_t)(lo + (int32_t)((q * (uint32_t)(hi - lo) + APSIS_Q15_ONE / 2u) /
                                     APSIS_Q15_ONE));
}

static void apsis_p8b_surface_mark(ApsisVisualRaster *raster, uint16_t colour)
{
    apsis_visual_hline(raster, 106, 134, 8, APSIS_VISUAL_DIM);
    apsis_visual_hline(raster, 114, 126, 8, colour);
}

static void apsis_p8b_step_dot(ApsisVisualRaster *raster, int16_t x, int16_t y,
                               uint8_t enabled, uint16_t colour)
{
    apsis_visual_disc(raster, x, y, enabled ? 2 : 1,
                      enabled ? colour : APSIS_VISUAL_DIM);
}

void apsis_visual_render_string(ApsisVisualRaster *raster,
                                const ApsisVisualContext *context)
{
    const ApsisStringState *string;
    int16_t x;
    uint8_t i;
    if (!raster || !context || !context->state) return;
    string = &context->state->string;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8b_surface_mark(raster, APSIS_VISUAL_STRING);

    /* The central 1:1 point is structural, never a text label. */
    apsis_visual_hline(raster, 24, 216, 122, APSIS_VISUAL_DIM);
    apsis_visual_line(raster, 120, 94, 120, 151, APSIS_VISUAL_MUTED);
    apsis_visual_circle(raster, 120, 122, 7, APSIS_VISUAL_DIM);
    apsis_visual_disc(raster, 120, 122, 2, APSIS_VISUAL_STRING);

    if (string->surface_mode == APSIS_STRING_DUAL) {
        int16_t left = apsis_p8b_map_q15(string->left_split_q15, 24, 216);
        int16_t right = apsis_p8b_map_q15(string->right_split_q15, 24, 216);
        uint16_t lc = string->active_side == APSIS_SIDE_LEFT ? APSIS_VISUAL_FG : APSIS_VISUAL_STRING;
        uint16_t rc = string->active_side == APSIS_SIDE_RIGHT ? APSIS_VISUAL_FG : APSIS_VISUAL_STRING;
        apsis_visual_line(raster, 120, 121, left, 121, APSIS_VISUAL_STRING);
        apsis_visual_line(raster, 120, 123, right, 123, APSIS_VISUAL_STRING);
        apsis_visual_disc(raster, left, 121, 5, lc);
        apsis_visual_circle(raster, left, 121, 8, APSIS_VISUAL_STRING);
        apsis_visual_disc(raster, right, 123, 5, rc);
        apsis_visual_circle(raster, right, 123, 8, APSIS_VISUAL_STRING);
        if (string->linked)
            apsis_visual_arc(raster, 120, 122, 23, 32, 32, APSIS_VISUAL_STRING);
    } else {
        x = apsis_p8b_map_q15(string->split_q15, 24, 216);
        apsis_visual_line(raster, 120, 122, x, 122, APSIS_VISUAL_STRING);
        apsis_visual_disc(raster, x, 122, 6, APSIS_VISUAL_STRING);
        apsis_visual_circle(raster, x, 122, 9, APSIS_VISUAL_FG);
    }

    /* Rhythm and attractor remain layers of the String field, not menu rows. */
    for (i = 0u; i < 16u; ++i) {
        int16_t px = (int16_t)(36 + i * 11);
        apsis_p8b_step_dot(raster, px, 181,
                           (uint8_t)((string->rhythm_mask >> i) & 1u),
                           APSIS_VISUAL_STRING);
        apsis_p8b_step_dot(raster, px, 204,
                           (uint8_t)((string->filter_attractor_gates >> i) & 1u),
                           APSIS_VISUAL_TUNING);
    }
    if (string->motion_loop_play) {
        uint8_t length = string->motion_loop_length;
        int16_t end;
        if (length < 1u) length = 1u;
        if (length > 24u) length = 24u;
        end = (int16_t)(36 + ((uint32_t)(length - 1u) * 165u) / 23u);
        apsis_visual_hline(raster, 36, end, 218, APSIS_VISUAL_STRING);
        apsis_visual_disc(raster, end, 218, 2, APSIS_VISUAL_FG);
    }
}

void apsis_visual_render_drift(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context)
{
    const ApsisDriftState *drift;
    const ApsisTemperState *temper;
    int16_t separation, radius, inner;
    int16_t lx, rx;
    uint8_t i;
    static const int16_t station_x[3] = { 72, 120, 168 };
    if (!raster || !context || !context->state) return;
    drift = &context->state->drift;
    temper = &context->state->temper;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8b_surface_mark(raster, APSIS_VISUAL_DRIFT);

    separation = apsis_p8b_map_q15(drift->amount_q15, 10, 42);
    radius = apsis_p8b_map_q15(drift->depth_q15, 16, 34);
    inner = apsis_p8b_map_q15(drift->width_q15, 5, 15);
    lx = (int16_t)(120 - separation);
    rx = (int16_t)(120 + separation);

    apsis_visual_line(raster, lx, 122, rx, 122, APSIS_VISUAL_DIM);
    apsis_visual_circle(raster, lx, 122, radius, APSIS_VISUAL_DRIFT);
    apsis_visual_circle(raster, rx, 122, radius, APSIS_VISUAL_DRIFT);
    apsis_visual_circle(raster, lx, 122, inner, APSIS_VISUAL_TUNING);
    apsis_visual_circle(raster, rx, 122, inner, APSIS_VISUAL_TUNING);
    apsis_visual_disc(raster, lx, 122, 3, APSIS_VISUAL_DRIFT);
    apsis_visual_disc(raster, rx, 122, 3, APSIS_VISUAL_DRIFT);
    apsis_visual_arc(raster, 120, 122,
                     apsis_p8b_map_q15(temper->strength_q15, 44, 61),
                     35, 23, APSIS_VISUAL_TUNING);

    /* Equal / Just / Pyth remain three spatial tuning stations. */
    for (i = 0u; i < 3u; ++i) {
        uint16_t c = temper->mode == i ? APSIS_VISUAL_TUNING : APSIS_VISUAL_DIM;
        apsis_visual_circle(raster, station_x[i], 194, temper->mode == i ? 8 : 5, c);
        apsis_visual_disc(raster, station_x[i], 194, 2, c);
    }
    apsis_visual_hline(raster, 72, 168, 194, APSIS_VISUAL_DIM);
}

static uint8_t apsis_p8b_clock_n(const ApsisClockState *clock)
{
    uint8_t n = clock ? clock->n : 0u;
    if (n < 1u) n = 1u;
    if (n > APSIS_CLOCK_STEPS_MAX) n = APSIS_CLOCK_STEPS_MAX;
    return n;
}

static uint8_t apsis_p8b_step_angle(uint8_t step, uint8_t n)
{
    return (uint8_t)(48u + ((uint32_t)step * 64u) / n);
}

void apsis_visual_render_clock(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context)
{
    const ApsisClockState *clock;
    uint8_t n, i, group;
    uint8_t play_step = 0u;
    uint8_t start_step = 0u;
    if (!raster || !context || !context->state) return;
    clock = &context->state->clock;
    n = apsis_p8b_clock_n(clock);
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8b_surface_mark(raster, APSIS_VISUAL_CLOCK);

    apsis_visual_circle(raster, 120, 124, 88, APSIS_VISUAL_DIM);
    apsis_visual_circle(raster, 120, 124, 62, APSIS_VISUAL_DIM);
    apsis_visual_circle(raster, 120, 124, 36, APSIS_VISUAL_DIM);

    for (i = 0u; i < n; ++i) {
        uint8_t angle = apsis_p8b_step_angle(i, n);
        int16_t x, y;
        apsis_visual_polar(120, 124, 88, angle, &x, &y);
        apsis_p8b_step_dot(raster, x, y, (uint8_t)((clock->main_mask >> i) & 1u),
                           APSIS_VISUAL_CLOCK);
        apsis_visual_polar(120, 124, 62, angle, &x, &y);
        apsis_p8b_step_dot(raster, x, y, (uint8_t)((clock->inner_mask >> i) & 1u),
                           APSIS_VISUAL_CLOCK);
        apsis_visual_polar(120, 124, 36, angle, &x, &y);
        apsis_p8b_step_dot(raster, x, y, (uint8_t)((clock->snap_mask >> i) & 1u),
                           APSIS_VISUAL_CLOCK);
    }

    /* Four group arcs: the three source bounds plus the closing group. */
    for (group = 0u; group < 4u; ++group) {
        uint8_t end_step = group < 3u ? clock->bounds[group] : n;
        if (end_step > n) end_step = n;
        if (end_step > start_step) {
            uint8_t start = apsis_p8b_step_angle(start_step, n);
            uint8_t end = apsis_p8b_step_angle(end_step, n);
            uint8_t count = (uint8_t)(end - start);
            if (group == 3u && end_step == n) count = (uint8_t)(64u - ((uint32_t)start_step * 64u) / n);
            if (count) apsis_visual_arc(raster, 120, 124, 101, start, count, APSIS_VISUAL_MUTED);
            start_step = end_step;
        }
    }

    if (context->runtime)
        play_step = (uint8_t)(context->runtime->transport.melody_step % n);
    {
        uint8_t angle = apsis_p8b_step_angle(play_step, n);
        int16_t x0, y0, x1, y1;
        apsis_visual_polar(120, 124, 25, angle, &x0, &y0);
        apsis_visual_polar(120, 124, 94, angle, &x1, &y1);
        apsis_visual_line(raster, x0, y0, x1, y1, APSIS_VISUAL_CLOCK);
        apsis_visual_disc(raster, x1, y1, 3, APSIS_VISUAL_FG);
    }
}

static void apsis_p8b_spiral_point(uint8_t index, int16_t *x, int16_t *y)
{
    int16_t radius;
    uint8_t angle;
    if (index >= APSIS_SPIRAL_POINTS) index = (uint8_t)(APSIS_SPIRAL_POINTS - 1u);
    radius = (int16_t)(18 + ((uint32_t)index * 70u) / (APSIS_SPIRAL_POINTS - 1u));
    angle = (uint8_t)(48u + ((uint32_t)index * 192u) / APSIS_SPIRAL_POINTS);
    apsis_visual_polar(120, 122, radius, angle, x, y);
}

void apsis_visual_render_spiral(ApsisVisualRaster *raster,
                                const ApsisVisualContext *context)
{
    const ApsisSpiralState *spiral;
    uint8_t i, count;
    int16_t px, py, x, y;
    if (!raster || !context || !context->state) return;
    spiral = &context->state->spiral;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8b_surface_mark(raster, APSIS_VISUAL_SPIRAL);

    apsis_p8b_spiral_point(0u, &px, &py);
    for (i = 1u; i < APSIS_SPIRAL_POINTS; ++i) {
        apsis_p8b_spiral_point(i, &x, &y);
        apsis_visual_line(raster, px, py, x, y, APSIS_VISUAL_DIM);
        px = x;
        py = y;
    }
    for (i = 0u; i < APSIS_SPIRAL_POINTS; ++i) {
        apsis_p8b_spiral_point(i, &x, &y);
        apsis_visual_disc(raster, x, y, 1, APSIS_VISUAL_MUTED);
    }

    count = spiral->route_count;
    if (count > APSIS_SPIRAL_ROUTE_MAX) count = APSIS_SPIRAL_ROUTE_MAX;
    if (count) {
        uint8_t point = spiral->route[0] < APSIS_SPIRAL_POINTS ? spiral->route[0] : 0u;
        apsis_p8b_spiral_point(point, &px, &py);
        apsis_visual_disc(raster, px, py, 3, APSIS_VISUAL_SPIRAL);
        for (i = 1u; i < count; ++i) {
            point = spiral->route[i] < APSIS_SPIRAL_POINTS ? spiral->route[i] : 0u;
            apsis_p8b_spiral_point(point, &x, &y);
            apsis_visual_line(raster, px, py, x, y, APSIS_VISUAL_SPIRAL);
            apsis_visual_disc(raster, x, y, 3, APSIS_VISUAL_SPIRAL);
            px = x;
            py = y;
        }
    }

    /* The 24-step attractor is a distinct outer orbit, never a step list. */
    apsis_visual_circle(raster, 120, 122, 104, APSIS_VISUAL_DIM);
    for (i = 0u; i < APSIS_ATTRACTOR_STEPS; ++i) {
        uint8_t angle = (uint8_t)(48u + ((uint32_t)i * 64u) / APSIS_ATTRACTOR_STEPS);
        uint8_t enabled = (uint8_t)((spiral->attractor_gates >> i) & 1u);
        apsis_visual_polar(120, 122, 104, angle, &x, &y);
        apsis_p8b_step_dot(raster, x, y, enabled, APSIS_VISUAL_TUNING);
        if (context->runtime && i == (uint8_t)(context->runtime->spiral_attractor_pos % APSIS_ATTRACTOR_STEPS))
            apsis_visual_circle(raster, x, y, 5, APSIS_VISUAL_FG);
    }
    if (context->runtime && context->runtime->last_spiral.valid &&
        context->runtime->last_spiral.route_point < APSIS_SPIRAL_POINTS) {
        apsis_p8b_spiral_point(context->runtime->last_spiral.route_point, &x, &y);
        apsis_visual_circle(raster, x, y, 6, APSIS_VISUAL_FG);
    }
}

static uint8_t apsis_p8b_bloom_count(const ApsisVisualContext *context)
{
    uint8_t count = 3u;
    if (context->runtime && context->runtime->last_bloom.valid) {
        count = context->runtime->last_bloom.plan.count;
    } else if (context->state) {
        switch (context->state->bloom.depth) {
        case APSIS_BLOOM_DEPTH_SEVENTH: count = 4u; break;
        case APSIS_BLOOM_DEPTH_NINTH: count = 5u; break;
        default: count = 3u; break;
        }
    }
    if (count < 1u) count = 1u;
    if (count > APSIS_BLOOM_VOICES_MAX) count = APSIS_BLOOM_VOICES_MAX;
    return count;
}

void apsis_visual_render_bloom(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context)
{
    const ApsisBloomState *bloom;
    uint8_t i, count;
    int16_t reach, petal_radius;
    if (!raster || !context || !context->state) return;
    bloom = &context->state->bloom;
    count = apsis_p8b_bloom_count(context);
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_p8b_surface_mark(raster, APSIS_VISUAL_BLOOM);

    reach = apsis_p8b_map_q15(bloom->width_q15, 36, 58);
    if (bloom->unfurl_ms > 3600u) reach = (int16_t)(reach + 14);
    else reach = (int16_t)(reach + ((uint32_t)bloom->unfurl_ms * 14u) / 3600u);
    petal_radius = apsis_p8b_map_q15(bloom->brightness_q15, 8, 15);

    for (i = 0u; i < count; ++i) {
        uint8_t angle;
        int16_t x, y;
        uint16_t c = APSIS_VISUAL_BLOOM;
        if (context->runtime && context->runtime->last_bloom.valid &&
            i < context->runtime->last_bloom.plan.count) {
            uint8_t tone = (uint8_t)(context->runtime->last_bloom.plan.tone[i] % 12u);
            angle = (uint8_t)(48u + ((uint32_t)tone * 64u) / 12u);
        } else {
            angle = (uint8_t)(48u + ((uint32_t)i * 64u) / count);
        }
        apsis_visual_polar(120, 116, reach, angle, &x, &y);
        apsis_visual_line(raster, 120, 116, x, y, APSIS_VISUAL_DIM);
        apsis_visual_circle(raster, x, y, petal_radius, c);
        apsis_visual_circle(raster, x, y, (int16_t)(petal_radius - 3), APSIS_VISUAL_MUTED);
        apsis_visual_disc(raster, x, y, 3, c);
        if (context->runtime && context->runtime->last_bloom_bind.valid &&
            i == (uint8_t)(context->runtime->last_bloom_bind.serial % count))
            apsis_visual_circle(raster, x, y, (int16_t)(petal_radius + 4), APSIS_VISUAL_FG);
    }

    apsis_visual_circle(raster, 120, 116, 13, APSIS_VISUAL_BLOOM);
    apsis_visual_disc(raster, 120, 116, 5, APSIS_VISUAL_FG);
    apsis_visual_line(raster, 120, 130, 120, 207, APSIS_VISUAL_BLOOM);
    apsis_visual_line(raster, 120, 176, 94, 188, APSIS_VISUAL_BLOOM);
    apsis_visual_line(raster, 94, 188, 113, 191, APSIS_VISUAL_DIM);
    apsis_visual_line(raster, 120, 191, 145, 178, APSIS_VISUAL_BLOOM);
    apsis_visual_line(raster, 145, 178, 127, 197, APSIS_VISUAL_DIM);
}

void apsis_visual_render_p8(ApsisVisualRaster *raster,
                            const ApsisVisualContext *context)
{
    if (!raster || !context) return;
    switch (context->surface) {
    case APSIS_VISUAL_SURFACE_STRING:
        apsis_visual_render_string(raster, context);
        break;
    case APSIS_VISUAL_SURFACE_DRIFT:
        apsis_visual_render_drift(raster, context);
        break;
    case APSIS_VISUAL_SURFACE_CLOCK:
        apsis_visual_render_clock(raster, context);
        break;
    case APSIS_VISUAL_SURFACE_SPIRAL:
        apsis_visual_render_spiral(raster, context);
        break;
    case APSIS_VISUAL_SURFACE_BLOOM:
        apsis_visual_render_bloom(raster, context);
        break;
    default:
        apsis_visual_render(raster, context);
        break;
    }
}
