#include "apsis_clock_dsp.h"

#define CLOCK_Q31_ONE 0x7fffffffu
#define CLOCK_Q30_ONE 1073741824ll
#define CLOCK_GAIN_FLOOR_Q15 3u
#define CLOCK_NOISE_SEED 0x126c10c5u
#define CLOCK_LEVEL_MAX_Q15 APSIS_Q15_MILLI(1250)

static const int16_t CLOCK_SINE_QUARTER[65] = {
0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};

static const uint16_t CLOCK_START_HZ[4][3] = {
    {118u,118u,168u}, {245u,245u,350u}, {760u,760u,1080u}, {88u,88u,132u}
};
static const uint16_t CLOCK_END_HZ[4][3] = {
    {67u,67u,116u}, {160u,160u,245u}, {520u,520u,790u}, {58u,58u,92u}
};
static const uint16_t CLOCK_DURATION_MS[4] = {120u,75u,280u,170u};
static const uint16_t CLOCK_GAIN_MILLI[4][3] = {
    {84u,58u,18u}, {70u,50u,14u}, {60u,43u,12u}, {56u,41u,11u}
};

static void clock_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static int32_t clock_sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

static uint32_t clock_samples_ms(uint32_t ms)
{
    return (APSIS_CLOCK_DSP_RATE * ms + 500u) / 1000u;
}

static uint32_t clock_samples_tenth_ms(uint32_t tenths)
{
    return (APSIS_CLOCK_DSP_RATE * tenths + 5000u) / 10000u;
}

static uint32_t clock_mul_q15_u32(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint32_t clock_mul_q31(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + (1ull << 30)) >> 31);
}

static uint32_t clock_pow_q31(uint32_t base, uint32_t exp)
{
    uint32_t out = CLOCK_Q31_ONE;
    while (exp) {
        if (exp & 1u) out = clock_mul_q31(out, base);
        exp >>= 1u;
        if (exp) base = clock_mul_q31(base, base);
    }
    return out;
}

static uint32_t clock_nth_root_q31(uint32_t target, uint32_t n)
{
    uint32_t lo, hi, k;
    if (!n || target >= CLOCK_Q31_ONE) return CLOCK_Q31_ONE;
    if (!target) return 0u;
    lo = target;
    hi = CLOCK_Q31_ONE;
    for (k = 0u; k < 24u; ++k) {
        uint32_t mid = lo + ((hi - lo) >> 1u);
        uint32_t p = clock_pow_q31(mid, n);
        if (p > target) hi = mid;
        else lo = mid;
    }
    return lo + ((hi - lo) >> 1u);
}

static int16_t clock_sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return CLOCK_SINE_QUARTER[o];
    if (q == 1u) return CLOCK_SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-CLOCK_SINE_QUARTER[o];
    return (int16_t)-CLOCK_SINE_QUARTER[64u - o];
}

static int16_t clock_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = clock_sine_point(i);
    int32_t b = clock_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static int16_t clock_triangle_q15(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    if (p < 16384u) return (int16_t)((uint32_t)p * 2u);
    if (p < 49152u) return (int16_t)(32767 - (int32_t)(p - 16384u) * 2);
    return (int16_t)(-32767 + (int32_t)(p - 49152u) * 2);
}

static uint16_t clock_quarter_sine_q15(uint16_t x_q15)
{
    uint32_t scaled = (uint32_t)x_q15 * 64u;
    uint32_t index = scaled / APSIS_Q15_ONE;
    uint32_t rem;
    int32_t a, b;
    if (index >= 64u) return 32767u;
    rem = scaled - index * APSIS_Q15_ONE;
    a = CLOCK_SINE_QUARTER[index];
    b = CLOCK_SINE_QUARTER[index + 1u];
    return (uint16_t)(a + ((int32_t)(b - a) * (int32_t)rem +
                           (int32_t)APSIS_Q15_ONE / 2) /
                          (int32_t)APSIS_Q15_ONE);
}

