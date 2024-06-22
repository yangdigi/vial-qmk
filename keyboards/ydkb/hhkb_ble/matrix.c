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
#include "debounce_pk.h"
#include "suspend.h"
#include "lufa.h"
#include "ble51.h"
#include "ble51_task.h"
#include "switch_board.h"


// matrix state buffer(1:on, 0:off)
static matrix_row_t *matrix;
static matrix_row_t *matrix_prev;
static matrix_row_t _matrix0[MATRIX_ROWS] = {0};
static matrix_row_t _matrix1[MATRIX_ROWS] = {0};

static uint8_t matrix_keys_down = 0;


static uint8_t matrix_debouncing[90] = {0}; //JP matrix is 16*5. Pro is 8*8

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

bool is_ver_jp = 0;

void hook_early_init()
{
    /* led init */
    led_all_off();
    //check the eeprom layout if it is JP
    if (eeprom_read_byte(VIA_EEPROM_LAYOUT_OPTIONS_ADDR) == 1) is_ver_jp = 1;  //0 us, 1 jp

    // always try hardware reset.
    if (1) { 
        //if (pgm_read_byte(0x7eb4) == 0xa0 || (~PINF & (1<<PF5))) { //v2.5 bootloader
        // always set this, no need to know if it it ver2.5
        DDRD  &= ~(1<<PD1);
        PORTD |=  (1<<PD1);
        // if ble reset is needed for v25
        if (ble_reset_key == 0xBBAA) {
            // PD1 to RESET_51 for v2.5 
            // PD3,TX_MCU to RX_51 and connected to RESET_51 through 10K resistance.
            DDRD  |=  (1<<3 | 1<<1);
            PORTD &= ~(1<<3 | 1<<1);
            bt_power_init();
            //light led1(PF4) led3(PF0)
            //DDRF  |= (1<<PF4 | 1<<PF0);
            PORTF |= (1<<PF4 | 1<<PF0);
            _delay_ms(5000);
            // turn off bt and ready to reinit it.
            turn_off_bt();
        }
    }
}

void matrix_init(void)
{
    //software ble reset for ver24 and ver23
    if (ble_reset_key == 0xBBAA) {
        ble_reset_key = 0;
        ble51_factory_reset();
    }

    KEY_INIT();
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
    uint8_t *debounce = &matrix_debouncing[0];
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
            //_delay_us(10);

            // NOTE: KEY_STATE is valid only in 20us after KEY_ENABLE.
            // If V-USB interrupts in this section we could lose 40us or so
            // and would read invalid value from KEY_STATE.
            uint8_t last = TIMER_RAW;

            KEY_ENABLE();

            // Wait for KEY_STATE outputs its value.
            //_delay_us(3); 
            if (BLE51_PowerState < 2 || BLE51_PowerState >= 10) _delay_us(3);  // 必需，否则无法识别到按键或者连击。但是节能时可以不用。

            uint8_t key = (~PIND & (1<<7))? 0x80 : 0; // save key state
            
            *debounce = (*debounce >> 1) | key;
            KEY_PREV_OFF();
            KEY_UNABLE(); // 确定在return 100前执行

            // 恢复 debouce for keydown 
            if (!wake_scan) {
                // 这里只检查一级节能唤醒，避免660c从二级节能中误唤醒
                if ((BLE51_PowerState == 2) && key) {
                    return 100;
                }

                if (1) { //always update matrix[row], it costs only a little time.
                    if (*debounce >= DEBOUNCE_DN_MASK) {
                        kb_idle_times = 0;
                        matrix_keys_down++;
                        matrix[row] |= (1<<col);
                    } else if (*debounce <= DEBOUNCE_UP_MASK) { //debounce KEY UP
                        matrix[row] &= ~(1<<col);
                    }
                }
            }
            
            *debounce++;

            // Ignore if this code region execution time elapses more than 20us.
            // MEMO: 20[us] * (TIMER_RAW_FREQ / 1000000)[count per us]
            // MEMO: then change above using this rule: a/(b/c) = a*1/(b/c) = a*(c/b)
            if (TIMER_DIFF_RAW(TIMER_RAW, last) > 20/(1000000/TIMER_RAW_FREQ)) {
                matrix[row] = matrix_prev[row];
            }


            //_delay_us(5); //作用未知，去除后似乎无影响。
            //KEY_PREV_OFF();
            //KEY_UNABLE();
            if (BLE51_PowerState >= 2) {
                //似乎部分从Lock Mode唤醒还需要这个。
                //如果我在一个C1和C3拆除的HHKB上不使用这个延迟，正常时又会触发2。用10us是可以的，3us会2
                _delay_us(6); // scan faster when power saving
                if (wake_scan && row == 7 && col == 6) { // wake scan complete
                //if (wake_scan && row == (hhkb_matrix_rows >> 1 )) { // 4的时候US正常(12号发的固件，13号接到有人反应2唤醒时不能按)，JP节能时无法唤醒。
                    wake_scan = 0;
                    return 1;
                }
            } else {
                // NOTE: KEY_STATE keep its state in 20us after KEY_ENABLE.
                // This takes 25us or more to make sure KEY_STATE returns to idle state.
                //_delay_us(20);
            }
        }
    }
    // 二级节能唤醒检测
    if (BLE51_PowerState == 4 && matrix_keys_down) return 100;
    if (BLE51_PowerState >= 10) { 
        //after a whole scan, check F and J when LOCK MODE
        // jp version, matrix[5,6] is F, matrix[9,6] is J.
        // already tran jp matrix to us.
        //us version, matrix[1,5] is J, matrix[4,5] is F.
        if (matrix_keys_down == 2) { // only two keys down.
            if (matrix[1]  == (1<<5) && matrix[4] == (1<<5)) {
                return 100;
            }
        } else if (!ble51_boot_on && matrix_keys_down) return 100; //蓝牙功能关闭时唤醒电脑
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
    // 如果不使用wake_scan，前4行的按键节能时无法识别。wake_scan也只需要扫到第5行即可。
    wake_scan = 1;
    matrix_scan();
    
    /* some 660c(maybe with tp1685) need one more scan when deep sleep. But maybe there is no hhkb pro2 with tp1685
     * 一个 hhkb pro2 (ajajz)，如果隔太久(一晚上),Lock Mode 无法唤醒，尝试加回下面这一条。添加回后他和另一个人测试正常。 */
    if (BT_POWERED == 0) {
        matrix_scan();
        //matrix_scan(); //因为加入了防抖，多扫一次。不在这里使用，在matrix.c的防抖里面直接多处理一次
    }
    //*/
    if (matrix_scan() == 100) {
        //KEY_UNABLE();
        return true;
    }
    KEY_POWER_OFF();
    return false;
}

void suspend_power_down_action(void)
{
    //KEY_POWER_OFF();
    led_all_off();
}

void suspend_wakeup_init_action(void)
{
    if (BT_POWERED == 0) {
        PORTF |= (1<<PF4 |1<<PF1 | 1<<PF0);
        _delay_ms(400);
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
                if (steps & 0b10) PORTF |= (1<<4 | 1<<1 | 1<<0);
            } else {
                if (bt_connected) {
                    if (steps & 0b110) PORTF |= (1<<1 | 1<<0);
                } else {
                    if (steps & 0b10) PORTF |= (1<<0);
                }
            }
        }
        // capslock
        else if (host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK)) {
            PORTF |= (1<<4);
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
    uint8_t kbd_nkro = keymap_config.nkro;
    type_num(kbd_nkro?6:0);
}