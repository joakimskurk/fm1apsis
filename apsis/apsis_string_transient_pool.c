/* M4b keeps the finite transient implementation mechanically separate from
 * the frozen M4a sustain struct. The implementation was intentionally written
 * against field names that are shared by ApsisStringTransientPool; this local
 * type alias lets us preserve the already-validated transient math while the
 * public ABI keeps ApsisStringDsp byte-for-byte M4a-compatible. */
#include "apsis_string_dsp.h"
#define ApsisStringDsp ApsisStringTransientPool
#include "apsis_string_transient.c"
#undef ApsisStringDsp

void apsis_string_transient_pool_reset(ApsisStringTransientPool *pool)
{
    if (!pool) return;
    strx_zero(pool, (uint32_t)sizeof(*pool));
}
