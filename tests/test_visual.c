#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_visual.h"
#include "apsis_visual_p8b.h"

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

    state->string.surface_mode = APSIS_STRING_DUAL;
    state->string.left_split_q15 = 6200u;
    state->string.right_split_q15 = 25800u;
    state->string.split_q15 = 17000u;
    state->string.active_side = APSIS_SIDE_RIGHT;
    state->string.linked = 1u;
    state->string.rhythm_mask = 0x0000A55Au;
    state->string.filter_attractor_gates = 0x00005AA5u;
    state->string.motion_loop_play = 1u;
    state->string.motion_loop_length = 16u;

    state->drift.amount_q15 = 19000u;
    state->drift.depth_q15 = 22500u;
    state->drift.width_q15 = 14500u;
    state->temper.mode = APSIS_TUNING_JUST;
    state->temper.strength_q15 = 24000u;

    state->clock.n = 12u;
    state->clock.bounds[0] = 3u;
    state->clock.bounds[1] = 7u;
    state->clock.bounds[2] = 10u;
    state->clock.main_mask = 0x00000AAAu;
    state->clock.inner_mask = 0x00000492u;
    state->clock.snap_mask = 0x00000111u;
    runtime->transport.melody_step = 7u;

    state->spiral.route_count = 7u;
    state->spiral.route[0] = 1u;
    state->spiral.route[1] = 5u;
    state->spiral.route[2] = 9u;
    state->spiral.route[3] = 14u;
    state->spiral.route[4] = 21u;
    state->spiral.route[5] = 28u;
    state->spiral.route[6] = 34u;
    state->spiral.attractor_length = APSIS_ATTRACTOR_STEPS;
    state->spiral.attractor_gates = 0x00A55A5Au;
    runtime->spiral_attractor_pos = 9u;
    runtime->last_spiral.valid = 1u;
    runtime->last_spiral.route_point = 21u;

    state->bloom.depth = APSIS_BLOOM_DEPTH_SEVENTH;
    state->bloom.width_q15 = 21000u;
    state->bloom.brightness_q15 = 24000u;
    state->bloom.unfurl_ms = 2800u;
    runtime->last_bloom.valid = 1u;
    runtime->last_bloom.plan.count = 4u;
    runtime->last_bloom.plan.tone[0] = 0u;
    runtime->last_bloom.plan.tone[1] = 4u;
    runtime->last_bloom.plan.tone[2] = 7u;
    runtime->last_bloom.plan.tone[3] = 11u;
    runtime->last_bloom_bind.valid = 1u;
    runtime->last_bloom_bind.serial = 5u;
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
    apsis_visual_render_p8(&raster, &context);
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
    apsis_visual_render_p8(&raster, &context);
    apsis_visual_bind(&raster, strip_frame + 120u * 240u,
                      240u, 120u, 240u, 0, 120);
    apsis_visual_render_p8(&raster, &context);
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

static uint32_t assert_strip_surface(ApsisVisualSurface surface,
                                     uint16_t identity_colour,
                                     const ApsisState *state,
                                     const ApsisRuntime *runtime)
{
    uint32_t full = render_full(surface, state, runtime);
    uint32_t strips = render_two_strips(surface, state, runtime);
    assert(full == strips);
    assert(memcmp(full_frame, strip_frame, sizeof full_frame) == 0);
    assert(count_colour(full_frame, identity_colour) > 0u);
    return full;
}

static void test_strip_equivalence(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    uint32_t mother, gravity, string, drift, clock, spiral, bloom;
    fixture(&state, &runtime);

    mother = assert_strip_surface(APSIS_VISUAL_SURFACE_MOTHER,
                                  APSIS_VISUAL_MOTHER, &state, &runtime);
    assert(mother == P8A_MOTHER_REFERENCE);
    gravity = assert_strip_surface(APSIS_VISUAL_SURFACE_GRAVITY,
                                   APSIS_VISUAL_GRAVITY, &state, &runtime);
    assert(gravity == P8A_GRAVITY_REFERENCE);
    assert(count_colour(full_frame, APSIS_VISUAL_CLOCK) > 0u);

    string = assert_strip_surface(APSIS_VISUAL_SURFACE_STRING,
                                  APSIS_VISUAL_STRING, &state, &runtime);
    assert(count_colour(full_frame, APSIS_VISUAL_TUNING) > 0u);
    drift = assert_strip_surface(APSIS_VISUAL_SURFACE_DRIFT,
                                 APSIS_VISUAL_DRIFT, &state, &runtime);
    assert(count_colour(full_frame, APSIS_VISUAL_TUNING) > 0u);
    clock = assert_strip_surface(APSIS_VISUAL_SURFACE_CLOCK,
                                 APSIS_VISUAL_CLOCK, &state, &runtime);
    spiral = assert_strip_surface(APSIS_VISUAL_SURFACE_SPIRAL,
                                  APSIS_VISUAL_SPIRAL, &state, &runtime);
    assert(count_colour(full_frame, APSIS_VISUAL_TUNING) > 0u);
    bloom = assert_strip_surface(APSIS_VISUAL_SURFACE_BLOOM,
                                 APSIS_VISUAL_BLOOM, &state, &runtime);

    assert(mother != gravity);
    assert(string != drift && drift != clock && clock != spiral && spiral != bloom);
    printf("P8 refs mother=%08x gravity=%08x string=%08x drift=%08x clock=%08x spiral=%08x bloom=%08x\n",
           mother, gravity, string, drift, clock, spiral, bloom);
}

static void test_state_is_live_not_shadowed(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    uint32_t a, b;
    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_MOTHER, &state, &runtime);
    state.mother.mix_q15[0] = 0u;
    b = render_full(APSIS_VISUAL_SURFACE_MOTHER, &state, &runtime);
    assert(a != b);

    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_GRAVITY, &state, &runtime);
    runtime.gravity_pending.degree = 1u;
    b = render_full(APSIS_VISUAL_SURFACE_GRAVITY, &state, &runtime);
    assert(a != b);

    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_STRING, &state, &runtime);
    state.string.right_split_q15 = 12000u;
    b = render_full(APSIS_VISUAL_SURFACE_STRING, &state, &runtime);
    assert(a != b);

    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_CLOCK, &state, &runtime);
    runtime.transport.melody_step = 2u;
    b = render_full(APSIS_VISUAL_SURFACE_CLOCK, &state, &runtime);
    assert(a != b);

    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_SPIRAL, &state, &runtime);
    runtime.spiral_attractor_pos = 17u;
    b = render_full(APSIS_VISUAL_SURFACE_SPIRAL, &state, &runtime);
    assert(a != b);

    fixture(&state, &runtime);
    a = render_full(APSIS_VISUAL_SURFACE_BLOOM, &state, &runtime);
    runtime.last_bloom.plan.tone[1] = 5u;
    b = render_full(APSIS_VISUAL_SURFACE_BLOOM, &state, &runtime);
    assert(a != b);
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
    puts("APSIS Phase 8A/P8B visual tests passed");
    return 0;
}
