#include "apsis_controls.h"
#include "apsis_harmony.h"

#define STRING_RESONANCE_MIN APSIS_Q15_MILLI(300)
#define STRING_RESONANCE_MAX APSIS_Q15_MILLI(880)
#define STRING_RESONANCE_STEP APSIS_Q15_MILLI(10)
#define STRING_BRIGHTNESS_MIN APSIS_Q15_MILLI(200)
#define STRING_BRIGHTNESS_MAX APSIS_Q15_MILLI(820)
#define STRING_BRIGHTNESS_STEP APSIS_Q15_MILLI(10)
#define STRING_GLIDE_MIN APSIS_Q15_MILLI(18)
#define STRING_GLIDE_MAX APSIS_Q15_MILLI(380)
#define STRING_GLIDE_STEP APSIS_Q15_MILLI(4)
#define STRING_NUDGE_Q15 ((uint16_t)(((uint32_t)APSIS_Q15_ONE * 45u + 5000u) / 10000u))

static uint32_t apsis_button_bit(uint8_t button)
{
    return 1u << button;
}

static int32_t apsis_controls_abs_i32(int32_t v)
{
    return v < 0 ? -v : v;
}

static uint16_t apsis_controls_clamp_q15_i64(int64_t v, uint16_t lo, uint16_t hi)
{
    if (v < (int64_t)lo) return lo;
    if (v > (int64_t)hi) return hi;
    return (uint16_t)v;
}

static uint8_t apsis_controls_clock_n(const ApsisClockState *clock)
{
    uint8_t n;
    if (!clock) return 3u;
    n = clock->n;
    if (n < 3u) return 3u;
    if (n > APSIS_CLOCK_STEPS_MAX) return APSIS_CLOCK_STEPS_MAX;
    return n;
}

static void apsis_controls_bounds_normalize(ApsisClockState *clock, uint8_t n)
{
    uint8_t v[3] = {0u, 0u, 0u};
    uint8_t count = 0u, i, j;
    if (!clock) return;
    for (i = 0u; i < 3u; ++i) {
        uint8_t b = clock->bounds[i];
        uint8_t duplicate = 0u;
        if (b == 0u || b >= n) continue;
        for (j = 0u; j < count; ++j)
            if (v[j] == b) duplicate = 1u;
        if (!duplicate && count < 3u) v[count++] = b;
    }
    for (i = 1u; i < count; ++i) {
        uint8_t x = v[i];
        j = i;
        while (j > 0u && v[j - 1u] > x) {
            v[j] = v[j - 1u];
            --j;
        }
        v[j] = x;
    }
    for (i = 0u; i < 3u; ++i) clock->bounds[i] = i < count ? v[i] : 0u;
}

static uint8_t apsis_controls_toggle_boundary(ApsisClockState *clock,
                                              uint8_t n, uint8_t step)
{
    uint8_t i;
    if (!clock || step == 0u || step >= n) return 0u;
    apsis_controls_bounds_normalize(clock, n);
    for (i = 0u; i < 3u; ++i) {
        if (clock->bounds[i] == step) {
            clock->bounds[i] = 0u;
            apsis_controls_bounds_normalize(clock, n);
            return 1u;
        }
    }
    for (i = 0u; i < 3u; ++i) {
        if (clock->bounds[i] == 0u) {
            clock->bounds[i] = step;
            apsis_controls_bounds_normalize(clock, n);
            return 1u;
        }
    }
    return 0u;
}

static uint16_t apsis_controls_string_clamp(uint8_t side, int64_t split)
{
    uint16_t lo = side == APSIS_SIDE_LEFT ? APSIS_STRING_SPLIT_MIN_Q15
                                           : APSIS_STRING_SPLIT_MID_Q15;
    uint16_t hi = side == APSIS_SIDE_LEFT ? APSIS_STRING_SPLIT_MID_Q15
                                           : APSIS_STRING_SPLIT_MAX_Q15;
    return apsis_controls_clamp_q15_i64(split, lo, hi);
}

