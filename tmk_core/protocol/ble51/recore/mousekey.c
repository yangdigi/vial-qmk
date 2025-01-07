/*
Copyright 2019 YANG

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

#include <stdint.h>
#include "keycode.h"
#include "host.h"
#include "timer.h"
#include "print.h"
#include "debug.h"
#include "mousekey.h"

#include "action_util.h"



report_mouse_t mouse_report = {};

static uint8_t mousekey_wheel_timer = 0;

static uint8_t mousekey_accel = 0;

static void mousekey_debug(void);

// rapidfire key

/*
 * Mouse keys  acceleration algorithm
 *  http://en.wikipedia.org/wiki/Mouse_keys
 *
 *  speed = delta * max_speed * (repeat / time_to_max)**((1000+curve)/1000)
 */
/* milliseconds between the initial key press and first repeated motion event (0-2550) */
//uint8_t mk_delay = MOUSEKEY_DELAY/10;
/* milliseconds between repeated motion events (0-255) */
//uint8_t mk_interval = MOUSEKEY_INTERVAL;
/* steady speed (in action_delta units) applied each event (0-255) */
//uint8_t mk_max_speed = MOUSEKEY_MAX_SPEED;
/* number of events (count) accelerating to steady speed (0-255) */
//uint8_t mk_time_to_max = MOUSEKEY_TIME_TO_MAX;
/* ramp used to reach maximum pointer speed (NOT SUPPORTED) */
//int8_t mk_curve = 0;
/* wheel params */
//uint8_t mk_wheel_max_speed = MOUSEKEY_WHEEL_MAX_SPEED;
//uint8_t mk_wheel_time_to_max = MOUSEKEY_WHEEL_TIME_TO_MAX;


static uint16_t mousekey_last_timer = 0;


void mousekey_task(void)
{
    //rapidfire_key_task();

    static uint8_t mouse_repeat_time_interval = 50;
    //delay about 50ms to start //(mouse_repeat ? mk_interval : mk_delay*10))
    if (timer_elapsed(mousekey_last_timer) < mouse_repeat_time_interval) return;
    
    //int8_t *p = &mouse_report.x;
    //if ((*p++ | *p++ | *p++ | *p++) == 0) {
    if ((mouse_report.x | mouse_report.y | mouse_report.v | mouse_report.h) == 0) {
        mouse_repeat_time_interval = 50;
        return;
    } else {
        mouse_repeat_time_interval = 5;
    }
    //control wheel speed
    //int8_t mouse_v, mouse_h;
    uint16_t *mouse_wheel = &mouse_report.v;
    uint16_t save = *mouse_wheel;
    //mouse_v = mouse_report.v;
    //mouse_h = mouse_report.h;
    if ((++mousekey_wheel_timer) & 0b1111) {
        *mouse_wheel = 0;
    }
    mousekey_send();
    *mouse_wheel = save;
    //mouse_report.v = mouse_v;
    //mouse_report.h = mouse_h;

    static uint8_t timer1 = 0;
    if (++timer1 > MOUSEKEY_REPEAT_UPDATE_INTERVAL) { //control mouse speedup
#if defined(BLE_NAME) || defined(BLE_NAME_VARIABLE)
    #if (SERIAL_UART_BAUD == 19200) 
        //默认REPEAT_INTERVAL是12，蓝牙5到12是7，使用19200时改到10
        timer1 = (keyboard_protocol & (1<<7))? 10:0;
    #else
        timer1 = (keyboard_protocol & (1<<7))? 5:0;
    #endif
#else
        timer1 = 0;
#endif
        int8_t  *p_report = &mouse_report.x;
        for (uint8_t i=0; i<4; i++, *p_report++) {
            if (*p_report == 0) continue;
            if (mousekey_accel) { // 定速移动
                *p_report = (MOUSEKEY_MOVE_REPEAT_MAX / 4) * mousekey_accel * (*p_report > 0? 1: -1); // mousekey_accel 1 2 4
            } else if (*p_report > 0) {
                if (++(*p_report) > MOUSEKEY_MOVE_REPEAT_MAX) {
                    *p_report = MOUSEKEY_MOVE_REPEAT_MAX;
                } 
            } else if (--(*p_report) < -MOUSEKEY_MOVE_REPEAT_MAX) {
                 *p_report = -MOUSEKEY_MOVE_REPEAT_MAX;
            }
        }
    }

}

void mousekey_on(uint8_t code)
{
    if (code < KC_MS_UP) return;
    if      (code == KC_MS_UP)       mouse_report.y = -1; 
    else if (code == KC_MS_DOWN)     mouse_report.y =  1;
    else if (code == KC_MS_LEFT)     mouse_report.x = -1; 
    else if (code == KC_MS_RIGHT)    mouse_report.x =  1;
    else if (code <= KC_MS_BTN5)     mouse_report.buttons |= (1<<(code-KC_MS_BTN1)); //0xF4 - 0xF8
    else if (code == KC_MS_WH_UP)    mouse_report.v =  1; 
    else if (code == KC_MS_WH_DOWN)  mouse_report.v = -1; 
    else if (code == KC_MS_WH_LEFT)  mouse_report.h = -1;
    else if (code == KC_MS_WH_RIGHT) mouse_report.h =  1;
    else if (code <= KC_MS_ACCEL2)   mousekey_accel |= (1<<(code-KC_MS_ACCEL0)); //0xFD - 0xFF
    if (code >= KC_MS_WH_UP && code <= KC_MS_WH_RIGHT) mousekey_wheel_timer = 0;
}

void mousekey_off(uint8_t code)
{
    if (code < KC_MS_UP) return;
    if      (code == KC_MS_UP      ) { if (mouse_report.y < 0) mouse_report.y = 0; }
    else if (code == KC_MS_DOWN    ) { if (mouse_report.y > 0) mouse_report.y = 0; }
    else if (code == KC_MS_LEFT    ) { if (mouse_report.x < 0) mouse_report.x = 0; }
    else if (code == KC_MS_RIGHT   ) { if (mouse_report.x > 0) mouse_report.x = 0; }
    else if (code <= KC_MS_BTN5    ) { mouse_report.buttons &= ~(1<<(code-KC_MS_BTN1)); }//0xF4 - 0xF8
    else if (code == KC_MS_WH_UP   ) { if (mouse_report.v > 0) mouse_report.v = 0; }
    else if (code == KC_MS_WH_DOWN ) { if (mouse_report.v < 0) mouse_report.v = 0; }
    else if (code == KC_MS_WH_LEFT ) { if (mouse_report.h < 0) mouse_report.h = 0; }
    else if (code == KC_MS_WH_RIGHT) { if (mouse_report.h > 0) mouse_report.h = 0; }
    else if (code <= KC_MS_ACCEL2  ) { mousekey_accel &= ~(1<<(code-KC_MS_ACCEL0)); } //0xFD - 0xFF

}

void mousekey_send(void)
{
    mousekey_debug();
    host_mouse_send(&mouse_report);
    mousekey_last_timer = timer_read();
}

void mousekey_clear(void)
{
    mouse_report = (report_mouse_t){};
    mousekey_accel = 0;
}

static void mousekey_debug(void)
{
    if (!debug_mouse) return;
    print("mousekey [btn|x y v h](rep/acl): [");
    print_hex8(mouse_report.buttons); print("|");
    print_decs(mouse_report.x); print(" ");
    print_decs(mouse_report.y); print(" ");
    print_decs(mouse_report.v); print(" ");
    print_decs(mouse_report.h); print("](");
    print_dec(mousekey_accel); print(")\n");
}
