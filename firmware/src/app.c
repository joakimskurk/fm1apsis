/* SPDX-License-Identifier: GPL-3.0-only */
/* APSIS for the M-VAVE FM-1, on Dinghy 0.4 (firmware/src/platform.c and below are Dinghy's; see PLATFORM.md).
 *
 * One shared musical state (apsis/apsis_state.h) seen through ten surfaces, one shared sample-referenced
 * runtime/scheduler (apsis/apsis_runtime.c), five sounding sources into one shared Space/Cathedral.
 *   app_audio()   the audio interrupt: renders the sources and the room, advances the runtime; timed
 *   app_main()    the panel, the surfaces, the project journal; never renders sound
 * The browser APSIS (Study 126) is the authority for behaviour; docs/reference holds the contracts.
 * Rules of the chip (docs/dinghy/RULES.md): integers only, no float, nothing slow in the audio interrupt. */
#define APP_NAME "APSIS"
#include "platform.c"
#ifdef DINGHY_HOST                              /* (tests/host.c has no cycle counter and an instant screen) */
#ifdef APSIS_HOST_TIMING   /* the computer's clock in ns instead, so the load probes show where a host run spends */
#include <time.h>
static uint32_t fm1_ticks(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec);
}
#define FM1_TICKS_PER_US 1000u
#else
#define fm1_ticks() 0u
#define FM1_TICKS_PER_US 1u
#endif
#define fm1_lcd_done() 1
static const uint8_t lcd_busy = 0;
#endif
#include "apsis_state.h"
#include "apsis_harmony.h"
#include "apsis_gravity.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string98.h"
#include "apsis_string_attractor.h"
#include "apsis_drift.h"
#include "apsis_drift_dsp.h"
#include "apsis_clock.h"
#include "apsis_clock_dsp.h"
#include "apsis_spiral.h"
#include "apsis_spiral_audio.h"
#include "apsis_spiral_audio_runtime.h"
#include "apsis_spiral_dsp.h"
#include "apsis_runtime_m4f.h"
#include "apsis_bloom.h"
#include "apsis_bloom_audio.h"
#include "apsis_bloom_audio_runtime.h"
#include "apsis_bloom_dsp.h"
#include "apsis_space_dsp.h"
#include "apsis_space_room.h"
#include "apsis_space_target.h"
#include "apsis_project_store.h"
#include "apsis_project_fm1.h"
#include "apsis_visual.h"
#include "apsis_visual_p8b.h"
#include "apsis_visual_p8c.h"
#include "apsis_controls.h"
#include "apsis_controls_drift.h"
#include "apsis_controls_bloom.h"
#include "apsis_controls_keep_chain.h"
#include "apsis_controls_mother.h"
#include "apsis_controls_space.h"
#include "apsis_controls_apply.h"
#include "apsis_controls_dispatch.h"
#include "apsis_controls_dirty.h"
#include "apsis_controls_performance.h"
#include "apsis_bloom_manual_audio.h"
#include "apsis_string_motion.h"
#include "apsis_string_motion_dsp.h"
#include "apsis_string_motion_clock.h"
#include "apsis_controls_string_motion.h"
#include "apsis_string_motion_mailbox.h"
#include "apsis_controls_string_attractor.h"
#include "apsis_controls_spiral_attractor.h"
#include "apsis_controls_gravity_point.h"
#include "apsis_runtime_m4g.h"
#include "apsis_keep.h"
#include "apsis_chain.h"
#include "apsis_mother.h"
#include "apsis_transport.h"
#include "apsis_runtime.h"
#include "apsis_q31.c"
#include "apsis_sine.c"
#include "apsis_state.c"
#if defined(APSIS_STRESS) || defined(APSIS_DEMO)
#include "apsis_stress.c"
#endif
#include "apsis_harmony.c"
#include "apsis_gravity.c"
#include "apsis_string.c"
#include "apsis_string_dsp.c"
#include "apsis_string_transient_pool.c"
#include "apsis_string98.c"
#include "apsis_string_attractor.c"
#include "apsis_drift.c"
#include "apsis_drift_dsp.c"
#include "apsis_clock.c"
#include "apsis_target_div64.c"
#include "apsis_clock_dsp.c"
#include "apsis_spiral.c"
#include "apsis_spiral_audio.c"
#include "apsis_spiral_audio_runtime.c"
#include "apsis_spiral_dsp.c"
#include "apsis_bloom.c"
#include "apsis_bloom_audio.c"
#include "apsis_bloom_audio_runtime.c"
#include "apsis_bloom_dsp.c"
#include "apsis_space_dsp.c"
#include "apsis_space_room.c"
#include "apsis_space_target.c"
#include "apsis_project_store.c"
/* the project journal's flash (apsis_project_fm1.c): Dinghy's driver on the FM-1, two RAM sectors in host mode */
#ifdef DINGHY_HOST
static uint8_t host_project_flash[2][4096];
static uint8_t host_project_flash_init;
#define PROJECT_FLASH_OK 1
static uint8_t *project_flash_at(uint32_t at, uint32_t n)
{
    uint32_t i;
    if (!host_project_flash_init) {
        for (i = 0; i < sizeof host_project_flash; ++i) (&host_project_flash[0][0])[i] = 0xFFu;
        host_project_flash_init = 1u;
    }
    if (at < BLOBS_AT || at + n > BLOBS_AT + sizeof host_project_flash) return 0;
    return &host_project_flash[0][0] + (at - BLOBS_AT);
}
static int project_flash_read(uint32_t at, void *p, uint32_t n)
{
    const uint8_t *f = project_flash_at(at, n);
    uint8_t *d = p;
    if (!f) return -1;
    while (n--) *d++ = *f++;
    return 0;
}
static int project_flash_erase(uint32_t at)
{
    uint8_t *f = project_flash_at(at, 4096u);
    uint32_t i;
    if (!f || (at & 0xFFFu)) return -1;
    for (i = 0; i < 4096u; ++i) f[i] = 0xFFu;
    return 0;
}
static int project_flash_write(uint32_t at, const uint8_t *p, uint32_t n)
{
    uint8_t *f = project_flash_at(at, n);
    if (!f) return -1;
    while (n--) *f++ &= *p++;                   /* NOR: programming only clears bits */
    return 0;
}
#else
#define PROJECT_FLASH_OK flash_ok
static int project_flash_read(uint32_t at, void *p, uint32_t n) { return st_read(at, p, n); }
static int project_flash_erase(uint32_t at) { uint32_t took; return fl_erase4k(at, &took); }
static int project_flash_write(uint32_t at, const uint8_t *p, uint32_t n) { return fl_write(at, p, n); }
#endif
#include "apsis_project_fm1.c"
#include "apsis_visual.c"
#include "apsis_visual_p8b.c"
#include "apsis_visual_p8c.c"
#include "apsis_controls.c"
#include "apsis_controls_drift.c"
#include "apsis_controls_bloom.c"
#include "apsis_controls_keep_chain.c"
#include "apsis_controls_mother.c"
#include "apsis_controls_space.c"
#include "apsis_controls_apply.c"
#include "apsis_controls_dispatch.c"
#include "apsis_controls_dirty.c"
#include "apsis_controls_performance.c"
#include "apsis_bloom_manual_audio.c"
#include "apsis_string_motion.c"
#include "apsis_string_motion_dsp.c"
#include "apsis_string_motion_clock.c"
#include "apsis_controls_string_motion.c"
#include "apsis_string_motion_mailbox.c"
#include "apsis_controls_string_attractor.c"
#include "apsis_controls_spiral_attractor.c"
#include "apsis_controls_gravity_point.c"
#include "apsis_keep.c"
#include "apsis_chain.c"
#include "apsis_mother.c"
#include "apsis_transport.c"
#include "apsis_late_binding.c"
#include "apsis_runtime.c"

