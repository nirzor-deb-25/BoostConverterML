#include <stdint.h>

#include "stm32_regs.h"
#include "timing.h"

#define LED_GREEN  (1UL << 12)
#define LED_ORANGE (1UL << 13)
#define LED_RED    (1UL << 14)
#define LED_BLUE   (1UL << 15)
#define LED_ALL    (LED_GREEN | LED_ORANGE | LED_RED | LED_BLUE)

typedef struct
{
    volatile uint32_t request_seq;
    volatile uint32_t input_voltage_mV;
    volatile uint32_t measured_output_mV;
    volatile uint32_t target_output_mV;
    volatile uint32_t load_milliohm;
    volatile uint32_t duty_permille;
    volatile uint32_t status_code;
    volatile uint32_t response_seq;
} boost_exchange_t;

/* PowerShell will locate this mailbox in the ELF and access it over SWD. */
volatile boost_exchange_t boost_exchange __attribute__((used));

static void leds_init(void)
{
    RCC_AHB1ENR |= (1UL << 3);
    (void)RCC_AHB1ENR;

    GPIOD_BSRR = LED_ALL << 16;
    GPIOD_OTYPER &= ~LED_ALL;

    const uint32_t mode_mask = 0xFFUL << 24;
    GPIOD_MODER =
        (GPIOD_MODER & ~mode_mask) |
        (0x55UL << 24);
}

static void show_status(uint32_t status)
{
    uint32_t selected_led = LED_BLUE;

    if (status == 1U)
    {
        selected_led = LED_GREEN;
    }
    else if (status == 2U)
    {
        selected_led = LED_ORANGE;
    }
    else if (status == 3U)
    {
        selected_led = LED_RED;
    }

    GPIOD_BSRR =
        ((LED_ALL & ~selected_led) << 16) |
        selected_led;
}

int main(void)
{
    timing_init();
    leds_init();
    show_status(0U);

    uint32_t last_request = 0U;
    int32_t integral_permille = 0;

    for (;;)
    {
        const uint32_t request = boost_exchange.request_seq;

        if (request == last_request)
        {
            continue;
        }

        __asm volatile ("dmb" ::: "memory");

        const uint32_t vin = boost_exchange.input_voltage_mV;
        const uint32_t vout = boost_exchange.measured_output_mV;
        const uint32_t target = boost_exchange.target_output_mV;

        uint32_t duty_permille = 200U;
        uint32_t status = 3U;

        if ((vin >= 1000U) &&
            (vin <= 60000U) &&
            (target > vin) &&
            (target <= 60000U) &&
            (vout <= 60000U))
        {
            const int32_t feedforward =
                (int32_t)(((target - vin) * 1000U) / target);

            const int32_t error =
                (int32_t)target - (int32_t)vout;

            integral_permille += error / 500;

            if (integral_permille > 80)
            {
                integral_permille = 80;
            }
            else if (integral_permille < -80)
            {
                integral_permille = -80;
            }

            int32_t duty =
                feedforward + (error / 50) + integral_permille;

            if (duty < 200)
            {
                duty = 200;
            }
            else if (duty > 500)
            {
                duty = 500;
            }

            duty_permille = (uint32_t)duty;

            if ((vout + 300U) < target)
            {
                status = 2U;
            }
            else if (vout > (target + 500U))
            {
                status = 3U;
            }
            else
            {
                status = 1U;
            }
        }
        else
        {
            integral_permille = 0;
        }

        boost_exchange.duty_permille = duty_permille;
        boost_exchange.status_code = status;
        show_status(status);

        __asm volatile ("dmb" ::: "memory");
        boost_exchange.response_seq = request;
        last_request = request;
    }
}
