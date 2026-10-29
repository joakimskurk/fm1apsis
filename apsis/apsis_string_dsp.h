#ifndef APSIS_STRING_DSP_H
#define APSIS_STRING_DSP_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_string.h"

#define APSIS_STRING_DSP_RATE 44100u
#define APSIS_STRING_DSP_VOICES 4u
#define APSIS_STRING_TRANSIENT_LOGICAL_CAP 80u
#define APSIS_STRING_TRANSIENT_TAIL_RESERVE 32u
#define APSIS_STRING_TRANSIENT_POOL_CAP (APSIS_STRING_TRANSIENT_LOGICAL_CAP + APSIS_STRING_TRANSIENT_TAIL_RESERVE)
#define APSIS_STRING_TRANSIENT_PARTIALS_MAX 8u

typedef enum {
    APSIS_STRING_DSP_FOUNDATION_LO = 0,
    APSIS_STRING_DSP_FOUNDATION_HI,
    APSIS_STRING_DSP_PRIMARY_A,
    APSIS_STRING_DSP_PRIMARY_B
} ApsisStringDspVoiceId;

typedef enum {
    APSIS_STRING_DSP_WAVE_SINE = 0,
    APSIS_STRING_DSP_WAVE_TRIANGLE,
    APSIS_STRING_DSP_WAVE_BODY
} ApsisStringDspWaveform;

typedef struct {
    uint32_t freq_millihz[APSIS_STRING_DSP_VOICES];
    uint16_t gain_q15[APSIS_STRING_DSP_VOICES];
    uint16_t mix_left_q15[APSIS_STRING_DSP_VOICES];
    uint16_t mix_right_q15[APSIS_STRING_DSP_VOICES];
    uint16_t lowpass_alpha_q15[APSIS_STRING_DSP_VOICES];
    uint8_t waveform[APSIS_STRING_DSP_VOICES];
    uint8_t enabled[APSIS_STRING_DSP_VOICES];
} ApsisStringDspTargets;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t target_phase_inc;
    int32_t gain_q15;
    int32_t target_gain_q15;
    int32_t desired_gain_q15;
    int32_t lowpass_q15;
    uint16_t pitch_alpha_q15;
    uint16_t lowpass_alpha_q15;
    uint16_t desired_lowpass_alpha_q15;
    uint16_t mix_left_q15;
    uint16_t mix_right_q15;
    uint16_t desired_mix_left_q15;
    uint16_t desired_mix_right_q15;
    uint8_t waveform;
    uint8_t desired_waveform;
    uint8_t pitch_initialized;
    uint8_t timbre_transition;
} ApsisStringDspVoice;

/* Keep this struct byte-for-byte compatible with merged M4a. Finite M4b
 * voices live in ApsisStringTransientPool so adding a large voice reserve does
 * not enlarge sustain-field offsets or relocate the M3c runtime state. */
typedef struct {
    ApsisStringDspVoice voice[APSIS_STRING_DSP_VOICES];
    uint8_t transport_running;
} ApsisStringDsp;

typedef struct {
    uint32_t freq_millihz;
    uint32_t start_offset_samples;
    uint32_t duration_samples;
    uint16_t peak_gain_q15;
    uint16_t mix_left_q15;
    uint16_t mix_right_q15;
    uint16_t lowpass_alpha_q15;
    uint8_t waveform;
    uint8_t root_shadow;
} ApsisStringTransientSpec;

typedef struct {
    ApsisStringTransientSpec partial[APSIS_STRING_TRANSIENT_PARTIALS_MAX];
    uint8_t count;
} ApsisStringTransientPlan;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t start_delay_samples;
    uint32_t age_samples;
    uint32_t duration_samples;
    uint32_t decay_step_q16;
    uint32_t env_phase_q16;
    uint32_t serial;
    int32_t lowpass_q15;
    int32_t steal_gain_q15;
    uint16_t peak_gain_q15;
    uint16_t mix_left_q15;
    uint16_t mix_right_q15;
    uint16_t lowpass_alpha_q15;
    uint16_t attack_left_samples;
    uint16_t steal_left_samples;
    uint8_t waveform;
    uint8_t active;
    uint8_t stealing;
    uint8_t envelope_stage;
} ApsisStringTransientVoice;

typedef struct {
    ApsisStringTransientVoice transient[APSIS_STRING_TRANSIENT_POOL_CAP];
    uint32_t transient_next_serial;
    uint32_t transient_trigger_count;
    uint32_t transient_steal_count;
    uint32_t transient_drop_count;
    uint16_t transient_logical_high_water;
    uint16_t transient_physical_high_water;
} ApsisStringTransientPool;

/* Source-resolved persistent acoustic targets:
 * - Study-98 Foundation in both modes;
 * - Study-98 Classic sine/Drift pair in Classic;
 * - Study-126 BODY/WIRE pair in Dual, which did not exist in Study 98. */
void apsis_string_dsp_targets(const ApsisState *state,
                              ApsisStringPitchMemory *pitch_memory,
                              uint8_t transport_running,
                              ApsisStringDspTargets *out);

/* One shared state/tuning -> frequency conversion for persistent + transient
 * String code. Public only so M4b can use the exact M4a conversion path. */
uint32_t apsis_string_dsp_frequency_millihz(const ApsisState *state,
                                             int32_t step_milli);

void apsis_string_dsp_transient_plan(const ApsisState *state,
                                     uint32_t base_freq_millihz,
                                     uint16_t level_q15,
                                     uint32_t duration_ms,
                                     int16_t pan_q15,
                                     uint8_t pizz,
                                     uint16_t root_gain_q15,
                                     ApsisStringTransientPlan *out);

void apsis_string_transient_pool_reset(ApsisStringTransientPool *pool);

uint8_t apsis_string_dsp_trigger_classic(ApsisStringTransientPool *pool,
                                         const ApsisState *state,
                                         uint32_t base_freq_millihz,
                                         uint16_t level_q15,
                                         uint32_t duration_ms,
                                         int16_t pan_q15,
                                         uint8_t pizz,
                                         uint16_t root_gain_q15);

uint16_t apsis_string_dsp_transient_logical_active(const ApsisStringTransientPool *pool);
uint16_t apsis_string_dsp_transient_physical_active(const ApsisStringTransientPool *pool);

void apsis_string_dsp_reset(ApsisStringDsp *dsp);
void apsis_string_dsp_update(ApsisStringDsp *dsp,
                             const ApsisState *state,
                             ApsisStringPitchMemory *pitch_memory,
                             uint8_t transport_running);

/* Frozen M4a sustain renderer. */
void apsis_string_dsp_render(ApsisStringDsp *dsp, int32_t *out, uint32_t frames);

/* M4b1 finite transient renderer. It is intentionally separate until M4b2
 * connects real transport events. */
void apsis_string_transient_render(ApsisStringTransientPool *pool, int32_t *out,
                                   uint32_t frames);

#endif
