#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "action.h"
#include "command.h"
#include "keyboard.h"
#include "timer.h"
#include "matrix.h"
#include "suspend.h"
#include "lufa.h"
#include "rgblight.h"
#include "ble51.h"
#include "ble51_task.h"
#include "backlight.h"

#define DEBOUNCE_DN_MASK (uint8_t)(~(0x80 >> 5))
#define DEBOUNCE_UP_MASK (uint8_t)(0x80 >> 5)


extern rgblight_config_t rgblight_config;
extern backlight_config_t backlight_config;

static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};


static void init_cols(void);
static void init_rows(void);
static void select_row(uint8_t row);
static uint8_t get_key(uint8_t col);

__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
    hook_keyboard_loop();
}

inline
uint8_t matrix_rows(void)
{
    return MATRIX_ROWS;
}

inline
uint8_t matrix_cols(void)
{
    return MATRIX_COLS;
}

void hook_early_init(void) {
        //BLE Reset
    if (ble_reset_key == 0xBBAA) {
        if (ble51_boot_on) {
            // PD3,TX_MCU to RX_51
            DDRD  |=  (1<<3 );
            PORTD &= ~(1<<3);
            bt_power_init();
            // CapsLED on, PB6
            DDRB  |= (1<<PB6);
            PORTB |= (1<<PB6);
            _delay_ms(5000);
            // turn off bt and ready to reinit it.
            turn_off_bt();
        }
    }
} 
void matrix_init(void)
{
    //software ble reset
    if (ble_reset_key == 0xBBAA) {
        ble_reset_key = 0;
        ble51_factory_reset();
    }

    init_cols();
    init_rows();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;

    uint8_t *debounce = &matrix_debouncing[0][0];
    for (uint8_t row=0; row<matrix_rows(); row++) {
        select_row(row);
        _delay_us(6);
        for (uint8_t col=0; col<matrix_cols(); col++, *debounce++) {
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
        }
        if (matrix[row] > 0) {
            if (!rgblight_config.enable && !backlight_config.enable) kb_idle_times = 12; 
            else kb_idle_times = 0;
        }
        //unselect_rows();
    }
    
    matrix_scan_quantum();
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
    print("\nr/c 01234567\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        print_hex8(row); print(": ");
        print_bin_reverse8(matrix_get_row(row));
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

/* Col pin configuration
 * 
 */

static void  init_cols(void)
{
    DDRB  &= ~0b00111111;
    PORTB |=  0b00111111;
}

static uint8_t get_key(uint8_t col)
{
    return PINB&(1<<col) ? 0 : 0x80;
}

/* Row pin configuration
 * row：    F7  F6  F5
 *   0：     0   0   0
 *   1：     0   0   1  
 */
static void init_rows(void)
{
    DDRF  |= (1<<PF7 | 1<<PF6 | 1<<PF5);
}

void unselect_rows(void)
{
}


static void select_row(uint8_t row)
{
    (row & (1<<0)) ? (PORTF |= (1<<PF5)) : (PORTF &= ~(1<<PF5));
    (row & (1<<1)) ? (PORTF |= (1<<PF6)) : (PORTF &= ~(1<<PF6));
    (row & (1<<2)) ? (PORTF |= (1<<PF7)) : (PORTF &= ~(1<<PF7));
    // Output low(DDR:1, PORT:0) to select
}


void select_all_rows(void)
{
    return;
}

bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState >= 10) {
        matrix_scan();
        // K43 F, K22 J
        uint8_t *debounce = &matrix_debouncing[0][0];
        uint8_t matrix_keys_down = 0;
        for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
            if (*debounce > 0) {
                if (i == KP(2,2) || i == KP(4,3)) matrix_keys_down += 100;
                else matrix_keys_down++;
            }
        }
        if (matrix_keys_down == 200) {
            return true;
        } else if (!ble51_boot_on && matrix_keys_down) return true;
    } else {
        for (uint8_t i=0; i<8; i++) {
            select_row(i);
            if ((~PINB & 0b00111111)) {
                return true;
            }
        }
    }
    return false;
}
