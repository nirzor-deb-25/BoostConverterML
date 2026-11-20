#ifndef MOTION_LIVE_H
#define MOTION_LIVE_H

#include <stdint.h>
#include "lis3dsh.h"

void motion_live_init(void);
void motion_live_poll(void);
void motion_live_sample(const lis3dsh_raw_t *sample, uint8_t status);

#endif
