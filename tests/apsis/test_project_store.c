#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_project_store.h"
#include "apsis_state.h"
#include "apsis_keep.h"
#include "apsis_chain.h"

static void write_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void fill_fixture(ApsisKeepBank *bank, ApsisChainSequence *seq,
                         ApsisChainState *settings)
{
    ApsisState s;
    uint8_t i;
    apsis_keep_bank_reset(bank);
    for (i = 0u; i < APSIS_KEEP_SLOTS; ++i) {
        apsis_state_default(&s);
        s.string.left_split_q15 = (uint16_t)(6000u + 500u * i);
        s.string.right_split_q15 = (uint16_t)(19000u + 500u * i);
        s.string.surface_mode = (uint8_t)(i & 1u);
        s.string.linked = (uint8_t)((i + 1u) & 1u);
        s.string.left_anchor = (uint8_t)(i % 5u);
        s.string.right_anchor = (uint8_t)((i + 2u) % 5u);
        s.string.left_follow_gravity = (uint8_t)(i & 1u);
        s.string.right_follow_gravity = (uint8_t)((i + 1u) & 1u);
        s.string.left_gravity_degree = (int8_t)(i % 8u);
        s.string.right_gravity_degree = (int8_t)((i + 3u) % 8u);
        s.drift.amount_q15 = (uint16_t)(1000u + 1000u * i);
        s.clock.n = (uint8_t)(12u + (i % 4u));
        s.clock.bounds[0] = 3u;
        s.clock.bounds[1] = 6u;
        s.clock.bounds[2] = 9u;
        s.clock.main_mask = 0x00000FFFu ^ ((uint32_t)i << 4);
        s.clock.inner_mask = 0x00000249u ^ ((uint32_t)i << 5);
        s.clock.snap_mask = 0x00000810u ^ ((uint32_t)i << 6);
        s.spiral.route_count = 4u;
        s.spiral.route[0] = i;
        s.spiral.route[1] = (uint8_t)(8u + i);
        s.spiral.route[2] = (uint8_t)(16u + i);
        s.spiral.route[3] = (uint8_t)(24u + i);
        s.temper.mode = (uint8_t)(i % 3u);
        s.gravity.tonic = (uint8_t)((2u * i) % 12u);
        s.gravity.scale = (uint8_t)(i % APSIS_SCALE_COUNT);
        s.gravity.degree = (uint8_t)(i % 8u);
        s.gravity.voicing = (uint8_t)(i % 3u);
        s.gravity.voices = (uint8_t)(3u + (i % 3u));
        s.gravity.inversion = (uint8_t)(i % s.gravity.voices);
        bank->slots[i] = apsis_keep_capture(&s);
    }
    bank->selected_slot = 5;

    apsis_state_default(&s);
    *settings = s.chain;
    settings->meter_lock = 1u;
    settings->start_sync = APSIS_CHAIN_SYNC_PULSE;
    settings->loop_sync = APSIS_CHAIN_SYNC_ONE;
    settings->default_cycles = 3u;
    settings->default_fine_pulses = 2u;
    settings->default_morph_quarters = APSIS_CHAIN_MORPH_HALF;
    settings->scope_key_scale = 1u;
    settings->scope_chord = 1u;
    settings->scope_tension = 1u;
    settings->scope_drift = 1u;
    settings->scope_clock = 1u;
    settings->scope_trace = 1u;
    settings->scope_tuning = 1u;

    apsis_chain_sequence_reset(seq);
    seq->count = APSIS_CHAIN_MAX;
    seq->selected = 17;
    for (i = 0u; i < APSIS_CHAIN_MAX; ++i) {
        static const uint8_t morph[4] = {
            APSIS_CHAIN_MORPH_NONE, APSIS_CHAIN_MORPH_QUARTER,
            APSIS_CHAIN_MORPH_HALF, APSIS_CHAIN_MORPH_FULL
        };
        seq->cells[i] = apsis_chain_default_cell((uint8_t)(i % 8u), settings, 12u);
        seq->cells[i].cycles = (uint8_t)(1u + (i % 32u));
        seq->cells[i].fine_pulses = (uint8_t)(i % 12u);
        seq->cells[i].chance_q15 = (uint16_t)(APSIS_Q15_ONE - (uint16_t)(i * 257u));
        seq->cells[i].transpose = (int8_t)((int32_t)(i % 25u) - 12);
        seq->cells[i].morph_quarters = morph[i & 3u];
    }

    /* These are runtime-only and must never survive a project load. */
    seq->playing = 1u;
    seq->armed = 1u;
    seq->pos = 9u;
    seq->pulse = 1234u;
    seq->phrase_pulse = 5678u;
    seq->master_pulses = 9999u;
    seq->hold_tail = 1u;
    seq->last_applied = 0u;
}