static ApsisState apsis;
static ApsisStringPitchMemory string_pitch_memory;
static ApsisStringDsp string_dsp;
static ApsisDriftDsp drift_dsp;
static ApsisClockDsp clock_dsp;
static ApsisSpiralDsp spiral_dsp;
static ApsisBloomVoicingState bloom_voicing_state;
static ApsisKeepBank keep_bank;
static ApsisChainSequence chain_sequence;
static ApsisMotherPending mother_pending;
static ApsisRuntime apsis_runtime;
static volatile uint8_t runtime_ready;
/* Telemetry for the emulator tests (symbols looked up in the ELF). The runtime: magic, queue high-water, queue
 * overflow, sample cursor, late/starve flags, late trace hash. The audio interrupt's load: the last block, the worst
 * block, blocks rendered, their total (us; a block is 5805 us). */
static volatile uint32_t apsis_runtime_probe[6];
static volatile uint32_t apsis_load_probe[4];
/* the worst block so far, by part, on core 0 (us): 0 the block's updates and Clock triggers, 1 the String job
 * handed to core 1 (or done here: drone, Drift, notes), 2 the wait for core 1, 3 Clock, 4 the six commits,
 * 5 the runtime's planning (Spiral, Bloom, the attractor), 6 the triggered notes (strums, rhythm, plucks, Bloom
 * gestures), 7 the room and Motion, 8 Spiral and Bloom's sound (rendered inside part 5, timed apart) */
#define BLOCK_PARTS 9u
static volatile uint32_t apsis_worst_probe[BLOCK_PARTS];
/* each part's own worst (us), and how many blocks went over 80 % and over 100 % of the 5805 us */
static volatile uint32_t apsis_part_max_probe[BLOCK_PARTS];
static volatile uint32_t apsis_part_max_block_probe[BLOCK_PARTS];   /* the block number of each part's worst */
/* the first blocks after boot fill tables (the Q31 root memo, the first plans) once: their worst is kept apart, and
 * the part maxima above start after them */
#define BOOT_BLOCKS 16u
static volatile uint32_t apsis_boot_worst_probe;
static volatile uint32_t apsis_over_probe[2];
/* the finite voices' high water: String notes (logical, physical), Bloom petals, Clock voices */
static volatile uint32_t apsis_voice_probe[4];
static uint32_t block_part_us[BLOCK_PARTS];
static uint32_t block_mark;

static uint32_t sound_ticks;

#ifdef APSIS_STRESS
/* what one voice costs, measured while the stress build plays (docs/MARGIN_PLAN.md B): for String partials, Spiral
 * oscillators, Bloom oscillators and Clock voices, ns per voice per block at the most voices seen (the worst block
 * at that count), so a pool's fixed cost does not inflate it. [2k] ns, [2k + 1] voices. */
static volatile uint32_t apsis_cost_probe[8];

static void cost_note(uint32_t k, uint32_t ticks, uint32_t voices)
{
    uint32_t ns;
    if (voices < 4u)
        return;
    ns = (uint32_t)((uint64_t)ticks * 1000u / FM1_TICKS_PER_US / voices);
    if (voices > apsis_cost_probe[2u * k + 1u] ||
        (voices == apsis_cost_probe[2u * k + 1u] && ns > apsis_cost_probe[2u * k])) {
        apsis_cost_probe[2u * k] = ns;
        apsis_cost_probe[2u * k + 1u] = voices;
    }
}

static uint32_t clock_voices(void)
{
    uint32_t i, v = 0u;
    for (i = 0u; i < APSIS_CLOCK_DSP_VOICES; ++i) v += clock_dsp.voice[i].active;
    return v;
}
/* the planning part (5) by piece, per block, its worst (us): [0] the runtime's planning, [1] Spiral's note
 * setup, [2] Bloom's note setup */
static volatile uint32_t apsis_plan_probe[3];
static uint32_t plan_ticks[3];
#define PLAN_BEGIN() uint32_t plan_t0 = fm1_ticks()
#define PLAN_END(k) (plan_ticks[(k)] += fm1_ticks() - plan_t0)
#define COST_BEGIN(count) uint32_t cost_n = (count), cost_t0 = fm1_ticks()
#define COST_END(k) cost_note((k), fm1_ticks() - cost_t0, cost_n)
#else
#define COST_BEGIN(count) do { } while (0)
#define COST_END(k) do { } while (0)
#define PLAN_BEGIN() do { } while (0)
#define PLAN_END(k) do { } while (0)
#endif

static void block_part(uint32_t part)
{
    uint32_t now = fm1_ticks();
    block_part_us[part] = (uint32_t)(now - block_mark) / FM1_TICKS_PER_US;
    block_mark = now;
}

/* part 5 without the Spiral and Bloom sound inside it, which is part 8 */
static void block_part_planning(void)
{
    uint32_t now = fm1_ticks(), t = now - block_mark;
    block_part_us[5] = (t > sound_ticks ? t - sound_ticks : 0u) / FM1_TICKS_PER_US;
    block_part_us[8] = sound_ticks / FM1_TICKS_PER_US;
    block_mark = now;
}
static ApsisStringTransientPool string_transient_pool;
static ApsisStringAttractorRuntime string_attractor_runtime;
static ApsisSpiralAudioRuntime spiral_audio_runtime;
static uint8_t spiral_audio_started;
static ApsisBloomAudioRuntime bloom_audio_runtime;
static ApsisBloomDsp bloom_dsp;
static uint8_t bloom_audio_started;
static ApsisSpaceTarget space_target;
static ApsisProjectFm1 project_store;

enum {
    SURF_MOTHER = 0,
    SURF_STRING,
    SURF_DRIFT,
    SURF_CLOCK,
    SURF_SPIRAL,
    SURF_BLOOM,
    SURF_GRAVITY,
    SURF_CHAIN,
    SURF_KEEP,
    SURF_SPACE,
    SURF_COUNT
};

static uint8_t surface = SURF_MOTHER;
static ApsisControlsSession apsis_controls_session;
static ApsisControlsProjectDirty apsis_project_dirty;
static ApsisControlsPerformanceMailbox apsis_controls_performance;
static ApsisBloomManualAudio apsis_bloom_manual_audio;
static ApsisStringMotionStore apsis_string_motion_store
    __attribute__((section(".pool.apsis_motion"), aligned(4)));
static ApsisStringMotionRuntime apsis_string_motion_runtime;
static ApsisStringMotionMailbox apsis_string_motion_mailbox;
static ApsisStringMotionPulseBatch apsis_string_motion_pulses;
static ApsisStringMotionPlayback apsis_string_motion_playback_frame;
static uint8_t apsis_string_motion_active;
static uint8_t apsis_string_motion_playback_valid;
static ApsisStringAttractorEditorSession apsis_string_attractor_editor;
static ApsisSpiralAttractorEditorSession apsis_spiral_attractor_editor;
static ApsisGravityPointEditorSession apsis_gravity_point_editor;

