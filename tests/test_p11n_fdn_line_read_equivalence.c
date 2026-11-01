#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The current P11L implementation is the authority in this translation unit. */
#include "../apsis/apsis_space_dsp.c"

typedef struct {
    uint64_t helper_cases;
    uint64_t full_render_blocks;
    uint64_t full_render_frames;
} P11nStats;

static P11nStats g_stats;

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

/* Candidate authorized by P11M only:
 * - retain the defensive helper guard;
 * - partial lines retain the exact historical readiness calculation;
 * - full lines bypass `needed`, because capacity > maximum legal needed;
 * - production modulation bounds make the minimum-delay clamp unreachable;
 * - topology bounds make the second/upper ring-wrap correction unreachable;
 * - all interpolation and negative-wrap arithmetic remains byte-for-byte shaped
 *   like the current authority. */
static int32_t p11n_candidate_line_read(const ApsisSpaceFdn *fdn,
                                        const ApsisSpaceFdnLine *line,
                                        int32_t modulation_q8)
{
    int32_t delay_q8, read_q8, span_q8;
    uint32_t needed, i0, i1, frac;
    int32_t a, b;
    const int16_t *base;
    if (!fdn || !line || !fdn->storage_bound) return 0;
    delay_q8 = (int32_t)line->base_delay * 256 + modulation_q8;
    if (line->fill_count < line->capacity) {
        needed = (uint32_t)(delay_q8 + 255) >> 8;
        if (line->fill_count < needed) return 0;
    }

    span_q8 = (int32_t)line->capacity * 256;
    read_q8 = (int32_t)line->write_pos * 256 - delay_q8;
    if (read_q8 < 0) read_q8 += span_q8;
    i0 = (uint32_t)read_q8 >> 8;
    frac = (uint32_t)read_q8 & 255u;
    i1 = i0 + 1u;
    if (i1 >= line->capacity) i1 = 0u;
    base = fdn->storage + line->offset;
    a = base[i0];
    if (!frac) return a;
    b = base[i1];
    return (a * (int32_t)(256u - frac) + b * (int32_t)frac) >> 8;
}

static void check_helper_case(ApsisSpaceFdn *fdn, uint8_t line_index,
                              uint16_t fill_count, uint16_t write_pos,
                              int32_t modulation_q8)
{
    ApsisSpaceFdnLine *line = &fdn->line[line_index];
    int32_t authority, candidate;
    assert(fill_count <= line->capacity);
    assert(write_pos < line->capacity);
    assert(modulation_q8 >= -2304 && modulation_q8 <= 2303);
    line->fill_count = fill_count;
    line->write_pos = write_pos;
    authority = space_line_read(fdn, line, modulation_q8);
    candidate = p11n_candidate_line_read(fdn, line, modulation_q8);
    assert(authority == candidate);
    g_stats.helper_cases++;
}

static void add_unique_u16(uint16_t *v, uint8_t *count, uint16_t x, uint16_t limit)
{
    uint8_t i;
    if (x >= limit) return;
    for (i = 0u; i < *count; ++i) if (v[i] == x) return;
    v[(*count)++] = x;
}

static void test_helper_boundary_matrix(void)
{
    ApsisSpaceFdn fdn = {0};
    int16_t *storage = malloc(APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*storage));
    uint32_t i;
    uint8_t k;
    int32_t mod;
    assert(storage);
    for (i = 0u; i < APSIS_SPACE_FDN_STORAGE_SAMPLES; ++i)
        storage[i] = (int16_t)((int32_t)i - 10000);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        ApsisSpaceFdnLine *line = &fdn.line[k];
        for (mod = -2304; mod <= 2303; ++mod) {
            int32_t delay_q8 = (int32_t)line->base_delay * 256 + mod;
            uint16_t needed = (uint16_t)(((uint32_t)delay_q8 + 255u) >> 8);
            uint16_t fills[6];
            uint16_t writes[8];
            uint8_t fill_count = 0u, write_count = 0u, f, w;

            add_unique_u16(fills, &fill_count, 0u, line->capacity);
            add_unique_u16(fills, &fill_count, 1u, line->capacity);
            if (needed) add_unique_u16(fills, &fill_count, (uint16_t)(needed - 1u), line->capacity);
            add_unique_u16(fills, &fill_count, needed, line->capacity);
            add_unique_u16(fills, &fill_count, (uint16_t)(line->capacity - 1u), line->capacity);

            /* Natural partial-fill state: write_pos advances with fill_count. */
            for (f = 0u; f < fill_count; ++f)
                check_helper_case(&fdn, k, fills[f], fills[f], mod);

            /* Full-line state: write_pos can be anywhere in the ring. Include
             * both ring ends and the raw-read zero-crossing neighborhood. */
            add_unique_u16(writes, &write_count, 0u, line->capacity);
            add_unique_u16(writes, &write_count, 1u, line->capacity);
            if (needed) add_unique_u16(writes, &write_count, (uint16_t)(needed - 1u), line->capacity);
            add_unique_u16(writes, &write_count, needed, line->capacity);
            add_unique_u16(writes, &write_count, (uint16_t)(line->capacity - 2u), line->capacity);
            add_unique_u16(writes, &write_count, (uint16_t)(line->capacity - 1u), line->capacity);
            for (w = 0u; w < write_count; ++w)
                check_helper_case(&fdn, k, line->capacity, writes[w], mod);
        }
    }
    free(storage);
}

