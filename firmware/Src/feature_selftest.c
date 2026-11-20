#include "feature_selftest.h"
#include "motion_features.h"
#include "feature_golden.h"
#include "uart.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

volatile uint32_t feature_test_failures;
volatile uint32_t feature_test_total;
volatile float feature_test_max_error_mg;

void feature_selftest(void)
{
    feature_test_total = (uint32_t)FEATURE_GOLDEN_COUNT;
    feature_test_failures = 0U;
    feature_test_max_error_mg = 0.0F;

    for (uint32_t i = 0U; i < feature_test_total; i++)
    {
        float actual[6];
        bool passed = motion_features(feature_golden[i].samples, actual);

        if (passed)
        {
            for (unsigned int j = 0U; j < 6U; j++)
            {
                float error = fabsf(actual[j] - feature_golden[i].expected[j]);

                if (!isfinite(error) || error > 0.02F)
                {
                    passed = false;
                }

                if (isfinite(error) && error > feature_test_max_error_mg)
                {
                    feature_test_max_error_mg = error;
                }
            }
        }

        if (!passed)
        {
            feature_test_failures++;
        }
    }

    char message[128];

    (void)snprintf(
        message,
        sizeof(message),
        "# FEATURE_SELFTEST total=%lu failures=%lu tolerance_mg=0.02\r\n",
        (unsigned long)feature_test_total,
        (unsigned long)feature_test_failures
    );

    (void)uart_write(message);
}