_Static_assert((int)BTN_OCT_DOWN == (int)APSIS_FM1_BTN_OCT_DOWN, "P9 BTN_OCT_DOWN id");
_Static_assert((int)BTN_OCT_UP == (int)APSIS_FM1_BTN_OCT_UP, "P9 BTN_OCT_UP id");
_Static_assert((int)BTN_FX == (int)APSIS_FM1_BTN_FX, "P9 BTN_FX id");
_Static_assert((int)BTN_SEL == (int)APSIS_FM1_BTN_SEL, "P9 BTN_SEL id");
_Static_assert((int)BTN_ENV == (int)APSIS_FM1_BTN_ENV, "P9 BTN_ENV id");
_Static_assert((int)BTN_LFO == (int)APSIS_FM1_BTN_LFO, "P9 BTN_LFO id");
_Static_assert((int)BTN_EDIT == (int)APSIS_FM1_BTN_EDIT, "P9 BTN_EDIT id");
_Static_assert((int)BTN_GLO == (int)APSIS_FM1_BTN_GLO, "P9 BTN_GLO id");
_Static_assert((int)BTN_HOME == (int)APSIS_FM1_BTN_HOME, "P9 BTN_HOME id");
_Static_assert((int)BTN_SAVE == (int)APSIS_FM1_BTN_SAVE, "P9 BTN_SAVE id");
_Static_assert((int)BTN_ARP == (int)APSIS_FM1_BTN_ARP, "P9 BTN_ARP id");
_Static_assert((int)BTN_SEQ == (int)APSIS_FM1_BTN_SEQ, "P9 BTN_SEQ id");
_Static_assert((int)BTN_PLAY == (int)APSIS_FM1_BTN_PLAY, "P9 BTN_PLAY id");
_Static_assert((int)BTN_REC == (int)APSIS_FM1_BTN_REC, "P9 BTN_REC id");
_Static_assert((int)ENC_SELECT == (int)APSIS_FM1_ENC_SELECT, "P9 ENC_SELECT id");
_Static_assert((int)ENC_ALGORITHM == (int)APSIS_FM1_ENC_ALGORITHM, "P9 ENC_ALGORITHM id");
_Static_assert((int)ENC_KNOB1 == (int)APSIS_FM1_ENC_K1, "P9 ENC_K1 id");
_Static_assert((int)ENC_KNOB2 == (int)APSIS_FM1_ENC_K2, "P9 ENC_K2 id");
_Static_assert((int)ENC_KNOB3 == (int)APSIS_FM1_ENC_K3, "P9 ENC_K3 id");
_Static_assert((int)ENC_KNOB4 == (int)APSIS_FM1_ENC_K4, "P9 ENC_K4 id");
_Static_assert((int)ENC_PRESETS == (int)APSIS_FM1_ENC_PRESETS, "P9 ENC_PRESETS id");
#ifndef DINGHY_HOST
_Static_assert((int)FM1_NENC == (int)APSIS_FM1_ENCODER_COUNT, "P9 encoder count");
_Static_assert(FM1_NKEY - APSIS_FM1_BUTTON_COUNT == APSIS_FM1_KEY_COUNT, "P9 note-key count");
#endif
_Static_assert((int)SURF_MOTHER == (int)APSIS_SURFACE_MOTHER, "P9 MOTHER surface order");
_Static_assert((int)SURF_STRING == (int)APSIS_SURFACE_STRING, "P9 STRING surface order");
_Static_assert((int)SURF_DRIFT == (int)APSIS_SURFACE_DRIFT, "P9 DRIFT surface order");
_Static_assert((int)SURF_CLOCK == (int)APSIS_SURFACE_CLOCK, "P9 CLOCK surface order");
_Static_assert((int)SURF_SPIRAL == (int)APSIS_SURFACE_SPIRAL, "P9 SPIRAL surface order");
_Static_assert((int)SURF_BLOOM == (int)APSIS_SURFACE_BLOOM, "P9 BLOOM surface order");
_Static_assert((int)SURF_GRAVITY == (int)APSIS_SURFACE_GRAVITY, "P9 GRAVITY surface order");
_Static_assert((int)SURF_CHAIN == (int)APSIS_SURFACE_CHAIN, "P9 CHAIN surface order");
_Static_assert((int)SURF_KEEP == (int)APSIS_SURFACE_KEEP, "P9 KEEP surface order");
_Static_assert((int)SURF_SPACE == (int)APSIS_SURFACE_SPACE, "P9 SPACE surface order");
_Static_assert((int)SURF_COUNT == (int)APSIS_SURFACE_COUNT, "P9 surface count");
_Static_assert(SURF_SPACE == (int)APSIS_VISUAL_SURFACE_SPACE && SURF_COUNT == (int)APSIS_VISUAL_SURFACE_COUNT,
               "surfaces and visual surfaces share one order");
_Static_assert(APSIS_PROJECT_FM1_SLOT_A == BLOBS_AT && APSIS_PROJECT_FM1_SLOT_B == BLOBS_AT + 0x1000u && NBLOB >= 2u,
               "the project journal owns blob sectors 0 and 1");


static void apsis_p10_motion_recompute_active(void)
{
    apsis_string_motion_active =
        (uint8_t)(apsis_string_motion_runtime.armed ||
                  apsis_string_motion_runtime.recording ||
                  apsis.string.motion_loop_play);
    if (!apsis_string_motion_active)
        apsis_string_motion_playback_valid = 0u;
}

static void apsis_p10_motion_take_control(void)
{
    uint32_t packed;
    ApsisStringMotionMailboxFrame frame;

    if (!apsis_string_motion_mailbox.pending) return;
    packed = apsis_string_motion_mailbox_take(&apsis_string_motion_mailbox);
    if (!packed) return;
    apsis_string_motion_mailbox_decode(packed, &frame);

    if (frame.target_valid)
        apsis_string_motion_runtime.record_target = frame.target;

    if (frame.action == APSIS_STRING_MOTION_ACTION_ARM_TOGGLE) {
        (void)apsis_string_motion_arm(&apsis_string_motion_runtime, &apsis,
                                      apsis_string_motion_runtime.record_target);
    } else if (frame.action == APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE) {
        (void)apsis_string_motion_toggle_play(&apsis_string_motion_store,
                                              &apsis_string_motion_runtime,
                                              &apsis);
    } else if (frame.action == APSIS_STRING_MOTION_ACTION_CLEAR) {
        /* Metadata-only clear. The 19 KiB NOLOAD body is unreachable once both
         * counts are zero, so never memset it from the audio ISR. */
        apsis_string_motion_store.left.count = 0u;
        apsis_string_motion_store.right.count = 0u;
        apsis_string_motion_reset(0, &apsis_string_motion_runtime);
        apsis.string.motion_loop_play = 0u;
    }

    if (frame.manual_valid)
        apsis_string_motion_manual_override(&apsis_string_motion_runtime,
                                            frame.manual_active);
    if (frame.capture && apsis_string_motion_runtime.recording)
        (void)apsis_string_motion_capture(&apsis_string_motion_store,
                                          &apsis_string_motion_runtime,
                                          &apsis, apsis_runtime.cursor_sample, 0u);
    apsis_p10_motion_recompute_active();
}

static void apsis_p10_motion_prepare_block(uint32_t n)
{
    apsis_p10_motion_take_control();
    if (!apsis_string_motion_active) return;

    apsis_string_motion_collect_pulses(&apsis_runtime, &apsis, n,
                                       &apsis_string_motion_pulses);
    apsis_string_motion_playback_valid = apsis_string_motion_playback(
        &apsis_string_motion_store, &apsis_string_motion_runtime, &apsis,
        apsis_runtime.cursor_sample, &apsis_string_motion_playback_frame);
    if (apsis_string_motion_playback_valid) {
        /* Playback owns the visible physical String coordinates, while P10B
         * separately restores the frozen absolute pitch after M4a update. */
        apsis.string.left_split_q15 = apsis_string_motion_playback_frame.left_split_q15;
        apsis.string.right_split_q15 = apsis_string_motion_playback_frame.right_split_q15;
        apsis.string.split_q15 = apsis.string.right_split_q15;
    }
}

static void apsis_p10_motion_finish_block(void)
{
    uint8_t i;
    if (!apsis_string_motion_active) return;

    /* Pulses were observed before M3c consumed the current callback. Apply them
     * only after this block has sounded: native Motion may be <=1 block late,
     * but is never early relative to the shared Clock. */
    for (i = 0u; i < apsis_string_motion_pulses.count; ++i) {
        const ApsisStringMotionPulse *pulse = &apsis_string_motion_pulses.pulse[i];
        apsis_string_motion_pulse(&apsis_string_motion_store,
                                  &apsis_string_motion_runtime, &apsis,
                                  pulse->step, pulse->sample,
                                  pulse->interval_samples);
    }
    if (apsis_string_motion_runtime.recording)
        (void)apsis_string_motion_capture(&apsis_string_motion_store,
                                          &apsis_string_motion_runtime,
                                          &apsis, apsis_runtime.cursor_sample, 0u);
    apsis_string_motion_restore_owned(&apsis_string_motion_runtime, &apsis);
    apsis_p10_motion_recompute_active();
}

