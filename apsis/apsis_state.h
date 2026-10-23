#ifndef APSIS_STATE_H
#define APSIS_STATE_H

#include <stdint.h>

#define APSIS_STATE_VERSION 1u
#define APSIS_SPIRAL_POINTS 36u
#define APSIS_SPIRAL_ROUTE_MAX 30u
#define APSIS_ATTRACTOR_STEPS 16u
#define APSIS_CLOCK_STEPS_MAX 24u
#define APSIS_KEEP_SLOTS 8u
#define APSIS_CHAIN_MAX 32u
#define APSIS_Q15_ONE 32767u
#define APSIS_Q15_MILLI(x) ((uint16_t)((((uint32_t)(x)) * 32767u + 500u) / 1000u))

typedef enum {
    APSIS_SCALE_MAJOR = 0,
    APSIS_SCALE_NATURAL_MINOR,
    APSIS_SCALE_HARMONIC_MINOR,
    APSIS_SCALE_MELODIC_MINOR,
    APSIS_SCALE_DORIAN,
    APSIS_SCALE_PHRYGIAN,
    APSIS_SCALE_LYDIAN,
    APSIS_SCALE_MIXOLYDIAN,
    APSIS_SCALE_COUNT
} ApsisScaleId;

typedef enum {
    APSIS_TUNING_EQUAL = 0,
    APSIS_TUNING_JUST,
    APSIS_TUNING_PYTH
} ApsisTuningMode;

typedef enum { APSIS_STRING_CLASSIC = 0, APSIS_STRING_DUAL } ApsisStringSurfaceMode;
typedef enum { APSIS_SIDE_LEFT = 0, APSIS_SIDE_RIGHT } ApsisSide;
typedef enum { APSIS_ANCHOR_ROOT = 0, APSIS_ANCHOR_FIFTH } ApsisStringAnchor;
typedef enum { APSIS_HARMONY_SCALE = 0, APSIS_HARMONY_CHORD, APSIS_HARMONY_CHROMATIC } ApsisStringHarmony;
typedef enum { APSIS_STRING_CHARACTER_CLASSIC = 0 } ApsisStringCharacter;
typedef enum { APSIS_STRING_RHYTHM_OFF = 0 } ApsisStringRhythm;
typedef enum { APSIS_STRING_FILTER_OFF = 0 } ApsisStringFilterMode;
typedef enum { APSIS_CLOCK_STONE = 0 } ApsisClockVoice;
typedef enum { APSIS_SPIRAL_THREAD = 0, APSIS_SPIRAL_GLASS, APSIS_SPIRAL_REED, APSIS_SPIRAL_HALO } ApsisSpiralVoice;
typedef enum { APSIS_BLOOM_VEIL = 0, APSIS_BLOOM_CHOIR, APSIS_BLOOM_ORGAN, APSIS_BLOOM_GLASS } ApsisBloomVoice;
typedef enum { APSIS_GRAVITY_ANCHOR = 0, APSIS_GRAVITY_COLOUR, APSIS_GRAVITY_AIR } ApsisGravityRing;
typedef enum { APSIS_CHANGE_PULSE = 0 } ApsisChangeTiming;
typedef enum { APSIS_CHAIN_SYNC_ONE = 0, APSIS_CHAIN_SYNC_PULSE } ApsisChainSync;

typedef struct {
    uint16_t split_q15;
    uint16_t left_split_q15;
    uint16_t right_split_q15;
    uint8_t surface_mode;
    uint8_t linked;
    uint8_t active_side;
    uint8_t left_anchor;
    uint8_t right_anchor;
    uint8_t left_follow_gravity;
    uint8_t right_follow_gravity;
    int8_t left_gravity_degree;
    int8_t right_gravity_degree;
    uint16_t left_level_q15;
    uint16_t right_level_q15;
    uint16_t resonance_q15;
    uint16_t brightness_q15;
    uint8_t quantize;
    uint8_t harmony;
    uint16_t span_q15;
    uint8_t range;
    uint8_t character;
    uint8_t foundation;
    uint16_t foundation_level_q15;
    int8_t foundation_octave;
    uint8_t rhythm;
    uint8_t rhythm_every;
    uint8_t rhythm_steps[4];
    uint16_t rhythm_level_q15;
    uint8_t filter_mode;
    uint16_t filter_cutoff_hz;
    uint16_t filter_depth_q15;
    uint16_t filter_q_q15;
    uint16_t filter_decay_q15;
    uint16_t filter_slew_q15;
    uint8_t filter_steps[4];
    uint8_t filter_attractor_every;
    uint8_t filter_attractor_length;
    uint16_t filter_attractor_lock_q15;
    uint16_t filter_attractor_strum_q15;
    uint16_t filter_attractor_strum_level_q15;
    uint16_t filter_attractor_gates;
    uint16_t filter_attractor_seed;
    uint8_t motion_loop_length;
    uint8_t motion_loop_play;
    uint8_t motion_loop_pitch_continuous;
    uint16_t motion_level_q15;
    uint16_t glide_q15;
    uint16_t send_q15;
    uint8_t sound;
    uint16_t level_q15;
} ApsisStringState;

