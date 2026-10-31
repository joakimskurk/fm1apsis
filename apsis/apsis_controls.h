#ifndef APSIS_CONTROLS_H
#define APSIS_CONTROLS_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_FM1_BUTTON_COUNT 14u
#define APSIS_FM1_ENCODER_COUNT 7u
#define APSIS_FM1_KEY_COUNT 27u
#define APSIS_GRAVITY_DIRECT_KEYS 8u

/* Pinned Dinghy/FM-1 physical ids. The generated target adapter must static-
 * assert these against platform.c before using this contract. */
typedef enum {
    APSIS_FM1_BTN_OCT_DOWN = 0,
    APSIS_FM1_BTN_OCT_UP = 1,
    APSIS_FM1_BTN_FX = 2,
    APSIS_FM1_BTN_SEL = 3,
    APSIS_FM1_BTN_ENV = 4,
    APSIS_FM1_BTN_LFO = 5,
    APSIS_FM1_BTN_EDIT = 6,
    APSIS_FM1_BTN_GLO = 7,
    APSIS_FM1_BTN_HOME = 8,
    APSIS_FM1_BTN_SAVE = 9,
    APSIS_FM1_BTN_ARP = 10,
    APSIS_FM1_BTN_SEQ = 11,
    APSIS_FM1_BTN_PLAY = 12,
    APSIS_FM1_BTN_REC = 13
} ApsisFm1Button;

typedef enum {
    APSIS_FM1_ENC_SELECT = 0,
    APSIS_FM1_ENC_ALGORITHM = 1,
    APSIS_FM1_ENC_K1 = 2,
    APSIS_FM1_ENC_K2 = 3,
    APSIS_FM1_ENC_K3 = 4,
    APSIS_FM1_ENC_K4 = 5,
    APSIS_FM1_ENC_PRESETS = 6
} ApsisFm1Encoder;

/* Canonical UI surface vocabulary. This is navigation state only; it is not a
 * duplicate of APSIS musical state. Keep the order aligned with the frozen P8
 * target surface order so the adapter can migrate without translation tables. */
typedef enum {
    APSIS_SURFACE_MOTHER = 0,
    APSIS_SURFACE_STRING,
    APSIS_SURFACE_DRIFT,
    APSIS_SURFACE_CLOCK,
    APSIS_SURFACE_SPIRAL,
    APSIS_SURFACE_BLOOM,
    APSIS_SURFACE_GRAVITY,
    APSIS_SURFACE_CHAIN,
    APSIS_SURFACE_KEEP,
    APSIS_SURFACE_SPACE,
    APSIS_SURFACE_COUNT
} ApsisControlSurface;

typedef struct {
    uint32_t buttons_pressed;
    uint32_t buttons_released;
    uint32_t buttons_held;
    uint32_t notes_pressed;
    uint32_t notes_held;
    int16_t encoder_steps[APSIS_FM1_ENCODER_COUNT];
} ApsisFm1InputFrame;

typedef enum {
    APSIS_GLOBAL_ACTION_NONE = 0,
    APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE
} ApsisGlobalAction;

/* CLOCK has four physical edit layers. This byte is UI focus only; the actual
 * musical truth remains the masks/bounds in ApsisClockState. */
typedef enum {
    APSIS_CLOCK_CONTROL_MAIN = 0,
    APSIS_CLOCK_CONTROL_BETWEEN,
    APSIS_CLOCK_CONTROL_SNAP,
    APSIS_CLOCK_CONTROL_GROUP,
    APSIS_CLOCK_CONTROL_LAYER_COUNT
} ApsisClockControlLayer;

/* P9A global navigation. HOME is absolute. SPACE is deliberately a chord:
 * an FX press edge is interpreted as SPACE when SEL is held at consumption. */
uint8_t apsis_controls_surface_from_fm1(const ApsisFm1InputFrame *input,
                                        uint8_t current_surface);

/* PLAY alone owns the global transport toggle. SEL+PLAY is reserved for the
 * later CHAIN interaction contract and must not accidentally toggle transport. */
ApsisGlobalAction apsis_controls_global_action(const ApsisFm1InputFrame *input);

/* P9B CLOCK: ALGORITHM detents cycle MAIN/BETWEEN/SNAP/GROUP. The first n
 * physical note-key press edges edit the authoritative Clock masks/bounds.
 * Returns the normalized layer or number of musical cells changed. */
uint8_t apsis_controls_clock_layer_step(uint8_t current_layer, int32_t detents);
uint8_t apsis_controls_clock_apply_keys(ApsisClockState *clock,
                                        uint8_t layer,
                                        uint32_t notes_pressed);

/* P9B GRAVITY: first eight note keys address degrees 0..7. OCT-/OCT+ cycle the
 * selected Anchor/Colour/Air ring. Degree selection is returned as a candidate;
 * the target adapter must pass it through apsis_runtime_request_gravity(). */
uint8_t apsis_controls_gravity_degree_from_fm1(const ApsisFm1InputFrame *input,
                                               uint8_t *degree_out);
uint8_t apsis_controls_gravity_ring_from_fm1(const ApsisFm1InputFrame *input,
                                             uint8_t current_ring);

#endif
