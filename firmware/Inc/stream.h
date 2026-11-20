#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include "lis3dsh.h"

void stream_init(void);
void stream_poll_clock(void);
void stream_sample(const lis3dsh_raw_t *sample,
                   uint8_t status,
                   uint32_t spi_errors);

#endif