static int32_t apsis_controls_string_target_milli(const ApsisState *state,
                                                  uint8_t key_index,
                                                  uint8_t bank_base)
{
    uint8_t local, group, octave;
    int32_t seq;
    if (!state) return 0;
    if (key_index >= APSIS_FM1_KEY_COUNT) key_index = APSIS_FM1_KEY_COUNT - 1u;
    bank_base = bank_base > APSIS_STRING_BANK_BASE_MAX ? APSIS_STRING_BANK_BASE_MAX : bank_base;
    group = (uint8_t)(key_index / APSIS_STRING_KEYS_PER_BANK);
    local = (uint8_t)(key_index % APSIS_STRING_KEYS_PER_BANK);
    octave = (uint8_t)(bank_base + group);
    if (octave > 3u) octave = 3u;

    if (!state->string.quantize)
        return (int32_t)octave * 12000 + ((int32_t)16000 * local + 4) / 8;
    if (state->string.harmony == APSIS_HARMONY_CHROMATIC)
        return (int32_t)octave * 12000 + ((int32_t)12000 * local + 4) / 8;
    if (state->string.harmony == APSIS_HARMONY_CHORD) {
        int16_t chord[APSIS_GRAVITY_VOICES_MAX];
        uint8_t n = apsis_gravity_ring_intervals(&state->gravity, chord);
        if (!n) return (int32_t)octave * 12000;
        seq = (int32_t)local + (int32_t)octave * n;
        return (seq / n) * 12000 + (int32_t)chord[seq % n] * 1000;
    }
    {
        const int8_t *scale = apsis_scale_intervals(state->gravity.scale);
        seq = (int32_t)local + (int32_t)octave * APSIS_SCALE_DEGREES;
        return (seq / (int32_t)APSIS_SCALE_DEGREES) * 12000 +
               (int32_t)scale[seq % (int32_t)APSIS_SCALE_DEGREES] * 1000;
    }
}

static uint16_t apsis_controls_string_right_split_for_milli(const ApsisState *state,
                                                            int32_t target_milli)
{
    uint16_t lo = APSIS_STRING_SPLIT_MID_Q15;
    uint16_t hi = APSIS_STRING_SPLIT_MAX_Q15;
    uint16_t best = lo;
    int32_t best_err = 0x7FFFFFFF;
    uint8_t i;
    if (!state || target_milli <= 0) return lo;
    for (i = 0u; i < 16u && lo <= hi; ++i) {
        uint16_t mid = (uint16_t)(lo + (uint16_t)((hi - lo) / 2u));
        int32_t mag = apsis_string_magnitude_milli(mid, state->string.span_q15);
        int32_t err = apsis_controls_abs_i32(mag - target_milli);
        if (err < best_err) { best_err = err; best = mid; }
        if (mag < target_milli) {
            if (mid == 0xFFFFu) break;
            lo = (uint16_t)(mid + 1u);
        } else {
            if (mid == 0u) break;
            hi = (uint16_t)(mid - 1u);
        }
    }
    if (lo <= APSIS_STRING_SPLIT_MAX_Q15) {
        int32_t mag = apsis_string_magnitude_milli(lo, state->string.span_q15);
        int32_t err = apsis_controls_abs_i32(mag - target_milli);
        if (err < best_err) best = lo;
    }
    return best;
}

static uint16_t apsis_controls_q15_step(uint16_t current, int32_t detents,
                                        uint16_t step, uint16_t lo, uint16_t hi)
{
    uint16_t base = current < lo ? lo : (current > hi ? hi : current);
    return apsis_controls_clamp_q15_i64((int64_t)base + (int64_t)detents * step, lo, hi);
}

static uint16_t apsis_controls_span_step(uint16_t current, int32_t detents)
{
    static const uint16_t values[4] = {
        APSIS_Q15_MILLI(720), APSIS_Q15_ONE,
        APSIS_Q15_MILLI(1280), APSIS_Q15_MILLI(1560)
    };
    uint8_t i, best = 0u;
    int32_t best_err = 0x7FFFFFFF;
    int32_t idx;
    for (i = 0u; i < 4u; ++i) {
        int32_t err = apsis_controls_abs_i32((int32_t)current - (int32_t)values[i]);
        if (err < best_err) { best_err = err; best = i; }
    }
    idx = (int32_t)best + detents;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;
    return values[idx];
}

static uint8_t apsis_controls_spiral_count(ApsisSpiralState *spiral)
{
    if (!spiral) return 0u;
    if (spiral->route_count > APSIS_SPIRAL_ROUTE_MAX)
        spiral->route_count = APSIS_SPIRAL_ROUTE_MAX;
    return spiral->route_count;
}

