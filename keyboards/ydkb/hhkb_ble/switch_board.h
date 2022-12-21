#ifndef SWITCH_BOARD_H
#define SWITCH_BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timer_avr.h"

// Timer resolution check
#if (1000000/TIMER_RAW_FREQ > 20)
#   error "Timer resolution(>20us) is not enough for HHKB matrix scan tweak on V-USB."
#endif


extern bool is_ver_jp;

//add to row7 and row8
// 7                                                           UM_CAPS, }
// 8 { UM_HOME,UM_APP, UM_END, UM_RCTL,UM_LEFT,UM_DOWN,UM_UP,  UM_RGHT, }

// transform jp matrix to us.
static const uint8_t matrix_trans[16][5] = {
    { 0x31, 0x32, 0x80, 0x34, 0x77 },
    { 0x11, 0x13, 0x81, 0x07, 0x14 },
    { 0x10, 0x02, 0x35, 0x06, 0x03 },
    { 0x30, 0x27, 0x33, 0x27, 0x27 },
    { 0x27, 0x27, 0x27, 0x27, 0x27 },
    { 0x20, 0x12, 0x27, 0x16, 0x15 },
    { 0x00, 0x01, 0x36, 0x05, 0x04 },
    { 0x21, 0x23, 0x37, 0x17, 0x24 },
    { 0x60, 0x43, 0x56, 0x66, 0x44 },
    { 0x41, 0x42, 0x82, 0x46, 0x45 },
    { 0x40, 0x22, 0x27, 0x26, 0x25 },
    { 0x61, 0x62, 0x57, 0x76, 0x65 },
    { 0x52, 0x27, 0x87, 0x55, 0x53 },
    { 0x51, 0x72, 0x85, 0x86, 0x50 },
    { 0x70, 0x63, 0x83, 0x75, 0x64 },
    { 0x71, 0x73, 0x84, 0x54, 0x74 }
};

/*
 * HHKB Matrix I/O
 *
 * row:     HC4051[A,B,C]  selects scan row0-7
 * row-ext: [En0,En1] row extention for JP
 * col:     LS145[A,B,C,D] selects scan col0-7 and enable(D)
 * key:     on: 0/off: 1
 * prev:    hysteresis control: assert(1) when previous key state is on
 */

/*
 * For HHKB BLE Controller (atmega32u4)
 *
 * row:     PB0-2
 * col:     PB3-5,6
 * key:     PD7(pull-uped)
 * prev:    PB7
 * power:   PD4(L:off/H:on)
 * row-ext: PC6,7 for HHKB JP(active low)
 */
static inline void KEY_ENABLE(void) { (PORTB &= ~(1<<6)); }
static inline void KEY_UNABLE(void) { (PORTB |=  (1<<6)); }
static inline bool KEY_STATE(void) { return (PIND & (1<<7)); }
static inline void KEY_PREV_ON(void) { (PORTB |=  (1<<7)); }
static inline void KEY_PREV_OFF(void) { (PORTB &= ~(1<<7)); }

static inline void KEY_POWER_ON(void) {
    DDRB = 0xFF; PORTB = 0x40;
    DDRD |= (1<<6); 
    PORTD &= ~(1<<6);

    if (is_ver_jp) {
        DDRC  |= (1<<6|1<<7);
        PORTC |= (1<<6|1<<7);
    }
    /* Without this wait you will miss or get false key events. */
    _delay_us(20);
}

static inline void KEY_POWER_OFF(void) {
    /* input with pull-up consumes less than without it when pin is open. */
    DDRB = 0x00; PORTB = 0xFF;
    DDRD |= (1<<6); 
    PORTD |= (1<<6);

    if (is_ver_jp) {
        DDRC  &= ~(1<<6|1<<7);
        PORTC |=  (1<<6|1<<7);
    }
}

static inline void KEY_INIT(void)
{
    /* row,col,prev: output */
    DDRB  = 0xFF;
    PORTB = 0x40;   // unable
    /* key: input with pull-up */
    DDRD  &= ~(1<<7);
    PORTD |=  (1<<7);

    if (is_ver_jp) {
        /* row extention for HHKB JP */
        DDRC  |= (1<<6|1<<7);
        PORTC |= (1<<6|1<<7);
    } else {
        /* input with pull up to save power */
        DDRC  &= ~(1<<6|1<<7);
        PORTC |=  (1<<6|1<<7);
    }
    KEY_UNABLE();
    KEY_PREV_OFF();

    KEY_POWER_ON();
}
static inline void KEY_SELECT(uint8_t ROW, uint8_t COL)
{
    if (is_ver_jp) COL += 2;
    PORTB = (PORTB & 0xC0) | (((COL) & 0x07)<<3) | ((ROW) & 0x07);
    if (is_ver_jp) {
        if ((ROW) & 0x08) PORTC = (PORTC & ~(1<<6|1<<7)) | (1<<6);
        else              PORTC = (PORTC & ~(1<<6|1<<7)) | (1<<7);
    }
}

#endif
