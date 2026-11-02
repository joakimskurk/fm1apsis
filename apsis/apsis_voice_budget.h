#ifndef APSIS_VOICE_BUDGET_H
#define APSIS_VOICE_BUDGET_H

/* The finite voices' budget (docs/MARGIN_PLAN.md B): a hard ceiling for each pool, so no input, however dense, can
 * make the audio interrupt's notes cost more than these caps times the measured cost of one voice. FM1_STRICT, the
 * stress build, us a block for one voice: String partial 43, Spiral oscillator 51 (2-3 a leg), Bloom FM petal ~96,
 * Clock voice ~40. A full pool steals its least audible voice with a short fade; the fades have a few slots of
 * their own (the reserve), and when those are full too, the quietest fade ends at once. */

#define APSIS_BUDGET_STRING_PARTIALS 24u      /* three plucks with their root shadow */
#define APSIS_BUDGET_STRING_FADES 6u
#define APSIS_BUDGET_SPIRAL_LEGS 6u           /* three two-leg hits */
#define APSIS_BUDGET_SPIRAL_FADES 2u
#define APSIS_BUDGET_BLOOM_PETALS 10u         /* two five-voice Blooms */
#define APSIS_BUDGET_BLOOM_FADES 2u
#define APSIS_BUDGET_CLOCK_VOICES 12u         /* a hit's layers, a few hits ringing */
#define APSIS_BUDGET_CLOCK_FADES 4u
#define APSIS_BUDGET_FADE_SAMPLES 441u        /* 10 ms to -60 dB: a stolen Spiral or Bloom voice */
#define APSIS_BUDGET_FADE_MUL_Q31 2114107878u  /* 0.001 ^ (1 / 441), a sample */

#endif
