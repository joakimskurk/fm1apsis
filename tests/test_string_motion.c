#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_motion.h"

static uint32_t freq_mhz(const ApsisStringMotionTrack *t, uint16_t i)
{
    uint32_t off = (uint32_t)i * 3u;
    uint32_t chz;
    assert(t && i < t->count);
    chz = (uint32_t)t->frequency_centi_hz[off] |
          ((uint32_t)t->frequency_centi_hz[off + 1u] << 8) |
          ((uint32_t)t->frequency_centi_hz[off + 2u] << 16);
    return chz * 10u;
}

static uint32_t track_hash(const ApsisStringMotionTrack *t)
{
    uint32_t h = 2166136261u;
    uint16_t i;
    for (i = 0u; i < t->count; ++i) {
        uint32_t f = freq_mhz(t, i);
        h = (h ^ t->phase_q15[i]) * 16777619u;
        h = (h ^ t->split_q15[i]) * 16777619u;
        h = (h ^ f) * 16777619u;
    }
    return h ^ t->count;
}

static void default_state(ApsisState *s)
{
    apsis_state_default(s);
    s->clock.n = 4u;
    s->string.surface_mode = APSIS_STRING_DUAL;
    s->string.linked = 0u;
    s->string.motion_loop_length = 2u;
    s->string.pulse_diag = APSIS_STRING_PULSE_FULL;
}

static void record_two_cycle_loop(ApsisStringMotionRuntime *m, ApsisState *s,
                                  uint8_t target)
{
    uint64_t at = 0u;
    uint8_t cycle, step;
    assert(apsis_string_motion_arm(m, s, target));
    apsis_string_motion_pulse(m, s, 0u, at, 1000u);
    assert(m->recording);
    for (cycle = 0u; cycle < 2u; ++cycle) {
        for (step = cycle ? 1u : 1u; step < 4u; ++step) {
            at += 1000u;
            apsis_string_motion_manual_split(
                m, s, APSIS_SIDE_RIGHT,
                (uint16_t)(APSIS_STRING_SPLIT_MID_Q15 + 500u + cycle * 200u + step * 60u),
                s->string.linked, at);
            apsis_string_motion_pulse(m, s, step, at, 1000u);
        }
        at += 1000u;
        apsis_string_motion_pulse(m, s, 0u, at, 1000u);
        if (!m->recording) break;
    }
    assert(!m->recording);
    assert(s->string.motion_loop_play);
}

static void test_target_resolution(void)
{
    ApsisState s;
    default_state(&s);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_LEFT) ==
           APSIS_STRING_MOTION_RIGHT);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 1u;
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_LEFT) ==
           APSIS_STRING_MOTION_BOTH);
    s.string.linked = 0u;
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_LEFT) ==
           APSIS_STRING_MOTION_LEFT);
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_RIGHT) ==
           APSIS_STRING_MOTION_RIGHT);
}

static void test_quantized_start_finish_and_frozen_pitch(void)
{
    ApsisState s;
    ApsisStringMotionRuntime m;
    uint32_t f0, f1;
    default_state(&s);
    apsis_string_motion_reset(&m);
    assert(apsis_string_motion_storage_bytes() < 26000u);

    assert(apsis_string_motion_arm(&m, &s, APSIS_STRING_MOTION_RIGHT));
    apsis_string_motion_pulse(&m, &s, 2u, 2000u, 1000u);
    assert(m.armed && !m.recording);
    apsis_string_motion_pulse(&m, &s, 0u, 4000u, 1000u);
    assert(!m.armed && m.recording);
    assert(m.right.count >= 1u);
    f0 = freq_mhz(&m.right, (uint16_t)(m.right.count - 1u));

    /* Gravity may change underneath the recording, but the pitch map was
     * frozen at the record-start 1. Same physical position must stay the same
     * recorded melody pitch. */
    s.gravity.degree = 6u;
    apsis_string_motion_manual_split(&m, &s, APSIS_SIDE_RIGHT,
                                     s.string.right_split_q15, 0u, 5500u);
    assert(apsis_string_motion_capture(&m, &s, 5500u, 1u));
    f1 = freq_mhz(&m.right, (uint16_t)(m.right.count - 1u));
    assert(f0 == f1);

    apsis_string_motion_pulse(&m, &s, 1u, 5000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 2u, 6000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 3u, 7000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 0u, 8000u, 1000u);
    assert(m.recording);
    apsis_string_motion_pulse(&m, &s, 1u, 9000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 2u, 10000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 3u, 11000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 0u, 12000u, 1000u);
    assert(!m.recording);
    assert(s.string.motion_loop_play);
    assert(m.right.count > 1u && m.right.count <= APSIS_STRING_MOTION_LOOP_MAX);
    assert(m.right.phase_q15[m.right.count - 1u] == 32767u);
}

