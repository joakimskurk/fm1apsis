#ifndef APSIS_DRIFT_H
#define APSIS_DRIFT_H

#include <stdint.h>
#include "apsis_state.h"

/* Browser beatDelta() in milli-Hz: 0.05 + 3.95 * amount^1.65. */
uint16_t apsis_drift_delta_millihz(uint16_t amount_q15);
uint16_t apsis_drift_audible_delta_millihz(const ApsisDriftState *drift);

#endif
