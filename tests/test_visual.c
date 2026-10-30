#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_visual.h"

#define PIXELS (APSIS_VISUAL_WIDTH * APSIS_VISUAL_HEIGHT)
#define P8A_MOTHER_REFERENCE 0x27373657u
#define P8A_GRAVITY_REFERENCE 0x55AFF30Bu

static uint16_t full_frame[PIXELS];
static uint16_t strip_frame[PIXELS];

static uint32_t fnv1a_pixels(const uint16_t *pixels, uint32_t count)
{
    uint32_t h = 2166136261u;
    uint32_t i;
    for (i = 0u; i < count; ++i) {
        h ^= (uint8_t)pixels[i];
        h *= 16777619u;
        h ^= (uint8_t)(pixels[i] >> 8);
        h *= 16777619u;
    }
    return h;
}

static uint32_t count_colour(const uint16_t *pixels, uint16_t colour)
{
    uint32_t i, n = 0u;
    for (i = 0u; i < PIXELS; ++i)
        if (pixels[i] == colour) ++n;
    return n;
}

static void fixture(ApsisState *state, ApsisRuntime *runtime)
{
    uint8_t i;
    memset(state, 0, sizeof(*state));
    memset(runtime, 0, sizeof(*runtime));
    state->mother.sync = 1u;
    state->mother.mix_q15[0] = APSIS_Q15_ONE;
    state->mother.mix_q15[1] = 24575u;
    state->mother.mix_q15[2] = 16384u;
    state->mother.mix_q15[3] = 8192u;
    state->mother.mix_q15[4] = 4096u;
    state->mother.mute_mask = (uint8_t)(1u << 3);

    state->gravity.degree = 2u;
    state->gravity.voicing = 1u;
    state->gravity.selected_ring = 1u;
    state->gravity.selected_point_degree = 5u;
    for (i = 0u; i < APSIS_GRAVITY_POINTS; ++i)
        state->gravity.points[i].enabled = (uint8_t)((i % 5u) != 0u);
    runtime->gravity_pending.active = 1u;
    runtime->gravity_pending.degree = 6u;
    runtime->gravity_pending.ring = 0u;
}

static uint32_t render_full(ApsisVisualSurface surface,
                            const ApsisState *state, const ApsisRuntime *runtime)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    apsis_visual_bind(&raster, full_frame, APSIS_VISUAL_WIDTH,
                      APSIS_VISUAL_HEIGHT, APSIS_VISUAL_WIDTH, 0, 0);
    context.surface = surface;
    context.state = state;
    context.runtime = runtime;
    context.animation_tick = 0u;
    apsis_visual_render(&raster, &context);
    return fnv1a_pixels(full_frame, PIXELS);
}

static uint32_t render_two_strips(ApsisVisualSurface surface,
                                  const ApsisState *state, const ApsisRuntime *runtime)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    context.surface = surface;
    context.state = state;
    context.runtime = runtime;
    context.animation_tick = 0u;
    memset(strip_frame, 0xA5, sizeof strip_frame);

    apsis_visual_bind(&raster, strip_frame, 240u, 120u, 240u, 0, 0);
    apsis_visual_render(&raster, &context);
    apsis_visual_bind(&raster, strip_frame + 120u * 240u,
                      240u, 120u, 240u, 0, 120);
    apsis_visual_render(&raster, &context);
    return fnv1a_pixels(strip_frame, PIXELS);
}

static void test_palette(void)
{
    assert(APSIS_VISUAL_BG == 0x0841u);
    assert(APSIS_VISUAL_STRING == 0xF5EAu);
    assert(APSIS_VISUAL_DRIFT == 0xFB6Cu);
    assert(APSIS_VISUAL_CLOCK == 0xCF6Du);
    assert(APSIS_VISUAL_SPIRAL == 0xCBDFu);
    assert(APSIS_VISUAL_BLOOM == 0xF571u);
    assert(APSIS_VISUAL_TUNING == 0xFBD6u);
    assert(APSIS_VISUAL_GRAVITY == 0xEF19u);
    assert(APSIS_VISUAL_CHAIN == 0x8EBFu);
    assert(APSIS_VISUAL_MOTHER == 0xD655u);
}

static void test_strip_equivalence(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    uint32_t mother_full, mother_strips, gravity_full, gravity_strips;
    fixture(&state, &runtime);

    mother_full = render_full(APSIS_VISUAL_SURFACE_MOTHER, &state, &runtime);
    mother_strips = render_two_strips(APSIS_VISUAL_SURFACE_MOTHER, &state, &runtime);
    assert(mother_full == mother_strips);
    assert(mother_full == P8A_MOTHER_REFERENCE);
    assert(memcmp(full_frame, strip_frame, sizeof full_frame) == 0);
    assert(count_colour(full_frame, APSIS_VISUAL_STRING) > 0u);
    assert(count_colour(full_frame, APSIS_VISUAL_MOTHER) > 0u);

    gravity_full = render_full(APSIS_VISUAL_SURFACE_GRAVITY, &state, &runtime);
    gravity_strips = render_two_strips(APSIS_VISUAL_SURFACE_GRAVITY, &state, &runtime);
    assert(gravity_full == gravity_strips);
    assert(gravity_full == P8A_GRAVITY_REFERENCE);
    assert(memcmp(full_frame, strip_frame, sizeof full_frame) == 0);
    assert(count_colour(full_frame, APSIS_VISUAL_GRAVITY) > 0u);
    assert(count_colour(full_frame, APSIS_VISUAL_CLOCK) > 0u);
    assert(mother_full != gravity_full);

    printf("P8A mother=%08x gravity=%08x\n", mother_full, gravity_full);
}

static void test_state_is_live_not_shadowed(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    uint32_t a, b, c;
    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_MOTHER, &state, &runtime);
    state.mother.mix_q15[0] = 0u;
    b = render_full(APSIS_VISUAL_SURFACE_MOTHER, &state, &runtime);
    assert(a != b);

    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_GRAVITY, &state, &runtime);
    runtime.gravity_pending.degree = 1u;
    c = render_full(APSIS_VISUAL_SURFACE_GRAVITY, &state, &runtime);
    assert(a != c);
}

static void test_clip_guard(void)
{
    struct {
        uint16_t before[8];
        uint16_t tile[32u * 16u];
        uint16_t after[8];
    } mem;
    ApsisVisualRaster raster;
    uint32_t i;
    for (i = 0u; i < 8u; ++i) mem.before[i] = mem.after[i] = 0x5AA5u;
    memset(mem.tile, 0, sizeof mem.tile);
    apsis_visual_bind(&raster, mem.tile, 32u, 16u, 32u, 80, 100);
    apsis_visual_disc(&raster, 80, 100, 40, APSIS_VISUAL_STRING);
    apsis_visual_line(&raster, -300, -300, 400, 400, APSIS_VISUAL_CLOCK);
    for (i = 0u; i < 8u; ++i) {
        assert(mem.before[i] == 0x5AA5u);
        assert(mem.after[i] == 0x5AA5u);
    }
}

int main(void)
{
    test_palette();
    test_strip_equivalence();
    test_state_is_live_not_shadowed();
    test_clip_guard();
    puts("APSIS Phase 8A visual primitive tests passed");
    return 0;
}
