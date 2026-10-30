#include "apsis_visual.h"

static const int16_t apsis_visual_cos_q10[APSIS_VISUAL_ANGLE_STEPS] = {
    1024,1019,1004,980,946,903,851,792,724,650,569,483,392,297,200,100,
    0,-100,-200,-297,-392,-483,-569,-650,-724,-792,-851,-903,-946,-980,-1004,-1019,
    -1024,-1019,-1004,-980,-946,-903,-851,-792,-724,-650,-569,-483,-392,-297,-200,-100,
    0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019
};

static const int16_t apsis_visual_sin_q10[APSIS_VISUAL_ANGLE_STEPS] = {
    0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,
    1024,1019,1004,980,946,903,851,792,724,650,569,483,392,297,200,100,
    0,-100,-200,-297,-392,-483,-569,-650,-724,-792,-851,-903,-946,-980,-1004,-1019,
    -1024,-1019,-1004,-980,-946,-903,-851,-792,-724,-650,-569,-483,-392,-297,-200,-100
};

static int32_t apsis_visual_abs32(int32_t x)
{
    return x < 0 ? -x : x;
}

static uint16_t apsis_visual_source_colour(uint8_t source)
{
    static const uint16_t colours[5] = {
        APSIS_VISUAL_STRING, APSIS_VISUAL_DRIFT, APSIS_VISUAL_CLOCK,
        APSIS_VISUAL_SPIRAL, APSIS_VISUAL_BLOOM
    };
    return source < 5u ? colours[source] : APSIS_VISUAL_FG;
}

void apsis_visual_bind(ApsisVisualRaster *raster, uint16_t *pixels,
                       uint16_t width, uint16_t height, uint16_t stride,
                       int16_t x0, int16_t y0)
{
    if (!raster) return;
    raster->pixels = pixels;
    raster->width = width;
    raster->height = height;
    raster->stride = stride;
    raster->x0 = x0;
    raster->y0 = y0;
}

void apsis_visual_clear(ApsisVisualRaster *raster, uint16_t colour)
{
    uint16_t y, x;
    if (!raster || !raster->pixels || raster->stride < raster->width) return;
    for (y = 0u; y < raster->height; ++y)
        for (x = 0u; x < raster->width; ++x)
            raster->pixels[(uint32_t)y * raster->stride + x] = colour;
}

void apsis_visual_pixel(ApsisVisualRaster *raster, int16_t x, int16_t y,
                        uint16_t colour)
{
    int32_t lx, ly;
    if (!raster || !raster->pixels) return;
    lx = (int32_t)x - raster->x0;
    ly = (int32_t)y - raster->y0;
    if (lx < 0 || ly < 0 || lx >= raster->width || ly >= raster->height) return;
    raster->pixels[(uint32_t)ly * raster->stride + (uint32_t)lx] = colour;
}

void apsis_visual_hline(ApsisVisualRaster *raster, int16_t x0, int16_t x1,
                        int16_t y, uint16_t colour)
{
    int32_t x;
    int16_t t;
    if (x1 < x0) { t = x0; x0 = x1; x1 = t; }
    for (x = x0; x <= x1; ++x)
        apsis_visual_pixel(raster, (int16_t)x, y, colour);
}

void apsis_visual_line(ApsisVisualRaster *raster, int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1, uint16_t colour)
{
    int32_t x = x0, y = y0;
    const int32_t dx = apsis_visual_abs32((int32_t)x1 - x0);
    const int32_t sx = x0 < x1 ? 1 : -1;
    const int32_t dy = -apsis_visual_abs32((int32_t)y1 - y0);
    const int32_t sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    for (;;) {
        apsis_visual_pixel(raster, (int16_t)x, (int16_t)y, colour);
        if (x == x1 && y == y1) break;
        {
            int32_t e2 = err << 1;
            if (e2 >= dy) { err += dy; x += sx; }
            if (e2 <= dx) { err += dx; y += sy; }
        }
    }
}

void apsis_visual_fill_rect(ApsisVisualRaster *raster, int16_t x, int16_t y,
                            int16_t width, int16_t height, uint16_t colour)
{
    int32_t yy;
    if (width <= 0 || height <= 0) return;
    for (yy = y; yy < (int32_t)y + height; ++yy)
        apsis_visual_hline(raster, x, (int16_t)(x + width - 1), (int16_t)yy, colour);
}

void apsis_visual_rect(ApsisVisualRaster *raster, int16_t x, int16_t y,
                       int16_t width, int16_t height, uint16_t colour)
{
    if (width <= 0 || height <= 0) return;
    apsis_visual_hline(raster, x, (int16_t)(x + width - 1), y, colour);
    apsis_visual_hline(raster, x, (int16_t)(x + width - 1),
                       (int16_t)(y + height - 1), colour);
    apsis_visual_line(raster, x, y, x, (int16_t)(y + height - 1), colour);
    apsis_visual_line(raster, (int16_t)(x + width - 1), y,
                      (int16_t)(x + width - 1), (int16_t)(y + height - 1), colour);
}

