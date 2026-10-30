#include "apsis_space_room.h"

#define ROOM_Q15_ONE 32767
#define ROOM_TRIM_Q15 23592
#define ROOM_EARLY_BUS_Q15 5243
#define ROOM_COMP_ATTACK_Q15 492
#define ROOM_COMP_RELEASE_Q15 7
#define ROOM_SMOOTH_40MS_Q15 19
#define ROOM_SMOOTH_50MS_Q15 15
#define ROOM_SMOOTH_80MS_Q15 9
#define ROOM_SMOOTH_100MS_Q15 7

#define ROOM_GUARD_LP_MIN_HZ 3800u
#define ROOM_GUARD_LP_MAX_HZ 5600u
#define ROOM_TAIL_DAMP_MIN_HZ 2200u
#define ROOM_TAIL_DAMP_MAX_HZ 6800u

/* Study-126 early reflections in Q16 frames at 44.1 kHz. */
static const uint32_t ROOM_EARLY_DELAY_Q16[APSIS_SPACE_EARLY_TAPS] = {
    60692890u, 124275917u, 205199770u, 315024998u
};
static const int16_t ROOM_EARLY_GAIN_Q15[APSIS_SPACE_EARLY_TAPS] = {
    3277, 2458, 1704, 1180
};

typedef struct {
    int32_t b0;
    int32_t b1;
    int32_t b2;
    int32_t a1;
    int32_t a2;
} SpaceRoomBiquadCoeff;

/* RBJ coefficients in Q30 at 44.1 kHz. */
static const SpaceRoomBiquadCoeff ROOM_GUARD_HP_170 = {
    1037682714, -2075365429, 1037682714, -2075061020, 1001928014
};
static const SpaceRoomBiquadCoeff ROOM_TAIL_HP_190 = {
    1042201374, -2084402747, 1042201374, -2084020834, 1011042836
};
static const SpaceRoomBiquadCoeff ROOM_ECHO_LP_4200 = {
    40910607, 81821215, 40910607, -778123657, -131975737
};

/* Guard LP, Q=.22, 3800..5600 Hz in 100-Hz steps. The cutoff target itself is
 * smoothed before table selection, preserving Study-126's ~100-ms update role
 * without runtime trig/libm. */
static const SpaceRoomBiquadCoeff ROOM_GUARD_LP[19] = {
    {35362936,70725872,35362936,-847609204,-84680877},
    {36732462,73464924,36732462,-829702948,-97109028},
    {38113746,76227493,38113746,-812163552,-109123287},
    {39506537,79013074,39506537,-794975390,-120740286},
    {40910607,81821215,40910607,-778123657,-131975737},
    {42325752,84651504,42325752,-761594320,-142844495},
    {43751788,87503575,43751788,-745374060,-153360613},
    {45188550,90377101,45188550,-729450230,-163537393},
    {46635895,93271790,46635895,-713810811,-173387433},
    {48093695,96187389,48093695,-698444369,-182922676},
    {49561838,99123677,49561838,-683340024,-192154447},
    {51040232,102080464,51040232,-668487408,-201093489},
    {52528795,105057590,52528795,-653876641,-209750003},
    {54027463,108054926,54027463,-639498294,-218133678},
    {55536184,111072367,55536184,-625343370,-226253720},
    {57054918,114109836,57054918,-611403269,-234118882},
    {58583640,117167281,58583640,-597669773,-241737489},
    {60122336,120244671,60122336,-584135018,-249117464},
    {61671000,123342001,61671000,-570791477,-256266346}
};

/* Static soft-knee compressor transfer for threshold -22 dB, knee 16 dB,
 * ratio 3.2:1. It approximates the WebAudio guard's safety transfer while the
 * 1.5-ms/110-ms gain smoothing below preserves its attack/release role. */
