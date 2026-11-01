#include "apsis_controls_dirty.h"

void apsis_controls_project_dirty_reset(ApsisControlsProjectDirty *dirty)
{
    if (!dirty) return;
    dirty->deadline_sample = 0u;
    dirty->dirty = 0u;
}

void apsis_controls_project_dirty_mark(ApsisControlsProjectDirty *dirty,
                                       uint64_t now_sample)
{
    uint64_t delay = APSIS_CONTROLS_PROJECT_DIRTY_DELAY_SAMPLES;
    if (!dirty) return;
    dirty->dirty = 1u;
    dirty->deadline_sample = now_sample > UINT64_MAX - delay
        ? UINT64_MAX : now_sample + delay;
}

uint8_t apsis_controls_project_dirty_take_due(ApsisControlsProjectDirty *dirty,
                                              uint64_t now_sample,
                                              uint8_t transport_running)
{
    if (!dirty || !dirty->dirty || transport_running) return 0u;
    if (now_sample < dirty->deadline_sample) return 0u;
    dirty->dirty = 0u;
    return 1u;
}
