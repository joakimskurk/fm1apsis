#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include the production implementation so its current static
 * space_room_stereo_read() is the reference authority in this translation
 * unit. P11I must prove the candidate before apsis_space_room.c is edited. */
#include "../apsis/apsis_space_room.c"

typedef struct {
    int32_t left;
    int32_t right;
} P11iStereoPair;

static P11iStereoPair p11i_candidate_stereo_read_pair(
    const int16_t *storage,
    uint32_t capacity_frames,
    uint32_t write_frame,
    uint32_t fill_frames,
    uint32_t delay_q16)
{
    P11iStereoPair out = {0, 0};
    uint32_t needed, span_q16, write_q16, read_q16, i0, i1, frac;
    int32_t a_left, a_right, b_left, b_right;
    int64_t mixed;

    if (!storage || !capacity_frames) return out;
    needed = (delay_q16 + 65535u) >> 16;
    if (!needed || fill_frames < needed) return out;
    span_q16 = capacity_frames << 16;
    write_q16 = write_frame << 16;
    if (write_q16 >= delay_q16) read_q16 = write_q16 - delay_q16;
    else read_q16 = span_q16 - (delay_q16 - write_q16);
    if (read_q16 >= span_q16) read_q16 -= span_q16;
    i0 = read_q16 >> 16;
    frac = read_q16 & 65535u;
    i1 = i0 + 1u;
    if (i1 >= capacity_frames) i1 = 0u;

    a_left = storage[i0 * 2u];
    a_right = storage[i0 * 2u + 1u];
    if (!frac) {
        out.left = a_left;
        out.right = a_right;
        return out;
    }

    b_left = storage[i1 * 2u];
    b_right = storage[i1 * 2u + 1u];
    mixed = (int64_t)a_left * (65536u - frac) + (int64_t)b_left * frac;
    out.left = (int32_t)(mixed >> 16);
    mixed = (int64_t)a_right * (65536u - frac) + (int64_t)b_right * frac;
    out.right = (int32_t)(mixed >> 16);
    return out;
}

static void check_case(const int16_t *storage,
                       uint32_t capacity_frames,
                       uint32_t write_frame,
                       uint32_t fill_frames,
                       uint32_t delay_q16,
                       uint64_t *cases)
{
    const int32_t ref_left = space_room_stereo_read(
        storage, capacity_frames, write_frame, fill_frames, delay_q16, 0u);
    const int32_t ref_right = space_room_stereo_read(
        storage, capacity_frames, write_frame, fill_frames, delay_q16, 1u);
    const P11iStereoPair got = p11i_candidate_stereo_read_pair(
        storage, capacity_frames, write_frame, fill_frames, delay_q16);

    if (got.left != ref_left || got.right != ref_right) {
        fprintf(stderr,
                "FAIL cap=%u write=%u fill=%u delay=%u ref=%d/%d got=%d/%d\n",
                capacity_frames, write_frame, fill_frames, delay_q16,
                ref_left, ref_right, got.left, got.right);
        abort();
    }
    (*cases)++;
}

static uint32_t lcg_next(uint32_t *state)
{
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static void fill_pattern(int16_t *storage, uint32_t capacity_frames, uint32_t seed)
{
    uint32_t i;
    uint32_t s = seed;
    for (i = 0u; i < capacity_frames * 2u; ++i) {
        uint32_t r = lcg_next(&s);
        storage[i] = (int16_t)(r >> 16);
    }
    if (capacity_frames) {
        storage[0] = INT16_MIN;
        storage[1] = INT16_MAX;
        storage[(capacity_frames - 1u) * 2u] = INT16_MAX;
        storage[(capacity_frames - 1u) * 2u + 1u] = INT16_MIN;
    }
}

static void boundary_matrix(uint64_t *cases)
{
    static const uint32_t capacities[] = {1u, 2u, 3u, 4u, 7u, 16u, 63u, 257u};
    static const uint32_t fracs[] = {0u, 1u, 2u, 32767u, 32768u, 32769u, 65534u, 65535u};
    uint32_t c;

    /* Explicit invalid/empty authority cases. */
    check_case(NULL, 0u, 0u, 0u, 0u, cases);
    check_case(NULL, 0u, 0u, 0u, 1u, cases);

    for (c = 0u; c < sizeof(capacities) / sizeof(capacities[0]); ++c) {
        const uint32_t cap = capacities[c];
        int16_t *storage = (int16_t *)malloc(sizeof(int16_t) * cap * 2u);
        uint32_t write, fill, whole, f;
        assert(storage != NULL);
        fill_pattern(storage, cap, 0x13579bdfu ^ cap);

        for (write = 0u; write < cap; ++write) {
            for (fill = 0u; fill <= cap; ++fill) {
                /* Whole-frame delays from 0 through one past the capacity,
                 * crossed with fractions bracketing every interpolation edge. */
                for (whole = 0u; whole <= cap + 1u; ++whole) {
                    for (f = 0u; f < sizeof(fracs) / sizeof(fracs[0]); ++f) {
                        const uint64_t q = ((uint64_t)whole << 16) + fracs[f];
                        if (q <= UINT32_MAX)
                            check_case(storage, cap, write, fill, (uint32_t)q, cases);
                    }
                }

                /* Delays immediately around the current write position force
                 * both direct subtraction and wrapped subtraction paths. */
                if (write) {
                    uint32_t q = write << 16;
                    check_case(storage, cap, write, fill, q - 1u, cases);
                    check_case(storage, cap, write, fill, q, cases);
                    check_case(storage, cap, write, fill, q + 1u, cases);
                } else {
                    check_case(storage, cap, write, fill, 1u, cases);
                    check_case(storage, cap, write, fill, 65535u, cases);
                    check_case(storage, cap, write, fill, 65536u, cases);
                }
            }
        }
        free(storage);
    }
}

static void deterministic_random_matrix(uint64_t *cases)
{
    enum { MAX_CAP = 512, TRIALS = 250000 };
    int16_t *storage = (int16_t *)malloc(sizeof(int16_t) * MAX_CAP * 2u);
    uint32_t s = 0x41505349u;
    uint32_t t;
    assert(storage != NULL);

    for (t = 0u; t < TRIALS; ++t) {
        const uint32_t cap = 1u + (lcg_next(&s) % MAX_CAP);
        const uint32_t write = lcg_next(&s) % cap;
        const uint32_t fill = lcg_next(&s) % (cap + 1u);
        const uint32_t whole = lcg_next(&s) % (cap + 2u);
        const uint32_t frac = lcg_next(&s) & 65535u;
        const uint32_t delay = (whole << 16) | frac;
        uint32_t i;

        for (i = 0u; i < cap * 2u; ++i)
            storage[i] = (int16_t)(lcg_next(&s) >> 16);
        check_case(storage, cap, write, fill, delay, cases);
    }
    free(storage);
}

int main(void)
{
    uint64_t cases = 0u;
    boundary_matrix(&cases);
    deterministic_random_matrix(&cases);
    printf("P11I stereo-read equivalence cases=%llu\n", (unsigned long long)cases);
    puts("PASS: P11I candidate stereo-pair reader is bit-identical to current single-channel authority");
    return 0;
}