/* ---- the second core (docs/MARGIN_PLAN.md D; platform cpu1.c, from fm1-dock): each block, core 1 renders the
 * String drone, Drift and the String notes while core 0 renders Clock and plans (Spiral and Bloom with it). Every
 * source renders into its own buffer, and they are all committed in the one-core order with the gains of that
 * order, so two cores sound exactly as one. With core 1 off (not started, screening, held), core 0 runs the job
 * itself in the same place. */
#ifdef DINGHY_HOST
#define APSIS_POOL_BUF __attribute__((aligned(4)))
static int cpu1_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn; (void)arg; return 0; }
static int cpu1_wait(void) { return 0; }
static void cpu1_screen_begin(void) {}
static void cpu1_screen_end(void) {}
#else
#define APSIS_POOL_BUF __attribute__((section(".pool.apsis_c1"), used, aligned(4)))
#endif
#define SRC_SAMPLES 512u                            /* 256 frames, stereo */
static int32_t buf_drone[SRC_SAMPLES] APSIS_POOL_BUF, buf_drift[SRC_SAMPLES] APSIS_POOL_BUF;
static int32_t buf_notes[SRC_SAMPLES] APSIS_POOL_BUF, buf_clock[SRC_SAMPLES] APSIS_POOL_BUF;
static int32_t buf_spiral[SRC_SAMPLES] APSIS_POOL_BUF, buf_bloom[SRC_SAMPLES] APSIS_POOL_BUF;
static volatile uint32_t apsis_c1_probe[4];       /* blocks core 1 rendered, blocks core 0 did the job, timeouts, the job's worst (us) */

static ApsisSpaceState room_state;                /* the room's settings at the block's start, for core 1 */

static void job_strings(uint32_t frames)         /* core 1's (or core 0's, with one core) */
{
    apsis_space_target_room_wet(&space_target, &room_state);   /* the last block's room (one block late) */
    apsis_string_dsp_render(&string_dsp, buf_drone, frames);
    apsis_drift_dsp_render(&drift_dsp, buf_drift, frames);
    {
        COST_BEGIN(apsis_string_dsp_transient_physical_active(&string_transient_pool));
        apsis_string_transient_render(&string_transient_pool, buf_notes, frames);
        COST_END(0u);
    }
}