void apsis_visual_circle(ApsisVisualRaster *raster, int16_t cx, int16_t cy,
                         int16_t radius, uint16_t colour)
{
    int32_t x, y, err;
    if (radius < 0) return;
    x = radius;
    y = 0;
    err = 1 - x;
    while (x >= y) {
        apsis_visual_pixel(raster, (int16_t)(cx + x), (int16_t)(cy + y), colour);
        apsis_visual_pixel(raster, (int16_t)(cx + y), (int16_t)(cy + x), colour);
        apsis_visual_pixel(raster, (int16_t)(cx - y), (int16_t)(cy + x), colour);
        apsis_visual_pixel(raster, (int16_t)(cx - x), (int16_t)(cy + y), colour);
        apsis_visual_pixel(raster, (int16_t)(cx - x), (int16_t)(cy - y), colour);
        apsis_visual_pixel(raster, (int16_t)(cx - y), (int16_t)(cy - x), colour);
        apsis_visual_pixel(raster, (int16_t)(cx + y), (int16_t)(cy - x), colour);
        apsis_visual_pixel(raster, (int16_t)(cx + x), (int16_t)(cy - y), colour);
        ++y;
        if (err < 0) err += 2 * y + 1;
        else { --x; err += 2 * (y - x) + 1; }
    }
}

void apsis_visual_disc(ApsisVisualRaster *raster, int16_t cx, int16_t cy,
                       int16_t radius, uint16_t colour)
{
    int32_t x, y, err;
    if (radius < 0) return;
    x = radius;
    y = 0;
    err = 1 - x;
    while (x >= y) {
        apsis_visual_hline(raster, (int16_t)(cx - x), (int16_t)(cx + x),
                           (int16_t)(cy + y), colour);
        apsis_visual_hline(raster, (int16_t)(cx - x), (int16_t)(cx + x),
                           (int16_t)(cy - y), colour);
        apsis_visual_hline(raster, (int16_t)(cx - y), (int16_t)(cx + y),
                           (int16_t)(cy + x), colour);
        apsis_visual_hline(raster, (int16_t)(cx - y), (int16_t)(cx + y),
                           (int16_t)(cy - x), colour);
        ++y;
        if (err < 0) err += 2 * y + 1;
        else { --x; err += 2 * (y - x) + 1; }
    }
}

void apsis_visual_polar(int16_t cx, int16_t cy, int16_t radius,
                        uint8_t angle64, int16_t *x, int16_t *y)
{
    uint8_t a = (uint8_t)(angle64 & 63u);
    if (x) *x = (int16_t)(cx + ((int32_t)radius * apsis_visual_cos_q10[a]) / 1024);
    if (y) *y = (int16_t)(cy + ((int32_t)radius * apsis_visual_sin_q10[a]) / 1024);
}

void apsis_visual_arc(ApsisVisualRaster *raster, int16_t cx, int16_t cy,
                      int16_t radius, uint8_t start64, uint8_t count64,
                      uint16_t colour)
{
    uint8_t i;
    int16_t x0, y0, x1, y1;
    if (count64 == 0u) return;
    if (count64 > 64u) count64 = 64u;
    apsis_visual_polar(cx, cy, radius, start64, &x0, &y0);
    for (i = 1u; i <= count64; ++i) {
        apsis_visual_polar(cx, cy, radius, (uint8_t)(start64 + i), &x1, &y1);
        apsis_visual_line(raster, x0, y0, x1, y1, colour);
        x0 = x1;
        y0 = y1;
    }
}

static void apsis_visual_surface_mark(ApsisVisualRaster *raster, uint16_t colour)
{
    apsis_visual_hline(raster, 106, 134, 8, APSIS_VISUAL_DIM);
    apsis_visual_hline(raster, 114, 126, 8, colour);
}