static void test_crc_reference(void)
{
    static const uint8_t text[] = "123456789";
    assert(apsis_project_crc32(text, 9u) == 0xCBF43926u);
}

static void test_payload_roundtrip(void)
{
    ApsisKeepBank a_keep, b_keep;
    ApsisChainSequence a_chain, b_chain;
    ApsisChainState a_settings, b_settings;
    uint8_t a[APSIS_PROJECT_PAYLOAD_SIZE];
    uint8_t b[APSIS_PROJECT_PAYLOAD_SIZE];

    fill_fixture(&a_keep, &a_chain, &a_settings);
    assert(apsis_project_payload_encode(a, sizeof a, &a_keep, &a_chain, &a_settings) == 0);
    assert(apsis_project_payload_decode(a, sizeof a, &b_keep, &b_chain, &b_settings) == 0);
    assert(b_keep.selected_slot == 5);
    assert(b_chain.count == APSIS_CHAIN_MAX);
    assert(b_chain.selected == 17);
    assert(b_chain.playing == 0u);
    assert(b_chain.armed == 0u);
    assert(b_chain.pos == 0u);
    assert(b_chain.pulse == 0u);
    assert(b_chain.phrase_pulse == 0u);
    assert(b_chain.master_pulses == 0u);
    assert(b_chain.hold_tail == 0u);
    assert(b_chain.last_applied == 1u);
    assert(apsis_project_payload_encode(b, sizeof b, &b_keep, &b_chain, &b_settings) == 0);
    assert(memcmp(a, b, sizeof a) == 0);
}

static void test_record_crc_and_version_gates(void)
{
    ApsisKeepBank keep;
    ApsisChainSequence chain;
    ApsisChainState settings;
    ApsisProjectRecordInfo info;
    uint8_t record[APSIS_PROJECT_RECORD_SIZE];
    uint8_t bad[APSIS_PROJECT_RECORD_SIZE];
    uint32_t header_crc;

    fill_fixture(&keep, &chain, &settings);
    assert(APSIS_PROJECT_PAYLOAD_SIZE == 816u);
    assert(APSIS_PROJECT_RECORD_SIZE == 840u);
    assert(apsis_project_record_build(record, sizeof record, 41u,
                                      &keep, &chain, &settings) == 0);
    assert(record[0] == 'A' && record[1] == 'P' && record[2] == 'S' && record[3] == '7');
    assert(apsis_project_record_inspect(record, sizeof record, &info) == 0);
    assert(info.valid == 1u && info.generation == 41u);

    memcpy(bad, record, sizeof bad);
    bad[APSIS_PROJECT_PAYLOAD_OFFSET + 37u] ^= 0x20u;
    assert(apsis_project_record_inspect(bad, sizeof bad, &info) != 0);

    memcpy(bad, record, sizeof bad);
    bad[16] ^= 1u;
    assert(apsis_project_record_inspect(bad, sizeof bad, &info) != 0);

    /* Version must fail even if the header CRC is recomputed correctly. */
    memcpy(bad, record, sizeof bad);
    bad[4] = 2u;
    bad[5] = 0u;
    header_crc = apsis_project_crc32(bad, 20u);
    write_u32(bad + 20u, header_crc);
    assert(apsis_project_record_inspect(bad, sizeof bad, &info) != 0);
}

