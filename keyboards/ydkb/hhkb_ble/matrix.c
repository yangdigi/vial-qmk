/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

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
#include "print.h"
#include "debug.h"
#include "util.h"
#include "action.h"
#include "command.h"
#include "keyboard.h"
#include "timer_avr.h"
#include "matrix.h"
#include "suspend.h"
#include "lufa.h"
#include "ble51.h"
#include "ble51_task.h"
#include "switch_board.h"



// matrix state buffer(1:on, 0:off)
static matrix_row_t *matrix;
static matrix_row_t *matrix_prev;
static matrix_row_t _matrix0[MATRIX_ROWS];
static matrix_row_t _matrix1[MATRIX_ROWS];

static uint8_t matrix_keys_down = 0;
struct {
    uint8_t rows;
    uint8_t final_col;
} hhkb_matrix;


static uint8_t wake_scan = 0;

bool is_ver_jp = 0;
bool is_ver25 = 0;
void hook_early_init()
{
    /* led init */
    DDRF  |=  (1<<PF4 |1<<PF1 | 1<<PF0);
    PORTF &= ~(1<<PF4 |1<<PF1 | 1<<PF0);
    
    //jp v25b
    DDRF  &= ~(1<<5);
    PORTF |=  (1<<5);
    //if jp 
    DDRE  &= ~(1<<PE2);
    PORTE |=  (1<<PE2);
    _delay_ms(2);
    is_ver_jp = (PINE&(1<<PE2))? 0 : 1;
        
    // if v25
    if (pgm_read_byte(0x7eb4) == 0xa0 || (~PINF & (1<<PF5))) {
        is_ver25 = 1;
        // PD1 input, hi-z for v25
        DDRD  &= ~(1<<PD1);
        PORTD |=  (1<<PD1);
        
        // if ble reset is needed for v25
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            
            // PD1 output low for 5s
            DDRD  |=  (1<<PD1);
            PORTD &= ~(1<<PD1);
            bt_power_init();
            //light led1(PF4) led3(PF0)
            DDRF  |= (1<<PF4 | 1<<PF0);
            PORTF |= (1<<PF4 | 1<<PF0);
            _delay_ms(5000);
            bootloader_jump(); 
        }
    }
}

void ble51_init_blename(void)
{
    if (is_ver_jp) ble51_set_blename("HHKB JP BLE\n");
    else ble51_set_blename("HHKB BLE\n");
}

void matrix_init(void)
{
    //software ble reset for ver24 and ver23
    if (ble_reset_key == 0xBBAA) {
        ble_reset_key = 0;
        ble51_factory_reset();
    }

    KEY_INIT();

    // initialize matrix state: all keys off
    for (uint8_t i=0; i < MATRIX_ROWS; i++) _matrix0[i] = 0x00;
    for (uint8_t i=0; i < MATRIX_ROWS; i++) _matrix1[i] = 0x00;
    matrix = _matrix0;
    matrix_prev = _matrix1;

    if (is_ver_jp) {
        hhkb_matrix.rows = 16;
        hhkb_matrix.final_col  = 4; //2-6 to 0-4
    } else {
        hhkb_matrix.rows = 8;
        hhkb_matrix.final_col  = 7;
    }
}

