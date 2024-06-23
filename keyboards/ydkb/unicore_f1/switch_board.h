#ifndef SWITCH_BOARD_H
#define SWITCH_BOARD_H

#include <stdint.h>
#include <stdbool.h>

extern bool is_ver5020;

//SDI PB13
static inline void KEY_SDI_OFF(void) {
    if (is_ver5020) {
        palClearPad(GPIOB, 13);
    } else {
        palSetPad(GPIOB, 13);
    }
}
static inline void KEY_SDI_ON(void) {
    if (is_ver5020) {
        palSetPad(GPIOB, 13);
    } else {
        palClearPad(GPIOB, 13);
    }
}

static inline void get_key_ready(void) {
    // PB13 input
    palSetPadMode(GPIOB, 13, PAL_MODE_INPUT_PULLUP);
    // PB13 PB14 pull up
    palSetPad(GPIOB, 13);
}

static inline void select_key_ready(void) {
    palSetPadMode(GPIOB, 13, PAL_MODE_OUTPUT_PUSHPULL);
} 

//SCK PB12
//asm("nop");
#define CLOCK_PULSE() \
    do { \
        palSetPad(GPIOB, 12); \
        palClearPad(GPIOB, 12); \
    } while(0)

//RCK PB11
#if 0
#define KEYS_LATCH() \
    do { \
        palSetPad(GPIOB, 11); \
        palClearPad(GPIOB, 11); \
    } while(0)

#endif
#endif