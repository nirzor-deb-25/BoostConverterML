#ifndef MOTION_FEATURES_H
#define MOTION_FEATURES_H

#include <stdbool.h>
#include <stdint.h>

#define MOTION_WINDOW_SAMPLES 100U

bool motion_features(
    const int16_t samples[MOTION_WINDOW_SAMPLES][3],
    float output[6]
);

#endif