static void test_helper_random_valid_states(void)
{
    enum { CASES = 500000 };
    ApsisSpaceFdn fdn = {0};
    int16_t *storage = malloc(APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*storage));
    uint32_t rng = 0x5031314eu;
    uint32_t i;
    assert(storage);
    for (i = 0u; i < APSIS_SPACE_FDN_STORAGE_SAMPLES; ++i)
        storage[i] = (int16_t)((int32_t)i - 10000);
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    for (i = 0u; i < CASES; ++i) {
        uint8_t k = (uint8_t)(lcg_next(&rng) % APSIS_SPACE_FDN_LINES);
        ApsisSpaceFdnLine *line = &fdn.line[k];
        int32_t mod = -2304 + (int32_t)(lcg_next(&rng) % 4608u);
        uint16_t fill, write;
        if (lcg_next(&rng) & 1u) {
            fill = line->capacity;
            write = (uint16_t)(lcg_next(&rng) % line->capacity);
        } else {
            fill = (uint16_t)(lcg_next(&rng) % line->capacity);
            write = fill;
        }
        check_helper_case(&fdn, k, fill, write, mod);
    }
    free(storage);
}

static void p11n_candidate_render(ApsisSpaceFdn *fdn,
                                  const int32_t *input_mono_q15,
                                  int32_t *out_stereo,
                                  uint32_t frames,
                                  uint16_t decay_ms,
                                  uint16_t damping_hz)
{
    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    if (!fdn || !fdn->storage_bound || !out_stereo) return;
    alpha = space_damping_alpha(damping_hz);
    if (!fdn->feedback_cache_valid || fdn->feedback_decay_ms != decay_ms) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
            fdn->feedback_q15[k] = space_feedback_gain(fdn->line[k].base_delay, decay_ms);
        fdn->feedback_decay_ms = decay_ms;
        fdn->feedback_cache_valid = 1u;
    }

    for (frame = 0u; frame < frames; ++frame) {
        int32_t mixed[APSIS_SPACE_FDN_LINES];
        int32_t out_l = 0, out_r = 0;
        int32_t input = input_mono_q15 ? input_mono_q15[frame] : 0;
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int32_t mod_q8, delayed;
            line->lfo_phase += line->lfo_phase_inc;
            mod_q8 = ((int32_t)space_sine_q15(line->lfo_phase) * SPACE_MOD_DEPTH_Q8) >> 15;
            delayed = p11n_candidate_line_read(fdn, line, mod_q8);
            line->damping_state_q15 += space_mul_q15(delayed - line->damping_state_q15, alpha);
            mixed[k] = line->damping_state_q15;
            out_l += (int32_t)SPACE_OUT_LEFT[k] * mixed[k];
            out_r += (int32_t)SPACE_OUT_RIGHT[k] * mixed[k];
        }
        fdn->post_left_q15 += space_mul_q15(
            space_mul_q15(out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha);
        fdn->post_right_q15 += space_mul_q15(
            space_mul_q15(out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha);
        out_stereo[frame * 2u] +=
            space_mul_q15(fdn->post_left_q15, SPACE_CROSS_SELF_Q15) +
            space_mul_q15(fdn->post_right_q15, SPACE_CROSS_OTHER_Q15);
        out_stereo[frame * 2u + 1u] +=
            space_mul_q15(fdn->post_right_q15, SPACE_CROSS_SELF_Q15) +
            space_mul_q15(fdn->post_left_q15, SPACE_CROSS_OTHER_Q15);
        space_hadamard8(mixed);
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int16_t *base = fdn->storage + line->offset;
            int32_t feedback = space_mul_q15(mixed[k], SPACE_HAD_NORM_Q15);
            int32_t excitation = space_mul_q15(input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15);
            int32_t write_value;
            feedback = space_mul_q15(feedback, fdn->feedback_q15[k]);
            write_value = space_clamp_q15((int64_t)excitation + feedback);
            base[line->write_pos] = (int16_t)write_value;
            line->write_pos++;
            if (line->write_pos >= line->capacity) line->write_pos = 0u;
            if (line->fill_count < line->capacity) line->fill_count++;
        }
    }
}

typedef struct {
    ApsisSpaceFdn authority;
    ApsisSpaceFdn candidate;
    int16_t *storage_authority;
    int16_t *storage_candidate;
} P11nPair;

