#include <assert.h>
#include <stdio.h>
#include "apsis_state.h"
#include "apsis_bloom.h"
#include "apsis_bloom_dsp.h"
#include "apsis_bloom_manual_audio.h"

static ApsisBloomDsp dsp;
static ApsisBloomManualAudio manual;

static uint32_t first_phase_inc(void)
{
    uint16_t i;
    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i)
        if (dsp.petal[i].active && dsp.petal[i].osc_count)
            return dsp.petal[i].osc[0].phase_inc;
    return 0u;
}

static uint32_t first_peak_q23(void)
{
    uint16_t i;
    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i)
        if (dsp.petal[i].active)
            return dsp.petal[i].env_event_gain_q23[0];
    return 0u;
}

static ApsisLatePendingGravity pending_gravity(uint8_t active,
                                               uint8_t degree,
                                               uint8_t ring,
                                               uint64_t target)
{
    ApsisLatePendingGravity p = {0};
    p.active = active;
    p.degree = degree;
    p.ring = ring;
    p.target_sample = target;
    return p;
}

static void test_direct_petal_and_grow(void)
{
    ApsisState s;
    ApsisBloomVoicingState voicing;
    ApsisLatePendingGravity none, future;
    uint32_t normal_pitch, pending_pitch, direct_peak, grow_peak;

    apsis_state_default(&s);
    apsis_bloom_voicing_reset(&voicing);
    none = pending_gravity(0u, 0u, 0u, 0u);
    future = pending_gravity(1u, 3u, APSIS_GRAVITY_COLOUR, 20000u);

    apsis_bloom_dsp_reset(&dsp);
    apsis_bloom_manual_audio_reset(&manual);
    assert(apsis_bloom_manual_trigger_petals(&manual, &dsp, &s, &voicing,
                                             &none, 0x01u, 0u, 10000u) == 1u);
    normal_pitch = first_phase_inc();
    direct_peak = first_peak_q23();
    assert(normal_pitch != 0u && direct_peak != 0u);

    /* Study-126 direct petal snapshots bloomDisplayHarmony(): a future queued
     * Gravity degree is heard immediately, rather than waiting for its target. */
    apsis_bloom_dsp_reset(&dsp);
    apsis_bloom_manual_audio_reset(&manual);
    assert(apsis_bloom_manual_trigger_petals(&manual, &dsp, &s, &voicing,
                                             &future, 0x01u, 0u, 10000u) == 1u);
    pending_pitch = first_phase_inc();
    assert(pending_pitch != 0u);
    assert(pending_pitch != normal_pitch);

    /* Grow uses the lower source scale .78 + .055*i, not direct petal 1.04. */
    apsis_bloom_dsp_reset(&dsp);
    apsis_bloom_manual_audio_reset(&manual);
    assert(apsis_bloom_manual_trigger_petals(&manual, &dsp, &s, &voicing,
                                             &none, 0x01u, 0x01u, 10000u) == 1u);
    grow_peak = first_peak_q23();
    assert(grow_peak != 0u);
    assert(grow_peak < direct_peak);
}

static void test_core_late_binding(void)
{
    ApsisState s;
    ApsisBloomVoicingState voicing;
    ApsisLatePendingGravity none;
    uint64_t occurrence = 10000u, max_bind = 0u;
    uint8_t voices, i, first_bound;
    ApsisBloomHarmony h;

    apsis_state_default(&s);
    s.bloom.unfurl_ms = 1000u;
    apsis_bloom_voicing_reset(&voicing);
    apsis_bloom_dsp_reset(&dsp);
    apsis_bloom_manual_audio_reset(&manual);
    none = pending_gravity(0u, 0u, 0u, 0u);
    apsis_bloom_harmony_from_state(&s, &h);
    voices = apsis_bloom_voice_count(&s, &h);

    assert(apsis_bloom_manual_trigger_core(&manual, &s, &voicing, &none,
                                           occurrence, 0x12345678u,
                                           0x87654321u) == voices);
    assert(voicing.serial == 1u);
    assert(manual.core_count == 1u);
    assert(apsis_bloom_manual_pending_count(&manual) == voices);

    for (i = 0u; i < APSIS_BLOOM_MANUAL_PENDING_MAX; ++i)
        if (manual.pending[i].active && manual.pending[i].bind_sample > max_bind)
            max_bind = manual.pending[i].bind_sample;
    assert(max_bind > occurrence);

    first_bound = apsis_bloom_manual_process(&manual, &dsp, &s, &none,
                                             occurrence, 256u);
    assert(first_bound == 1u);
    assert(apsis_bloom_dsp_active(&dsp) == 1u);
    assert(apsis_bloom_manual_pending_count(&manual) == (uint8_t)(voices - 1u));

    /* Later core petals stay finite and bind only when their normal 50-ms lead
     * window is reached; processing the final bind block clears the snapshot. */
    assert(apsis_bloom_manual_process(&manual, &dsp, &s, &none,
                                      max_bind, 256u) == (uint8_t)(voices - 1u));
    assert(apsis_bloom_manual_pending_count(&manual) == 0u);
    assert(apsis_bloom_dsp_active(&dsp) == voices);
    assert(manual.bind_count == voices);
    assert(manual.core[0].active == 0u);
}

static void test_core_capacity_and_mute(void)
{
    ApsisState s;
    ApsisBloomVoicingState voicing;
    ApsisLatePendingGravity none;
    uint8_t i;

    apsis_state_default(&s);
    s.bloom.unfurl_ms = 3600u;
    apsis_bloom_voicing_reset(&voicing);
    apsis_bloom_dsp_reset(&dsp);
    apsis_bloom_manual_audio_reset(&manual);
    none = pending_gravity(0u, 0u, 0u, 0u);

    for (i = 0u; i < APSIS_BLOOM_MANUAL_CORE_SLOTS; ++i)
        assert(apsis_bloom_manual_trigger_core(&manual, &s, &voicing, &none,
                                               10000u + (uint64_t)i * 64u,
                                               100u + i, 200u + i) > 0u);
    assert(apsis_bloom_manual_trigger_core(&manual, &s, &voicing, &none,
                                           11000u, 1u, 2u) == 0u);
    assert(manual.drop_count >= 1u);

    apsis_bloom_manual_audio_reset(&manual);
    apsis_bloom_dsp_reset(&dsp);
    s.bloom.sound = 0u;
    assert(apsis_bloom_manual_trigger_petals(&manual, &dsp, &s, &voicing,
                                             &none, 0x01u, 0u, 10000u) == 0u);
    assert(apsis_bloom_manual_trigger_core(&manual, &s, &voicing, &none,
                                           10000u, 1u, 2u) == 0u);
}

int main(void)
{
    test_direct_petal_and_grow();
    test_core_late_binding();
    test_core_capacity_and_mute();
    puts("PASS: P9H4b source-locked manual Bloom petal/grow/core audio");
    return 0;
}
