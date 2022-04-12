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
#include "stdint.h"
#include "quantum.h"
#include "led.h"
#include "rgblight.h"

bool led3_indicator_on = 0;
struct cRGB led3_indicator_color;

void led_set_user(uint8_t usb_led)
{
    if (usb_led & (1<<USB_LED_CAPS_LOCK)) {
        led3_indicator_on = 1;
        led3_indicator_color.r = 255;
        led3_indicator_color.g = 0;
        led3_indicator_color.b = 255;
    } else {
        led3_indicator_on = 0;
    }
    rgblight_set();
}

// rgblight control
// RGB_TOGGLE: 0x5cc2, Mode+:5cc3,Mode-:5cc4 ... VAL-:5cca
// USER00 - USER15, 0x5F80 - 0x5F8F
void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode >= 0x5f80 && keycode <= 0x5f88) {
        if (record->event.pressed) rgblight_action(keycode - 0x5f80);
    }
}