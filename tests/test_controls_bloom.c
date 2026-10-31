#include <stdio.h>
#include <string.h>
#include "apsis_controls_bloom.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint32_t bit(uint8_t b) { return 1u << b; }
static int32_t absi(int32_t v) { return v < 0 ? -v : v; }

static void bloom_state(ApsisState *s, uint8_t voices)
{
    memset(s, 0, sizeof *s);
    s->gravity.scale = APSIS_SCALE_DORIAN;
    s->gravity.voicing = APSIS_GRAVITY_COLOUR;
    s->gravity.voices = voices;
    s->bloom.depth = APSIS_BLOOM_DEPTH_FOLLOW;
    s->bloom.unfurl_ms = 420u;
    s->bloom.voicing_spread_q15 = APSIS_Q15_MILLI(480);
    s->bloom.length_ms = 1900u;
    s->bloom.brightness_q15 = APSIS_Q15_MILLI(720);
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
    return 0;
}

static int test_source_grow_thresholds(void)
{
    ApsisBloomGrowGesture g;
    uint8_t fired;

    apsis_controls_bloom_grow_reset(&g);
    fired = apsis_controls_bloom_grow_update(&g, 4u, 3);
    CHECK(fired == 0u && absi((int32_t)g.open_q15 - 3 * (int32_t)APSIS_BLOOM_GROW_STEP_Q15) <= 1);
    fired = apsis_controls_bloom_grow_update(&g, 4u, 1);
    CHECK(fired == 0x01u);
    fired = apsis_controls_bloom_grow_update(&g, 4u, 5);
    CHECK(fired == 0u);
    fired = apsis_controls_bloom_grow_update(&g, 4u, 1);
    CHECK(fired == 0x02u);
    fired = apsis_controls_bloom_grow_update(&g, 4u, 100);
    CHECK(fired == 0x0Cu && g.fired_mask == 0x0Fu && g.open_q15 == APSIS_Q15_ONE);
    fired = apsis_controls_bloom_grow_update(&g, 4u, -100);
    CHECK(fired == 0u && g.fired_mask == 0x0Fu && g.open_q15 == 0u);

    apsis_controls_bloom_grow_reset(&g);
    fired = apsis_controls_bloom_grow_update(&g, 5u, 8); /* 32% */
    CHECK(fired == 0x03u); /* source thresholds 14%, 32%, 50%, 68%, 86% */
    return 0;
}

static int test_source_params(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisBloomParamEdit e[4];
    uint8_t n;
    uint16_t spread_native, brightness_native;

    bloom_state(&s, 4u);
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1] = 3;
    in.encoder_steps[APSIS_FM1_ENC_K2] = 2;
    in.encoder_steps[APSIS_FM1_ENC_K3] = -5;
    in.encoder_steps[APSIS_FM1_ENC_K4] = -2;
    n = apsis_controls_bloom_param_edits(&s.bloom, &in, e);
    CHECK(n == 4u);
    CHECK(e[0].param == APSIS_BLOOM_PARAM_UNFURL && e[0].value == 450);
    spread_native = (uint16_t)(APSIS_Q15_MILLI(480) + 2u * APSIS_Q15_MILLI(10));
    CHECK(e[1].param == APSIS_BLOOM_PARAM_SPREAD && e[1].value == spread_native);
    CHECK(absi(e[1].value - (int32_t)APSIS_Q15_MILLI(500)) <= 1);
    CHECK(e[2].param == APSIS_BLOOM_PARAM_LENGTH && e[2].value == 1850);
    brightness_native = (uint16_t)(APSIS_Q15_MILLI(720) - 2u * APSIS_Q15_MILLI(10));
    CHECK(e[3].param == APSIS_BLOOM_PARAM_BRIGHTNESS && e[3].value == brightness_native);
    CHECK(absi(e[3].value - (int32_t)APSIS_Q15_MILLI(700)) <= 1);

    s.bloom.unfurl_ms = 3600u;
    s.bloom.voicing_spread_q15 = APSIS_Q15_ONE;
    s.bloom.length_ms = 650u;
    s.bloom.brightness_q15 = APSIS_Q15_MILLI(120);
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1] = 100;
    in.encoder_steps[APSIS_FM1_ENC_K2] = 100;
    in.encoder_steps[APSIS_FM1_ENC_K3] = -100;
    in.encoder_steps[APSIS_FM1_ENC_K4] = -100;
    n = apsis_controls_bloom_param_edits(&s.bloom, &in, e);
    CHECK(n == 4u);
    CHECK(e[0].value == 3600 && e[1].value == APSIS_Q15_ONE);
    CHECK(e[2].value == 650 && e[3].value == APSIS_Q15_MILLI(120));
    return 0;
}

int main(void)
{
    CHECK(test_petals_and_core() == 0);
    CHECK(test_depth_owns_petals() == 0);
    CHECK(test_grow_ownership() == 0);
    CHECK(test_source_grow_thresholds() == 0);
    CHECK(test_source_params() == 0);
    puts("PASS: APSIS P9E BLOOM source-locked performance/parameter contract");
    return 0;
}