static void render_block(int32_t *out, uint32_t n)
{
    /* Dinghy enables IRQs before app_main(), so the ISR can run while static
     * state is being initialized. out is already zeroed and is stereo Q15.
     *
     * M4a deliberately renders from the state visible at block start, then
     * advances M3c structural time for that block. A state change occurring
     * inside a 256-frame callback can therefore sound at most one callback
     * late, never early. Sample-exact structural-to-audio handoff is Phase 5. */
    if (runtime_ready) {
        block_mark = fm1_ticks();
        sound_ticks = 0u;
#ifdef APSIS_STRESS
        plan_ticks[0] = plan_ticks[1] = plan_ticks[2] = 0u;
#endif
        apsis_space_target_begin_block(&space_target, n);
        if (apsis_string_motion_active || apsis_string_motion_mailbox.pending)
            apsis_p10_motion_prepare_block(n);
        apsis_string_dsp_update(&string_dsp, &apsis, &string_pitch_memory,
                                apsis_runtime.transport.running);
        if (apsis_string_motion_playback_valid)
            apsis_string_motion_dsp_override(&string_dsp, &apsis,
                                             &apsis_string_motion_playback_frame);
        ApsisRuntimeStringRhythmObservation rhythm_observation;
        uint8_t attr_ix;
        ApsisClockEventBatch clock_batch;
        uint8_t clock_ix;
        uint64_t clock_block_start = apsis_runtime.cursor_sample;
        uint8_t spiral_ix;
        uint64_t spiral_block_start;
        apsis_drift_dsp_update(&drift_dsp, &apsis,
                               apsis_runtime.transport.running);
        apsis_clock_collect_block(&apsis_runtime, &apsis, n, &clock_batch);
        for (clock_ix = 0u; clock_ix < clock_batch.count; ++clock_ix)
            (void)apsis_clock_dsp_trigger_event(&clock_dsp, &apsis,
                                                &clock_batch.event[clock_ix],
                                                clock_block_start);
        block_part(0u);
        /* the four first sources' gains as the one-core order committed them: before the planning, which may
         * move them (Mother, CHAIN) */
        const uint16_t send_string = apsis.string.send_q15, send_drift = apsis.drift.send_q15;
        const uint16_t send_clock = apsis.clock.send_q15;
        const uint16_t mix0 = apsis.mother.mix_q15[0], mix1 = apsis.mother.mix_q15[1], mix2 = apsis.mother.mix_q15[2];
        const uint8_t mute = apsis.mother.mute_mask;
        uint8_t spiral_rendered = 0u, bloom_rendered = 0u, c1_taken;
        room_state = apsis.space;
        c1_taken = (uint8_t)cpu1_run(job_strings, n);
        if (!c1_taken) {
            job_strings(n);
            apsis_c1_probe[1]++;
        }
        block_part(1u);
        {
            COST_BEGIN(clock_voices());
            apsis_clock_dsp_render(&clock_dsp, buf_clock, n);
            COST_END(3u);
        }
        block_part(3u);
        if (!spiral_audio_started && apsis.spiral.sound && apsis.spiral.route_count)
            spiral_audio_started = 1u;
        if (!bloom_audio_started &&
            apsis_runtime_bloom_occurrence_due(
                &apsis_runtime, apsis_runtime.cursor_sample + n))
            bloom_audio_started = 1u;
        if (bloom_audio_started) {
            spiral_block_start = apsis_runtime.cursor_sample;
            { PLAN_BEGIN();
            apsis_runtime_process_bloom_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, apsis_controls_apply_pending_item, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                spiral_audio_started ? &spiral_audio_runtime : 0,
                &bloom_audio_runtime, &rhythm_observation);
            PLAN_END(0); }
            if (spiral_audio_started) {
                { PLAN_BEGIN();
                for (spiral_ix = 0u; spiral_ix < spiral_audio_runtime.binding_count; ++spiral_ix)
                    (void)apsis_spiral_dsp_trigger_binding(
                        &spiral_dsp, &apsis, &spiral_audio_runtime.binding[spiral_ix],
                        spiral_block_start);
                PLAN_END(1); }
                if (spiral_dsp.hit_trigger_count)
        {
            uint32_t sound_t0 = fm1_ticks();
            COST_BEGIN(apsis_spiral_dsp_oscillator_active(&spiral_dsp));
            apsis_spiral_dsp_render(&spiral_dsp, buf_spiral, n);
            COST_END(1u);
            spiral_rendered = 1u;
            sound_ticks += fm1_ticks() - sound_t0;
        }
            }
            {
                uint8_t bloom_ix;
                { PLAN_BEGIN();
                for (bloom_ix = 0u; bloom_ix < bloom_audio_runtime.binding_count; ++bloom_ix)
                    (void)apsis_bloom_dsp_trigger_binding(
                        &bloom_dsp, &apsis, &bloom_audio_runtime.binding[bloom_ix],
                        spiral_block_start);
                PLAN_END(2); }
                if (bloom_dsp.trigger_count)
        {
            uint32_t sound_t0 = fm1_ticks();
            COST_BEGIN(apsis_bloom_dsp_oscillator_active(&bloom_dsp));
            apsis_bloom_dsp_render(&bloom_dsp, buf_bloom, n);
            COST_END(2u);
            bloom_rendered = 1u;
            sound_ticks += fm1_ticks() - sound_t0;
        }
            }
        } else if (spiral_audio_started) {
            spiral_block_start = apsis_runtime.cursor_sample;
            { PLAN_BEGIN();
            apsis_runtime_process_spiral_audio(
                &apsis_runtime, &apsis, &keep_bank, &chain_sequence,
                &mother_pending, &bloom_voicing_state, n, apsis_controls_apply_pending_item, 0,
                (string_attractor_runtime.length ||
                 apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR)
                    ? &string_attractor_runtime : 0,
                &spiral_audio_runtime, &rhythm_observation);
            PLAN_END(0); }
            { PLAN_BEGIN();
            for (spiral_ix = 0u; spiral_ix < spiral_audio_runtime.binding_count; ++spiral_ix)
                (void)apsis_spiral_dsp_trigger_binding(
                    &spiral_dsp, &apsis, &spiral_audio_runtime.binding[spiral_ix],
                    spiral_block_start);
            PLAN_END(1); }
            if (spiral_dsp.hit_trigger_count)
        {
            uint32_t sound_t0 = fm1_ticks();
            COST_BEGIN(apsis_spiral_dsp_oscillator_active(&spiral_dsp));
            apsis_spiral_dsp_render(&spiral_dsp, buf_spiral, n);
            COST_END(1u);
            spiral_rendered = 1u;
            sound_ticks += fm1_ticks() - sound_t0;
        }
        } else if (string_attractor_runtime.length ||
                   apsis.string.filter_mode == APSIS_STRING_FILTER_ATTRACTOR) {
            { PLAN_BEGIN();
            apsis_runtime_process_observed_attractor(&apsis_runtime, &apsis,
                                                     &keep_bank, &chain_sequence,
                                                     &mother_pending,
                                                     &bloom_voicing_state, n, apsis_controls_apply_pending_item, 0,
                                                     &string_attractor_runtime,
                                                     &rhythm_observation);
            PLAN_END(0); }
        } else {
            { PLAN_BEGIN();
            apsis_runtime_process_observed(&apsis_runtime, &apsis, &keep_bank,
                                           &chain_sequence, &mother_pending,
                                           &bloom_voicing_state, n, apsis_controls_apply_pending_item, 0,
                                           &rhythm_observation);
            PLAN_END(0); }
        }
        block_part_planning();
        if (c1_taken) {
            if (cpu1_wait() == 0) {
                apsis_c1_probe[0]++;
            } else {                                     /* core 1 late: held from now on; this block's String
                                                          * sources are lost (its buffers half written), not doubled */
                uint32_t k;
                for (k = 0u; k < n * 2u; ++k) buf_drone[k] = buf_drift[k] = buf_notes[k] = 0;
                apsis_c1_probe[2]++;
            }
        }
        block_part(2u);
        apsis_space_target_commit_buffer(&space_target, buf_drone, out, n, send_string, mix0, (uint8_t)(mute & 1u));
        apsis_space_target_commit_buffer(&space_target, buf_drift, out, n, send_drift, mix1, (uint8_t)(mute & 2u));
        apsis_space_target_commit_buffer(&space_target, buf_clock, out, n, send_clock, mix2, (uint8_t)(mute & 4u));
        apsis_space_target_commit_buffer(&space_target, buf_notes, out, n, send_string, mix0, (uint8_t)(mute & 1u));
        if (spiral_rendered)
            apsis_space_target_commit_buffer(&space_target, buf_spiral, out, n, apsis.spiral.send_q15,
                                             apsis.mother.mix_q15[3], (uint8_t)(apsis.mother.mute_mask & (1u << 3)));
        if (bloom_rendered)
            apsis_space_target_commit_buffer(&space_target, buf_bloom, out, n, apsis.bloom.send_q15,
                                             apsis.mother.mix_q15[4], (uint8_t)(apsis.mother.mute_mask & (1u << 4)));
        block_part(4u);
        for (attr_ix = 0u; attr_ix < string_attractor_runtime.binding_count; ++attr_ix)
            (void)apsis_string98_strum(&string_transient_pool, &apsis, &string_pitch_memory,
                                       &string_attractor_runtime.binding[attr_ix], apsis_runtime.cursor_sample);
        if (rhythm_observation.valid ||
            apsis_controls_performance.string_request ||
            apsis_controls_performance.bloom_request ||
            apsis_bloom_manual_pending_count(&apsis_bloom_manual_audio)) {
            if (rhythm_observation.valid)
                (void)apsis_string98_rhythm(&string_transient_pool, &apsis, &string_pitch_memory,
                                            &rhythm_observation.event);
            if (apsis_controls_performance.string_request) {
                uint8_t manual_side;
                if (apsis_controls_performance_take_string(
                        &apsis_controls_performance, apsis_runtime.cursor_sample,
                        &manual_side))
                    (void)apsis_string98_pluck(&string_transient_pool, &apsis, &string_pitch_memory,
                                               apsis_runtime.cursor_sample);   /* one string: no side */
            }
            if (apsis_controls_performance.bloom_request ||
                apsis_bloom_manual_pending_count(&apsis_bloom_manual_audio)) {
                ApsisControlsBloomRequest bloom_request;
                ApsisLatePendingGravity bloom_pending;
                uint64_t bloom_block_start = apsis_runtime.cursor_sample;
                uint8_t bloom_started = 0u;
                bloom_pending.active = apsis_runtime.gravity_pending.active;
                bloom_pending.degree = apsis_runtime.gravity_pending.degree;
                bloom_pending.ring = apsis_runtime.gravity_pending.ring;
                bloom_pending.target_sample = apsis_runtime.gravity_pending.target_sample;

                if (apsis_controls_performance_take_bloom(
                        &apsis_controls_performance, &bloom_request)) {
                    if (bloom_request.petal_mask) {
                        if (apsis_bloom_manual_trigger_petals(
                                &apsis_bloom_manual_audio, &bloom_dsp, &apsis,
                                &bloom_voicing_state, &bloom_pending,
                                bloom_request.petal_mask, bloom_request.grow_mask,
                                bloom_block_start))
                            bloom_started = 1u;
                    }
                    if (bloom_request.core && apsis.bloom.sound &&
                        apsis.bloom.level_q15) {
                        uint32_t walk_random = m3c_random(&apsis_runtime, 0x50394257u);
                        uint32_t pick_random = m3c_random(&apsis_runtime, 0x50394250u);
                        if (apsis_bloom_manual_trigger_core(
                                &apsis_bloom_manual_audio, &apsis,
                                &bloom_voicing_state, &bloom_pending,
                                bloom_block_start, walk_random, pick_random))
                            bloom_started = 1u;
                    }
                }
                /* Process after a new CORE is queued so its zero-offset first
                 * petal is already bound for the next callback. */
                if (apsis_bloom_manual_pending_count(&apsis_bloom_manual_audio) &&
                    apsis_bloom_manual_process(&apsis_bloom_manual_audio,
                                               &bloom_dsp, &apsis,
                                               &bloom_pending,
                                               bloom_block_start, n))
                    bloom_started = 1u;
                if (bloom_started)
                    bloom_audio_started = 1u;
            }
        }
        block_part(6u);
        apsis_space_target_room_add(&space_target, out, n);   /* the room rendered beside this block, from the last one */
        apsis_space_target_room_prepare(&space_target, n);   /* this block's sends, for the next block's room */
        if (apsis_string_motion_active)
            apsis_p10_motion_finish_block();
        block_part(7u);
        apsis_runtime_probe[0] = 0x41505349u; /* "APSI" */
        apsis_runtime_probe[1] = apsis_runtime.queue_high_water;
        apsis_runtime_probe[2] = apsis_runtime.queue_overflowed;
        apsis_runtime_probe[3] = (uint32_t)apsis_runtime.cursor_sample;
        apsis_runtime_probe[4] = (uint32_t)apsis_runtime.late_slot_overflowed | ((uint32_t)apsis_runtime.scheduler_starved << 1);
        apsis_runtime_probe[5] = apsis_runtime.late_trace_hash;
    }
}

