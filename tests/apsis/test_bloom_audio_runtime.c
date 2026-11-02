#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_bloom_audio_runtime.h"
#include "apsis_state.h"

static ApsisTransportEvent bind_event(uint32_t token, uint8_t slot, uint8_t petal)
{
    ApsisTransportEvent e;
    memset(&e, 0, sizeof(e));
    e.type = APSIS_EVENT_BLOOM_BIND;
    e.absolute_pulse = token;
    e.sub_index = slot;
    e.sub_count = petal;
    return e;
}

static void set_native_binding(ApsisRuntime *r, uint32_t token,
                               uint64_t bind_sample, uint64_t target_sample,
                               uint8_t petal, int16_t step)
{
    memset(&r->last_bloom_bind, 0, sizeof(r->last_bloom_bind));
    r->last_bloom_bind.valid = 1u;
    r->last_bloom_bind.serial = token;
    r->last_bloom_bind.bind_sample = bind_sample;
    r->last_bloom_bind.target_sample = target_sample;
    r->last_bloom_bind.pitch.valid = 1u;
    r->last_bloom_bind.pitch.voice_index = petal;
    r->last_bloom_bind.pitch.step = step;
    r->last_bloom_bind.pitch.tuning_mode = APSIS_TUNING_EQUAL;
    r->last_bloom_bind.pitch.tuning_strength_q15 = APSIS_Q15_ONE;
}

static void test_occurrence_snapshot_and_bind_lateness(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisBloomAudioRuntime a;
    ApsisTransportEvent e;
    const ApsisBloomAudioBinding *b;

    apsis_state_default(&s);
    memset(&r, 0, sizeof(r));
    apsis_bloom_audio_runtime_reset(&a);

    r.bloom_slots[3].active = 1u;
    r.bloom_slots[3].token = 77u;
    r.bloom_slots[3].voice_count = 4u;
    r.bloom_slots[3].occurrence_sample = 1000u;
    r.bloom_slots[3].target_sample = 3000u;

    apsis_bloom_audio_runtime_capture(&a, &r, &s);
    assert(a.capture_count == 1u);
    assert(a.capture_miss_count == 0u);
    assert(a.snapshot[3].active == 1u);
    assert(a.snapshot[3].token == 77u);
    assert(a.snapshot[3].acoustic.petal[3].target_offset_samples == 18522u);
    assert(a.snapshot[3].acoustic.petal[3].duration_samples == 91728u);
    assert(a.snapshot[3].acoustic.petal[3].amp_q15 == 153u);
    assert(a.snapshot[3].acoustic.petal[3].pan_q15 == 17694);

    /* These are occurrence-frozen and must not affect the stored acoustic plan. */
    s.bloom.length_ms = 6000u;
    s.bloom.unfurl_ms = 3600u;
    s.bloom.width_q15 = 0u;
    s.bloom.level_q15 = 0u;

    /* These are deliberately bind-late in Study 126. */
    s.bloom.voice = APSIS_BLOOM_GLASS;
    s.bloom.attack_ms = 250u;
    s.bloom.brightness_q15 = APSIS_Q15_MILLI(300);

    set_native_binding(&r, 77u, 1200u, 3000u, 0u, 9);
    e = bind_event(77u, 3u, 0u);
    apsis_bloom_audio_runtime_begin_process(&a);
    apsis_bloom_audio_runtime_bind(&a, &r, &s, &e);

    assert(a.binding_count == 1u);
    b = &a.binding[0];
    assert(b->valid == 1u);
    assert(b->petal_index == 0u);
    assert(b->voice == APSIS_BLOOM_GLASS);
    assert(b->attack_ms == 250u);
    assert(b->brightness_q15 == APSIS_Q15_MILLI(300));
    assert(b->acoustic.duration_samples == 83790u);
    assert(b->acoustic.amp_q15 == 190u);
    assert(b->acoustic.pan_q15 == -17694);
    assert(b->pitch.step == 9);
    assert(a.snapshot[3].active == 1u); /* more petals remain */

    s.bloom.voice = APSIS_BLOOM_ORGAN;
    s.bloom.attack_ms = 1u;              /* bind clamp -> 8 ms */
    s.bloom.brightness_q15 = 0u;         /* bind clamp -> .12 */
    set_native_binding(&r, 77u, 19000u, 21522u, 3u, 16);
    e = bind_event(77u, 3u, 3u);
    apsis_bloom_audio_runtime_begin_process(&a);
    apsis_bloom_audio_runtime_bind(&a, &r, &s, &e);

    assert(a.binding_count == 1u);
    b = &a.binding[0];
    assert(b->petal_index == 3u);
    assert(b->voice == APSIS_BLOOM_ORGAN);
    assert(b->attack_ms == 8u);
    assert(b->brightness_q15 == APSIS_Q15_MILLI(120));
    assert(b->acoustic.target_offset_samples == 18522u);
    assert(b->acoustic.duration_samples == 91728u);
    assert(b->acoustic.amp_q15 == 153u);
    assert(b->acoustic.pan_q15 == 17694);
    assert(b->pitch.step == 16);
    assert(a.snapshot[3].active == 0u); /* final frozen petal */
    assert(a.bind_count == 2u);
}

static void test_skipped_bind_consumes_final_snapshot_without_audio(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisBloomAudioRuntime a;
    ApsisTransportEvent e;

    apsis_state_default(&s);
    memset(&r, 0, sizeof(r));
    apsis_bloom_audio_runtime_reset(&a);

    r.bloom_slots[0].active = 1u;
    r.bloom_slots[0].token = 9u;
    r.bloom_slots[0].voice_count = 1u;
    apsis_bloom_audio_runtime_capture(&a, &r, &s);
    assert(a.snapshot[0].active == 1u);

    memset(&r.last_bloom_bind, 0, sizeof(r.last_bloom_bind));
    r.last_bloom_bind.valid = 1u;
    r.last_bloom_bind.serial = 9u;
    r.last_bloom_bind.skipped = 1u;
    e = bind_event(9u, 0u, 0u);
    apsis_bloom_audio_runtime_begin_process(&a);
    apsis_bloom_audio_runtime_bind(&a, &r, &s, &e);

    assert(a.binding_count == 0u);
    assert(a.snapshot[0].active == 0u);
}

static void test_token_mismatch_is_visible(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisBloomAudioRuntime a;
    ApsisTransportEvent e;

    apsis_state_default(&s);
    memset(&r, 0, sizeof(r));
    apsis_bloom_audio_runtime_reset(&a);
    r.bloom_slots[2].active = 1u;
    r.bloom_slots[2].token = 4u;
    r.bloom_slots[2].voice_count = 3u;
    apsis_bloom_audio_runtime_capture(&a, &r, &s);

    set_native_binding(&r, 5u, 100u, 200u, 0u, 0);
    e = bind_event(5u, 2u, 0u);
    apsis_bloom_audio_runtime_bind(&a, &r, &s, &e);
    assert(a.bind_miss_count == 1u);
    assert(a.binding_count == 0u);
}

int main(void)
{
    test_occurrence_snapshot_and_bind_lateness();
    test_skipped_bind_consumes_final_snapshot_without_audio();
    test_token_mismatch_is_visible();
    puts("M4g Bloom snapshot/bind bridge PASS");
    return 0;
}
