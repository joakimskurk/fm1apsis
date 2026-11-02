#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include the production implementation so the current always-recomputed
 * render and its exact static helpers are the authority in this TU. */
#include "../apsis/apsis_space_dsp.c"

typedef struct {
    uint16_t feedback_q15[APSIS_SPACE_FDN_LINES];
    uint16_t decay_ms;
    uint8_t valid;
} P11kFeedbackCache;

typedef struct {
    uint64_t recompute_blocks;
    uint64_t feedback_gain_evals;
} P11kStats;

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void cache_reset(P11kFeedbackCache *cache)
{
    memset(cache, 0, sizeof(*cache));
}

static void cached_render(ApsisSpaceFdn *fdn,
                          P11kFeedbackCache *cache,
                          P11kStats *stats,
                          const int32_t *input_mono_q15,
                          int32_t *out_stereo,
                          uint32_t frames,
                          uint16_t decay_ms,
                          uint16_t damping_hz)
{
    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    if (!fdn || !fdn->storage_bound || !out_stereo || !cache || !stats) return;
    alpha = space_damping_alpha(damping_hz);
    if (!cache->valid || cache->decay_ms != decay_ms) {
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            cache->feedback_q15[k] = space_feedback_gain(
                fdn->line[k].base_delay, decay_ms);
            stats->feedback_gain_evals++;
        }
        cache->decay_ms = decay_ms;
        cache->valid = 1u;
        stats->recompute_blocks++;
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
            delayed = space_line_read(fdn, line, mod_q8);
            line->damping_state_q15 += space_mul_q15(
                delayed - line->damping_state_q15, alpha);
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
            int32_t excitation = space_mul_q15(
                input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15);
            int32_t write_value;
            feedback = space_mul_q15(feedback, cache->feedback_q15[k]);
            write_value = space_clamp_q15((int64_t)excitation + feedback);
            base[line->write_pos] = (int16_t)write_value;
            line->write_pos++;
            if (line->write_pos >= line->capacity) line->write_pos = 0u;
            if (line->fill_count < line->capacity) line->fill_count++;
        }
    }
}

static void assert_fdn_equal(const ApsisSpaceFdn *a, const ApsisSpaceFdn *b,
                             const int16_t *storage_a, const int16_t *storage_b)
{
    uint8_t k;
    assert(a->storage_samples == b->storage_samples);
    assert(a->storage_bound == b->storage_bound);
    assert(a->post_left_q15 == b->post_left_q15);
    assert(a->post_right_q15 == b->post_right_q15);
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
    assert(memcmp(storage_a, storage_b,
                  APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*storage_a)) == 0);
}

typedef struct {
    ApsisSpaceFdn baseline;
    ApsisSpaceFdn candidate;
    int16_t *storage_baseline;
    int16_t *storage_candidate;
    P11kFeedbackCache cache;
    P11kStats stats;
    uint64_t blocks;
    uint64_t frames;
} P11kPair;

