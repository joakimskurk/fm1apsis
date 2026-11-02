#include <stdio.h>
#include <stdint.h>

#include "apsis_runtime.h"
#include "apsis_state.h"
#include "apsis_string_motion.h"

int main(void) {
    const size_t runtime_size = sizeof(ApsisRuntime);
    const size_t state_size = sizeof(ApsisState);
    const size_t motion_point_size = sizeof(ApsisStringMotionPoint);
    const size_t motion_lane_size = sizeof(ApsisStringMotionLane);
    const size_t motion_store_size = sizeof(ApsisStringMotionStore);
    const size_t motion_runtime_size = sizeof(ApsisStringMotionRuntime);

    if (runtime_size != 21928u) {
        fprintf(stderr, "FAIL: ApsisRuntime size %zu != 21928\n", runtime_size);
        return 1;
    }
    if (motion_point_size != 8u) {
        fprintf(stderr, "FAIL: Motion point size %zu != 8\n", motion_point_size);
        return 2;
    }
    if (motion_lane_size != 9604u) {
        fprintf(stderr, "FAIL: Motion lane size %zu != 9604\n", motion_lane_size);
        return 3;
    }
    if (motion_store_size != 19208u) {
        fprintf(stderr, "FAIL: Motion store size %zu != 19208\n", motion_store_size);
        return 4;
    }

    printf("{\n");
    printf("  \"apsis_runtime_bytes\": %zu,\n", runtime_size);
    printf("  \"apsis_state_bytes\": %zu,\n", state_size);
    printf("  \"motion_point_bytes\": %zu,\n", motion_point_size);
    printf("  \"motion_lane_bytes\": %zu,\n", motion_lane_size);
    printf("  \"motion_store_bytes\": %zu,\n", motion_store_size);
    printf("  \"motion_runtime_bytes\": %zu,\n", motion_runtime_size);
    printf("  \"motion_points_per_lane\": %u,\n", (unsigned)APSIS_STRING_MOTION_POINTS_MAX);
    printf("  \"motion_capture_hz\": %u\n", (unsigned)APSIS_STRING_MOTION_CAPTURE_HZ);
    printf("}\n");
    return 0;
}