static const uint16_t ROOM_COMP_GAIN_Q15[257] = {
32767,32767,32767,32767,32767,32767,32767,32767,32767,32698,32495,32197,31833,31424,30983,30522,
30048,29568,29086,28605,28128,27657,27192,26734,26286,25846,25416,24995,24583,24182,23789,23406,
23032,22668,22312,21965,21626,21295,20973,20658,20351,20052,19759,19473,19194,18922,18656,18396,
18142,17894,17651,17414,17183,16959,16743,16533,16329,16132,15940,15754,15573,15397,15226,15059,
14897,14739,14585,14435,14289,14146,14007,13871,13738,13609,13482,13358,13237,13119,13003,12889,
12778,12670,12563,12459,12357,12257,12159,12062,11968,11875,11784,11695,11608,11522,11437,11354,
11273,11193,11114,11037,10961,10886,10813,10741,10669,10599,10531,10463,10396,10330,10266,10202,
10139,10078,10017,9957,9898,9840,9782,9726,9670,9615,9560,9507,9454,9402,9351,9300,
9250,9201,9152,9104,9056,9010,8963,8918,8872,8828,8784,8740,8697,8655,8613,8572,
8531,8490,8450,8410,8371,8333,8294,8257,8219,8182,8146,8110,8074,8038,8003,7969,
7934,7901,7867,7834,7801,7768,7736,7704,7673,7642,7611,7580,7550,7520,7490,7460,
7431,7402,7374,7345,7317,7289,7262,7235,7208,7181,7154,7128,7102,7076,7050,7025,
7000,6975,6950,6926,6901,6877,6853,6830,6806,6783,6760,6737,6714,6691,6669,6647,
6625,6603,6582,6560,6539,6518,6497,6476,6455,6435,6415,6394,6374,6355,6335,6315,
6296,6277,6258,6239,6220,6201,6182,6164,6146,6128,6110,6092,6074,6056,6039,6021,
6004,5987,5970,5953,5936,5920,5903,5887,5870,5854,5838,5822,5806,5790,5775,5759,
5744
};

static int32_t space_room_clamp_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

static int32_t space_room_abs_i32(int32_t v)
{
    if (v == INT32_MIN) return INT32_MAX;
    return v < 0 ? -v : v;
}

static int32_t space_room_mul_q15(int32_t a, int32_t b)
{
    int64_t p = (int64_t)a * b;
    if (p >= 0) p += 16384;
    else p -= 16384;
    return (int32_t)(p >> 15);
}

static int32_t space_room_shift_q30(int64_t v)
{
    if (v >= 0) v += (int64_t)1 << 29;
    else v -= (int64_t)1 << 29;
    return (int32_t)(v >> 30);
}

static int32_t space_room_biquad(ApsisSpaceBiquadState *s,
                                 const SpaceRoomBiquadCoeff *c,
                                 int32_t x)
{
    int32_t y;
    int64_t z;
    if (!s || !c) return x;
    y = space_room_shift_q30((int64_t)c->b0 * x + ((int64_t)s->z1_q15 << 30));
    z = (int64_t)c->b1 * x - (int64_t)c->a1 * y + ((int64_t)s->z2_q15 << 30);
    s->z1_q15 = space_room_shift_q30(z);
    z = (int64_t)c->b2 * x - (int64_t)c->a2 * y;
    s->z2_q15 = space_room_shift_q30(z);
    return y;
}

/* Exact rounded Q16 frames for integer milliseconds at 44.1 kHz using only
 * 32-bit arithmetic: 44.1 * 65536 = 44*65536 + 6553.6. */
static uint32_t space_room_ms_q16(uint16_t ms)
{
    return (uint32_t)ms * 2883584u + (uint32_t)ms * 6553u +
           ((uint32_t)ms * 3u + 2u) / 5u;
}

