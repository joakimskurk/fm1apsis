#include "apsis_string_motion_mailbox.h"

static void p10d_mb_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

void apsis_string_motion_mailbox_reset(ApsisStringMotionMailbox *mailbox)
{
    if (mailbox) mailbox->pending = 0u;
}

void apsis_string_motion_mailbox_publish(
    ApsisStringMotionMailbox *mailbox,
    const ApsisStringMotionControlResult *control,
    uint8_t capture,
    uint8_t manual_valid,
    uint8_t manual_active)
{
    uint32_t p;
    if (!mailbox) return;
    p = mailbox->pending;
    if (control) {
        if (control->action != APSIS_STRING_MOTION_ACTION_NONE) {
            p &= ~APSIS_STRING_MOTION_MB_ACTION_MASK;
            p |= (uint32_t)control->action & APSIS_STRING_MOTION_MB_ACTION_MASK;
        }
        if (control->target_changed) {
            p &= ~APSIS_STRING_MOTION_MB_TARGET_MASK;
            p |= ((uint32_t)control->record_target << APSIS_STRING_MOTION_MB_TARGET_SHIFT) &
                 APSIS_STRING_MOTION_MB_TARGET_MASK;
            p |= APSIS_STRING_MOTION_MB_TARGET_VALID;
        }
    }
    if (capture) p |= APSIS_STRING_MOTION_MB_CAPTURE;
    if (manual_valid) {
        p |= APSIS_STRING_MOTION_MB_MANUAL_VALID;
        if (manual_active) p |= APSIS_STRING_MOTION_MB_MANUAL_ACTIVE;
        else p &= ~APSIS_STRING_MOTION_MB_MANUAL_ACTIVE;
    }
    mailbox->pending = p;
}

uint32_t apsis_string_motion_mailbox_take(ApsisStringMotionMailbox *mailbox)
{
    uint32_t p;
    if (!mailbox) return 0u;
    p = mailbox->pending;
    mailbox->pending = 0u;
    return p;
}

void apsis_string_motion_mailbox_decode(uint32_t packed,
                                        ApsisStringMotionMailboxFrame *out)
{
    if (!out) return;
    p10d_mb_zero(out, (uint32_t)sizeof(*out));
    out->action = (uint8_t)(packed & APSIS_STRING_MOTION_MB_ACTION_MASK);
    out->target = (uint8_t)((packed & APSIS_STRING_MOTION_MB_TARGET_MASK) >>
                            APSIS_STRING_MOTION_MB_TARGET_SHIFT);
    out->target_valid = (packed & APSIS_STRING_MOTION_MB_TARGET_VALID) ? 1u : 0u;
    out->capture = (packed & APSIS_STRING_MOTION_MB_CAPTURE) ? 1u : 0u;
    out->manual_valid = (packed & APSIS_STRING_MOTION_MB_MANUAL_VALID) ? 1u : 0u;
    out->manual_active = (packed & APSIS_STRING_MOTION_MB_MANUAL_ACTIVE) ? 1u : 0u;
}
