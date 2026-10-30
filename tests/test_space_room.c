#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "apsis_state.h"
#include "apsis_space_room.h"

#define BLOCK 256u
#define ROOM_FRAMES (6u * 44100u)

typedef struct {
    int16_t *history;
    int16_t *echo;
    int16_t *fdn;
} SpaceStorage;

typedef struct {
    uint32_t hash;
    int32_t peak;
    uint64_t late_energy;
    uint32_t first_nonzero;
} RoomStats;

static int16_t clip16(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32767) return -32767;
    return (int16_t)x;
}

static uint32_t hash_sample(uint32_t h, int16_t x)
{
    uint16_t u = (uint16_t)x;
    h ^= (uint8_t)(u & 255u); h *= 16777619u;
    h ^= (uint8_t)(u >> 8); h *= 16777619u;
    return h;
}

static SpaceStorage storage_alloc(void)
{
    SpaceStorage s;
    s.history = malloc(sizeof(int16_t) * APSIS_SPACE_HISTORY_STORAGE_SAMPLES);
    s.echo = malloc(sizeof(int16_t) * APSIS_SPACE_ECHO_STORAGE_SAMPLES);
    s.fdn = malloc(sizeof(int16_t) * APSIS_SPACE_FDN_STORAGE_SAMPLES);
    assert(s.history && s.echo && s.fdn);
    return s;
}

static void storage_free(SpaceStorage *s)
{
    if (!s) return;
    free(s->history);
    free(s->echo);
    free(s->fdn);
    s->history = 0;
    s->echo = 0;
    s->fdn = 0;
}

static void fill16(int16_t *p, uint32_t n, int16_t value)
{
    uint32_t i;
    for (i = 0u; i < n; ++i) p[i] = value;
}

static void storage_fill(SpaceStorage *s, int16_t value)
{
    fill16(s->history, APSIS_SPACE_HISTORY_STORAGE_SAMPLES, value);
    fill16(s->echo, APSIS_SPACE_ECHO_STORAGE_SAMPLES, value);
    fill16(s->fdn, APSIS_SPACE_FDN_STORAGE_SAMPLES, value);
}

static void bind_room(ApsisSpaceRoom *room, SpaceStorage *s)
{
    assert(apsis_space_room_bind(room,
        s->history, APSIS_SPACE_HISTORY_STORAGE_SAMPLES,
        s->echo, APSIS_SPACE_ECHO_STORAGE_SAMPLES,
        s->fdn, APSIS_SPACE_FDN_STORAGE_SAMPLES));
}

static RoomStats render_impulse(SpaceStorage *storage, int16_t garbage,
                                const ApsisSpaceState *space, uint32_t frames)
{
    ApsisSpaceRoom room;
    int32_t input[BLOCK * 2u];
    int32_t output[BLOCK * 2u];
    RoomStats stats = {2166136261u, 0, 0u, UINT32_MAX};
    uint32_t done = 0u;
    uint32_t late_start = frames > 44100u ? frames - 44100u : 0u;

    storage_fill(storage, garbage);
    bind_room(&room, storage);

    while (done < frames) {
        uint32_t n = frames - done;
        uint32_t i;
        if (n > BLOCK) n = BLOCK;
        for (i = 0u; i < n; ++i) {
            int32_t impulse = (done == 0u && i == 0u) ? 32767 : 0;
            input[i * 2u] = impulse;
            input[i * 2u + 1u] = impulse;
            output[i * 2u] = 0;
            output[i * 2u + 1u] = 0;
        }
        apsis_space_room_render(&room, space, input, output, n);
        for (i = 0u; i < n; ++i) {
            int16_t l = clip16(output[i * 2u]);
            int16_t r = clip16(output[i * 2u + 1u]);
            int32_t al = l < 0 ? -(int32_t)l : l;
            int32_t ar = r < 0 ? -(int32_t)r : r;
            if (al > stats.peak) stats.peak = al;
            if (ar > stats.peak) stats.peak = ar;
            if (stats.first_nonzero == UINT32_MAX && (l || r))
                stats.first_nonzero = done + i;
            stats.hash = hash_sample(stats.hash, l);
            stats.hash = hash_sample(stats.hash, r);
            if (done + i >= late_start)
                stats.late_energy += (uint64_t)al + (uint64_t)ar;
        }
        done += n;
    }
    return stats;
}

