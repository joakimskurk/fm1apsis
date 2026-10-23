#include "apsis_keep.h"

static void apsis_keep_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t apsis_keep_clamp_u8(uint32_t v, uint8_t lo, uint8_t hi)
{
    return (uint8_t)(v < lo ? lo : (v > hi ? hi : v));
}

static uint32_t apsis_keep_clock_mask(uint8_t n)
{
    if (n >= 24u) return 0x00FFFFFFu;
    return (1u << n) - 1u;
}

static uint8_t apsis_keep_lane(const ApsisKeepApplyOptions *o, uint8_t bit)
{
    if (!o || !o->from_chain) return 1u;
    return (o->lane_mask & bit) ? 1u : 0u;
}

void apsis_keep_bank_reset(ApsisKeepBank *bank)
{
    if (!bank) return;
    apsis_keep_zero(bank, (uint32_t)sizeof(*bank));
    bank->selected_slot = -1;
}

ApsisKeepSnapshot apsis_keep_capture(const ApsisState *s)
{
    ApsisKeepSnapshot snap;
    uint8_t i, count;
    apsis_keep_zero(&snap, (uint32_t)sizeof(snap));
    if (!s) return snap;

    snap.valid = 1u;
    snap.split_q15 = s->string.right_split_q15;
    snap.left_split_q15 = s->string.left_split_q15;
    snap.right_split_q15 = s->string.right_split_q15;
    snap.surface_mode = s->string.surface_mode;
    snap.linked = s->string.linked ? 1u : 0u;
    snap.left_anchor = s->string.left_anchor;
    snap.right_anchor = s->string.right_anchor;
    snap.left_follow_gravity = s->string.left_follow_gravity ? 1u : 0u;
    snap.right_follow_gravity = s->string.right_follow_gravity ? 1u : 0u;
    snap.left_gravity_degree = apsis_keep_clamp_u8((uint8_t)s->string.left_gravity_degree, 0u, 7u);
    snap.right_gravity_degree = apsis_keep_clamp_u8((uint8_t)s->string.right_gravity_degree, 0u, 7u);

    snap.drift_amount_q15 = s->drift.amount_q15;

    snap.clock_n = apsis_keep_clamp_u8(s->clock.n, 3u, 24u);
    for (i = 0u; i < 3u; ++i) snap.clock_bounds[i] = s->clock.bounds[i];
    snap.clock_main_mask = s->clock.main_mask;
    snap.clock_inner_mask = s->clock.inner_mask;
    snap.clock_snap_mask = s->clock.snap_mask;

    count = s->spiral.route_count > APSIS_SPIRAL_ROUTE_MAX ? APSIS_SPIRAL_ROUTE_MAX : s->spiral.route_count;
    for (i = 0u; i < count; ++i) snap.spiral_route[i] = s->spiral.route[i];
    snap.spiral_route_count = count;

    snap.tuning_mode = s->temper.mode;

    snap.gravity_tonic = (uint8_t)(s->gravity.tonic % 12u);
    snap.gravity_scale = s->gravity.scale < APSIS_SCALE_COUNT ? s->gravity.scale : APSIS_SCALE_DORIAN;
    snap.gravity_degree = apsis_keep_clamp_u8(s->gravity.degree, 0u, 7u);
    snap.gravity_voicing = apsis_keep_clamp_u8(s->gravity.voicing, 0u, 2u);
    snap.gravity_ring = snap.gravity_voicing;
    snap.gravity_voices = apsis_keep_clamp_u8(s->gravity.voices, 3u, 5u);
    snap.gravity_inversion = s->gravity.inversion >= snap.gravity_voices
        ? (uint8_t)(snap.gravity_voices - 1u) : s->gravity.inversion;
    return snap;
}

int8_t apsis_keep_capture_to_bank(ApsisKeepBank *bank, const ApsisState *s)
{
    int8_t idx;
    uint8_t i;
    if (!bank || !s) return -1;
    idx = bank->selected_slot;
    if (idx < 0 || idx >= (int8_t)APSIS_KEEP_SLOTS) {
        idx = -1;
        for (i = 0u; i < APSIS_KEEP_SLOTS; ++i) {
            if (!bank->slots[i].valid) { idx = (int8_t)i; break; }
        }
        if (idx < 0) idx = (int8_t)(APSIS_KEEP_SLOTS - 1u);
    }
    bank->slots[(uint8_t)idx] = apsis_keep_capture(s);
    bank->selected_slot = idx;
    return idx;
}

uint8_t apsis_keep_lane_mask_from_chain(const ApsisChainState *c)
{
    uint8_t m = 0u;
    if (!c) return 0u;
    if (c->scope_key_scale) m |= APSIS_KEEP_LANE_KEY_SCALE;
    if (c->scope_chord) m |= APSIS_KEEP_LANE_CHORD;
    if (c->scope_tension) m |= APSIS_KEEP_LANE_TENSION;
    if (c->scope_drift) m |= APSIS_KEEP_LANE_DRIFT;
    if (c->scope_clock) m |= APSIS_KEEP_LANE_CLOCK;
    if (c->scope_trace) m |= APSIS_KEEP_LANE_TRACE;
    if (c->scope_tuning) m |= APSIS_KEEP_LANE_TUNING;
    return m;
}

