#include <stdint.h>
#include <stdio.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string_manual_audio.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static uint8_t count_wave(const ApsisStringTransientPool *pool, uint8_t wave)
{
    uint16_t i;
    uint8_t n = 0u;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (pool->transient[i].active && pool->transient[i].waveform == wave) ++n;
    return n;
}

int main(void)
{
    ApsisState s;
    ApsisStringPitchMemory memory;
    ApsisStringTransientPool pool;
    uint16_t i;
    uint8_t delayed = 0u;

    CHECK(APSIS_STRING_MANUAL_KEY_LEVEL_Q15 == APSIS_Q15_MILLI(80));
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_transient_pool_reset(&pool);

    CHECK(apsis_string_manual_trigger(&pool, &s, &memory, APSIS_SIDE_LEFT) == 1u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 1u);

    apsis_string_transient_pool_reset(&pool);
    apsis_string_pitch_memory_reset(&memory);
    CHECK(apsis_string_manual_trigger(&pool, &s, &memory, APSIS_SIDE_RIGHT) == 6u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 0u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_TRIANGLE) == 4u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_SINE) == 2u);

    s.string.surface_mode = APSIS_STRING_CLASSIC;
    apsis_string_transient_pool_reset(&pool);
    apsis_string_pitch_memory_reset(&memory);
    CHECK(apsis_string_manual_trigger(&pool, &s, &memory, APSIS_SIDE_LEFT) == 7u);
    CHECK(count_wave(&pool, APSIS_STRING_DSP_WAVE_BODY) == 1u);
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        const ApsisStringTransientVoice *v = &pool.transient[i];
        if (!v->active || v->waveform == APSIS_STRING_DSP_WAVE_BODY) continue;
        if (v->start_delay_samples >= 176u) delayed = 1u;
    }
    CHECK(delayed);

    s.string.sound = 0u;
    apsis_string_transient_pool_reset(&pool);
    CHECK(apsis_string_manual_trigger(&pool, &s, &memory, APSIS_SIDE_RIGHT) == 0u);
    s.string.sound = 1u;
    s.string.level_q15 = 0u;
    CHECK(apsis_string_manual_trigger(&pool, &s, &memory, APSIS_SIDE_RIGHT) == 0u);

    puts("PASS: P9H4a source-keyboard String manual audio topology");
    return 0;
}
