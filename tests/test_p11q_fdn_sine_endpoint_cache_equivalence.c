#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Frozen P11O production implementation is the authority. Static helpers are
 * intentionally visible in this proof translation unit. Production sources
 * themselves are not modified by P11Q. */
#include "../apsis/apsis_space_dsp.c"

typedef struct {
    int16_t a_q15[APSIS_SPACE_FDN_LINES];
    int16_t b_q15[APSIS_SPACE_FDN_LINES];
    uint64_t refreshes;
} P11qSineCache;

typedef struct {
    ApsisSpaceFdn fdn;
    P11qSineCache sine;
} P11qCandidate;

static uint32_t lcg_next(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void p11q_cache_line(P11qSineCache *cache, uint8_t line, uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    assert(cache && line < APSIS_SPACE_FDN_LINES);
    cache->a_q15[line] = space_sine_point(i);
    cache->b_q15[line] = space_sine_point((uint8_t)(i + 1u));
}

static void p11q_cache_init(P11qCandidate *candidate)
{
    uint8_t k;
    assert(candidate);
    candidate->sine.refreshes = 0u;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        p11q_cache_line(&candidate->sine, k, candidate->fdn.line[k].lfo_phase);
}

static int16_t p11q_sine_advance(P11qSineCache *cache,
                                 uint8_t line,
                                 uint32_t old_phase,
                                 uint32_t new_phase)
{
    uint8_t old_i = (uint8_t)(old_phase >> 24);
    uint8_t new_i = (uint8_t)(new_phase >> 24);
    uint8_t frac = (uint8_t)(new_phase >> 16);
    int32_t a, b;
    assert(cache && line < APSIS_SPACE_FDN_LINES);
    if (old_i != new_i) {
        p11q_cache_line(cache, line, new_phase);
        cache->refreshes++;
    }
    a = cache->a_q15[line];
    b = cache->b_q15[line];
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static void p11q_candidate_reset(P11qCandidate *candidate)
{
    assert(candidate);
    apsis_space_fdn_reset(&candidate->fdn);
    p11q_cache_init(candidate);
}

static void p11q_candidate_render(P11qCandidate *candidate,
                                  const int32_t *input_mono_q15,
                                  int32_t *out_stereo,
                                  uint32_t frames,
                                  uint16_t decay_ms,
                                  uint16_t damping_hz)
{
    ApsisSpaceFdn *fdn;
    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    assert(candidate);
    fdn = &candidate->fdn;
    if (!fdn->storage_bound || !out_stereo) return;
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
            uint32_t old_phase = line->lfo_phase;
            int32_t mod_q8, delayed;
            line->lfo_phase += line->lfo_phase_inc;
            mod_q8 = ((int32_t)p11q_sine_advance(
                &candidate->sine, k, old_phase, line->lfo_phase) *
                SPACE_MOD_DEPTH_Q8) >> 15;
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
            feedback = space_mul_q15(feedback, fdn->feedback_q15[k]);
            write_value = space_clamp_q15((int64_t)excitation + feedback);
            base[line->write_pos] = (int16_t)write_value;
            line->write_pos++;
            if (line->write_pos >= line->capacity) line->write_pos = 0u;
            if (line->fill_count < line->capacity) line->fill_count++;
        }
    }
}

static void assert_cache_exact(const P11qCandidate *candidate)
{
    uint8_t k;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        uint8_t i = (uint8_t)(candidate->fdn.line[k].lfo_phase >> 24);
        assert(candidate->sine.a_q15[k] == space_sine_point(i));
        assert(candidate->sine.b_q15[k] == space_sine_point((uint8_t)(i + 1u)));
    }
}

