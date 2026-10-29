/* SPDX-License-Identifier: GPL-3.0-only */
/* APSIS for M-VAVE FM-1 — parity-first native shell.
 *
 * M3b instantiates the shared transport/runtime coordinator in the real FM-1
 * single translation unit. The audio ISR advances structural time only; DSP
 * remains intentionally deferred until Phase 3 timing semantics are proven.
 */
#define APP_NAME "APSIS"
#include "platform.c"
#include "apsis_state.h"
#include "apsis_harmony.h"
#include "apsis_gravity.h"
#include "apsis_string.h"
#include "apsis_drift.h"
#include "apsis_clock.h"
#include "apsis_spiral.h"
#include "apsis_bloom.h"
#include "apsis_keep.h"
#include "apsis_chain.h"
#include "apsis_mother.h"
#include "apsis_transport.h"
#include "apsis_runtime.h"
#include "apsis_state.c"
#include "apsis_harmony.c"
#include "apsis_gravity.c"
#include "apsis_string.c"
#include "apsis_drift.c"
#include "apsis_clock.c"
#include "apsis_spiral.c"
#include "apsis_bloom.c"
#include "apsis_keep.c"
#include "apsis_chain.c"
#include "apsis_mother.c"
#include "apsis_transport.c"
#include "apsis_runtime.c"

static ApsisState apsis;
static ApsisStringPitchMemory string_pitch_memory;
static ApsisBloomVoicingState bloom_voicing_state;
static ApsisKeepBank keep_bank;
static ApsisChainSequence chain_sequence;
static ApsisMotherPending mother_pending;
static ApsisRuntime apsis_runtime;
static volatile uint8_t runtime_ready;

enum {
    SURF_MOTHER = 0,
    SURF_STRING,
    SURF_DRIFT,
    SURF_CLOCK,
    SURF_SPIRAL,
    SURF_BLOOM,
    SURF_GRAVITY,
    SURF_CHAIN,
    SURF_KEEP,
    SURF_COUNT
};

static uint8_t surface = SURF_MOTHER;

static void app_audio(int32_t *out, uint32_t n)
{
    (void)out;
    /* Dinghy supplies a zeroed stereo buffer and n is stereo frames. M3b uses
     * the callback only as the sample clock; the output intentionally stays 0. */
    if (runtime_ready)
        apsis_runtime_process(&apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                              &mother_pending, &bloom_voicing_state, n, 0, 0);
}

static const char *surface_name(uint8_t s)
{
    static const char *const names[SURF_COUNT] = {
        "MOTHER", "STRING", "DRIFT / TUNING", "CLOCK", "SPIRAL",
        "BLOOM", "GRAVITY", "CHAIN", "KEEP"
    };
    return s < SURF_COUNT ? names[s] : "MOTHER";
}

static void draw_orbit(uint8_t active)
{
    static const uint8_t px[12] = {120, 144, 162, 168, 162, 144, 120, 96, 78, 72, 78, 96};
    static const uint8_t py[12] = {3, 8, 19, 31, 43, 54, 59, 54, 43, 31, 19, 8};
    uint32_t i;
    uint16_t dim = RGB(64, 62, 58);
    uint16_t lit = RGB(210, 204, 190);
    uint16_t accent = RGB(168, 148, 112);

    cv_begin(240, 64, C_BLACK);
    for (i = 0; i < 12u; ++i) {
        uint16_t c = (active == SURF_CLOCK && (apsis.clock.main_mask & (1u << i))) ? lit : dim;
        cv_rect((int32_t)px[i] - 2, (int32_t)py[i] - 2, 5, 5, c);
    }
    cv_line(120, 3, 168, 31, active == SURF_CLOCK ? accent : dim);
    cv_line(168, 31, 120, 59, active == SURF_CLOCK ? accent : dim);
    cv_line(120, 59, 72, 31, active == SURF_CLOCK ? accent : dim);
    cv_line(72, 31, 120, 3, active == SURF_CLOCK ? accent : dim);
    cv_rect(117, 28, 7, 7, active == SURF_MOTHER ? accent : dim);
    cv_blit(0, 92);
}

static int32_t string_x(uint16_t split_q15)
{
    return 22 + (int32_t)(((uint32_t)split_q15 * 196u) / APSIS_Q15_ONE);
}

static void draw_string(void)
{
    int32_t left = string_x(apsis.string.left_split_q15);
    int32_t right = string_x(apsis.string.right_split_q15);
    const int32_t mid = string_x(APSIS_STRING_SPLIT_MID_Q15);
    uint16_t dim = RGB(64, 62, 58);
    uint16_t body = RGB(202, 197, 186);
    uint16_t wire = RGB(155, 170, 174);

    cv_begin(240, 64, C_BLACK);
    cv_line(22, 31, 218, 31, dim);
    cv_line(mid, 10, mid, 54, dim);
    cv_line(mid, 31, left, 31, body);
    cv_line(mid, 32, right, 32, wire);
    cv_rect(left - 3, 27, 7, 9, body);
    cv_rect(right - 3, 27, 7, 9, wire);
    cv_rect(mid - 2, 29, 5, 5, RGB(168, 148, 112));
    cv_blit(0, 92);
}

