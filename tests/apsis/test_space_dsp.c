#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "apsis_space_dsp.h"

#define BLOCK 256u
#define DEFAULT_FRAMES ((8400u + 1000u) * 44100u / 1000u)
#define SHORT_FRAMES (6000u * 44100u / 1000u)

#define P6_FDN_DEFAULT_PEAK 217
#define P6_FDN_DEFAULT_HASH 0xa190ca86u
#define P6_FDN_DEFAULT_LATE_ENERGY 2646000ull
#define P6_FDN_DECAY4_LATE_ENERGY 1455300ull
#define P6_FDN_DECAY12_LATE_ENERGY 3507773ull
#define P6_FDN_DARK_HASH 0x7d4c13e3u
#define P6_FDN_BRIGHT_HASH 0x7e1a4d9eu

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

typedef struct {
    uint32_t hash;
    int32_t peak;
    uint64_t late_energy;
} RenderStats;

static void fill_storage(int16_t *storage, int16_t value)
{
    uint32_t i;
    for (i = 0u; i < APSIS_SPACE_FDN_STORAGE_SAMPLES; ++i) storage[i] = value;
}

static RenderStats render_impulse(int16_t *storage, int16_t garbage,
                                  uint16_t decay_ms, uint16_t damping_hz,
                                  uint32_t frames)
{
    ApsisSpaceFdn fdn;
    int32_t in[BLOCK];
    int32_t out[BLOCK * 2u];
    RenderStats stats = {2166136261u, 0, 0u};
    uint32_t done = 0u;
    uint32_t late_start = frames > 44100u ? frames - 44100u : 0u;

    fill_storage(storage, garbage);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    while (done < frames) {
        uint32_t n = frames - done;
        uint32_t i;
        if (n > BLOCK) n = BLOCK;
        for (i = 0u; i < n; ++i) {
            in[i] = (done == 0u && i == 0u) ? 32767 : 0;
            out[i * 2u] = 0;
            out[i * 2u + 1u] = 0;
        }
        apsis_space_fdn_render(&fdn, in, out, n, decay_ms, damping_hz);
        for (i = 0u; i < n; ++i) {
            int16_t l = clip16(out[i * 2u]);
            int16_t r = clip16(out[i * 2u + 1u]);
            int32_t al = l < 0 ? -(int32_t)l : l;
            int32_t ar = r < 0 ? -(int32_t)r : r;
            if (al > stats.peak) stats.peak = al;
            if (ar > stats.peak) stats.peak = ar;
            stats.hash = hash_sample(stats.hash, l);
            stats.hash = hash_sample(stats.hash, r);
            if (done + i >= late_start)
                stats.late_energy += (uint64_t)al + (uint64_t)ar;
        }
        done += n;
    }
    return stats;
}

static void test_layout_and_no_implicit_clear(void)
{
    ApsisSpaceFdn fdn;
    int16_t *storage = malloc(sizeof(int16_t) * APSIS_SPACE_FDN_STORAGE_SAMPLES);
    int32_t in[BLOCK] = {0};
    int32_t out[BLOCK * 2u];
    uint32_t i, done = 0u;
    assert(storage);

    assert(!apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES - 1u));
    fill_storage(storage, (int16_t)0x4567);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(storage[0] == (int16_t)0x4567); /* bind/reset must not memset NOLOAD */
    assert(fdn.line[7].offset + fdn.line[7].capacity == APSIS_SPACE_FDN_STORAGE_SAMPLES);

    /* Garbage must remain acoustically invisible until corresponding history
     * has really been written. 1800 frames is shorter than the shortest
     * modulated line, so zero input must yield exact zero output. */
    while (done < 1800u) {
        uint32_t n = 1800u - done;
        if (n > BLOCK) n = BLOCK;
        for (i = 0u; i < n * 2u; ++i) out[i] = 0;
        apsis_space_fdn_render(&fdn, in, out, n, 8400u, 6200u);
        for (i = 0u; i < n * 2u; ++i) assert(out[i] == 0);
        done += n;
    }
    free(storage);
}

static void test_deterministic_garbage_independence(void)
{
    int16_t *storage = malloc(sizeof(int16_t) * APSIS_SPACE_FDN_STORAGE_SAMPLES);
    RenderStats a, b;
    assert(storage);
    a = render_impulse(storage, (int16_t)0x1111, 8400u, 6200u, DEFAULT_FRAMES);
    b = render_impulse(storage, (int16_t)0x6a5a, 8400u, 6200u, DEFAULT_FRAMES);
    assert(a.hash == b.hash);
    assert(a.peak == b.peak);
    assert(a.late_energy == b.late_energy);
    assert(a.peak == P6_FDN_DEFAULT_PEAK);
    assert(a.hash == P6_FDN_DEFAULT_HASH);
    assert(a.late_energy == P6_FDN_DEFAULT_LATE_ENERGY);
    printf("PASS: P6 FDN default peak=%d hash=%08x late=%llu\n",
           a.peak, a.hash, (unsigned long long)a.late_energy);
    free(storage);
}

static void test_decay_and_damping_controls(void)
{
    int16_t *storage = malloc(sizeof(int16_t) * APSIS_SPACE_FDN_STORAGE_SAMPLES);
    RenderStats short_decay, long_decay, dark, bright;
    assert(storage);

    short_decay = render_impulse(storage, (int16_t)0x2222, 4000u, 6200u, SHORT_FRAMES);
    long_decay = render_impulse(storage, (int16_t)0x3333, 12000u, 6200u, SHORT_FRAMES);
    assert(short_decay.late_energy == P6_FDN_DECAY4_LATE_ENERGY);
    assert(long_decay.late_energy == P6_FDN_DECAY12_LATE_ENERGY);
    assert(long_decay.late_energy > short_decay.late_energy * 2u);

    dark = render_impulse(storage, (int16_t)0x4444, 8400u, 2200u, SHORT_FRAMES);
    bright = render_impulse(storage, (int16_t)0x5555, 8400u, 6800u, SHORT_FRAMES);
    assert(dark.hash == P6_FDN_DARK_HASH);
    assert(bright.hash == P6_FDN_BRIGHT_HASH);
    assert(dark.hash != bright.hash);
    assert(dark.peak > 0 && bright.peak > 0);

    printf("PASS: P6 FDN controls decay4=%llu decay12=%llu dark=%08x bright=%08x\n",
           (unsigned long long)short_decay.late_energy,
           (unsigned long long)long_decay.late_energy,
           dark.hash, bright.hash);
    free(storage);
}

int main(void)
{
    assert(APSIS_SPACE_FDN_STORAGE_SAMPLES == 19170u);
    assert(APSIS_SPACE_FDN_STORAGE_BYTES == 38340u);
    test_layout_and_no_implicit_clear();
    test_deterministic_garbage_independence();
    test_decay_and_damping_controls();
    puts("PASS: APSIS P6 selected fixed-point FDN core");
    return 0;
}
