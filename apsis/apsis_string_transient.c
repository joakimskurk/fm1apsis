#include "apsis_string_dsp.h"
#include "apsis_harmony.h"

#define STRX_Q20_ONE 1048576u
#define STRX_ENV_PHASE_ONE (64u << 16)
#define STRX_ATTACK_SAMPLES 706u       /* round(44.1k * 16 ms) */
#define STRX_ATTACK_STEP_Q16 5941u     /* (64<<16) / 706 */
#define STRX_STAGGER_SAMPLES 132u      /* round(44.1k * 3 ms) */
#define STRX_STEAL_SAMPLES 4630u       /* round(44.1k * 105 ms) */
#define STRX_STEAL_ALPHA_Q15 53u       /* 14 ms setTarget-style time constant */
#define STRX_BRIGHT_MIN_Q15 4915u      /* 0.15 */
#define STRX_UPPER_PAN_SCALE_Q15 14745 /* 0.45 */

static const int16_t STRX_SINE_QUARTER[65] = {
    0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
    12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
    23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
    30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};

static const uint32_t STRX_SEMITONE_Q20[12] = {
    1048576u,1110928u,1176987u,1246974u,1321123u,1399681u,
    1482910u,1571089u,1664511u,1763488u,1868350u,1979448u
};

static const uint32_t STRX_CENT_Q20[101] = {
    1048576u,1049182u,1049788u,1050395u,1051002u,1051609u,1052216u,1052824u,1053433u,1054041u,
    1054650u,1055260u,1055869u,1056479u,1057090u,1057701u,1058312u,1058923u,1059535u,1060147u,
    1060760u,1061373u,1061986u,1062600u,1063214u,1063828u,1064443u,1065058u,1065673u,1066289u,
    1066905u,1067521u,1068138u,1068755u,1069373u,1069991u,1070609u,1071227u,1071846u,1072466u,
    1073085u,1073705u,1074326u,1074946u,1075568u,1076189u,1076811u,1077433u,1078055u,1078678u,
    1079302u,1079925u,1080549u,1081174u,1081798u,1082423u,1083049u,1083674u,1084301u,1084927u,
    1085554u,1086181u,1086809u,1087437u,1088065u,1088694u,1089323u,1089952u,1090582u,1091212u,
    1091843u,1092473u,1093105u,1093736u,1094368u,1095000u,1095633u,1096266u,1096900u,1097533u,
    1098167u,1098802u,1099437u,1100072u,1100708u,1101344u,1101980u,1102617u,1103254u,1103891u,
    1104529u,1105167u,1105806u,1106445u,1107084u,1107724u,1108364u,1109004u,1109645u,1110286u,1110928u
};

/* Study-98 normal harmonic law h^-(1.28 + (1-brightness)*0.58), sampled at
 * 17 equally spaced brightness points over 0.15..1.0. Linear interpolation
 * keeps worst relative error below 0.06% for h=2..4. */
static const uint16_t STRX_HARMONIC_Q15[3][17] = {
    {9588u,9795u,10006u,10222u,10443u,10668u,10898u,11134u,11374u,11620u,11870u,12127u,12388u,12656u,12929u,13208u,13493u},
    {4672u,4833u,4999u,5171u,5349u,5534u,5724u,5921u,6125u,6336u,6554u,6780u,7013u,7255u,7504u,7763u,8030u},
    {2805u,2928u,3056u,3189u,3328u,3473u,3625u,3783u,3948u,4120u,4300u,4488u,4684u,4888u,5102u,5324u,5556u}
};

static const uint16_t STRX_PIZZ_Q15[3] = {32767u,11425u,6169u};

/* 1-exp(-2*pi*f/44100), f=1600..16000 in 450 Hz steps. */
static const uint16_t STRX_LP_ALPHA_Q15[33] = {
    6679u,8299u,9819u,11244u,12581u,13834u,15010u,16113u,17147u,18117u,19027u,
    19880u,20680u,21431u,22135u,22795u,23415u,23995u,24540u,25051u,25530u,25980u,
    26401u,26796u,27167u,27515u,27841u,28147u,28434u,28703u,28955u,29192u,29414u
};

/* Equal-power pan at p=-1..+1 in 1/16 steps. Centre is overridden to direct
 * mono because Study-98 tone() skips StereoPanner for near-zero pan. */
