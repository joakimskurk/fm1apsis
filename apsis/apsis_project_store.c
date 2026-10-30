#include "apsis_project_store.h"

#define APSIS_PROJECT_KEEP_COUNT APSIS_KEEP_SLOTS
#define APSIS_PROJECT_CHAIN_COUNT APSIS_CHAIN_MAX

_Static_assert(APSIS_KEEP_SLOTS == 8u, "Phase 7 format requires eight KEEP slots");
_Static_assert(APSIS_CHAIN_MAX == 32u, "Phase 7 format requires 32 CHAIN cells");
_Static_assert(APSIS_PROJECT_PAYLOAD_SIZE ==
               3u + APSIS_KEEP_SLOTS * APSIS_PROJECT_KEEP_WIRE_SIZE +
               APSIS_CHAIN_MAX * APSIS_PROJECT_CHAIN_CELL_WIRE_SIZE +
               APSIS_PROJECT_CHAIN_SETTINGS_WIRE_SIZE,
               "Phase 7 payload size mismatch");
_Static_assert(APSIS_PROJECT_RECORD_SIZE < APSIS_PROJECT_SECTOR_SIZE,
               "Phase 7 record must fit in one flash sector");

static void project_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t clamp_u8(uint8_t v, uint8_t lo, uint8_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int8_t clamp_i8(int8_t v, int8_t lo, int8_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint16_t clamp_q15(uint16_t v)
{
    return v > APSIS_Q15_ONE ? APSIS_Q15_ONE : v;
}

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint8_t put_i8(int8_t v)
{
    return (uint8_t)((int16_t)v & 0x00FF);
}

static int8_t get_i8(uint8_t v)
{
    int16_t x = v < 128u ? (int16_t)v : (int16_t)v - 256;
    return (int8_t)x;
}

uint32_t apsis_project_crc32(const uint8_t *data, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    uint32_t k;
    if (!data && n) return 0u;
    while (n--) {
        c ^= *data++;
        for (k = 0u; k < 8u; ++k)
            c = (c & 1u) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}

static void normalize_keep(ApsisKeepSnapshot *s)
{
    if (!s) return;
    if (!s->valid) {
        project_zero(s, (uint32_t)sizeof(*s));
        return;
    }
    s->valid = 1u;
    if (s->surface_mode > APSIS_STRING_DUAL) s->surface_mode = APSIS_STRING_CLASSIC;
    s->linked = s->linked ? 1u : 0u;
    if (s->left_anchor > APSIS_ANCHOR_NINTH) s->left_anchor = APSIS_ANCHOR_ROOT;
    if (s->right_anchor > APSIS_ANCHOR_NINTH) s->right_anchor = APSIS_ANCHOR_ROOT;
    s->left_follow_gravity = s->left_follow_gravity ? 1u : 0u;
    s->right_follow_gravity = s->right_follow_gravity ? 1u : 0u;
    s->left_gravity_degree = clamp_u8(s->left_gravity_degree, 0u, 7u);
    s->right_gravity_degree = clamp_u8(s->right_gravity_degree, 0u, 7u);
    s->drift_amount_q15 = clamp_q15(s->drift_amount_q15);
    s->clock_n = clamp_u8(s->clock_n, 3u, 24u);
    if (s->spiral_route_count > APSIS_SPIRAL_ROUTE_MAX)
        s->spiral_route_count = APSIS_SPIRAL_ROUTE_MAX;
    if (s->tuning_mode > APSIS_TUNING_PYTH) s->tuning_mode = APSIS_TUNING_EQUAL;
    s->gravity_tonic = (uint8_t)(s->gravity_tonic % 12u);
    if (s->gravity_scale >= APSIS_SCALE_COUNT) s->gravity_scale = APSIS_SCALE_DORIAN;
    s->gravity_degree = clamp_u8(s->gravity_degree, 0u, 7u);
    s->gravity_voicing = clamp_u8(s->gravity_voicing, 0u, 2u);
    s->gravity_ring = clamp_u8(s->gravity_ring, 0u, 2u);
    s->gravity_voices = clamp_u8(s->gravity_voices, 3u, 5u);
    if (s->gravity_inversion >= s->gravity_voices)
        s->gravity_inversion = (uint8_t)(s->gravity_voices - 1u);
}

static void encode_keep(uint8_t *p, const ApsisKeepSnapshot *src)
{
    ApsisKeepSnapshot s;
    uint32_t i;
    project_zero(&s, (uint32_t)sizeof(s));
    if (src) s = *src;
    normalize_keep(&s);
    if (!s.valid) {
        for (i = 0u; i < APSIS_PROJECT_KEEP_WIRE_SIZE; ++i) p[i] = 0u;
        return;
    }
    p[0] = s.valid;
    put_u16(p + 1u, s.split_q15);
    put_u16(p + 3u, s.left_split_q15);
    put_u16(p + 5u, s.right_split_q15);
    p[7] = s.surface_mode;
    p[8] = s.linked;
    p[9] = s.left_anchor;
    p[10] = s.right_anchor;
    p[11] = s.left_follow_gravity;
    p[12] = s.right_follow_gravity;
    p[13] = s.left_gravity_degree;
    p[14] = s.right_gravity_degree;
    put_u16(p + 15u, s.drift_amount_q15);
    p[17] = s.clock_n;
    p[18] = s.clock_bounds[0];
    p[19] = s.clock_bounds[1];
    p[20] = s.clock_bounds[2];
    put_u32(p + 21u, s.clock_main_mask);
    put_u32(p + 25u, s.clock_inner_mask);
    put_u32(p + 29u, s.clock_snap_mask);
    for (i = 0u; i < APSIS_SPIRAL_ROUTE_MAX; ++i) p[33u + i] = s.spiral_route[i];
    p[63] = s.spiral_route_count;
    p[64] = s.tuning_mode;
    p[65] = s.gravity_tonic;
    p[66] = s.gravity_scale;
    p[67] = s.gravity_degree;
    p[68] = s.gravity_voicing;
    p[69] = s.gravity_ring;
    p[70] = s.gravity_voices;
    p[71] = s.gravity_inversion;
}

static void decode_keep(ApsisKeepSnapshot *s, const uint8_t *p)
{
    uint32_t i;
    project_zero(s, (uint32_t)sizeof(*s));
    s->valid = p[0];
    s->split_q15 = get_u16(p + 1u);
    s->left_split_q15 = get_u16(p + 3u);
    s->right_split_q15 = get_u16(p + 5u);
    s->surface_mode = p[7];
    s->linked = p[8];
    s->left_anchor = p[9];
    s->right_anchor = p[10];
    s->left_follow_gravity = p[11];
    s->right_follow_gravity = p[12];
    s->left_gravity_degree = p[13];
    s->right_gravity_degree = p[14];
    s->drift_amount_q15 = get_u16(p + 15u);
    s->clock_n = p[17];
    s->clock_bounds[0] = p[18];
    s->clock_bounds[1] = p[19];
    s->clock_bounds[2] = p[20];
    s->clock_main_mask = get_u32(p + 21u);
    s->clock_inner_mask = get_u32(p + 25u);
    s->clock_snap_mask = get_u32(p + 29u);
    for (i = 0u; i < APSIS_SPIRAL_ROUTE_MAX; ++i) s->spiral_route[i] = p[33u + i];
    s->spiral_route_count = p[63];
    s->tuning_mode = p[64];
    s->gravity_tonic = p[65];
    s->gravity_scale = p[66];
    s->gravity_degree = p[67];
    s->gravity_voicing = p[68];
    s->gravity_ring = p[69];
    s->gravity_voices = p[70];
    s->gravity_inversion = p[71];
    normalize_keep(s);
}

static void normalize_cell(ApsisChainCell *c)
{
    if (!c) return;
    c->slot = clamp_u8(c->slot, 0u, 7u);
    c->cycles = clamp_u8(c->cycles, 0u, 32u);
    c->fine_pulses = clamp_u8(c->fine_pulses, 0u, 23u);
    if (c->cycles == 0u && c->fine_pulses == 0u) c->cycles = 1u;
    c->chance_q15 = clamp_q15(c->chance_q15);
    c->transpose = clamp_i8(c->transpose, -12, 12);
    if (!apsis_chain_morph_valid(c->morph_quarters))
        c->morph_quarters = APSIS_CHAIN_MORPH_NONE;
}

static void encode_cell(uint8_t *p, const ApsisChainCell *src)
{
    ApsisChainCell c;
    project_zero(&c, (uint32_t)sizeof(c));
    if (src) c = *src;
    normalize_cell(&c);
    p[0] = c.slot;
    p[1] = c.cycles;
    p[2] = c.fine_pulses;
    put_u16(p + 3u, c.chance_q15);
    p[5] = put_i8(c.transpose);
    p[6] = c.morph_quarters;
}

static void decode_cell(ApsisChainCell *c, const uint8_t *p)
{
    project_zero(c, (uint32_t)sizeof(*c));
    c->slot = p[0];
    c->cycles = p[1];
    c->fine_pulses = p[2];
    c->chance_q15 = get_u16(p + 3u);
    c->transpose = get_i8(p[5]);
    c->morph_quarters = p[6];
    normalize_cell(c);
}

static void normalize_chain_settings(ApsisChainState *s)
{
    if (!s) return;
    s->meter_lock = s->meter_lock ? 1u : 0u;
    if (s->start_sync > APSIS_CHAIN_SYNC_PULSE) s->start_sync = APSIS_CHAIN_SYNC_ONE;
    if (s->loop_sync > APSIS_CHAIN_SYNC_PULSE) s->loop_sync = APSIS_CHAIN_SYNC_ONE;
    s->default_cycles = clamp_u8(s->default_cycles, 0u, 32u);
    s->default_fine_pulses = clamp_u8(s->default_fine_pulses, 0u, 23u);
    if (s->default_cycles == 0u && s->default_fine_pulses == 0u) s->default_cycles = 1u;
    if (!apsis_chain_morph_valid(s->default_morph_quarters))
        s->default_morph_quarters = APSIS_CHAIN_MORPH_NONE;
    s->scope_key_scale = s->scope_key_scale ? 1u : 0u;
    s->scope_chord = s->scope_chord ? 1u : 0u;
    s->scope_tension = s->scope_tension ? 1u : 0u;
    s->scope_drift = s->scope_drift ? 1u : 0u;
    s->scope_clock = s->scope_clock ? 1u : 0u;
    s->scope_trace = s->scope_trace ? 1u : 0u;
    s->scope_tuning = s->scope_tuning ? 1u : 0u;
}

static void encode_chain_settings(uint8_t *p, const ApsisChainState *src)
{
    ApsisChainState s;
    project_zero(&s, (uint32_t)sizeof(s));
    if (src) s = *src;
    normalize_chain_settings(&s);
    p[0] = s.meter_lock;
    p[1] = s.start_sync;
    p[2] = s.loop_sync;
    p[3] = s.default_cycles;
    p[4] = s.default_fine_pulses;
    p[5] = s.default_morph_quarters;
    p[6] = s.scope_key_scale;
    p[7] = s.scope_chord;
    p[8] = s.scope_tension;
    p[9] = s.scope_drift;
    p[10] = s.scope_clock;
    p[11] = s.scope_trace;
    p[12] = s.scope_tuning;
}

static void decode_chain_settings(ApsisChainState *s, const uint8_t *p)
{
    project_zero(s, (uint32_t)sizeof(*s));
    s->meter_lock = p[0];
    s->start_sync = p[1];
    s->loop_sync = p[2];
    s->default_cycles = p[3];
    s->default_fine_pulses = p[4];
    s->default_morph_quarters = p[5];
    s->scope_key_scale = p[6];
    s->scope_chord = p[7];
    s->scope_tension = p[8];
    s->scope_drift = p[9];
    s->scope_clock = p[10];
    s->scope_trace = p[11];
    s->scope_tuning = p[12];
    normalize_chain_settings(s);
}

int apsis_project_payload_encode(uint8_t *dst, uint32_t capacity,
                                 const ApsisKeepBank *keep,
                                 const ApsisChainSequence *chain,
                                 const ApsisChainState *chain_settings)
{
    uint32_t i, off;
    uint8_t count;
    int8_t keep_selected, chain_selected;
    if (!dst || !keep || !chain || !chain_settings || capacity < APSIS_PROJECT_PAYLOAD_SIZE)
        return -1;

    keep_selected = keep->selected_slot;
    if (keep_selected < -1 || keep_selected >= (int8_t)APSIS_KEEP_SLOTS) keep_selected = -1;
    count = chain->count > APSIS_CHAIN_MAX ? APSIS_CHAIN_MAX : chain->count;
    chain_selected = chain->selected;
    if (chain_selected < 0 || chain_selected >= (int8_t)count) chain_selected = -1;

    dst[0] = put_i8(keep_selected);
    dst[1] = count;
    dst[2] = put_i8(chain_selected);
    off = 3u;
    for (i = 0u; i < APSIS_PROJECT_KEEP_COUNT; ++i) {
        encode_keep(dst + off, &keep->slots[i]);
        off += APSIS_PROJECT_KEEP_WIRE_SIZE;
    }
    for (i = 0u; i < APSIS_PROJECT_CHAIN_COUNT; ++i) {
        if (i < count) encode_cell(dst + off, &chain->cells[i]);
        else {
            uint32_t k;
            for (k = 0u; k < APSIS_PROJECT_CHAIN_CELL_WIRE_SIZE; ++k) dst[off + k] = 0u;
        }
        off += APSIS_PROJECT_CHAIN_CELL_WIRE_SIZE;
    }
    encode_chain_settings(dst + off, chain_settings);
    off += APSIS_PROJECT_CHAIN_SETTINGS_WIRE_SIZE;
    return off == APSIS_PROJECT_PAYLOAD_SIZE ? 0 : -2;
}

int apsis_project_payload_decode(const uint8_t *src, uint32_t n,
                                 ApsisKeepBank *keep,
                                 ApsisChainSequence *chain,
                                 ApsisChainState *chain_settings)
{
    uint32_t i, off;
    int8_t selected;
    if (!src || !keep || !chain || !chain_settings || n != APSIS_PROJECT_PAYLOAD_SIZE)
        return -1;

    apsis_keep_bank_reset(keep);
    apsis_chain_sequence_reset(chain);
    project_zero(chain_settings, (uint32_t)sizeof(*chain_settings));

    selected = get_i8(src[0]);
    keep->selected_slot = (selected >= 0 && selected < (int8_t)APSIS_KEEP_SLOTS) ? selected : -1;
    chain->count = src[1] > APSIS_CHAIN_MAX ? APSIS_CHAIN_MAX : src[1];
    selected = get_i8(src[2]);
    chain->selected = (selected >= 0 && selected < (int8_t)chain->count) ? selected : -1;

    off = 3u;
    for (i = 0u; i < APSIS_PROJECT_KEEP_COUNT; ++i) {
        decode_keep(&keep->slots[i], src + off);
        off += APSIS_PROJECT_KEEP_WIRE_SIZE;
    }
    for (i = 0u; i < APSIS_PROJECT_CHAIN_COUNT; ++i) {
        decode_cell(&chain->cells[i], src + off);
        off += APSIS_PROJECT_CHAIN_CELL_WIRE_SIZE;
    }
    decode_chain_settings(chain_settings, src + off);
    off += APSIS_PROJECT_CHAIN_SETTINGS_WIRE_SIZE;
    return off == APSIS_PROJECT_PAYLOAD_SIZE ? 0 : -2;
}

int apsis_project_record_build(uint8_t *dst, uint32_t capacity,
                               uint32_t generation,
                               const ApsisKeepBank *keep,
                               const ApsisChainSequence *chain,
                               const ApsisChainState *chain_settings)
{
    uint32_t payload_crc, header_crc;
    int rc;
    if (!dst || capacity < APSIS_PROJECT_RECORD_SIZE) return -1;
    project_zero(dst, APSIS_PROJECT_HEADER_SIZE);
    rc = apsis_project_payload_encode(dst + APSIS_PROJECT_PAYLOAD_OFFSET,
                                      capacity - APSIS_PROJECT_PAYLOAD_OFFSET,
                                      keep, chain, chain_settings);
    if (rc) return rc;

    payload_crc = apsis_project_crc32(dst + APSIS_PROJECT_PAYLOAD_OFFSET,
                                      APSIS_PROJECT_PAYLOAD_SIZE);
    put_u32(dst + 0u, APSIS_PROJECT_RECORD_MAGIC);
    put_u16(dst + 4u, APSIS_PROJECT_FORMAT_VERSION);
    put_u16(dst + 6u, APSIS_PROJECT_HEADER_SIZE);
    put_u32(dst + 8u, APSIS_PROJECT_PAYLOAD_SIZE);
    put_u32(dst + 12u, generation);
    put_u32(dst + 16u, payload_crc);
    header_crc = apsis_project_crc32(dst, 20u);
    put_u32(dst + 20u, header_crc);
    return 0;
}

int apsis_project_record_inspect(const uint8_t *record, uint32_t n,
                                 ApsisProjectRecordInfo *info)
{
    uint32_t generation, payload_crc;
    if (info) {
        info->valid = 0u;
        info->generation = 0u;
    }
    if (!record || n < APSIS_PROJECT_RECORD_SIZE) return -1;
    if (get_u32(record + 0u) != APSIS_PROJECT_RECORD_MAGIC) return -2;
    if (get_u16(record + 4u) != APSIS_PROJECT_FORMAT_VERSION) return -3;
    if (get_u16(record + 6u) != APSIS_PROJECT_HEADER_SIZE) return -4;
    if (get_u32(record + 8u) != APSIS_PROJECT_PAYLOAD_SIZE) return -5;
    if (apsis_project_crc32(record, 20u) != get_u32(record + 20u)) return -6;
    payload_crc = apsis_project_crc32(record + APSIS_PROJECT_PAYLOAD_OFFSET,
                                      APSIS_PROJECT_PAYLOAD_SIZE);
    if (payload_crc != get_u32(record + 16u)) return -7;
    generation = get_u32(record + 12u);
    if (info) {
        info->valid = 1u;
        info->generation = generation;
    }
    return 0;
}

int apsis_project_record_decode(const uint8_t *record, uint32_t n,
                                ApsisKeepBank *keep,
                                ApsisChainSequence *chain,
                                ApsisChainState *chain_settings,
                                uint32_t *generation)
{
    ApsisProjectRecordInfo info;
    int rc = apsis_project_record_inspect(record, n, &info);
    if (rc) return rc;
    rc = apsis_project_payload_decode(record + APSIS_PROJECT_PAYLOAD_OFFSET,
                                      APSIS_PROJECT_PAYLOAD_SIZE,
                                      keep, chain, chain_settings);
    if (!rc && generation) *generation = info.generation;
    return rc;
}

uint8_t apsis_project_generation_newer(uint32_t candidate, uint32_t reference)
{
    uint32_t delta = candidate - reference;
    return delta != 0u && delta < 0x80000000u ? 1u : 0u;
}

int8_t apsis_project_journal_latest(const uint8_t *a, uint32_t a_n,
                                    const uint8_t *b, uint32_t b_n,
                                    uint32_t *generation)
{
    ApsisProjectRecordInfo ia, ib;
    uint8_t va = apsis_project_record_inspect(a, a_n, &ia) == 0 ? 1u : 0u;
    uint8_t vb = apsis_project_record_inspect(b, b_n, &ib) == 0 ? 1u : 0u;
    int8_t pick;
    if (!va && !vb) return -1;
    if (va && !vb) pick = 0;
    else if (!va && vb) pick = 1;
    else pick = apsis_project_generation_newer(ib.generation, ia.generation) ? 1 : 0;
    if (generation) *generation = pick ? ib.generation : ia.generation;
    return pick;
}
