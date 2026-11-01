#include "apsis_bloom_manual_audio.h"

static void bloom_manual_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint16_t bloom_manual_level(uint16_t level_q15)
{
    const uint32_t max_level = ((uint32_t)APSIS_Q15_ONE * 5u + 2u) / 4u;
    return level_q15 > max_level ? (uint16_t)max_level : level_q15;
}

static uint32_t bloom_manual_ms_samples(uint32_t ms)
{
    return 44u * ms + (100u * ms + 500u) / 1000u;
}

static int16_t bloom_manual_pan(uint16_t width_q15, uint8_t index, uint8_t count)
{
    int32_t num, scaled, mag;
    uint16_t width = width_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : width_q15;
    uint8_t den;
    if (count <= 1u) return 0;
    den = (uint8_t)(count - 1u);
    num = (int32_t)(2u * index) - (int32_t)den;
    scaled = (int32_t)width * num;
    mag = scaled < 0 ? -scaled : scaled;
    mag = (mag + den / 2u) / den;
    return (int16_t)(scaled < 0 ? -mag : mag);
}

static uint32_t bloom_manual_token(ApsisBloomManualAudio *manual)
{
    if (!manual) return 0x80000001u;
    manual->next_token++;
    if ((manual->next_token & 0x80000000u) == 0u)
        manual->next_token = 0x80000001u;
    return manual->next_token;
}

static ApsisBloomHarmony bloom_manual_display_harmony(
    const ApsisState *state,
    const ApsisLatePendingGravity *pending,
    uint64_t now_sample)
{
    uint64_t sample = now_sample;
    if (pending && pending->active) sample = pending->target_sample;
    return apsis_late_bloom_harmony_at(state, pending, sample,
                                       APSIS_BLOOM_AUDIO_RATE);
}

static ApsisBloomPlan bloom_manual_current_plan(
    const ApsisState *state,
    const ApsisBloomVoicingState *voicing,
    const ApsisBloomHarmony *harmony)
{
    uint8_t voices = apsis_bloom_voice_count(state, harmony);
    if (voicing && voicing->plan.count == voices && voices)
        return voicing->plan;
    return apsis_bloom_legacy_plan(state, harmony);
}

static uint8_t bloom_manual_bind(ApsisBloomManualAudio *manual,
                                 ApsisBloomDsp *dsp,
                                 const ApsisState *state,
                                 const ApsisBloomPlan *plan,
                                 const ApsisLatePendingGravity *pending,
                                 uint8_t petal_index,
                                 uint32_t token,
                                 const ApsisBloomPetalAcoustic *acoustic,
                                 uint64_t bind_sample,
                                 uint64_t target_sample,
                                 uint64_t block_start)
{
    ApsisBloomAudioBinding binding;
    if (!manual || !dsp || !state || !plan || !plan->count || !acoustic)
        return 0u;
    if (!state->bloom.sound || !state->bloom.level_q15) return 0u;

    bloom_manual_zero(&binding, (uint32_t)sizeof(binding));
    binding.valid = 1u;
    binding.petal_index = petal_index;
    binding.voice = state->bloom.voice <= APSIS_BLOOM_GLASS
        ? state->bloom.voice : APSIS_BLOOM_VEIL;
    binding.token = token;
    binding.bind_sample = bind_sample;
    binding.target_sample = target_sample;
    binding.attack_ms = state->bloom.attack_ms;
    binding.brightness_q15 = state->bloom.brightness_q15;
    binding.acoustic = *acoustic;
    if (!apsis_late_bloom_resolve(state, plan, petal_index, pending,
                                  target_sample, APSIS_BLOOM_AUDIO_RATE,
                                  &binding.pitch))
        return 0u;
    if (!apsis_bloom_dsp_trigger_binding(dsp, state, &binding, block_start))
        return 0u;
    manual->bind_count++;
    return 1u;
}

void apsis_bloom_manual_audio_reset(ApsisBloomManualAudio *manual)
{
    if (!manual) return;
    bloom_manual_zero(manual, (uint32_t)sizeof(*manual));
    manual->next_token = 0x80000000u;
}

