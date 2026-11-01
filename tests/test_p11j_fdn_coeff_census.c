#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apsis_space_room.h"

typedef struct {
    uint64_t fdn_render_calls;
    uint64_t feedback_gain_calls;
    uint64_t decay_change_events;
    uint64_t damping_change_events;
    uint32_t decay_unique;
    uint32_t damping_unique;
    uint16_t previous_decay;
    uint16_t previous_damping;
    uint8_t have_previous;
    uint8_t seen_decay[12001];
    uint8_t seen_damping[6801];
} P11jProbe;

static P11jProbe g_probe;

void p11j_probe_feedback_gain(uint16_t delay_samples, uint16_t decay_ms)
{
    (void)delay_samples;
    assert(decay_ms >= 4000u && decay_ms <= 12000u);
    g_probe.feedback_gain_calls++;
}

void p11j_probe_fdn_render(uint16_t decay_ms, uint16_t damping_hz)
{
    assert(decay_ms >= 4000u && decay_ms <= 12000u);
    assert(damping_hz >= 2200u && damping_hz <= 6800u);
    g_probe.fdn_render_calls++;
    if (!g_probe.seen_decay[decay_ms]) {
        g_probe.seen_decay[decay_ms] = 1u;
        g_probe.decay_unique++;
    }
    if (!g_probe.seen_damping[damping_hz]) {
        g_probe.seen_damping[damping_hz] = 1u;
        g_probe.damping_unique++;
    }
    if (g_probe.have_previous) {
        if (decay_ms != g_probe.previous_decay) g_probe.decay_change_events++;
        if (damping_hz != g_probe.previous_damping) g_probe.damping_change_events++;
    }
    g_probe.previous_decay = decay_ms;
    g_probe.previous_damping = damping_hz;
    g_probe.have_previous = 1u;
}

static void reset_probe(void)
{
    memset(&g_probe, 0, sizeof(g_probe));
}

typedef enum {
    P11J_STEADY = 0,
    P11J_ONE_SWEEP = 1,
    P11J_ADVERSARIAL = 2
} P11jScenario;

static void run_scenario(const char *name, P11jScenario scenario)
{
    enum { CALLBACKS = 256, ROOM_FRAMES = 64 };
    ApsisSpaceRoom room = {0};
    ApsisSpaceState state = {0};
    int16_t *history = calloc(APSIS_SPACE_HISTORY_STORAGE_SAMPLES, sizeof(*history));
    int16_t *echo = calloc(APSIS_SPACE_ECHO_STORAGE_SAMPLES, sizeof(*echo));
    int16_t *fdn = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*fdn));
    int32_t input[ROOM_FRAMES * 2u] = {0};
    int32_t output[ROOM_FRAMES * 2u];
    uint32_t callback;
    uint64_t cache_epochs, cached_gain_calls, saved_gain_calls;

    assert(history && echo && fdn);
    assert(apsis_space_room_bind(&room,
                                 history, APSIS_SPACE_HISTORY_STORAGE_SAMPLES,
                                 echo, APSIS_SPACE_ECHO_STORAGE_SAMPLES,
                                 fdn, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    state.mix_q15 = 16384u;
    state.predelay_ms = 40u;
    state.echo_time_ms = 300u;
    state.echo_feedback_q15 = 8000u;
    state.decay_ms = scenario == P11J_ONE_SWEEP ? 4000u : 8000u;
    state.damping_hz = scenario == P11J_ONE_SWEEP ? 2200u : 4500u;

    reset_probe();
    for (callback = 0u; callback < CALLBACKS; ++callback) {
        if (scenario == P11J_ONE_SWEEP && callback == 64u) {
            state.decay_ms = 12000u;
            state.damping_hz = 6800u;
        } else if (scenario == P11J_ADVERSARIAL) {
            state.decay_ms = (callback & 1u) ? 12000u : 4000u;
            state.damping_hz = (callback & 1u) ? 6800u : 2200u;
        }
        memset(output, 0, sizeof(output));
        apsis_space_room_render(&room, &state, input, output, ROOM_FRAMES);
    }

    assert(g_probe.fdn_render_calls == CALLBACKS);
    assert(g_probe.feedback_gain_calls == CALLBACKS * APSIS_SPACE_FDN_LINES);
    cache_epochs = 1u + g_probe.decay_change_events;
    cached_gain_calls = cache_epochs * APSIS_SPACE_FDN_LINES;
    assert(cached_gain_calls <= g_probe.feedback_gain_calls);
    saved_gain_calls = g_probe.feedback_gain_calls - cached_gain_calls;

    printf("P11J scenario=%s renders=%llu gain_calls=%llu decay_changes=%llu decay_unique=%u "
           "damping_changes=%llu damping_unique=%u cached_gain_calls=%llu saved_gain_calls=%llu "
           "divisions=%llu wide_multiplies=%llu\n",
           name,
           (unsigned long long)g_probe.fdn_render_calls,
           (unsigned long long)g_probe.feedback_gain_calls,
           (unsigned long long)g_probe.decay_change_events,
           g_probe.decay_unique,
           (unsigned long long)g_probe.damping_change_events,
           g_probe.damping_unique,
           (unsigned long long)cached_gain_calls,
           (unsigned long long)saved_gain_calls,
           (unsigned long long)g_probe.feedback_gain_calls,
           (unsigned long long)(g_probe.feedback_gain_calls * 3u));

    if (scenario == P11J_STEADY) {
        assert(g_probe.decay_change_events == 0u);
        assert(cached_gain_calls == APSIS_SPACE_FDN_LINES);
        assert(saved_gain_calls == (CALLBACKS - 1u) * APSIS_SPACE_FDN_LINES);
    }

    free(history);
    free(echo);
    free(fdn);
}

int main(void)
{
    run_scenario("steady", P11J_STEADY);
    run_scenario("one_sweep", P11J_ONE_SWEEP);
    run_scenario("adversarial", P11J_ADVERSARIAL);
    puts("PASS: P11J FDN coefficient recomputation census");
    return 0;
}
