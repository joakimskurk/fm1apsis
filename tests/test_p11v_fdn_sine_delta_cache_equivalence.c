#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Frozen P11R production implementation is the authority. */
#include "../apsis/apsis_space_dsp.c"

static uint32_t p11v_lcg(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static int32_t p11v_rand_bound(uint32_t *seed, int32_t bound)
{
    uint32_t span = (uint32_t)bound * 2u + 1u;
    return (int32_t)(p11v_lcg(seed) % span) - bound;
}

static int16_t p11v_delta_for_index(uint8_t i)
{
    int32_t a = space_sine_point(i);
    int32_t b = space_sine_point((uint8_t)(i + 1u));
    int32_t d = b - a;
    assert(d >= INT16_MIN && d <= INT16_MAX);
    return (int16_t)d;
}

static int16_t p11v_candidate_sine(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = space_sine_point(i);
    int32_t delta = p11v_delta_for_index(i);
    return (int16_t)(a + ((delta * frac) >> 8));
}

static void p11v_test_complete_phase_domain(void)
{
    uint32_t top16;
    uint64_t cases = 0u;
    for (top16 = 0u; top16 < 65536u; ++top16) {
        uint32_t phase = top16 << 16;
        uint8_t i = (uint8_t)(phase >> 24);
        uint8_t frac = (uint8_t)(phase >> 16);
        int32_t a = space_sine_point(i);
        int32_t b = space_sine_point((uint8_t)(i + 1u));
        int16_t baseline = (int16_t)(a + (((b - a) * frac) >> 8));
        int16_t candidate = p11v_candidate_sine(phase);
        assert(candidate == baseline);
        assert(candidate == (int16_t)(a + (((int32_t)p11v_delta_for_index(i) * frac) >> 8)));
        ++cases;
    }
    assert(cases == 65536u);
    printf("P11V top16 interpolation_equivalence_cases=%" PRIu64 "\n", cases);
}

static void p11v_test_index_transitions(void)
{
    uint32_t old_i, new_i;
    uint64_t cases = 0u;
    uint64_t refreshes = 0u, reuses = 0u;
    for (old_i = 0u; old_i < 256u; ++old_i) {
        int16_t base_a0 = space_sine_point((uint8_t)old_i);
        int16_t base_b0 = space_sine_point((uint8_t)(old_i + 1u));
        int16_t cand_a0 = base_a0;
        int16_t cand_d0 = (int16_t)(base_b0 - base_a0);
        for (new_i = 0u; new_i < 256u; ++new_i) {
            int16_t base_a = base_a0, base_b = base_b0;
            int16_t cand_a = cand_a0, cand_d = cand_d0;
            if (old_i != new_i) {
                base_a = space_sine_point((uint8_t)new_i);
                base_b = space_sine_point((uint8_t)(new_i + 1u));
                cand_a = space_sine_point((uint8_t)new_i);
                cand_d = p11v_delta_for_index((uint8_t)new_i);
                ++refreshes;
            } else {
                ++reuses;
            }
            assert(cand_a == base_a);
            assert((int32_t)cand_d == (int32_t)base_b - base_a);
            assert((int32_t)cand_a + cand_d == base_b);
            ++cases;
        }
    }
    assert(cases == 65536u);
    assert(refreshes == 65280u);
    assert(reuses == 256u);
    printf("P11V index_transition_cases=%" PRIu64
           " refreshes=%" PRIu64 " reuses=%" PRIu64 " incl_255_to_0=true\n",
           cases, refreshes, reuses);
}

static void p11v_candidate_reset(ApsisSpaceFdn *fdn)
{
    uint8_t k;
    apsis_space_fdn_reset(fdn);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        ApsisSpaceFdnLine *line = &fdn->line[k];
        line->lfo_sine_b_q15 = (int16_t)(
            (int32_t)line->lfo_sine_b_q15 - line->lfo_sine_a_q15);
    }
}

static uint8_t p11v_candidate_bind(ApsisSpaceFdn *fdn,
                                   int16_t *storage,
                                   uint32_t storage_samples)
{
    if (!fdn) return 0u;
    fdn->storage = storage;
    fdn->storage_samples = storage_samples;
    fdn->storage_bound =
        (storage && storage_samples >= APSIS_SPACE_FDN_STORAGE_SAMPLES) ? 1u : 0u;
    p11v_candidate_reset(fdn);
    return fdn->storage_bound;
}

