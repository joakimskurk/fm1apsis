#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_visual_p8c.h"

#define PIXELS (APSIS_VISUAL_WIDTH * APSIS_VISUAL_HEIGHT)

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

static void fixture(ApsisState *state, ApsisRuntime *runtime,
                    ApsisKeepBank *bank, ApsisChainSequence *seq)
{
    uint8_t i;
    memset(state, 0, sizeof(*state));
    memset(runtime, 0, sizeof(*runtime));
    memset(bank, 0, sizeof(*bank));
    memset(seq, 0, sizeof(*seq));

    bank->selected_slot = 2;
    for (i = 0u; i < APSIS_KEEP_SLOTS; ++i) {
        bank->slots[i].valid = (uint8_t)(i != 5u);
        bank->slots[i].gravity_degree = i;
        bank->slots[i].drift_amount_q15 = (uint16_t)(4000u + i * 3000u);
    }

    seq->count = 12u;
    seq->selected = 7;
    seq->playing = 1u;
    seq->armed = 1u;
    seq->pos = 4u;
    seq->phrase_pulse = 37u;
    seq->master_pulses = 96u;
    for (i = 0u; i < seq->count; ++i) {
        seq->cells[i].slot = (uint8_t)((i * 3u) % APSIS_KEEP_SLOTS);
        seq->cells[i].cycles = (uint8_t)(1u + (i % 4u));
        seq->cells[i].chance_q15 = (uint16_t)((i & 1u) ? 12000u : APSIS_Q15_ONE);
        seq->cells[i].transpose = (int8_t)((i % 3u) - 1);
        seq->cells[i].morph_quarters = (uint8_t)(i % 5u == 0u ? APSIS_CHAIN_MORPH_HALF : APSIS_CHAIN_MORPH_NONE);
    }

    state->space.mix_q15 = 15728u;
    state->space.decay_ms = 8400u;
    state->space.predelay_ms = 72u;
    state->space.damping_hz = 6200u;
    state->space.echo_time_ms = 362u;
    state->space.echo_feedback_q15 = 5898u;
    state->string.send_q15 = 23592u;
    state->drift.send_q15 = 6553u;
    state->clock.send_q15 = 1966u;
    state->spiral.send_q15 = 20316u;
    state->bloom.send_q15 = 19005u;
}

static uint32_t render_full(ApsisVisualSurface surface, const ApsisState *state,
                            const ApsisRuntime *runtime, const ApsisKeepBank *bank,
                            const ApsisChainSequence *seq)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    apsis_visual_bind(&raster, full_frame, APSIS_VISUAL_WIDTH,
                      APSIS_VISUAL_HEIGHT, APSIS_VISUAL_WIDTH, 0, 0);
    context.surface = surface;
    context.state = state;
    context.runtime = runtime;
    context.animation_tick = 0u;
    apsis_visual_render_p8c(&raster, &context, bank, seq);
    return fnv1a_pixels(full_frame, PIXELS);
}

static uint32_t render_strips(ApsisVisualSurface surface, const ApsisState *state,
                              const ApsisRuntime *runtime, const ApsisKeepBank *bank,
                              const ApsisChainSequence *seq)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    context.surface = surface;
    context.state = state;
    context.runtime = runtime;
    context.animation_tick = 0u;
    memset(strip_frame, 0xA5, sizeof strip_frame);
    apsis_visual_bind(&raster, strip_frame, 240u, 120u, 240u, 0, 0);
    apsis_visual_render_p8c(&raster, &context, bank, seq);
    apsis_visual_bind(&raster, strip_frame + 120u * 240u,
                      240u, 120u, 240u, 0, 120);
    apsis_visual_render_p8c(&raster, &context, bank, seq);
    return fnv1a_pixels(strip_frame, PIXELS);
}

static uint32_t assert_surface(ApsisVisualSurface surface, uint16_t colour,
                               const ApsisState *state, const ApsisRuntime *runtime,
                               const ApsisKeepBank *bank, const ApsisChainSequence *seq)
{
    uint32_t a = render_full(surface, state, runtime, bank, seq);
    uint32_t b = render_strips(surface, state, runtime, bank, seq);
    assert(a == b);
    assert(memcmp(full_frame, strip_frame, sizeof full_frame) == 0);
    assert(count_colour(full_frame, colour) > 0u);
    return a;
}

static void test_project_surfaces(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    ApsisKeepBank bank;
    ApsisChainSequence seq;
    uint32_t keep, chain, space;
    fixture(&state, &runtime, &bank, &seq);

    keep = assert_surface(APSIS_VISUAL_SURFACE_KEEP, APSIS_VISUAL_MOTHER,
                          &state, &runtime, &bank, &seq);
    assert(count_colour(full_frame, APSIS_VISUAL_CHAIN) > 0u);
    chain = assert_surface(APSIS_VISUAL_SURFACE_CHAIN, APSIS_VISUAL_CHAIN,
                           &state, &runtime, &bank, &seq);
    assert(count_colour(full_frame, APSIS_VISUAL_CLOCK) > 0u);
    space = assert_surface(APSIS_VISUAL_SURFACE_SPACE, APSIS_VISUAL_MOTHER,
                           &state, &runtime, &bank, &seq);
    assert(count_colour(full_frame, APSIS_VISUAL_STRING) > 0u);
    assert(count_colour(full_frame, APSIS_VISUAL_DRIFT) > 0u);
    assert(count_colour(full_frame, APSIS_VISUAL_CLOCK) > 0u);
    assert(count_colour(full_frame, APSIS_VISUAL_SPIRAL) > 0u);
    assert(count_colour(full_frame, APSIS_VISUAL_BLOOM) > 0u);
    assert(keep != chain && chain != space && keep != space);
    printf("P8C refs keep=%08x chain=%08x space=%08x\n", keep, chain, space);
}

static void test_live_project_state(void)
{
    ApsisState state;
    ApsisRuntime runtime;
    ApsisKeepBank bank;
    ApsisChainSequence seq;
    uint32_t a, b;
    fixture(&state, &runtime, &bank, &seq);

    a = render_full(APSIS_VISUAL_SURFACE_KEEP, &state, &runtime, &bank, &seq);
    bank.selected_slot = 6;
    b = render_full(APSIS_VISUAL_SURFACE_KEEP, &state, &runtime, &bank, &seq);
    assert(a != b);

    fixture(&state, &runtime, &bank, &seq);
    a = render_full(APSIS_VISUAL_SURFACE_CHAIN, &state, &runtime, &bank, &seq);
    seq.pos = 8u;
    seq.phrase_pulse = 73u;
    b = render_full(APSIS_VISUAL_SURFACE_CHAIN, &state, &runtime, &bank, &seq);
    assert(a != b);

    fixture(&state, &runtime, &bank, &seq);
    a = render_full(APSIS_VISUAL_SURFACE_SPACE, &state, &runtime, &bank, &seq);
    state.string.send_q15 = 1000u;
    b = render_full(APSIS_VISUAL_SURFACE_SPACE, &state, &runtime, &bank, &seq);
    assert(a != b);
}

int main(void)
{
    test_project_surfaces();
    test_live_project_state();
    puts("APSIS Phase 8C visual tests passed");
    return 0;
}
