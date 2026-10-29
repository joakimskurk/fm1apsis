#include <stdint.h>
#include <stdio.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static uint32_t fnv(uint32_t h, int32_t v)
{
    uint32_t x = (uint32_t)v;
    h ^= x & 255u; h *= 16777619u;
    h ^= (x >> 8) & 255u; h *= 16777619u;
    h ^= (x >> 16) & 255u; h *= 16777619u;
    h ^= (x >> 24) & 255u; return h * 16777619u;
}

static uint32_t render_hash(ApsisState *s, uint32_t blocks)
{
    ApsisStringPitchMemory memory;
    ApsisStringDsp dsp;
    int32_t buf[512];
    uint32_t block, i, hash = 2166136261u;
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_dsp_reset(&dsp);
    apsis_string_dsp_update(&dsp, s, &memory, 1u);
    for (block = 0u; block < blocks; ++block) {
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&dsp, buf, 256u);
        /* M4b must be a mathematical no-op while no transient is active. */
        apsis_string_transient_render(&dsp, buf, 256u);
        for (i = 0u; i < 512u; ++i) hash = fnv(hash, buf[i]);
    }
    CHECK(apsis_string_dsp_transient_physical_active(&dsp) == 0u);
    CHECK(dsp.transient_trigger_count == 0u);
    return hash;
}

int main(void)
{
    ApsisState s;
    uint32_t classic, dual;

    apsis_state_default(&s);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    classic = render_hash(&s, 600u);
    CHECK(classic == 0xb20610c1u);

    apsis_state_default(&s);
    dual = render_hash(&s, 200u);
    CHECK(dual == 0x10d23cf4u);

    printf("PASS: M4a frozen audio hashes Classic=%08x Dual=%08x with idle M4b\n",
           classic, dual);
    return 0;
}
