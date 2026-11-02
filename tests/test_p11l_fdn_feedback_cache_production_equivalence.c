#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Production implementation is the candidate. Static helpers are reused by
 * the frozen pre-P11L authority below so arithmetic is compared, not guessed. */
#include "../apsis/apsis_space_dsp.c"

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

/* Frozen pre-P11L authority: feedback gains are recomputed on every render. */
static void p11l_frozen_render(ApsisSpaceFdn *fdn,
                               const int32_t *input_mono_q15,
                               int32_t *out_stereo,
                               uint32_t frames,
                               uint16_t decay_ms,
                               uint16_t damping_hz)
{
    uint16_t feedback_q15[APSIS_SPACE_FDN_LINES];
    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    if (!fdn || !fdn->storage_bound || !out_stereo) return;
    alpha = space_damping_alpha(damping_hz);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        feedback_q15[k] = space_feedback_gain(fdn->line[k].base_delay, decay_ms);

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
            feedback = space_mul_q15(feedback, feedback_q15[k]);
            write_value = space_clamp_q15((int64_t)excitation + feedback);
            base[line->write_pos] = (int16_t)write_value;
            line->write_pos++;
            if (line->write_pos >= line->capacity) line->write_pos = 0u;
            if (line->fill_count < line->capacity) line->fill_count++;
        }
    }
}

static void assert_legacy_fdn_equal(const ApsisSpaceFdn *a,
                                    const ApsisSpaceFdn *b,
                                    const int16_t *storage_a,
                                    const int16_t *storage_b)
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

static void assert_cache_exact(const ApsisSpaceFdn *fdn, uint16_t decay_ms)
{
    uint8_t k;
    assert(fdn->feedback_cache_valid == 1u);
    assert(fdn->feedback_decay_ms == decay_ms);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        assert(fdn->feedback_q15[k] ==
               space_feedback_gain(fdn->line[k].base_delay, decay_ms));
}

typedef struct {
    ApsisSpaceFdn baseline;
    ApsisSpaceFdn production;
    int16_t *storage_baseline;
    int16_t *storage_production;
    uint64_t blocks;
    uint64_t frames;
} P11lPair;

static void pair_init(P11lPair *p)
{
    memset(p, 0, sizeof(*p));
    p->storage_baseline = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES,
                                 sizeof(*p->storage_baseline));
    p->storage_production = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES,
                                   sizeof(*p->storage_production));
    assert(p->storage_baseline && p->storage_production);
    assert(apsis_space_fdn_bind(&p->baseline, p->storage_baseline,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(apsis_space_fdn_bind(&p->production, p->storage_production,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(p->baseline.feedback_cache_valid == 0u);
    assert(p->production.feedback_cache_valid == 0u);
    assert_legacy_fdn_equal(&p->baseline, &p->production,
                            p->storage_baseline, p->storage_production);
}

static void pair_reset(P11lPair *p)
{
    apsis_space_fdn_reset(&p->baseline);
    apsis_space_fdn_reset(&p->production);
    assert(p->baseline.feedback_cache_valid == 0u);
    assert(p->production.feedback_cache_valid == 0u);
    assert_legacy_fdn_equal(&p->baseline, &p->production,
                            p->storage_baseline, p->storage_production);
}

static void pair_free(P11lPair *p)
{
    free(p->storage_baseline);
    free(p->storage_production);
}

static void pair_render(P11lPair *p, uint32_t frames,
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

    p11l_frozen_render(&p->baseline, input, out_a, frames, decay_ms, damping_hz);
    apsis_space_fdn_render(&p->production, input, out_b, frames, decay_ms, damping_hz);
    assert(memcmp(out_a, out_b, frames * 2u * sizeof(*out_a)) == 0);
    assert_legacy_fdn_equal(&p->baseline, &p->production,
                            p->storage_baseline, p->storage_production);
    assert_cache_exact(&p->production, decay_ms);
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

static void test_partial_reset_and_steady(void)
{
    P11lPair p;
    uint32_t rng = 0x4c535444u;
    uint32_t i;
    pair_init(&p);
    for (i = 1u; i <= 32u; ++i)
        pair_render(&p, 1u + (i % 17u), 8000u, 4500u, &rng);
    pair_reset(&p);
    p.blocks = p.frames = 0u;
    for (i = 0u; i < 256u; ++i)
        pair_render(&p, 64u, 8000u, 4500u, &rng);
    printf("P11L steady blocks=%llu frames=%llu\n",
           (unsigned long long)p.blocks, (unsigned long long)p.frames);
    pair_free(&p);
}

static void test_smoothed_sweep(void)
{
    P11lPair p;
    uint32_t rng = 0x4c535750u;
    uint32_t i, changes = 0u;
    int32_t decay_q16 = (int32_t)(4000u << 16);
    uint16_t previous = 0u;
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
    printf("P11L sweep changes=%u blocks=%llu\n",
           changes, (unsigned long long)p.blocks);
    pair_free(&p);
}

static void test_adversarial(void)
{
    P11lPair p;
    uint32_t rng = 0x4c414456u;
    uint32_t i;
    pair_init(&p);
    for (i = 0u; i < 256u; ++i)
        pair_render(&p, 64u, (i & 1u) ? 12000u : 4000u,
                    (i & 1u) ? 6800u : 2200u, &rng);
    printf("P11L adversarial blocks=%llu\n", (unsigned long long)p.blocks);
    pair_free(&p);
}

static void test_all_legal_decay_keys(void)
{
    P11lPair p;
    uint32_t rng = 0x4c4b4559u;
    uint32_t d;
    pair_init(&p);
    for (d = 4000u; d <= 12000u; ++d) {
        uint16_t damping_a = (uint16_t)(2200u + ((d - 4000u) % 4601u));
        uint16_t damping_b = (uint16_t)(6800u - ((d - 4000u) % 4601u));
        pair_render(&p, 1u, (uint16_t)d, damping_a, &rng);
        pair_render(&p, 1u, (uint16_t)d, damping_b, &rng);
    }
    assert(p.blocks == 16002u);
    printf("P11L all_keys blocks=%llu\n", (unsigned long long)p.blocks);
    pair_free(&p);
}

static void test_random_with_resets(void)
{
    P11lPair p;
    uint32_t rng = 0x4c524e44u;
    uint32_t i;
    pair_init(&p);
    for (i = 0u; i < 4096u; ++i) {
        uint16_t decay;
        uint16_t damping;
        uint32_t r;
        if (i && i % 257u == 0u) pair_reset(&p);
        r = lcg_next(&rng);
        if ((r & 3u) == 0u && p.production.feedback_cache_valid)
            decay = p.production.feedback_decay_ms;
        else
            decay = (uint16_t)(4000u + (lcg_next(&rng) % 8001u));
        damping = (uint16_t)(2200u + (lcg_next(&rng) % 4601u));
        pair_render(&p, 1u + (lcg_next(&rng) % 64u), decay, damping, &rng);
    }
    printf("P11L random blocks=%llu frames=%llu\n",
           (unsigned long long)p.blocks, (unsigned long long)p.frames);
    pair_free(&p);
}

int main(void)
{
    printf("P11L sizeof_ApsisSpaceFdn_host=%zu\n", sizeof(ApsisSpaceFdn));
    test_partial_reset_and_steady();
    test_smoothed_sweep();
    test_adversarial();
    test_all_legal_decay_keys();
    test_random_with_resets();
    puts("PASS: P11L production FDN cache is bit-identical to frozen always-recomputed authority");
    return 0;
}