uint8_t apsis_keep_apply(ApsisState *s, const ApsisKeepSnapshot *snap,
                         const ApsisKeepApplyOptions *o)
{
    uint8_t i, write, voices;
    uint32_t active;
    int32_t tonic;
    if (!s || !snap || !snap->valid) return 0u;

    if ((!o || !o->skip_continuous) && apsis_keep_lane(o, APSIS_KEEP_LANE_TENSION)) {
        s->string.left_split_q15 = snap->left_split_q15 > APSIS_Q15_MILLI(500)
            ? APSIS_Q15_MILLI(500) : snap->left_split_q15;
        s->string.right_split_q15 = snap->right_split_q15 < APSIS_Q15_MILLI(500)
            ? APSIS_Q15_MILLI(500) : snap->right_split_q15;
        s->string.split_q15 = s->string.right_split_q15;
        if (snap->surface_mode <= APSIS_STRING_DUAL) s->string.surface_mode = snap->surface_mode;
        s->string.linked = snap->linked ? 1u : 0u;
        if (snap->left_anchor <= APSIS_ANCHOR_NINTH) s->string.left_anchor = snap->left_anchor;
        if (snap->right_anchor <= APSIS_ANCHOR_NINTH) s->string.right_anchor = snap->right_anchor;
        s->string.left_follow_gravity = snap->left_follow_gravity ? 1u : 0u;
        s->string.right_follow_gravity = snap->right_follow_gravity ? 1u : 0u;
        s->string.left_gravity_degree = (int8_t)apsis_keep_clamp_u8(snap->left_gravity_degree, 0u, 7u);
        s->string.right_gravity_degree = (int8_t)apsis_keep_clamp_u8(snap->right_gravity_degree, 0u, 7u);
    }

    if ((!o || !o->skip_continuous) && apsis_keep_lane(o, APSIS_KEEP_LANE_DRIFT))
        s->drift.amount_q15 = snap->drift_amount_q15;

    if (apsis_keep_lane(o, APSIS_KEEP_LANE_CLOCK)) {
        s->clock.n = apsis_keep_clamp_u8(snap->clock_n, 3u, 24u);
        write = 0u;
        for (i = 0u; i < 3u; ++i) {
            uint8_t b = snap->clock_bounds[i];
            if (b > 0u && b < s->clock.n && write < 3u) s->clock.bounds[write++] = b;
        }
        while (write < 3u) s->clock.bounds[write++] = 0u;
        active = apsis_keep_clock_mask(s->clock.n);
        s->clock.main_mask = snap->clock_main_mask & active;
        s->clock.inner_mask = snap->clock_inner_mask & active;
        s->clock.snap_mask = snap->clock_snap_mask & active;
    }

    if (apsis_keep_lane(o, APSIS_KEEP_LANE_TRACE)) {
        write = 0u;
        for (i = 0u; i < snap->spiral_route_count && i < APSIS_SPIRAL_ROUTE_MAX; ++i) {
            if (snap->spiral_route[i] <= 35u) s->spiral.route[write++] = snap->spiral_route[i];
        }
        s->spiral.route_count = write;
        while (write < APSIS_SPIRAL_ROUTE_MAX) s->spiral.route[write++] = 0u;
    }

    if (apsis_keep_lane(o, APSIS_KEEP_LANE_TUNING) && snap->tuning_mode <= APSIS_TUNING_PYTH)
        s->temper.mode = snap->tuning_mode;

    if (apsis_keep_lane(o, APSIS_KEEP_LANE_KEY_SCALE)) {
        tonic = snap->gravity_tonic;
        if (o && o->from_chain) tonic += o->transpose;
        tonic %= 12;
        if (tonic < 0) tonic += 12;
        s->gravity.tonic = (uint8_t)tonic;
        s->gravity.scale = snap->gravity_scale < APSIS_SCALE_COUNT ? snap->gravity_scale : APSIS_SCALE_DORIAN;
    }

    if (apsis_keep_lane(o, APSIS_KEEP_LANE_CHORD)) {
        s->gravity.degree = apsis_keep_clamp_u8(snap->gravity_degree, 0u, 7u);
        s->gravity.voicing = apsis_keep_clamp_u8(snap->gravity_ring, 0u, 2u);
        s->gravity.selected_ring = s->gravity.voicing;
        voices = apsis_keep_clamp_u8(snap->gravity_voices, 3u, 5u);
        s->gravity.voices = voices;
        s->gravity.inversion = snap->gravity_inversion >= voices
            ? (uint8_t)(voices - 1u) : snap->gravity_inversion;
        s->gravity.selected_point_degree = s->gravity.degree;
    }
    return 1u;
}
