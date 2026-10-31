#include <stdio.h>
#include <string.h>
#include "apsis_controls.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; \
} } while (0)

static uint32_t bit(uint8_t b) { return 1u << b; }

static uint8_t nav(uint8_t button, uint8_t current)
{
    ApsisFm1InputFrame in;
    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(button);
    in.buttons_held = bit(button);
    return apsis_controls_surface_from_fm1(&in, current);
}

static int test_p9a(void)
{
    ApsisFm1InputFrame in;
    static const struct { uint8_t button, surface; } direct[] = {
        {APSIS_FM1_BTN_HOME, APSIS_SURFACE_MOTHER},
        {APSIS_FM1_BTN_ENV, APSIS_SURFACE_STRING},
        {APSIS_FM1_BTN_LFO, APSIS_SURFACE_DRIFT},
        {APSIS_FM1_BTN_SEQ, APSIS_SURFACE_CLOCK},
        {APSIS_FM1_BTN_ARP, APSIS_SURFACE_SPIRAL},
        {APSIS_FM1_BTN_FX, APSIS_SURFACE_BLOOM},
        {APSIS_FM1_BTN_GLO, APSIS_SURFACE_GRAVITY},
        {APSIS_FM1_BTN_EDIT, APSIS_SURFACE_CHAIN},
        {APSIS_FM1_BTN_SAVE, APSIS_SURFACE_KEEP}
    };
    uint32_t i;

    CHECK(APSIS_FM1_BUTTON_COUNT == 14u);
    CHECK(APSIS_FM1_ENCODER_COUNT == 7u);
    CHECK(APSIS_FM1_KEY_COUNT == 27u);
    CHECK(APSIS_SURFACE_COUNT == 10u);

    for (i = 0u; i < sizeof direct / sizeof direct[0]; ++i)
        CHECK(nav(direct[i].button, APSIS_SURFACE_MOTHER) == direct[i].surface);

    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_FX);
    in.buttons_held = bit(APSIS_FM1_BTN_SEL) | bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_surface_from_fm1(&in, APSIS_SURFACE_BLOOM) == APSIS_SURFACE_SPACE);

    /* SEL+FX pressed in one consumed input frame also qualifies: the contract
     * is edge-on-FX plus current SEL-held state, exactly what Dinghy exposes. */
    in.buttons_pressed |= bit(APSIS_FM1_BTN_SEL);
    CHECK(apsis_controls_surface_from_fm1(&in, APSIS_SURFACE_BLOOM) == APSIS_SURFACE_SPACE);

    /* Pressing SEL while FX is merely held is not the SPACE gesture. */
    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_SEL);
    in.buttons_held = bit(APSIS_FM1_BTN_SEL) | bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_surface_from_fm1(&in, APSIS_SURFACE_BLOOM) == APSIS_SURFACE_BLOOM);

    /* HOME stays absolute even if the SPACE chord edge arrives simultaneously. */
    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_HOME) | bit(APSIS_FM1_BTN_FX);
    in.buttons_held = bit(APSIS_FM1_BTN_HOME) | bit(APSIS_FM1_BTN_SEL) | bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_surface_from_fm1(&in, APSIS_SURFACE_SPACE) == APSIS_SURFACE_MOTHER);

    memset(&in, 0, sizeof in);
    CHECK(apsis_controls_surface_from_fm1(&in, 255u) == APSIS_SURFACE_MOTHER);
    CHECK(apsis_controls_surface_from_fm1(0, APSIS_SURFACE_CLOCK) == APSIS_SURFACE_CLOCK);

    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_PLAY);
    in.buttons_held = bit(APSIS_FM1_BTN_PLAY);
    CHECK(apsis_controls_global_action(&in) == APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE);

    in.buttons_held |= bit(APSIS_FM1_BTN_SEL);
    CHECK(apsis_controls_global_action(&in) == APSIS_GLOBAL_ACTION_NONE);
    CHECK(apsis_controls_global_action(0) == APSIS_GLOBAL_ACTION_NONE);
    return 0;
}