uint8_t apsis_bloom_manual_trigger_petals(ApsisBloomManualAudio *manual,
                                          ApsisBloomDsp *dsp,
                                          const ApsisState *state,
                                          const ApsisBloomVoicingState *voicing,
                                          const ApsisLatePendingGravity *pending,
                                          uint8_t petal_mask,
                                          uint8_t grow_mask,
                                          uint64_t block_start)
{
    ApsisState direct;
    ApsisBloomHarmony harmony;
    ApsisBloomPlan plan;
    uint16_t level;
    uint8_t voices, i, accepted = 0u;

    if (!manual || !dsp || !state || !petal_mask ||
        !state->bloom.sound || !state->bloom.level_q15)
        return 0u;

    /* Study-126 direct petal receives explicit bloomDisplayHarmony(), so a
     * queued future Gravity point is heard immediately. Freeze that display
     * harmony into a local state and resolve with no timing projection. */
    harmony = bloom_manual_display_harmony(state, pending, block_start);
    direct = *state;
    direct.gravity.tonic = harmony.tonic;
    direct.gravity.scale = harmony.scale;
    direct.gravity.degree = harmony.degree;
    direct.gravity.voicing = harmony.voicing;
    direct.gravity.inversion = harmony.inversion;
    direct.gravity.voices = harmony.voices;
    voices = apsis_bloom_voice_count(&direct, &harmony);
    if (voices > APSIS_BLOOM_VOICES_MAX) voices = APSIS_BLOOM_VOICES_MAX;
    plan = bloom_manual_current_plan(&direct, voicing, &harmony);
    level = bloom_manual_level(direct.bloom.level_q15);

    for (i = 0u; i < voices; ++i) {
        ApsisBloomPetalAcoustic acoustic;
        uint32_t duration_ms, scale_milli, amp;
        uint32_t token;
        if ((petal_mask & (uint8_t)(1u << i)) == 0u) continue;

        duration_ms = ((uint32_t)direct.bloom.length_ms * 680u + 500u) / 1000u;
        if (duration_ms < 450u) duration_ms = 450u;
        if (duration_ms > 4500u) duration_ms = 4500u;
        scale_milli = (grow_mask & (uint8_t)(1u << i))
            ? (uint32_t)(780u + 55u * i) : 1040u;
        amp = ((uint32_t)level * 15u * scale_milli + 500000u) / 1000000u;
        if (amp > 65535u) amp = 65535u;

        bloom_manual_zero(&acoustic, (uint32_t)sizeof(acoustic));
        acoustic.duration_samples = bloom_manual_ms_samples(duration_ms);
        acoustic.amp_q15 = (uint16_t)amp;
        acoustic.pan_q15 = bloom_manual_pan(direct.bloom.width_q15, i, voices);
        token = bloom_manual_token(manual);
        if (bloom_manual_bind(manual, dsp, &direct, &plan, 0, i, token,
                              &acoustic, block_start, block_start, block_start))
            accepted++;
        else
            manual->drop_count++;
    }
    manual->direct_count += accepted;
    return accepted;
}

static int8_t bloom_manual_free_core(const ApsisBloomManualAudio *manual)
{
    uint8_t i;
    if (!manual) return -1;
    for (i = 0u; i < APSIS_BLOOM_MANUAL_CORE_SLOTS; ++i)
        if (!manual->core[i].active) return (int8_t)i;
    return -1;
}

static uint8_t bloom_manual_free_pending(const ApsisBloomManualAudio *manual)
{
    uint8_t i, count = 0u;
    if (!manual) return 0u;
    for (i = 0u; i < APSIS_BLOOM_MANUAL_PENDING_MAX; ++i)
        if (!manual->pending[i].active) count++;
    return count;
}

