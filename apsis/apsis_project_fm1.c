#include "apsis_project_fm1.h"

/* This file is target-only and is included into Dinghy's single translation
 * unit after platform.c. st_read(), fl_erase4k(), fl_write() and flash_ok are
 * therefore intentionally the pinned platform primitives, not a second flash
 * driver. */

static uint32_t project_fm1_addr(uint8_t slot)
{
    return slot ? APSIS_PROJECT_FM1_SLOT_B : APSIS_PROJECT_FM1_SLOT_A;
}

static uint32_t project_fm1_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int project_fm1_read_valid(ApsisProjectFm1 *store, uint8_t slot,
                                  ApsisProjectRecordInfo *info)
{
    uint32_t at;
    int rc;
    if (!store || !info || !flash_ok) return -1;
    info->valid = 0u;
    info->generation = 0u;
    at = project_fm1_addr(slot);

    /* Empty factory sectors are overwhelmingly the common case. Read only the
     * 24-byte header first so first boot does not consume eight 256-byte flash
     * read windows merely to discover 0xFFFFFFFF. */
    rc = st_read(at, store->record, APSIS_PROJECT_HEADER_SIZE);
    if (rc) return rc;
    if (project_fm1_u32le(store->record) != APSIS_PROJECT_RECORD_MAGIC)
        return 1;

    rc = st_read(at, store->record, APSIS_PROJECT_RECORD_SIZE);
    if (rc) return rc;
    rc = apsis_project_record_inspect(store->record,
                                      APSIS_PROJECT_RECORD_SIZE, info);
    return rc ? 1 : 0;
}

void apsis_project_fm1_reset(ApsisProjectFm1 *store)
{
    uint32_t i;
    if (!store) return;
    store->generation = 0u;
    store->active_slot = -1;
    store->save_pending = 0u;
    store->last_result = 0;
    for (i = 0u; i < APSIS_PROJECT_RECORD_SIZE; ++i) store->record[i] = 0u;
}

int apsis_project_fm1_load(ApsisProjectFm1 *store,
                           ApsisKeepBank *keep,
                           ApsisChainSequence *chain,
                           ApsisChainState *chain_settings)
{
    ApsisProjectRecordInfo a, b;
    uint8_t va, vb, pick;
    uint32_t at, generation = 0u;
    int rc;
    if (!store || !keep || !chain || !chain_settings || !flash_ok) return -1;

    va = project_fm1_read_valid(store, 0u, &a) == 0 ? 1u : 0u;
    vb = project_fm1_read_valid(store, 1u, &b) == 0 ? 1u : 0u;
    if (!va && !vb) {
        store->active_slot = -1;
        store->generation = 0u;
        store->last_result = 1; /* clean no-project state */
        return 1;
    }
    if (va && !vb) pick = 0u;
    else if (!va && vb) pick = 1u;
    else pick = apsis_project_generation_newer(b.generation, a.generation) ? 1u : 0u;

    at = project_fm1_addr(pick);
    rc = st_read(at, store->record, APSIS_PROJECT_RECORD_SIZE);
    if (rc) {
        store->last_result = (int16_t)rc;
        return rc;
    }
    rc = apsis_project_record_decode(store->record,
                                     APSIS_PROJECT_RECORD_SIZE,
                                     keep, chain, chain_settings, &generation);
    if (rc) {
        store->last_result = (int16_t)rc;
        return rc;
    }
    store->active_slot = (int8_t)pick;
    store->generation = generation;
    store->last_result = 0;
    return 0;
}

void apsis_project_fm1_request_save(ApsisProjectFm1 *store)
{
    if (store) store->save_pending = 1u;
}

int apsis_project_fm1_service_save(ApsisProjectFm1 *store,
                                   const ApsisKeepBank *keep,
                                   const ApsisChainSequence *chain,
                                   const ApsisChainState *chain_settings)
{
    ApsisProjectRecordInfo info;
    uint32_t generation, at, took;
    uint8_t target;
    int rc;
    if (!store || !keep || !chain || !chain_settings || !flash_ok) return -1;
    if (!store->save_pending) return 0;

    /* Consume one explicit request per attempt. A failed NOR operation is not
     * hammered again every main-loop iteration; the UI may request a retry. */
    store->save_pending = 0u;
    target = store->active_slot == 0 ? 1u : 0u;
    generation = store->generation + 1u;
    rc = apsis_project_record_build(store->record,
                                    APSIS_PROJECT_RECORD_SIZE,
                                    generation, keep, chain, chain_settings);
    if (rc) goto done;

    at = project_fm1_addr(target);
    rc = fl_erase4k(at, &took);
    if (rc) goto done;

    /* Commit protocol: payload first, verify its CRC from flash, then publish
     * the 24-byte CRC-protected header. A power cut before the last operation
     * cannot turn this sector into the newest valid project. */
    rc = fl_write(at + APSIS_PROJECT_PAYLOAD_OFFSET,
                  store->record + APSIS_PROJECT_PAYLOAD_OFFSET,
                  APSIS_PROJECT_PAYLOAD_SIZE);
    if (rc) goto done;
    rc = st_read(at + APSIS_PROJECT_PAYLOAD_OFFSET,
                 store->record + APSIS_PROJECT_PAYLOAD_OFFSET,
                 APSIS_PROJECT_PAYLOAD_SIZE);
    if (rc) goto done;
    rc = apsis_project_record_inspect(store->record,
                                      APSIS_PROJECT_RECORD_SIZE, &info);
    if (rc) goto done;

    rc = fl_write(at, store->record, APSIS_PROJECT_HEADER_SIZE);
    if (rc) goto done;
    rc = st_read(at, store->record, APSIS_PROJECT_RECORD_SIZE);
    if (rc) goto done;
    rc = apsis_project_record_inspect(store->record,
                                      APSIS_PROJECT_RECORD_SIZE, &info);
    if (rc) goto done;
    if (!info.valid || info.generation != generation) {
        rc = -9;
        goto done;
    }

    store->active_slot = (int8_t)target;
    store->generation = generation;
    rc = 0;

done:
    store->last_result = (int16_t)rc;
    return rc;
}
