#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_drift.h"
#include "apsis_spiral_audio.h"

static void test_default_pair(void)
{
    ApsisState s;
    ApsisSpiralAcousticPlan p;
    const uint64_t interval_q32 = (uint64_t)18375u << 32; /* 72 BPM, 1/8 */

    apsis_state_default(&s);
    apsis_spiral_audio_plan(&s, APSIS_Q15_MILLI(820), interval_q32, 2u, &p);

    assert(p.valid == 1u);
    assert(p.voice == APSIS_SPIRAL_THREAD);
    assert(p.energy_q15 == APSIS_Q15_MILLI(820));
    assert(p.duration_samples == 5840u);
    assert(p.drift_delta_millihz == apsis_drift_audible_delta_millihz(&s.drift));
    assert(p.drift_delta_millihz > 1u);
    assert(p.width_q15 == APSIS_Q15_MILLI(640));
    assert(p.leg_count == 2u);

    assert(p.leg[0].start_offset_samples == 0u);
    assert(p.leg[0].duration_samples == 5840u);
    assert(p.leg[0].gain_q15 == 1522u);
    assert(p.leg[0].pan_q15 == -7130);
    assert(p.leg[0].drift_sign == -1 && p.leg[0].shadow == 0u);

    assert(p.leg[1].start_offset_samples == 353u); /* 8 ms at 44.1 kHz */
    assert(p.leg[1].duration_samples == 5139u);
    assert(p.leg[1].gain_q15 == 883u);
    assert(p.leg[1].pan_q15 == 7130);
    assert(p.leg[1].drift_sign == 1 && p.leg[1].shadow == 1u);
}

static void test_drift_off_single_leg(void)
{
    ApsisState s;
    ApsisSpiralAcousticPlan p;
    const uint64_t interval_q32 = (uint64_t)18375u << 32;

    apsis_state_default(&s);
    s.drift.sound = 0u;
    apsis_spiral_audio_plan(&s, APSIS_Q15_MILLI(820), interval_q32, 2u, &p);

    assert(p.valid == 1u);
    assert(p.drift_delta_millihz == 0u);
    assert(p.leg_count == 1u);
    assert(p.leg[0].gain_q15 == 1644u);
    assert(p.leg[0].pan_q15 == 0);
    assert(p.leg[0].drift_sign == 0);
    assert(p.leg[0].shadow == 0u);
}

static void test_source_clamps_and_floor(void)
{
    ApsisState s;
    ApsisSpiralAcousticPlan p;
    uint64_t short_interval_q32;

    apsis_state_default(&s);
    s.spiral.voice = APSIS_SPIRAL_HALO;
    s.spiral.note_length_q15 = APSIS_Q15_MILLI(1350);
    s.spiral.level_q15 = APSIS_Q15_MILLI(1250);
    s.drift.width_q15 = APSIS_Q15_ONE;

    /* Shorter than the legal minimum 180-BPM/max-swing pulse. At 4x this
     * guarantees the source 110 ms raw-duration floor is the active branch. */
    short_interval_q32 = (uint64_t)2000u << 32;
    apsis_spiral_audio_plan(&s, APSIS_Q15_ONE, short_interval_q32, 4u, &p);

    assert(p.valid == 1u);
    assert(p.voice == APSIS_SPIRAL_HALO);
    assert(p.energy_q15 == APSIS_Q15_ONE);
    assert(p.duration_samples == 6549u); /* 110 ms * 1.35 */
    assert(p.leg_count == 2u);
    assert(p.leg[0].pan_q15 == -(int16_t)APSIS_Q15_MILLI(340));
    assert(p.leg[1].pan_q15 == (int16_t)APSIS_Q15_MILLI(340));
}

static void test_invalid_and_muted(void)
{
    ApsisState s;
    ApsisSpiralAcousticPlan p;
    apsis_state_default(&s);

    apsis_spiral_audio_plan(&s, APSIS_Q15_ONE, 0u, 2u, &p);
    assert(p.valid == 0u);

    s.spiral.sound = 0u;
    apsis_spiral_audio_plan(&s, APSIS_Q15_ONE, (uint64_t)1000u << 32, 2u, &p);
    assert(p.valid == 0u);
}

int main(void)
{
    test_default_pair();
    test_drift_off_single_leg();
    test_source_clamps_and_floor();
    test_invalid_and_muted();
    puts("PASS: APSIS M4f source-derived Spiral acoustic planning");
    return 0;
}