static void p11v_candidate_render(ApsisSpaceFdn *fdn,
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
            uint32_t old_phase = line->lfo_phase;
            int32_t mod_q8, delayed;
            int32_t sine_a, sine_delta;
            uint8_t sine_i, sine_frac;
            line->lfo_phase += line->lfo_phase_inc;
            sine_i = (uint8_t)(line->lfo_phase >> 24);
            if ((uint8_t)(old_phase >> 24) != sine_i) {
                int32_t a = space_sine_point(sine_i);
                int32_t b = space_sine_point((uint8_t)(sine_i + 1u));
                line->lfo_sine_a_q15 = (int16_t)a;
                line->lfo_sine_b_q15 = (int16_t)(b - a);
            }
            sine_frac = (uint8_t)(line->lfo_phase >> 16);
            sine_a = line->lfo_sine_a_q15;
            sine_delta = line->lfo_sine_b_q15;
            mod_q8 = (int32_t)(int16_t)(sine_a + ((sine_delta * sine_frac) >> 8));
            mod_q8 = (mod_q8 * SPACE_MOD_DEPTH_Q8) >> 15;
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

typedef struct {
    ApsisSpaceFdn baseline;
    ApsisSpaceFdn candidate;
    int16_t *storage_baseline;
    int16_t *storage_candidate;
    uint64_t blocks;
    uint64_t frames;
} P11vPair;

static void p11v_assert_semantic_state_equal(const P11vPair *p)
{
    const ApsisSpaceFdn *a = &p->baseline;
    const ApsisSpaceFdn *b = &p->candidate;
    uint8_t k;
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
        assert(x->lfo_sine_a_q15 == y->lfo_sine_a_q15);
        assert((int32_t)y->lfo_sine_b_q15 ==
               (int32_t)x->lfo_sine_b_q15 - x->lfo_sine_a_q15);
        assert((int32_t)y->lfo_sine_a_q15 + y->lfo_sine_b_q15 ==
               x->lfo_sine_b_q15);
    }
    assert(memcmp(p->storage_baseline, p->storage_candidate,
                  APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*p->storage_baseline)) == 0);
}

static void p11v_pair_init(P11vPair *p)
{
    memset(p, 0, sizeof(*p));
    p->storage_baseline = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->storage_baseline));
    p->storage_candidate = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->storage_candidate));
    assert(p->storage_baseline && p->storage_candidate);
    assert(apsis_space_fdn_bind(&p->baseline, p->storage_baseline,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(p11v_candidate_bind(&p->candidate, p->storage_candidate,
                               APSIS_SPACE_FDN_STORAGE_SAMPLES));
    p11v_assert_semantic_state_equal(p);
}

static void p11v_pair_reset(P11vPair *p)
{
    apsis_space_fdn_reset(&p->baseline);
    p11v_candidate_reset(&p->candidate);
    p11v_assert_semantic_state_equal(p);
}

static void p11v_pair_randomize_phases(P11vPair *p, uint32_t *seed)
{
    uint8_t k;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        uint32_t phase = p11v_lcg(seed);
        uint8_t i = (uint8_t)(phase >> 24);
        int16_t a = space_sine_point(i);
        int16_t bb = space_sine_point((uint8_t)(i + 1u));
        p->baseline.line[k].lfo_phase = phase;
        p->candidate.line[k].lfo_phase = phase;
        p->baseline.line[k].lfo_sine_a_q15 = a;
        p->baseline.line[k].lfo_sine_b_q15 = bb;
        p->candidate.line[k].lfo_sine_a_q15 = a;
        p->candidate.line[k].lfo_sine_b_q15 = (int16_t)(bb - a);
    }
    p11v_assert_semantic_state_equal(p);
}

static void p11v_pair_render(P11vPair *p,
                             const int32_t *input,
                             const int32_t *initial_output,
                             uint32_t frames,
                             uint16_t decay_ms,
                             uint16_t damping_hz)
{
    int32_t out_a[256], out_b[256];
    assert(frames <= 128u);
    if (initial_output) {
        memcpy(out_a, initial_output, frames * 2u * sizeof(*out_a));
        memcpy(out_b, initial_output, frames * 2u * sizeof(*out_b));
    } else {
        memset(out_a, 0, frames * 2u * sizeof(*out_a));
        memset(out_b, 0, frames * 2u * sizeof(*out_b));
    }
    apsis_space_fdn_render(&p->baseline, input, out_a, frames, decay_ms, damping_hz);
    p11v_candidate_render(&p->candidate, input, out_b, frames, decay_ms, damping_hz);
    assert(memcmp(out_a, out_b, frames * 2u * sizeof(*out_a)) == 0);
    ++p->blocks;
    p->frames += frames;
    p11v_assert_semantic_state_equal(p);
}

static void p11v_fill_random(int32_t *input, int32_t *out,
                             uint32_t frames, uint32_t *seed)
{
    uint32_t i;
    for (i = 0u; i < frames; ++i) input[i] = p11v_rand_bound(seed, 32767);
    for (i = 0u; i < frames * 2u; ++i) out[i] = p11v_rand_bound(seed, 1024);
}

