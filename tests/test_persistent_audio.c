#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_drift_dsp.h"
#include "apsis_persistent_audio.h"

static uint32_t hash_samples(const int32_t *v, uint32_t n)
{
    uint32_t h = 2166136261u;
    uint32_t i;
    for (i = 0u; i < n; ++i) {
        uint32_t x = (uint32_t)v[i];
        h ^= x & 0xffu; h *= 16777619u;
        h ^= (x >> 8) & 0xffu; h *= 16777619u;
        h ^= (x >> 16) & 0xffu; h *= 16777619u;
        h ^= (x >> 24) & 0xffu; h *= 16777619u;
    }
    return h;
}

static void assert_apply_equivalent(const ApsisState *source)
{
    ApsisState a_state = *source;
    ApsisState b_state = *source;
    ApsisStringPitchMemory a_mem, b_mem;
    ApsisStringDsp a_string, b_string;
    ApsisDriftDsp a_drift, b_drift;
    ApsisPersistentAudioTargets targets;
    int32_t a_out[512 * 2];
    int32_t b_out[512 * 2];

    apsis_string_pitch_memory_reset(&a_mem);
    apsis_string_pitch_memory_reset(&b_mem);
    apsis_string_dsp_reset(&a_string);
    apsis_string_dsp_reset(&b_string);
    apsis_drift_dsp_reset(&a_drift);
    apsis_drift_dsp_reset(&b_drift);

    apsis_string_dsp_update(&a_string, &a_state, &a_mem, 1u);
    apsis_drift_dsp_update(&a_drift, &a_state, 1u);

    apsis_persistent_audio_capture(&b_state, &b_mem, 1u, &targets);
    apsis_persistent_audio_apply(&b_string, &b_drift, &targets);

    assert(memcmp(&a_mem, &b_mem, sizeof(a_mem)) == 0);
    assert(memcmp(&a_string, &b_string, sizeof(a_string)) == 0);
    assert(memcmp(&a_drift, &b_drift, sizeof(a_drift)) == 0);

    memset(a_out, 0, sizeof(a_out));
    memset(b_out, 0, sizeof(b_out));
    apsis_string_dsp_render(&a_string, a_out, 512u);
    apsis_drift_dsp_render(&a_drift, a_out, 512u);
    apsis_string_dsp_render(&b_string, b_out, 512u);
    apsis_drift_dsp_render(&b_drift, b_out, 512u);
    assert(memcmp(a_out, b_out, sizeof(a_out)) == 0);
    assert(hash_samples(a_out, 1024u) == hash_samples(b_out, 1024u));
}

static void test_direct_update_equivalence(void)
{
    ApsisState s;
    apsis_state_default(&s);
    assert_apply_equivalent(&s);

    s.gravity.degree = 4u;
    s.gravity.voicing = 2u;
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    s.string.glide_q15 = APSIS_Q15_MILLI(310);
    s.string.foundation_octave = 0;
    s.drift.amount_q15 = APSIS_Q15_MILLI(770);
    s.drift.depth_q15 = APSIS_Q15_MILLI(910);
    s.drift.width_q15 = APSIS_Q15_MILLI(230);
    assert_apply_equivalent(&s);

    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.left_level_q15 = APSIS_Q15_MILLI(410);
    s.string.right_level_q15 = APSIS_Q15_MILLI(880);
    s.temper.mode = APSIS_TUNING_JUST;
    s.temper.strength_q15 = APSIS_Q15_MILLI(650);
    assert_apply_equivalent(&s);
}

static ApsisPersistentAudioTargets marker_targets(uint16_t marker)
{
    ApsisPersistentAudioTargets t;
    memset(&t, 0, sizeof(t));
    t.transport_running = 1u;
    t.string_surface_mode = APSIS_STRING_CLASSIC;
    t.string_glide_q15 = APSIS_Q15_MILLI(55);
    t.string_targets.freq_millihz[0] = 100000u + marker;
    t.string_targets.gain_q15[0] = marker;
    t.string_targets.enabled[0] = 1u;
    t.string_targets.mix_left_q15[0] = APSIS_Q15_ONE;
    t.string_targets.mix_right_q15[0] = APSIS_Q15_ONE;
    t.drift_targets.freq_millihz[0] = 200000u + marker;
    t.drift_targets.gain_q15[0] = marker;
    return t;
}