static int test_clock_layers_and_keys(void)
{
    ApsisClockState c;
    memset(&c, 0, sizeof c);

    CHECK(APSIS_CLOCK_CONTROL_LAYER_COUNT == 4u);
    CHECK(apsis_controls_clock_layer_step(APSIS_CLOCK_CONTROL_MAIN, 1) == APSIS_CLOCK_CONTROL_BETWEEN);
    CHECK(apsis_controls_clock_layer_step(APSIS_CLOCK_CONTROL_MAIN, 3) == APSIS_CLOCK_CONTROL_GROUP);
    CHECK(apsis_controls_clock_layer_step(APSIS_CLOCK_CONTROL_GROUP, 1) == APSIS_CLOCK_CONTROL_MAIN);
    CHECK(apsis_controls_clock_layer_step(APSIS_CLOCK_CONTROL_MAIN, -1) == APSIS_CLOCK_CONTROL_GROUP);
    CHECK(apsis_controls_clock_layer_step(255u, 0) == APSIS_CLOCK_CONTROL_MAIN);

    c.n = 5u;
    c.main_mask = 0x1Fu;
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_MAIN,
                                          (1u << 1) | (1u << 4) | (1u << 5)) == 2u);
    CHECK(c.main_mask == (0x1Fu ^ (1u << 1) ^ (1u << 4)));

    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_BETWEEN,
                                          (1u << 0) | (1u << 3)) == 2u);
    CHECK(c.inner_mask == ((1u << 0) | (1u << 3)));
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_SNAP,
                                          (1u << 2)) == 1u);
    CHECK(c.snap_mask == (1u << 2));
    CHECK(apsis_controls_clock_apply_keys(&c, 255u, 0xFFFFFFFFu) == 0u);
    CHECK(apsis_controls_clock_apply_keys(0, APSIS_CLOCK_CONTROL_MAIN, 1u) == 0u);

    memset(&c, 0, sizeof c);
    c.n = 8u;
    c.bounds[0] = 6u;
    c.bounds[1] = 2u;
    c.bounds[2] = 4u;
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_GROUP, 1u << 4) == 1u);
    CHECK(c.bounds[0] == 2u && c.bounds[1] == 6u && c.bounds[2] == 0u);
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_GROUP, 1u << 5) == 1u);
    CHECK(c.bounds[0] == 2u && c.bounds[1] == 5u && c.bounds[2] == 6u);
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_GROUP, 1u << 3) == 0u);
    CHECK(c.bounds[0] == 2u && c.bounds[1] == 5u && c.bounds[2] == 6u);
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_GROUP, 1u << 0) == 0u);
    CHECK(apsis_controls_clock_apply_keys(&c, APSIS_CLOCK_CONTROL_GROUP, 1u << 2) == 1u);
    CHECK(c.bounds[0] == 5u && c.bounds[1] == 6u && c.bounds[2] == 0u);
    return 0;
}

static int test_gravity_direct_controls(void)
{
    ApsisFm1InputFrame in;
    uint8_t degree = 99u;

    memset(&in, 0, sizeof in);
    in.notes_pressed = (1u << 5) | (1u << 2);
    CHECK(apsis_controls_gravity_degree_from_fm1(&in, &degree) == 1u);
    CHECK(degree == 2u);

    in.notes_pressed = 1u << 8;
    CHECK(apsis_controls_gravity_degree_from_fm1(&in, &degree) == 0u);
    CHECK(apsis_controls_gravity_degree_from_fm1(0, &degree) == 0u);
    CHECK(apsis_controls_gravity_degree_from_fm1(&in, 0) == 0u);

    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_UP);
    CHECK(apsis_controls_gravity_ring_from_fm1(&in, 0u) == 1u);
    CHECK(apsis_controls_gravity_ring_from_fm1(&in, 2u) == 0u);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_DOWN);
    CHECK(apsis_controls_gravity_ring_from_fm1(&in, 0u) == 2u);
    CHECK(apsis_controls_gravity_ring_from_fm1(&in, 2u) == 1u);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_DOWN) | bit(APSIS_FM1_BTN_OCT_UP);
    CHECK(apsis_controls_gravity_ring_from_fm1(&in, 1u) == 1u);
    in.buttons_pressed = 0u;
    CHECK(apsis_controls_gravity_ring_from_fm1(&in, 9u) == 0u);
    CHECK(apsis_controls_gravity_ring_from_fm1(0, 2u) == 2u);
    return 0;
}

int main(void)
{
    CHECK(test_p9a() == 0);
    CHECK(test_clock_layers_and_keys() == 0);
    CHECK(test_gravity_direct_controls() == 0);
    puts("PASS: APSIS P9A/P9B FM-1 interaction contract");
    return 0;
}
