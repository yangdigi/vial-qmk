/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include "quantum.h"
#include "stdint.h"
#include "led.h"
#include "via.h"
#include "command.h"
#include "ble51.h"

extern bool is_980c;

void led_all_off(void) {
    DDRB  |= (1<<6 |1<<5);
    if (is_980c) {
        PORTB &= ~(1<<6 |1<<5);
        DDRD  |=  (1<<7);
        PORTD &= ~(1<<7);
    } else {
        PORTB |= (1<<6 |1<<5);
    }
}

#if 0
void led_set_user(uint8_t usb_led)
{
    if (is_980c) {
        PORTD &= ~(1<<7);
        PORTB &= ~(1<<6 | 1<<5);
    } else {
        PORTB |= (1<<6 | 1<<5);
    }

    if (usb_led &  (1<<USB_LED_NUM_LOCK)) {
        if (is_980c) PORTD |= (1<<7);
    }
    
    if (usb_led &  (1<<USB_LED_CAPS_LOCK)) {
        if (is_980c) PORTB |= (1<<6);
        else PORTB &= ~(1<<5);
    }
    
    if (usb_led &  (1<<USB_LED_SCROLL_LOCK)) {
        if (is_980c) PORTB |= (1<<5);
        else PORTB &= ~(1<<6);
    }
}
#endif

void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        static const uint8_t userx_to_command[4] = {
            KC_U, // 0 Host Switch 
            KC_B, // 1 Reset
            KC_V, // 2 Output Battery Value
            KC_L  // 3 Lock Mode
        };
        if (keycode >= USER00 && keycode < USER04) command_extra(userx_to_command[keycode-USER00]);
    }
}