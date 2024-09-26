/*
Copyright 2023 YANG

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
            KC_L  // C Lock Mode
        };
        if (keycode >= USER00) {
            if (keycode < USER04) command_extra(userx_to_command[keycode-USER00]);
            else if (keycode <= USER12) rgblight_action(keycode - USER04);
            else if (keycode == USER13) {
                send_nibble(0);
                send_nibble(0);
            }
        }
    }
#if 0
    static uint8_t mod_keys_registered;
    uint8_t pressed_mods = get_mods();
    switch (keycode) {
        case 0x5c00: // via/vial reset to bootloader
            if (record->event.pressed) {
                clear_keyboard();
            }
            return false;
        // 0x5f8f for Alt+Esc=f4 and RShift+Esc=~
        case 0x5F8F:
            if (record->event.pressed) {
                if ((pressed_mods & MOD_BIT(KC_RSHIFT)) && (~pressed_mods & MOD_BIT(KC_LCTRL))) {
                    mod_keys_registered = KC_GRV;
                } else if (pressed_mods & MOD_BIT(KC_LALT)) {
                    mod_keys_registered = KC_F4;
                } else {
                    mod_keys_registered = KC_ESC;
                }
                register_code(mod_keys_registered);
                send_keyboard_report();
            } else {
                unregister_code(mod_keys_registered);
                send_keyboard_report();
            }
            return false;
        default:
            return true; // Process all other keycodes normally
    }
#endif
}
