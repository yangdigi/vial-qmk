/*
Copyright 2022 YANG

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
#include "rgblight.h"

void led_fn_init(void) {
    // led init
    DDRD  |=  (1<<7 | 1<<6 | 1<<4);
    PORTD &= ~(1<<7 | 1<<6 | 1<<4);
}

void led_set_user(uint8_t usb_led)
{
    if (usb_led &  (1<<USB_LED_NUM_LOCK)) {
        PORTD |= (1<<7);
    } else {
       PORTD &= ~(1<<7);
    }
    
    if (usb_led &  (1<<USB_LED_CAPS_LOCK)) {
        PORTD |= (1<<6);
    } else {
        PORTD &= ~(1<<6);
    }
    
    if (usb_led &  (1<<USB_LED_SCROLL_LOCK)) {
        PORTD |= (1<<4);
    } else {
        // output low
        PORTD &= ~(1<<4);
    }
}

void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case USER00:
                command_extra(KC_U);
                break;
            case USER01: //RESET
                command_extra(KC_B);
                break;
            case USER02: //BATTERY LEVEL
                command_extra(KC_V);
                break;
            case USER03: //LOCK MODE
                command_extra(KC_L); 
                break;
            case USER04 ... USER12:
                rgblight_action(keycode - USER04);
                break;
        }
    }
}