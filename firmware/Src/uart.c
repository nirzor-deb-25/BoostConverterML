#include "uart.h"
#include "timing.h"
#include "stm32_regs.h"

#define RCC_APB1ENR REG32(0x40023840UL)

#define USART2_SR  REG32(0x40004400UL)
#define USART2_DR  REG32(0x40004404UL)
#define USART2_BRR REG32(0x40004408UL)
#define USART2_CR1 REG32(0x4000440CUL)
#define USART2_CR2 REG32(0x40004410UL)
#define USART2_CR3 REG32(0x40004414UL)

void uart_init(void)
{
    RCC_AHB1ENR |= (1UL << 0);
    (void)RCC_AHB1ENR;

    /* PA2: alternate function AF7, USART2 TX. */
    GPIOA_OTYPER &= ~(1UL << 2);
    GPIOA_OSPEEDR =
        (GPIOA_OSPEEDR & ~(3UL << 4)) | (1UL << 4);
    GPIOA_PUPDR &= ~(3UL << 4);
    GPIOA_AFRL =
        (GPIOA_AFRL & ~(15UL << 8)) | (7UL << 8);
    GPIOA_MODER =
        (GPIOA_MODER & ~(3UL << 4)) | (2UL << 4);

    RCC_APB1ENR |= (1UL << 17);
    (void)RCC_APB1ENR;

    /* APB1 = 16 MHz, 115200 baud, 8N1. */
    USART2_CR1 = 0U;
    USART2_CR2 = 0U;
    USART2_CR3 = 0U;
    USART2_BRR = 139U;
    USART2_CR1 = (1UL << 13) | (1UL << 3);
}

bool uart_write(const char *text)
{
    if (text == 0)
    {
        return false;
    }

    while (*text != '\0')
    {
        uint32_t start = timing_cycles();

        while ((USART2_SR & (1UL << 7)) == 0U)
        {
            if (timing_has_elapsed(start, 10U))
            {
                return false;
            }
        }

        USART2_DR = (uint8_t)*text;
        text++;
    }

    return true;
}
