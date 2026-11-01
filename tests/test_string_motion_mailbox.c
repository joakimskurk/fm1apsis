#include <assert.h>
#include <stdio.h>

#include "apsis_string_motion_mailbox.h"

static void test_publish_and_take(void)
{
    ApsisStringMotionMailbox mb;
    ApsisStringMotionControlResult c = {0};
    ApsisStringMotionMailboxFrame f;
    uint32_t packed;

    apsis_string_motion_mailbox_reset(&mb);
    c.action = APSIS_STRING_MOTION_ACTION_ARM_TOGGLE;
    c.record_target = APSIS_STRING_MOTION_TARGET_LEFT;
    c.target_changed = 1u;
    apsis_string_motion_mailbox_publish(&mb, &c, 1u, 1u, 1u);
    packed = apsis_string_motion_mailbox_take(&mb);
    assert(packed != 0u);
    assert(mb.pending == 0u);
    apsis_string_motion_mailbox_decode(packed, &f);
    assert(f.action == APSIS_STRING_MOTION_ACTION_ARM_TOGGLE);
    assert(f.target_valid && f.target == APSIS_STRING_MOTION_TARGET_LEFT);
    assert(f.capture);
    assert(f.manual_valid && f.manual_active);
}

static void test_coalescing(void)
{
    ApsisStringMotionMailbox mb;
    ApsisStringMotionControlResult c = {0};
    ApsisStringMotionMailboxFrame f;
    uint32_t packed;

    apsis_string_motion_mailbox_reset(&mb);
    c.action = APSIS_STRING_MOTION_ACTION_ARM_TOGGLE;
    c.record_target = APSIS_STRING_MOTION_TARGET_LEFT;
    c.target_changed = 1u;
    apsis_string_motion_mailbox_publish(&mb, &c, 1u, 1u, 1u);

    c.action = APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE;
    c.record_target = APSIS_STRING_MOTION_TARGET_BOTH;
    c.target_changed = 1u;
    apsis_string_motion_mailbox_publish(&mb, &c, 0u, 1u, 0u);

    packed = apsis_string_motion_mailbox_take(&mb);
    apsis_string_motion_mailbox_decode(packed, &f);
    assert(f.action == APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE);
    assert(f.target_valid && f.target == APSIS_STRING_MOTION_TARGET_BOTH);
    assert(f.capture); /* sticky until ISR consumes */
    assert(f.manual_valid && !f.manual_active); /* latest state wins */
}

static void test_empty_take(void)
{
    ApsisStringMotionMailbox mb;
    ApsisStringMotionMailboxFrame f;
    apsis_string_motion_mailbox_reset(&mb);
    assert(apsis_string_motion_mailbox_take(&mb) == 0u);
    apsis_string_motion_mailbox_decode(0u, &f);
    assert(f.action == APSIS_STRING_MOTION_ACTION_NONE);
    assert(!f.target_valid && !f.capture && !f.manual_valid);
}

int main(void)
{
    test_publish_and_take();
    test_coalescing();
    test_empty_take();
    puts("PASS: Phase 10D String Motion main/audio mailbox");
    return 0;
}