uint8_t apsis_controls_surface_from_fm1(const ApsisFm1InputFrame *input,
                                        uint8_t current_surface)
{
    uint32_t p, h;
    uint8_t current = current_surface < APSIS_SURFACE_COUNT ? current_surface : APSIS_SURFACE_MOTHER;
    if (!input) return current;
    p = input->buttons_pressed;
    h = input->buttons_held;
    if (p & apsis_button_bit(APSIS_FM1_BTN_HOME)) return APSIS_SURFACE_MOTHER;
    if ((h & apsis_button_bit(APSIS_FM1_BTN_SEL)) && (p & apsis_button_bit(APSIS_FM1_BTN_FX))) return APSIS_SURFACE_SPACE;
    if (p & apsis_button_bit(APSIS_FM1_BTN_ENV)) return APSIS_SURFACE_STRING;
    if (p & apsis_button_bit(APSIS_FM1_BTN_LFO)) return APSIS_SURFACE_DRIFT;
    if (p & apsis_button_bit(APSIS_FM1_BTN_SEQ)) return APSIS_SURFACE_CLOCK;
    if (p & apsis_button_bit(APSIS_FM1_BTN_ARP)) return APSIS_SURFACE_SPIRAL;
    if (p & apsis_button_bit(APSIS_FM1_BTN_FX)) return APSIS_SURFACE_BLOOM;
    if (p & apsis_button_bit(APSIS_FM1_BTN_GLO)) return APSIS_SURFACE_GRAVITY;
    if (p & apsis_button_bit(APSIS_FM1_BTN_EDIT)) return APSIS_SURFACE_CHAIN;
    if (p & apsis_button_bit(APSIS_FM1_BTN_SAVE)) return APSIS_SURFACE_KEEP;
    return current;
}

ApsisGlobalAction apsis_controls_global_action(const ApsisFm1InputFrame *input)
{
    uint32_t play, select;
    if (!input) return APSIS_GLOBAL_ACTION_NONE;
    play = apsis_button_bit(APSIS_FM1_BTN_PLAY);
    select = apsis_button_bit(APSIS_FM1_BTN_SEL);
    if ((input->buttons_pressed & play) && !(input->buttons_held & select)) return APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE;
    return APSIS_GLOBAL_ACTION_NONE;
}

uint8_t apsis_controls_clock_layer_step(uint8_t current_layer, int32_t detents)
{
    int32_t layer = current_layer < APSIS_CLOCK_CONTROL_LAYER_COUNT ? (int32_t)current_layer : (int32_t)APSIS_CLOCK_CONTROL_MAIN;
    int32_t span = (int32_t)APSIS_CLOCK_CONTROL_LAYER_COUNT;
    int32_t delta = detents % span;
    layer = (layer + delta) % span;
    if (layer < 0) layer += span;
    return (uint8_t)layer;
}

uint8_t apsis_controls_clock_apply_keys(ApsisClockState *clock, uint8_t layer, uint32_t notes_pressed)
{
    uint8_t n, step, changed = 0u;
    if (!clock || layer >= APSIS_CLOCK_CONTROL_LAYER_COUNT) return 0u;
    n = apsis_controls_clock_n(clock);
    for (step = 0u; step < n; ++step) {
        uint32_t bit = 1u << step;
        if (!(notes_pressed & bit)) continue;
        if (layer == APSIS_CLOCK_CONTROL_MAIN) { clock->main_mask ^= bit; ++changed; }
        else if (layer == APSIS_CLOCK_CONTROL_BETWEEN) { clock->inner_mask ^= bit; ++changed; }
        else if (layer == APSIS_CLOCK_CONTROL_SNAP) { clock->snap_mask ^= bit; ++changed; }
        else if (apsis_controls_toggle_boundary(clock, n, step)) ++changed;
    }
    return changed;
}

uint8_t apsis_controls_gravity_degree_from_fm1(const ApsisFm1InputFrame *input, uint8_t *degree_out)
{
    uint8_t degree;
    if (!input || !degree_out) return 0u;
    for (degree = 0u; degree < APSIS_GRAVITY_DIRECT_KEYS; ++degree)
        if (input->notes_pressed & (1u << degree)) { *degree_out = degree; return 1u; }
    return 0u;
}

uint8_t apsis_controls_gravity_ring_from_fm1(const ApsisFm1InputFrame *input, uint8_t current_ring)
{
    uint8_t ring = current_ring < 3u ? current_ring : 0u;
    uint8_t down, up;
    if (!input) return ring;
    down = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_DOWN)) != 0u;
    up = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_UP)) != 0u;
    if (down == up) return ring;
    if (up) return (uint8_t)((ring + 1u) % 3u);
    return ring == 0u ? 2u : (uint8_t)(ring - 1u);
}

uint8_t apsis_controls_string_side_from_fm1(const ApsisStringState *string, const ApsisFm1InputFrame *input)
{
    if (!string || string->surface_mode == APSIS_STRING_CLASSIC) return APSIS_SIDE_RIGHT;
    if (input && (input->buttons_held & apsis_button_bit(APSIS_FM1_BTN_SEL))) return APSIS_SIDE_LEFT;
    return APSIS_SIDE_RIGHT;
}