static const uint16_t STRX_PAN_Q15[33][2] = {
    {32767u,0u},{32728u,1608u},{32609u,3212u},{32412u,4808u},{32137u,6393u},
    {31785u,7962u},{31356u,9512u},{30852u,11039u},{30273u,12539u},{29621u,14010u},
    {28898u,15446u},{28105u,16846u},{27245u,18204u},{26319u,19519u},{25329u,20787u},
    {24279u,22005u},{23170u,23170u},{22005u,24279u},{20787u,25329u},{19519u,26319u},
    {18204u,27245u},{16846u,28105u},{15446u,28898u},{14010u,29621u},{12539u,30273u},
    {11039u,30852u},{9512u,31356u},{7962u,31785u},{6393u,32137u},{4808u,32412u},
    {3212u,32609u},{1608u,32728u},{0u,32767u}
};

/* Exponential-style 0.002 -> 1 attack curve. Decay uses the same table in
 * reverse. Index zero is forced to exact digital silence for click safety. */
static const uint16_t STRX_ENV_Q15[65] = {
    0u,72u,80u,88u,97u,106u,117u,129u,143u,157u,173u,191u,210u,232u,255u,281u,
    310u,341u,376u,415u,457u,504u,555u,612u,674u,743u,818u,902u,994u,1095u,1207u,
    1330u,1465u,1615u,1779u,1961u,2161u,2381u,2624u,2892u,3187u,3512u,3870u,4264u,
    4699u,5178u,5706u,6288u,6929u,7636u,8415u,9273u,10218u,11260u,12409u,13674u,
    15068u,16605u,18298u,20164u,22220u,24486u,26983u,29735u,32767u
};

static void strx_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static int32_t strx_clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint16_t strx_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static int32_t strx_div100_round(int32_t v)
{
    return v >= 0 ? (v + 50) / 100 : -((-v + 50) / 100);
}

static uint8_t strx_pitch_class(int32_t step)
{
    int32_t pc = step % 12;
    if (pc < 0) pc += 12;
    return (uint8_t)pc;
}

static uint32_t strx_equal_frequency_millihz(int32_t step_milli)
{
    int32_t semitone, frac, octave, pc;
    uint32_t f, r, idx, rem;
    step_milli = strx_clamp_i32(step_milli, -60000, 84000);
    semitone = step_milli / 1000;
    frac = step_milli % 1000;
    if (frac < 0) { frac += 1000; --semitone; }
    octave = semitone / 12;
    pc = semitone % 12;
    if (pc < 0) { pc += 12; --octave; }
    f = (uint32_t)(((uint64_t)130813u * STRX_SEMITONE_Q20[pc] + STRX_Q20_ONE / 2u) >> 20);
    if (octave >= 0) {
        if (octave > 7) octave = 7;
        f <<= (uint32_t)octave;
    } else {
        uint32_t sh = (uint32_t)(-octave);
        if (sh > 12u) sh = 12u;
        f = (f + (1u << (sh ? sh - 1u : 0u))) >> sh;
    }
    idx = (uint32_t)frac / 10u;
    rem = (uint32_t)frac - idx * 10u;
    r = STRX_CENT_Q20[idx];
    if (rem && idx < 100u)
        r += ((STRX_CENT_Q20[idx + 1u] - r) * rem + 5u) / 10u;
    return (uint32_t)(((uint64_t)f * r + STRX_Q20_ONE / 2u) >> 20);
}

uint32_t apsis_string_dsp_frequency_millihz(const ApsisState *s, int32_t step_milli)
{
    int32_t whole, dev = 0;
    if (!s) return 0u;
    if ((step_milli % 1000) == 0) {
        whole = step_milli / 1000;
        whole = apsis_quantize_step_to_scale(whole, s->gravity.tonic, s->gravity.scale,
                                             s->gravity.scale_lock);
        step_milli = whole * 1000;
        dev = apsis_tuning_deviation_milli(strx_pitch_class(whole), s->gravity.tonic,
                                           s->temper.mode, s->temper.strength_q15);
        step_milli += strx_div100_round(dev);
    }
    return strx_equal_frequency_millihz(step_milli);
}

