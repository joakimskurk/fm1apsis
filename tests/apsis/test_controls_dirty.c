#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "apsis_controls_dirty.h"

int main(void)
{
    ApsisControlsProjectDirty d;

    apsis_controls_project_dirty_reset(&d);
    assert(d.dirty == 0u);
    assert(apsis_controls_project_dirty_take_due(&d, 999999u, 0u) == 0u);

    apsis_controls_project_dirty_mark(&d, 100u);
    assert(d.dirty == 1u);
    assert(d.deadline_sample == 44200u);
    assert(apsis_controls_project_dirty_take_due(&d, 44199u, 0u) == 0u);

    /* A later edit restarts the quiet window instead of queueing another write. */
    apsis_controls_project_dirty_mark(&d, 44199u);
    assert(d.deadline_sample == 88299u);
    assert(apsis_controls_project_dirty_take_due(&d, 88298u, 0u) == 0u);

    /* Transport never consumes the dirty bit; the save becomes due on stop. */
    assert(apsis_controls_project_dirty_take_due(&d, 100000u, 1u) == 0u);
    assert(d.dirty == 1u);
    assert(apsis_controls_project_dirty_take_due(&d, 100000u, 0u) == 1u);
    assert(d.dirty == 0u);
    assert(apsis_controls_project_dirty_take_due(&d, 100001u, 0u) == 0u);

    /* Saturating deadline keeps wraparound from making a write immediately due. */
    apsis_controls_project_dirty_mark(&d, UINT64_MAX - 100u);
    assert(d.deadline_sample == UINT64_MAX);
    assert(apsis_controls_project_dirty_take_due(&d, UINT64_MAX - 1u, 0u) == 0u);
    assert(apsis_controls_project_dirty_take_due(&d, UINT64_MAX, 0u) == 1u);

    puts("PASS: P9H3b project dirty writes coalesce and remain transport-deferred");
    return 0;
}
