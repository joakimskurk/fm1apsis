#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_bloom_dsp.h"
#include "apsis_bloom_audio.h"
#include "apsis_state.h"

#define HASH_OFFSET 2166136261u
#define HASH_PRIME 16777619u

static uint32_t hash_sample(uint32_t h, int32_t sample)
{
    h ^= (uint16_t)sample;
    return h * HASH_PRIME;
}

static uint32_t render_hash(ApsisBloomDsp *dsp, uint32_t frames, uint32_t *peak)
{
    int32_t out[512];
    uint32_t done = 0u, h = HASH_OFFSET, p = 0u;
    while (done < frames) {
        uint32_t n = frames - done > 256u ? 256u : frames - done;
        uint32_t i;
        memset(out, 0, sizeof(out));
        apsis_bloom_dsp_render(dsp, out, n);
        for (i = 0u; i < n * 2u; ++i) {
            uint32_t a = out[i] < 0 ? (uint32_t)(-out[i]) : (uint32_t)out[i];
            if (a > p) p = a;
            h = hash_sample(h, out[i]);
        }
        done += n;
    }
    if (peak) *peak = p;
    return h;
}

static ApsisBloomAudioBinding make_binding(const ApsisState *s, uint8_t voice,
                                           uint8_t petal, uint64_t target,
                                           const ApsisBloomPetalAcoustic *acoustic)
{
    ApsisBloomAudioBinding b;
    memset(&b, 0, sizeof(b));
    b.valid = 1u;
    b.petal_index = petal;
    b.voice = voice;
    b.token = 1u;
    b.target_sample = target;
    b.attack_ms = s->bloom.attack_ms;
    b.brightness_q15 = s->bloom.brightness_q15;
    b.acoustic = *acoustic;
    b.pitch.valid = 1u;
    b.pitch.voice_index = petal;
    b.pitch.step = 0;
    b.pitch.tuning_mode = APSIS_TUNING_EQUAL;
    b.pitch.tuning_strength_q15 = APSIS_Q15_ONE;
    return b;
}

static void test_voice_vectors(void)
{
    ApsisState s;
    ApsisBloomAcousticPlan plan;
    ApsisBloomDsp dsp;
    ApsisBloomAudioBinding b;
    uint8_t voice;

    apsis_state_default(&s);
    apsis_bloom_audio_plan(&s, 4u, &plan);
    assert(plan.valid == 1u);

    for (voice = APSIS_BLOOM_VEIL; voice <= APSIS_BLOOM_GLASS; ++voice) {
        uint32_t peak, hash;
        apsis_bloom_dsp_reset(&dsp);
        b = make_binding(&s, voice, 0u, 0u, &plan.petal[0]);
        assert(apsis_bloom_dsp_trigger_binding(&dsp, &s, &b, 0u) == 1u);
        assert(dsp.petal[0].active == 1u);
        if (voice == APSIS_BLOOM_VEIL) {
            assert(dsp.petal[0].osc_count == 1u);
            assert(dsp.petal[0].osc[0].waveform == APSIS_BLOOM_DSP_TRIANGLE);
            assert(dsp.petal[0].env_event_sample[0] == 794u);
        } else {
            assert(dsp.petal[0].osc_count == 3u);
        }
        if (voice == APSIS_BLOOM_CHOIR) {
            assert(dsp.petal[0].q_milli == 473u);
            assert(dsp.petal[0].env_event_sample[0] == 1985u);
        } else if (voice == APSIS_BLOOM_ORGAN) {
            assert(dsp.petal[0].q_milli == 420u);
            assert(dsp.petal[0].env_event_sample[0] == 794u);
        } else if (voice == APSIS_BLOOM_GLASS) {
            assert(dsp.petal[0].q_milli == 240u);
            assert(dsp.petal[0].env_event_sample[0] == 572u);
        }
        hash = render_hash(&dsp, 97020u, &peak); /* 2.2 seconds */
        printf("M4g Bloom voice %u peak/hash %u / %08x\n",
               (unsigned)voice, (unsigned)peak, (unsigned)hash);
        assert(apsis_bloom_dsp_active(&dsp) == 0u);
        assert(dsp.drop_count == 0u);
    }

    /* VEIL later petals switch to sine while keeping the one-oscillator bank. */
    apsis_bloom_dsp_reset(&dsp);
    b = make_binding(&s, APSIS_BLOOM_VEIL, 1u, 0u, &plan.petal[1]);
    assert(apsis_bloom_dsp_trigger_binding(&dsp, &s, &b, 0u) == 1u);
    assert(dsp.petal[0].osc_count == 1u);
    assert(dsp.petal[0].osc[0].waveform == APSIS_BLOOM_DSP_SINE);
}