/* ---- the audio interrupt: the render above, timed. 256 frames = 5.8 ms; docs/dinghy/RULES.md asks for under half. */
static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t t0 = fm1_ticks(), us;
    cpu1_screen_begin();                            /* core 1's first blocks: screened for misreads beside the render */
    render_block(out, n);
    cpu1_screen_end();
#ifndef DINGHY_HOST
    apsis_c1_probe[3] = cpu1_job_worst / FM1_TICKS_PER_US;
#endif
    us = (uint32_t)(fm1_ticks() - t0) / FM1_TICKS_PER_US;
    apsis_load_probe[0] = us;
    {
        uint32_t i;
        if (us > apsis_load_probe[1]) {
            apsis_load_probe[1] = us;
            for (i = 0u; i < BLOCK_PARTS; ++i) apsis_worst_probe[i] = block_part_us[i];
        }
        if (apsis_load_probe[2] < BOOT_BLOCKS) {
            if (us > apsis_boot_worst_probe) apsis_boot_worst_probe = us;
        } else for (i = 0u; i < BLOCK_PARTS; ++i)
            if (block_part_us[i] > apsis_part_max_probe[i]) {
                apsis_part_max_probe[i] = block_part_us[i];
                apsis_part_max_block_probe[i] = apsis_load_probe[2];
            }
        apsis_voice_probe[0] = string_transient_pool.transient_logical_high_water;
        apsis_voice_probe[1] = string_transient_pool.transient_physical_high_water;
        if (apsis_bloom_dsp_active(&bloom_dsp) > apsis_voice_probe[2]) apsis_voice_probe[2] = apsis_bloom_dsp_active(&bloom_dsp);
#ifdef APSIS_STRESS
        if (apsis_load_probe[2] >= BOOT_BLOCKS)
            for (i = 0u; i < 3u; ++i)
                if (plan_ticks[i] / FM1_TICKS_PER_US > apsis_plan_probe[i]) apsis_plan_probe[i] = plan_ticks[i] / FM1_TICKS_PER_US;
#endif
        if (apsis_load_probe[2] >= BOOT_BLOCKS) {    /* (the boot blocks have their own probe) */
            if (us > 4644u) apsis_over_probe[0]++;
            if (us >= 5805u) apsis_over_probe[1]++;
        }
    }
    apsis_load_probe[2]++;
    apsis_load_probe[3] += us;
}

#ifdef APSIS_HOST_TIMING
#include <stdio.h>
__attribute__((destructor)) static void host_timing_report(void)
{
    static const char *const name[BLOCK_PARTS] = {"updates+Clock triggers", "String drone", "Drift", "Clock",
                                                  "String notes", "planning", "triggers", "room+Motion",
                                                  "Spiral+Bloom sound"};
    uint32_t i, sum = 0u;
    printf("host load: %u blocks, avg %u us, worst %u us\n", apsis_load_probe[2],
           apsis_load_probe[2] ? apsis_load_probe[3] / apsis_load_probe[2] : 0u, apsis_load_probe[1]);
    for (i = 0u; i < BLOCK_PARTS; ++i) {
        sum += apsis_part_max_probe[i];
        printf("  %-24s own worst %6u us at %5.2f s\n", name[i], apsis_part_max_probe[i],
               apsis_part_max_block_probe[i] * 256.0 / 44100.0);
    }
    printf("  bound %u us (after the first %u blocks; their worst %u us)\n", sum, BOOT_BLOCKS, apsis_boot_worst_probe);
}
#endif

/* ---- the screen: the current surface, drawn again and again in 8-row strips, each strip only once the
 * previous one's DMA is done, so the main loop never waits on the LCD and every change shows within a frame */
#define STRIP_ROWS 8u

static uint16_t draw_y;
static uint32_t frame_count;

#if defined(APSIS_STRESS) && !defined(APSIS_LOADMETER)
#define APSIS_LOADMETER 1
#endif
#ifdef APSIS_LOADMETER
/* a diagnostic build: the audio interrupt's load in the top 20 rows, for a photo of a real FM-1.
 * "A44 W69 80:0 100:0" = average and worst % of the 5805 us block, blocks over 80 % and over 100 % */
#define METER_ROWS 20u

static char *meter_num(char *p, uint32_t v)
{
    char d[10];
    uint32_t n = 0u;
    do {
        d[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v && n < sizeof d);
    while (n) *p++ = d[--n];
    return p;
}

static void meter_draw(void)
{
    char line[48], *p = line;
    uint32_t blocks = apsis_load_probe[2];
    uint32_t avg = blocks ? (uint32_t)(((uint64_t)apsis_load_probe[3] * 100u / 5805u) / blocks) : 0u;
    if (draw_y >= METER_ROWS)
        return;
    cv_oy = -(int32_t)draw_y;
    cv_rect(0, 0, (int32_t)APSIS_VISUAL_WIDTH, (int32_t)METER_ROWS, C_BLACK);
    *p++ = 'A';
    p = meter_num(p, avg);
    *p++ = ' ';
    *p++ = 'W';
    p = meter_num(p, apsis_load_probe[1] * 100u / 5805u);
    *p++ = ' ';
    p = meter_num(p, 80u);
    *p++ = ':';
    p = meter_num(p, apsis_over_probe[0]);
    *p++ = ' ';
    p = meter_num(p, 100u);
    *p++ = ':';
    p = meter_num(p, apsis_over_probe[1]);
    *p = 0;
    (void)cv_text(4, 2, &FONT_S, line, C_WHITE);
    cv_oy = 0;
}
#endif

static void draw_step(void)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    uint32_t i;
    uint16_t rows;

    if (lcd_busy && !fm1_lcd_done())
        return;
    rows = (uint16_t)(APSIS_VISUAL_HEIGHT - draw_y);
    if (rows > STRIP_ROWS)
        rows = STRIP_ROWS;

    context.surface = (ApsisVisualSurface)surface;
    context.state = &apsis;
    context.runtime = &apsis_runtime;
    context.animation_tick = frame_count;

    cv_begin(APSIS_VISUAL_WIDTH, rows, C_BLACK);
    apsis_visual_bind(&raster, cv_px, APSIS_VISUAL_WIDTH, rows, APSIS_VISUAL_WIDTH, 0, (int16_t)draw_y);
    apsis_visual_render_p8c(&raster, &context, &keep_bank, &chain_sequence);
    for (i = 0u; i < (uint32_t)APSIS_VISUAL_WIDTH * rows; ++i)
        cv_px[i] = swap16(cv_px[i]);
#ifdef APSIS_LOADMETER
    meter_draw();
#endif
    cv_blit(0u, draw_y);
    draw_y = (uint16_t)(draw_y + rows);
    if (draw_y >= APSIS_VISUAL_HEIGHT) {
        draw_y = 0u;
        frame_count++;
    }
}

static void draw_restart(void)
{
    draw_y = 0u;
}

static void leds(void)
{
    static const uint8_t lit_on[SURF_COUNT] = {
        BTN_HOME, BTN_ENV, BTN_LFO, BTN_SEQ, BTN_ARP, BTN_FX, BTN_GLO, BTN_EDIT, BTN_SAVE, BTN_FX
    };
    static uint8_t shown = 0xFFu;
    uint32_t i;
    if (shown == surface)
        return;
    shown = surface;
    for (i = 0; i < APSIS_FM1_BUTTON_COUNT; ++i)
        fm1_led_key(i, 0);
    fm1_led_key(lit_on[surface], 1);
}

/* ---- the panel: one input frame per main-loop pass */
static uint32_t notes_seen;