static void test_record_decode_resets_runtime(void)
{
    ApsisKeepBank keep, out_keep;
    ApsisChainSequence chain, out_chain;
    ApsisChainState settings, out_settings;
    uint8_t record[APSIS_PROJECT_RECORD_SIZE];
    uint32_t generation = 0u;

    fill_fixture(&keep, &chain, &settings);
    assert(apsis_project_record_build(record, sizeof record, 77u,
                                      &keep, &chain, &settings) == 0);
    memset(&out_chain, 0xA5, sizeof out_chain);
    assert(apsis_project_record_decode(record, sizeof record,
                                       &out_keep, &out_chain, &out_settings,
                                       &generation) == 0);
    assert(generation == 77u);
    assert(out_chain.playing == 0u && out_chain.armed == 0u);
    assert(out_chain.last_applied == 1u);
}

static void test_two_slot_power_loss_recovery(void)
{
    ApsisKeepBank keep;
    ApsisChainSequence chain;
    ApsisChainState settings;
    uint8_t rec_a[APSIS_PROJECT_RECORD_SIZE];
    uint8_t rec_b[APSIS_PROJECT_RECORD_SIZE];
    uint8_t slot_a[APSIS_PROJECT_SECTOR_SIZE];
    uint8_t slot_b[APSIS_PROJECT_SECTOR_SIZE];
    uint32_t generation = 0u;
    int8_t pick;

    fill_fixture(&keep, &chain, &settings);
    assert(apsis_project_record_build(rec_a, sizeof rec_a, 5u,
                                      &keep, &chain, &settings) == 0);
    assert(apsis_project_record_build(rec_b, sizeof rec_b, 6u,
                                      &keep, &chain, &settings) == 0);
    memset(slot_a, 0xFF, sizeof slot_a);
    memset(slot_b, 0xFF, sizeof slot_b);
    memcpy(slot_a, rec_a, sizeof rec_a);

    pick = apsis_project_journal_latest(slot_a, sizeof slot_a,
                                        slot_b, sizeof slot_b, &generation);
    assert(pick == 0 && generation == 5u);

    /* Power cut after erase + payload programming: header is still erased. */
    memcpy(slot_b + APSIS_PROJECT_PAYLOAD_OFFSET,
           rec_b + APSIS_PROJECT_PAYLOAD_OFFSET,
           APSIS_PROJECT_PAYLOAD_SIZE);
    pick = apsis_project_journal_latest(slot_a, sizeof slot_a,
                                        slot_b, sizeof slot_b, &generation);
    assert(pick == 0 && generation == 5u);

    /* Power cut in the middle of the commit header: still invalid. */
    memcpy(slot_b, rec_b, 10u);
    pick = apsis_project_journal_latest(slot_a, sizeof slot_a,
                                        slot_b, sizeof slot_b, &generation);
    assert(pick == 0 && generation == 5u);

    /* Header-last commit completed: the new generation wins. */
    memcpy(slot_b, rec_b, APSIS_PROJECT_HEADER_SIZE);
    pick = apsis_project_journal_latest(slot_a, sizeof slot_a,
                                        slot_b, sizeof slot_b, &generation);
    assert(pick == 1 && generation == 6u);

    /* Later corruption of B falls back to the older valid A. */
    slot_b[APSIS_PROJECT_PAYLOAD_OFFSET + 11u] ^= 0x01u;
    pick = apsis_project_journal_latest(slot_a, sizeof slot_a,
                                        slot_b, sizeof slot_b, &generation);
    assert(pick == 0 && generation == 5u);
}

static void test_generation_wrap(void)
{
    assert(apsis_project_generation_newer(6u, 5u) == 1u);
    assert(apsis_project_generation_newer(5u, 5u) == 0u);
    assert(apsis_project_generation_newer(5u, 6u) == 0u);
    assert(apsis_project_generation_newer(1u, 0xFFFFFFFEu) == 1u);
    assert(apsis_project_generation_newer(0xFFFFFFFEu, 1u) == 0u);
}

int main(void)
{
    test_crc_reference();
    test_payload_roundtrip();
    test_record_crc_and_version_gates();
    test_record_decode_resets_runtime();
    test_two_slot_power_loss_recovery();
    test_generation_wrap();
    puts("APSIS Phase 7 project-store tests passed");
    return 0;
}
