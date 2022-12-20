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

extern rgblight_config_t rgblight_config;
static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static void select_key(uint8_t mode);
static uint8_t get_key(uint8_t col);

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

static void get_key_ready(void) {
    DDRB  &= ~(1<<3);
    PORTB |=  (1<<3);
    _delay_us(6);
}

inline void select_key_ready(void) {
    DDRB |= (1<<3);
} 

void hook_early_init()
{
    if (pgm_read_byte(0x7e65) == 0x4c) {
        // USB Only version
        is_ble_version = 0;
        ble51_boot_on = 0;
    } else {
        // PF7 for BLE Reset, PE2 for BT_SW
        DDRF  &= ~(1<<PF7);
        PORTF |=  (1<<PF7);
        DDRE  &= ~(1<<PE2);
        PORTE |=  (1<<PE2);
        _delay_ms(2);
        if (~PINE & (1<<PE2)) ble51_boot_on = 0;
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            ble_reset_key = 0;
            if (ble51_boot_on) {
                // PF7 for BLE Reset
                DDRF  |=  (1<<PF7);
                PORTF &= ~(1<<PF7);
                bt_power_init();
                // light LED
                DDRE  |= (1<<PE6);
                PORTE |= (1<<PE6);
                DDRB  |= (1<<PB2);
                PORTB |= (1<<PB2);
                _delay_ms(5000);
                bootloader_jump(); 
            }
        }
    }
}

void matrix_init(void)
{
    // led init
    DDRE  |=  (1<<PE6);
    PORTE &= ~(1<<PE6);
    DDRF  |=  (1<<PF0);
    PORTF &= ~(1<<PF0);
    DDRB  |=  (1<<PB2);
    PORTB &= ~(1<<PB2);

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
            uint8_t real_col = col/2; //col 0-7
            if (col & 1) real_col += 8; //col 8-15

            uint8_t key = get_key(real_col);
            *debounce = (*debounce >> 1) | key;

            if (real_col >= 8) select_key(1);
            
            //if ((*debounce > 0) && (*debounce < 255)) {
            if (1) {
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << real_col);
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

void init_cols(void)
{
    //595 pin
    DDRB  |=  (1<<PB3 | 1<<PB1);
    PORTB |=  (1<<PB3 | 1<<PB1);
    //key1(col) pin,  key2 is PB3
    DDRF  &= ~(1<<PF1);
    PORTF |=  (1<<PF1);
}


static uint8_t get_key(uint8_t col) {
    if (col<8) return PINF&(1<<PF1) ? 0 : 0x80;
    else return PINB&(1<<PB3) ? 0 : 0x80;
}



void unselect_rows(void)
{
}

void select_key(uint8_t mode)
{
    select_key_ready();
    if (mode == 0) {
        DS_PL_HI();
        for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS / 2; i++) {
            CLOCK_PULSE();
        }
        DS_PL_LO();
        CLOCK_PULSE();
    } else {
        DS_PL_HI();
        CLOCK_PULSE();
    }
    get_key_ready();
}


bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState >= 10) {  //lock mode  
        matrix_scan();
        // Key1_S14 F,   real debounce p is 1*16+4*2
        // Key2_K14 J,   real debounce p is 1*16+4*2+1
        uint8_t *debounce = &matrix_debouncing[0][0];
        uint8_t matrix_keys_down = 0;
        for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
            if (*debounce > 0) {
                if (i == KP(1,8) || i == KP(1,9)) matrix_keys_down += 100;
                else matrix_keys_down++;
            }
        }
        if (matrix_keys_down == 200) {
            return true;
        }
    } else {
        //check all keys
        select_key_ready();
        DS_PL_LO();
        for (uint8_t i = 0; i < MATRIX_ROWS * MATRIX_COLS / 2; i++) {
            CLOCK_PULSE();
        }
        get_key_ready();
        if (get_key(0) || get_key(8)) { //
            return true;
        }
    }
    return false;
}