static uint8_t input_take(ApsisFm1InputFrame *in)
{
    uint32_t released = 0u, e;
    uint8_t active;
    uint8_t *z = (uint8_t *)in;
    uint32_t n = (uint32_t)sizeof(*in);
    while (n--) *z++ = 0u;

    in->buttons_pressed = fm1_input_edges(&released) & ((1u << APSIS_FM1_BUTTON_COUNT) - 1u);
    in->buttons_released = released & ((1u << APSIS_FM1_BUTTON_COUNT) - 1u);
    in->buttons_held = fm1_in.buttons;
    in->notes_held = fm1_in.notes;
    in->notes_pressed = fm1_input_note_edges() | (in->notes_held & ~notes_seen);   /* (host mode has no edges) */
    active = (uint8_t)(in->buttons_pressed || in->buttons_released || in->notes_pressed ||
                       in->notes_held != notes_seen);
    notes_seen = in->notes_held;
    for (e = 0u; e < APSIS_FM1_ENCODER_COUNT; ++e) {
        in->encoder_steps[e] = (int16_t)fm1_enc_take(e);
        if (in->encoder_steps[e])
            active = 1u;
    }
    return active;
}

static void apply_transport(const ApsisFm1InputFrame *in)
{
    if (apsis_controls_global_action(in) != APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE)
        return;
    if (apsis_runtime.transport.running)
        apsis_runtime_stop_attractor(&apsis_runtime, &string_attractor_runtime);
    else
        apsis_runtime_start_attractor(&apsis_runtime, &apsis, &string_attractor_runtime);
}

/* Routes one input frame through the editors and the shared controls dispatch. All musical results go through
 * the shared runtime (Gravity, MOTHER, CHAIN) or the audio mailboxes (String pluck, Bloom, Motion); nothing here
 * renders sound. Returns 1 when the frame was consumed and must not also navigate. */
static uint8_t dispatch(ApsisFm1InputFrame *in, uint8_t current)
{
    ApsisControlsDispatchResult result = {0};
    ApsisStringMotionControlResult motion_control;
    ApsisStringAttractorControlResult attractor_control;
    ApsisSpiralAttractorControlResult spiral_attractor_control;
    ApsisGravityPointContextResult gravity_point_context;
    ApsisGravityPointControlResult gravity_point_control = {0};
    ApsisFm1InputFrame gravity_dispatch_input;
    const ApsisFm1InputFrame *dispatch_input = in;
    uint8_t attractor_editor_frame = 0u;
    uint8_t spiral_attractor_editor_frame = 0u;
    uint8_t motion_control_valid = 0u;
    uint8_t motion_capture = 0u;
    uint8_t motion_manual_valid = 0u;
    uint8_t motion_manual_active = 0u;
    int32_t last_abs_step;

    (void)apsis_controls_string_attractor_context(&apsis_string_attractor_editor, current, in, 0);
    if (current == SURF_STRING && apsis_string_attractor_editor.active)
        attractor_editor_frame = apsis_controls_string_attractor_edit(&apsis.string, 1u, in, &attractor_control);
    (void)apsis_controls_spiral_attractor_context(&apsis_spiral_attractor_editor, current, in, 0);
    if (current == SURF_SPIRAL && apsis_spiral_attractor_editor.active)
        spiral_attractor_editor_frame = apsis_controls_spiral_attractor_edit(
            &apsis_spiral_attractor_editor, &apsis.spiral, in, &spiral_attractor_control);
    (void)apsis_controls_gravity_point_context(&apsis_gravity_point_editor, current, in, &gravity_point_context);
    if (current == SURF_GRAVITY && apsis_gravity_point_editor.page == APSIS_GRAVITY_PAGE_POINT) {
        (void)apsis_controls_gravity_point_edit(&apsis_gravity_point_editor, &apsis.gravity, in,
                                                &gravity_point_control);
        if (gravity_point_control.suppress_degree_request) {
            gravity_dispatch_input = *in;
            gravity_dispatch_input.notes_pressed &= ~0xffu;
            dispatch_input = &gravity_dispatch_input;
        }
    }
    if (current == SURF_STRING && !attractor_editor_frame) {
        motion_control_valid = apsis_controls_string_motion(
            &apsis, apsis_string_motion_runtime.record_target, in, &motion_control);
        if (motion_control.suppress_string_bank)
            in->buttons_pressed &= ~(1u << APSIS_FM1_BTN_OCT_DOWN);
        motion_capture = (uint8_t)(in->notes_pressed != 0u || in->encoder_steps[APSIS_FM1_ENC_SELECT] != 0);
        /* every STRING frame publishes the held keys: manual override ends on the last key-up */
        motion_manual_valid = 1u;
        motion_manual_active = in->notes_held ? 1u : 0u;
    }
    last_abs_step = apsis_runtime.transport.absolute_pulse
        ? (int32_t)(apsis_runtime.transport.absolute_pulse - 1u) : -1;
    if (!attractor_editor_frame && !spiral_attractor_editor_frame)
        apsis_controls_dispatch(&apsis_controls_session, &apsis, &keep_bank, &chain_sequence, &mother_pending,
                                current, dispatch_input, last_abs_step, &result);

    if (current == SURF_STRING && (motion_control_valid || motion_capture || motion_manual_valid)) {
        fm1_irq_off();                          /* a one-word read-modify-write shared with app_audio */
        apsis_string_motion_mailbox_publish(&apsis_string_motion_mailbox,
                                            motion_control_valid ? &motion_control : 0,
                                            motion_capture, motion_manual_valid, motion_manual_active);
        fm1_irq_on();
    }

    if (result.string_pluck)
        apsis_controls_performance_queue_string(&apsis_controls_performance, result.string_side);
    if (result.bloom_core || result.bloom_petal_mask) {
        uint8_t bloom_grow_mask = (in->buttons_held & (1u << BTN_REC)) ? result.bloom_petal_mask : 0u;
        fm1_irq_off();                          /* a 16-bit read-modify-write shared with app_audio */
        apsis_controls_performance_queue_bloom(&apsis_controls_performance, result.bloom_core,
                                               result.bloom_petal_mask, bloom_grow_mask);
        fm1_irq_on();
    }

    if (result.gravity_request)
        (void)apsis_runtime_request_gravity(&apsis_runtime, &apsis, &mother_pending, result.gravity_degree,
                                            result.gravity_ring, apsis_runtime.cursor_sample, 0u);

    if (result.chain_transport == APSIS_CHAIN_TRANSPORT_START)
        apsis_runtime_chain_start(&apsis_runtime, &apsis, &chain_sequence);
    else if (result.chain_transport == APSIS_CHAIN_TRANSPORT_STOP)
        apsis_runtime_chain_stop(&apsis_runtime, &chain_sequence);

    /* removeChainCell() in the browser applies the cell that now holds the active position at once */
    if (result.chain_apply_current && chain_sequence.count && chain_sequence.pos < chain_sequence.count)
        (void)runtime_apply_chain_cell(&apsis_runtime, &apsis, &keep_bank, &chain_sequence, chain_sequence.pos,
                                       apsis_runtime.cursor_sample);

    if (result.project_dirty)
        apsis_controls_project_dirty_mark(&apsis_project_dirty, apsis_runtime.cursor_sample);

    if (result.chain_transport != APSIS_CHAIN_TRANSPORT_NONE)
        return 1u;                              /* SEL+PLAY belongs to CHAIN, never the global transport */
    return result.suppress_navigation;
}

/* the surface buttons (frozen P9 interaction contract); SEL+FX opens SPACE */
static uint8_t navigate(ApsisFm1InputFrame *in, uint8_t current)
{
    static const struct { uint8_t button, surface; } NAV[] = {
        {BTN_HOME, SURF_MOTHER}, {BTN_ENV, SURF_STRING}, {BTN_LFO, SURF_DRIFT}, {BTN_SEQ, SURF_CLOCK},
        {BTN_ARP, SURF_SPIRAL}, {BTN_FX, SURF_BLOOM}, {BTN_GLO, SURF_GRAVITY}, {BTN_EDIT, SURF_CHAIN},
        {BTN_SAVE, SURF_KEEP},
    };
    uint32_t i, pressed = in->buttons_pressed;

    if (dispatch(in, current))
        return current;
    for (i = 0; i < sizeof NAV / sizeof NAV[0]; ++i) {
        if (!(pressed & (1u << NAV[i].button)))
            continue;
        if (NAV[i].button == BTN_FX && (in->buttons_held & (1u << BTN_SEL)))
            return SURF_SPACE;
        return NAV[i].surface;
    }
    if (pressed & (1u << BTN_PLAY))
        apply_transport(in);
    return current;
}