static void test_capture_throttle_and_ownership(void)
{
    ApsisState s;
    ApsisStringMotionRuntime m;
    uint16_t before;
    default_state(&s);
    apsis_string_motion_reset(&m);
    s.string.motion_loop_length = 1u;
    assert(apsis_string_motion_arm(&m, &s, APSIS_STRING_MOTION_BOTH));
    apsis_string_motion_pulse(&m, &s, 0u, 0u, 2000u);
    before = m.right.count;
    apsis_string_motion_manual_split(&m, &s, APSIS_SIDE_RIGHT,
                                     (uint16_t)(s.string.right_split_q15 + 100u), 0u, 1000u);
    assert(m.right.count == before); /* < ceil(1/32 s) */
    apsis_string_motion_manual_split(&m, &s, APSIS_SIDE_RIGHT,
                                     (uint16_t)(s.string.right_split_q15 + 100u), 0u, 1500u);
    assert(m.right.count > before);

    before = m.owned_right_q15;
    s.string.left_split_q15 = APSIS_STRING_SPLIT_MIN_Q15;
    s.string.right_split_q15 = APSIS_STRING_SPLIT_MAX_Q15;
    s.string.split_q15 = s.string.right_split_q15;
    apsis_string_motion_restore_owned(&m, &s);
    assert(s.string.right_split_q15 == before);
    assert(s.string.left_split_q15 == m.owned_left_q15);
}

static void test_dual_free_overdub_preserves_other_side(void)
{
    ApsisState s;
    ApsisStringMotionRuntime m;
    uint32_t right_hash;
    uint64_t at = 0u;
    uint8_t step;
    default_state(&s);
    apsis_string_motion_reset(&m);

    s.string.linked = 1u;
    record_two_cycle_loop(&m, &s, APSIS_STRING_MOTION_BOTH);
    assert(m.left.count > 1u && m.right.count > 1u);
    right_hash = track_hash(&m.right);

    s.string.linked = 0u;
    s.string.motion_loop_length = 1u;
    assert(apsis_string_motion_arm(&m, &s, APSIS_STRING_MOTION_LEFT));
    apsis_string_motion_pulse(&m, &s, 0u, at, 1000u);
    for (step = 1u; step < 4u; ++step) {
        at += 1000u;
        apsis_string_motion_manual_split(
            &m, &s, APSIS_SIDE_LEFT,
            (uint16_t)(APSIS_STRING_SPLIT_MID_Q15 - 500u - step * 80u), 0u, at);
        apsis_string_motion_pulse(&m, &s, step, at, 1000u);
    }
    at += 1000u;
    apsis_string_motion_pulse(&m, &s, 0u, at, 1000u);
    assert(!m.recording);
    assert(track_hash(&m.right) == right_hash);
    assert(m.left.count > 1u);
}

static void test_manual_override_release_and_pulse_audit(void)
{
    ApsisState s;
    ApsisStringMotionRuntime m;
    ApsisStringMotionPlayback p0, p1;
    default_state(&s);
    apsis_string_motion_reset(&m);
    s.string.motion_loop_length = 1u;
    record_two_cycle_loop(&m, &s, APSIS_STRING_MOTION_RIGHT);
    s.string.motion_loop_play = 1u;
    apsis_string_motion_pulse(&m, &s, 1u, 10000u, 2000u);
    assert(apsis_string_motion_playback(&m, &s, 11000u, &p0));
    assert(p0.right.valid);

    apsis_string_motion_manual_split(&m, &s, APSIS_SIDE_RIGHT,
                                     APSIS_STRING_SPLIT_MAX_Q15, 0u, 11200u);
    assert(m.manual_override);
    assert(!apsis_string_motion_playback(&m, &s, 12000u, &p1));
    apsis_string_motion_manual_release(&m);
    assert(!m.manual_override);
    assert(apsis_string_motion_playback(&m, &s, 12000u, &p1));

    s.string.pulse_diag = APSIS_STRING_PULSE_SUSTAIN;
    assert(!apsis_string_motion_playback(&m, &s, 12000u, &p1));
}

static void test_continuous_pitch_flag(void)
{
    ApsisState s;
    ApsisStringMotionRuntime m;
    default_state(&s);
    apsis_string_motion_reset(&m);
    s.string.motion_loop_length = 1u;
    s.string.quantize = 0u;
    assert(apsis_string_motion_arm(&m, &s, APSIS_STRING_MOTION_RIGHT));
    apsis_string_motion_pulse(&m, &s, 0u, 0u, 1000u);
    apsis_string_motion_pulse(&m, &s, 1u, 1000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 2u, 2000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 3u, 3000u, 1000u);
    apsis_string_motion_pulse(&m, &s, 0u, 4000u, 1000u);
    assert(!m.recording);
    assert(s.string.motion_loop_pitch_continuous == 1u);
}

int main(void)
{
    test_target_resolution();
    test_quantized_start_finish_and_frozen_pitch();
    test_capture_throttle_and_ownership();
    test_dual_free_overdub_preserves_other_side();
    test_manual_override_release_and_pulse_audit();
    test_continuous_pitch_flag();
    printf("PASS: Phase 10A String Motion source contract (%u bytes runtime)\n",
           (unsigned)apsis_string_motion_storage_bytes());
    return 0;
}
