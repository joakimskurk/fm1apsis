#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Frozen P11W production implementation is the baseline authority. Static
 * helpers are reused by the candidate so only the output sign-mix changes. */
#include "../apsis/apsis_space_dsp.c"

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void baseline_signmix(const int32_t mixed[APSIS_SPACE_FDN_LINES],
                             int32_t *left, int32_t *right)
{
    int32_t l = 0, r = 0;
    uint8_t k;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        l += (int32_t)SPACE_OUT_LEFT[k] * mixed[k];
        r += (int32_t)SPACE_OUT_RIGHT[k] * mixed[k];
    }
    *left = l;
    *right = r;
}

static void candidate_signmix(const int32_t mixed[APSIS_SPACE_FDN_LINES],
                              int32_t *left, int32_t *right)
{
    int32_t l = mixed[0];
    int32_t r = mixed[0];
    l += mixed[1];
    r -= mixed[1];
    l -= mixed[2];
    r += mixed[2];
    l += mixed[3];
    r += mixed[3];
    l -= mixed[4];
    r += mixed[4];
    l += mixed[5];
    r -= mixed[5];
    l += mixed[6];
    r += mixed[6];
    l -= mixed[7];
    r -= mixed[7];
    *left = l;
    *right = r;
}

static void assert_signmix_equal(const int32_t mixed[APSIS_SPACE_FDN_LINES])
{
    int32_t bl, br, cl, cr;
    baseline_signmix(mixed, &bl, &br);
    candidate_signmix(mixed, &cl, &cr);
    assert(bl == cl);
    assert(br == cr);
    assert(bl >= -262136 && bl <= 262136);
    assert(br >= -262136 && br <= 262136);
}

static void test_arithmetic_domain(void)
{
    static const int32_t edge[] = {-32767, -32766, -1, 0, 1, 32766, 32767};
    int32_t mixed[APSIS_SPACE_FDN_LINES];
    uint32_t rng = 0x59534947u;
    uint64_t cases = 0u;
    uint32_t mask, i, k;

    /* Every sign combination at the legal extremes. */
    for (mask = 0u; mask < 256u; ++mask) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
            mixed[k] = (mask & (1u << k)) ? 32767 : -32767;
        assert_signmix_equal(mixed);
        cases++;
    }

    /* One-hot/one-field boundary sweep. */
    memset(mixed, 0, sizeof(mixed));
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        size_t j;
        for (j = 0u; j < sizeof(edge)/sizeof(edge[0]); ++j) {
            memset(mixed, 0, sizeof(mixed));
            mixed[k] = edge[j];
            assert_signmix_equal(mixed);
            cases++;
        }
    }

    /* Million deterministic legal-Q15 vectors. */
    for (i = 0u; i < 1000000u; ++i) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            uint32_t r = lcg_next(&rng);
            mixed[k] = (int32_t)(r % 65535u) - 32767;
        }
        assert_signmix_equal(mixed);
        cases++;
    }
    printf("P11Y arithmetic_cases=%llu\n", (unsigned long long)cases);
    assert(cases == 1000312u);
}

static void candidate_render(ApsisSpaceFdn *fdn,
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
        int32_t out_l, out_r;
        int32_t input = input_mono_q15 ? input_mono_q15[frame] : 0;

        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int32_t mod_q8, delayed;
            int16_t sine_q15;
            uint32_t old_phase = line->lfo_phase;
            uint8_t old_index = (uint8_t)(old_phase >> 24);
            uint8_t index;
            line->lfo_phase += line->lfo_phase_inc;
            index = (uint8_t)(line->lfo_phase >> 24);
            if (index != old_index) {
                int16_t a = space_sine_point(index);
                int16_t b = space_sine_point((uint8_t)(index + 1u));
                line->lfo_sine_a_q15 = a;
                line->lfo_sine_delta_q15 = (int16_t)((int32_t)b - a);
            }
            sine_q15 = (int16_t)((int32_t)line->lfo_sine_a_q15 +
                (((int32_t)line->lfo_sine_delta_q15 *
                  (int32_t)((uint8_t)(line->lfo_phase >> 16))) >> 8));
            mod_q8 = ((int32_t)sine_q15 * SPACE_MOD_DEPTH_Q8) >> 15;
            delayed = space_line_read(fdn, line, mod_q8);
            line->damping_state_q15 += space_mul_q15(
                delayed - line->damping_state_q15, alpha);
            mixed[k] = line->damping_state_q15;
        }

        candidate_signmix(mixed, &out_l, &out_r);

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
            int32_t excitation = space_mul_q15(
                input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15);
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

