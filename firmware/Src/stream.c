#include "stream.h"
#include "timing.h"
#include "uart.h"

#include <stdio.h>

static uint32_t previous_cycles;
static uint32_t time_us;
static uint32_t remaining_cycles;
static uint32_t sequence;
static uint32_t tx_errors;

void stream_poll_clock(void)
{
    uint32_t now = timing_cycles();
    uint32_t elapsed = now - previous_cycles;
    previous_cycles = now;

    /* 16 MHz: 16 CPU cycles per microsecond. */
    time_us += elapsed / 16U;
    remaining_cycles += elapsed % 16U;
    time_us += remaining_cycles / 16U;
    remaining_cycles %= 16U;
}

void stream_init(void)
{
    previous_cycles = timing_cycles();
    time_us = 0U;
    remaining_cycles = 0U;
    sequence = 0U;
    tx_errors = 0U;

    (void)uart_write(
        "# MotionSense stream v1; raw counts; ODR=100Hz; range=2g\r\n"
    );

    (void)uart_write(
        "seq,time_us,x_raw,y_raw,z_raw,status,spi_errors,tx_errors\r\n"
    );
}

void stream_sample(const lis3dsh_raw_t *sample,
                   uint8_t status,
                   uint32_t spi_errors)
{
    stream_poll_clock();

    char line[128];

    int length = snprintf(
        line,
        sizeof(line),
        "%lu,%lu,%d,%d,%d,%u,%lu,%lu\r\n",
        (unsigned long)sequence,
        (unsigned long)time_us,
        (int)sample->x,
        (int)sample->y,
        (int)sample->z,
        (unsigned int)status,
        (unsigned long)spi_errors,
        (unsigned long)tx_errors
    );

    sequence++;

    if ((length <= 0) || ((unsigned int)length >= sizeof(line)))
    {
        tx_errors++;
        return;
    }

    if (!uart_write(line))
    {
        tx_errors++;

        /* Help the receiver recover its line boundary. */
        (void)uart_write("\r\n");
    }
}
