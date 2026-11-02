#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_space_dsp.h"

enum {
    P11S_MUL_ALPHA = 0,
    P11S_MUL_OUTPUT = 1,
    P11S_MUL_CROSS = 2,
    P11S_MUL_HAD_NORM = 3,
    P11S_MUL_EXCITATION = 4,
    P11S_MUL_FEEDBACK = 5,
    P11S_MUL_SITES = 6
};

typedef struct {
    uint64_t count;
    int32_t min_a, max_a;
    int32_t min_b, max_b;
    int64_t min_product, max_product;
    uint64_t max_abs_product;
} P11sMulStat;

static P11sMulStat stats[P11S_MUL_SITES];
static const char *site_name[P11S_MUL_SITES] = {
    "alpha", "output", "cross", "had_norm", "excitation", "feedback"
};
static uint32_t rng_state = 0x6a09e667u;

static uint32_t rng32(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static int32_t q15_random(void)
{
    int32_t v = (int32_t)(rng32() & 0xffffu) - 32768;
    return v == -32768 ? -32767 : v;
}

static uint64_t abs64u(int64_t v)
{
    return v < 0 ? (uint64_t)(-(v + 1)) + 1u : (uint64_t)v;
}

static void reset_stats(void)
{
    unsigned i;
    memset(stats, 0, sizeof(stats));
    for (i = 0; i < P11S_MUL_SITES; ++i) {
        stats[i].min_a = INT32_MAX;
        stats[i].max_a = INT32_MIN;
        stats[i].min_b = INT32_MAX;
        stats[i].max_b = INT32_MIN;
        stats[i].min_product = INT64_MAX;
        stats[i].max_product = INT64_MIN;
    }
}

void p11s_observe_mul(unsigned site, int32_t a, int32_t b, int64_t product)
{
    P11sMulStat *s;
    uint64_t ap;
    assert(site < P11S_MUL_SITES);
    s = &stats[site];
    ++s->count;
    if (a < s->min_a) s->min_a = a;
    if (a > s->max_a) s->max_a = a;
    if (b < s->min_b) s->min_b = b;
    if (b > s->max_b) s->max_b = b;
    if (product < s->min_product) s->min_product = product;
    if (product > s->max_product) s->max_product = product;
    ap = abs64u(product);
    if (ap > s->max_abs_product) s->max_abs_product = ap;
}

static void print_stats(const char *label, uint32_t frames)
{
    unsigned i;
    uint64_t total = 0;
    printf("P11S %s frames=%" PRIu32, label, frames);
    for (i = 0; i < P11S_MUL_SITES; ++i) total += stats[i].count;
    printf(" total_calls=%" PRIu64 "\n", total);
    for (i = 0; i < P11S_MUL_SITES; ++i) {
        const P11sMulStat *s = &stats[i];
        printf("P11S %s site=%s calls=%" PRIu64
               " a=[%" PRId32 ",%" PRId32 "] b=[%" PRId32 ",%" PRId32
               "] product=[%" PRId64 ",%" PRId64 "] max_abs=%" PRIu64 "\n",
               label, site_name[i], s->count,
               s->min_a, s->max_a, s->min_b, s->max_b,
               s->min_product, s->max_product, s->max_abs_product);
    }
}

static void fill_input(int32_t *input, uint32_t frames)
{
    uint32_t i;
    for (i = 0; i < frames; ++i) input[i] = q15_random();
}

static void render_blocks(ApsisSpaceFdn *fdn, uint32_t blocks, uint32_t frames_per_block,
                          uint16_t decay_ms, uint16_t damping_hz)
{
    int32_t input[128];
    int32_t output[256];
    uint32_t b;
    assert(frames_per_block <= 128u);
    for (b = 0; b < blocks; ++b) {
        fill_input(input, frames_per_block);
        memset(output, 0, sizeof(output));
        apsis_space_fdn_render(fdn, input, output, frames_per_block, decay_ms, damping_hz);
    }
}

static int64_t rounded_q15(int64_t product)
{
    if (product >= 0) product += 16384;
    else product -= 16384;
    return product >> 15;
}

static int fits_i32_product_and_round(uint64_t abs_product)
{
    return abs_product <= (uint64_t)INT32_MAX - 16384u;
}

static void structural_bounds(ApsisSpaceFdn *fdn)
{
    const uint64_t alpha_product = (uint64_t)65534u * 15311u;
    const uint64_t output_product = (uint64_t)(8u * 32767u) * 3277u;
    const uint64_t cross_product = (uint64_t)65534u * 28998u;
    const uint64_t excitation_product = (uint64_t)32767u * 12976u;
    const uint64_t had_product = (uint64_t)(8u * 32767u) * 11585u;
    const int64_t had_negative_out = rounded_q15(-(int64_t)had_product);
    uint16_t max_feedback = 0u;
    uint64_t feedback_product;
    int32_t input = 0, output[2] = {0, 0};
    unsigned i;

    /* Legal maximum feedback coefficient occurs at the longest decay; ask the
     * unchanged production law to materialize all eight exact coefficients. */
    apsis_space_fdn_reset(fdn);
    apsis_space_fdn_render(fdn, &input, output, 1u, 12000u, 2200u);
    for (i = 0; i < APSIS_SPACE_FDN_LINES; ++i)
        if (fdn->feedback_q15[i] > max_feedback) max_feedback = fdn->feedback_q15[i];
    feedback_product = (uint64_t)(-had_negative_out) * max_feedback;

    assert(fits_i32_product_and_round(alpha_product));
    assert(fits_i32_product_and_round(output_product));
    assert(fits_i32_product_and_round(cross_product));
    assert(fits_i32_product_and_round(excitation_product));
    assert(!fits_i32_product_and_round(had_product));
    assert(!fits_i32_product_and_round(feedback_product));

    printf("P11S structural alpha abs_a=65534 abs_b=15311 abs_product=%" PRIu64 " safe_i32=true\n", alpha_product);
    printf("P11S structural output abs_a=262136 abs_b=3277 abs_product=%" PRIu64 " safe_i32=true\n", output_product);
    printf("P11S structural cross abs_a=65534 abs_b=28998 abs_product=%" PRIu64 " safe_i32=true\n", cross_product);
    printf("P11S structural excitation abs_a=32767 abs_b=12976 abs_product=%" PRIu64 " safe_i32=true\n", excitation_product);
    printf("P11S structural had_norm abs_a=262136 abs_b=11585 abs_product=%" PRIu64 " safe_i32=false\n", had_product);
    printf("P11S structural feedback abs_a=%" PRId64 " max_legal_b=%u abs_product=%" PRIu64 " safe_i32=false\n",
           -had_negative_out, max_feedback, feedback_product);
    printf("P11S structural safe_dynamic_calls_per_frame=24 unsafe_dynamic_calls_per_frame=16\n");
}

int main(void)
{
    static int16_t storage[APSIS_SPACE_FDN_STORAGE_SAMPLES];
    ApsisSpaceFdn fdn;
    uint32_t random_frames = 0u;
    uint32_t b;
    unsigned i;

    memset(storage, 0x5a, sizeof(storage));
    memset(&fdn, 0, sizeof(fdn));
    assert(apsis_space_fdn_bind(&fdn, storage, APSIS_SPACE_FDN_STORAGE_SAMPLES));

    reset_stats();
    render_blocks(&fdn, 256u, 64u, 8000u, 4500u);
    print_stats("cold", 16384u);
    assert(stats[P11S_MUL_ALPHA].count == 163840u);
    assert(stats[P11S_MUL_OUTPUT].count == 32768u);
    assert(stats[P11S_MUL_CROSS].count == 65536u);
    assert(stats[P11S_MUL_HAD_NORM].count == 131072u);
    assert(stats[P11S_MUL_EXCITATION].count == 131072u);
    assert(stats[P11S_MUL_FEEDBACK].count == 131072u);

    reset_stats();
    render_blocks(&fdn, 1024u, 64u, 8000u, 4500u);
    print_stats("warm", 65536u);
    assert(stats[P11S_MUL_ALPHA].count == 655360u);
    assert(stats[P11S_MUL_OUTPUT].count == 131072u);
    assert(stats[P11S_MUL_CROSS].count == 262144u);
    assert(stats[P11S_MUL_HAD_NORM].count == 524288u);
    assert(stats[P11S_MUL_EXCITATION].count == 524288u);
    assert(stats[P11S_MUL_FEEDBACK].count == 524288u);

    reset_stats();
    apsis_space_fdn_reset(&fdn);
    for (i = 0; i < 47u; ++i) {
        const uint16_t damping = (uint16_t)(2200u + i * 100u);
        render_blocks(&fdn, 3u, 32u, 4000u, damping);
        render_blocks(&fdn, 3u, 32u, 8000u, damping);
        render_blocks(&fdn, 3u, 32u, 12000u, damping);
    }
    print_stats("legal_sweep", 47u * 9u * 32u);

    reset_stats();
    apsis_space_fdn_reset(&fdn);
    for (b = 0; b < 4096u; ++b) {
        int32_t input[128];
        int32_t output[256];
        uint32_t frames = 1u + (rng32() % 96u);
        uint16_t decay = (uint16_t)(4000u + (rng32() % 8001u));
        uint16_t damping = (uint16_t)(2200u + (rng32() % 4601u));
        if ((rng32() % 31u) == 0u) apsis_space_fdn_reset(&fdn);
        fill_input(input, frames);
        memset(output, 0, sizeof(output));
        apsis_space_fdn_render(&fdn, input, output, frames, decay, damping);
        random_frames += frames;
    }
    print_stats("random", random_frames);
    for (i = 0; i < P11S_MUL_SITES; ++i) assert(stats[i].count > 0u);

    structural_bounds(&fdn);

    printf("PASS: P11S FDN Q15 operand/product census and structural-width classification\n");
    return 0;
}
