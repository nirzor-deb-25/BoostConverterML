#include "motion_live.h"
#include "motion_features.h"
#include "motion_model.h"
#include "stm32_regs.h"
#include "timing.h"
#include "uart.h"

#include <stdbool.h>
#include <stdio.h>

#define LED_MASK (15UL << 12)

extern volatile uint32_t motion_test_failures;
extern volatile uint32_t feature_test_failures;

static int16_t window[100][3];
static unsigned int position;
static unsigned int count;
static unsigned int hop;
static uint32_t last_sample_cycles;
static bool enabled;

static motion_class_t candidate = MOTION_INVALID;
static motion_class_t accepted = MOTION_INVALID;
static unsigned int confirmations;

volatile uint32_t motion_window_resets;
volatile uint32_t motion_sensor_overruns;
volatile uint32_t motion_processing_cycles;
volatile int32_t motion_live_class = MOTION_INVALID;

static void display_class(motion_class_t value)
{
    uint32_t selected = 0U;

    switch (value)
    {
        case MOTION_NEUTRAL:
            selected = 1UL << 12;
            break;
        case MOTION_SHAKE:
            selected = 1UL << 14;
            break;
        case MOTION_TILT_X_NEGATIVE:
            selected = 1UL << 13;
            break;
        case MOTION_TILT_X_POSITIVE:
            selected = 1UL << 15;
            break;
        default:
            break;
    }

    GPIOD_BSRR = selected | ((LED_MASK & ~selected) << 16);
}

static void clear_window(void)
{
    position = 0U;
    count = 0U;
    hop = 0U;
    candidate = MOTION_INVALID;
    accepted = MOTION_INVALID;
    confirmations = 0U;
    motion_live_class = MOTION_INVALID;
    display_class(MOTION_INVALID);
}

void motion_live_init(void)
{
    enabled =
        (motion_test_failures == 0U) &&
        (feature_test_failures == 0U);

    clear_window();
    last_sample_cycles = timing_cycles();

    (void)uart_write(
        enabled ?
        "# LIVE model=" MOTION_MODEL_ID " window=100 hop=25\r\n" :
        "# LIVE_DISABLED selftest_failure\r\n"
    );
}

void motion_live_poll(void)
{
    if (!enabled)
    {
        return;
    }

    /* Clear stale predictions if no successful sample arrives for 50 ms. */
    if (timing_has_elapsed(last_sample_cycles, 50U))
    {
        if (count != 0U)
        {
            motion_window_resets++;
            clear_window();
        }

        /* Keep the comparison interval bounded across counter rollover. */
        last_sample_cycles = timing_cycles();
    }

    display_class(accepted);
}

void motion_live_sample(const lis3dsh_raw_t *sample, uint8_t status)
{
    if (!enabled || sample == 0)
    {
        return;
    }

    uint32_t now = timing_cycles();
    bool gap = timing_has_elapsed(last_sample_cycles, 25U);
    last_sample_cycles = now;

    if ((status & 0x80U) != 0U)
    {
        motion_sensor_overruns++;
    }

    bool near_limit =
        sample->x >= 32000 || sample->x <= -32000 ||
        sample->y >= 32000 || sample->y <= -32000 ||
        sample->z >= 32000 || sample->z <= -32000;

    if (gap || near_limit || ((status & 0x88U) != 0x08U))
    {
        motion_window_resets++;
        clear_window();
        return;
    }

    window[position][0] = sample->x;
    window[position][1] = sample->y;
    window[position][2] = sample->z;
    position = (position + 1U) % 100U;

    if (count < 100U)
    {
        count++;
    }

    hop++;

    if (count < 100U || hop < 25U)
    {
        return;
    }

    hop = 0U;

    /*
     * Mean/std are independent of sample order, so the circular buffer
     * does not need to be rearranged for these six features.
     */
    float features[6];
    uint32_t processing_start = timing_cycles();

    motion_class_t predicted = MOTION_INVALID;

    if (motion_features((const int16_t (*)[3])window, features))
    {
        predicted = motion_predict(features);
    }

    motion_processing_cycles = timing_cycles() - processing_start;

    if (predicted == MOTION_INVALID)
    {
        motion_window_resets++;
        clear_window();
        return;
    }

    if (predicted != candidate)
    {
        candidate = predicted;
        confirmations = 1U;
    }
    else if (confirmations < 2U)
    {
        confirmations++;
    }

    if (confirmations >= 2U)
    {
        accepted = candidate;
    }

    motion_live_class = (int32_t)accepted;
    display_class(accepted);

    char message[128];

    (void)snprintf(
        message,
        sizeof(message),
        "# P raw=%d led=%d compute_us=%lu resets=%lu overruns=%lu\r\n",
        (int)predicted,
        (int)accepted,
        (unsigned long)(motion_processing_cycles / 16U),
        (unsigned long)motion_window_resets,
        (unsigned long)motion_sensor_overruns
    );

    (void)uart_write(message);
}
