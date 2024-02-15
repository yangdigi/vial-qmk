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

#include "hal.h"
#include "ch.h"
#include "led.h"
#include "rgblight.h"

#include "stdint.h"
#include "quantum.h"

extern rgblight_config_t rgblight_config;
static LED_TYPE RGBLIGHT_COLOR_OFF   = { .r = 0, .g = 0, .b = 0 };
uint8_t indicator_state = 0;
#ifdef WELCOME_LIGHT
uint8_t welcome_light_on = 10;
#else
uint8_t welcome_light_on = 0;
#endif

LED_TYPE rgbled[INDICATOR_NUM+RGBLED_NUM];

void rgblight_call_driver(LED_TYPE *start_led, uint8_t num_leds) {
    // keep indicator color
#ifdef INDICATOR_0_FUNCT
    if (indicator_state & (1<<0)) {
        #ifdef INDICATOR_0_VAL
        sethsv(rgblight_config.hue, rgblight_config.sat, INDICATOR_0_VAL, &rgbled[0]);
        #else
        rgbled[0] = INDICATOR_0_COLOR;
        #endif
        #ifdef INDICATOR_0_INSTRIP
        start_led[INDICATOR_0_INSTRIP] = rgbled[0];
        #endif
    } else {
        rgbled[0] = RGBLIGHT_COLOR_OFF;
    }
#endif
#ifdef INDICATOR_1_FUNCT
    if (indicator_state & (1<<1)) {
        rgbled[1] = INDICATOR_1_COLOR;
        #ifdef INDICATOR_1_INSTRIP
        start_led[INDICATOR_1_INSTRIP] = INDICATOR_1_COLOR;
        #endif
    } else {
        rgbled[1] = RGBLIGHT_COLOR_OFF;
    }
#endif
#ifdef INDICATOR_2_FUNCT
    if (indicator_state & (1<<2)) {
        rgbled[2] = INDICATOR_2_COLOR;
        #ifdef INDICATOR_2_INSTRIP
        start_led[INDICATOR_2_INSTRIP] = INDICATOR_2_COLOR;
        #endif
    } else {
        rgbled[2] = RGBLIGHT_COLOR_OFF;
    }
#endif

    memcpy(&rgbled[INDICATOR_NUM], start_led, RGBLED_NUM*3);
    if (!welcome_light_on) ws2812_setleds(rgbled, INDICATOR_NUM+RGBLED_NUM);
}

void led_set_user(uint8_t usb_led)
{
    indicator_state = 0;
#ifdef INDICATOR_0_FUNCT
    if (usb_led & INDICATOR_0_FUNCT) {
        indicator_state |= (1<<0);
    }
#endif
#ifdef INDICATOR_1_FUNCT
    if (usb_led & INDICATOR_1_FUNCT) {
        indicator_state |= (1<<1);
    }
#endif
#ifdef INDICATOR_2_FUNCT
    if (usb_led & INDICATOR_2_FUNCT) {
        indicator_state |= (1<<2);
    }
#endif
    if (rgblight_config.mode == 1) rgblight_mode_noeeprom(rgblight_config.mode);
    rgblight_set(); //set rgb even when rgblight.enable=0
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static uint8_t mod_keys_registered;
    uint8_t pressed_mods = get_mods();
    switch (keycode) {
        case 0x5c00: // via/vial reset to bootloader
            if (record->event.pressed) {
                clear_keyboard();
                volatile uint32_t *uf2bl_backup_reg = (uint32_t*)0x20004000;
            	*uf2bl_backup_reg = 0x9d5bfc2bUL;
            	NVIC_SystemReset();
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
}

void enter_bootloader(void) {
    clear_keyboard();
    volatile uint32_t *uf2bl_backup_reg = (uint32_t*)0x20004000;
    *uf2bl_backup_reg = 0x9d5bfc2bUL;
    NVIC_SystemReset();
}
/* LShift+RShift+LCtrl+B to Bootloader */
#include "command.h"

bool command_extra(uint8_t code)
{
    uint8_t pressed_mods = get_mods();
    clear_keyboard();
    switch (code) {
        case KC_B:
            ;
            wait_us(500*1000);
            if (pressed_mods & MOD_BIT(KC_LCTRL)) {
                enter_bootloader();
            }
            //soft reset
            NVIC_SystemReset();
            //*(uint32_t *)(0xE000ED0CUL) = 0x05FA0000UL | (*(uint32_t *)(0xE000ED0CUL) & 0x0700) | 0x04;
            break;
        default:
            return false;   // yield to default command
    }
    return true;
}

//rgblight welcome
extern rgblight_config_t rgblight_config;
extern bool is_rgblight_initialized;
extern LED_TYPE led[];
void hook_keyboard_loop(void) {
#ifndef WELCOME_LIGHT
    return;
#endif
    //only for taco75
    //ready
    static LED_TYPE *led = &rgbled[INDICATOR_NUM];
    if (welcome_light_on == 10 && is_rgblight_initialized) {
        welcome_light_on = 4;
        rgblight_config.enable = 0;
        rgblight_config.mode = 0;
    }

    // show
    static uint16_t rgb_update_timer = 0;
    if (welcome_light_on && rgb_update_timer != timer_read() && timer_elapsed(rgb_update_timer) > 30) {
        rgb_update_timer = timer_read();

        static uint8_t welcome_step = 0;
        LED_TYPE welcome_led_color;
        sethsv(rgblight_config.hue, rgblight_config.sat, rgblight_config.val, &welcome_led_color);
        if (welcome_light_on == 2 || welcome_light_on == 4) {
            memset(led, 0, RGBLED_NUM*3);
        } else if (welcome_light_on == 1 || welcome_light_on == 3) {
            for (uint8_t i=0;i<RGBLED_NUM;i++) {
                led[i] = welcome_led_color;
            }
            welcome_led_color = RGBLIGHT_COLOR_OFF;
        }
        for (uint8_t i=0; i<welcome_step; i++) {
            led[6-i] = led[7+i] = welcome_led_color;
        }
        // force rgb update even rgblight off
        ws2812_setleds(rgbled, INDICATOR_NUM+RGBLED_NUM);
        if (welcome_step < 7) {
            static uint8_t led_step = 0;
            if (++led_step < 3) return;
            led_step = 0;
            welcome_step++;
        } else {
            welcome_step = 0;
            welcome_light_on--;
            //restore config
            if (welcome_light_on == 0) {
                rgblight_reload_from_eeprom();
            }
        }
    }
}