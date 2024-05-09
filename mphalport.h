#include "port/mphalport.h"

#include "py/ringbuf.h"

// There is probably some STM32 register that we could read out, but for now
// let's stay portable and use the Playdate API.
#define mp_hal_ticks_cpu mp_hal_ticks_us

extern ringbuf_t stdin_ringbuf;