typedef struct {
    uint16_t amount_q15;
    uint16_t depth_q15;
    uint16_t width_q15;
    uint16_t send_q15;
    uint8_t sound;
    uint16_t level_q15;
} ApsisDriftState;

typedef struct {
    uint8_t n;
    uint8_t bounds[3];
    uint32_t main_mask;
    uint32_t inner_mask;
    uint32_t snap_mask;
    uint16_t bpm;
    uint8_t division_num;
    uint8_t division_den;
    uint16_t swing_q15;
    uint16_t ghost_q15;
    uint8_t step_one_resync;
    uint16_t send_q15;
    uint8_t sound;
    uint16_t level_q15;
    uint8_t voice;
    int8_t pitch;
    uint16_t inner_level_q15;
    uint16_t snap_level_q15;
    uint16_t snap_tone_hz;
} ApsisClockState;

typedef struct {
    uint8_t route[APSIS_SPIRAL_ROUTE_MAX];
    uint8_t route_count;
    uint8_t rate;
    uint16_t note_length_q15;
    int8_t register_oct;
    uint8_t voice;
    uint8_t follow_frame;
    uint8_t attractor_rate;
    uint8_t attractor_length;
    uint16_t attractor_lock_q15;
    uint16_t attractor_density_q15;
    uint16_t attractor_seed;
    uint16_t attractor_gates;
    uint16_t attractor_energy_q15[APSIS_ATTRACTOR_STEPS];
    uint16_t send_q15;
    uint8_t sound;
    uint16_t level_q15;
} ApsisSpiralState;

typedef struct {
    uint8_t cycles;
    uint8_t rate;
    uint8_t trigger_step;
    uint16_t delay_ms;
    uint8_t voice;
    int8_t register_oct;
    uint16_t voicing_spread_q15;
    uint16_t length_ms;
    uint16_t attack_ms;
    uint16_t unfurl_q15;
    uint16_t spread_q15;
    uint16_t width_q15;
    uint16_t brightness_q15;
    uint8_t tuning_follow;
    uint8_t tuning_reference_chord;
    uint16_t tuning_amount_q15;
    uint16_t send_q15;
    uint8_t sound;
    uint16_t level_q15;
} ApsisBloomState;

typedef struct {
    uint8_t mode;
    uint16_t strength_q15;
} ApsisTemperState;

typedef struct {
    uint8_t tonic;
    uint8_t scale;
    uint8_t degree;
    uint8_t voicing;
    uint8_t voices;
    uint8_t inversion;
    uint8_t scale_lock;
    uint8_t change_timing;
    uint8_t change_grid;
    uint8_t ring_play;
    uint16_t ring_level_q15;
    uint8_t selected_ring;
    uint8_t point_override_count;
} ApsisGravityState;

typedef struct {
    uint16_t mix_q15;
    uint16_t decay_ms;
    uint16_t predelay_ms;
    uint16_t damping_hz;
    uint16_t echo_time_ms;
    uint16_t echo_feedback_q15;
} ApsisSpaceState;

typedef struct {
    uint8_t sync;
    uint8_t horizon;
    uint16_t mix_q15[5];
    uint8_t mute_mask;
} ApsisMotherState;

typedef struct {
    uint8_t meter_lock;
    uint8_t start_sync;
    uint8_t loop_sync;
    uint8_t default_cycles;
    uint8_t default_fine_pulses;
    uint8_t default_morph_quarters;
    uint8_t scope_key_scale;
    uint8_t scope_chord;
    uint8_t scope_tension;
    uint8_t scope_drift;
    uint8_t scope_clock;
    uint8_t scope_trace;
    uint8_t scope_tuning;
} ApsisChainState;

typedef struct {
    uint32_t version;
    ApsisStringState string;
    ApsisDriftState drift;
    ApsisClockState clock;
    ApsisSpiralState spiral;
    ApsisBloomState bloom;
    ApsisTemperState temper;
    ApsisGravityState gravity;
    ApsisSpaceState space;
    ApsisMotherState mother;
    ApsisChainState chain;
} ApsisState;

void apsis_state_default(ApsisState *state);

#endif