/* ---- persistence: saves run here, with the transport stopped and the audio quiesced (a sector erase holds the
 * interrupts off for ~50 ms anyway) */
static void service_save(void)
{
    uint32_t i;
    if (apsis_controls_project_dirty_take_due(&apsis_project_dirty, apsis_runtime.cursor_sample,
                                              apsis_runtime.transport.running))
        apsis_project_fm1_request_save(&project_store);
    if (!project_store.save_pending || apsis_runtime.transport.running)
        return;
    runtime_ready = 0u;
#ifndef DINGHY_HOST
    fm1_audio_stop();
#endif
    (void)apsis_project_fm1_service_save(&project_store, &keep_bank, &chain_sequence, &apsis.chain);
#ifndef DINGHY_HOST
    for (i = 0u; i < (2u * HALF_WORDS); ++i)
        abuf[i] = 0;
    fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);
#else
    (void)i;
#endif
    runtime_ready = 1u;
}

static void editors_follow(uint8_t next)
{
    if (next != SURF_STRING)
        apsis_controls_string_attractor_editor_reset(&apsis_string_attractor_editor);
    if (next != SURF_SPIRAL)
        apsis_controls_spiral_attractor_editor_reset(&apsis_spiral_attractor_editor);
    if (next != SURF_GRAVITY)
        apsis_controls_gravity_point_editor_reset(&apsis_gravity_point_editor);
}

#ifdef APSIS_BENCH
/* a diagnostic build: what one String partial costs on each of its paths, at boot before the audio starts, in
 * tenths of a CPU cycle (240 MHz) per partial per sample: [0] the block's fixed cost (us), [1] the decay mix
 * (2 triangle + LP, 2 sine), [2] all sine, [3] all triangle + LP, [4] the attack and stagger, [5] the steal fade */
static volatile uint32_t apsis_bench_probe[6];
static int32_t bench_out[512];

static uint32_t bench_render(uint32_t blocks)
{
    uint32_t t0 = fm1_ticks(), b, i;
    for (b = 0u; b < blocks; ++b) {
        apsis_string_transient_render(&string_transient_pool, bench_out, 256u);
        for (i = 0u; i < 512u; ++i) bench_out[i] = 0;
    }
    return fm1_ticks() - t0;
}

static uint32_t bench_tenths(uint32_t ticks, uint32_t fixed, uint32_t blocks, uint32_t partials)
{
    ticks = ticks > fixed ? ticks - fixed : 0u;
    return (uint32_t)((uint64_t)ticks * 100u / ((uint64_t)blocks * 256u * partials));   /* 10 cycles a tick */
}

static void bench_pluck(ApsisState *s)
{
    apsis_string_transient_pool_reset(&string_transient_pool);
    (void)apsis_string_dsp_trigger_classic(&string_transient_pool, s, 220000u, APSIS_Q15_MILLI(500), 3000u, 0, 0u,
                                           0u);
}

static void bench_set(uint8_t waveform, uint8_t lp)
{
    uint32_t i;
    for (i = 0u; i < 4u; ++i) {
        string_transient_pool.transient[i].waveform = waveform;
        if (!lp) string_transient_pool.transient[i].lowpass_alpha_q15 = 0u;
        else if (!string_transient_pool.transient[i].lowpass_alpha_q15)
            string_transient_pool.transient[i].lowpass_alpha_q15 = 12000u;
    }
}

static void bench_run(void)
{
    ApsisState s;
    uint32_t fixed, i;
    apsis_state_default(&s);
    s.string.foundation = 0u;
    apsis_string_transient_pool_reset(&string_transient_pool);
    fixed = bench_render(16u);
    apsis_bench_probe[0] = fixed / 16u / FM1_TICKS_PER_US;
    bench_pluck(&s);                                  /* 4 partials: 2 triangle + LP, 2 sine */
    apsis_bench_probe[4] = bench_tenths(bench_render(5u), fixed * 5u / 16u, 5u, 4u);
    apsis_bench_probe[1] = bench_tenths(bench_render(16u), fixed, 16u, 4u);
    bench_set(APSIS_STRING_DSP_WAVE_SINE, 0u);
    apsis_bench_probe[2] = bench_tenths(bench_render(16u), fixed, 16u, 4u);
    bench_set(APSIS_STRING_DSP_WAVE_TRIANGLE, 1u);
    apsis_bench_probe[3] = bench_tenths(bench_render(16u), fixed, 16u, 4u);
    for (i = 0u; i < 4u; ++i) {
        string_transient_pool.transient[i].stealing = 1u;
        string_transient_pool.transient[i].steal_left_samples = 4630u;
        string_transient_pool.transient[i].steal_gain_q15 = APSIS_Q15_ONE;
    }
    apsis_bench_probe[5] = bench_tenths(bench_render(16u), fixed, 16u, 4u);
    apsis_string_transient_pool_reset(&string_transient_pool);
}
#endif

static void app_main(void)
{
    ApsisFm1InputFrame in;
    uint32_t i;

    runtime_ready = 0u;
#ifdef APSIS_BENCH
    bench_run();
#endif
    for (i = 0u; i < sizeof apsis_runtime_probe / sizeof apsis_runtime_probe[0]; ++i)
        apsis_runtime_probe[i] = 0u;
    apsis_runtime_probe[0] = 0x41505349u;      /* "APSI" */
#ifdef APSIS_STRESS
    apsis_state_stress(&apsis);
#elif defined(APSIS_DEMO)
    apsis_state_demo(&apsis);
#else
    apsis_state_default(&apsis);
#endif
    apsis_string_pitch_memory_reset(&string_pitch_memory);
    apsis_string_dsp_reset(&string_dsp);
    apsis_bloom_voicing_reset(&bloom_voicing_state);
    apsis_keep_bank_reset(&keep_bank);
    apsis_chain_sequence_reset(&chain_sequence);
    apsis_project_fm1_reset(&project_store);
    (void)apsis_project_fm1_load(&project_store, &keep_bank, &chain_sequence, &apsis.chain);
    apsis_mother_pending_reset(&mother_pending);
    apsis_controls_session_reset(&apsis_controls_session);
    apsis_controls_project_dirty_reset(&apsis_project_dirty);
    apsis_controls_performance_reset(&apsis_controls_performance);
    apsis_bloom_manual_audio_reset(&apsis_bloom_manual_audio);
    apsis_string_motion_store.left.count = 0u;
    apsis_string_motion_store.right.count = 0u;
    apsis_string_motion_reset(0, &apsis_string_motion_runtime);
    apsis_string_motion_mailbox_reset(&apsis_string_motion_mailbox);
    apsis_string_motion_active = 0u;
    apsis_string_motion_playback_valid = 0u;
    apsis_controls_string_attractor_editor_reset(&apsis_string_attractor_editor);
    apsis_controls_spiral_attractor_editor_reset(&apsis_spiral_attractor_editor);
    apsis_controls_gravity_point_editor_reset(&apsis_gravity_point_editor);
    apsis_runtime_reset(&apsis_runtime, FS, 0u, 0x41505349u);
    apsis_runtime_start(&apsis_runtime);
    (void)apsis_space_target_bind(&space_target);
    runtime_ready = 1u;

    for (;;) {
        uint8_t next = surface;
        platform_poll();
        if (input_take(&in))
            next = navigate(&in, surface);
        if (next != surface) {
            editors_follow(next);
            surface = next;
            draw_restart();
        }
        service_save();
        leds();
        draw_step();
    }
}
