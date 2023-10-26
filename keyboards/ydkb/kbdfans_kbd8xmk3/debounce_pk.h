#ifndef DEBOUNCE_PK_H
#define DEBOUNCE_PK_H

#include <stdbool.h>
#include "print.h"
#ifdef __AVR__
#include "avr_config.h"
#endif

#ifndef DEBOUNCE_DN
#define DEBOUNCE_DN 5
#endif
#ifndef DEBOUNCE16_DN
#define DEBOUNCE16_DN DEBOUNCE_DN
#endif

#ifndef DEBOUNCE_NK
#define DEBOUNCE_NK 1
#endif
#ifndef DEBOUNCE16_NK
#define DEBOUNCE16_NK DEBOUNCE_NK
#endif

#ifndef DEBOUNCE_UP
#define DEBOUNCE_UP 5
#endif
#ifndef DEBOUNCE16_UP
#define DEBOUNCE16_UP DEBOUNCE_UP
#endif

#if (DEBOUNCE_DN < 8) && (DEBOUNCE_NK < 8) && (DEBOUNCE_UP < 8)
#define DEBOUNCE_DN_MASK (uint8_t)(~(0x80 >> DEBOUNCE_DN))
#define DEBOUNCE_NK_MASK (uint8_t)(~(0x80 >> DEBOUNCE_NK))
#define DEBOUNCE_UP_MASK (uint8_t)(0x80 >> DEBOUNCE_UP)
#else
#error "DEBOUNCE VALUE must not exceed 7"
#endif

#if (DEBOUNCE16_DN < 16) && (DEBOUNCE16_NK < 16) && (DEBOUNCE16_UP < 16)
#define DEBOUNCE16_DN_MASK (uint16_t)(~(0x8000 >> DEBOUNCE16_DN))
#define DEBOUNCE16_NK_MASK (uint16_t)(~(0x8000 >> DEBOUNCE16_NK))
#define DEBOUNCE16_UP_MASK (uint16_t)(0x8000 >> DEBOUNCE16_UP)
#else
#error "DEBOUNCE16 VALUE must not exceed 15"
#endif


#endif
