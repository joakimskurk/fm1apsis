#include <assert.h>
#include <stdio.h>

#include "apsis_bloom_audio.h"
#include "apsis_state.h"

static void test_default_four_petal_plan(void)
{
    static const uint32_t offset[4] = {0u, 6174u, 12348u, 18522u};
    static const uint32_t duration[4] = {83790u, 86436u, 89082u, 91728u};
    static const uint16_t amp[4] = {190u, 178u, 165u, 153u};
    static const int16_t pan[4] = {-17694, -5898, 5898, 17694};
    ApsisState s;
    ApsisBloomAcousticPlan p;
    uint8_t i;

    apsis_state_default(&s);
    apsis_bloom_audio_plan(&s, 4u, &p);

    assert(p.valid == 1u);
    assert(p.count == 4u);
    assert(p.unfurl_samples == 18522u);
    for (i = 0u; i < 4u; ++i) {
        assert(p.petal[i].target_offset_samples == offset[i]);
        assert(p.petal[i].duration_samples == duration[i]);
        assert(p.petal[i].amp_q15 == amp[i]);
        assert(p.petal[i].pan_q15 == pan[i]);
    }
}

static void test_five_petal_max_edges(void)
{
    static const uint32_t offset[5] = {0u, 39690u, 79380u, 119070u, 158760u};
    static const uint32_t duration[5] = {264600u, 267246u, 269892u, 272538u, 275184u};
    static const uint16_t amp[5] = {410u, 383u, 356u, 330u, 303u};
    static const int16_t pan[5] = {-32767, -16384, 0, 16384, 32767};
    ApsisState s;
    ApsisBloomAcousticPlan p;
    uint8_t i;

    apsis_state_default(&s);
    s.bloom.length_ms = 65000u;      /* source clamp -> 6000 ms */
    s.bloom.unfurl_ms = 5000u;       /* source clamp -> 3600 ms */
    s.bloom.width_q15 = 65535u;      /* source clamp -> 1.0 */
    s.bloom.level_q15 = 65535u;      /* source clamp -> 1.25 */

    apsis_bloom_audio_plan(&s, 5u, &p);
    assert(p.valid == 1u);
    assert(p.count == 5u);
    assert(p.unfurl_samples == 158760u);
    for (i = 0u; i < 5u; ++i) {
        assert(p.petal[i].target_offset_samples == offset[i]);
        assert(p.petal[i].duration_samples == duration[i]);
        assert(p.petal[i].amp_q15 == amp[i]);
        assert(p.petal[i].pan_q15 == pan[i]);
    }
}

static void test_minimum_and_invalid_count(void)
{
    ApsisState s;
    ApsisBloomAcousticPlan p;

    apsis_state_default(&s);
    s.bloom.length_ms = 0u;
    s.bloom.unfurl_ms = 5000u;
    s.bloom.width_q15 = 65535u;
    s.bloom.level_q15 = 65535u;

    apsis_bloom_audio_plan(&s, 1u, &p);
    assert(p.valid == 1u);
    assert(p.count == 1u);
    assert(p.unfurl_samples == 158760u);
    assert(p.petal[0].target_offset_samples == 0u);
    assert(p.petal[0].duration_samples == 28665u);
    assert(p.petal[0].amp_q15 == 410u);
    assert(p.petal[0].pan_q15 == 0);

    apsis_bloom_audio_plan(&s, 0u, &p);
    assert(p.valid == 0u);
    assert(p.count == 0u);
}

static void test_plan_is_a_value_snapshot(void)
{
    ApsisState s;
    ApsisBloomAcousticPlan p;

    apsis_state_default(&s);
    apsis_bloom_audio_plan(&s, 4u, &p);
    s.bloom.length_ms = 6000u;
    s.bloom.unfurl_ms = 3600u;
    s.bloom.width_q15 = 0u;
    s.bloom.level_q15 = 0u;

    /* Occurrence-time acoustic values stay frozen after later state edits. */
    assert(p.petal[3].target_offset_samples == 18522u);
    assert(p.petal[3].duration_samples == 91728u);
    assert(p.petal[3].amp_q15 == 153u);
    assert(p.petal[3].pan_q15 == 17694);
}

int main(void)
{
    test_default_four_petal_plan();
    test_five_petal_max_edges();
    test_minimum_and_invalid_count();
    test_plan_is_a_value_snapshot();
    puts("M4g Bloom occurrence acoustic planning PASS");
    return 0;
}
