#ifndef APSIS_STRING_MOTION_MAILBOX_H
#define APSIS_STRING_MOTION_MAILBOX_H

#include "apsis_controls_string_motion.h"

#define APSIS_STRING_MOTION_MB_ACTION_MASK 0x00000003u
#define APSIS_STRING_MOTION_MB_TARGET_SHIFT 2u
#define APSIS_STRING_MOTION_MB_TARGET_MASK 0x0000000cu
#define APSIS_STRING_MOTION_MB_TARGET_VALID 0x00000010u
#define APSIS_STRING_MOTION_MB_CAPTURE 0x00000020u
#define APSIS_STRING_MOTION_MB_MANUAL_VALID 0x00000040u
#define APSIS_STRING_MOTION_MB_MANUAL_ACTIVE 0x00000080u

typedef struct {
    volatile uint32_t pending;
} ApsisStringMotionMailbox;

typedef struct {
    uint8_t action;
    uint8_t target;
    uint8_t target_valid;
    uint8_t capture;
    uint8_t manual_valid;
    uint8_t manual_active;
} ApsisStringMotionMailboxFrame;

void apsis_string_motion_mailbox_reset(ApsisStringMotionMailbox *mailbox);

/* Single main-loop producer. Target wraps this read/modify/write in its already
 * established short IRQ guard because the audio ISR owns take(). Multiple
 * pre-callback publications coalesce: latest action/target/manual state wins,
 * while capture remains sticky until consumed. */
void apsis_string_motion_mailbox_publish(
    ApsisStringMotionMailbox *mailbox,
    const ApsisStringMotionControlResult *control,
    uint8_t capture,
    uint8_t manual_valid,
    uint8_t manual_active);

uint32_t apsis_string_motion_mailbox_take(ApsisStringMotionMailbox *mailbox);
void apsis_string_motion_mailbox_decode(uint32_t packed,
                                        ApsisStringMotionMailboxFrame *out);

#endif
