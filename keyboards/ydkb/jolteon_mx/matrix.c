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

/* rough code */

#include "ch.h"
#include "hal.h"

/*
 * scan matrix
 */
#include "action.h"
#include "print.h"
#include "debug.h"
#include "timer.h"
#include "util.h"
#include "matrix.h"
#include "debounce_pk.h"
#include "wait.h"
#include "ec_matrix.h"
#include "rgblight.h"



/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};

void unselect_rows(void);
static uint8_t get_key(uint8_t row, uint8_t col);
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) 
{
    matrix_scan_user();
}

#if CONSOLE_ENABLE
#define DEBUG_SCAN_SPEED 1
uint16_t scan_speed=0;
#endif
//#define DEBUG_SCAN_SPEED 1

void matrix_init(void)
{
    //rgblight_init();
    ec_matrix_init();
}

static bool process_key_press = 0;
bool should_process_keypress(void)
{
    return process_key_press;
}

uint8_t matrix_scan(void)
{
    uint8_t matrix_keys_down = 0;
    uint8_t matrix_keys_down_row[MATRIX_ROWS] = {0};
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
            if (*debounce) {
                matrix_keys_down_row[row]++;
                matrix_keys_down++;
            }
        }
    }

    memcpy(keys_down_row, matrix_keys_down_row, sizeof(keys_down_row));

    process_key_press = (keys_down_row[0] < MATRIX_COLS);

#if DEBUG_SCAN_SPEED
    static uint16_t test=0;
    static uint16_t test_timestamp = 0;
    test++;
    if (timer_elapsed(test_timestamp) >= 1000) {
        test_timestamp = timer_read();
        scan_speed = test;
        //ec_apc_update();
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


void bootmagic_lite(void)
{
    return;
}

void early_hardware_init_pre(void)
{
    palSetPadMode(GPIOA, 12, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOA, 12);
    for (uint32_t i = 0; i < 800000; i++) {
        __asm__("nop");
    }
}
