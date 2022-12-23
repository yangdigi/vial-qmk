#ifndef SWITCH_BOARD_H
#define SWITCH_BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

#define DS_PL_HI()      (PORTF |=  (1<<6))
#define DS_PL_LO()      (PORTF &= ~(1<<6))


#define CLOCK_PULSE() \
    do { \
        PORTF |= (1<<4); \
        asm("nop"); \
        PORTF &= ~(1<<4); \
    } while(0)

#endif
