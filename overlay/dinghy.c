/* SPDX-License-Identifier: GPL-3.0-only */
/* APSIS for M-VAVE FM-1 — Phase 1 native shell.
 *
 * This intentionally makes no sound yet. The first target milestone is a
 * reproducible APSIS identity, source-derived state model and native surface
 * shell before DSP is allowed to shape the architecture.
 */
#define APP_NAME "APSIS"
#include "platform.c"
#include "apsis_state.h"
#include "apsis_harmony.h"
#include "apsis_state.c"
#include "apsis_harmony.c"

static ApsisState apsis;

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
    (void)n;
    /* Platform supplies an already-zeroed stereo buffer. Phase 1 stays silent. */
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
    /* 12-point CLOCK orbit: first real APSIS-native visual primitive. */
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

static void draw_surface(void)
{
    draw_text_box(0, 12, 240, &FONT_L, "APSIS", C_WHITE, 1);
    draw_text_box(0, 49, 240, &FONT_S, surface_name(surface), RGB(174, 168, 157), 1);
    draw_orbit(surface);
    draw_text_box(0, 169, 240, &FONT_S, "SOURCE-LOCKED NATIVE PORT", RGB(92, 89, 83), 1);
    draw_text_box(0, 195, 240, &FONT_S, "M1 SILENT SHELL  v" APP_VER, RGB(92, 89, 83), 1);
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
    apsis_state_default(&apsis);
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
