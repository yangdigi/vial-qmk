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


/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static void select_row(uint8_t row);
static uint8_t get_key(uint8_t col);

static void init_cols(void);

inline static void select_all_rows(void) {
    select_row(0xff);
}

__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}


void hook_early_init()
{
    {
        // PD1 for BLE Reset, PB3 for BT_SW
        DDRD  &= ~(1<<1);
        PORTD |=  (1<<1);
        DDRB  &= ~(1<<3);
        PORTB |=  (1<<3);
        _delay_ms(2);
        if ((~PINB & (1<<3))) ble51_boot_on = 0;
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // PD3,TX_MCU to RX_51
                DDRD  |=  (1<<3 );
                PORTD &= ~(1<<3);
                bt_power_init();
                // light CapsLED
                DDRB  |= (1<<2);
                PORTB |= (1<<2);
                _delay_ms(5000);
                bootloader_jump();
            }
        }
    }
}

void matrix_init(void)
{
    DDRB  |=  (1<<2);
    //PORTB &= ~(1<<2);
    init_cols();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;

    uint8_t matrix_keys_down = 0;

    uint8_t *debounce = &matrix_debouncing[0][0];
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        select_row(row);
        _delay_us(6);
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++) {
            uint8_t key = get_key(col);
            *debounce = (*debounce >> 1) | key;

            //if ((*debounce > 0) && (*debounce < 255)) {
            if (1) {
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
    }
        //unselect_rows();

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

static const struct AVR_PINS col_PIN[] = { PD(6), PD(7), PB(4), PB(5), PB(6), PC(6), PC(7), PE(2), PF(7), PF(6), PB(0), PF(5) };
static const struct AVR_PINS row_PIN[] = {PF(4), PF(1), PF(0), PE(6)};

/* Column pin configuration
 *  col: 0  1  2  3  4  5  6  7  8  9  10  
 *  pin: D6 D7 B4 B5 B6 C6 C7 E2 F7 F6 F5 
 */
static void  init_cols(void)
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
void unselect_rows(void)
{
}

static void select_row(uint8_t row)
{
    for (uint8_t i=0; i<MATRIX_ROWS; i++) {
        if (row == i || row == 0xff) _SFR_IO8(row_PIN[i].pin + 1) |= row_PIN[i].mask;
        else _SFR_IO8(row_PIN[i].pin+1) &= ~row_PIN[i].mask;
    }
}

bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState >= 10) {
        uint8_t matrix_keys_down = matrix_scan();
        if (matrix_keys_down == 2) {
            // K14 F, K17 J
            if (matrix_debouncing[1][4] == 0xff && matrix_debouncing[1][7] == 0xff) return true;
        }
        if (!ble51_boot_on) return true;
        return false;

    } else {
        select_all_rows();
        _delay_us(6);
        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            if (get_key(i)) {
                return true;
            }
        }
    }
    return false;
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

