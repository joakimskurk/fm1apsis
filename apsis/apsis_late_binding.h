#ifndef APSIS_LATE_BINDING_H
#define APSIS_LATE_BINDING_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_bloom.h"

/* Locked Study 126 browser lead facts. Native FM-1 deliberately uses the
 * desktop profile; narrow values remain explicit source-parity data. */
#define APSIS_SPIRAL_BIND_LEAD_DESKTOP_MS 80u
#define APSIS_SPIRAL_BIND_LEAD_NARROW_MS 110u
#define APSIS_BLOOM_OCCURRENCE_LEAD_MS 45u
#define APSIS_BLOOM_BIND_LEAD_DESKTOP_MS 50u
#define APSIS_BLOOM_BIND_LEAD_NARROW_MS 70u
#define APSIS_SPIRAL_BIND_LEAD_NATIVE_MS APSIS_SPIRAL_BIND_LEAD_DESKTOP_MS
#define APSIS_BLOOM_BIND_LEAD_NATIVE_MS APSIS_BLOOM_BIND_LEAD_DESKTOP_MS

/* Study 126 bloomGravitySnapshotAt() projects ordinary framePending degree/ring
 * when pending.at <= requested target + 0.5 ms. Mother-held state is not
 * represented here and therefore cannot be projected by this helper. */
typedef struct {
    uint8_t active;
    uint8_t degree;
    uint8_t ring;
    uint64_t target_sample;
} ApsisLatePendingGravity;

typedef struct {
    uint8_t valid;
    uint8_t route_index;
    int16_t step;
    int32_t tuning_deviation_milli;
    uint8_t tuning_mode;
    uint16_t tuning_strength_q15;
} ApsisSpiralLateBinding;

typedef struct {
    uint8_t valid;
    uint8_t voice_index;
    int8_t plan_octave;
    int16_t step;
    int32_t tuning_deviation_milli;
    uint8_t tuning_mode;
    uint16_t tuning_strength_q15;
    ApsisBloomHarmony harmony;
} ApsisBloomLateBinding;

ApsisBloomHarmony apsis_late_bloom_harmony_at(const ApsisState *state,
                                               const ApsisLatePendingGravity *pending,
                                               uint64_t target_sample,
                                               uint32_t sample_rate);

/* Resolve Spiral against current committed state only. Deliberately no pending
 * parameter: Study 126 derivedSequence()/freqForStep() do not project
 * framePending at Spiral bind time. */
uint8_t apsis_late_spiral_resolve(const ApsisState *state,
                                  uint8_t frozen_route_index,
                                  ApsisSpiralLateBinding *out);

/* Resolve one Bloom petal using the occurrence-frozen plan but target-aware
 * Gravity and current tuning/depth state. */
uint8_t apsis_late_bloom_resolve(const ApsisState *state,
                                 const ApsisBloomPlan *frozen_plan,
                                 uint8_t voice_index,
                                 const ApsisLatePendingGravity *pending,
                                 uint64_t voice_target_sample,
                                 uint32_t sample_rate,
                                 ApsisBloomLateBinding *out);

#endif
