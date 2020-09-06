#include "Utils.h"

#include <stdint.h>

void fixedDelayLong() {
    for (uint8_t i = 0; i < 255; i++) {
        __asm__ __volatile__ ("nop");
    }
}
