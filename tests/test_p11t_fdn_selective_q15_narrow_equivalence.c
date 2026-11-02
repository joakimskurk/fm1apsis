#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Frozen P11R production implementation is the authority. Static helpers are
 * intentionally visible in this proof translation unit. Repository production
 * source is not modified by P11T. */
#include "../apsis/apsis_space_dsp.c"

static uint32_t p11t_lcg(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static int32_t p11t_mul_q15_i32(int32_t a, int32_t b)
{
    int32_t p = a * b;
    if (p >= 0) p += 16384;
    else p -= 16384;
    return p >> 15;
}

static int32_t p11t_rand_bound(uint32_t *seed, int32_t bound)
{
    uint32_t span = (uint32_t)bound * 2u + 1u;
    return (int32_t)(p11t_lcg(seed) % span) - bound;
}

static void p11t_assert_safe_bound(int32_t abs_a, int32_t abs_b)
{
    int64_t product = (int64_t)abs_a * abs_b;
    assert(product <= (int64_t)INT32_MAX - 16384);
}

static void p11t_test_boundaries(const char *name,
                                  int32_t abs_a,
                                  const int32_t *b_values,
                                  size_t b_count)
{
    int32_t a_values[7];
    size_t ai, bi;
    uint64_t cases = 0u;
    a_values[0] = -abs_a;
    a_values[1] = -abs_a + 1;
    a_values[2] = -1;
    a_values[3] = 0;
    a_values[4] = 1;
    a_values[5] = abs_a - 1;
    a_values[6] = abs_a;
    for (bi = 0; bi < b_count; ++bi) {
        p11t_assert_safe_bound(abs_a, b_values[bi] < 0 ? -b_values[bi] : b_values[bi]);
        for (ai = 0; ai < sizeof(a_values) / sizeof(a_values[0]); ++ai) {
            assert(p11t_mul_q15_i32(a_values[ai], b_values[bi]) ==
                   space_mul_q15(a_values[ai], b_values[bi]));
            ++cases;
        }
    }
    printf("P11T boundary family=%s cases=%" PRIu64 "\n", name, cases);
}

static void p11t_test_helper_domain(void)
{
    static const int32_t alpha_b[] = {6040, 6041, 11167, 15310, 15311};
    static const int32_t output_b[] = {SPACE_OUTPUT_Q15};
    static const int32_t cross_b[] = {SPACE_CROSS_OTHER_Q15, SPACE_CROSS_SELF_Q15};
    static const int32_t excitation_b[] = {-SPACE_INPUT_Q15, SPACE_INPUT_Q15};
    uint32_t seed = 0x243f6a88u;
    uint64_t random_cases = 0u;
    uint32_t i;

    p11t_test_boundaries("alpha", 65534, alpha_b, sizeof(alpha_b) / sizeof(alpha_b[0]));
    p11t_test_boundaries("output", 262136, output_b, sizeof(output_b) / sizeof(output_b[0]));
    p11t_test_boundaries("cross", 65534, cross_b, sizeof(cross_b) / sizeof(cross_b[0]));
    p11t_test_boundaries("excitation", 32767, excitation_b, sizeof(excitation_b) / sizeof(excitation_b[0]));

    for (i = 0u; i < 250000u; ++i) {
        int32_t a = p11t_rand_bound(&seed, 65534);
        int32_t b = 6040 + (int32_t)(p11t_lcg(&seed) % (15311u - 6040u + 1u));
        assert(p11t_mul_q15_i32(a, b) == space_mul_q15(a, b));
        ++random_cases;
    }
    for (i = 0u; i < 250000u; ++i) {
        int32_t a = p11t_rand_bound(&seed, 262136);
        assert(p11t_mul_q15_i32(a, SPACE_OUTPUT_Q15) ==
               space_mul_q15(a, SPACE_OUTPUT_Q15));
        ++random_cases;
    }
    for (i = 0u; i < 250000u; ++i) {
        int32_t a = p11t_rand_bound(&seed, 65534);
        int32_t b = (p11t_lcg(&seed) & 1u) ? SPACE_CROSS_SELF_Q15 : SPACE_CROSS_OTHER_Q15;
        assert(p11t_mul_q15_i32(a, b) == space_mul_q15(a, b));
        ++random_cases;
    }
    for (i = 0u; i < 250000u; ++i) {
        int32_t a = p11t_rand_bound(&seed, 32767);
        int32_t b = (p11t_lcg(&seed) & 1u) ? SPACE_INPUT_Q15 : -SPACE_INPUT_Q15;
        assert(p11t_mul_q15_i32(a, b) == space_mul_q15(a, b));
        ++random_cases;
    }
    assert(random_cases == 1000000u);

    /* P11S veto: these legal structural bounds do not fit signed 32-bit with
     * the frozen rounding term. P11T must never route these families narrow. */
    assert((int64_t)262136 * SPACE_HAD_NORM_Q15 > (int64_t)INT32_MAX - 16384);
    assert((int64_t)92678 * 32291 > (int64_t)INT32_MAX - 16384);
    printf("P11T helper random_equivalence_cases=%" PRIu64
           " safe_dynamic_calls_per_frame=24 wide_dynamic_calls_per_frame=16\n",
           random_cases);
}

static void p11t_candidate_render(ApsisSpaceFdn *fdn,
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
            int32_t sine_a, sine_b;
            uint8_t sine_i, sine_frac;
            line->lfo_phase += line->lfo_phase_inc;
            sine_i = (uint8_t)(line->lfo_phase >> 24);
            if ((uint8_t)(old_phase >> 24) != sine_i) {
                line->lfo_sine_a_q15 = space_sine_point(sine_i);
                line->lfo_sine_b_q15 = space_sine_point((uint8_t)(sine_i + 1u));
            }
            sine_frac = (uint8_t)(line->lfo_phase >> 16);
            sine_a = line->lfo_sine_a_q15;
            sine_b = line->lfo_sine_b_q15;
            mod_q8 = (int32_t)(int16_t)(sine_a + (((sine_b - sine_a) * sine_frac) >> 8));
            mod_q8 = (mod_q8 * SPACE_MOD_DEPTH_Q8) >> 15;
            delayed = space_line_read(fdn, line, mod_q8);
            line->damping_state_q15 += p11t_mul_q15_i32(
                delayed - line->damping_state_q15, alpha);
            mixed[k] = line->damping_state_q15;
            out_l += (int32_t)SPACE_OUT_LEFT[k] * mixed[k];
            out_r += (int32_t)SPACE_OUT_RIGHT[k] * mixed[k];
        }

        fdn->post_left_q15 += p11t_mul_q15_i32(
            p11t_mul_q15_i32(out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha);
        fdn->post_right_q15 += p11t_mul_q15_i32(
            p11t_mul_q15_i32(out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha);

        out_stereo[frame * 2u] +=
            p11t_mul_q15_i32(fdn->post_left_q15, SPACE_CROSS_SELF_Q15) +
            p11t_mul_q15_i32(fdn->post_right_q15, SPACE_CROSS_OTHER_Q15);
        out_stereo[frame * 2u + 1u] +=
            p11t_mul_q15_i32(fdn->post_right_q15, SPACE_CROSS_SELF_Q15) +
            p11t_mul_q15_i32(fdn->post_left_q15, SPACE_CROSS_OTHER_Q15);

        space_hadamard8(mixed);
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int16_t *base = fdn->storage + line->offset;
            /* P11S vetoed these two families from narrowing. */
            int32_t feedback = space_mul_q15(mixed[k], SPACE_HAD_NORM_Q15);
            int32_t excitation = p11t_mul_q15_i32(
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
} P11tPair;

static void p11t_assert_state_equal(const P11tPair *p)
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
        assert(x->lfo_sine_b_q15 == y->lfo_sine_b_q15);
    }
    assert(memcmp(p->storage_baseline, p->storage_candidate,
                  APSIS_SPACE_FDN_STORAGE_SAMPLES * sizeof(*p->storage_baseline)) == 0);
}

static void p11t_pair_init(P11tPair *p)
{
    memset(p, 0, sizeof(*p));
    p->storage_baseline = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->storage_baseline));
    p->storage_candidate = calloc(APSIS_SPACE_FDN_STORAGE_SAMPLES, sizeof(*p->storage_candidate));
    assert(p->storage_baseline && p->storage_candidate);
    assert(apsis_space_fdn_bind(&p->baseline, p->storage_baseline,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    assert(apsis_space_fdn_bind(&p->candidate, p->storage_candidate,
                                APSIS_SPACE_FDN_STORAGE_SAMPLES));
    p11t_assert_state_equal(p);
}

static void p11t_pair_reset(P11tPair *p)
{
    apsis_space_fdn_reset(&p->baseline);
    apsis_space_fdn_reset(&p->candidate);
    p11t_assert_state_equal(p);
}

static void p11t_pair_randomize_phases(P11tPair *p, uint32_t *seed)
{
    uint8_t k;
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
        uint32_t phase = p11t_lcg(seed);
        uint8_t i = (uint8_t)(phase >> 24);
        p->baseline.line[k].lfo_phase = phase;
        p->candidate.line[k].lfo_phase = phase;
        p->baseline.line[k].lfo_sine_a_q15 = space_sine_point(i);
        p->candidate.line[k].lfo_sine_a_q15 = space_sine_point(i);
        p->baseline.line[k].lfo_sine_b_q15 = space_sine_point((uint8_t)(i + 1u));
        p->candidate.line[k].lfo_sine_b_q15 = space_sine_point((uint8_t)(i + 1u));
    }
    p11t_assert_state_equal(p);
}

