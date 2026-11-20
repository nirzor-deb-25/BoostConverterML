#ifndef UART_H
#define UART_H

#include <stdbool.h>

void uart_init(void);
bool uart_write(const char *text);

#endif
