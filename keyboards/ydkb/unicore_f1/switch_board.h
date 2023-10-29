#ifndef SWITCH_BOARD_H
#define SWITCH_BOARD_H

#include <stdint.h>
#include <stdbool.h>

//DS PB13
#define DS_PL_HI()      (palSetPad(GPIOB, 13))
#define DS_PL_LO()      (palClearPad(GPIOB, 13))

static inline void get_key_ready(void) {
    palSetPadMode(GPIOB, 13, PAL_MODE_INPUT_PULLUP);
}

static inline void select_key_ready(void) {
    palSetPadMode(GPIOB, 13, PAL_MODE_OUTPUT_PUSHPULL);
} 

//SCK PB12
#define CLOCK_PULSE() \
    do { \
        palSetPad(GPIOB, 12); \
        asm("nop"); \
        palClearPad(GPIOB, 12); \
    } while(0)
#endif
