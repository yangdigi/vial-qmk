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

/*
 * scan matrix
 */
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "command.h"
#include "timer.h"
#include "matrix.h"
#include "debounce_pk.h"
#include "suspend.h"
#include "lufa.h"
#include "rgblight.h"
#include "ble51.h"
#include "ble51_task.h"
#include "switch_board.h"

#define DOUBLE_CLICK_FIX_DELAY 10

bool is_ble_version = 1;


extern debug_config_t debug_config;
extern rgblight_config_t rgblight_config;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t matrix_double_click_fix[MATRIX_ROWS][MATRIX_COLS] = {0};
#ifdef DEFAULT_6KRO
static uint8_t now_debounce_dn_mask = DEBOUNCE_DN_MASK;
#else
static uint8_t now_debounce_dn_mask = DEBOUNCE_NK_MASK;
#endif
static bool matrix_idle = false;
static bool first_key_scan = false;

static void select_key(uint8_t mode);
static void select_all_keys(void);
static uint8_t get_key(void);
static void init_cols(void);

__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}

void hook_early_init()
{
    // PB4 for BLE Check;
    DDRB  &= ~(1<<4);
    PORTB |=  (1<<4);
    WAIT_MS(2);
    if (PINB & (1<<4)) {
        // USB Only version
        is_ble_version = 0;
        ble51_boot_on = 0;
        // light LED3
        //DDRD  |= (1<<4);
        //PORTD |= (1<<4);
        //_delay_ms(1000);
    } else {
        is_ble_version = 1;
        //DDRD  |= (1<<7);
        //PORTD |= (1<<7);
        //_delay_ms(1000);
        // PE2 for BT_SW
        DDRE  &= ~(1<<2);
        PORTE |=  (1<<2);
        // PB4 for BLE Check; PB5 for BLE Reset
        DDRB  &= ~(1<<5);
        PORTB |=  (1<<5);
        WAIT_MS(10); // 2ms时似乎有问题，可能一直是 ble51_boot_on = 1
        if (~PINE & (1<<2)) ble51_boot_on = 0;
        else ble51_boot_on = 1;
    
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // PB6 for BLE Reset
                DDRB  |=  (1<<5);
                PORTB &= ~(1<<5);
                bt_power_init();
                // light LED3
                DDRD  |= (1<<4);
                PORTD |= (1<<4);
                _delay_ms(5000);
                bootloader_jump(); 
            }
        }
    }
    // led init
    led_fn_init();
    //DDRD  |=  (1<<7 | 1<<6 | 1<<4);
    //PORTD &= ~(1<<7 | 1<<6 | 1<<4);
}

void matrix_init(void)
{
    init_cols();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    matrix_scan_quantum(); // qmk needs this to run hook_keyboard_loop()

    if (matrix_idle) {
        if (get_key() == 0) return 1;
        else {
            matrix_idle = false;
            first_key_scan = true;
        }
    }
    if (!first_key_scan) {
        //scan matrix every 1ms
        uint16_t time_check = timer_read();
        if (matrix_scan_timestamp == time_check) return 1;
        matrix_scan_timestamp = time_check;
    }

    select_key(0);
    uint8_t *debounce = &matrix_debouncing[0][0];
    uint8_t *double_click_fix = &matrix_double_click_fix[0][0];
    uint8_t one_scan_down = 0;
    uint8_t matrix_up_keys = MATRIX_ROWS * MATRIX_COLS;
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++, *double_click_fix++) {
            uint8_t key = get_key();
            *debounce = (*debounce >> 1) | key;
            //select next key
            select_key(1);
            if (1) {
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << col);
                if (*double_click_fix > 0 && (*p_row & col_mask) == 0) {
                    (*double_click_fix)--;
                } else {
                    if        (*debounce > now_debounce_dn_mask) {  //debounce KEY DOWN 
                        *p_row |=  col_mask;
                        *double_click_fix = DOUBLE_CLICK_FIX_DELAY; 
                    } else if (*debounce < DEBOUNCE_UP_MASK) { //debounce KEY UP
                        *p_row &= ~col_mask;
                        matrix_up_keys--;
                    }
                }
            }
        }
    }

    // 最终检测一次是不是完全没有按键按下
    if (matrix_up_keys == 0) {
        select_all_keys();
        matrix_idle = true;
    } else {
        kb_idle_times = 0;
        // 从空闲到检测第一个按键时多扫一次
        if (first_key_scan) {
            first_key_scan = false;
            #if DEBOUNCE_NK > 0
            matrix_scan();
            #endif
        }
    }

    return 1;
}


inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & ((matrix_row_t)1<<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
    print("\nr/c 01234567\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        print_hex8(row); print(": ");
        print_bin_reverse8(matrix_get_row(row));
        print("\n");
    }
}

uint8_t matrix_key_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        count += bitpop8(matrix[i]);
    }
    return count;
}

void init_cols(void)
{
    //595 pin
    DDRB  |=  (1<<3 | 1<<1);
    DDRB  &= ~(1<<2);
    PORTB |=  (1<<3 | 1<<2 | 1<<1);
}


static uint8_t get_key(void) {
    return PINB&(1<<2) ? 0 : 0x80;
}


void select_all_keys(void)
{
    DS_PL_LO();
    for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS; i++) {
        CLOCK_PULSE();
    }
}

void unselect_rows(void)
{
}

static void select_key(uint8_t mode)
{
    if (mode == 0) {
        DS_PL_HI();
        for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS; i++) {
            CLOCK_PULSE();
        }
        DS_PL_LO();
        CLOCK_PULSE();
    } else {
        DS_PL_HI();
        CLOCK_PULSE();
    }
    _delay_us(3);
}


bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState>= 10) {  //lock mode  
        matrix_scan();
        //  K95 F， K75 J
        uint8_t *debounce = &matrix_debouncing[0][0];
        uint8_t matrix_keys_down = 0;
        for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
            if (*debounce > 0) {
                if (i == KP(7,5) || i == KP(9,5)) matrix_keys_down += 100;
                else matrix_keys_down++;
            }
        }
        if (matrix_keys_down == 200) {
            return true;
        }
        else if (!ble51_boot_on && matrix_keys_down) return true;
    } else {
        //check all keys
        select_all_keys();
        _delay_us(5);
        if (get_key()) { //
            return true;
        }
    }
    return false;
}

void hook_nkro_change(void) {
    uint8_t kbd_nkro = (keyboard_protocol & (1<<4));
    if (!kbd_nkro) {  // kbd_nkro before hook_nkro_change()
        now_debounce_dn_mask = DEBOUNCE_NK_MASK;
    } else {
        now_debounce_dn_mask = DEBOUNCE_DN_MASK;
    }
    type_num(kbd_nkro?6:0);
}