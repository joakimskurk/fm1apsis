/* Spiral's derived route (apsis_spiral.c) against the scan it replaced: the nearest scale index by binary search,
 * and the count and single-step helpers the audio interrupt uses against the whole sequence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_harmony.h"
#include "apsis_spiral.h"
#include "apsis_state.h"

/* the old per-point map: a scan over -42..56 keeping the first strictly smaller error */
static int16_t reference_entry(const ApsisState *s, int32_t raw)
{
    int32_t i, best = 0, err = 999, base;
    if (!s->spiral.follow_frame)
        return (int16_t)(s->gravity.scale_lock
            ? apsis_quantize_step_to_scale(raw, s->gravity.tonic, s->gravity.scale, 1u) : raw);
    base = apsis_quantize_step_to_scale(raw, s->gravity.tonic, s->gravity.scale, 1u);
    for (i = -42; i <= 56; ++i) {
        int32_t cand = apsis_scale_at(s->gravity.tonic, s->gravity.scale, i);
        int32_t e = cand > base ? cand - base : base - cand;
        if (e < err) { err = e; best = i; }
    }
    return (int16_t)apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                                   best + (int32_t)(s->gravity.degree > 7u ? 7u : s->gravity.degree));
}

int main(void)
{
    ApsisState s;
    int16_t seq[APSIS_SPIRAL_ROUTE_MAX];
    uint8_t tonic, scale, degree, follow, lock, i, n;
    uint32_t checked = 0u;
    apsis_state_default(&s);
    for (i = 0u; i < 36u; ++i) s.spiral.route[i < APSIS_SPIRAL_ROUTE_MAX ? i : 0] = i;
    s.spiral.route_count = APSIS_SPIRAL_ROUTE_MAX;
    s.spiral.route[3] = 40u;                         /* an invalid point: skipped, as before */
    for (tonic = 0u; tonic < 12u; ++tonic)
        for (scale = 0u; scale < APSIS_SCALE_COUNT; ++scale)
            for (degree = 0u; degree <= 8u; ++degree)
                for (follow = 0u; follow < 2u; ++follow)
                    for (lock = 0u; lock < 2u; ++lock) {
                        int16_t at;
                        uint8_t k, w = 0u;
                        s.gravity.tonic = tonic;
                        s.gravity.scale = scale;
                        s.gravity.degree = degree;
                        s.spiral.follow_frame = follow;
                        s.gravity.scale_lock = lock;
                        n = apsis_spiral_derived_sequence(&s, seq);
                        assert(n == apsis_spiral_derived_count(&s));
                        for (k = 0u; k < APSIS_SPIRAL_ROUTE_MAX; ++k) {
                            if (s.spiral.route[k] > 35u) continue;
                            assert(seq[w] == reference_entry(&s, s.spiral.route[k]));
                            assert(apsis_spiral_derived_step(&s, w, &at) && at == seq[w]);
                            ++w;
                            ++checked;
                        }
                        assert(w == n && !apsis_spiral_derived_step(&s, n, &at));
                    }
    printf("PASS: the Spiral route's binary-search map equals the old scan (%u points)\n", checked);
    return 0;
}