static uint32_t strx_phase_inc_from_millihz(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint32_t strx_samples_for_ms(uint32_t ms)
{
    return (APSIS_STRING_DSP_RATE * ms + 500u) / 1000u;
}

static uint16_t strx_harmonic_gain(uint16_t brightness_q15, uint8_t harmonic,
                                   uint8_t pizz)
{
    uint32_t b, pos, idx, frac;
    uint16_t a, z;
    if (harmonic <= 1u) return APSIS_Q15_ONE;
    if (pizz) {
        if (harmonic > 3u) harmonic = 3u;
        return STRX_PIZZ_Q15[harmonic - 1u];
    }
    if (harmonic > 4u) harmonic = 4u;
    b = brightness_q15 < STRX_BRIGHT_MIN_Q15 ? STRX_BRIGHT_MIN_Q15 : brightness_q15;
    if (b > APSIS_Q15_ONE) b = APSIS_Q15_ONE;
    pos = ((b - STRX_BRIGHT_MIN_Q15) * 4096u +
           (APSIS_Q15_ONE - STRX_BRIGHT_MIN_Q15) / 2u) /
          (APSIS_Q15_ONE - STRX_BRIGHT_MIN_Q15);
    idx = pos >> 8;
    frac = pos & 255u;
    if (idx >= 16u) return STRX_HARMONIC_Q15[harmonic - 2u][16];
    a = STRX_HARMONIC_Q15[harmonic - 2u][idx];
    z = STRX_HARMONIC_Q15[harmonic - 2u][idx + 1u];
    return (uint16_t)(a + (((int32_t)z - a) * (int32_t)frac + 128) / 256);
}

static uint16_t strx_lp_alpha_for_hz(uint32_t hz)
{
    uint32_t pos, rem;
    uint16_t a, b;
    if (hz <= 1600u) return STRX_LP_ALPHA_Q15[0];
    if (hz >= 16000u) return STRX_LP_ALPHA_Q15[32];
    pos = (hz - 1600u) / 450u;
    rem = (hz - 1600u) - pos * 450u;
    a = STRX_LP_ALPHA_Q15[pos];
    b = STRX_LP_ALPHA_Q15[pos + 1u];
    return (uint16_t)(a + ((uint32_t)(b - a) * rem + 225u) / 450u);
}

static int16_t strx_upper_pan(int16_t pan)
{
    int32_t prod = (int32_t)pan * STRX_UPPER_PAN_SCALE_Q15;
    int32_t scaled;
    if (prod >= 0) scaled = (prod + APSIS_Q15_ONE / 2) / APSIS_Q15_ONE;
    else scaled = -((-prod + APSIS_Q15_ONE / 2) / APSIS_Q15_ONE);
    scaled = -scaled;
    return (int16_t)strx_clamp_i32(scaled, -32767, 32767);
}

static void strx_pan_mix(int16_t pan_q15, uint16_t *left, uint16_t *right)
{
    int32_t p = strx_clamp_i32(pan_q15, -32767, 32767);
    uint32_t pos, idx, frac;
    uint16_t l0, l1, r0, r1;
    if (!left || !right) return;
    if (p >= -197 && p <= 197) {
        *left = APSIS_Q15_ONE;
        *right = APSIS_Q15_ONE;
        return;
    }
    pos = ((uint32_t)(p + 32767) * 8192u + 32767u) / 65534u;
    idx = pos >> 8;
    frac = pos & 255u;
    if (idx >= 32u) {
        *left = STRX_PAN_Q15[32][0];
        *right = STRX_PAN_Q15[32][1];
        return;
    }
    l0 = STRX_PAN_Q15[idx][0];
    l1 = STRX_PAN_Q15[idx + 1u][0];
    r0 = STRX_PAN_Q15[idx][1];
    r1 = STRX_PAN_Q15[idx + 1u][1];
    *left = (uint16_t)((int32_t)l0 + (((int32_t)l1 - l0) * (int32_t)frac + 128) / 256);
    *right = (uint16_t)((int32_t)r0 + (((int32_t)r1 - r0) * (int32_t)frac + 128) / 256);
}

static void strx_append_group(const ApsisState *s, ApsisStringTransientPlan *out,
                              uint32_t base_freq_millihz, uint16_t level_q15,
                              uint32_t base_duration_samples, int16_t pan_q15,
                              uint8_t pizz, uint16_t scale_q15, uint8_t root_shadow)
{
    uint8_t count = pizz ? 3u : 4u;
    uint8_t hi;
    uint16_t resonance = s->string.resonance_q15 < APSIS_Q15_MILLI(250)
        ? APSIS_Q15_MILLI(250) : s->string.resonance_q15;
    uint16_t base_gain = strx_mul_q15(strx_mul_q15(level_q15, scale_q15), resonance);
    for (hi = 0u; hi < count && out->count < APSIS_STRING_TRANSIENT_PARTIALS_MAX; ++hi) {
        ApsisStringTransientSpec *spec = &out->partial[out->count++];
        uint8_t harmonic = (uint8_t)(hi + 1u);
        uint32_t duration_mul = pizz ? (100u - (uint32_t)hi * 9u)
                                     : (100u + (uint32_t)hi * 16u);
        int16_t partial_pan = hi ? strx_upper_pan(pan_q15) : pan_q15;
        uint32_t freq = base_freq_millihz * harmonic;
        uint32_t cutoff_hz;
        spec->freq_millihz = freq;
        spec->start_offset_samples = (uint32_t)hi * STRX_STAGGER_SAMPLES;
        spec->duration_samples = (base_duration_samples * duration_mul + 50u) / 100u;
        spec->peak_gain_q15 = strx_mul_q15(base_gain,
            strx_harmonic_gain(s->string.brightness_q15, harmonic, pizz));
        spec->waveform = harmonic <= 2u ? APSIS_STRING_DSP_WAVE_TRIANGLE
                                        : APSIS_STRING_DSP_WAVE_SINE;
        spec->root_shadow = root_shadow;
        strx_pan_mix(partial_pan, &spec->mix_left_q15, &spec->mix_right_q15);
        if (spec->waveform == APSIS_STRING_DSP_WAVE_TRIANGLE) {
            cutoff_hz = (freq * 235u + 50000u) / 100000u;
            if (cutoff_hz < 1600u) cutoff_hz = 1600u;
            if (cutoff_hz > 16000u) cutoff_hz = 16000u;
            spec->lowpass_alpha_q15 = strx_lp_alpha_for_hz(cutoff_hz);
        } else {
            spec->lowpass_alpha_q15 = 0u;
        }
    }
}

void apsis_string_dsp_transient_plan(const ApsisState *s,
                                     uint32_t base_freq_millihz,
                                     uint16_t level_q15,
                                     uint32_t duration_ms,
                                     int16_t pan_q15,
                                     uint8_t pizz,
                                     uint16_t root_gain_q15,
                                     ApsisStringTransientPlan *out)
{
    uint32_t base_samples, root;
    int32_t root_step;
    if (!out) return;
    strx_zero(out, (uint32_t)sizeof(*out));
    if (!s) return;
    if (base_freq_millihz < 35000u) base_freq_millihz = 35000u;
    if (base_freq_millihz > 2800000u) base_freq_millihz = 2800000u;
    if (duration_ms < 20u) duration_ms = 20u;
    if (duration_ms > 3000u) duration_ms = 3000u;
    base_samples = strx_samples_for_ms(duration_ms);
    strx_append_group(s, out, base_freq_millihz, level_q15, base_samples,
                      pan_q15, pizz ? 1u : 0u, APSIS_Q15_ONE, 0u);
    if (!root_gain_q15) return;
    if (root_gain_q15 > APSIS_Q15_ONE) root_gain_q15 = APSIS_Q15_ONE;
    root_step = apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                               s->gravity.degree > 7u ? 7 : s->gravity.degree) * 1000;
    root = apsis_string_dsp_frequency_millihz(s, root_step);
    while (root > 1u && root * 5u > base_freq_millihz * 6u) root >>= 1u;
    strx_append_group(s, out, root, level_q15, base_samples, pan_q15,
                      pizz ? 1u : 0u, root_gain_q15, 1u);
}

