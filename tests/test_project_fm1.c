#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_keep.h"
#include "apsis_chain.h"
#include "apsis_project_store.h"
#include "apsis_project_fm1.h"

/* Host fake for the two FM-1 sectors. apsis_project_fm1.c is intentionally
 * target-TU code, so this test provides the same pinned platform primitives
 * before including it. */
static uint8_t fake_flash[2u * APSIS_PROJECT_SECTOR_SIZE];
static uint8_t flash_ok = 1u;
static int fail_erase;
static int fail_write_at_header;
static int corrupt_payload_readback;

static int fake_map(uint32_t off, uint32_t n, uint32_t *idx)
{
    uint32_t base;
    if (off >= APSIS_PROJECT_FM1_SLOT_A &&
        off + n <= APSIS_PROJECT_FM1_SLOT_A + APSIS_PROJECT_SECTOR_SIZE)
        base = APSIS_PROJECT_FM1_SLOT_A;
    else if (off >= APSIS_PROJECT_FM1_SLOT_B &&
             off + n <= APSIS_PROJECT_FM1_SLOT_B + APSIS_PROJECT_SECTOR_SIZE) {
        base = APSIS_PROJECT_FM1_SLOT_B;
        *idx = APSIS_PROJECT_SECTOR_SIZE + (off - base);
        return 0;
    } else
        return -8;
    *idx = off - base;
    return 0;
}

static int st_read(uint32_t off, void *p, uint32_t n)
{
    uint32_t idx;
    if (fake_map(off, n, &idx)) return -8;
    memcpy(p, fake_flash + idx, n);
    if (corrupt_payload_readback && n == APSIS_PROJECT_PAYLOAD_SIZE &&
        (off & 0xFFFu) == APSIS_PROJECT_PAYLOAD_OFFSET) {
        ((uint8_t *)p)[17] ^= 0x01u;
        corrupt_payload_readback = 0;
    }
    return 0;
}

static int fl_erase4k(uint32_t off, uint32_t *took)
{
    uint32_t idx;
    if (fail_erase) {
        fail_erase = 0;
        return -31;
    }
    if ((off & (APSIS_PROJECT_SECTOR_SIZE - 1u)) != 0u ||
        fake_map(off, APSIS_PROJECT_SECTOR_SIZE, &idx))
        return -8;
    memset(fake_flash + idx, 0xFF, APSIS_PROJECT_SECTOR_SIZE);
    if (took) *took = 50000u;
    return 0;
}

static int fl_write(uint32_t off, const void *p, uint32_t n)
{
    uint32_t idx, i;
    if (fail_write_at_header && n == APSIS_PROJECT_HEADER_SIZE &&
        (off == APSIS_PROJECT_FM1_SLOT_A || off == APSIS_PROJECT_FM1_SLOT_B)) {
        fail_write_at_header = 0;
        return -32;
    }
    if (fake_map(off, n, &idx)) return -8;
    /* NOR semantics: programming may only clear bits. */
    for (i = 0u; i < n; ++i) {
        uint8_t src = ((const uint8_t *)p)[i];
        if ((uint8_t)(~fake_flash[idx + i]) & src) return -33;
        fake_flash[idx + i] &= src;
    }
    return 0;
}

#include "../apsis/apsis_project_fm1.c"

static void defaults(ApsisState *state, ApsisKeepBank *keep,
                     ApsisChainSequence *chain)
{
    apsis_state_default(state);
    apsis_keep_bank_reset(keep);
    apsis_chain_sequence_reset(chain);
}

static void capture_distinct(ApsisState *state, ApsisKeepBank *keep,
                             ApsisChainSequence *chain, uint8_t slot,
                             uint8_t tonic, int8_t transpose)
{
    ApsisChainCell c;
    state->gravity.tonic = tonic;
    state->string.left_split_q15 = (uint16_t)(7000u + tonic * 100u);
    keep->slots[slot] = apsis_keep_capture(state);
    keep->selected_slot = (int8_t)slot;
    c = apsis_chain_default_cell(slot, &state->chain, state->clock.n);
    c.transpose = transpose;
    chain->cells[0] = c;
    chain->count = 1u;
    chain->selected = 0;
}

static void reset_fake(void)
{
    memset(fake_flash, 0xFF, sizeof fake_flash);
    flash_ok = 1u;
    fail_erase = 0;
    fail_write_at_header = 0;
    corrupt_payload_readback = 0;
}

