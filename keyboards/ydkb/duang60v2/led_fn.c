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


void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        static const uint8_t userx_to_command[4] = {
            KC_U, // 0 Host Switch 
            KC_B, // 1 Reset
            KC_V, // 2 Output Battery Value
            KC_L  // 3 Lock Mode
        };
        if (keycode >= USER00) {
            if (keycode < USER04) command_extra(userx_to_command[keycode-USER00]);
            else if (keycode <= USER12) rgblight_action(keycode - USER04);
        }
    }
}