static void pair_init(P11kPair *p)
{
    memset(p, 0, sizeof(*p));
    p->storage_baseline = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES,
                                 sizeof(*p->storage_baseline));
    p->storage_candidate = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES,
                                  sizeof(*p->storage_candidate));
    assert(p->storage_baseline && p->storage_candidate);
    assert(apsis_space_fdn_bind(&p->baseline, p->storage_baseline,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(apsis_space_fdn_bind(&p->candidate, p->storage_candidate,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    cache_reset(&p->cache);
    assert_fdn_equal(&p->baseline, &p->candidate,
                     p->storage_baseline, p->storage_candidate);
}

static void pair_reset(P11kPair *p)
{
    apsis_space_fdn_reset(&p->baseline);
    apsis_space_fdn_reset(&p->candidate);
    cache_reset(&p->cache);
    assert_fdn_equal(&p->baseline, &p->candidate,
                     p->storage_baseline, p->storage_candidate);
}

static void pair_free(P11kPair *p)
{
    free(p->storage_baseline);
    free(p->storage_candidate);
}

static void pair_render(P11kPair *p, uint32_t frames,
                        uint16_t decay_ms, uint16_t damping_hz,
                        uint32_t *rng)
{
    int32_t input[64];
    int32_t out_a[128];
    int32_t out_b[128];
    uint32_t i;
    assert(frames <= 64u);
    for (i = 0u; i < frames; ++i)
        input[i] = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 2;
    for (i = 0u; i < frames * 2u; ++i) {
        int32_t seed = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 8;
        out_a[i] = seed;
        out_b[i] = seed;
    }

    apsis_space_fdn_render(&p->baseline, input, out_a, frames,
                           decay_ms, damping_hz);
    cached_render(&p->candidate, &p->cache, &p->stats,
                  input, out_b, frames, decay_ms, damping_hz);

    assert(memcmp(out_a, out_b, frames * 2u * sizeof(*out_a)) == 0);
    assert_fdn_equal(&p->baseline, &p->candidate,
                     p->storage_baseline, p->storage_candidate);
    p->blocks++;
    p->frames += frames;
}

static int32_t smooth_q16(int32_t cur, int32_t target)
{
    int64_t delta = (int64_t)target - cur;
    int64_t step;
    if (!delta) return cur;
    step = (delta * 9) >> 15;
    if (!step) step = delta > 0 ? 1 : -1;
    return (int32_t)((int64_t)cur + step);
}

static uint16_t block_smoothed_decay(int32_t *cur_q16, uint16_t target_ms)
{
    uint32_t i;
    int32_t target = (int32_t)((uint32_t)target_ms << 16);
    for (i = 0u; i < 64u; ++i) *cur_q16 = smooth_q16(*cur_q16, target);
    return (uint16_t)(((uint32_t)*cur_q16 + 32768u) >> 16);
}

static void test_partial_and_steady(void)
{
    P11kPair p;
    uint32_t rng = 0x50415254u;
    uint32_t i;
    pair_init(&p);
    for (i = 1u; i <= 32u; ++i)
        pair_render(&p, 1u + (i % 17u), 8000u, 4500u, &rng);
    pair_reset(&p);
    memset(&p.stats, 0, sizeof(p.stats));
    p.blocks = p.frames = 0u;
    for (i = 0u; i < 256u; ++i)
        pair_render(&p, 64u, 8000u, 4500u, &rng);
    assert(p.stats.recompute_blocks == 1u);
    assert(p.stats.feedback_gain_evals == 8u);
    printf("P11K steady blocks=%llu frames=%llu recompute_blocks=%llu gain_evals=%llu\n",
           (unsigned long long)p.blocks, (unsigned long long)p.frames,
           (unsigned long long)p.stats.recompute_blocks,
           (unsigned long long)p.stats.feedback_gain_evals);
    pair_free(&p);
}

static void test_smoothed_sweep(void)
{
    P11kPair p;
    uint32_t rng = 0x53574545u;
    uint32_t i;
    int32_t decay_q16 = (int32_t)(4000u << 16);
    uint16_t previous = 0u;
    uint32_t changes = 0u;
    pair_init(&p);
    for (i = 0u; i < 256u; ++i) {
        uint16_t target = i < 64u ? 4000u : 12000u;
        uint16_t decay = block_smoothed_decay(&decay_q16, target);
        if (i && decay != previous) changes++;
        previous = decay;
        pair_render(&p, 64u, decay,
                    (uint16_t)(2200u + (i * 4600u) / 255u), &rng);
    }
    assert(changes == 192u);
    assert(p.stats.recompute_blocks == 193u);
    assert(p.stats.feedback_gain_evals == 1544u);
    printf("P11K sweep changes=%u recompute_blocks=%llu gain_evals=%llu\n",
           changes, (unsigned long long)p.stats.recompute_blocks,
           (unsigned long long)p.stats.feedback_gain_evals);
    pair_free(&p);
}

static void test_adversarial(void)
{
    P11kPair p;
    uint32_t rng = 0x41445645u;
    uint32_t i;
    pair_init(&p);
    for (i = 0u; i < 256u; ++i)
        pair_render(&p, 64u, (i & 1u) ? 12000u : 4000u,
                    (i & 1u) ? 6800u : 2200u, &rng);
    assert(p.stats.recompute_blocks == 256u);
    assert(p.stats.feedback_gain_evals == 2048u);
    printf("P11K adversarial recompute_blocks=%llu gain_evals=%llu\n",
           (unsigned long long)p.stats.recompute_blocks,
           (unsigned long long)p.stats.feedback_gain_evals);
    pair_free(&p);
}

static void test_all_legal_decay_keys(void)
{
    P11kPair p;
    uint32_t rng = 0x4b455953u;
    uint32_t d;
    pair_init(&p);
    for (d = 4000u; d <= 12000u; ++d) {
        uint16_t damping = (uint16_t)(2200u + ((d - 4000u) % 4601u));
        pair_render(&p, 1u, (uint16_t)d, damping, &rng);
        pair_render(&p, 1u, (uint16_t)d, (uint16_t)(6800u - (d % 4601u)), &rng);
    }
    assert(p.stats.recompute_blocks == 8001u);
    assert(p.stats.feedback_gain_evals == 8001u * 8u);
    printf("P11K all_keys blocks=%llu recompute_blocks=%llu gain_evals=%llu\n",
           (unsigned long long)p.blocks,
           (unsigned long long)p.stats.recompute_blocks,
           (unsigned long long)p.stats.feedback_gain_evals);
    pair_free(&p);
}

static void test_random_with_resets(void)
{
    P11kPair p;
    uint32_t rng = 0x52414e44u;
    uint32_t i;
    pair_init(&p);
    for (i = 0u; i < 4096u; ++i) {
        uint16_t decay;
        uint16_t damping;
        uint32_t r;
        if (i && i % 257u == 0u) pair_reset(&p);
        r = lcg_next(&rng);
        if ((r & 3u) == 0u && p.cache.valid) decay = p.cache.decay_ms;
        else decay = (uint16_t)(4000u + (lcg_next(&rng) % 8001u));
        damping = (uint16_t)(2200u + (lcg_next(&rng) % 4601u));
        pair_render(&p, 1u + (lcg_next(&rng) % 64u), decay, damping, &rng);
    }
    printf("P11K random blocks=%llu frames=%llu recompute_blocks=%llu gain_evals=%llu\n",
           (unsigned long long)p.blocks, (unsigned long long)p.frames,
           (unsigned long long)p.stats.recompute_blocks,
           (unsigned long long)p.stats.feedback_gain_evals);
    pair_free(&p);
}

int main(void)
{
    printf("P11K projected_cache_size_host=%zu\n", sizeof(P11kFeedbackCache));
    assert(sizeof(P11kFeedbackCache) == 20u);
    test_partial_and_steady();
    test_smoothed_sweep();
    test_adversarial();
    test_all_legal_decay_keys();
    test_random_with_resets();
    puts("PASS: P11K cached FDN feedback gains are bit-identical to always-recomputed authority");
    return 0;
}
