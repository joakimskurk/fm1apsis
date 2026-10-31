#ifndef APSIS_CONTROLS_H
#define APSIS_CONTROLS_H

#include <stdint.h>
#include "apsis_string.h"

#define APSIS_FM1_BUTTON_COUNT 14u
#define APSIS_FM1_ENCODER_COUNT 7u
#define APSIS_FM1_KEY_COUNT 27u
#define APSIS_GRAVITY_DIRECT_KEYS 8u
#define APSIS_STRING_KEYS_PER_BANK 9u
#define APSIS_STRING_VISIBLE_BANKS 3u
#define APSIS_STRING_BANK_BASE_MAX 1u
#define APSIS_SPIRAL_BANK_COUNT 2u
#define APSIS_SPIRAL_UPPER_BANK_OFFSET 12u
#define APSIS_SPIRAL_UPPER_BANK_KEYS 24u

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

typedef enum {
    APSIS_CLOCK_CONTROL_MAIN = 0,
    APSIS_CLOCK_CONTROL_BETWEEN,
    APSIS_CLOCK_CONTROL_SNAP,
    APSIS_CLOCK_CONTROL_GROUP,
    APSIS_CLOCK_CONTROL_LAYER_COUNT
} ApsisClockControlLayer;

typedef enum {
    APSIS_CONTROL_PARAM_NONE = 0,
    APSIS_CONTROL_PARAM_STRING_RESONANCE,
    APSIS_CONTROL_PARAM_STRING_BRIGHTNESS,
    APSIS_CONTROL_PARAM_STRING_GLIDE,
    APSIS_CONTROL_PARAM_STRING_SPAN
} ApsisControlParam;

typedef struct {
    uint8_t param;
    int32_t value;
} ApsisControlParamEdit;

uint8_t apsis_controls_surface_from_fm1(const ApsisFm1InputFrame *input,
                                        uint8_t current_surface);
ApsisGlobalAction apsis_controls_global_action(const ApsisFm1InputFrame *input);

uint8_t apsis_controls_clock_layer_step(uint8_t current_layer, int32_t detents);
uint8_t apsis_controls_clock_apply_keys(ApsisClockState *clock,
                                        uint8_t layer,
                                        uint32_t notes_pressed);

uint8_t apsis_controls_gravity_degree_from_fm1(const ApsisFm1InputFrame *input,
                                               uint8_t *degree_out);
uint8_t apsis_controls_gravity_ring_from_fm1(const ApsisFm1InputFrame *input,
                                             uint8_t current_ring);

/* P9C STRING input remains an expression of the existing String coordinate.
 * The 27 FM-1 keys expose three copies of the source browser's 9-key String
 * keyboard. bank_base=0 exposes source octave banks 0/1/2; bank_base=1 exposes
 * 1/2/3. The split returned here is then applied with the existing
 * apsis_string_set_side_split(..., mirror=1) path. */
uint8_t apsis_controls_string_side_from_fm1(const ApsisStringState *string,
                                            const ApsisFm1InputFrame *input);
uint8_t apsis_controls_string_key_from_fm1(const ApsisFm1InputFrame *input,
                                           uint8_t *key_out);
uint8_t apsis_controls_string_bank_from_fm1(const ApsisFm1InputFrame *input,
                                            uint8_t current_base);
uint16_t apsis_controls_string_split_for_key(const ApsisState *state,
                                             uint8_t side,
                                             uint8_t key_index,
                                             uint8_t bank_base);
uint16_t apsis_controls_string_split_nudge(uint8_t side,
                                           uint16_t current_split,
                                           int32_t detents);
uint8_t apsis_controls_string_mode_step(uint8_t current_mode, int32_t detents);

/* Source-bounded K1..K4 edits. Values are native state units (Q15 for all four
 * current String roles). This function never mutates ApsisState: the target
 * adapter must either apply the edit or queue it through MOTHER ->1. */
uint8_t apsis_controls_string_param_edits(const ApsisStringState *string,
                                          const ApsisFm1InputFrame *input,
                                          ApsisControlParamEdit out[4]);

/* P9D SPIRAL keeps the Study-126 route as ordered raw point ids 0..35. Bank 0
 * exposes points 0..26. Bank 1 exposes points 12..35 on keys 0..23; keys
 * 24..26 are intentionally unused. Route mutators operate on route indices,
 * so duplicates remain legal and ordinary edits never reorder other nodes. */
uint8_t apsis_controls_spiral_bank_from_fm1(const ApsisFm1InputFrame *input,
                                            uint8_t current_bank);
uint8_t apsis_controls_spiral_point_from_fm1(const ApsisFm1InputFrame *input,
                                             uint8_t bank,
                                             uint8_t *point_out);
uint8_t apsis_controls_spiral_cursor_step(uint8_t current_cursor,
                                          uint8_t route_count,
                                          int32_t detents);
uint8_t apsis_controls_spiral_append(ApsisSpiralState *spiral,
                                     uint8_t point,
                                     uint8_t *cursor_io);
uint8_t apsis_controls_spiral_insert(ApsisSpiralState *spiral,
                                     uint8_t point,
                                     uint8_t *cursor_io);
uint8_t apsis_controls_spiral_move_selected(ApsisSpiralState *spiral,
                                            uint8_t cursor,
                                            int32_t detents);
uint8_t apsis_controls_spiral_delete_selected(ApsisSpiralState *spiral,
                                              uint8_t *cursor_io);

#endif
