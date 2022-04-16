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


extern bool is_980c;

// transform 660c matrix to 980c.
static const uint8_t matrix_trans[5][16] = {
    { 0x39, 0x58, 0x5B, 0x5A, 0x3B, 0x3E, 0x3F, 0x5E, 0x52, 0x51, 0x50, 0x3C, 0x33, 0x68, 0x43, 0x64 },
    { 0x59, 0x49, 0x48, 0x0D, 0x4B, 0x4E, 0x5F, 0x4F, 0x4C, 0x42, 0x41, 0x5C, 0x40, 0x68, 0x45, 0x66 },
    { 0x68, 0x09, 0x19, 0x1A, 0x68, 0x68, 0x68, 0x0F, 0x02, 0x68, 0x01, 0x68, 0x10, 0x04, 0x00, 0x06 },
    { 0x68, 0x18, 0x08, 0x2A, 0x1B, 0x1E, 0x0B, 0x1F, 0x22, 0x11, 0x21, 0x0E, 0x30, 0x34, 0x68, 0x68 },
    { 0x29, 0x38, 0x28, 0x3A, 0x2B, 0x2C, 0x2F, 0x2E, 0x32, 0x31, 0x20, 0x1C, 0x68, 0x68, 0x53, 0x68 }
};

/*
 * Pin configuration for ATMega32U4
 *
 * Row:     PD4-6, 7(~EN)
 * Col:     PB0-2, 3(Z5 ~EN), 4(Z4 ~EN)
 * Key:     PC6(pull-uped)
 * Hys:     PC7
 * power:   PF7(L:on/H:off)
*/
static inline void KEY_ENABLE(void) { is_980c? (PORTD &= ~(1<<4)) : (PORTD &= ~(1<<7)); }
static inline void KEY_UNABLE(void) { is_980c? (PORTD |= (1<<4)) : (PORTD |=  (1<<7)); }
static inline bool KEY_STATE(void) { return (PINC & (1<<6)); }
static inline void KEY_HYS_ON(void) { (PORTC |=  (1<<7)); }
static inline void KEY_HYS_OFF(void) { (PORTC &= ~(1<<7)); }
static inline bool KEY_POWER_STATE(void) { return (!(PORTF & (1<<7))); }

static inline void KEY_POWER_ON(void) {

    DDRB |= 0b00011111;  // change col pins output
    DDRC |= (1<<7);
    if (is_980c) {
        DDRD |= 0b01110000;  
    } else {
        DDRD |= 0b11110000;  // change row pins output
    }
    DDRF |= (1<<7); PORTF &= ~(1<<7);    // PMOS switch on
    /* Without this wait you will miss or get false key events. */
    _delay_us(100);                       // wait for powering up, no DC so it may need less delay.
}

static inline void KEY_POWER_OFF(void) {
    /* input with pull-up consumes less than without it when pin is open. */
    DDRB  &= ~0b00011111;
    PORTB |=  0b00011111;
    DDRC  &= ~(1<<7);
    PORTC |=  (1<<7);
    if (is_980c) {
        DDRD  &= ~0b01100000;
        PORTD |=  0b01100000;
    } else {
        DDRD  &= ~0b11110000;
        PORTD |=  0b11110000;
    }
    
    DDRF |= (1<<7); PORTF |= (1<<7);    // PMOS switch off

}

static inline void KEY_INIT(void)
{
    KEY_POWER_ON();
    /* Key: input with pull-up */
    DDRC  &= ~(1<<6);
    PORTC |=  (1<<6);


    KEY_UNABLE();
    KEY_HYS_OFF();
}
static inline void SET_ROW(uint8_t ROW)
{
    if (is_980c) {
        if (ROW >= 4) ROW++;
        // set row with unabling key  B0 D6 D5
        (ROW & (1<<0)) ? (PORTD |= (1<<5)) : (PORTD &= ~(1<<5));
        (ROW & (1<<1)) ? (PORTD |= (1<<6)) : (PORTD &= ~(1<<6));
        (ROW & (1<<2)) ? (PORTB |= (1<<0)) : (PORTB &= ~(1<<0));
    } else {
        // set row with unabling key
        PORTD = (PORTD & 0x0F) | (1<<7) | ((ROW & 0x07) << 4);
    }
}
static inline void SET_COL(uint8_t COL)
{
    if (is_980c) {
        //PB1-4
        PORTB = (PORTB & 0b11100001) | ((COL<<1) & 0b00011110);
    } else {
        //         |PB3(Z5 ~EN)|PB4(Z4 ~EN)
        // --------|-----------|-----------
        // Col:0-7 |high       |low
        // Col:8-F |low        |high
        PORTB = (PORTB & 0xE0) | ((COL & 0x08) ? 1<<4 : 1<<3) | (COL & 0x07);
    }
}
#endif
