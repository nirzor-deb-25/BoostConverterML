#include "motion_features.h"
#include <math.h>

bool motion_features(
    const int16_t samples[MOTION_WINDOW_SAMPLES][3],
    float output[6]
)
{
    if ((samples == 0) || (output == 0))
    {
        return false;
    }

    int64_t sums[3] = {0, 0, 0};
    int64_t squared_sums[3] = {0, 0, 0};

    /*
     * int64_t safely holds sums of 100 squared int16_t samples.
     * Cast before multiplication to avoid intermediate overflow.
     */
    for (unsigned int i = 0U; i < MOTION_WINDOW_SAMPLES; i++)
    {
        for (unsigned int axis = 0U; axis < 3U; axis++)
        {
            int64_t value = samples[i][axis];
            sums[axis] += value;
            squared_sums[axis] += value * value;
        }
    }

    for (unsigned int axis = 0U; axis < 3U; axis++)
    {
        double mean_raw =
            (double)sums[axis] / (double)MOTION_WINDOW_SAMPLES;

        double variance_raw =
            (double)squared_sums[axis] /
            (double)MOTION_WINDOW_SAMPLES -
            mean_raw * mean_raw;

        /* Protect sqrt against tiny negative rounding errors. */
        if (variance_raw < 0.0)
        {
            variance_raw = 0.0;
        }

        output[axis] = (float)(mean_raw * 0.06);
        output[axis + 3U] = (float)(sqrt(variance_raw) * 0.06);
    }

    return true;
}