static void p11t_pair_render(P11tPair *p,
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
    p11t_candidate_render(&p->candidate, input, out_b, frames, decay_ms, damping_hz);
    assert(memcmp(out_a, out_b, frames * 2u * sizeof(*out_a)) == 0);
    ++p->blocks;
    p->frames += frames;
    p11t_assert_state_equal(p);
}

static void p11t_fill_random(int32_t *input, int32_t *out, uint32_t frames, uint32_t *seed)
{
    uint32_t i;
    for (i = 0u; i < frames; ++i)
        input[i] = p11t_rand_bound(seed, 32767);
    for (i = 0u; i < frames * 2u; ++i)
        out[i] = p11t_rand_bound(seed, 1024);
}

static void p11t_run_complete_fdn(void)
{
    P11tPair p;
    uint32_t seed = 0x13198a2eu;
    int32_t input[128], initial[256];
    uint32_t block, d, decay_i;
    static const uint16_t decays[] = {4000u, 8000u, 12000u};
    uint64_t cold_blocks, cold_frames, warm_blocks, warm_frames;
    uint64_t sweep_blocks, sweep_frames, random_blocks, random_frames;
    uint64_t adversarial_blocks, adversarial_frames;

    p11t_pair_init(&p);

    for (block = 0u; block < 256u; ++block) {
        p11t_fill_random(input, initial, 64u, &seed);
        p11t_pair_render(&p, input, initial, 64u, 8000u, 4500u);
    }
    cold_blocks = p.blocks;
    cold_frames = p.frames;
    printf("P11T cold blocks=%" PRIu64 " frames=%" PRIu64 "\n", cold_blocks, cold_frames);

    for (block = 0u; block < 1024u; ++block) {
        p11t_fill_random(input, initial, 64u, &seed);
        p11t_pair_render(&p, input, initial, 64u, 8000u, 4500u);
    }
    warm_blocks = p.blocks - cold_blocks;
    warm_frames = p.frames - cold_frames;
    printf("P11T warm blocks=%" PRIu64 " frames=%" PRIu64 "\n", warm_blocks, warm_frames);

    p11t_pair_reset(&p);
    {
        uint64_t b0 = p.blocks, f0 = p.frames;
        for (d = 2200u; d <= 6800u; d += 100u) {
            for (decay_i = 0u; decay_i < sizeof(decays) / sizeof(decays[0]); ++decay_i) {
                for (block = 0u; block < 3u; ++block) {
                    p11t_fill_random(input, initial, 32u, &seed);
                    p11t_pair_render(&p, input, initial, 32u, decays[decay_i], (uint16_t)d);
                }
            }
        }
        sweep_blocks = p.blocks - b0;
        sweep_frames = p.frames - f0;
    }
    printf("P11T legal_sweep blocks=%" PRIu64 " frames=%" PRIu64 "\n",
           sweep_blocks, sweep_frames);

    {
        uint64_t b0 = p.blocks, f0 = p.frames;
        for (block = 0u; block < 4096u; ++block) {
            uint32_t frames = 1u + (p11t_lcg(&seed) % 96u);
            uint16_t decay = (uint16_t)(4000u + (p11t_lcg(&seed) % 8001u));
            uint16_t damping = (uint16_t)(2200u + (p11t_lcg(&seed) % 4601u));
            if ((p11t_lcg(&seed) % 29u) == 0u) {
                p11t_pair_reset(&p);
                p11t_pair_randomize_phases(&p, &seed);
            }
            p11t_fill_random(input, initial, frames, &seed);
            p11t_pair_render(&p, input, initial, frames, decay, damping);
        }
        random_blocks = p.blocks - b0;
        random_frames = p.frames - f0;
    }
    printf("P11T random blocks=%" PRIu64 " frames=%" PRIu64 "\n",
           random_blocks, random_frames);

    {
        uint64_t b0 = p.blocks, f0 = p.frames;
        for (block = 0u; block < 512u; ++block) {
            uint32_t i;
            uint16_t decay = (block & 1u) ? 4000u : 12000u;
            uint16_t damping = (block & 1u) ? 2200u : 6800u;
            if ((block % 37u) == 0u) {
                p11t_pair_reset(&p);
                p11t_pair_randomize_phases(&p, &seed);
            }
            for (i = 0u; i < 64u; ++i) input[i] = (i & 1u) ? -32767 : 32767;
            for (i = 0u; i < 128u; ++i) initial[i] = (i & 1u) ? -1024 : 1024;
            p11t_pair_render(&p, input, initial, 64u, decay, damping);
        }
        adversarial_blocks = p.blocks - b0;
        adversarial_frames = p.frames - f0;
    }
    printf("P11T adversarial blocks=%" PRIu64 " frames=%" PRIu64 "\n",
           adversarial_blocks, adversarial_frames);

    assert(cold_blocks == 256u && cold_frames == 16384u);
    assert(warm_blocks == 1024u && warm_frames == 65536u);
    assert(sweep_blocks == 423u && sweep_frames == 13536u);
    assert(random_blocks == 4096u);
    assert(adversarial_blocks == 512u && adversarial_frames == 32768u);
    printf("P11T complete_fdn blocks=%" PRIu64 " frames=%" PRIu64 " random_blocks=4096\n",
           p.blocks, p.frames);

    free(p.storage_baseline);
    free(p.storage_candidate);
}

int main(void)
{
    p11t_test_helper_domain();
    p11t_run_complete_fdn();
    puts("PASS: P11T selective Q15 narrow-helper candidate is bit-identical to frozen P11R authority");
    return 0;
}
