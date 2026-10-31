#include "apsis_controls.h"

static uint32_t apsis_button_bit(uint8_t button)
{
    return 1u << button;
}

static uint8_t apsis_controls_clock_n(const ApsisClockState *clock)
{
    uint8_t n;
    if (!clock) return 3u;
    n = clock->n;
    if (n < 3u) return 3u;
    if (n > APSIS_CLOCK_STEPS_MAX) return APSIS_CLOCK_STEPS_MAX;
    return n;
}

static void apsis_controls_bounds_normalize(ApsisClockState *clock, uint8_t n)
{
    uint8_t v[3] = {0u, 0u, 0u};
    uint8_t count = 0u, i, j;
    if (!clock) return;
    for (i = 0u; i < 3u; ++i) {
        uint8_t b = clock->bounds[i];
        uint8_t duplicate = 0u;
        if (b == 0u || b >= n) continue;
        for (j = 0u; j < count; ++j)
            if (v[j] == b) duplicate = 1u;
        if (!duplicate && count < 3u) v[count++] = b;
    }
    for (i = 1u; i < count; ++i) {
        uint8_t x = v[i];
        j = i;
        while (j > 0u && v[j - 1u] > x) {
            v[j] = v[j - 1u];
            --j;
        }
        v[j] = x;
    }
    for (i = 0u; i < 3u; ++i) clock->bounds[i] = i < count ? v[i] : 0u;
}

static uint8_t apsis_controls_toggle_boundary(ApsisClockState *clock,
                                              uint8_t n, uint8_t step)
{
    uint8_t i;
    if (!clock || step == 0u || step >= n) return 0u;
    apsis_controls_bounds_normalize(clock, n);
    for (i = 0u; i < 3u; ++i) {
        if (clock->bounds[i] == step) {
            clock->bounds[i] = 0u;
            apsis_controls_bounds_normalize(clock, n);
            return 1u;
        }
    }
    for (i = 0u; i < 3u; ++i) {
        if (clock->bounds[i] == 0u) {
            clock->bounds[i] = step;
            apsis_controls_bounds_normalize(clock, n);
            return 1u;
        }
    }
    return 0u;
}

uint8_t apsis_controls_surface_from_fm1(const ApsisFm1InputFrame *input,
                                        uint8_t current_surface)
{
    uint32_t p, h;
    uint8_t current = current_surface < APSIS_SURFACE_COUNT ?
                      current_surface : APSIS_SURFACE_MOTHER;

    if (!input) return current;
    p = input->buttons_pressed;
    h = input->buttons_held;

    /* HOME is intentionally absolute even if another modifier/button edge is
     * present in the same consumed input frame. */
    if (p & apsis_button_bit(APSIS_FM1_BTN_HOME))
        return APSIS_SURFACE_MOTHER;

    /* SPACE is a secondary shared-room surface, not a sixth source. The FX
     * press edge is interpreted contextually from SEL's held state. */
    if ((h & apsis_button_bit(APSIS_FM1_BTN_SEL)) &&
        (p & apsis_button_bit(APSIS_FM1_BTN_FX)))
        return APSIS_SURFACE_SPACE;

    if (p & apsis_button_bit(APSIS_FM1_BTN_ENV))
        return APSIS_SURFACE_STRING;
    if (p & apsis_button_bit(APSIS_FM1_BTN_LFO))
        return APSIS_SURFACE_DRIFT;
    if (p & apsis_button_bit(APSIS_FM1_BTN_SEQ))
        return APSIS_SURFACE_CLOCK;
    if (p & apsis_button_bit(APSIS_FM1_BTN_ARP))
        return APSIS_SURFACE_SPIRAL;
    if (p & apsis_button_bit(APSIS_FM1_BTN_FX))
        return APSIS_SURFACE_BLOOM;
    if (p & apsis_button_bit(APSIS_FM1_BTN_GLO))
        return APSIS_SURFACE_GRAVITY;
    if (p & apsis_button_bit(APSIS_FM1_BTN_EDIT))
        return APSIS_SURFACE_CHAIN;
    if (p & apsis_button_bit(APSIS_FM1_BTN_SAVE))
        return APSIS_SURFACE_KEEP;

    return current;
}

ApsisGlobalAction apsis_controls_global_action(const ApsisFm1InputFrame *input)
{
    uint32_t play, select;
    if (!input) return APSIS_GLOBAL_ACTION_NONE;
    play = apsis_button_bit(APSIS_FM1_BTN_PLAY);
    select = apsis_button_bit(APSIS_FM1_BTN_SEL);
    if ((input->buttons_pressed & play) && !(input->buttons_held & select))
        return APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE;
    return APSIS_GLOBAL_ACTION_NONE;
}

uint8_t apsis_controls_clock_layer_step(uint8_t current_layer, int32_t detents)
{
    int32_t layer = current_layer < APSIS_CLOCK_CONTROL_LAYER_COUNT ?
                    (int32_t)current_layer : (int32_t)APSIS_CLOCK_CONTROL_MAIN;
    int32_t span = (int32_t)APSIS_CLOCK_CONTROL_LAYER_COUNT;
    int32_t delta = detents % span;
    layer = (layer + delta) % span;
    if (layer < 0) layer += span;
    return (uint8_t)layer;
}

uint8_t apsis_controls_clock_apply_keys(ApsisClockState *clock,
                                        uint8_t layer,
                                        uint32_t notes_pressed)
{
    uint8_t n, step, changed = 0u;
    if (!clock || layer >= APSIS_CLOCK_CONTROL_LAYER_COUNT) return 0u;
    n = apsis_controls_clock_n(clock);
    for (step = 0u; step < n; ++step) {
        uint32_t bit = 1u << step;
        if (!(notes_pressed & bit)) continue;
        if (layer == APSIS_CLOCK_CONTROL_MAIN) {
            clock->main_mask ^= bit;
            ++changed;
        } else if (layer == APSIS_CLOCK_CONTROL_BETWEEN) {
            clock->inner_mask ^= bit;
            ++changed;
        } else if (layer == APSIS_CLOCK_CONTROL_SNAP) {
            clock->snap_mask ^= bit;
            ++changed;
        } else if (apsis_controls_toggle_boundary(clock, n, step)) {
            ++changed;
        }
    }
    return changed;
}

uint8_t apsis_controls_gravity_degree_from_fm1(const ApsisFm1InputFrame *input,
                                               uint8_t *degree_out)
{
    uint8_t degree;
    if (!input || !degree_out) return 0u;
    for (degree = 0u; degree < APSIS_GRAVITY_DIRECT_KEYS; ++degree) {
        if (input->notes_pressed & (1u << degree)) {
            *degree_out = degree;
            return 1u;
        }
    }
    return 0u;
}

uint8_t apsis_controls_gravity_ring_from_fm1(const ApsisFm1InputFrame *input,
                                             uint8_t current_ring)
{
    uint8_t ring = current_ring < 3u ? current_ring : 0u;
    uint8_t down, up;
    if (!input) return ring;
    down = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_DOWN)) != 0u;
    up = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_UP)) != 0u;
    if (down == up) return ring;
    if (up) return (uint8_t)((ring + 1u) % 3u);
    return ring == 0u ? 2u : (uint8_t)(ring - 1u);
}