void apsis_visual_render_mother(ApsisVisualRaster *raster,
                                const ApsisVisualContext *context)
{
    static const uint8_t centers[5] = { 48u, 61u, 10u, 23u, 36u };
    const ApsisMotherState *mother;
    uint8_t i;
    if (!raster || !context || !context->state) return;
    mother = &context->state->mother;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_visual_surface_mark(raster, APSIS_VISUAL_MOTHER);

    for (i = 0u; i < 5u; ++i) {
        const uint8_t span = 9u;
        uint16_t mix = mother->mix_q15[i];
        uint8_t active;
        uint8_t start = (uint8_t)(centers[i] - span / 2u);
        uint16_t colour = apsis_visual_source_colour(i);
        int16_t nx, ny;
        if (mix > APSIS_Q15_ONE) mix = APSIS_Q15_ONE;
        active = (uint8_t)(((uint32_t)mix * span + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
        apsis_visual_arc(raster, 120, 122, 88, start, span, APSIS_VISUAL_DIM);
        if (active) {
            uint16_t active_colour = (mother->mute_mask & (1u << i)) ? APSIS_VISUAL_DIM : colour;
            apsis_visual_arc(raster, 120, 122, 88, start, active, active_colour);
            apsis_visual_polar(120, 122, 88, (uint8_t)(start + active), &nx, &ny);
            apsis_visual_disc(raster, nx, ny, 2, active_colour);
        }
    }

    apsis_visual_circle(raster, 120, 122, 31, APSIS_VISUAL_DIM);
    apsis_visual_circle(raster, 120, 122, 24, APSIS_VISUAL_MOTHER);
    apsis_visual_disc(raster, 120, 122, 6, APSIS_VISUAL_MOTHER);
    if (mother->sync)
        apsis_visual_circle(raster, 120, 122, 36, APSIS_VISUAL_GRAVITY);

    /* Five source witnesses make the topology readable even at minimum mix. */
    for (i = 0u; i < 5u; ++i) {
        int16_t x, y;
        apsis_visual_polar(120, 122, 48, centers[i], &x, &y);
        apsis_visual_disc(raster, x, y, 2,
                          (mother->mute_mask & (1u << i)) ? APSIS_VISUAL_DIM : apsis_visual_source_colour(i));
    }
}

void apsis_visual_render_gravity(ApsisVisualRaster *raster,
                                 const ApsisVisualContext *context)
{
    static const int16_t radii[APSIS_GRAVITY_RINGS] = { 44, 66, 88 };
    const ApsisGravityState *gravity;
    uint8_t ring, degree;
    if (!raster || !context || !context->state) return;
    gravity = &context->state->gravity;
    apsis_visual_clear(raster, APSIS_VISUAL_BG);
    apsis_visual_surface_mark(raster, APSIS_VISUAL_GRAVITY);

    for (ring = 0u; ring < APSIS_GRAVITY_RINGS; ++ring) {
        apsis_visual_circle(raster, 120, 124, radii[ring], APSIS_VISUAL_DIM);
        for (degree = 0u; degree < APSIS_GRAVITY_DEGREES; ++degree) {
            const uint8_t point_index = (uint8_t)(ring * APSIS_GRAVITY_DEGREES + degree);
            const ApsisGravityPoint *point = &gravity->points[point_index];
            const uint8_t angle = (uint8_t)(48u + degree * 8u);
            int16_t x, y;
            uint16_t colour = point->enabled ? APSIS_VISUAL_MUTED : APSIS_VISUAL_DIM;
            apsis_visual_polar(120, 124, radii[ring], angle, &x, &y);
            apsis_visual_disc(raster, x, y, point->enabled ? 2 : 1, colour);

            if (ring == gravity->selected_ring && degree == gravity->selected_point_degree)
                apsis_visual_circle(raster, x, y, 5, APSIS_VISUAL_GRAVITY);
            if (degree == gravity->degree && ring == gravity->voicing)
                apsis_visual_disc(raster, x, y, 3, APSIS_VISUAL_GRAVITY);
        }
    }

    if (context->runtime && context->runtime->gravity_pending.active) {
        const uint8_t ring_index = context->runtime->gravity_pending.ring < APSIS_GRAVITY_RINGS ?
                                   context->runtime->gravity_pending.ring : 0u;
        const uint8_t degree_index = (uint8_t)(context->runtime->gravity_pending.degree & 7u);
        int16_t x, y;
        apsis_visual_polar(120, 124, radii[ring_index],
                           (uint8_t)(48u + degree_index * 8u), &x, &y);
        apsis_visual_circle(raster, x, y, 8, APSIS_VISUAL_CLOCK);
    }

    apsis_visual_disc(raster, 120, 124, 3, APSIS_VISUAL_GRAVITY);
}

void apsis_visual_render(ApsisVisualRaster *raster,
                         const ApsisVisualContext *context)
{
    if (!raster || !context) return;
    switch (context->surface) {
    case APSIS_VISUAL_SURFACE_MOTHER:
        apsis_visual_render_mother(raster, context);
        break;
    case APSIS_VISUAL_SURFACE_GRAVITY:
        apsis_visual_render_gravity(raster, context);
        break;
    default:
        apsis_visual_clear(raster, APSIS_VISUAL_BG);
        break;
    }
}