static void clock_pan_milli(int16_t pan_milli, uint16_t *left, uint16_t *right)
{
    int32_t pan = pan_milli;
    uint16_t x;
    if (pan < -1000) pan = -1000;
    if (pan > 1000) pan = 1000;
    x = (uint16_t)(((uint32_t)(pan + 1000) * APSIS_Q15_ONE + 1000u) / 2000u);
    if (right) *right = clock_quarter_sine_q15(x);
    if (left) *left = clock_quarter_sine_q15((uint16_t)(APSIS_Q15_ONE - x));
}

static uint32_t clock_phase_inc_from_millihz(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint32_t clock_pitch_hz_milli(uint32_t hz, uint8_t pitch_centi)
{
    uint32_t pitch = pitch_centi;
    if (pitch < 50u) pitch = 50u;
    if (pitch > 200u) pitch = 200u;
    return hz * pitch * 10u;
}

static int32_t clock_sin_q30_for_hz(uint16_t hz)
{
    uint32_t phase = (uint32_t)(((uint64_t)hz << 32) / APSIS_CLOCK_DSP_RATE);
    return (int32_t)(((int64_t)clock_sine_q15(phase) * CLOCK_Q30_ONE) / 32767ll);
}

static int32_t clock_cos_q30_for_hz(uint16_t hz)
{
    uint32_t phase = (uint32_t)(((uint64_t)hz << 32) / APSIS_CLOCK_DSP_RATE);
    return (int32_t)(((int64_t)clock_sine_q15(phase + 0x40000000u) * CLOCK_Q30_ONE) / 32767ll);
}

static int32_t clock_norm_q30(int64_t value_q30, int64_t a0_q30)
{
    return (int32_t)((value_q30 * CLOCK_Q30_ONE) / a0_q30);
}

static void clock_biquad_clear(ApsisClockBiquad *b)
{
    if (!b) return;
    b->x1 = b->x2 = b->y1 = b->y2 = 0;
}

static void clock_biquad_lowpass(ApsisClockBiquad *b, uint16_t hz)
{
    int64_t s = clock_sin_q30_for_hz(hz), c = clock_cos_q30_for_hz(hz);
    int64_t alpha = (s * 10ll) / 7ll;
    int64_t a0 = CLOCK_Q30_ONE + alpha;
    int64_t one_minus_c = CLOCK_Q30_ONE - c;
    if (!b) return;
    b->b0_q30 = clock_norm_q30(one_minus_c / 2ll, a0);
    b->b1_q30 = clock_norm_q30(one_minus_c, a0);
    b->b2_q30 = b->b0_q30;
    b->a1_q30 = clock_norm_q30(-2ll * c, a0);
    b->a2_q30 = clock_norm_q30(CLOCK_Q30_ONE - alpha, a0);
    clock_biquad_clear(b);
}

static void clock_biquad_highpass(ApsisClockBiquad *b, uint16_t hz)
{
    int64_t s = clock_sin_q30_for_hz(hz), c = clock_cos_q30_for_hz(hz);
    int64_t alpha = s / 2ll;
    int64_t a0 = CLOCK_Q30_ONE + alpha;
    int64_t one_plus_c = CLOCK_Q30_ONE + c;
    if (!b) return;
    b->b0_q30 = clock_norm_q30(one_plus_c / 2ll, a0);
    b->b1_q30 = clock_norm_q30(-one_plus_c, a0);
    b->b2_q30 = b->b0_q30;
    b->a1_q30 = clock_norm_q30(-2ll * c, a0);
    b->a2_q30 = clock_norm_q30(CLOCK_Q30_ONE - alpha, a0);
    clock_biquad_clear(b);
}

static void clock_biquad_bandpass(ApsisClockBiquad *b, uint16_t hz)
{
    int64_t s = clock_sin_q30_for_hz(hz), c = clock_cos_q30_for_hz(hz);
    int64_t alpha = (s * 10ll) / 23ll;
    int64_t a0 = CLOCK_Q30_ONE + alpha;
    if (!b) return;
    b->b0_q30 = clock_norm_q30(alpha, a0);
    b->b1_q30 = 0;
    b->b2_q30 = -b->b0_q30;
    b->a1_q30 = clock_norm_q30(-2ll * c, a0);
    b->a2_q30 = clock_norm_q30(CLOCK_Q30_ONE - alpha, a0);
    clock_biquad_clear(b);
}

static int32_t clock_biquad_process(ApsisClockBiquad *b, int32_t x)
{
    int64_t acc;
    int32_t y;
    if (!b) return x;
    acc = (int64_t)b->b0_q30 * x + (int64_t)b->b1_q30 * b->x1 +
          (int64_t)b->b2_q30 * b->x2 - (int64_t)b->a1_q30 * b->y1 -
          (int64_t)b->a2_q30 * b->y2;
    y = clock_sat_q15(acc >> 30);
    b->x2 = b->x1;
    b->x1 = x;
    b->y2 = b->y1;
    b->y1 = y;
    return y;
}

static uint32_t clock_noise_next(uint32_t *state)
{
    uint32_t x = state && *state ? *state : CLOCK_NOISE_SEED;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (state) *state = x;
    return x;
}

static uint16_t clock_active_count(const ApsisClockDsp *dsp)
{
    uint16_t n = 0u;
    uint8_t i;
    if (!dsp) return 0u;
    for (i = 0u; i < APSIS_CLOCK_DSP_VOICES; ++i)
        if (dsp->voice[i].active) ++n;
    return n;
}

static ApsisClockDspVoice *clock_alloc(ApsisClockDsp *dsp)
{
    uint8_t i;
    if (!dsp) return 0;
    for (i = 0u; i < APSIS_CLOCK_DSP_VOICES; ++i) {
        if (!dsp->voice[i].active) {
            uint16_t n;
            clock_zero(&dsp->voice[i], (uint32_t)sizeof(dsp->voice[i]));
            dsp->voice[i].active = 1u;
            dsp->trigger_count++;
            n = clock_active_count(dsp);
            if (n > dsp->high_water) dsp->high_water = n;
            return &dsp->voice[i];
        }
    }
    dsp->drop_count++;
    return 0;
}

static uint32_t clock_decay_mul(uint32_t peak_q23, uint32_t decay_samples)
{
    uint32_t floor_q23 = CLOCK_GAIN_FLOOR_Q15 << 8;
    uint32_t target;
    if (!decay_samples || peak_q23 <= floor_q23) return 0u;
    target = (uint32_t)(((uint64_t)floor_q23 << 31) / peak_q23);
    return clock_nth_root_q31(target, decay_samples);
}

static uint32_t clock_pitch_mul(uint32_t start_inc, uint32_t end_inc, uint32_t samples)
{
    uint32_t target;
    if (!samples || !start_inc || end_inc >= start_inc) return CLOCK_Q31_ONE;
    target = (uint32_t)(((uint64_t)end_inc << 31) / start_inc);
    return clock_nth_root_q31(target, samples);
}

static void clock_voice_common(ApsisClockDspVoice *v,
                               uint32_t delay, uint8_t waveform,
                               uint32_t start_mhz, uint32_t end_mhz,
                               uint32_t pitch_ramp_samples,
                               uint16_t peak_gain_q15,
                               uint32_t attack_samples,
                               uint32_t decay_end_samples,
                               uint32_t life_samples,
                               uint16_t mix_l, uint16_t mix_r)
{
    uint32_t decay_samples;
    if (!v) return;
    v->waveform = waveform;
    v->delay_samples = delay;
    v->attack_samples = attack_samples ? attack_samples : 1u;
    v->decay_end_samples = decay_end_samples;
    v->life_samples = life_samples;
    v->pitch_ramp_samples = pitch_ramp_samples;
    v->phase_inc = clock_phase_inc_from_millihz(start_mhz);
    v->end_phase_inc = clock_phase_inc_from_millihz(end_mhz ? end_mhz : start_mhz);
    v->pitch_mul_q31 = clock_pitch_mul(v->phase_inc, v->end_phase_inc, pitch_ramp_samples);
    v->peak_gain_q23 = (uint32_t)peak_gain_q15 << 8;
    v->gain_q23 = 0u;
    decay_samples = decay_end_samples > v->attack_samples
        ? decay_end_samples - v->attack_samples : 1u;
    v->decay_mul_q31 = clock_decay_mul(v->peak_gain_q23, decay_samples);
    v->mix_left_q15 = mix_l;
    v->mix_right_q15 = mix_r;
}

static uint32_t clock_event_delay(const ApsisTransportEvent *event, uint64_t block_start)
{
    uint64_t d;
    if (!event || event->sample <= block_start) return 0u;
    d = event->sample - block_start;
    return d > 0xffffffffull ? 0xffffffffu : (uint32_t)d;
}

static uint16_t clock_peak_gain(uint16_t base_milli, uint16_t source_level, uint16_t scale_q15)
{
    uint32_t source = source_level > CLOCK_LEVEL_MAX_Q15 ? CLOCK_LEVEL_MAX_Q15 : source_level;
    uint32_t scale = scale_q15 > CLOCK_LEVEL_MAX_Q15 ? CLOCK_LEVEL_MAX_Q15 : scale_q15;
    uint32_t g = APSIS_Q15_MILLI(base_milli);
    g = clock_mul_q15_u32(g, source);
    g = clock_mul_q15_u32(g, scale);
    if (g < APSIS_Q15_MILLI(1)) g = APSIS_Q15_MILLI(1);
    return (uint16_t)(g > 65535u ? 65535u : g);
}

static uint8_t clock_add_perc(ApsisClockDsp *dsp, const ApsisState *s,
                              uint32_t delay, uint8_t accent, uint8_t ghost,
                              uint16_t scale_q15)
{
    ApsisClockDspVoice *v;
    uint8_t voice, index, waveform;
    uint16_t duration_ms, peak;
    uint32_t start_mhz, end_mhz, pitch_ms, allocated = 0u;
    if (!dsp || !s || !s->clock.sound || !s->clock.level_q15) return 0u;
    voice = s->clock.voice <= APSIS_CLOCK_SOFT ? s->clock.voice : APSIS_CLOCK_STONE;
    index = accent ? 0u : (ghost ? 2u : 1u);
    duration_ms = CLOCK_DURATION_MS[voice];
    waveform = voice == APSIS_CLOCK_WOOD ? APSIS_CLOCK_DSP_TRIANGLE : APSIS_CLOCK_DSP_SINE;
    start_mhz = clock_pitch_hz_milli(CLOCK_START_HZ[voice][index], s->clock.pitch_centi);
    end_mhz = clock_pitch_hz_milli(CLOCK_END_HZ[voice][index], s->clock.pitch_centi);
    pitch_ms = (duration_ms * 65u + 50u) / 100u;
    if (pitch_ms > 90u) pitch_ms = 90u;
    peak = clock_peak_gain(CLOCK_GAIN_MILLI[voice][index], s->clock.level_q15, scale_q15);
    v = clock_alloc(dsp);
    if (v) {
        clock_voice_common(v, delay, waveform, start_mhz, end_mhz,
                           clock_samples_ms(pitch_ms), peak,
                           clock_samples_tenth_ms(25u), clock_samples_ms(duration_ms),
                           clock_samples_ms(duration_ms + 35u),
                           APSIS_Q15_ONE, APSIS_Q15_ONE);
        if (voice == APSIS_CLOCK_WOOD) {
            v->filter_count = 1u;
            clock_biquad_lowpass(&v->filter[0], 2600u);
        }
        allocated++;
    }
    if (voice == APSIS_CLOCK_GLASS && accent) {
        uint32_t overtone_mhz = (start_mhz * 1503u + 500u) / 1000u;
        uint16_t overtone_peak = clock_peak_gain(18u, s->clock.level_q15, APSIS_Q15_ONE);
        v = clock_alloc(dsp);
        if (v) {
            clock_voice_common(v, delay, APSIS_CLOCK_DSP_SINE,
                               overtone_mhz, overtone_mhz, 0u, overtone_peak,
                               clock_samples_tenth_ms(25u), clock_samples_ms(420u),
                               clock_samples_ms(450u), APSIS_Q15_ONE, APSIS_Q15_ONE);
            allocated++;
        }
    }
    return (uint8_t)allocated;
}

static uint8_t clock_add_snap_noise(ApsisClockDsp *dsp, const ApsisState *s,
                                    uint32_t delay, uint16_t base_gain_milli,
                                    uint16_t decay_ms, int16_t pan_milli,
                                    uint16_t tone_hz)
{
    ApsisClockDspVoice *v;
    uint16_t mix_l, mix_r, peak, snap_scale;
    if (!dsp || !s) return 0u;
    snap_scale = s->clock.snap_level_q15 > CLOCK_LEVEL_MAX_Q15
        ? CLOCK_LEVEL_MAX_Q15 : s->clock.snap_level_q15;
    peak = clock_peak_gain(base_gain_milli, s->clock.level_q15, snap_scale);
    v = clock_alloc(dsp);
    if (!v) return 0u;
    clock_pan_milli(pan_milli, &mix_l, &mix_r);
    clock_voice_common(v, delay, APSIS_CLOCK_DSP_NOISE, 1u, 1u, 0u, peak,
                       clock_samples_ms(2u), clock_samples_ms(decay_ms),
                       clock_samples_ms(decay_ms + 20u), mix_l, mix_r);
    v->noise_state = CLOCK_NOISE_SEED;
    v->filter_count = 2u;
    clock_biquad_highpass(&v->filter[0], 1100u);
    clock_biquad_bandpass(&v->filter[1], tone_hz);
    return 1u;
}

static uint8_t clock_add_snap(ApsisClockDsp *dsp, const ApsisState *s, uint32_t delay)
{
    ApsisClockDspVoice *v;
    uint16_t tone, snap_scale, peak;
    uint32_t start_mhz, end_mhz;
    uint8_t n = 0u;
    if (!dsp || !s || !s->clock.sound || !s->clock.level_q15) return 0u;
    tone = s->clock.snap_tone_hz;
    if (tone < 1800u) tone = 1800u;
    if (tone > 8500u) tone = 8500u;
    tone = (uint16_t)(((uint32_t)tone + 50u) / 100u * 100u);
    n += clock_add_snap_noise(dsp, s, delay, 78u, 55u, -180, tone);
    n += clock_add_snap_noise(dsp, s, delay + clock_samples_ms(13u), 55u, 40u, 200, tone);
    n += clock_add_snap_noise(dsp, s, delay + clock_samples_ms(27u), 32u, 28u, -50, tone);
    snap_scale = s->clock.snap_level_q15 > CLOCK_LEVEL_MAX_Q15
        ? CLOCK_LEVEL_MAX_Q15 : s->clock.snap_level_q15;
    peak = clock_peak_gain(18u, s->clock.level_q15, snap_scale);
    start_mhz = (uint32_t)tone * 310u;
    end_mhz = (uint32_t)tone * 120u;
    if (end_mhz < 120000u) end_mhz = 120000u;
    v = clock_alloc(dsp);
    if (v) {
        clock_voice_common(v, delay, APSIS_CLOCK_DSP_SQUARE,
                           start_mhz, end_mhz, clock_samples_ms(18u), peak,
                           clock_samples_tenth_ms(18u), clock_samples_ms(26u),
                           clock_samples_ms(30u), APSIS_Q15_ONE, APSIS_Q15_ONE);
        n++;
    }
    return n;
}

void apsis_clock_dsp_reset(ApsisClockDsp *dsp)
{
    if (dsp) clock_zero(dsp, (uint32_t)sizeof(*dsp));
}

void apsis_clock_event_batch_reset(ApsisClockEventBatch *batch)
{
    if (batch) clock_zero(batch, (uint32_t)sizeof(*batch));
}

static uint8_t clock_is_audio_event(uint8_t type)
{
    return type == APSIS_EVENT_CLOCK_MAIN || type == APSIS_EVENT_CLOCK_SNAP ||
           type == APSIS_EVENT_CLOCK_INNER || type == APSIS_EVENT_CLOCK_GHOST;
}

static void clock_batch_add(ApsisClockEventBatch *batch, ApsisTransportEvent e)
{
    if (!batch) return;
    if (batch->count >= APSIS_CLOCK_DSP_BATCH_CAP) {
        batch->overflowed = 1u;
        return;
    }
    batch->event[batch->count++] = e;
}

static ApsisTransportEvent clock_synthetic_event(uint8_t type, uint64_t sample,
                                                  uint64_t interval_q32,
                                                  uint8_t flags)
{
    ApsisTransportEvent e;
    clock_zero(&e, (uint32_t)sizeof(e));
    e.type = type;
    e.sample = sample;
    e.interval_q32 = interval_q32;
    e.clock_step = 0u;
    e.sub_count = 1u;
    e.flags = flags;
    return e;
}

void apsis_clock_collect_block(const ApsisRuntime *r, const ApsisState *s,
                               uint32_t frames, ApsisClockEventBatch *batch)
{
    uint64_t end;
    uint16_t i;
    if (!batch) return;
    apsis_clock_event_batch_reset(batch);
    if (!r || !s || !frames) return;
    end = r->cursor_sample + frames;
    if (end < r->cursor_sample) end = ~(uint64_t)0;

    if (r->transport.running && r->queue.count == 0u &&
        r->transport.absolute_pulse == 0u && r->transport.cycle_pos == 0u &&
        r->transport.next_sample == r->cursor_sample) {
        uint64_t interval = apsis_transport_interval_q32(&s->clock,
                                                          r->transport.sample_rate,
                                                          0u);
        uint8_t flags = APSIS_EVENT_FLAG_STEP_ONE;
        if (s->clock.main_mask & 1u) {
            flags |= APSIS_EVENT_FLAG_ACCENT;
            clock_batch_add(batch, clock_synthetic_event(APSIS_EVENT_CLOCK_MAIN,
                                                         r->cursor_sample,
                                                         interval, flags));
        }
        if (s->clock.sound && (s->clock.snap_mask & 1u)) {
            uint64_t at = r->cursor_sample + clock_samples_ms(4u);
            if (at < end)
                clock_batch_add(batch,
                    clock_synthetic_event(APSIS_EVENT_CLOCK_SNAP, at, interval,
                                          APSIS_EVENT_FLAG_STEP_ONE));
        }
        if (s->clock.sound && (s->clock.inner_mask & 1u)) {
            uint64_t half = interval >> 1u;
            uint64_t off = (half + 0x80000000ull) >> 32;
            uint64_t at = r->cursor_sample + off;
            if (at < end)
                clock_batch_add(batch,
                    clock_synthetic_event(APSIS_EVENT_CLOCK_INNER, at, interval,
                                          APSIS_EVENT_FLAG_STEP_ONE));
        }
        return;
    }

    for (i = 0u; i < r->queue.count; ++i) {
        const ApsisTransportEvent *e = &r->queue.events[i];
        if (e->sample >= end) break;
        if (clock_is_audio_event(e->type)) clock_batch_add(batch, *e);
    }
}

uint8_t apsis_clock_dsp_trigger_event(ApsisClockDsp *dsp, const ApsisState *s,
                                      const ApsisTransportEvent *e,
                                      uint64_t block_start_sample)
{
    uint32_t delay;
    if (!dsp || !s || !e || !s->clock.sound) return 0u;
    delay = clock_event_delay(e, block_start_sample);
    switch (e->type) {
        case APSIS_EVENT_CLOCK_MAIN:
            return clock_add_perc(dsp, s, delay,
                                  (e->flags & APSIS_EVENT_FLAG_ACCENT) ? 1u : 0u,
                                  0u, APSIS_Q15_ONE);
        case APSIS_EVENT_CLOCK_INNER:
            return clock_add_perc(dsp, s, delay, 0u, 0u,
                                  s->clock.inner_level_q15);
        case APSIS_EVENT_CLOCK_GHOST:
            return clock_add_perc(dsp, s, delay, 0u, 1u,
                                  APSIS_Q15_MILLI(550));
        case APSIS_EVENT_CLOCK_SNAP:
            return clock_add_snap(dsp, s, delay);
        default:
            return 0u;
    }
}

void apsis_clock_dsp_render(ApsisClockDsp *dsp, int32_t *out, uint32_t frames)
{
    uint32_t f;
    if (!dsp || !out || !frames) return;
    for (f = 0u; f < frames; ++f) {
        int64_t sum_l = 0, sum_r = 0;
        uint8_t i;
        for (i = 0u; i < APSIS_CLOCK_DSP_VOICES; ++i) {
            ApsisClockDspVoice *v = &dsp->voice[i];
            int32_t wave, sample;
            uint32_t gain;
            if (!v->active) continue;
            if (v->delay_samples) {
                v->delay_samples--;
                continue;
            }
            if (v->age_samples >= v->life_samples) {
                v->active = 0u;
                continue;
            }

            if (v->waveform == APSIS_CLOCK_DSP_NOISE) {
                wave = (int16_t)(clock_noise_next(&v->noise_state) >> 16);
            } else if (v->waveform == APSIS_CLOCK_DSP_TRIANGLE) {
                wave = clock_triangle_q15(v->phase);
            } else if (v->waveform == APSIS_CLOCK_DSP_SQUARE) {
                wave = (v->phase & 0x80000000u) ? -32767 : 32767;
            } else {
                wave = clock_sine_q15(v->phase);
            }
            if (v->filter_count > 0u) wave = clock_biquad_process(&v->filter[0], wave);
            if (v->filter_count > 1u) wave = clock_biquad_process(&v->filter[1], wave);

            if (v->age_samples < v->attack_samples) {
                gain = (uint32_t)(((uint64_t)v->peak_gain_q23 *
                                   (v->age_samples + 1u)) / v->attack_samples);
                v->gain_q23 = gain;
            } else if (v->age_samples == v->attack_samples) {
                v->gain_q23 = v->peak_gain_q23;
                gain = v->gain_q23;
            } else if (v->age_samples < v->decay_end_samples) {
                v->gain_q23 = clock_mul_q31(v->gain_q23, v->decay_mul_q31);
                gain = v->gain_q23;
            } else {
                gain = 0u;
            }

            sample = (int32_t)(((int64_t)wave * (int64_t)gain) >> 23);
            sum_l += ((int64_t)sample * v->mix_left_q15) >> 15;
            sum_r += ((int64_t)sample * v->mix_right_q15) >> 15;

            if (v->waveform != APSIS_CLOCK_DSP_NOISE) {
                v->phase += v->phase_inc;
                if (v->pitch_ramp_samples && v->age_samples < v->pitch_ramp_samples) {
                    v->phase_inc = clock_mul_q31(v->phase_inc, v->pitch_mul_q31);
                    if (v->age_samples + 1u >= v->pitch_ramp_samples)
                        v->phase_inc = v->end_phase_inc;
                }
            }
            v->age_samples++;
            if (v->age_samples >= v->life_samples) v->active = 0u;
        }
        out[f * 2u] = clock_sat_q15((int64_t)out[f * 2u] + sum_l);
        out[f * 2u + 1u] = clock_sat_q15((int64_t)out[f * 2u + 1u] + sum_r);
    }
}
