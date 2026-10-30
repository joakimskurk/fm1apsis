#include <stdint.h>

/* Phase-6 target-memory probe only.
 *
 * These are the exact int16 storage classes selected by the host FDN prototype:
 * - one shared stereo 140 ms input history (predelay + early-reflection taps),
 * - one maximum-length stereo 1.2 s feedback echo,
 * - the eight FDN lines plus 16 samples of interpolation/modulation reserve
 *   per line, packed into one contiguous tail store.
 *
 * The arrays deliberately live in Dinghy's large .pool section rather than
 * ordinary .bss. No Phase-6 renderer reads them yet. The tiny probe function
 * uses volatile accesses so compiler/linker GC cannot silently discard the
 * storage during the CI-only real-target memory gate.
 */
#define APSIS_SPACE_PROBE_INPUT_FRAMES 6174u
#define APSIS_SPACE_PROBE_ECHO_FRAMES 52920u
#define APSIS_SPACE_PROBE_FDN_SAMPLES 19170u

#define APSIS_SPACE_PROBE_INPUT_SAMPLES (APSIS_SPACE_PROBE_INPUT_FRAMES * 2u)
#define APSIS_SPACE_PROBE_ECHO_SAMPLES (APSIS_SPACE_PROBE_ECHO_FRAMES * 2u)

#define APSIS_SPACE_PROBE_INPUT_BYTES (APSIS_SPACE_PROBE_INPUT_SAMPLES * 2u)
#define APSIS_SPACE_PROBE_ECHO_BYTES (APSIS_SPACE_PROBE_ECHO_SAMPLES * 2u)
#define APSIS_SPACE_PROBE_FDN_BYTES (APSIS_SPACE_PROBE_FDN_SAMPLES * 2u)
#define APSIS_SPACE_PROBE_TOTAL_BYTES \
    (APSIS_SPACE_PROBE_INPUT_BYTES + APSIS_SPACE_PROBE_ECHO_BYTES + \
     APSIS_SPACE_PROBE_FDN_BYTES)

static volatile int16_t apsis_space_probe_input_history[APSIS_SPACE_PROBE_INPUT_SAMPLES]
    __attribute__((section(".pool"), used));
static volatile int16_t apsis_space_probe_echo[APSIS_SPACE_PROBE_ECHO_SAMPLES]
    __attribute__((section(".pool"), used));
static volatile int16_t apsis_space_probe_fdn[APSIS_SPACE_PROBE_FDN_SAMPLES]
    __attribute__((section(".pool"), used));

static uint32_t apsis_space_pool_probe(void)
{
    /* Touch both ends of every allocation. The values are already zeroed by
     * Dinghy; this is only a retention probe and is intentionally inaudible. */
    apsis_space_probe_input_history[0] = apsis_space_probe_input_history[APSIS_SPACE_PROBE_INPUT_SAMPLES - 1u];
    apsis_space_probe_echo[0] = apsis_space_probe_echo[APSIS_SPACE_PROBE_ECHO_SAMPLES - 1u];
    apsis_space_probe_fdn[0] = apsis_space_probe_fdn[APSIS_SPACE_PROBE_FDN_SAMPLES - 1u];
    return APSIS_SPACE_PROBE_TOTAL_BYTES;
}