static void test_storage_layout_and_no_clear(void)
{
    ApsisSpaceRoom room;
    ApsisState state;
    SpaceStorage s = storage_alloc();
    int32_t input[BLOCK * 2u] = {0};
    int32_t output[BLOCK * 2u];
    uint32_t done = 0u;

    assert(APSIS_SPACE_HISTORY_STORAGE_BYTES == 24696u);
    assert(APSIS_SPACE_ECHO_STORAGE_BYTES == 211680u);
    assert(APSIS_SPACE_FDN_STORAGE_BYTES == 38340u);
    assert(APSIS_SPACE_EXTERNAL_STORAGE_BYTES == 274716u);

    storage_fill(&s, (int16_t)0x4567);
    assert(!apsis_space_room_bind(&room,
        s.history, APSIS_SPACE_HISTORY_STORAGE_SAMPLES - 1u,
        s.echo, APSIS_SPACE_ECHO_STORAGE_SAMPLES,
        s.fdn, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    bind_room(&room, &s);
    assert(s.history[0] == (int16_t)0x4567);
    assert(s.echo[0] == (int16_t)0x4567);
    assert(s.fdn[0] == (int16_t)0x4567);

    apsis_state_default(&state);
    while (done < 6000u) {
        uint32_t n = 6000u - done;
        uint32_t i;
        if (n > BLOCK) n = BLOCK;
        for (i = 0u; i < n * 2u; ++i) output[i] = 0;
        apsis_space_room_render(&room, &state.space, input, output, n);
        for (i = 0u; i < n * 2u; ++i) assert(output[i] == 0);
        done += n;
    }
    storage_free(&s);
}

static void test_early_reflections_are_direct(void)
{
    ApsisState state;
    SpaceStorage s = storage_alloc();
    RoomStats early;
    apsis_state_default(&state);
    state.space.mix_q15 = 0u;
    state.space.echo_feedback_q15 = 0u;
    early = render_impulse(&s, (int16_t)0x2222, &state.space, 20000u);

    /* 21 ms = 926.1 samples. History validity makes the first fractional tap
     * observable on the next complete sample; it must not arrive early. */
    assert(early.first_nonzero >= 926u && early.first_nonzero <= 928u);
    assert(early.peak > 0);
    assert(early.late_energy == 0u);
    storage_free(&s);
}

static void test_room_determinism_and_controls(void)
{
    ApsisState state;
    SpaceStorage s = storage_alloc();
    RoomStats a, b, no_echo, dark;
    apsis_state_default(&state);

    a = render_impulse(&s, (int16_t)0x1111, &state.space, ROOM_FRAMES);
    b = render_impulse(&s, (int16_t)0x6a5a, &state.space, ROOM_FRAMES);
    assert(a.hash == b.hash);
    assert(a.peak == b.peak);
    assert(a.late_energy == b.late_energy);
    assert(a.first_nonzero == b.first_nonzero);
    assert(a.peak > 0);
    assert(a.late_energy > 0u);

    state.space.echo_feedback_q15 = 0u;
    no_echo = render_impulse(&s, (int16_t)0x3333, &state.space, ROOM_FRAMES);
    assert(no_echo.hash != a.hash);

    apsis_state_default(&state);
    state.space.damping_hz = 2200u;
    dark = render_impulse(&s, (int16_t)0x4444, &state.space, ROOM_FRAMES);
    assert(dark.hash != a.hash);
    assert(dark.late_energy != a.late_energy);

    printf("PASS: P6 room default peak=%d hash=%08x late=%llu first=%u\n",
           a.peak, a.hash, (unsigned long long)a.late_energy, a.first_nonzero);
    printf("PASS: P6 room controls noecho=%08x dark=%08x\n",
           no_echo.hash, dark.hash);
    storage_free(&s);
}

int main(void)
{
    test_storage_layout_and_no_clear();
    test_early_reflections_are_direct();
    test_room_determinism_and_controls();
    puts("PASS: APSIS P6 shared room front-end/topology host engine");
    return 0;
}
