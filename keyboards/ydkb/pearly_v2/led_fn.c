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
#include "rgblight.h"
#include "command.h"
#include "ble51.h"

void indicator_task(void)
{
    static uint8_t steps = 0;
    if ((steps++ & 0b11)) return;
    // run every 4*40 = 160ms
    PORTB &= ~(1<<2); //caps off

    bool indicator_need_update = 0;
    // low_battery and display_connection_status when ble51_on
    if (ble51_boot_on && (low_battery || display_connection_status_check_times)) {
        // 320ms on,320ms off. bt connected: 320ms*3 on, 320ms off.
        if (low_battery) {
            rgblight_timer_disable(); 
            if (steps & 0b1000) PORTB |= (1<<2);
        } else {
            indicator_need_update = 1;
            if (bt_connected) {
                if (steps & 0b11000) { 
                    PORTB |= (1<<2);
                    rgbled[0].g = 128;
                }
            } else {
                if (steps & 0b1000)  {
                    PORTB |= (1<<2);
                    rgbled[0].b = 128;
                }
            }
            //所有灯用于连接指示
            rgblight_set_all_as(&rgbled[0]);
            //rgblight_setrgb(rgbled[0].r, rgbled[0].g, rgbled[0].b);
        }
    }
    // capslock
    else if (host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK)) {
        PORTB |= (1<<2);
    }

    if (indicator_need_update) {
        rgblight_timer_enable();
        //直接更新，防止连接指示灯或低电量也fading
        ws2812_setleds(rgbled);
        // 清除，为下一次更新做准备。也避免连接指示后，底灯依然处于亮的状态。
        rgblight_clear();
    } else if (rgblight_config.enable == 0) {
        //rgblight off and no indicator
        rgblight_timer_disable();
    }
}

void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        static const uint8_t userx_to_command[4] = {
            KC_U, // 0 Host Switch 
            KC_B, // 1 Reset
            KC_V, // 2 Output Battery Value
            KC_L  // 3 Lock Mode
        };
        // RGB_TOG 0x5cc1, RGB_VAI 0x5cc9, RGB_VAD 0x5cca
        // Toggle, M+, M-, HUE+, HUE-, SAT+, SAT-, VAL+, VAL-
        if (keycode >= RGB_TOG && keycode <= RGB_VAD) rgblight_action(keycode - RGB_TOG);
        else if (keycode >= USER00) {
            if (keycode < USER04) command_extra(userx_to_command[keycode-USER00]);
        }
    }
}