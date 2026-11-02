#include <assert.h>
#include <stdio.h>

#include "apsis_runtime.h"

int main(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint64_t remaining = 60u * 44100u;
    uint8_t i;

    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 0u, 0x5a17c0deu);
    apsis_keep_bank_reset(&bank);
    apsis_chain_sequence_reset(&chain);
    apsis_mother_pending_reset(&mother);
    apsis_bloom_voicing_reset(&bloom);

    s.clock.n = 3u;
    s.clock.bpm = 180u;
    s.clock.division_num = 1u;
    s.clock.division_den = 4u;
    s.clock.swing_q15 = APSIS_Q15_MILLI(350);
    s.clock.main_mask = 0x7u;
    s.clock.snap_mask = 0x7u;
    s.clock.inner_mask = 0x7u;
    s.clock.ghost_q15 = APSIS_Q15_ONE;
    s.clock.step_one_resync = 1u;

    s.string.rhythm = APSIS_STRING_RHYTHM_PIZZ;
    s.string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s.string.rhythm_every = 1u;
    s.string.pulse_diag = APSIS_STRING_PULSE_FULL;

    s.spiral.attractor_rate = 4u;
    s.spiral.route_count = 3u;
    s.spiral.route[0] = 0u;
    s.spiral.route[1] = 4u;
    s.spiral.route[2] = 7u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = 0x00ffffffu;
    for (i = 0u; i < APSIS_ATTRACTOR_STEPS; ++i)
        s.spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(900);

    s.bloom.cycles = 1u;
    s.bloom.rate = 4u;
    s.bloom.trigger_step = 0u;
    s.bloom.delay_ms = 0u;
    s.bloom.depth = APSIS_BLOOM_DEPTH_NINTH;
    s.bloom.unfurl_ms = 3600u;

    apsis_runtime_start(&r);
    while (remaining) {
        /* 512 frames intentionally exceeds the common emulator block size so
         * the capacity proof includes a small scheduling-burst margin. */
        uint32_t frames = remaining > 512u ? 512u : (uint32_t)remaining;
        apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom,
                              frames, 0, 0);
        remaining -= frames;
    }

    assert(r.queue_overflowed == 0u);
    assert(r.scheduler_starved == 0u);
    assert(r.late_slot_overflowed == 0u);
    assert(r.spiral_slot_high_water < APSIS_RUNTIME_SPIRAL_SLOTS);
    assert(r.bloom_slot_high_water < APSIS_RUNTIME_BLOOM_SLOTS);
    assert(r.spiral_occurrence_count > 2000u);
    assert(r.bloom_occurrence_count > 500u);

    printf("PASS: APSIS M3c extreme burst (queue=%u/%u, spiral-slots=%u/%u, bloom-slots=%u/%u)\n",
           (unsigned)r.queue_high_water, (unsigned)APSIS_TRANSPORT_QUEUE_CAP,
           (unsigned)r.spiral_slot_high_water, (unsigned)APSIS_RUNTIME_SPIRAL_SLOTS,
           (unsigned)r.bloom_slot_high_water, (unsigned)APSIS_RUNTIME_BLOOM_SLOTS);
    return 0;
}