uint8_t matrix_scan(void)
{
    matrix_row_t *tmp;

    tmp = matrix_prev;
    matrix_prev = matrix;
    matrix = tmp;
    
    matrix_keys_down = 0;

    for (uint8_t r = 0; r < hhkb_matrix.rows; r++) {
        for (uint8_t c = 0; c <= hhkb_matrix.final_col; c++) {


            KEY_SELECT(r, c);
            _delay_us(3);

            uint8_t row = r;
            uint8_t col = c;
            if (is_ver_jp) {
                if (matrix_trans[r][c] == 0x27) continue;
                row = matrix_trans[r][c] >> 4;
                col = matrix_trans[r][c] & 0xf;
            }
            
            if (matrix_prev[row] & (1<<col)) {
                KEY_PREV_ON();
            }
            _delay_us(10);

            // NOTE: KEY_STATE is valid only in 20us after KEY_ENABLE.
            // If V-USB interrupts in this section we could lose 40us or so
            // and would read invalid value from KEY_STATE.
            uint8_t last = TIMER_RAW;

            KEY_ENABLE();

            // Wait for KEY_STATE outputs its value.
            _delay_us(3); 

            if (KEY_STATE()) {
                matrix[row] &= ~(1<<col);
            } else if (!wake_scan) { // wake_scan时不更新matrix[], 否则tp1685在二级节能时会被唤醒
                matrix_keys_down++;
                matrix[row] |= (1<<col);
            }

            // Ignore if this code region execution time elapses more than 20us.
            // MEMO: 20[us] * (TIMER_RAW_FREQ / 1000000)[count per us]
            // MEMO: then change above using this rule: a/(b/c) = a*1/(b/c) = a*(c/b)
            if (TIMER_DIFF_RAW(TIMER_RAW, last) > 20/(1000000/TIMER_RAW_FREQ)) {
                matrix[row] = matrix_prev[row];
            }

            if (matrix[row] > 0) {
                if (BLE51_PowerState < 2) {
                    kb_idle_times = 0;
                } else if (BLE51_PowerState < 10) {
                    return 100;
                }
            }

            _delay_us(5);
            KEY_PREV_OFF();
            KEY_UNABLE();

            if (BLE51_PowerState >= 2) {
                _delay_us(10); // scan faster when power saving
                if (wake_scan && row == 7 && col == 6) { // wake scan complete
                    wake_scan = 0;
                    return 1;
                }
            } else {
                // NOTE: KEY_STATE keep its state in 20us after KEY_ENABLE.
                // This takes 25us or more to make sure KEY_STATE returns to idle state.
                _delay_us(30);
            }
        }
    }

    if (BLE51_PowerState >= 10) { 
        if (matrix_keys_down == 2) {
            if (matrix[1]  == (1<<5) && matrix[4] == (1<<5)) {
                return 100;
            }
        }
    }

    return 1;
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
bool suspend_wakeup_condition(void)
{
    static uint8_t sleep_timer = 0;
    if (BLE51_PowerState >= 4) {
        if (++sleep_timer < 80) return false;
        else sleep_timer = 0;
    }
    
    KEY_POWER_ON();
    wake_scan = 1;
    matrix_scan();
    if (matrix_scan() == 100) {
        return true;
    }
    KEY_POWER_OFF();
    return false;
}

void suspend_power_down_action(void)
{
    KEY_POWER_OFF();
    PORTF &= ~(1<<PF4 |1<<PF1 | 1<<PF0); // turn off all leds.
}

void suspend_wakeup_init_action(void)
{
    if (BLE51_PowerState >=4) {
        PORTF |= (1<<PF4 |1<<PF1 | 1<<PF0);
        _delay_ms(400);
        display_connection_status_check_times = 1;
    }
}

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
                if (PINF&(1<<PF6)) { //not charging
                    if (PORTF & (1<<4)) PORTF &= ~(1<<PF4 |1<<PF1 | 1<<PF0);
                    else PORTF |= (1<<PF4 |1<<PF1 | 1<<PF0);; //all off
                } else {
                    low_battery = 0;
                    suspend_wakeup_init_action();
                }
            }
        } else if (display_connection_status_check_times) {
            if (ble51_task_steps == 1) {
                PORTF &= ~(1<<1 | 1<<0);
            } else if (ble51_task_steps == 3) {
                PORTF |= (1<<0);
                if (bt_connected) PORTF |= (1<<1);
            }
            if ( (!bt_connected && ble51_task_steps >= 5) || ble51_task_steps >= 11 ) {
                ble51_task_steps = 0;
            }
        }
    }
}