uint16_t apsis_string_dsp_transient_logical_active(const ApsisStringDsp *dsp)
{
    uint16_t i, n = 0u;
    if (!dsp) return 0u;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (dsp->transient[i].active && !dsp->transient[i].stealing) ++n;
    return n;
}

uint16_t apsis_string_dsp_transient_physical_active(const ApsisStringDsp *dsp)
{
    uint16_t i, n = 0u;
    if (!dsp) return 0u;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (dsp->transient[i].active) ++n;
    return n;
}

static int32_t strx_find_free(const ApsisStringDsp *dsp)
{
    uint16_t i;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (!dsp->transient[i].active) return (int32_t)i;
    return -1;
}

static uint8_t strx_steal_oldest(ApsisStringDsp *dsp)
{
    uint16_t i;
    int32_t oldest = -1;
    uint32_t serial = 0xFFFFFFFFu;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &dsp->transient[i];
        if (v->active && !v->stealing && v->serial < serial) {
            serial = v->serial;
            oldest = (int32_t)i;
        }
    }
    if (oldest < 0) return 0u;
    if (dsp->transient[oldest].start_delay_samples) {
        dsp->transient[oldest].active = 0u;
    } else {
        dsp->transient[oldest].stealing = 1u;
        dsp->transient[oldest].steal_gain_q15 = APSIS_Q15_ONE;
        dsp->transient[oldest].steal_left_samples = STRX_STEAL_SAMPLES;
    }
    dsp->transient_steal_count++;
    return 1u;
}