uint8_t apsis_bloom_manual_trigger_core(ApsisBloomManualAudio *manual,
                                        const ApsisState *state,
                                        ApsisBloomVoicingState *voicing,
                                        const ApsisLatePendingGravity *pending,
                                        uint64_t occurrence_sample,
                                        uint32_t walk_random,
                                        uint32_t pick_random)
{
    ApsisBloomHarmony harmony;
    ApsisBloomPlan plan;
    ApsisBloomAcousticPlan acoustic;
    ApsisBloomManualCore *core;
    int8_t core_index;
    uint32_t token;
    uint8_t voices, i, pending_index = 0u;

    if (!manual || !state || !voicing || !state->bloom.sound ||
        !state->bloom.level_q15)
        return 0u;

    harmony = bloom_manual_display_harmony(state, pending, occurrence_sample);
    voices = apsis_bloom_voice_count(state, &harmony);
    if (!voices || voices > APSIS_BLOOM_VOICES_MAX) return 0u;
    core_index = bloom_manual_free_core(manual);
    if (core_index < 0 || bloom_manual_free_pending(manual) < voices) {
        manual->drop_count++;
        return 0u;
    }

    plan = apsis_bloom_choose_voicing(state, &harmony, voicing, 1u,
                                      walk_random, pick_random);
    if (!plan.count) {
        manual->drop_count++;
        return 0u;
    }
    apsis_bloom_audio_plan(state, plan.count, &acoustic);
    if (!acoustic.valid || acoustic.count != plan.count) {
        manual->drop_count++;
        return 0u;
    }

    token = bloom_manual_token(manual);
    core = &manual->core[(uint8_t)core_index];
    bloom_manual_zero(core, (uint32_t)sizeof(*core));
    core->active = 1u;
    core->remaining = acoustic.count;
    core->token = token;
    core->plan = plan;

    for (i = 0u; i < acoustic.count; ++i) {
        ApsisBloomManualPending *p;
        uint64_t target = occurrence_sample + acoustic.petal[i].target_offset_samples;
        while (pending_index < APSIS_BLOOM_MANUAL_PENDING_MAX &&
               manual->pending[pending_index].active)
            pending_index++;
        if (pending_index >= APSIS_BLOOM_MANUAL_PENDING_MAX) {
            manual->drop_count++;
            core->active = 0u;
            core->remaining = 0u;
            return 0u;
        }
        p = &manual->pending[pending_index++];
        bloom_manual_zero(p, (uint32_t)sizeof(*p));
        p->active = 1u;
        p->core_slot = (uint8_t)core_index;
        p->petal_index = i;
        p->token = token;
        p->target_sample = target;
        p->bind_sample = apsis_late_bloom_bind_sample(
            occurrence_sample, target, APSIS_BLOOM_AUDIO_RATE);
        p->acoustic = acoustic.petal[i];
    }

    manual->core_count++;
    return acoustic.count;
}

uint8_t apsis_bloom_manual_process(ApsisBloomManualAudio *manual,
                                   ApsisBloomDsp *dsp,
                                   const ApsisState *state,
                                   const ApsisLatePendingGravity *pending,
                                   uint64_t block_start,
                                   uint32_t frames)
{
    uint64_t block_end = block_start + frames;
    uint8_t i, accepted = 0u;
    if (!manual || !dsp || !state || !frames) return 0u;

    for (i = 0u; i < APSIS_BLOOM_MANUAL_PENDING_MAX; ++i) {
        ApsisBloomManualPending *p = &manual->pending[i];
        ApsisBloomManualCore *core;
        if (!p->active || p->bind_sample >= block_end) continue;
        if (p->core_slot >= APSIS_BLOOM_MANUAL_CORE_SLOTS) {
            p->active = 0u;
            manual->drop_count++;
            continue;
        }
        core = &manual->core[p->core_slot];
        if (core->active && core->token == p->token) {
            if (bloom_manual_bind(manual, dsp, state, &core->plan, pending,
                                  p->petal_index, p->token, &p->acoustic,
                                  p->bind_sample, p->target_sample, block_start))
                accepted++;
            else if (state->bloom.sound && state->bloom.level_q15)
                manual->drop_count++;
            if (core->remaining) core->remaining--;
            if (!core->remaining) core->active = 0u;
        } else {
            manual->drop_count++;
        }
        p->active = 0u;
    }
    return accepted;
}

uint8_t apsis_bloom_manual_pending_count(const ApsisBloomManualAudio *manual)
{
    uint8_t i, count = 0u;
    if (!manual) return 0u;
    for (i = 0u; i < APSIS_BLOOM_MANUAL_PENDING_MAX; ++i)
        if (manual->pending[i].active) count++;
    return count;
}
