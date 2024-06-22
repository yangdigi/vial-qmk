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
#include "ble51.h"
#include "ble51_task.h"
#include "switch_board.h"

bool is_ver595 = 1;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS] = {0};
static matrix_row_t matrix_v595[MATRIX_ROWS] = {0};
static matrix_row_t matrix_v11[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t matrix_debouncing_v11[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t matrix_double_click_fix[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t matrix_double_click_fix_v11[MATRIX_ROWS][MATRIX_COLS] = {0};
#ifdef DEFAULT_6KRO
static uint8_t now_debounce_dn_mask = DEBOUNCE_DN_MASK;
#else
static uint8_t now_debounce_dn_mask = DEBOUNCE_NK_MASK;
#endif
static void matrix_scan_ver595(void);
static void matrix_scan_ver11(void);

static uint8_t get_key(uint8_t col);
static void init_cols(void);
static void select_row(uint8_t row);
static void unselect_rows(void);
static void select_all_rows(void);

__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
  matrix_scan_user();
}

void hook_early_init()
{
    {
        // PE6 for BLE Reset, PE2 for BT_SW
        DDRD  &= ~(1<<1);
        PORTD |=  (1<<1);
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // PE6 for BLE Reset
                DDRD  |=  (1<<1);
                PORTD &= ~(1<<1);
                bt_power_init();
                // light CapsLED
                DDRC  |= (1<<6);
                PORTC |= (1<<6);
                _delay_ms(5000);
                bootloader_jump(); 
            }
        }
    }
}

void matrix_init(void)
{
    DDRC  |=  (1<<6);
    PORTC &= ~(1<<6);

    //debug_config.enable = true;


    // initialize row and col
    // init_cols();
    // initialize matrix state: all keys off
}


uint8_t matrix_scan(void)
{
    //scan matrix every 1ms
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;

    matrix_scan_ver595();
    
    matrix_scan_ver11();

    matrix_scan_quantum();

    return 1;
}

static void matrix_scan_ver595(void) {
    // scan for ver595
    is_ver595 = 1;
    init_cols();
    select_row(10);  // 10 for 595 key0
    uint8_t *debounce = &matrix_debouncing[0][0];
    uint8_t *double_click_fix = &matrix_double_click_fix[0][0];
    uint8_t one_scan_down = 0;
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++, *double_click_fix++) {
            uint8_t real_col = col/2;
            if (col & 1) real_col += 8;

            uint8_t key = get_key(real_col);
            *debounce = (*debounce >> 1) | key;


            if (real_col >= 8) select_row(11); //11 for 595 next key

            //if ((*debounce > 0) && (*debounce < 0xff)) {
            if (1) {  //always update matrix[row], it costs only a little time.
                uint8_t row_trans = row;
                uint8_t col_trans = real_col;
                if (row >= 2) {
                    row_trans = matrix_trans[row-2][real_col] >> 4;
                    col_trans = matrix_trans[row-2][real_col] & 0xf;
                }

                matrix_row_t row_prev  = matrix_v595[row_trans];
                matrix_row_t *p_row = &matrix_v595[row_trans];
                matrix_row_t col_mask = ((matrix_row_t)1 << col_trans);
                if (*double_click_fix > 0 && (*p_row & col_mask) == 0) {
                    (*double_click_fix)--;
                } else {
                    if        (*debounce > now_debounce_dn_mask) {  //debounce KEY DOWN 
                        *p_row |=  col_mask;
                        *double_click_fix = DOUBLE_CLICK_FIX_DELAY; 
                    } else if (*debounce < DEBOUNCE_UP_MASK) { //debounce KEY UP
                        *p_row &= ~col_mask;
                    }
                }
                if (*p_row != row_prev) {
                    matrix[row_trans] = matrix_v595[row_trans];
                }
            }
        }
        //if key pressed, update kb_idle_times
        if (matrix[row] > 0) {
            kb_idle_times = 0; 
        }
    }
}