static void strx_update_high_water(ApsisStringDsp *dsp)
{
    uint16_t logical = apsis_string_dsp_transient_logical_active(dsp);
    uint16_t physical = apsis_string_dsp_transient_physical_active(dsp);
    if (logical > dsp->transient_logical_high_water)
        dsp->transient_logical_high_water = logical;
    if (physical > dsp->transient_physical_high_water)
        dsp->transient_physical_high_water = physical;
}

static uint8_t strx_allocate(ApsisStringDsp *dsp,
                             const ApsisStringTransientSpec *spec)
{
    int32_t free_ix;
    ApsisStringTransientVoice *v;
    uint32_t decay_samples;
    free_ix = strx_find_free(dsp);
    if (free_ix < 0) {
        dsp->transient_drop_count++;
        return 0u;
    }
    if (apsis_string_dsp_transient_logical_active(dsp) >= APSIS_STRING_TRANSIENT_LOGICAL_CAP)
        (void)strx_steal_oldest(dsp);
    v = &dsp->transient[free_ix];
    strx_zero(v, (uint32_t)sizeof(*v));
    v->active = 1u;
    v->serial = ++dsp->transient_next_serial;
    v->phase_inc = strx_phase_inc_from_millihz(spec->freq_millihz);
    v->start_delay_samples = spec->start_offset_samples;
    v->duration_samples = spec->duration_samples;
    v->peak_gain_q15 = spec->peak_gain_q15;
    v->mix_left_q15 = spec->mix_left_q15;
    v->mix_right_q15 = spec->mix_right_q15;
    v->lowpass_alpha_q15 = spec->lowpass_alpha_q15;
    v->waveform = spec->waveform;
    v->steal_gain_q15 = APSIS_Q15_ONE;
    v->attack_left_samples = v->duration_samples < STRX_ATTACK_SAMPLES
        ? (uint16_t)v->duration_samples : STRX_ATTACK_SAMPLES;
    decay_samples = v->duration_samples > v->attack_left_samples
        ? v->duration_samples - v->attack_left_samples : 1u;
    v->decay_step_q16 = (STRX_ENV_PHASE_ONE + decay_samples / 2u) / decay_samples;
    if (!v->decay_step_q16) v->decay_step_q16 = 1u;
    strx_update_high_water(dsp);
    return 1u;
}

uint8_t apsis_string_dsp_trigger_classic(ApsisStringDsp *dsp,
                                         const ApsisState *s,
                                         uint32_t base_freq_millihz,
                                         uint16_t level_q15,
                                         uint32_t duration_ms,
                                         int16_t pan_q15,
                                         uint8_t pizz,
                                         uint16_t root_gain_q15)
{
    ApsisStringTransientPlan plan;
    uint8_t i, accepted = 0u;
    if (!dsp || !s) return 0u;
    apsis_string_dsp_transient_plan(s, base_freq_millihz, level_q15, duration_ms,
                                    pan_q15, pizz, root_gain_q15, &plan);
    dsp->transient_trigger_count++;
    for (i = 0u; i < plan.count; ++i)
        accepted = (uint8_t)(accepted + strx_allocate(dsp, &plan.partial[i]));
    return accepted;
}

