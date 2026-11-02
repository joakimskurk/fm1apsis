/* The finite String notes against Study 98 (old_index.html): stringPluck / keyboardStringStrum, stringRhythm
 * (bow, pizz, motion) and turingStrum, each a classicStringNote of 3-4 partials (+ a root shadow group). */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string98.h"

static uint32_t ms(uint32_t m) { return (44100u * m + 500u) / 1000u; }

static uint16_t count_active(const ApsisStringTransientPool *p)
{
    uint16_t i, n = 0u;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) n = (uint16_t)(n + p->transient[i].active);
    return n;
}

static const ApsisStringTransientVoice *voice(const ApsisStringTransientPool *p, uint16_t k)
{
    uint16_t i;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (p->transient[i].active && k-- == 0u) return &p->transient[i];
    return 0;
}

static void setup(ApsisState *s, ApsisStringPitchMemory *m, ApsisStringTransientPool *p)
{
    apsis_state_default(s);
    apsis_string_pitch_memory_reset(m);
    apsis_string_transient_pool_reset(p);
}

static void test_pluck(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringTransientPool p;
    const ApsisStringTransientVoice *f;
    setup(&s, &m, &p);
    /* one string: classicStringNote(tension, .082 * level, 1.05 s, pan, rootGain .28): 4 partials + 4 root */
    assert(s.string.surface_mode == APSIS_STRING_CLASSIC && s.string.foundation);
    assert(apsis_string98_pluck(&p, &s, &m, 10000u) == 8u);
    assert(count_active(&p) == 8u);
    f = voice(&p, 0u);
    assert(f && f->duration_samples == ms(1050u));
    assert(f->waveform == APSIS_STRING_DSP_WAVE_TRIANGLE);
    /* split .53 -> pan (.53 - .5) * 1.24 = +.037: the fundamental leans right */
    assert(f->mix_right_q15 >= f->mix_left_q15);
    /* the 35 ms guard */
    assert(apsis_string98_pluck(&p, &s, &m, 10000u + ms(30u)) == 0u);
    assert(apsis_string98_pluck(&p, &s, &m, 10000u + ms(36u)) == 8u);
    /* no foundation: no root shadow */
    setup(&s, &m, &p);
    s.string.foundation = 0u;
    assert(apsis_string98_pluck(&p, &s, &m, 0u) == 4u);
    /* sound off or level 0: nothing */
    setup(&s, &m, &p);
    s.string.sound = 0u;
    assert(apsis_string98_pluck(&p, &s, &m, 0u) == 0u);
    puts("PASS: Study-98 pluck: 4 partials + root shadow, 1.05 s, pan by position, 35 ms guard");
}

static void test_rhythm(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringTransientPool p;
    ApsisTransportEvent e;
    memset(&e, 0, sizeof e);
    e.type = APSIS_EVENT_STRING_RHYTHM;
    e.absolute_pulse = 4u;

    setup(&s, &m, &p);
    s.string.rhythm_level_q15 = APSIS_Q15_ONE;
    e.sub_index = APSIS_STRING_RHYTHM_BOW;     /* classicStringNote(tension, lvl, .70): no root */
    assert(apsis_string98_rhythm(&p, &s, &m, &e) == 4u);
    assert(voice(&p, 0u)->duration_samples == ms(700u));

    setup(&s, &m, &p);
    s.string.rhythm_level_q15 = APSIS_Q15_ONE;
    e.sub_index = APSIS_STRING_RHYTHM_PIZZ;    /* pizz: 3 partials, lvl * 1.15, .34 s, rootGain .18 */
    assert(apsis_string98_rhythm(&p, &s, &m, &e) == 6u);
    assert(voice(&p, 0u)->duration_samples == ms(340u));

    setup(&s, &m, &p);
    s.string.rhythm_level_q15 = APSIS_Q15_ONE;
    e.sub_index = APSIS_STRING_RHYTHM_MOTION;  /* a chord tone, lvl * .92, .72 s */
    assert(apsis_string98_rhythm(&p, &s, &m, &e) == 4u);
    assert(voice(&p, 0u)->duration_samples == ms(720u));

    setup(&s, &m, &p);
    s.string.rhythm_level_q15 = 0u;
    e.sub_index = APSIS_STRING_RHYTHM_BOW;
    assert(apsis_string98_rhythm(&p, &s, &m, &e) == 0u);
    puts("PASS: Study-98 rhythm: bow .70 s, pizz 3+3 .34 s, motion .72 s");
}

static void test_strum(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringTransientPool p;
    ApsisStringAttractorBinding b;
    const ApsisStringTransientVoice *f;
    uint16_t quiet, loud;
    memset(&b, 0, sizeof b);
    b.valid = 1u;
    b.pos = 2u;                                 /* pan ((2 % 5) - 2) * .045 = 0 */
    b.target_sample = 1000u + 100u;

    setup(&s, &m, &p);
    b.value_q15 = 0u;                           /* dur .26 + .34 = .60 s; level .055 * amount * user */
    assert(apsis_string98_strum(&p, &s, &m, &b, 1000u) == 6u);   /* pizz 3 + root shadow 3 (.12) */
    f = voice(&p, 0u);
    assert(f->duration_samples == ms(600u));
    assert(f->start_delay_samples == 100u);
    quiet = f->peak_gain_q15;

    setup(&s, &m, &p);
    b.value_q15 = APSIS_Q15_ONE;                /* dur .26 + .34 * .75 = .515 s; level .160 * ... */
    assert(apsis_string98_strum(&p, &s, &m, &b, 1000u) == 6u);
    f = voice(&p, 0u);
    assert(f->duration_samples == ms(515u));
    loud = f->peak_gain_q15;
    assert(loud > quiet * 2u && loud < quiet * 4u);  /* .160 / .055 = 2.9 */

    setup(&s, &m, &p);
    s.string.filter_attractor_strum_q15 = 0u;   /* amount 0: no strum */
    assert(apsis_string98_strum(&p, &s, &m, &b, 1000u) == 0u);
    puts("PASS: Study-98 turingStrum: pizz + root .12, level and length by the cell's value, bound start");
}

int main(void)
{
    test_pluck();
    test_rhythm();
    test_strum();
    return 0;
}