static void matrix_scan_ver11(void) {
    // scan for ver11    
    is_ver595 = 0;
    init_cols();
    uint8_t *debounce = &matrix_debouncing_v11[0][0];
    uint8_t *double_click_fix = &matrix_double_click_fix_v11[0][0];
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        select_row(row);
        _delay_us(6);
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++, *double_click_fix++) {
            uint8_t key = get_key(col);
            *debounce = (*debounce >> 1) | key;

            //if ((*debounce_v11 > 0) && (*debounce_v11 < 0xff))  {
            if (1) {
                matrix_row_t row_prev = matrix_v11[row]; //save prev value of this row
                matrix_row_t *p_row = &matrix_v11[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << col);
                if (*double_click_fix > 0 && (*p_row & col_mask) == 0) {
                    (*double_click_fix)--;
                } else {
                    if        (*debounce > now_debounce_dn_mask) {  //debounce KEY DOWN 
                        *p_row |=  col_mask;
                        *double_click_fix = DOUBLE_CLICK_FIX_DELAY; 
                    } else if (*debounce < DEBOUNCE_UP_MASK) { //debounce KEY UP
                        *p_row &= ~col_mask;
                    }
                }
                if (*p_row != row_prev) {
                    matrix[row] = matrix_v11[row];
                }                    
            }
        }
        //if key pressed, update kb_idle_times
        if (matrix[row] > 0) {
            kb_idle_times = 0; 
        }
        unselect_rows();
    }
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

static const struct AVR_PINS col_PIN[] = { PB(5), PB(6), PE(2), PB(4), PF(6), PE(6), PB(1), PB(2), PB(0), PB(3), PB(7), PF(0), PF(1), PF(4), PF(5) };
static const struct AVR_PINS row_PIN[] = { PD(0), PD(5), PD(4), PD(6), PD(7) };

/* Column pin configuration
 *  col: 0  1   2  3  4  5  6  7  8  9  10 11 12 13 14
 *  pin: B5 B6 E2 B4 F6 E6 B1 B2 B0 B3  B7 F0 F1 F4 F5
 */
/* Row pin configuration
 * row: 0   1   2   3   4 
 * pin: D0  D5  D4  D6  D7
 */

static void init_cols(void)
{
    if (is_ver595) {
        //595 pin
        DDRF  |=  (1<<6 | 1<<4);
        //key(col) pin
        DDRF  &= ~(1<<5 | 1<<1);
        PORTF |=  (1<<6 | 1<<5 | 1<<4 | 1<<1);
    } else {
        DDRF  &= ~(1<<6 | 1<<5 | 1<<4 | 1<<1 | 1<<0);
        PORTF |=  (1<<6 | 1<<5 | 1<<4 | 1<<1 | 1<<0);
        DDRE  &= ~(1<<6 | 1<<2);
        PORTE |=  (1<<6 | 1<<2);
        DDRB  &= ~0xff;
        PORTB |=  0xff;
    }
    unselect_rows();
    //_delay_us(12);
}


static uint8_t get_key(uint8_t col) {
    if (is_ver595) {
        if (col<8) return PINF&(1<<5) ? 0 : 0x80;
        else return PINF&(1<<1) ? 0 : 0x80;
    } else if (col<15) {
        return (_SFR_IO8(col_PIN[col].pin) & col_PIN[col].mask) ? 0:0x80;
    }
    return 0;
}


static void unselect_rows(void)
{
    DDRD  &= ~0b11110001;
}

static void select_all_rows(void)
{
    DDRD  |=  0b11110001;
}

static void select_row(uint8_t row)
{
    if (is_ver595) {
        if (row == 10) {
            DS_PL_HI();
            for (uint8_t i = 0; i < 40; i++) {
                CLOCK_PULSE();
            }
            DS_PL_LO();
            CLOCK_PULSE();
        } else if (row == 11) {
            DS_PL_HI();
            CLOCK_PULSE();
        }
        _delay_us(3);
    } else {
        _SFR_IO8(row_PIN[row].pin + 1) |=  row_PIN[row].mask;
    }
}