static void assert_fdn_equal(const P11nPair *p)
{
    uint8_t k;
    const ApsisSpaceFdn *a = &p->authority;
    const ApsisSpaceFdn *b = &p->candidate;
    assert(a->storage_samples == b->storage_samples);
    assert(a->storage_bound == b->storage_bound);
    assert(a->post_left_q15 == b->post_left_q15);
    assert(a->post_right_q15 == b->post_right_q15);
    assert(a->feedback_decay_ms == b->feedback_decay_ms);
    assert(a->feedback_cache_valid == b->feedback_cache_valid);
    assert(memcmp(a->feedback_q15, b->feedback_q15, sizeof(a->feedback_q15)) == 0);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        const ApsisSpaceFdnLine *x = &a->line[k];
        const ApsisSpaceFdnLine *y = &b->line[k];
        assert(x->offset == y->offset);
        assert(x->base_delay == y->base_delay);
        assert(x->capacity == y->capacity);
        assert(x->write_pos == y->write_pos);
        assert(x->fill_count == y->fill_count);
        assert(x->lfo_phase == y->lfo_phase);
        assert(x->lfo_phase_inc == y->lfo_phase_inc);
        assert(x->damping_state_q15 == y->damping_state_q15);
    }
    assert(memcmp(p->storage_authority, p->storage_candidate,
                  APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*p->storage_authority)) == 0);
}

static void pair_init(P11nPair *p)
{
    memset(p, 0, sizeof(*p));
    p->storage_authority = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->storage_authority));
    p->storage_candidate = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->storage_candidate));
    assert(p->storage_authority && p->storage_candidate);
    assert(apsis_space_fdn_bind(&p->authority, p->storage_authority, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(apsis_space_fdn_bind(&p->candidate, p->storage_candidate, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert_fdn_equal(p);
}

static void pair_free(P11nPair *p)
{
    free(p->storage_authority);
    free(p->storage_candidate);
}

static void pair_reset(P11nPair *p)
{
    apsis_space_fdn_reset(&p->authority);
    apsis_space_fdn_reset(&p->candidate);
    assert_fdn_equal(p);
}

static void pair_randomize_phases(P11nPair *p, uint32_t *rng)
{
    uint8_t k;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        uint32_t phase = lcg_next(rng);
        p->authority.line[k].lfo_phase = phase;
        p->candidate.line[k].lfo_phase = phase;
    }
}

static void pair_render(P11nPair *p, uint32_t frames, uint16_t decay_ms,
                        uint16_t damping_hz, uint32_t *rng)
{
    int32_t input[64];
    int32_t out_a[128];
    int32_t out_b[128];
    uint32_t i;
    assert(frames >= 1u && frames <= 64u);
    for (i = 0u; i < frames; ++i)
        input[i] = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 3;
    for (i = 0u; i < frames * 2u; ++i) {
        int32_t seed = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 7;
        out_a[i] = seed;
        out_b[i] = seed;
    }
    apsis_space_fdn_render(&p->authority, input, out_a, frames, decay_ms, damping_hz);
    p11n_candidate_render(&p->candidate, input, out_b, frames, decay_ms, damping_hz);
    assert(memcmp(out_a, out_b, frames * 2u * sizeof(*out_a)) == 0);
    assert_fdn_equal(p);
    g_stats.full_render_blocks++;
    g_stats.full_render_frames += frames;
}

static void render_total(P11nPair *p, uint32_t frames_total, uint32_t *rng)
{
    uint32_t done = 0u;
    while (done < frames_total) {
        uint32_t n = frames_total - done;
        if (n > 64u) n = 64u;
        pair_render(p, n, 8000u, 4500u, rng);
        done += n;
    }
}

static void test_full_render_trajectories(void)
{
    P11nPair p;
    uint32_t rng = 0x46554c4cu;
    uint32_t i;
    pair_init(&p);

    /* Cold trajectory covers startup readiness. */
    render_total(&p, 16384u, &rng);

    /* Warm trajectory covers sustained full-line ring operation. */
    render_total(&p, 65536u, &rng);

    /* Repeated resets plus arbitrary valid LFO phases/parameters exercise the
     * production caller across a much wider deterministic state trajectory. */
    for (i = 0u; i < 2048u; ++i) {
        uint16_t decay, damping;
        uint32_t frames;
        if (i % 257u == 0u) {
            pair_reset(&p);
            pair_randomize_phases(&p, &rng);
        }
        decay = (uint16_t)(4000u + (lcg_next(&rng) % 8001u));
        damping = (uint16_t)(2200u + (lcg_next(&rng) % 4601u));
        frames = 1u + (lcg_next(&rng) % 64u);
        pair_render(&p, frames, decay, damping, &rng);
    }
    pair_free(&p);
}

int main(void)
{
    memset(&g_stats, 0, sizeof(g_stats));
    test_helper_boundary_matrix();
    test_helper_random_valid_states();
    test_full_render_trajectories();
    printf("P11N helper_cases=%llu full_render_blocks=%llu full_render_frames=%llu\n",
           (unsigned long long)g_stats.helper_cases,
           (unsigned long long)g_stats.full_render_blocks,
           (unsigned long long)g_stats.full_render_frames);
    assert(g_stats.helper_cases > 800000u);
    assert(g_stats.full_render_frames > 140000u);
    puts("PASS: P11N invariant-simplified line read is bit-identical to P11L authority");
    return 0;
}