uint8_t apsis_controls_string_key_from_fm1(const ApsisFm1InputFrame *input, uint8_t *key_out)
{
    uint8_t key;
    if (!input || !key_out) return 0u;
    for (key = 0u; key < APSIS_FM1_KEY_COUNT; ++key)
        if (input->notes_pressed & (1u << key)) { *key_out = key; return 1u; }
    return 0u;
}

uint8_t apsis_controls_string_bank_from_fm1(const ApsisFm1InputFrame *input, uint8_t current_base)
{
    uint8_t base = current_base > APSIS_STRING_BANK_BASE_MAX ? 0u : current_base;
    uint8_t down, up;
    if (!input) return base;
    down = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_DOWN)) != 0u;
    up = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_UP)) != 0u;
    if (down == up) return base;
    if (up) return base < APSIS_STRING_BANK_BASE_MAX ? (uint8_t)(base + 1u) : base;
    return base ? (uint8_t)(base - 1u) : 0u;
}

uint16_t apsis_controls_string_split_for_key(const ApsisState *state, uint8_t side,
                                             uint8_t key_index, uint8_t bank_base)
{
    uint16_t right;
    int32_t target = apsis_controls_string_target_milli(state, key_index, bank_base);
    if (target < 0) target = 0;
    right = apsis_controls_string_right_split_for_milli(state, target);
    if (side == APSIS_SIDE_LEFT) {
        uint16_t delta = right > APSIS_STRING_SPLIT_MID_Q15 ? (uint16_t)(right - APSIS_STRING_SPLIT_MID_Q15) : 0u;
        return (uint16_t)(APSIS_STRING_SPLIT_MID_Q15 - delta);
    }
    return right;
}

uint16_t apsis_controls_string_split_nudge(uint8_t side, uint16_t current_split, int32_t detents)
{
    int32_t direction = side == APSIS_SIDE_LEFT ? -1 : 1;
    return apsis_controls_string_clamp(side, (int64_t)current_split + (int64_t)direction * detents * STRING_NUDGE_Q15);
}

uint8_t apsis_controls_string_mode_step(uint8_t current_mode, int32_t detents)
{
    uint8_t mode = current_mode == APSIS_STRING_CLASSIC ? APSIS_STRING_CLASSIC : APSIS_STRING_DUAL;
    int32_t odd = detents % 2;
    if (odd < 0) odd = -odd;
    return odd ? (uint8_t)(mode == APSIS_STRING_CLASSIC ? APSIS_STRING_DUAL : APSIS_STRING_CLASSIC) : mode;
}

uint8_t apsis_controls_string_param_edits(const ApsisStringState *string,
                                          const ApsisFm1InputFrame *input,
                                          ApsisControlParamEdit out[4])
{
    uint8_t count = 0u;
    int32_t d;
    if (!string || !input || !out) return 0u;
    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) { out[count].param = APSIS_CONTROL_PARAM_STRING_RESONANCE; out[count++].value = apsis_controls_q15_step(string->resonance_q15, d, STRING_RESONANCE_STEP, STRING_RESONANCE_MIN, STRING_RESONANCE_MAX); }
    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) { out[count].param = APSIS_CONTROL_PARAM_STRING_BRIGHTNESS; out[count++].value = apsis_controls_q15_step(string->brightness_q15, d, STRING_BRIGHTNESS_STEP, STRING_BRIGHTNESS_MIN, STRING_BRIGHTNESS_MAX); }
    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) { out[count].param = APSIS_CONTROL_PARAM_STRING_GLIDE; out[count++].value = apsis_controls_q15_step(string->glide_q15, d, STRING_GLIDE_STEP, STRING_GLIDE_MIN, STRING_GLIDE_MAX); }
    d = input->encoder_steps[APSIS_FM1_ENC_K4];
    if (d) { out[count].param = APSIS_CONTROL_PARAM_STRING_SPAN; out[count++].value = apsis_controls_span_step(string->span_q15, d); }
    return count;
}

uint8_t apsis_controls_spiral_bank_from_fm1(const ApsisFm1InputFrame *input,
                                            uint8_t current_bank)
{
    uint8_t bank = current_bank < APSIS_SPIRAL_BANK_COUNT ? current_bank : 0u;
    uint8_t down, up;
    if (!input) return bank;
    down = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_DOWN)) != 0u;
    up = (input->buttons_pressed & apsis_button_bit(APSIS_FM1_BTN_OCT_UP)) != 0u;
    if (down == up) return bank;
    if (up) return 1u;
    return 0u;
}

