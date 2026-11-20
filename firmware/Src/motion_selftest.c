#include "motion_selftest.h"
#include "motion_model.h"
#include "motion_golden.h"
#include "uart.h"

#include <stdio.h>
#include <stdint.h>

volatile uint32_t motion_test_total;
volatile uint32_t motion_test_failures;

void motion_selftest(void)
{
    motion_test_total = (uint32_t)MOTION_GOLDEN_COUNT;
    motion_test_failures = 0U;

    for (uint32_t i = 0U; i < motion_test_total; i++)
    {
        int predicted = (int)motion_predict(motion_golden[i].features);

        if (predicted != motion_golden[i].expected)
        {
            motion_test_failures++;
        }
    }

    char message[128];

    (void)snprintf(
        message,
        sizeof(message),
        "# MODEL_SELFTEST model=%s total=%lu failures=%lu\r\n",
        MOTION_MODEL_ID,
        (unsigned long)motion_test_total,
        (unsigned long)motion_test_failures
    );

    (void)uart_write(message);
}
