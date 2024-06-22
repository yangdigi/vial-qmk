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


// 这个时间即使加到800，980c也无法正常。但是980c加入22uf的电容后，200也可以。
static uint8_t matrix_start_delay_timer = 200;

// matrix state buffer(1:on, 0:off)
static matrix_row_t *matrix;
static matrix_row_t *matrix_prev;
static matrix_row_t _matrix0[MATRIX_ROWS] = {0};
static matrix_row_t _matrix1[MATRIX_ROWS] = {0};

static uint8_t matrix_keys_down = 0;

static uint16_t matrix_scan_timestamp = 0;
static uint8_t fc_matrix_rows;

struct {
    uint8_t rows;
    uint8_t final_col;
} hhkb_matrix;

void matrix_scan_user(void) {}
__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}

static uint8_t wake_scan = 0;

bool is_980c = 0;

void hook_early_init()
{
#if 0 // 使用PF6判断 980C，舍弃此方法
    DDRF &= ~(1<<PF6);
    PORTF |= (1<<PF6);
    _delay_ms(2);
    is_980c = (PINF&(1<<PF6))? 0 : 1;
#endif
    // 从BL的文件名判断是否为980C
    for (uint16_t i=0x7fff-1024; i < 0x7ffff; i++) {
        //BLE9, 42 4C 45 39, BLE6,36
        //if (pgm_read_byte(i)   != 0x42) continue; // B
        if (pgm_read_byte(i) != 0x4C) continue; // L
        if (pgm_read_byte(++i) != 0x45) continue; // E
        if (pgm_read_byte(++i) == 0x39) is_980c = 1;
        break;
    }

    /* led init */
    led_all_off();
        
    // always try hardware reset.
    if (1) { 
        // PB7 input, hi-z for v25
        DDRB  &= ~(1<<7);
        PORTB |=  (1<<7);
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            // PB7 to RESET_51 for v2.2 
            // PD3,TX_MCU to RX_51 and connected to RESET_51 through 10K resistance.
            DDRB  |=  (1<<7);
            PORTB &= ~(1<<7);
            DDRD  |=  (1<<3);
            PORTD &= ~(1<<3);
            bt_power_init();
            #if 0
            if (is_980c) {
                PORTD |= (1<<7);
                PORTB |= (1<<6 | 1<<5);
            } else {
                PORTB &= ~(1<<6 | 1<<5);
            }
            #endif
            _delay_ms(5000);
            turn_off_bt();
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
    //for (uint8_t i=0; i < MATRIX_ROWS; i++) _matrix0[i] = 0x00;
    //for (uint8_t i=0; i < MATRIX_ROWS; i++) _matrix1[i] = 0x00;
    matrix = _matrix0;
    matrix_prev = _matrix1;

    fc_matrix_rows = is_980c? 7 : 5;
}

uint8_t matrix_started(void) {
    if (matrix_start_delay_timer) {
        uint16_t time_check = timer_read();
        if (matrix_scan_timestamp != time_check) {
            matrix_scan_timestamp = time_check;
            if (matrix_start_delay_timer == 1) KEY_INIT();
            matrix_start_delay_timer--;
        }
        return 0;
    }
    return 1;
}

uint8_t matrix_scan(void)
{
    if (matrix_started() == 0) return 1; // 延迟启动，这样可以不要钽电容了，但还是保留。
    
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
        //after a whole scan, check F and J when LOCK MODE
        // 980c, matrix[2,11] is F, matrix[2,12] is J
        // 660c, matrix[4,4] is F, matrix[4,5] is J
        if (matrix_keys_down == 2) { // only two keys down.

            //if (matrix[2] == (1<<12 | 1<<11) || matrix[4]  == (1<<5 | 1<<4)) {
            if (matrix[2] == (1<<12 | 1<<11)) {
                return 100;
            }
        }
    }
    matrix_scan_quantum();

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
    if (BT_POWERED == 0) {
        if (++sleep_timer < 80) return false;
        else sleep_timer = 0;
    }
    
    KEY_POWER_ON();
    wake_scan = 1;
    matrix_scan();
    //tp1685 fix
    if (BT_POWERED == 0) {
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
    led_all_off();
}

void suspend_wakeup_init_action(void)
{
    if (BT_POWERED == 0) {
        matrix_start_delay_timer = 200; // matrix board works delay
        display_connection_status_check_times = 1;
    }
}

void hook_keyboard_loop()
{
    if (BLE51_PowerState > 1) return;
    static uint8_t steps = 0;
    static uint16_t battery_timer = 0;
    if (timer_elapsed(battery_timer) > 160) {
        battery_timer = timer_read();

        // all led, default off.
        led_all_off();

        // low_battery and display_connection_status when ble51_on
        if (ble51_boot_on && (low_battery || display_connection_status_check_times)) {
            // 320ms on,320ms off. bt connected: 320ms*3 on, 320ms off.
            if (low_battery) {
                if (steps & 0b10) {
                    if (is_980c) {
                        PORTD |= (1<<7);
                        PORTB |= (1<<6 | 1<<5);
                    } else {
                        PORTB &= ~(1<<6 | 1<<5);
                    }
                }
            } else {
                if (bt_connected) {
                    if (steps & 0b110) {
                        if (is_980c) PORTB |= (1<<6 | 1<<5);
                        else PORTB &= ~(1<<6 | 1<<5);
                    }
                } else {
                    if (steps & 0b10) {
                        if (is_980c) PORTB |= (1<<5);
                        else PORTB &= ~(1<<6 | 1<<5);
                    }
                }
            }
        } else {
            if (host_keyboard_leds() &  (1<<USB_LED_NUM_LOCK)) {
                if (is_980c) PORTD |= (1<<7);
            }
    
            if (host_keyboard_leds() &  (1<<USB_LED_CAPS_LOCK)) {
                if (is_980c) PORTB |= (1<<6);
                else PORTB &= ~(1<<5);
            }
    
            if (host_keyboard_leds() &  (1<<USB_LED_SCROLL_LOCK)) {
                if (is_980c) PORTB |= (1<<5);
                else PORTB &= ~(1<<6);
            }
        }
        steps++;
    }
}

void bootmagic_lite(void)
{
    //do nothing
    return;
}

void hook_nkro_change(void)
{
    return;
    uint8_t kbd_nkro = ble51_usb_nkro;
    type_num(kbd_nkro?6:0);
}