static void assert_fdn_equal(const ApsisSpaceFdn *a, const ApsisSpaceFdn *b,
                             const int16_t *sa, const int16_t *sb)
{
    uint8_t k;
    assert(a->storage_samples == b->storage_samples);
    assert(a->storage_bound == b->storage_bound);
    assert(a->post_left_q15 == b->post_left_q15);
    assert(a->post_right_q15 == b->post_right_q15);
    assert(a->feedback_decay_ms == b->feedback_decay_ms);
    assert(a->feedback_cache_valid == b->feedback_cache_valid);
    assert(memcmp(a->feedback_q15, b->feedback_q15, sizeof(a->feedback_q15)) == 0);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        assert(memcmp(&a->line[k], &b->line[k], sizeof(a->line[k])) == 0);
    assert(memcmp(sa, sb, APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*sa)) == 0);
}

typedef struct {
    ApsisSpaceFdn baseline;
    ApsisSpaceFdn candidate;
    int16_t *sa;
    int16_t *sb;
    uint64_t blocks;
    uint64_t frames;
} Pair;

static void pair_init(Pair *p)
{
    memset(p, 0, sizeof(*p));
    p->sa = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->sa));
    p->sb = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->sb));
    assert(p->sa && p->sb);
    assert(apsis_space_fdn_bind(&p->baseline, p->sa, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(apsis_space_fdn_bind(&p->candidate, p->sb, APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert_fdn_equal(&p->baseline, &p->candidate, p->sa, p->sb);
}

static void pair_reset(Pair *p)
{
    apsis_space_fdn_reset(&p->baseline);
    apsis_space_fdn_reset(&p->candidate);
    assert_fdn_equal(&p->baseline, &p->candidate, p->sa, p->sb);
}

static void pair_render(Pair *p, uint32_t frames, uint16_t decay,
                        uint16_t damping, uint32_t *rng)
{
    int32_t input[64], oa[128], ob[128];
    uint32_t i;
    assert(frames <= 64u);
    for (i = 0u; i < frames; ++i)
        input[i] = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 2;
    for (i = 0u; i < frames * 2u; ++i) {
        int32_t seed = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 8;
        oa[i] = seed;
        ob[i] = seed;
    }
    apsis_space_fdn_render(&p->baseline, input, oa, frames, decay, damping);
    candidate_render(&p->candidate, input, ob, frames, decay, damping);
    assert(memcmp(oa, ob, frames * 2u * sizeof(*oa)) == 0);
    assert_fdn_equal(&p->baseline, &p->candidate, p->sa, p->sb);
    p->blocks++;
    p->frames += frames;
}

static void pair_free(Pair *p)
{
    free(p->sa);
    free(p->sb);
}

static void test_trajectories(void)
{
    Pair p;
    uint32_t rng = 0x5946444eu;
    uint32_t i;
    pair_init(&p);

    /* Cold then warm steady state. */
    for (i = 0u; i < 256u; ++i) pair_render(&p, 64u, 8000u, 4500u, &rng);
    for (i = 0u; i < 1024u; ++i) pair_render(&p, 64u, 8000u, 4500u, &rng);

    /* Legal parameter sweep. */
    for (i = 0u; i < 423u; ++i) {
        uint16_t decay = (uint16_t)(4000u + (i * 8000u) / 422u);
        uint16_t damping = (uint16_t)(2200u + (i * 4600u) / 422u);
        pair_render(&p, 32u, decay, damping, &rng);
    }

    /* Adversarial extremes. */
    for (i = 0u; i < 512u; ++i)
        pair_render(&p, 64u, (i & 1u) ? 12000u : 4000u,
                    (i & 1u) ? 6800u : 2200u, &rng);

    /* Deterministic random frames/parameters with regular resets. */
    for (i = 0u; i < 4096u; ++i) {
        if (i && i % 257u == 0u) pair_reset(&p);
        pair_render(&p, 1u + (lcg_next(&rng) % 64u),
                    (uint16_t)(4000u + (lcg_next(&rng) % 8001u)),
                    (uint16_t)(2200u + (lcg_next(&rng) % 4601u)), &rng);
    }

    printf("P11Y fdn_blocks=%llu fdn_frames=%llu\n",
           (unsigned long long)p.blocks, (unsigned long long)p.frames);
    assert(p.blocks == 6311u);
    assert(p.frames > 330000u);
    pair_free(&p);
}

int main(void)
{
    test_arithmetic_domain();
    test_trajectories();
    puts("PASS: P11Y fixed add-sub output mix is bit-identical to P11W sign-multiply authority");
    return 0;
}
