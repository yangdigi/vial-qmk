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
static uint8_t fc_matrix_rows;

static uint8_t wake_scan = 0;

bool is_980c = 0;
bool is_ver22 = 0;


void hook_early_init()
{
    DDRF &= ~(1<<PF6);
    PORTF |= (1<<PF6);
    _delay_ms(2);
    is_980c = (PINF&(1<<PF6))? 0 : 1;

    /* led init */
    DDRB  |= (1<<PB6 |1<<PB5);
    if (is_980c) {
        PORTB &= ~(1<<PB6 |1<<PB5);
        DDRD  |=  (1<<PD7);
        PORTD &= ~(1<<PD7);
    } else {
        PORTB |= (1<<PB6 |1<<PB5);
    }
        
    // v22, check ble reset
    if (pgm_read_byte(0x681f) == 0xcf) {
        is_ver22 = 1;
        // PB7 input, hi-z for v25
        PORTB |=  (1<<PB7);
        DDRB  &= ~(1<<PB7);
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            DDRB  |=  (1<<PB7);
            PORTB &= ~(1<<PB7);
            bt_power_init();
            if (is_980c) {
                PORTD |= (1<<PD7);
                PORTB |= (1<<PB6 | 1<<PB5);
            } else {
                PORTB &= ~(1<<PB6 | 1<<PB5);
            }
            _delay_ms(5000);
            bootloader_jump(); 
        }

    } 
}

void ble51_init_blename(void)
{
    if (is_980c) ble51_set_blename("BLE980C\n");
    else ble51_set_blename("BLE660C\n");
}

void matrix_init(void)
{
    //software ble reset
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

    fc_matrix_rows = is_980c? 7 : 5;
}

uint8_t matrix_scan(void)
{
    matrix_row_t *tmp;

    tmp = matrix_prev;
    matrix_prev = matrix;
    matrix = tmp;
    
    matrix_keys_down = 0;

    for (uint8_t r = 0; r < fc_matrix_rows; r++) {
        SET_ROW(r);
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            uint8_t row = r;
            uint8_t col = c;
            if (!is_980c) {
                if (matrix_trans[r][c] == 0x68) continue;
                row = matrix_trans[r][c] >> 4;
                col = matrix_trans[r][c] & 0xf;
            }

            SET_COL(c);
            _delay_us(3);

            
            if (matrix_prev[row] & (1<<col)) {
                KEY_HYS_ON();
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
            KEY_HYS_OFF();
            KEY_UNABLE();

            if (BLE51_PowerState >= 2) {
                _delay_us(10); // scan faster when power saving
                if (r == 3 && wake_scan) { // wake scan for nothing
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
        if (matrix_keys_down == 2) { // only two keys down.
            if (matrix[2] == (1<<12 | 1<<11)) {
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
    //tp1685 fix
    if (BLE51_PowerState >= 4) {
        matrix_scan();
    }
    if (matrix_scan() == 100) {
        return true;
    }
    KEY_POWER_OFF();
    return false;
}

void suspend_power_down_action(void)
{
    KEY_POWER_OFF();
    if (is_980c) {
        PORTD &= ~(1<<7);
        PORTB &= ~(1<<6 | 1<<5); // turn off all leds.
    } else {
        PORTB |= (1<<6 | 1<<5); // turn off all leds.
    }
}

void suspend_wakeup_init_action(void)
{
    if (BLE51_PowerState >=4) {
        if (is_980c) {
            PORTD |= (1<<7);
            PORTB |= (1<<6 |1<<5);
        } else {
            PORTB &= ~(1<<6 |1<<5);
        }
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
                if (PINE&(1<<PE2)) { //not charging
                    if (PORTB & (1<<6)) {
                        PORTB &= ~(1<<6 |1<<5);  //660c on, 980c off
                        if (is_980c) PORTD &= ~(1<<7); //980c led1 off
                    } else {
                        PORTB |= (1<<6 |1<<5); //660c off, 980c on
                        if (is_980c) PORTD |= (1<<7);
                    }
                } else {
                    low_battery = 0;
                    suspend_wakeup_init_action();
                }
            }
        } else if (display_connection_status_check_times) {
            if (ble51_task_steps == 1) {
                if (is_980c) {
                    PORTB &= ~(1<<6 | 1<<5);
                } else {
                    PORTB |= (1<<6 | 1<<5);
                }
            }
            if (ble51_task_steps == 3) {
                if (is_980c) {
                    PORTB |= (1<<5);
                    if (bt_connected) PORTB |= (1<<6);
                } else {
                    PORTB &= ~(1<<6 | 1<<5);
                }
            }
            if ( (!bt_connected && ble51_task_steps >= 5) || ble51_task_steps >= 11 ) {
                ble51_task_steps = 0;
            }
        }
    }
}
