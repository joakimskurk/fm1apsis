/* The stress preset (apsis_stress.c, docs/MARGIN_PLAN.md step A): the densest values the transport accepts, so the
 * diagnostic build measures the real worst case and not a value the transport quietly replaces with its default. */
#include <assert.h>
#include <stdio.h>

#include "apsis_stress.h"

int main(void)
{
    ApsisState s;
    apsis_state_stress(&s);
    assert(s.clock.bpm == 180u);                                     /* the transport's clamp is 35..180 */
    assert(s.clock.division_num == 1u && s.clock.division_den == 4u); /* 1/1, 1/2, 1/4: 1/4 is the densest */
    assert(s.string.rhythm_every == 1u);                             /* 1, 2, 4, 8 */
    assert(s.string.filter_attractor_every == APSIS_STRING_ATTRACTOR_HALF);
    assert(s.spiral.attractor_rate == 4u && s.spiral.route_count > 0u && s.spiral.sound);
    assert(s.bloom.cycles == 1u && s.bloom.rate == 4u);              /* cycles 0 is off */
    assert(s.bloom.length_ms == 6000u && s.space.decay_ms == 12000u);
    assert(s.string.sound && s.drift.sound && s.clock.sound && s.bloom.sound);
    puts("PASS: the stress preset is the densest the transport accepts");
    return 0;
}
