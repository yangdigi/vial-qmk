/*
Copyright 2023 YANG <drk@live.com>

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
#include <stdbool.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "matrix.h"
#include "action.h"
#include "suspend.h"
#include "timer.h"
#include "send_string.h"
#include "recore.h"

// 使用 send_string 里的 send_nibble()， 进行数字输出。
void type_numbers(uint16_t value)
{
#if 1 //save 24B
    static char numbers[5] = {0};
    sprintf(numbers,"%d",value); // char 0 to 9, 48 to 57. but key code 1 to 9 to 0, 30 to 39.
    for (uint8_t i=0; i<5; i++) { // strlen(str) needs 14B more
        uint8_t code = numbers[i];
        if (code == '\0') break;
        else {
            if (code == 48) code = 58;
            tap_code(code - 19);
        }
    }
#else
    for (uint16_t i=10000; i>=1; i=i/10) {
        uint8_t this_num = value/i % 10;
        if (value/i) {
            send_nibble(this_num);
        }
    }
#endif
}

void keyboard_post_init_kb(void)
{
#ifdef VIAL_ENABLE
    vial_init();
#endif
}

//22B
void tap_code(uint8_t code) {
    register_code(code);
    if (code == KC_CAPS) wait_ms(100);
    unregister_code(code);
}

// 4B ever if (mods)
void register_mods(uint8_t mods) {
    add_mods(mods);
    send_keyboard_report();
}

void unregister_mods(uint8_t mods) {
    del_mods(mods);
    send_keyboard_report();
}

void register_weak_mods(uint8_t mods) {
    add_weak_mods(mods);
    send_keyboard_report();
}

void unregister_weak_mods(uint8_t mods) {
    del_weak_mods(mods);
    send_keyboard_report();
}
// if (mods) end. 4B*4 = 16B

__attribute__((weak))
void hook_nkro_change(void) {}
