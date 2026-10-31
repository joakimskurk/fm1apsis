#include <stdio.h>
#include <string.h>
#include "apsis_controls_bloom.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint32_t bit(uint8_t b) { return 1u << b; }

static void bloom_state(ApsisState *s, uint8_t voices)
{
    memset(s, 0, sizeof *s);
    s->gravity.scale = APSIS_SCALE_DORIAN;
    s->gravity.voicing = APSIS_GRAVITY_COLOUR;
    s->gravity.voices = voices;
    s->bloom.depth = APSIS_BLOOM_DEPTH_FOLLOW;
}

static int test_petals_and_core(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisBloomControlFrame f;

    bloom_state(&s, 4u);
    memset(&in, 0, sizeof in);
    in.notes_pressed = 1u << 2;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_PETAL && f.petal_index == 2u);

    memset(&in, 0, sizeof in);
    in.notes_pressed = 1u << 4;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 0u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_NONE);

    memset(&in, 0, sizeof in);
    in.notes_pressed = (1u << 0) | (1u << APSIS_BLOOM_CORE_KEY);
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_CORE);

    bloom_state(&s, 3u);
    memset(&in, 0, sizeof in);
    in.notes_pressed = (1u << 2) | (1u << 1);
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_PETAL && f.petal_index == 1u);

    bloom_state(&s, 5u);
    memset(&in, 0, sizeof in);
    in.notes_pressed = 1u << 4;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_PETAL && f.petal_index == 4u);
    return 0;
}

static int test_depth_owns_petals(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisBloomControlFrame f;

    bloom_state(&s, 5u);
    s.bloom.depth = APSIS_BLOOM_DEPTH_TRIAD;
    memset(&in, 0, sizeof in);
    in.notes_pressed = 1u << 3;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 0u);
    in.notes_pressed = 1u << 2;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_PETAL && f.petal_index == 2u);

    s.bloom.depth = APSIS_BLOOM_DEPTH_NINTH;
    in.notes_pressed = 1u << 4;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.petal_index == 4u);
    return 0;
}

static int test_grow_ownership(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisBloomControlFrame f;

    bloom_state(&s, 4u);
    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    in.buttons_held = bit(APSIS_FM1_BTN_REC);
    in.encoder_steps[APSIS_FM1_ENC_SELECT] = 3;
    in.notes_pressed = (1u << 1) | (1u << APSIS_BLOOM_CORE_KEY);
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.grow_begin == 1u && f.grow_end == 0u && f.grow_detents == 3);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_NONE);

    memset(&in, 0, sizeof in);
    in.buttons_held = bit(APSIS_FM1_BTN_REC);
    in.encoder_steps[APSIS_FM1_ENC_SELECT] = -2;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.grow_begin == 0u && f.grow_end == 0u && f.grow_detents == -2);

    memset(&in, 0, sizeof in);
    in.buttons_released = bit(APSIS_FM1_BTN_REC);
    in.notes_pressed = 1u << 0;
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 1u);
    CHECK(f.grow_end == 1u && f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_NONE);

    memset(&in, 0, sizeof in);
    CHECK(apsis_controls_bloom_frame(&s, &in, &f) == 0u);
    CHECK(f.trigger == APSIS_BLOOM_CONTROL_TRIGGER_NONE && f.grow_detents == 0);
    return 0;
}

int main(void)
{
    CHECK(test_petals_and_core() == 0);
    CHECK(test_depth_owns_petals() == 0);
    CHECK(test_grow_ownership() == 0);
    puts("PASS: APSIS P9E1 BLOOM performance-intent contract");
    return 0;
}