uint8_t apsis_controls_spiral_point_from_fm1(const ApsisFm1InputFrame *input,
                                             uint8_t bank,
                                             uint8_t *point_out)
{
    uint8_t key;
    if (!input || !point_out) return 0u;
    bank = bank < APSIS_SPIRAL_BANK_COUNT ? bank : 0u;
    for (key = 0u; key < APSIS_FM1_KEY_COUNT; ++key) {
        if (!(input->notes_pressed & (1u << key))) continue;
        if (bank == 0u) {
            *point_out = key;
            return 1u;
        }
        if (key < APSIS_SPIRAL_UPPER_BANK_KEYS) {
            *point_out = (uint8_t)(APSIS_SPIRAL_UPPER_BANK_OFFSET + key);
            return 1u;
        }
    }
    return 0u;
}

uint8_t apsis_controls_spiral_cursor_step(uint8_t current_cursor,
                                          uint8_t route_count,
                                          int32_t detents)
{
    int32_t cursor, count, delta;
    route_count = route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : route_count;
    if (!route_count) return 0u;
    count = route_count;
    cursor = current_cursor < route_count ? current_cursor : (int32_t)route_count - 1;
    delta = detents % count;
    cursor = (cursor + delta) % count;
    if (cursor < 0) cursor += count;
    return (uint8_t)cursor;
}

uint8_t apsis_controls_spiral_append(ApsisSpiralState *spiral,
                                     uint8_t point,
                                     uint8_t *cursor_io)
{
    uint8_t count, i;
    if (!spiral || point >= APSIS_SPIRAL_POINTS) return 0u;
    count = apsis_controls_spiral_count(spiral);
    if (count < APSIS_SPIRAL_ROUTE_MAX) {
        spiral->route[count] = point;
        spiral->route_count = (uint8_t)(count + 1u);
        if (cursor_io) *cursor_io = count;
        return 1u;
    }
    for (i = 1u; i < APSIS_SPIRAL_ROUTE_MAX; ++i)
        spiral->route[i - 1u] = spiral->route[i];
    spiral->route[APSIS_SPIRAL_ROUTE_MAX - 1u] = point;
    if (cursor_io) *cursor_io = APSIS_SPIRAL_ROUTE_MAX - 1u;
    return 1u;
}

uint8_t apsis_controls_spiral_insert(ApsisSpiralState *spiral,
                                     uint8_t point,
                                     uint8_t *cursor_io)
{
    uint8_t count, cursor, i;
    if (!spiral || point >= APSIS_SPIRAL_POINTS) return 0u;
    count = apsis_controls_spiral_count(spiral);
    if (count >= APSIS_SPIRAL_ROUTE_MAX) return 0u;
    cursor = cursor_io && *cursor_io < count ? *cursor_io : count;
    for (i = count; i > cursor; --i)
        spiral->route[i] = spiral->route[i - 1u];
    spiral->route[cursor] = point;
    spiral->route_count = (uint8_t)(count + 1u);
    if (cursor_io) *cursor_io = cursor;
    return 1u;
}

uint8_t apsis_controls_spiral_move_selected(ApsisSpiralState *spiral,
                                            uint8_t cursor,
                                            int32_t detents)
{
    uint8_t count;
    int32_t point;
    if (!spiral || !detents) return 0u;
    count = apsis_controls_spiral_count(spiral);
    if (!count || cursor >= count) return 0u;
    point = (int32_t)spiral->route[cursor] + detents;
    if (point < 0) point = 0;
    if (point >= (int32_t)APSIS_SPIRAL_POINTS) point = APSIS_SPIRAL_POINTS - 1;
    if ((uint8_t)point == spiral->route[cursor]) return 0u;
    spiral->route[cursor] = (uint8_t)point;
    return 1u;
}

uint8_t apsis_controls_spiral_delete_selected(ApsisSpiralState *spiral,
                                              uint8_t *cursor_io)
{
    uint8_t count, cursor, i;
    if (!spiral) return 0u;
    count = apsis_controls_spiral_count(spiral);
    if (!count) {
        if (cursor_io) *cursor_io = 0u;
        return 0u;
    }
    cursor = cursor_io && *cursor_io < count ? *cursor_io : (uint8_t)(count - 1u);
    for (i = (uint8_t)(cursor + 1u); i < count; ++i)
        spiral->route[i - 1u] = spiral->route[i];
    spiral->route[count - 1u] = 0u;
    spiral->route_count = (uint8_t)(count - 1u);
    if (cursor_io) {
        if (!spiral->route_count) *cursor_io = 0u;
        else if (cursor >= spiral->route_count) *cursor_io = (uint8_t)(spiral->route_count - 1u);
        else *cursor_io = cursor;
    }
    return 1u;
}