static void p11v_run_complete_fdn(void)
{
    P11vPair p;
    uint32_t seed = 0x9e3779b9u;
    int32_t input[128], initial[256];
    uint32_t block, d, decay_i;
    static const uint16_t decays[] = {4000u, 8000u, 12000u};
    uint64_t cold_blocks, cold_frames, warm_blocks, warm_frames;
    uint64_t sweep_blocks, sweep_frames, random_blocks, random_frames;
    uint64_t adversarial_blocks, adversarial_frames;

    p11v_pair_init(&p);
    assert(sizeof(ApsisSpaceFdnLine) == 28u);
    assert(sizeof(ApsisSpaceFdn) == 264u);

    for (block = 0u; block < 256u; ++block) {
        p11v_fill_random(input, initial, 64u, &seed);
        p11v_pair_render(&p, input, initial, 64u, 8000u, 4500u);
    }
    cold_blocks = p.blocks;
    cold_frames = p.frames;
    printf("P11V cold blocks=%" PRIu64 " frames=%" PRIu64 "\n", cold_blocks, cold_frames);

    for (block = 0u; block < 1024u; ++block) {
        p11v_fill_random(input, initial, 64u, &seed);
        p11v_pair_render(&p, input, initial, 64u, 8000u, 4500u);
    }
    warm_blocks = p.blocks - cold_blocks;
    warm_frames = p.frames - cold_frames;
    printf("P11V warm blocks=%" PRIu64 " frames=%" PRIu64 "\n", warm_blocks, warm_frames);

    p11v_pair_reset(&p);
    {
        uint64_t b0 = p.blocks, f0 = p.frames;
        for (d = 2200u; d <= 6800u; d += 100u) {
            for (decay_i = 0u; decay_i < sizeof(decays) / sizeof(decays[0]); ++decay_i) {
                for (block = 0u; block < 3u; ++block) {
                    p11v_fill_random(input, initial, 32u, &seed);
                    p11v_pair_render(&p, input, initial, 32u,
                                     decays[decay_i], (uint16_t)d);
                }
            }
        }
        sweep_blocks = p.blocks - b0;
        sweep_frames = p.frames - f0;
    }
    printf("P11V legal_sweep blocks=%" PRIu64 " frames=%" PRIu64 "\n",
           sweep_blocks, sweep_frames);

    {
        uint64_t b0 = p.blocks, f0 = p.frames;
        for (block = 0u; block < 4096u; ++block) {
            uint32_t frames = 1u + (p11v_lcg(&seed) % 96u);
            uint16_t decay = (uint16_t)(4000u + (p11v_lcg(&seed) % 8001u));
            uint16_t damping = (uint16_t)(2200u + (p11v_lcg(&seed) % 4601u));
            if ((p11v_lcg(&seed) % 29u) == 0u) {
                p11v_pair_reset(&p);
                p11v_pair_randomize_phases(&p, &seed);
            }
            p11v_fill_random(input, initial, frames, &seed);
            p11v_pair_render(&p, input, initial, frames, decay, damping);
        }
        random_blocks = p.blocks - b0;
        random_frames = p.frames - f0;
    }
    printf("P11V random blocks=%" PRIu64 " frames=%" PRIu64 "\n",
           random_blocks, random_frames);

    {
        uint64_t b0 = p.blocks, f0 = p.frames;
        for (block = 0u; block < 512u; ++block) {
            uint32_t i;
            uint16_t decay = (block & 1u) ? 4000u : 12000u;
            uint16_t damping = (block & 1u) ? 2200u : 6800u;
            if ((block % 37u) == 0u) {
                p11v_pair_reset(&p);
                p11v_pair_randomize_phases(&p, &seed);
            }
            for (i = 0u; i < 64u; ++i) input[i] = (i & 1u) ? -32767 : 32767;
            for (i = 0u; i < 128u; ++i) initial[i] = (i & 1u) ? -1024 : 1024;
            p11v_pair_render(&p, input, initial, 64u, decay, damping);
        }
        adversarial_blocks = p.blocks - b0;
        adversarial_frames = p.frames - f0;
    }
    printf("P11V adversarial blocks=%" PRIu64 " frames=%" PRIu64 "\n",
           adversarial_blocks, adversarial_frames);

    assert(cold_blocks == 256u && cold_frames == 16384u);
    assert(warm_blocks == 1024u && warm_frames == 65536u);
    assert(sweep_blocks == 423u && sweep_frames == 13536u);
    assert(random_blocks == 4096u);
    assert(adversarial_blocks == 512u && adversarial_frames == 32768u);
    printf("P11V complete_fdn blocks=%" PRIu64 " frames=%" PRIu64
           " random_blocks=4096 sizeof_line=%zu sizeof_fdn=%zu\n",
           p.blocks, p.frames, sizeof(ApsisSpaceFdnLine), sizeof(ApsisSpaceFdn));

    free(p.storage_baseline);
    free(p.storage_candidate);
}

int main(void)
{
    p11v_test_complete_phase_domain();
    p11v_test_index_transitions();
    p11v_run_complete_fdn();
    puts("PASS: P11V cached sine A+delta candidate is bit-identical to frozen P11R authority");
    return 0;
}