static uint8_t spiral_selected(uint8_t step)
{
    uint8_t i, count = apsis.spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : apsis.spiral.route_count;
    for (i = 0u; i < count; ++i) if (apsis.spiral.route[i] == step) return 1u;
    return 0u;
}

static void draw_spiral(void)
{
    static const uint8_t px[36] = {
        118,115,114,114,116,120,124,128,130,130,126,120,
        114,108,106,107,112,120,128,135,138,136,130,120,
        110,101,98,100,108,120,132,142,146,143,134,120
    };
    static const uint8_t py[36] = {
        36,35,32,28,25,24,24,27,32,37,42,44,
        43,39,32,25,18,16,17,23,32,41,49,52,
        50,43,32,21,12,8,10,19,32,46,56,60
    };
    uint8_t i;
    uint16_t linec = RGB(96, 78, 88);
    uint16_t pointc = RGB(164, 139, 151);
    uint16_t selected = RGB(216, 198, 171);
    cv_begin(240, 64, C_BLACK);
    for (i = 1u; i < 36u; ++i) cv_line(px[i - 1u], py[i - 1u], px[i], py[i], linec);
    for (i = 0u; i < 36u; ++i) {
        uint16_t c = spiral_selected(i) ? selected : pointc;
        cv_rect((int32_t)px[i] - 1, (int32_t)py[i] - 1, 3, 3, c);
        if (spiral_selected(i)) {
            cv_rect((int32_t)px[i] - 3, (int32_t)py[i] - 3, 7, 1, c);
            cv_rect((int32_t)px[i] - 3, (int32_t)py[i] + 3, 7, 1, c);
            cv_rect((int32_t)px[i] - 3, (int32_t)py[i] - 2, 1, 5, c);
            cv_rect((int32_t)px[i] + 3, (int32_t)py[i] - 2, 1, 5, c);
        }
    }
    cv_blit(0, 92);
}

static void draw_primary_surface(void)
{
    if (surface == SURF_STRING) draw_string();
    else if (surface == SURF_SPIRAL) draw_spiral();
    else draw_orbit(surface);
}

static void draw_surface(void)
{
    draw_text_box(0, 12, 240, &FONT_L, "APSIS", C_WHITE, 1);
    draw_text_box(0, 49, 240, &FONT_S, surface_name(surface), RGB(174, 168, 157), 1);
    draw_primary_surface();
    draw_text_box(0, 169, 240, &FONT_S, "SOURCE-LOCKED NATIVE PORT", RGB(92, 89, 83), 1);
    draw_text_box(0, 195, 240, &FONT_S, "M3 SILENT RUNTIME  v" APP_VER, RGB(92, 89, 83), 1);
}

static void leds(void)
{
    uint32_t i;
    for (i = 0; i < 14u; ++i) fm1_led_key(i, 0);
    fm1_led_key(BTN_HOME, surface == SURF_MOTHER);
    fm1_led_key(BTN_ENV, surface == SURF_STRING);
    fm1_led_key(BTN_LFO, surface == SURF_DRIFT);
    fm1_led_key(BTN_SEQ, surface == SURF_CLOCK);
    fm1_led_key(BTN_ARP, surface == SURF_SPIRAL);
    fm1_led_key(BTN_FX, surface == SURF_BLOOM);
    fm1_led_key(BTN_GLO, surface == SURF_GRAVITY);
    fm1_led_key(BTN_EDIT, surface == SURF_CHAIN);
    fm1_led_key(BTN_SAVE, surface == SURF_KEEP);
}

static uint8_t surface_from_buttons(uint32_t b, uint8_t current)
{
    if (b & (1u << BTN_HOME)) return SURF_MOTHER;
    if (b & (1u << BTN_ENV)) return SURF_STRING;
    if (b & (1u << BTN_LFO)) return SURF_DRIFT;
    if (b & (1u << BTN_SEQ)) return SURF_CLOCK;
    if (b & (1u << BTN_ARP)) return SURF_SPIRAL;
    if (b & (1u << BTN_FX)) return SURF_BLOOM;
    if (b & (1u << BTN_GLO)) return SURF_GRAVITY;
    if (b & (1u << BTN_EDIT)) return SURF_CHAIN;
    if (b & (1u << BTN_SAVE)) return SURF_KEEP;
    return current;
}

static void app_main(void)
{
    uint8_t drawn = 255u;
    runtime_ready = 0u;
    apsis_state_default(&apsis);
    apsis_string_pitch_memory_reset(&string_pitch_memory);
    apsis_bloom_voicing_reset(&bloom_voicing_state);
    apsis_keep_bank_reset(&keep_bank);
    apsis_chain_sequence_reset(&chain_sequence);
    apsis_mother_pending_reset(&mother_pending);
    apsis_runtime_reset(&apsis_runtime, FS, 0u, 0x41505349u);
    apsis_runtime_start(&apsis_runtime);
    RING_PUBLISH();
    runtime_ready = 1u;
    lcd_fill(0, 0, 240, 240, C_BLACK);

    for (;;) {
        uint32_t released = 0u;
        uint32_t pressed;
        uint8_t next;
        platform_poll();
        pressed = fm1_input_edges(&released);
        (void)released;
        next = surface_from_buttons(pressed, surface);
        if (next != surface) surface = next;
        leds();
        if (drawn != surface) {
            drawn = surface;
            draw_surface();
        }
    }
}