static void test_queue_order_and_boundary(void)
{
    ApsisPersistentAudioQueue q;
    ApsisPersistentAudioTargets a = marker_targets(11u);
    ApsisPersistentAudioTargets b = marker_targets(22u);
    ApsisPersistentAudioTargets c = marker_targets(33u);
    const ApsisPersistentAudioEvent *e;
    uint64_t sample = 0u;

    apsis_persistent_audio_queue_reset(&q);
    assert(apsis_persistent_audio_queue_push(&q, 200u, &a));
    assert(apsis_persistent_audio_queue_push(&q, 100u, &b));
    assert(apsis_persistent_audio_queue_push(&q, 200u, &c));
    assert(q.count == 3u);
    assert(q.high_water == 3u);
    assert(!q.overflowed);

    e = apsis_persistent_audio_queue_peek(&q);
    assert(e && e->sample == 100u && e->targets.string_targets.gain_q15[0] == 22u);
    assert(apsis_persistent_audio_next_boundary(&q, 90u, 100u, &sample) == 0u);
    assert(apsis_persistent_audio_next_boundary(&q, 90u, 101u, &sample) == 1u);
    assert(sample == 100u);

    assert(apsis_persistent_audio_queue_pop(&q, 0));
    e = apsis_persistent_audio_queue_peek(&q);
    assert(e && e->sample == 200u && e->targets.string_targets.gain_q15[0] == 11u);
    assert(apsis_persistent_audio_queue_pop(&q, 0));
    e = apsis_persistent_audio_queue_peek(&q);
    assert(e && e->sample == 200u && e->targets.string_targets.gain_q15[0] == 33u);
}

static void test_late_boundary_and_apply_order(void)
{
    ApsisPersistentAudioQueue q;
    ApsisPersistentAudioTargets a = marker_targets(101u);
    ApsisPersistentAudioTargets b = marker_targets(202u);
    ApsisStringDsp sd;
    ApsisDriftDsp dd;
    uint64_t boundary = 0u;

    apsis_persistent_audio_queue_reset(&q);
    apsis_string_dsp_reset(&sd);
    apsis_drift_dsp_reset(&dd);
    assert(apsis_persistent_audio_queue_push(&q, 50u, &a));
    assert(apsis_persistent_audio_queue_push(&q, 50u, &b));

    assert(apsis_persistent_audio_next_boundary(&q, 64u, 128u, &boundary));
    assert(boundary == 64u);
    assert(apsis_persistent_audio_apply_through(&q, 64u, &sd, &dd) == 2u);
    assert(q.count == 0u);
    /* Same-target serial order means the later snapshot wins. */
    assert(sd.voice[0].desired_gain_q15 == 202u);
    assert(dd.voice[0].target_gain_q23 == ((int32_t)202u << 8));
}

static void test_frozen_capacity_is_visible(void)
{
    ApsisPersistentAudioQueue q;
    ApsisPersistentAudioTargets t = marker_targets(1u);
    uint8_t i;
    apsis_persistent_audio_queue_reset(&q);
    assert(APSIS_PERSISTENT_AUDIO_QUEUE_CAP == 4u);
    for (i = 0u; i < APSIS_PERSISTENT_AUDIO_QUEUE_CAP; ++i)
        assert(apsis_persistent_audio_queue_push(&q, (uint64_t)i, &t));
    assert(q.high_water == APSIS_PERSISTENT_AUDIO_QUEUE_CAP);
    assert(!q.overflowed);
    assert(!apsis_persistent_audio_queue_push(&q, 999u, &t));
    assert(q.overflowed);
    assert(sizeof(q) < 512u);
}

int main(void)
{
    test_direct_update_equivalence();
    test_queue_order_and_boundary();
    test_late_boundary_and_apply_order();
    test_frozen_capacity_is_visible();
    printf("M5a persistent audio PASS queue_bytes=%zu frozen_cap=%u\n",
           sizeof(ApsisPersistentAudioQueue),
           (unsigned)APSIS_PERSISTENT_AUDIO_QUEUE_CAP);
    return 0;
}
