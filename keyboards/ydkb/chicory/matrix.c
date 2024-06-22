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
#include <avr/io.h>
#include <util/delay.h>
#include "action.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "debounce_pk.h"
#include "ble51.h"
#include "ble51_task.h"
#include "rgblight.h"
#include "backlight.h"
#include "timer.h"
#include "wait.h"

bool is_ble_version = 1;
bool is_ver2 = 0;
bool no_backlight = 0;
bool no_rgblight = 0;

extern rgblight_config_t rgblight_config;
static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t now_debounce_dn_mask = DEBOUNCE_NK_MASK;
static bool matrix_idle = false;
static bool first_key_scan = false;
static bool is_matrix_active(void);

static void select_row(uint8_t row);
static void select_all_rows(void);
static uint8_t get_key(uint8_t col);
static void init_cols(void);
static void unselect_rows(void);

static void init_cols(void);
__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}

void hook_early_init(void) {
    DDRD  &= ~(1<<0);
    PORTD |=  (1<<0);
    WAIT_MS(2);
    if (~PIND & (1<<0)) {
        is_ver2 = 1;
        no_backlight = 1;
        no_rgblight = 1;
    }
    if (pgm_read_byte(0x7ff9) == 0xB0) {
        is_ble_version = 0;
        ble51_boot_on = 0;
    } else {
        if (ble_reset_key == 0xBBAA) {
            if (ble51_boot_on) {
                // PD3,TX_MCU to RX_51
                DDRD  |=  (1<<3 );
                PORTD &= ~(1<<3);
                bt_power_init();
                // Light CapsLED
                DDRB  |= (1<<7);
                PORTB |= (1<<7);
                _delay_ms(5000);
                // turn off bt and ready to reinit it.
                turn_off_bt();
            }
        }
    }
} 

void matrix_init(void)
{
    //software ble reset
    if (ble_reset_key == 0xBBAA) {
        ble_reset_key = 0;
        ble51_factory_reset();
    }
    
    // led init
    DDRB  |=  (1<<7);
    PORTB &= ~(1<<7);

    init_cols();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    if (matrix_idle) {
        if (is_matrix_active() == false) return 1;
        else {
            matrix_idle = false;
            first_key_scan = true;
        }
    }

    //scan matrix every 1ms except first_key_scan. maybe not necessary for some avr kbd.
    #if 1
    if (!first_key_scan) {
        uint16_t time_check = timer_read();
        if (matrix_scan_timestamp == time_check) return 1;
        matrix_scan_timestamp = time_check;
    }
    #endif

    uint8_t *debounce = &matrix_debouncing[0][0];
    uint8_t matrix_up_keys = MATRIX_ROWS * MATRIX_COLS;
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        select_row(row);
        _delay_us(6); // // without this wait read unstable value. tmk default value is 30. 10 is OK.
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++) {
            uint8_t key = get_key(col);
            *debounce = (*debounce >> 1) | key;


            //if ((*debounce > 0) && (*debounce < 0xff)) {
            if (1) {  //always update matrix[row], it costs only a little time.
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << col);
                if        (*debounce >= now_debounce_dn_mask) {  //debounce KEY DOWN 
                    *p_row |=  col_mask;
                } else if (*debounce <= DEBOUNCE_UP_MASK) { //debounce KEY UP
                    *p_row &= ~col_mask;
                    if (*debounce == 0) matrix_up_keys--;
                } 
            }
        }

        //if key pressed, update kb_idle_times
        //if (matrix[row] > 0) {
        //    if (!rgblight_config.enable && !backlight_config.enable) kb_idle_times = 12; 
        //    else kb_idle_times = 0;
        //}
    }

    // 最终检测一次是不是完全没有按键按下
    if (matrix_up_keys == 0) {
        select_all_rows();
        matrix_idle = true;
    } else {
        if (!rgblight_config.enable) kb_idle_times = 12; 
        else kb_idle_times = 0;
        // 从空闲到检测第一个按键时多扫一次
        if (first_key_scan) {
            first_key_scan = false;
            matrix_scan();
        }
        //unselect_rows();
    }
    
    matrix_scan_quantum();
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
    print("\nr/c 0123456789ABCDEF\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        print_hex8(row); print(": ");
        print_bin_reverse16(matrix_get_row(row));
        print("\n");
    }
}

uint8_t matrix_key_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        count += bitpop16(matrix[i]);
    }
    return count;
}

static const struct AVR_PINS col_PIN[] = { PD(6), PD(7), PB(4), PB(5), PB(6), PC(6), PC(7), PE(2), PF(7), PF(6), PF(5) };
static const struct AVR_PINS row_PIN[] = {PF(4), PF(1), PF(0), PE(6)};

/* Column pin configuration
 *  col: 0  1  2  3  4  5  6  7  8  9  10  
 *  pin: D6 D7 B4 B5 B6 C6 C7 E2 F7 F6 F5 
 */
void  init_cols(void)
{
    for (uint8_t col=0; col<MATRIX_COLS; col++) {
        _SFR_IO8(col_PIN[col].pin + 1) &= ~col_PIN[col].mask;
        _SFR_IO8(col_PIN[col].pin + 2) |=  col_PIN[col].mask;
    }
}
/* Column pin configuration
 *  col: 0  1  2  3  4  5  6  7  8  9  10  
 *  pin: D6 D7 B4 B5 B6 C6 C7 E2 F7 F6 F5 
 */
static uint8_t get_key(uint8_t col)
{
    return (_SFR_IO8(col_PIN[col].pin) & col_PIN[col].mask) ? 0:0x80;
}


/* Row pin configuration
 * row: 0   1   2   3   
 * pin: F4  F1  F0  E6  
 */
static void unselect_rows(void)
{
}

static void select_row(uint8_t row)
{
    //0xff for select_all_rows
    for (uint8_t i=0; i<MATRIX_ROWS; i++) {
        if (row == i || row == 0xff) _SFR_IO8(row_PIN[i].pin + 1) |= row_PIN[i].mask;
        else _SFR_IO8(row_PIN[i].pin+1) &= ~row_PIN[i].mask;
    }
}

static void select_all_rows(void)
{
    select_row(0xff);
}

static bool is_matrix_active(void) {
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        if (get_key(i)) {
            return true;
        }
    }
    return false;
}

bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState >= 10) {
        matrix_scan();
        // K14 F, K17 J
        uint8_t *debounce = &matrix_debouncing[0][0];
        uint8_t matrix_keys_down = 0;
        for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
            if (*debounce > 0) {
                if (i == KP(1,4) || i == KP(1,7)) matrix_keys_down += 100;
                else matrix_keys_down++;
            }
        }
        if (matrix_keys_down == 200) { // only two keys down.
            if (!ble51_boot_on) command_extra(KC_W);
            return true;
        } else if (!ble51_boot_on && matrix_keys_down) return true; //蓝牙功能关闭时唤醒电脑
    } else {
        select_all_rows();
        _delay_us(6);
        return is_matrix_active();
    }
    return false;
}

void bootmagic_lite(void)
{
    //do nothing
    return;

}


void hook_nkro_change(void) {
    //return;
    uint8_t kbd_nkro = keymap_config.nkro;
    if (!kbd_nkro) {  // kbd_nkro before hook_nkro_change()
        now_debounce_dn_mask = DEBOUNCE_NK_MASK;
    } else {
        now_debounce_dn_mask = DEBOUNCE_DN_MASK;
    }
    type_num(kbd_nkro?6:0);
}