static int16_t strx_sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return STRX_SINE_QUARTER[o];
    if (q == 1u) return STRX_SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-STRX_SINE_QUARTER[o];
    return (int16_t)-STRX_SINE_QUARTER[64u - o];
}

static int16_t strx_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = strx_sine_point(i);
    int32_t b = strx_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static int16_t strx_triangle_q15(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    if (p < 16384u) return (int16_t)((uint32_t)p * 2u);
    if (p < 49152u) return (int16_t)(32767 - (int32_t)(p - 16384u) * 2);
    return (int16_t)(-32767 + (int32_t)(p - 49152u) * 2);
}

static int32_t strx_sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

static uint16_t strx_envelope(const ApsisStringTransientVoice *v)
{
    uint32_t idx = v->env_phase_q16 >> 16;
    if (idx > 64u) idx = 64u;
    return STRX_ENV_Q15[idx];
}

static void strx_advance_envelope(ApsisStringTransientVoice *v)
{
    if (v->envelope_stage == 0u) {
        if (v->attack_left_samples) --v->attack_left_samples;
        if (v->env_phase_q16 + STRX_ATTACK_STEP_Q16 >= STRX_ENV_PHASE_ONE)
            v->env_phase_q16 = STRX_ENV_PHASE_ONE;
        else
            v->env_phase_q16 += STRX_ATTACK_STEP_Q16;
        if (!v->attack_left_samples) {
            v->envelope_stage = 1u;
            v->env_phase_q16 = STRX_ENV_PHASE_ONE;
        }
    } else {
        if (v->env_phase_q16 > v->decay_step_q16)
            v->env_phase_q16 -= v->decay_step_q16;
        else
            v->env_phase_q16 = 0u;
    }
}

static void strx_advance_steal(ApsisStringTransientVoice *v)
{
    int32_t delta;
    if (!v->stealing) return;
    delta = (int32_t)(((int64_t)-v->steal_gain_q15 * STRX_STEAL_ALPHA_Q15) >> 15);
    if (!delta && v->steal_gain_q15 > 0) delta = -1;
    v->steal_gain_q15 += delta;
    if (v->steal_gain_q15 < 0) v->steal_gain_q15 = 0;
    if (v->steal_left_samples) --v->steal_left_samples;
    if (!v->steal_left_samples) v->active = 0u;
}

void apsis_string_transient_render(ApsisStringDsp *dsp, int32_t *out,
                                   uint32_t frames)
{
    uint32_t n;
    uint16_t i;
    if (!dsp || !out) return;
    for (n = 0u; n < frames; ++n) {
        int64_t mix_l = 0, mix_r = 0;
        for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
            ApsisStringTransientVoice *v = &dsp->transient[i];
            int32_t raw, filtered, voiced;
            uint16_t env, gain;
            if (!v->active) continue;
            if (v->start_delay_samples) {
                --v->start_delay_samples;
                continue;
            }
            if (v->age_samples >= v->duration_samples) {
                v->active = 0u;
                continue;
            }
            v->phase += v->phase_inc;
            raw = v->waveform == APSIS_STRING_DSP_WAVE_TRIANGLE
                ? strx_triangle_q15(v->phase) : strx_sine_q15(v->phase);
            if (v->lowpass_alpha_q15) {
                v->lowpass_q15 += (int32_t)(((int64_t)(raw - v->lowpass_q15) *
                                               v->lowpass_alpha_q15) >> 15);
                filtered = v->lowpass_q15;
            } else {
                filtered = raw;
            }
            env = strx_envelope(v);
            gain = strx_mul_q15(v->peak_gain_q15, env);
            if (v->stealing)
                gain = strx_mul_q15(gain, (uint16_t)v->steal_gain_q15);
            voiced = (int32_t)(((int64_t)filtered * gain) >> 15);
            mix_l += ((int64_t)voiced * v->mix_left_q15) >> 15;
            mix_r += ((int64_t)voiced * v->mix_right_q15) >> 15;
            ++v->age_samples;
            strx_advance_envelope(v);
            strx_advance_steal(v);
            if (v->age_samples >= v->duration_samples) v->active = 0u;
        }
        out[n * 2u] = strx_sat_q15((int64_t)out[n * 2u] + mix_l);
        out[n * 2u + 1u] = strx_sat_q15((int64_t)out[n * 2u + 1u] + mix_r);
    }
}
