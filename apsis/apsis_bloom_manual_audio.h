#ifndef APSIS_BLOOM_MANUAL_AUDIO_H
#define APSIS_BLOOM_MANUAL_AUDIO_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_bloom.h"
#include "apsis_bloom_audio.h"
#include "apsis_bloom_audio_runtime.h"
#include "apsis_bloom_dsp.h"
#include "apsis_late_binding.h"

#define APSIS_BLOOM_MANUAL_CORE_SLOTS 4u
#define APSIS_BLOOM_MANUAL_PENDING_MAX \
    (APSIS_BLOOM_MANUAL_CORE_SLOTS * APSIS_BLOOM_VOICES_MAX)

typedef struct {
    uint8_t active;
    uint8_t remaining;
    uint16_t reserved;
    uint32_t token;
    ApsisBloomPlan plan;
} ApsisBloomManualCore;

typedef struct {
    uint8_t active;
    uint8_t core_slot;
    uint8_t petal_index;
    uint8_t reserved;
    uint32_t token;
    uint64_t bind_sample;
    uint64_t target_sample;
    ApsisBloomPetalAcoustic acoustic;
} ApsisBloomManualPending;

typedef struct {
    ApsisBloomManualCore core[APSIS_BLOOM_MANUAL_CORE_SLOTS];
    ApsisBloomManualPending pending[APSIS_BLOOM_MANUAL_PENDING_MAX];
    uint32_t next_token;
    uint32_t direct_count;
    uint32_t core_count;
    uint32_t bind_count;
    uint32_t drop_count;
} ApsisBloomManualAudio;

void apsis_bloom_manual_audio_reset(ApsisBloomManualAudio *manual);

/* Direct petal and grow hits sound immediately from the display harmony. The
 * two masks are kept distinct because Study 126 uses 1.04 for a direct petal
 * and .78 + .055*i for a grow-fired petal. */
uint8_t apsis_bloom_manual_trigger_petals(ApsisBloomManualAudio *manual,
                                          ApsisBloomDsp *dsp,
                                          const ApsisState *state,
                                          const ApsisBloomVoicingState *voicing,
                                          const ApsisLatePendingGravity *pending,
                                          uint8_t petal_mask,
                                          uint8_t grow_mask,
                                          uint64_t block_start);

/* CORE freezes one newly advanced voice-leading plan and source acoustic
 * geometry, but keeps each future petal late-bound. No M3c transport event and
 * no private musical clock are created; this finite queue only owns the manual
 * gesture's absolute sample targets. */
uint8_t apsis_bloom_manual_trigger_core(ApsisBloomManualAudio *manual,
                                        const ApsisState *state,
                                        ApsisBloomVoicingState *voicing,
                                        const ApsisLatePendingGravity *pending,
                                        uint64_t occurrence_sample,
                                        uint32_t walk_random,
                                        uint32_t pick_random);

/* Called once per audio callback before Bloom DSP render. It resolves current
 * Gravity/tuning/voice/attack/brightness only when each manual core petal has
 * reached the normal native Bloom bind lead. */
uint8_t apsis_bloom_manual_process(ApsisBloomManualAudio *manual,
                                   ApsisBloomDsp *dsp,
                                   const ApsisState *state,
                                   const ApsisLatePendingGravity *pending,
                                   uint64_t block_start,
                                   uint32_t frames);

uint8_t apsis_bloom_manual_pending_count(const ApsisBloomManualAudio *manual);

#endif