static void test_target_delay_is_exact(void)
{
    ApsisState s;
    ApsisBloomAcousticPlan plan;
    ApsisBloomDsp dsp;
    ApsisBloomAudioBinding b;
    int32_t out[512];
    uint32_t i;

    apsis_state_default(&s);
    apsis_bloom_audio_plan(&s, 4u, &plan);
    apsis_bloom_dsp_reset(&dsp);
    b = make_binding(&s, APSIS_BLOOM_ORGAN, 0u, 1234u, &plan.petal[0]);
    assert(apsis_bloom_dsp_trigger_binding(&dsp, &s, &b, 1000u) == 1u);
    assert(dsp.petal[0].start_delay_samples == 234u);
    memset(out, 0, sizeof(out));
    apsis_bloom_dsp_render(&dsp, out, 256u);
    for (i = 0u; i < 234u * 2u; ++i) assert(out[i] == 0);
    assert(dsp.petal[0].age_samples == 22u);
}

static void test_provisional_worst_case_capacity(void)
{
    ApsisState s;
    ApsisBloomAcousticPlan plan;
    ApsisBloomDsp dsp;
    uint64_t block_start = 0u;
    const uint64_t total_frames = 485100u; /* 11 seconds */
    const uint32_t occurrence_gap = 11025u; /* 180 BPM, 1/16, 4 Bloom points/cycle */
    uint32_t hash = HASH_OFFSET, peak = 0u;
    uint32_t occurrence;

    apsis_state_default(&s);
    s.bloom.length_ms = 6000u;
    s.bloom.unfurl_ms = 3600u;
    s.bloom.width_q15 = APSIS_Q15_ONE;
    s.bloom.level_q15 = (uint16_t)(((uint32_t)APSIS_Q15_ONE * 5u + 2u) / 4u);
    s.bloom.attack_ms = 800u;
    s.bloom.brightness_q15 = APSIS_Q15_ONE;
    apsis_bloom_audio_plan(&s, 5u, &plan);
    assert(plan.valid == 1u && plan.count == 5u);
    apsis_bloom_dsp_reset(&dsp);

    while (block_start < total_frames) {
        uint32_t frames = (uint32_t)(total_frames - block_start > 256u
            ? 256u : total_frames - block_start);
        uint64_t block_end = block_start + frames;
        int32_t out[512];
        uint8_t petal;
        uint32_t i;

        for (occurrence = 0u; occurrence < 64u; ++occurrence) {
            uint64_t base = (uint64_t)occurrence * occurrence_gap;
            if (base >= total_frames) break;
            for (petal = 0u; petal < 5u; ++petal) {
                uint64_t target = base + plan.petal[petal].target_offset_samples;
                ApsisBloomAudioBinding b;
                if (target < block_start || target >= block_end) continue;
                b = make_binding(&s, APSIS_BLOOM_CHOIR, petal, target,
                                 &plan.petal[petal]);
                b.token = occurrence + 1u;
                assert(apsis_bloom_dsp_trigger_binding(&dsp, &s, &b,
                                                       block_start) == 1u);
            }
        }

        memset(out, 0, sizeof(out));
        apsis_bloom_dsp_render(&dsp, out, frames);
        for (i = 0u; i < frames * 2u; ++i) {
            uint32_t a = out[i] < 0 ? (uint32_t)(-out[i]) : (uint32_t)out[i];
            if (a > peak) peak = a;
            hash = hash_sample(hash, out[i]);
        }
        block_start = block_end;
    }

    printf("M4g Bloom stress triggers=%u petals=%u/%u oscs=%u drops=%u peak=%u hash=%08x\n",
           (unsigned)dsp.trigger_count,
           (unsigned)dsp.petal_high_water,
           (unsigned)APSIS_BLOOM_DSP_PETAL_CAP,
           (unsigned)dsp.oscillator_high_water,
           (unsigned)dsp.drop_count,
           (unsigned)peak,
           (unsigned)hash);
    assert(dsp.trigger_count > 150u);
    assert(dsp.petal_high_water < APSIS_BLOOM_DSP_PETAL_CAP);
    assert(dsp.drop_count == 0u);
}

int main(void)
{
    test_voice_vectors();
    test_target_delay_is_exact();
    test_provisional_worst_case_capacity();
    puts("PASS: APSIS M4g Bloom finite renderer and provisional capacity probe");
    return 0;
}
