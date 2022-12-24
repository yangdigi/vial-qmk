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


static uint8_t matrix_current_row = 0;
static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t encoder_state_prev[2][2] = {0};
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

void hook_early_init()
{
    if (pgm_read_byte(0x7e65) == 0x4c) {
        // USB Only version
        is_ble_version = 0;
        ble51_boot_on = 0;
    } else {
        // PE6 for BLE Reset, PE2 for BT_SW
        DDRE &= ~(1<<6 | 1<<2);
        PORTE |= (1<<6 | 1<<2);
        _delay_ms(2);
        if (~PINE & (1<<2)) ble51_boot_on = 0;
        //BLE Reset
        if (ble_reset_key == 0xBBAA) {
            if (ble51_boot_on) {
                // PE6 for BLE Reset
                DDRE  |=  (1<<6);
                PORTE &= ~(1<<6);
                bt_power_init();
                // light CapsLED
                DDRD  |= (1<<4);
                PORTD |= (1<<4);
                _delay_ms(5000);
                bootloader_jump(); 
            }
        }
    }
}

void matrix_init(void)
{
    DDRD  |=  (1<<4);
    PORTD &= ~(1<<4);
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
        matrix_current_row = row;
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
    DDRB  |=  (1<<PB3 | 1<<PB1);
    PORTB |=  (1<<PB3 | 1<<PB1);
    //key(col) pin
    DDRF  &= ~(1<<PF1 | 1<<PF0);
    PORTF |=  (1<<PF1 | 1<<PF0);
}


static uint8_t get_key_f(uint8_t col) {
    if (col<8) return PINF&(1<<PF0) ? 0 : 0x80;
    else return PINF&(1<<PF1) ? 0 : 0x80;

}

uint8_t get_key(uint8_t col)
{
    uint8_t value = get_key_f(col);
    if (matrix_current_row < 2 && (col == 7 || col == 15)) {
        static uint8_t encoder_debounce = 0;
        static uint16_t encoder_idle_timer;
        uint8_t encoder_state_new = 0;
        uint8_t direction = matrix_current_row & 0b1 ;

        if (timer_elapsed(encoder_idle_timer) > 400) encoder_debounce = direction?0b111:0;
        encoder_state_new = value? 1 : 0;

        uint8_t num = (col == 7)? 0:1;
        if (encoder_state_new != encoder_state_prev[num][direction]) { 
            encoder_state_prev[num][direction] = encoder_state_new;
            if (encoder_state_new == 0) { 
                if (encoder_state_prev[num][direction?0:1] == 0) {
                    encoder_debounce = (encoder_debounce<<1) + direction;
                    if ((encoder_debounce & 0b111) == (direction?0b111:0)) {
                        encoder_idle_timer = timer_read();
                        return DEBOUNCE_DN_MASK;
                    }
                } 
            } 
        }
        return 0;
    }
    return value; 
}

void unselect_rows(void)
{
}

static void select_key(uint8_t mode)
{
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
    _delay_us(3);
}


bool suspend_wakeup_condition(void)
{
    if (BLE51_PowerState >= 10) {  //lock mode  
        matrix_scan();
        // Left21 F, Right22 J. KP(2,2) KP(2,5)   
        uint8_t *debounce = &matrix_debouncing[0][0];
        uint8_t matrix_keys_down = 0;
        for (uint8_t i=0; i< MATRIX_ROWS * MATRIX_COLS; i++, *debounce++) {
            if (*debounce > 0) {
                if (i == KP(2,2) || i == KP(2,5)) matrix_keys_down += 100;
                else matrix_keys_down++;
            }
        }
        if (matrix_keys_down == 200) {
            return true;
        }
    } else {
        //check encoder
        DS_PL_LO();
        for (uint8_t i = 0; i < 2; i++) {
            for (uint8_t j = 0; j < 8; j++) {
                CLOCK_PULSE();
                DS_PL_HI();
            }
            _delay_us(6);
            uint8_t encoder_state_0 = PINF&(1<<PF0) ? 0 : 1;
            uint8_t encoder_state_1 = PINF&(1<<PF1) ? 0 : 1;
            if (encoder_state_0 != encoder_state_prev[0][i] || encoder_state_1 != encoder_state_prev[1][i]) {
                return true;
            }
        }
        //check all keys
        for (uint8_t i = 0; i < 5; i++) {
            for (uint8_t j = 0; j < 8; j++) {
                if (j == 0) DS_PL_HI();
                else DS_PL_LO();
                CLOCK_PULSE();
            }
        }
        _delay_us(6);
        if ( (PINF&0b11) < 0b11) { //
            return true;
        }
    }
    return false;
}
