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
#include "suspend.h"
#include "lufa.h"
#include "rgblight.h"
#include "ble51.h"
#include "ble51_task.h"
#include "switch_board.h"

#define DEBOUNCE_DN_MASK (uint8_t)(~(0x80 >> 5))
#define DEBOUNCE_UP_MASK (uint8_t)(0x80 >> 5)

bool is_ble_version = 1;
bool no_rgblight;

extern rgblight_config_t rgblight_config;
static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static void select_key(uint8_t mode);
static uint8_t get_key(void);

static void init_cols(void);
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

void hook_early_init()
{
    // rama kara version has no rgblight. PF0
    DDRF  &= ~(1<<0);
    PORTF |=  (1<<0);
    _delay_ms(2);
    if (~PINF & (1<<0)) no_rgblight = 1;

    if (pgm_read_byte(0x7ea4) == 0x91) {
        // USB Only version
        is_ble_version = 0;
        ble51_boot_on = 0;
    } else {
        // PD4 for BLE Reset, PF7 for BT_SW
        DDRD  &= ~(1<<4);
        PORTD |=  (1<<4);
        DDRF  &= ~(1<<7);
        PORTF |=  (1<<7); 
        _delay_ms(2);
        if (~PINF & (1<<7)) ble51_boot_on = 0;
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // PE6 for BLE Reset
                DDRD  |=  (1<<4);
                PORTD &= ~(1<<4);
                bt_power_init();
                // light CapsLED
                DDRB  |= (1<<6);
                PORTB |= (1<<6);
                _delay_ms(5000);
                bootloader_jump(); 
            }
        }
    }
}

void matrix_init(void)
{
    DDRB  |=  (1<<6);
    PORTB &= ~(1<<6);
    init_cols();
    rgblight_init();
}

uint8_t matrix_scan(void)
{
    
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;

    select_key(0);
    uint8_t *debounce = &matrix_debouncing[0][0];
    for (uint8_t row=0; row<matrix_rows(); row++) {
        for (uint8_t col=0; col<matrix_cols(); col++, *debounce++) {
            uint8_t key = get_key();
            *debounce = (*debounce >> 1) | key;

            select_key(1);
            if ((*debounce > 0) && (*debounce < 255)) {
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
            if (!rgblight_config.enable) kb_idle_times = 12; 
            else kb_idle_times = 0;
        }
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

void init_cols(void)
{
    //595 pin
    DDRB  |=  (1<<3 | 1<<1);
    DDRB  &= ~(1<<2);
    PORTB |=  (1<<3 | 1<<2 | 1<<1);
}


static uint8_t get_key(void) {
    return PINB&(1<<2) ? 0 : 0x80;
}



void unselect_rows(void)
{
}

static void select_key(uint8_t mode)
{
    if (mode == 0) {
        DS_PL_HI();
        for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS; i++) {
            CLOCK_PULSE();
        }
        DS_PL_LO();
        CLOCK_PULSE();
    } else {
        DS_PL_HI();
        CLOCK_PULSE();
    }
    _delay_us(3);
}


bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState >= 10) {  //lock mode  
        matrix_scan();
        // K24 F, K61 J
        uint8_t *debounce = &matrix_debouncing[0][0];
        uint8_t matrix_keys_down = 0;
        for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
            if (*debounce > 0) {
                if (i == KP(2,4) || i == KP(6,1)) matrix_keys_down += 100;
                else matrix_keys_down++;
            }
        }
        if (matrix_keys_down == 200) {
            return true;
        }
    } else {
        //check all keys
        DS_PL_LO();
        for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS; i++) {
            CLOCK_PULSE();
        }
        _delay_us(5);
        if (get_key()) { //
            return true;
        }
    }
    return false;
}
