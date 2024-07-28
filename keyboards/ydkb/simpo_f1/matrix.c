/*
Copyright 2022 YANG <drk@live.com>

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
#include "rgblight.h"

extern debug_config_t debug_config;

static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t now_debounce_dn_mask = DEBOUNCE_NK_MASK;

static uint8_t get_key(uint8_t col);

static void init_cols(void);
static void select_row(uint8_t row);
static void unselect_rows(void);


__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void)
{
    matrix_scan_user();
    hook_keyboard_loop();
}

void matrix_init(void)
{
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

    debug_config.enable = 1;
    debug_config.matrix = 0;

    init_cols();
    static LED_TYPE RGBLIGHT_COLOR_OFF   = { .r = 0, .g = 0, .b = 0 };
    ws2812_setleds(&RGBLIGHT_COLOR_OFF,1);
}

uint8_t matrix_scan(void)
{
    matrix_scan_quantum();
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;


    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        select_row(row);
        wait_us(20);
        for (uint8_t col=0; col<MATRIX_COLS; col++) {
            uint8_t *debounce = &matrix_debouncing[row][col];
            uint8_t key = get_key(col);
            *debounce = (*debounce >> 1) | key;

            if (1) {
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << col);
                if        (*debounce >= now_debounce_dn_mask) {
                    *p_row |=  col_mask;
                } else if (*debounce <= DEBOUNCE_UP_MASK) {
                    *p_row &= ~col_mask;
                } 
            }
        }

        unselect_rows();
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

}

uint8_t matrix_key_count(void)
{
    return 0;
}

/* Column pin configuration
 *  col: 0   1   2   3   4   5   6   7   8   9   10  11  12  13 14
 *  pin: B15 A2  B0  B1  B10 B11 B12 B13 B14 B9  B8  B3  B4  B5 A15
 */
static void  init_cols(void)
{
    palSetGroupMode(GPIOA, (1<<15|1<<2), 0, PAL_MODE_INPUT_PULLUP);
    palSetGroupMode(GPIOB, 0b1111111100111011, 0, PAL_MODE_INPUT_PULLUP);
    
}
 
static uint8_t get_key(uint8_t col)
{
    switch (col) {
        case 0: return (palReadPad(GPIOB, 15)==PAL_HIGH) ? 0 : 0x80;
        case 1: return (palReadPad(GPIOA,  2)==PAL_HIGH) ? 0 : 0x80;
        case 2: return (palReadPad(GPIOB,  0)==PAL_HIGH) ? 0 : 0x80;
        case 3: return (palReadPad(GPIOB,  1)==PAL_HIGH) ? 0 : 0x80;
        case 4 ... 8: return (palReadPad(GPIOB, col+6)==PAL_HIGH) ? 0 : 0x80;
        case 9 ... 10: return (palReadPad(GPIOB, 18-col)==PAL_HIGH) ? 0 : 0x80;
        case 11 ... 13: return (palReadPad(GPIOB, col-8)==PAL_HIGH) ? 0 : 0x80;
        case 14: return (palReadPad(GPIOA, 15)==PAL_HIGH) ? 0 : 0x80;
        default: return 0;
    }
}

/* Row pin configuration
 *  row: 0   1  2  3  4  5 
 *  pin: A3  A4 A5 A6 B2 A8
 */
static void unselect_rows(void)
{
    palSetGroupMode(GPIOA,  0b01111000, 0, PAL_MODE_INPUT);
    palSetPadMode(GPIOB,  2, PAL_MODE_INPUT);
}

static void select_row(uint8_t row)
{
    if (row == 4) {
        palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);
        palClearPad(GPIOB, 2);
    } else {
        palSetPadMode(GPIOA, row+3, PAL_MODE_OUTPUT_PUSHPULL);
        palClearPad(GPIOA, row+3);
    }
}

void early_hardware_init_pre(void)
{
    // Override hard-wired USB pullup to disconnect and reconnect
    palSetPadMode(GPIOA, 12, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOA, 12);
    for (uint32_t i = 0; i < 800000; i++) {
        __asm__("nop");
    }
}
