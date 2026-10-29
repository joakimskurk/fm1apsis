#include "apsis_state.h"
#include "apsis_gravity.h"

static void apsis_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

void apsis_state_default(ApsisState *s)
{
    uint32_t i;
    static const uint16_t spiral_energy[APSIS_ATTRACTOR_STEPS] = {
        APSIS_Q15_MILLI(820), APSIS_Q15_MILLI(340), APSIS_Q15_MILLI(680), APSIS_Q15_MILLI(930),
        APSIS_Q15_MILLI(280), APSIS_Q15_MILLI(760), APSIS_Q15_MILLI(420), APSIS_Q15_MILLI(220),
        APSIS_Q15_MILLI(880), APSIS_Q15_MILLI(310), APSIS_Q15_MILLI(640), APSIS_Q15_MILLI(790),
        APSIS_Q15_MILLI(270), APSIS_Q15_MILLI(710), APSIS_Q15_MILLI(390), APSIS_Q15_MILLI(240),
        APSIS_Q15_MILLI(820), APSIS_Q15_MILLI(340), APSIS_Q15_MILLI(680), APSIS_Q15_MILLI(930),
        APSIS_Q15_MILLI(280), APSIS_Q15_MILLI(760), APSIS_Q15_MILLI(420), APSIS_Q15_MILLI(220)
    };

    if (!s) return;
    apsis_zero(s, (uint32_t)sizeof(*s));
    s->version = APSIS_STATE_VERSION;

    s->string.split_q15 = APSIS_Q15_MILLI(530);
    s->string.left_split_q15 = APSIS_Q15_MILLI(470);
    s->string.right_split_q15 = APSIS_Q15_MILLI(530);
    s->string.surface_mode = APSIS_STRING_DUAL;
    s->string.linked = 1u;
    s->string.active_side = APSIS_SIDE_RIGHT;
    s->string.left_anchor = APSIS_ANCHOR_ROOT;
    s->string.right_anchor = APSIS_ANCHOR_FIFTH;
    s->string.left_follow_gravity = 1u;
    s->string.right_follow_gravity = 1u;
    s->string.left_gravity_degree = 0;
    s->string.right_gravity_degree = 0;
    s->string.left_level_q15 = APSIS_Q15_MILLI(720);
    s->string.right_level_q15 = APSIS_Q15_MILLI(680);
    s->string.resonance_q15 = APSIS_Q15_MILLI(660);
    s->string.brightness_q15 = APSIS_Q15_MILLI(540);
    s->string.quantize = 1u;
    s->string.harmony = APSIS_HARMONY_SCALE;
    s->string.span_q15 = APSIS_Q15_ONE;
    s->string.range = 7u;
    s->string.character = APSIS_STRING_CHARACTER_CLASSIC;
    s->string.foundation = 1u;
    s->string.foundation_level_q15 = APSIS_Q15_MILLI(520);
    s->string.foundation_octave = -1;
    s->string.rhythm = APSIS_STRING_RHYTHM_OFF;
    s->string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s->string.rhythm_every = 2u;
    s->string.rhythm_mask = 0x00000249u; /* 0,3,6,9 */
    s->string.rhythm_level_q15 = APSIS_Q15_MILLI(320);
    s->string.filter_mode = APSIS_STRING_FILTER_OFF;
    s->string.filter_cutoff_hz = 6200u;
    s->string.filter_depth_q15 = APSIS_Q15_MILLI(780);
    s->string.filter_q_q15 = APSIS_Q15_MILLI(280);
    s->string.filter_decay_q15 = APSIS_Q15_MILLI(340);
    s->string.filter_slew_q15 = APSIS_Q15_MILLI(55);
    s->string.filter_mask = 0x00000249u; /* 0,3,6,9 */
    s->string.filter_attractor_every = APSIS_STRING_ATTRACTOR_1;
    s->string.filter_attractor_length = 8u;
    s->string.attractor_target = APSIS_STRING_ATTRACTOR_ALTERNATE;
    s->string.filter_attractor_lock_q15 = APSIS_Q15_MILLI(860);
    s->string.filter_attractor_strum_q15 = APSIS_Q15_MILLI(620);
    s->string.filter_attractor_strum_level_q15 = APSIS_Q15_MILLI(780);
    s->string.filter_attractor_gates = 0x9535u;
    s->string.filter_attractor_seed = 317u;
    s->string.pulse_diag = APSIS_STRING_PULSE_FULL;
    s->string.motion_loop_length = 1u;
    s->string.motion_loop_play = 0u;
    s->string.motion_loop_pitch_continuous = 0u;
    s->string.motion_level_q15 = APSIS_Q15_MILLI(180);
    s->string.glide_q15 = APSIS_Q15_MILLI(55);
    s->string.send_q15 = APSIS_Q15_MILLI(720);
    s->string.sound = 1u;
    s->string.level_q15 = APSIS_Q15_MILLI(800);

    s->drift.amount_q15 = APSIS_Q15_MILLI(420);
    s->drift.depth_q15 = APSIS_Q15_MILLI(720);
    s->drift.width_q15 = APSIS_Q15_MILLI(640);
    s->drift.send_q15 = APSIS_Q15_MILLI(200);
    s->drift.sound = 1u;
    s->drift.level_q15 = APSIS_Q15_MILLI(820);

    s->clock.n = 12u;
    s->clock.bounds[0] = 3u;
    s->clock.bounds[1] = 6u;
    s->clock.bounds[2] = 9u;
    s->clock.main_mask = 0x0FFFu;
    s->clock.inner_mask = 0u;
    s->clock.snap_mask = 0u;
    s->clock.bpm = 72u;
    s->clock.division_num = 1u;
    s->clock.division_den = 2u;
    s->clock.swing_q15 = 0u;
    s->clock.ghost_q15 = 0u;
    s->clock.step_one_resync = 0u;
    s->clock.send_q15 = APSIS_Q15_MILLI(60);
    s->clock.sound = 1u;
    s->clock.level_q15 = APSIS_Q15_MILLI(780);
    s->clock.voice = APSIS_CLOCK_STONE;
    s->clock.pitch = 1;
    s->clock.inner_level_q15 = APSIS_Q15_MILLI(660);
    s->clock.snap_level_q15 = APSIS_Q15_MILLI(620);
    s->clock.snap_tone_hz = 4200u;

    s->spiral.route_count = 0u;
    s->spiral.rate = 1u;
    s->spiral.note_length_q15 = APSIS_Q15_MILLI(820);
    s->spiral.register_oct = 0;
    s->spiral.voice = APSIS_SPIRAL_THREAD;
    s->spiral.follow_frame = 1u;
    s->spiral.attractor_rate = 2u;
    s->spiral.attractor_length = 16u;
    s->spiral.attractor_lock_q15 = APSIS_Q15_MILLI(860);
    s->spiral.attractor_density_q15 = APSIS_Q15_MILLI(580);
    s->spiral.attractor_seed = 911u;
    s->spiral.attractor_gates = 0x002D2D2Du;
    for (i = 0; i < APSIS_ATTRACTOR_STEPS; ++i) s->spiral.attractor_energy_q15[i] = spiral_energy[i];
    s->spiral.send_q15 = APSIS_Q15_MILLI(620);
    s->spiral.sound = 1u;
    s->spiral.level_q15 = APSIS_Q15_MILLI(780);

    s->bloom.cycles = 2u;
    s->bloom.rate = 1u;
    s->bloom.trigger_step = 0u;
    s->bloom.delay_ms = 60u;
    s->bloom.voice = APSIS_BLOOM_VEIL;
    s->bloom.depth = APSIS_BLOOM_DEPTH_FOLLOW;
    s->bloom.voicing_mode = APSIS_BLOOM_VOICING_FOLLOW;
    s->bloom.inversion_follow = 1u;
    s->bloom.inversion = 0u;
    s->bloom.register_oct = 0;
    s->bloom.voicing_spread_q15 = APSIS_Q15_MILLI(480);
    s->bloom.length_ms = 1900u;
    s->bloom.attack_ms = 18u;
    s->bloom.unfurl_ms = 420u;
    s->bloom.width_q15 = APSIS_Q15_MILLI(540);
    s->bloom.brightness_q15 = APSIS_Q15_MILLI(720);
    s->bloom.tuning_mode = APSIS_BLOOM_TUNING_FOLLOW;
    s->bloom.tuning_reference = APSIS_BLOOM_REF_CHORD;
    s->bloom.tuning_amount_q15 = APSIS_Q15_ONE;
    s->bloom.send_q15 = APSIS_Q15_MILLI(580);
    s->bloom.sound = 1u;
    s->bloom.level_q15 = APSIS_Q15_MILLI(580);

    s->temper.mode = APSIS_TUNING_EQUAL;
    s->temper.strength_q15 = APSIS_Q15_ONE;
    s->temper.send_q15 = APSIS_Q15_MILLI(420);
    s->temper.sound = 1u;
    s->temper.level_q15 = APSIS_Q15_MILLI(580);

    s->gravity.tonic = 0u;
    s->gravity.scale = APSIS_SCALE_DORIAN;
    s->gravity.degree = 0u;
    s->gravity.voicing = APSIS_GRAVITY_COLOUR;
    s->gravity.voices = 4u;
    s->gravity.inversion = 0u;
    s->gravity.scale_lock = 1u;
    s->gravity.change_timing = APSIS_CHANGE_PULSE;
    s->gravity.change_grid = 2u;
    s->gravity.ring_play = 1u;
    s->gravity.ring_level_q15 = APSIS_Q15_MILLI(720);
    s->gravity.selected_ring = APSIS_GRAVITY_ANCHOR;
    s->gravity.selected_point_degree = 0u;
    for (i = 0u; i < APSIS_GRAVITY_POINTS; ++i) {
        uint8_t degree = (uint8_t)(i / APSIS_GRAVITY_RINGS);
        uint8_t ring = (uint8_t)(i % APSIS_GRAVITY_RINGS);
        s->gravity.points[i] = apsis_gravity_point_default(degree, ring);
    }

    s->space.mix_q15 = APSIS_Q15_MILLI(480);
    s->space.decay_ms = 8400u;
    s->space.predelay_ms = 72u;
    s->space.damping_hz = 6200u;
    s->space.echo_time_ms = 362u;
    s->space.echo_feedback_q15 = APSIS_Q15_MILLI(180);

    s->mother.sync = 0u;
    s->mother.horizon = 8u;
    for (i = 0; i < 5u; ++i) s->mother.mix_q15[i] = APSIS_Q15_ONE;
    s->mother.mute_mask = 0u;

    s->chain.meter_lock = 1u;
    s->chain.start_sync = APSIS_CHAIN_SYNC_ONE;
    s->chain.loop_sync = APSIS_CHAIN_SYNC_ONE;
    s->chain.default_cycles = 4u;
    s->chain.default_fine_pulses = 0u;
    s->chain.default_morph_quarters = 0u;
    s->chain.scope_key_scale = 1u;
    s->chain.scope_chord = 0u;
    s->chain.scope_tension = 0u;
    s->chain.scope_drift = 0u;
    s->chain.scope_clock = 0u;
    s->chain.scope_trace = 0u;
    s->chain.scope_tuning = 0u;
}
