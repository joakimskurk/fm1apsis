#include <stdint.h>
#include <stdio.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string_rhythm_audio.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static ApsisTransportEvent rhythm_event(uint32_t pulse, uint8_t mode)
{
    ApsisTransportEvent e = {0};
    e.type = APSIS_EVENT_STRING_RHYTHM;
    e.absolute_pulse = pulse;
    e.clock_step = (uint8_t)(pulse % 12u);
    e.sub_index = mode;
    e.sub_count = 1u;
    return e;
}

static uint8_t count_wave(const ApsisStringTransientPool *pool, uint8_t wave)
{
    uint16_t i;
    uint8_t n = 0u;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (pool->transient[i].active && pool->transient[i].waveform == wave) ++n;
    return n;
}

static int classic_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory memory;
    ApsisStringTransientPool pool;
    ApsisTransportEvent e = rhythm_event(0u, APSIS_STRING_RHYTHM_MOTION);
    uint16_t i;
    uint8_t delayed = 0u;

    apsis_state_default(&s);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_transient_pool_reset(&pool);

    /* Classic ignores BOW/PIZZ/MOTION topology and always uses composite. */
    CHECK(apsis_string_rhythm_trigger_event(&pool, &s, &memory, &e) == 7u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 1u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_TRIANGLE) == 4u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_SINE) == 2u);
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        const ApsisStringTransientVoice *v = &pool.transient[i];
        if (!v->active || v->waveform == APSIS_STRING_DSP_WAVE_BODY) continue;
        if (v->start_delay_samples >= 176u) delayed = 1u;
    }
    CHECK(delayed);
    CHECK(pool.transient_drop_count == 0u);
    return 0;
}

static int dual_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory memory;
    ApsisStringTransientPool pool;
    ApsisTransportEvent left_bow = rhythm_event(0u, APSIS_STRING_RHYTHM_BOW);
    ApsisTransportEvent right_bow = rhythm_event(1u, APSIS_STRING_RHYTHM_BOW);
    ApsisTransportEvent left_pizz = rhythm_event(2u, APSIS_STRING_RHYTHM_PIZZ);
    uint16_t bow_peak, pizz_peak;

    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_transient_pool_reset(&pool);

    CHECK(apsis_string_rhythm_trigger_event(&pool, &s, &memory, &left_bow) == 1u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 1u);
    bow_peak = pool.transient[0].peak_gain_q15;

    apsis_string_transient_pool_reset(&pool);
    apsis_string_pitch_memory_reset(&memory);
    CHECK(apsis_string_rhythm_trigger_event(&pool, &s, &memory, &right_bow) == 6u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 0u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_TRIANGLE) == 4u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_SINE) == 2u);

    apsis_string_transient_pool_reset(&pool);
    apsis_string_pitch_memory_reset(&memory);
    CHECK(apsis_string_rhythm_trigger_event(&pool, &s, &memory, &left_pizz) == 1u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 1u);
    pizz_peak = pool.transient[0].peak_gain_q15;
    CHECK(pizz_peak > bow_peak); /* PIZZ mode scales the selected side 1.05 vs .90. */
    return 0;
}

static int frozen_mode_and_late_state_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory memory;
    ApsisStringTransientPool a, b;
    ApsisTransportEvent e = rhythm_event(1u, APSIS_STRING_RHYTHM_PIZZ);
    uint32_t inc_a = 0u, inc_b = 0u;
    uint16_t i;

    apsis_state_default(&s);
    s.string.rhythm = APSIS_STRING_RHYTHM_OFF; /* current mode no longer matters */
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_transient_pool_reset(&a);
    CHECK(apsis_string_rhythm_trigger_event(&a, &s, &memory, &e) == 6u);

    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (a.transient[i].active) { inc_a = a.transient[i].phase_inc; break; }
    CHECK(inc_a != 0u);

    /* Pitch/surface-side state is re-read when the event binds. */
    apsis_string_set_side_split(&s.string, APSIS_SIDE_RIGHT,
                                APSIS_Q15_MILLI(650), 0u);
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_transient_pool_reset(&b);
    CHECK(apsis_string_rhythm_trigger_event(&b, &s, &memory, &e) == 6u);
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (b.transient[i].active) { inc_b = b.transient[i].phase_inc; break; }
    CHECK(inc_b != 0u && inc_b != inc_a);

    s.string.sound = 0u;
    apsis_string_transient_pool_reset(&b);
    CHECK(apsis_string_rhythm_trigger_event(&b, &s, &memory, &e) == 0u);
    return 0;
}

static int motion_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory memory;
    ApsisStringTransientPool pool;
    ApsisTransportEvent e = rhythm_event(5u, APSIS_STRING_RHYTHM_MOTION);

    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_transient_pool_reset(&pool);
    CHECK(apsis_string_rhythm_trigger_event(&pool, &s, &memory, &e) == 1u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 1u);
    CHECK(pool.transient[0].duration_samples == 30870u); /* 700 ms */
    CHECK(pool.transient[0].attack_left_samples == 2029u); /* 46 ms */
    return 0;
}

int main(void)
{
    CHECK(classic_contract() == 0);
    CHECK(dual_contract() == 0);
    CHECK(frozen_mode_and_late_state_contract() == 0);
    CHECK(motion_contract() == 0);
    puts("PASS: APSIS M4b source-derived String Rhythm event/audio topology");
    return 0;
}
