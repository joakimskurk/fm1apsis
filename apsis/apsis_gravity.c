#include "apsis_gravity.h"
#include "apsis_harmony.h"

static int8_t gravity_clamp_i8(int8_t value, int8_t lo, int8_t hi)
{
    return value < lo ? lo : (value > hi ? hi : value);
}

static uint16_t gravity_clamp_u16(uint16_t value, uint16_t lo, uint16_t hi)
{
    return value < lo ? lo : (value > hi ? hi : value);
}

static uint16_t gravity_q15_milli(uint32_t milli)
{
    return (uint16_t)((milli * APSIS_Q15_ONE + 500u) / 1000u);
}

uint8_t apsis_gravity_point_index(uint8_t degree, uint8_t ring)
{
    uint8_t d = degree > 7u ? 7u : degree;
    uint8_t r = ring > 2u ? 2u : ring;
    return (uint8_t)(d * APSIS_GRAVITY_RINGS + r);
}

ApsisGravityPoint apsis_gravity_point_default(uint8_t degree, uint8_t ring)
{
    static const uint16_t base_bright_milli[3] = {240u, 390u, 560u};
    static const uint16_t base_level_milli[3] = {760u, 680u, 600u};
    static const uint16_t base_attack_ms[3] = {50u, 34u, 20u};
    static const uint16_t base_release_ms[3] = {920u, 1120u, 1420u};
    ApsisGravityPoint p;
    uint8_t d = degree > 7u ? 7u : degree;
    uint8_t r = ring > 2u ? 2u : ring;
    uint8_t seed = (uint8_t)(((uint32_t)d * 17u + (uint32_t)r * 31u + 7u) % 23u);
    uint8_t mod = (uint8_t)(seed % 3u);
    p.enabled = 1u;
    p.offset = 0;
    p.octave = 0;
    p.level_q15 = gravity_q15_milli((uint32_t)base_level_milli[r] + 25u * mod);
    p.brightness_q15 = gravity_q15_milli((uint32_t)base_bright_milli[r] + 18u * mod);
    p.attack_ms = (uint16_t)(base_attack_ms[r] + ((d + r) % 3u) * 8u);
    p.release_ms = (uint16_t)(base_release_ms[r] + (d % 3u) * 70u);
    return p;
}

ApsisGravityPoint *apsis_gravity_point(ApsisGravityState *g, uint8_t degree, uint8_t ring)
{
    if (!g) return 0;
    return &g->points[apsis_gravity_point_index(degree, ring)];
}

const ApsisGravityPoint *apsis_gravity_point_const(const ApsisGravityState *g, uint8_t degree, uint8_t ring)
{
    if (!g) return 0;
    return &g->points[apsis_gravity_point_index(degree, ring)];
}

void apsis_gravity_sanitize_point(ApsisGravityPoint *p)
{
    if (!p) return;
    p->enabled = p->enabled ? 1u : 0u;
    p->offset = gravity_clamp_i8(p->offset, -2, 2);
    p->octave = gravity_clamp_i8(p->octave, -1, 1);
    p->level_q15 = gravity_clamp_u16(p->level_q15, APSIS_Q15_MILLI(150), APSIS_Q15_MILLI(1250));
    p->brightness_q15 = gravity_clamp_u16(p->brightness_q15, APSIS_Q15_MILLI(150), APSIS_Q15_MILLI(780));
    p->attack_ms = gravity_clamp_u16(p->attack_ms, 12u, 220u);
    p->release_ms = gravity_clamp_u16(p->release_ms, 250u, 3000u);
}

void apsis_gravity_reset_point(ApsisGravityState *g, uint8_t degree, uint8_t ring)
{
    ApsisGravityPoint *p = apsis_gravity_point(g, degree, ring);
    if (p) *p = apsis_gravity_point_default(degree, ring);
}

int32_t apsis_gravity_point_step(const ApsisState *s, uint8_t degree, uint8_t ring)
{
    static const int8_t anchors[3] = {0, 2, 8};
    const ApsisGravityPoint *p;
    uint8_t d, r;
    int32_t index;
    if (!s) return 0;
    d = degree > 7u ? 7u : degree;
    r = ring > 2u ? 2u : ring;
    p = apsis_gravity_point_const(&s->gravity, d, r);
    index = (int32_t)(d % 7u) + anchors[r] + (p ? p->offset : 0);
    return apsis_scale_at(s->gravity.tonic, s->gravity.scale, index) +
           (d == 7u ? 12 : 0) + (p ? (int32_t)p->octave * 12 : 0);
}
