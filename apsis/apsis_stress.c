#include "apsis_stress.h"

void apsis_state_stress(ApsisState *s)
{
    uint8_t i;
    apsis_state_default(s);

    s->clock.bpm = 180u;
    s->clock.division_num = 1u;
    s->clock.division_den = 4u;
    s->clock.main_mask = 0x0FFFu;
    s->clock.inner_mask = 0x0FFFu;
    s->clock.snap_mask = 0x0FFFu;
    s->clock.ghost_q15 = APSIS_Q15_ONE;

    s->string.foundation = 1u;
    s->string.rhythm = APSIS_STRING_RHYTHM_PIZZ;
    s->string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s->string.rhythm_every = 1u;
    s->string.rhythm_level_q15 = APSIS_Q15_ONE;
    s->string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    s->string.filter_attractor_every = APSIS_STRING_ATTRACTOR_HALF;
    s->string.filter_attractor_length = 16u;
    s->string.filter_attractor_gates = 0xFFFFu;
    s->string.filter_attractor_strum_q15 = APSIS_Q15_ONE;
    s->string.filter_attractor_strum_level_q15 = APSIS_Q15_ONE;

    for (i = 0u; i < 16u; ++i) s->spiral.route[i] = (uint8_t)((i * 5u) % 36u);
    s->spiral.route_count = 16u;
    s->spiral.attractor_rate = 4u;
    s->spiral.attractor_density_q15 = APSIS_Q15_ONE;
    s->spiral.attractor_gates = 0x00FFFFFFu;

    s->bloom.cycles = 1u;
    s->bloom.rate = 4u;
    s->bloom.depth = APSIS_BLOOM_DEPTH_NINTH;
    s->bloom.length_ms = 6000u;
    s->bloom.unfurl_ms = 3600u;
    s->gravity.voices = 5u;

    s->space.decay_ms = 12000u;
}

void apsis_state_demo(ApsisState *s)
{
    static const uint8_t route[8] = {12u, 19u, 24u, 16u, 14u, 21u, 26u, 19u};
    uint8_t i;
    apsis_state_default(s);
    for (i = 0u; i < 8u; ++i) s->spiral.route[i] = route[i];
    s->spiral.route_count = 8u;
    s->string.rhythm = APSIS_STRING_RHYTHM_PIZZ;
    s->string.rhythm_every = 2u;
    s->string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
#ifdef APSIS_DEMO_SOLO   /* one source alone: 0 String, 1 Drift, 2 Clock, 3 Spiral, 4 Bloom */
    s->mother.mute_mask = (uint8_t)(0x1Fu & ~(1u << APSIS_DEMO_SOLO));
#endif
}