static void test_save_alternate_and_reboot(void)
{
    ApsisState state, loaded_state;
    ApsisKeepBank keep, loaded_keep;
    ApsisChainSequence chain, loaded_chain;
    ApsisProjectFm1 store, boot;

    reset_fake();
    defaults(&state, &keep, &chain);
    apsis_project_fm1_reset(&store);
    assert(apsis_project_fm1_load(&store, &keep, &chain, &state.chain) == 1);
    assert(store.active_slot == -1 && store.generation == 0u);

    capture_distinct(&state, &keep, &chain, 2u, 5u, -3);
    apsis_project_fm1_request_save(&store);
    assert(store.save_pending == 1u);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) == 0);
    assert(store.active_slot == 0 && store.generation == 1u && !store.save_pending);

    state.gravity.tonic = 9u;
    keep.slots[2].gravity_tonic = 9u;
    chain.cells[0].transpose = 7;
    apsis_project_fm1_request_save(&store);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) == 0);
    assert(store.active_slot == 1 && store.generation == 2u);

    defaults(&loaded_state, &loaded_keep, &loaded_chain);
    apsis_project_fm1_reset(&boot);
    assert(apsis_project_fm1_load(&boot, &loaded_keep, &loaded_chain,
                                  &loaded_state.chain) == 0);
    assert(boot.active_slot == 1 && boot.generation == 2u);
    assert(loaded_keep.slots[2].gravity_tonic == 9u);
    assert(loaded_chain.count == 1u && loaded_chain.cells[0].transpose == 7);
    assert(loaded_chain.playing == 0u && loaded_chain.armed == 0u);
}

static void test_failed_new_commit_keeps_old_project(void)
{
    ApsisState state, loaded_state;
    ApsisKeepBank keep, loaded_keep;
    ApsisChainSequence chain, loaded_chain;
    ApsisProjectFm1 store, boot;

    reset_fake();
    defaults(&state, &keep, &chain);
    apsis_project_fm1_reset(&store);
    capture_distinct(&state, &keep, &chain, 1u, 3u, -4);
    apsis_project_fm1_request_save(&store);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) == 0);
    assert(store.active_slot == 0 && store.generation == 1u);

    keep.slots[1].gravity_tonic = 11u;
    chain.cells[0].transpose = 10;
    fail_write_at_header = 1;
    apsis_project_fm1_request_save(&store);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) == -32);
    assert(store.active_slot == 0 && store.generation == 1u);

    defaults(&loaded_state, &loaded_keep, &loaded_chain);
    apsis_project_fm1_reset(&boot);
    assert(apsis_project_fm1_load(&boot, &loaded_keep, &loaded_chain,
                                  &loaded_state.chain) == 0);
    assert(boot.active_slot == 0 && boot.generation == 1u);
    assert(loaded_keep.slots[1].gravity_tonic == 3u);
    assert(loaded_chain.cells[0].transpose == -4);
}

static void test_readback_corruption_aborts_before_header(void)
{
    ApsisState state, loaded_state;
    ApsisKeepBank keep, loaded_keep;
    ApsisChainSequence chain, loaded_chain;
    ApsisProjectFm1 store, boot;

    reset_fake();
    defaults(&state, &keep, &chain);
    apsis_project_fm1_reset(&store);
    capture_distinct(&state, &keep, &chain, 0u, 1u, 2);
    apsis_project_fm1_request_save(&store);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) == 0);

    keep.slots[0].gravity_tonic = 8u;
    corrupt_payload_readback = 1;
    apsis_project_fm1_request_save(&store);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) != 0);
    assert(store.active_slot == 0 && store.generation == 1u);

    defaults(&loaded_state, &loaded_keep, &loaded_chain);
    apsis_project_fm1_reset(&boot);
    assert(apsis_project_fm1_load(&boot, &loaded_keep, &loaded_chain,
                                  &loaded_state.chain) == 0);
    assert(loaded_keep.slots[0].gravity_tonic == 1u);
}

static void test_flash_unavailable_is_non_destructive(void)
{
    ApsisState state;
    ApsisKeepBank keep;
    ApsisChainSequence chain;
    ApsisProjectFm1 store;

    reset_fake();
    defaults(&state, &keep, &chain);
    apsis_project_fm1_reset(&store);
    flash_ok = 0u;
    apsis_project_fm1_request_save(&store);
    assert(apsis_project_fm1_service_save(&store, &keep, &chain, &state.chain) == -1);
    assert(store.active_slot == -1 && store.generation == 0u);
    flash_ok = 1u;
}

int main(void)
{
    test_save_alternate_and_reboot();
    test_failed_new_commit_keeps_old_project();
    test_readback_corruption_aborts_before_header();
    test_flash_unavailable_is_non_destructive();
    puts("APSIS Phase 7 FM-1 fake-flash integration tests passed");
    return 0;
}