#ifndef APSIS_CONTROLS_DIRTY_H
#define APSIS_CONTROLS_DIRTY_H

#include <stdint.h>

#define APSIS_CONTROLS_PROJECT_DIRTY_DELAY_SAMPLES 44100u

typedef struct {
    uint64_t deadline_sample;
    uint8_t dirty;
} ApsisControlsProjectDirty;

void apsis_controls_project_dirty_reset(ApsisControlsProjectDirty *dirty);
void apsis_controls_project_dirty_mark(ApsisControlsProjectDirty *dirty,
                                       uint64_t now_sample);
uint8_t apsis_controls_project_dirty_take_due(ApsisControlsProjectDirty *dirty,
                                              uint64_t now_sample,
                                              uint8_t transport_running);

#endif