bool suspend_wakeup_condition(void)
{
    uint8_t verx_has_key_pressed = 0;

    // ver595 check all keys
    is_ver595 = 1;
    init_cols();
    DS_PL_LO();
    for (uint8_t i = 0; i < 40; i++) {
        CLOCK_PULSE();
    }
    _delay_us(6);
    if ( (PINF&0b100010) < 0b100010) { //
        verx_has_key_pressed = 10;
    }

    // ver 11
    if (verx_has_key_pressed == 0) {
        is_ver595 = 0;
        init_cols();
        select_all_rows();
        _delay_us(6);
        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            if (get_key(i)) {
                verx_has_key_pressed = 11;
                break;
            }
        }
    }
    if (verx_has_key_pressed && BLE51_PowerState >= 10) {  //lock mode
        uint8_t matrix_keys_down = 0;
        // ver595(1.0) lock mode
        if (verx_has_key_pressed == 10) {
            // ver595: Key1_S24 F (debounce[2][8]),  Key2_K20 J (debounce[2][1])
            //memset(matrix_debouncing, 0, sizeof(matrix_debouncing));
            matrix_scan_ver595();
            uint8_t *debounce = &matrix_debouncing[0][0];
            for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
                if (*debounce > 0) {
                    if (i == KP(2,1) || i == KP(2,8)) matrix_keys_down += 100;
                    else matrix_keys_down++;
                }
            }
        } else { // verx_has_key_pressed == 11)
            // ver11: K24 F, K27 J
            //memset(matrix_debouncing_v11, 0, sizeof(matrix_debouncing_v11));
            matrix_scan_ver11();
            uint8_t *debounce_v11 = &matrix_debouncing_v11[0][0];
            for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce_v11++) {
                if (*debounce_v11 > 0) {
                    if (i == KP(2,4) || i == KP(2,7)) matrix_keys_down += 100;
                    else matrix_keys_down++;
                }
            }
        }
        if (matrix_keys_down == 200) {
            if (!ble51_boot_on) command_extra(KC_W);
            return true;
        } else if (!ble51_boot_on && matrix_keys_down) return true;
        else return false; //has_key_pressed but not wake up in lock mode.
    }
    return verx_has_key_pressed;
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

#ifdef SUSPEND_ACTION
void suspend_power_down_action(void)
{
    PORTC &= ~(1<<6);  //caps led off
}

void suspend_wakeup_init_action(void)
{
}
#endif


void ble51_task_user(void)
{
    static uint8_t ble51_task_steps = 0;
    static uint16_t battery_timer = 0;
    if (timer_elapsed(battery_timer) > 150) {
        battery_timer = timer_read();
        ble51_task_steps++;
        if (low_battery) {
            if (ble51_task_steps > 3) {
                ble51_task_steps = (low_battery == 1)? 3:0; //value 1 is extremely low battery
                if (PINC & (1<<7)) { //not charging
                    PORTC ^= (1<<6);
                } else {
                    low_battery = 0;
                    suspend_wakeup_init_action();
                }
            }
        } else if (display_connection_status_check_times) {
            if (ble51_task_steps == 1) {
                PORTC &= ~(1<<6);
            }
            if (ble51_task_steps == 3) {
                PORTC |= (1<<6);
            }
            if ((!bt_connected && ble51_task_steps >= 5) || ble51_task_steps >= 11) {
                ble51_task_steps = 0;
            }
        }
    }
}

void bootmagic_lite(void) {
    matrix_scan();
    wait_ms(10);
    matrix_scan();

    if (matrix_get_row(1) == (1<<3 | 1<<0)) {  //tab and E
        bootmagic_lite_reset_eeprom();
        // Jump to bootloader.
        bootloader_jump();
    }
}

