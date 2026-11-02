/* The finite voices' budget (apsis_voice_budget.h): each pool's ceiling holds however dense the playing, and a full
 * pool lets its least audible voice go. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string_dsp.h"
#include "apsis_string98.h"
#include "apsis_voice_budget.h"

static int32_t out[512];

static void string_render(ApsisStringTransientPool *p, uint32_t blocks)
{
    while (blocks--) {
        memset(out, 0, sizeof out);
        apsis_string_transient_render(p, out, 256u);
    }
}

static void test_string(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringTransientPool p;
    uint32_t i, k;
    uint64_t t = 0u;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&m);
    apsis_string_transient_pool_reset(&p);

    /* an old pluck, left to decay for 0.8 s, then fresh plucks until the pool is full and stealing */
    (void)apsis_string98_pluck(&p, &s, &m, t);
    string_render(&p, 138u);
    t += 138u * 256u;
    for (k = 0u; k < 60u; ++k) {
        t += 2000u;                                  /* past the 35 ms guard */
        (void)apsis_string98_pluck(&p, &s, &m, t);
        string_render(&p, 1u);
        assert(apsis_string_dsp_transient_logical_active(&p) <= APSIS_BUDGET_STRING_PARTIALS);
        assert(apsis_string_dsp_transient_physical_active(&p) <=
               APSIS_BUDGET_STRING_PARTIALS + APSIS_BUDGET_STRING_FADES);
        /* the pluck just struck is never what goes: all its partials are in their attack, at their peak */
        for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
            if (p.transient[i].active && p.transient[i].serial > p.transient_next_serial - 8u)
                assert(!p.transient[i].stealing);
    }
    assert(p.transient_steal_count > 0u && p.transient_drop_count == 0u);
    puts("PASS: String partials stay within the budget; a note just struck is never stolen");
}

int main(void)
{
    test_string();
    return 0;
}