static uint16_t space_room_clamp_u16(uint16_t v, uint16_t lo, uint16_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int32_t space_room_smooth_i32(int32_t cur, int32_t target, uint16_t alpha)
{
    int64_t delta = (int64_t)target - cur;
    int64_t step;
    if (!delta) return cur;
    step = (delta * alpha) >> 15;
    if (!step) step = delta > 0 ? 1 : -1;
    return (int32_t)((int64_t)cur + step);
}

static uint32_t space_room_smooth_u32(uint32_t cur, uint32_t target, uint16_t alpha)
{
    int64_t delta = (int64_t)target - cur;
    int64_t step;
    if (!delta) return cur;
    step = (delta * alpha) >> 15;
    if (!step) step = delta > 0 ? 1 : -1;
    return (uint32_t)((int64_t)cur + step);
}

static const SpaceRoomBiquadCoeff *space_room_guard_lp_coeff(int32_t hz_q16)
{
    uint32_t hz, index;
    if (hz_q16 <= 0) return &ROOM_GUARD_LP[0];
    hz = ((uint32_t)hz_q16 + 32768u) >> 16;
    if (hz <= ROOM_GUARD_LP_MIN_HZ) return &ROOM_GUARD_LP[0];
    if (hz >= ROOM_GUARD_LP_MAX_HZ) return &ROOM_GUARD_LP[18];
    index = (hz - ROOM_GUARD_LP_MIN_HZ + 50u) / 100u;
    if (index > 18u) index = 18u;
    return &ROOM_GUARD_LP[index];
}

static int32_t space_room_stereo_read(const int16_t *storage,
                                      uint32_t capacity_frames,
                                      uint32_t write_frame,
                                      uint32_t fill_frames,
                                      uint32_t delay_q16,
                                      uint8_t channel)
{
    uint32_t needed, span_q16, write_q16, read_q16, i0, i1, frac;
    int32_t a, b;
    int64_t mixed;
    if (!storage || !capacity_frames || channel > 1u) return 0;
    needed = (delay_q16 + 65535u) >> 16;
    if (!needed || fill_frames < needed) return 0;
    span_q16 = capacity_frames << 16;
    write_q16 = write_frame << 16;
    if (write_q16 >= delay_q16) read_q16 = write_q16 - delay_q16;
    else read_q16 = span_q16 - (delay_q16 - write_q16);
    if (read_q16 >= span_q16) read_q16 -= span_q16;
    i0 = read_q16 >> 16;
    frac = read_q16 & 65535u;
    i1 = i0 + 1u;
    if (i1 >= capacity_frames) i1 = 0u;
    a = storage[i0 * 2u + channel];
    if (!frac) return a;
    b = storage[i1 * 2u + channel];
    mixed = (int64_t)a * (65536u - frac) + (int64_t)b * frac;
    return (int32_t)(mixed >> 16);
}

static void space_room_stereo_write(int16_t *storage,
                                    uint32_t capacity_frames,
                                    uint32_t *write_frame,
                                    uint32_t *fill_frames,
                                    int32_t left,
                                    int32_t right)
{
    uint32_t w;
    if (!storage || !capacity_frames || !write_frame || !fill_frames) return;
    w = *write_frame;
    storage[w * 2u] = (int16_t)space_room_clamp_q15(left);
    storage[w * 2u + 1u] = (int16_t)space_room_clamp_q15(right);
    w++;
    if (w >= capacity_frames) w = 0u;
    *write_frame = w;
    if (*fill_frames < capacity_frames) (*fill_frames)++;
}

static int32_t space_room_compressor(ApsisSpaceRoom *room, int32_t left,
                                     int32_t right, uint8_t channel)
{
    int32_t al, ar, peak, desired, alpha;
    uint32_t index;
    if (!room) return channel ? right : left;
    al = space_room_abs_i32(left);
    ar = space_room_abs_i32(right);
    peak = al > ar ? al : ar;
    if (peak > 32767) peak = 32767;
    index = ((uint32_t)peak + 64u) >> 7;
    if (index > 256u) index = 256u;
    desired = ROOM_COMP_GAIN_Q15[index];
    alpha = desired < room->compressor_gain_q15
        ? ROOM_COMP_ATTACK_Q15 : ROOM_COMP_RELEASE_Q15;
    room->compressor_gain_q15 = space_room_smooth_i32(
        room->compressor_gain_q15, desired, (uint16_t)alpha);
    return space_room_clamp_q15(space_room_mul_q15(
        channel ? right : left, room->compressor_gain_q15));
}

static void space_room_targets(ApsisSpaceRoom *room, const ApsisSpaceState *state)
{
    uint16_t mix, decay, predelay, damping, echo_time, feedback;
    uint32_t guard_hz;
    if (!room || !state) return;
    mix = state->mix_q15 > APSIS_Q15_MILLI(720) ? APSIS_Q15_MILLI(720) : state->mix_q15;
    decay = space_room_clamp_u16(state->decay_ms, 4000u, 12000u);
    predelay = space_room_clamp_u16(state->predelay_ms, 15u, 140u);
    damping = space_room_clamp_u16(state->damping_hz,
                                   ROOM_TAIL_DAMP_MIN_HZ, ROOM_TAIL_DAMP_MAX_HZ);
    echo_time = space_room_clamp_u16(state->echo_time_ms, 80u, 1200u);
    feedback = state->echo_feedback_q15 > APSIS_Q15_MILLI(480)
        ? APSIS_Q15_MILLI(480) : state->echo_feedback_q15;
    guard_hz = ((uint32_t)damping * 84u + 50u) / 100u;
    if (guard_hz < ROOM_GUARD_LP_MIN_HZ) guard_hz = ROOM_GUARD_LP_MIN_HZ;
    if (guard_hz > ROOM_GUARD_LP_MAX_HZ) guard_hz = ROOM_GUARD_LP_MAX_HZ;

    room->target_wet_q15 = mix;
    room->target_predelay_frames_q16 = space_room_ms_q16(predelay);
    room->target_decay_ms_q16 = (int32_t)((uint32_t)decay << 16);
    room->target_damping_hz_q16 = (int32_t)((uint32_t)damping << 16);
    room->target_guard_lp_hz_q16 = (int32_t)(guard_hz << 16);
    room->target_echo_delay_frames_q16 = space_room_ms_q16(echo_time);
    room->target_echo_feedback_q15 = feedback;

    if (!room->params_initialized) {
        room->wet_q15 = room->target_wet_q15;
        room->predelay_frames_q16 = room->target_predelay_frames_q16;
        room->decay_ms_q16 = room->target_decay_ms_q16;
        room->damping_hz_q16 = room->target_damping_hz_q16;
        room->guard_lp_hz_q16 = room->target_guard_lp_hz_q16;
        room->echo_delay_frames_q16 = room->target_echo_delay_frames_q16;
        room->echo_feedback_q15 = room->target_echo_feedback_q15;
        room->params_initialized = 1u;
    }
}

static void space_room_smooth_params(ApsisSpaceRoom *room)
{
    if (!room || !room->params_initialized) return;
    room->wet_q15 = space_room_smooth_i32(room->wet_q15,
        room->target_wet_q15, ROOM_SMOOTH_50MS_Q15);
    room->predelay_frames_q16 = space_room_smooth_u32(room->predelay_frames_q16,
        room->target_predelay_frames_q16, ROOM_SMOOTH_50MS_Q15);
    room->decay_ms_q16 = space_room_smooth_i32(room->decay_ms_q16,
        room->target_decay_ms_q16, ROOM_SMOOTH_80MS_Q15);
    room->damping_hz_q16 = space_room_smooth_i32(room->damping_hz_q16,
        room->target_damping_hz_q16, ROOM_SMOOTH_80MS_Q15);
    room->guard_lp_hz_q16 = space_room_smooth_i32(room->guard_lp_hz_q16,
        room->target_guard_lp_hz_q16, ROOM_SMOOTH_100MS_Q15);
    room->echo_delay_frames_q16 = space_room_smooth_u32(room->echo_delay_frames_q16,
        room->target_echo_delay_frames_q16, ROOM_SMOOTH_40MS_Q15);
    room->echo_feedback_q15 = space_room_smooth_i32(room->echo_feedback_q15,
        room->target_echo_feedback_q15, ROOM_SMOOTH_40MS_Q15);
}

uint8_t apsis_space_room_bind(ApsisSpaceRoom *room,
                              int16_t *history_storage,
                              uint32_t history_storage_samples,
                              int16_t *echo_storage,
                              uint32_t echo_storage_samples,
                              int16_t *fdn_storage,
                              uint32_t fdn_storage_samples)
{
    uint8_t fdn_ok;
    if (!room) return 0u;
    room->history_storage = history_storage;
    room->history_storage_samples = history_storage_samples;
    room->echo_storage = echo_storage;
    room->echo_storage_samples = echo_storage_samples;
    fdn_ok = apsis_space_fdn_bind(&room->fdn, fdn_storage, fdn_storage_samples);
    room->storage_bound = history_storage &&
        history_storage_samples >= APSIS_SPACE_HISTORY_STORAGE_SAMPLES &&
        echo_storage && echo_storage_samples >= APSIS_SPACE_ECHO_STORAGE_SAMPLES &&
        fdn_ok ? 1u : 0u;
    apsis_space_room_reset(room);
    return room->storage_bound;
}

void apsis_space_room_reset(ApsisSpaceRoom *room)
{
    uint8_t ch;
    if (!room) return;
    room->history_write_frame = 0u;
    room->history_fill_frames = 0u;
    room->echo_write_frame = 0u;
    room->echo_fill_frames = 0u;
    room->compressor_gain_q15 = ROOM_Q15_ONE;
    room->wet_q15 = 0;
    room->target_wet_q15 = 0;
    room->predelay_frames_q16 = 0u;
    room->target_predelay_frames_q16 = 0u;
    room->decay_ms_q16 = 0;
    room->target_decay_ms_q16 = 0;
    room->damping_hz_q16 = 0;
    room->target_damping_hz_q16 = 0;
    room->guard_lp_hz_q16 = 0;
    room->target_guard_lp_hz_q16 = 0;
    room->echo_delay_frames_q16 = 0u;
    room->target_echo_delay_frames_q16 = 0u;
    room->echo_feedback_q15 = 0;
    room->target_echo_feedback_q15 = 0;
    room->params_initialized = 0u;
    for (ch = 0u; ch < 2u; ++ch) {
        room->guard_hp[ch].z1_q15 = room->guard_hp[ch].z2_q15 = 0;
        room->guard_lp[ch].z1_q15 = room->guard_lp[ch].z2_q15 = 0;
        room->tail_hp[ch].z1_q15 = room->tail_hp[ch].z2_q15 = 0;
        room->echo_lp[ch].z1_q15 = room->echo_lp[ch].z2_q15 = 0;
    }
    apsis_space_fdn_reset(&room->fdn);
}

void apsis_space_room_render(ApsisSpaceRoom *room,
                             const ApsisSpaceState *state,
                             const int32_t *input_stereo_q15,
                             int32_t *out_stereo,
                             uint32_t frames)
{
    uint32_t done = 0u;
    if (!room || !state || !out_stereo || !room->storage_bound) return;
    space_room_targets(room, state);

    while (done < frames) {
        int32_t tail_mono[APSIS_SPACE_ROOM_BLOCK_MAX];
        int32_t fdn_out[APSIS_SPACE_ROOM_BLOCK_MAX * 2u];
        int32_t wet_gain[APSIS_SPACE_ROOM_BLOCK_MAX];
        uint32_t n = frames - done;
        uint32_t i;
        uint16_t decay_ms, damping_hz;
        if (n > APSIS_SPACE_ROOM_BLOCK_MAX) n = APSIS_SPACE_ROOM_BLOCK_MAX;

        for (i = 0u; i < n; ++i) {
            const SpaceRoomBiquadCoeff *guard_lp;
            int32_t in_l = input_stereo_q15 ? input_stereo_q15[(done + i) * 2u] : 0;
            int32_t in_r = input_stereo_q15 ? input_stereo_q15[(done + i) * 2u + 1u] : 0;
            int32_t g_l, g_r, early_l = 0, early_r = 0;
            int32_t pred_l, pred_r, echo_l, echo_r, echo_fb_l, echo_fb_r;
            int32_t tail_l, tail_r;
            uint32_t tap;

            space_room_smooth_params(room);
            guard_lp = space_room_guard_lp_coeff(room->guard_lp_hz_q16);

            g_l = space_room_mul_q15(in_l, ROOM_TRIM_Q15);
            g_r = space_room_mul_q15(in_r, ROOM_TRIM_Q15);
            g_l = space_room_biquad(&room->guard_hp[0], &ROOM_GUARD_HP_170, g_l);
            g_r = space_room_biquad(&room->guard_hp[1], &ROOM_GUARD_HP_170, g_r);
            g_l = space_room_biquad(&room->guard_lp[0], guard_lp, g_l);
            g_r = space_room_biquad(&room->guard_lp[1], guard_lp, g_r);
            /* Linked stereo detector: update gain once, then apply the same
             * compressor state to both channels. */
            g_l = space_room_compressor(room, g_l, g_r, 0u);
            g_r = space_room_clamp_q15(space_room_mul_q15(g_r, room->compressor_gain_q15));

            for (tap = 0u; tap < APSIS_SPACE_EARLY_TAPS; ++tap) {
                early_l += space_room_mul_q15(
                    space_room_stereo_read(room->history_storage,
                        APSIS_SPACE_HISTORY_MAX_FRAMES,
                        room->history_write_frame, room->history_fill_frames,
                        ROOM_EARLY_DELAY_Q16[tap], 0u),
                    ROOM_EARLY_GAIN_Q15[tap]);
                early_r += space_room_mul_q15(
                    space_room_stereo_read(room->history_storage,
                        APSIS_SPACE_HISTORY_MAX_FRAMES,
                        room->history_write_frame, room->history_fill_frames,
                        ROOM_EARLY_DELAY_Q16[tap], 1u),
                    ROOM_EARLY_GAIN_Q15[tap]);
            }
            out_stereo[(done + i) * 2u] += space_room_mul_q15(early_l, ROOM_EARLY_BUS_Q15);
            out_stereo[(done + i) * 2u + 1u] += space_room_mul_q15(early_r, ROOM_EARLY_BUS_Q15);

            pred_l = space_room_stereo_read(room->history_storage,
                APSIS_SPACE_HISTORY_MAX_FRAMES, room->history_write_frame,
                room->history_fill_frames, room->predelay_frames_q16, 0u);
            pred_r = space_room_stereo_read(room->history_storage,
                APSIS_SPACE_HISTORY_MAX_FRAMES, room->history_write_frame,
                room->history_fill_frames, room->predelay_frames_q16, 1u);

            echo_l = space_room_stereo_read(room->echo_storage,
                APSIS_SPACE_ECHO_MAX_FRAMES, room->echo_write_frame,
                room->echo_fill_frames, room->echo_delay_frames_q16, 0u);
            echo_r = space_room_stereo_read(room->echo_storage,
                APSIS_SPACE_ECHO_MAX_FRAMES, room->echo_write_frame,
                room->echo_fill_frames, room->echo_delay_frames_q16, 1u);
            echo_fb_l = space_room_biquad(&room->echo_lp[0], &ROOM_ECHO_LP_4200, echo_l);
            echo_fb_r = space_room_biquad(&room->echo_lp[1], &ROOM_ECHO_LP_4200, echo_r);
            echo_fb_l = space_room_mul_q15(echo_fb_l, room->echo_feedback_q15);
            echo_fb_r = space_room_mul_q15(echo_fb_r, room->echo_feedback_q15);

            space_room_stereo_write(room->echo_storage, APSIS_SPACE_ECHO_MAX_FRAMES,
                &room->echo_write_frame, &room->echo_fill_frames,
                (int32_t)space_room_clamp_q15((int64_t)g_l + echo_fb_l),
                (int32_t)space_room_clamp_q15((int64_t)g_r + echo_fb_r));

            tail_l = space_room_biquad(&room->tail_hp[0], &ROOM_TAIL_HP_190,
                                       pred_l + echo_l);
            tail_r = space_room_biquad(&room->tail_hp[1], &ROOM_TAIL_HP_190,
                                       pred_r + echo_r);
            tail_mono[i] = space_room_clamp_q15(((int64_t)tail_l + tail_r) / 2);
            fdn_out[i * 2u] = 0;
            fdn_out[i * 2u + 1u] = 0;
            wet_gain[i] = room->wet_q15;

            {
                uint32_t hw = room->history_write_frame;
                uint32_t hf = room->history_fill_frames;
                space_room_stereo_write(room->history_storage,
                    APSIS_SPACE_HISTORY_MAX_FRAMES, &hw, &hf, g_l, g_r);
                room->history_write_frame = (uint16_t)hw;
                room->history_fill_frames = (uint16_t)hf;
            }
        }

        decay_ms = (uint16_t)(((uint32_t)room->decay_ms_q16 + 32768u) >> 16);
        damping_hz = (uint16_t)(((uint32_t)room->damping_hz_q16 + 32768u) >> 16);
        apsis_space_fdn_render(&room->fdn, tail_mono, fdn_out, n,
                              decay_ms, damping_hz);
        for (i = 0u; i < n; ++i) {
            out_stereo[(done + i) * 2u] +=
                space_room_mul_q15(fdn_out[i * 2u], wet_gain[i]);
            out_stereo[(done + i) * 2u + 1u] +=
                space_room_mul_q15(fdn_out[i * 2u + 1u], wet_gain[i]);
        }
        done += n;
    }
}
