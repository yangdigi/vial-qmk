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
#include "wait.h"
#include "action_layer.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "timer.h"
#include "matrix.h"
#include "debounce_pk.h"
#include "suspend.h"
#include "rgblight.h"
#include "ble51.h"
#include "ec_matrix.h"


extern struct cRGB rgbled[];

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};

__attribute__((unused)) static void select_row(uint8_t row);
__attribute__((unused)) static void select_col(uint8_t col);
void unselect_rows(void);
static uint8_t get_key(uint8_t row, uint8_t col);
static void init_cols(void);
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}

#if CONSOLE_ENABLE
#define DEBUG_SCAN_SPEED 1
uint16_t scan_speed=0;
#endif
//#define DEBUG_SCAN_SPEED 1

void hook_early_init()
{
    DDRC = (1<<7);
    PORTC = (1<<6);
    WAIT_MS(6);
    if (~PINC & (1<<6)) {
        // USB Only version
        ble51_boot_on = 0;
    } else {
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // BLE Reset, PD3 TX
                DDRD  |=  (1<<3);
                PORTD &= ~(1<<3);
                bt_power_init();
                rgblight_timer_enable();
                rgbled[0] = (struct cRGB){ .r = 255, .g = 0, .b = 0 };
                ws2812_setleds(rgbled);
                _delay_ms(5000);
                bootloader_jump();
            }
        }
    }
    DDRC = 0;
}

void matrix_init(void)
{
    ec_matrix_init();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    uint8_t matrix_keys_down = 0;
    for (uint8_t col=0; col<MATRIX_COLS; col++) {
        ec_select_col(col);
        for (uint8_t row=0; row<MATRIX_ROWS; row++) {
            uint8_t *debounce = &matrix_debouncing[row][col];

            uint8_t key = get_key(row, col);
            if (key != 0b10) {
                *debounce = (*debounce >> 1) | key;
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << col);
                if        (*debounce >= DEBOUNCE_DN_MASK) {
                    *p_row |=  col_mask;
                } else if (*debounce <= DEBOUNCE_UP_MASK) {
                    *p_row &= ~col_mask;
                }
            }
            if (*debounce) matrix_keys_down++;
        }
    }

    if (matrix_keys_down) {
        if (BLE_LIGHT_ON == 0) kb_idle_times = 12;
        else kb_idle_times = 0;
        if (matrix_keys_down >= 20) memset(matrix, 0, sizeof(matrix));
    }

#if defined(DEBUG_SCAN_SPEED) || defined(APC_ADJ_ENABLE)
    static uint16_t test=0;
    static uint16_t test_timestamp = 0;
    test++;
    if (timer_elapsed(test_timestamp) >= 1000) {
        test_timestamp = timer_read();
        #ifdef DEBUG_SCAN_SPEED
        scan_speed = test;
        #endif
        #ifdef APC_ADJ_ENABLE
        ec_apc_update();
        #endif
        test = 0;
      #if CONSOLE_ENABLE
        ec_matrix_print();
      #endif
    }
#endif
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
    return;
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

static uint8_t get_key(uint8_t row, uint8_t col)
{
    return ec_get_key(row, col);
}

void unselect_rows(void)
{
}

static void select_row(uint8_t row)
{
}

#include "eeprom.h"
#include "via.h"

#ifndef UNLOCK_KEY1
#define UNLOCK_KEY1 KC_F
#endif
#ifndef UNLOCK_KEY2
#define UNLOCK_KEY2 KC_J
#endif

bool suspend_wakeup_condition(void)
{
    uint8_t matrix_keys_down = matrix_scan();
    if (matrix_keys_down == 0) return false;

    if (BLE51_PowerState>= 10) {//lock mode
        if (matrix_keys_down == 2) {
            uint8_t *debounce = &matrix_debouncing[0][0];
            for (uint8_t i=0; i< sizeof(matrix_debouncing); i++, *debounce++) {
                if (*debounce > 0) {
                    // Read F and J from dynamic keymap layer0 in eeprom
                    // qmk use Big-Endian, so addr+1
                    uint8_t unimap_offset = i*2;
                    uint8_t checking_key = eeprom_read_byte(VIA_EEPROM_CONFIG_END+1 + unimap_offset);
                    if (checking_key != UNLOCK_KEY1 && checking_key != UNLOCK_KEY2) return false; //if not f j
                    if (*debounce != 0xff) return false; //press f+j longer.
                }
            }
            // not return false, then true.
            if (!ble51_boot_on) command_extra(KC_W);
            return true;
        }
        if (!ble51_boot_on) return true;
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
