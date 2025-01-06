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
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "avr_config.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "action.h"
#include "command.h"
#include "keyboard.h"
#include "timer.h"
#include "matrix.h"
#include "debounce_pk.h"
#include "suspend.h"
#include "lufa.h"
#include "rgblight.h"
#include "ble51.h"
#include "ble51_task.h"
#include "switch_board.h"


static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t now_debounce_dn_mask = DEBOUNCE_NK_MASK;
static void select_key(uint8_t mode);
static uint8_t get_key(uint8_t col);

static void init_cols(void);
__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}

static void get_key_ready(void) {
    DDRB  &= ~(1<<3);
    PORTB |=  (1<<3);
    _delay_us(6);
}

static inline void select_key_ready(void) {
    DDRB |= (1<<3);
} 

void hook_early_init()
{
    if (pgm_read_byte(0x7e65) == 0x4c) {
        // USB Only version
        ble51_boot_on = 0;
    } else {
        // PF7 for BLE Reset, PE2 for BT_SW
        DDRF  &= ~(1<<PF7);
        PORTF |=  (1<<PF7);
        DDRE  &= ~(1<<PE2);
        PORTE |=  (1<<PE2);
        _delay_ms(2);
        if (~PINE & (1<<PE2)) ble51_boot_on = 0;
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // PF7 for BLE Reset
                DDRF  |=  (1<<PF7);
                PORTF &= ~(1<<PF7);
                bt_power_init();
                DDRE  |= (1<<PE6);
                PORTE |= (1<<PE6);
                DDRB  |= (1<<PB2);
                PORTB |= (1<<PB2);
                _delay_ms(5000);
                bootloader_jump();
            }
        }
    }
}

void matrix_init(void)
{
    // led init
    DDRE  |=  (1<<PE6);
    //PORTE &= ~(1<<PE6);
    DDRF  |=  (1<<PF0);
    //PORTF &= ~(1<<PF0);
    DDRB  |=  (1<<PB2);
    //PORTB &= ~(1<<PB2);

    init_cols();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    
    //uint16_t time_check = timer_read();
    //if (matrix_scan_timestamp == time_check) return 1;
    //matrix_scan_timestamp = time_check;
    uint8_t matrix_keys_down = 0;

    select_key(0);
    uint8_t *debounce = &matrix_debouncing[0][0];
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++) {
            uint8_t real_col = col/2; //col 0-7
            if (col & 1) real_col += 8; //col 8-15

            uint8_t key = get_key(real_col);
            *debounce = (*debounce >> 1) | key;

            if (real_col >= 8) select_key(1);

            //if ((*debounce > 0) && (*debounce < 255)) {
            if (1) {
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << real_col);
                if        (*debounce >= DEBOUNCE_DN_MASK) {  //debounce KEY DOWN
                    *p_row |=  col_mask;
                } else if (*debounce <= DEBOUNCE_UP_MASK) { //debounce KEY UP
                    *p_row &= ~col_mask;
                }
            }
            if (*debounce) matrix_keys_down++;
        }
    }

    if (matrix_keys_down) {
        #if 0
        if (BLE_LIGHT_ON == 0) kb_idle_times = 12;
        else 
        #endif
        kb_idle_times = 0;
    }

    matrix_scan_quantum();
    return matrix_keys_down;
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

void init_cols(void)
{
    //595 pin
    DDRB  |=  (1<<PB3 | 1<<PB1);
    //PORTB |=  (1<<PB3 | 1<<PB1);
    //key1(col) pin,  key2 is PB3
    DDRF  &= ~(1<<PF1);
    PORTF |=  (1<<PF1);
}


static uint8_t get_key(uint8_t col)
{
    if (col<8) return PINF&(1<<PF1) ? 0 : 0x80;
    else return PINB&(1<<PB3) ? 0 : 0x80;
}


void unselect_rows(void)
{
}

void select_key(uint8_t mode)
{
    select_key_ready();

    DS_PL_HI();
    if (mode == 0) {
        for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS / 2; i++) {
            CLOCK_PULSE();
        }
        DS_PL_LO();
    }
    CLOCK_PULSE();

    get_key_ready();
}


bool suspend_wakeup_condition(void)
{
    uint8_t matrix_keys_down = matrix_scan();
    if (matrix_keys_down == 0) return false;

    if (BLE51_PowerState >= 10) {//lock mode
        if (matrix_keys_down == 2) {
            // Key1_S14 F,   real debounce p is 1*16+4*2
            // Key2_K14 J,   real debounce p is 1*16+4*2+1
            if (matrix_debouncing[1][8] == 0xff && matrix_debouncing[1][9] == 0xff) return true;
        }
        if (!ble51_boot_on) return true; //蓝牙功能关闭时唤醒电脑
        return false;
    }

    return matrix_keys_down;
}

void bootmagic_lite(void)
{
    //do nothing
    return;

}

void hook_nkro_change(void)
{
    return;
    uint8_t kbd_nkro = keymap_config.nkro;
    type_num(kbd_nkro?6:0);
}