static void assert_legacy_state_equal(const ApsisSpaceFdn *a,
                                      const ApsisSpaceFdn *b,
                                      const int16_t *storage_a,
                                      const int16_t *storage_b)
{
    uint8_t k;
    assert(a->storage_samples == b->storage_samples);
    assert(a->storage_bound == b->storage_bound);
    assert(a->post_left_q15 == b->post_left_q15);
    assert(a->post_right_q15 == b->post_right_q15);
    assert(a->feedback_decay_ms == b->feedback_decay_ms);
    assert(a->feedback_cache_valid == b->feedback_cache_valid);
    assert(memcmp(a->feedback_q15, b->feedback_q15,
                  sizeof(a->feedback_q15)) == 0);
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

static void test_complete_top16_domain(void)
{
    P11qSineCache cache;
    uint32_t i, frac;
    memset(&cache, 0, sizeof(cache));
    for (i = 0u; i < 256u; ++i) {
        uint32_t old_phase = i << 24;
        p11q_cache_line(&cache, 0u, old_phase);
        for (frac = 0u; frac < 256u; ++frac) {
            uint32_t phase = (i << 24) | (frac << 16);
            assert(p11q_sine_advance(&cache, 0u, old_phase, phase) ==
                   space_sine_q15(phase));
        }
    }
    puts("P11Q top16 helper equivalence cases=65536");
}

static void test_all_index_transitions(void)
{
    static const uint8_t fracs[] = {0u, 1u, 127u, 255u};
    P11qSineCache cache;
    uint32_t old_i, new_i, f;
    uint64_t cases = 0u;
    memset(&cache, 0, sizeof(cache));
    for (old_i = 0u; old_i < 256u; ++old_i) {
        for (new_i = 0u; new_i < 256u; ++new_i) {
            uint32_t old_phase = old_i << 24;
            p11q_cache_line(&cache, 0u, old_phase);
            for (f = 0u; f < sizeof(fracs) / sizeof(fracs[0]); ++f) {
                uint32_t new_phase = (new_i << 24) | ((uint32_t)fracs[f] << 16);
                assert(p11q_sine_advance(&cache, 0u, old_phase, new_phase) ==
                       space_sine_q15(new_phase));
                cases++;
                /* Restore the old cache so every fractional case tests the
                 * same old->new transition independently. */
                p11q_cache_line(&cache, 0u, old_phase);
            }
        }
    }
    assert(cases == 262144u);
    puts("P11Q index transition equivalence cases=262144 incl_255_to_0=true");
}

typedef struct {
    ApsisSpaceFdn baseline;
    P11qCandidate candidate;
    int16_t *storage_baseline;
    int16_t *storage_candidate;
    uint64_t blocks;
    uint64_t frames;
} P11qPair;

static void pair_init(P11qPair *p)
{
    memset(p, 0, sizeof(*p));
    p->storage_baseline = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES,
                                 sizeof(*p->storage_baseline));
    p->storage_candidate = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES,
                                  sizeof(*p->storage_candidate));
    assert(p->storage_baseline && p->storage_candidate);
    assert(apsis_space_fdn_bind(&p->baseline, p->storage_baseline,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(apsis_space_fdn_bind(&p->candidate.fdn, p->storage_candidate,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    p11q_cache_init(&p->candidate);
    assert_legacy_state_equal(&p->baseline, &p->candidate.fdn,
                              p->storage_baseline, p->storage_candidate);
    assert_cache_exact(&p->candidate);
}

static void pair_reset(P11qPair *p)
{
    apsis_space_fdn_reset(&p->baseline);
    p11q_candidate_reset(&p->candidate);
    assert_legacy_state_equal(&p->baseline, &p->candidate.fdn,
                              p->storage_baseline, p->storage_candidate);
    assert_cache_exact(&p->candidate);
}

static void pair_randomize_phases(P11qPair *p, uint32_t *rng)
{
    uint8_t k;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        uint32_t phase = lcg_next(rng);
        p->baseline.line[k].lfo_phase = phase;
        p->candidate.fdn.line[k].lfo_phase = phase;
    }
    p11q_cache_init(&p->candidate);
    assert_legacy_state_equal(&p->baseline, &p->candidate.fdn,
                              p->storage_baseline, p->storage_candidate);
    assert_cache_exact(&p->candidate);
}

static void pair_render(P11qPair *p,
                        uint32_t frames,
                        uint16_t decay_ms,
                        uint16_t damping_hz,
                        uint32_t *rng)
{
    int32_t input[64];
    int32_t out_a[128];
    int32_t out_b[128];
    uint32_t i;
    assert(frames >= 1u && frames <= 64u);
    for (i = 0u; i < frames; ++i)
        input[i] = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 2;
    for (i = 0u; i < frames * 2u; ++i) {
        int32_t seed = (int32_t)((int16_t)(lcg_next(rng) >> 16)) / 8;
        out_a[i] = seed;
        out_b[i] = seed;
    }
    apsis_space_fdn_render(&p->baseline, input, out_a, frames,
                           decay_ms, damping_hz);
    p11q_candidate_render(&p->candidate, input, out_b, frames,
                          decay_ms, damping_hz);
    assert(memcmp(out_a, out_b, frames * 2u * sizeof(*out_a)) == 0);
    assert_legacy_state_equal(&p->baseline, &p->candidate.fdn,
                              p->storage_baseline, p->storage_candidate);
    assert_cache_exact(&p->candidate);
    p->blocks++;
    p->frames += frames;
}

static void pair_free(P11qPair *p)
{
    free(p->storage_baseline);
    free(p->storage_candidate);
}

static void test_full_fdn_equivalence(void)
{
    P11qPair p;
    uint32_t rng = 0x51314c46u;
    uint32_t i;
    uint64_t cold_refreshes, warm_refreshes;
    pair_init(&p);

    for (i = 0u; i < 256u; ++i)
        pair_render(&p, 64u, 8000u, 4500u, &rng);
    assert(p.frames == 16384u);
    cold_refreshes = p.candidate.sine.refreshes;
    printf("P11Q cold blocks=256 frames=16384 refreshes=%llu\n",
           (unsigned long long)cold_refreshes);

    p.candidate.sine.refreshes = 0u;
    for (i = 0u; i < 1024u; ++i)
        pair_render(&p, 64u, 8000u,
                    (uint16_t)(2200u + (i % 4601u)), &rng);
    warm_refreshes = p.candidate.sine.refreshes;
    printf("P11Q warm blocks=1024 frames=65536 refreshes=%llu\n",
           (unsigned long long)warm_refreshes);

    for (i = 0u; i < 4096u; ++i) {
        uint32_t r = lcg_next(&rng);
        uint32_t frames = 1u + (lcg_next(&rng) % 64u);
        uint16_t decay = (uint16_t)(4000u + (lcg_next(&rng) % 8001u));
        uint16_t damping = (uint16_t)(2200u + (lcg_next(&rng) % 4601u));
        if (i && (r % 257u) == 0u) pair_reset(&p);
        if (i && (r % 193u) == 0u) pair_randomize_phases(&p, &rng);
        pair_render(&p, frames, decay, damping, &rng);
    }
    printf("P11Q full_fdn blocks=%llu frames=%llu random_blocks=4096\n",
           (unsigned long long)p.blocks, (unsigned long long)p.frames);
    pair_free(&p);
}

int main(void)
{
    printf("P11Q sizeof_ApsisSpaceFdn_host=%zu sizeof_external_candidate=%zu delta=%zu\n",
           sizeof(ApsisSpaceFdn), sizeof(P11qCandidate),
           sizeof(P11qCandidate) - sizeof(ApsisSpaceFdn));
    assert(sizeof(P11qCandidate) - sizeof(ApsisSpaceFdn) == 32u + sizeof(uint64_t));
    /* The proof wrapper includes an 8-byte refresh counter that is not part of
     * the proposed production state. The actual proposed cache payload is 32 B. */
    test_complete_top16_domain();
    test_all_index_transitions();
    test_full_fdn_equivalence();
    puts("PASS: P11Q FDN sine endpoint-cache proof is bit-identical to frozen P11O authority");
    return 0;
}
