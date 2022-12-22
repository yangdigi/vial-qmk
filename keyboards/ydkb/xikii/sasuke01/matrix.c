#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "wait.h"
#include "action_layer.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "timer.h"
#include "matrix.h"
#include "switch_board.h"

#define DEBOUNCE_DN_MASK (uint8_t)(~(0x80 >> 5))
#define DEBOUNCE_UP_MASK (uint8_t)(0x80 >> 5)

static matrix_row_t matrix[MATRIX_ROWS] = {0};

static uint8_t matrix_current_row = 0;
static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint8_t encoder_debouncing[10] = {0};

static uint8_t encoder_state_prev[10][2] = {0}; // 用0记录B，用1记录A，主要是为了direction方向
static void select_next_key(uint8_t mode);
static uint8_t get_key(uint8_t col);

static void init_cols(void);
__attribute__ ((weak))
void matrix_scan_user(void) {}

__attribute__ ((weak))
void matrix_scan_kb(void) {
  matrix_scan_user();
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

void matrix_init(void)
{
    init_cols();
}

uint8_t matrix_scan(void)
{
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;

    select_next_key(0);
    uint8_t *debounce = &matrix_debouncing[0][0];
    for (uint8_t row=0; row<matrix_rows(); row++) {
        matrix_current_row = row;
        for (uint8_t col=0; col<matrix_cols(); col++, *debounce++) {

            uint8_t key = get_key(col);
            select_next_key(1);
            *debounce = (*debounce >> 1) | key;
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
    DDRF  |=  (1<<6 | 1<<4);
    //key(col) pin
    DDRF  &= ~(1<<5 | 1<<1);
    PORTF |=  (1<<6 | 1<<5 | 1<<4 | 1<<1);
}


static uint8_t get_key_f(uint8_t col) {
    return PINF&(1<<5) ? 0 : 0x80;
}

uint8_t get_key(uint8_t col)
{
    uint8_t value = get_key_f(col);
    if (col == 1 || col == 2 || col == 5 || col == 6) {
        uint8_t this_encoder = matrix_current_row * 2;
        if (col > 4) this_encoder += 1;
        static uint16_t encoder_idle_timer;
        uint8_t encoder_state_new = 0;
        uint8_t direction = col&1;

        if (timer_elapsed(encoder_idle_timer) > 400) encoder_debouncing[this_encoder] = direction?0b111:0;
        encoder_state_new = value? 1 : 0;

        if (encoder_state_new != encoder_state_prev[this_encoder][direction]) {
            encoder_state_prev[this_encoder][direction] = encoder_state_new;
            if (encoder_state_new == 0) {
                if (encoder_state_prev[this_encoder][direction?0:1] == 0) {
                    encoder_debouncing[this_encoder] = (encoder_debouncing[this_encoder]<<1) + direction;
                    if ((encoder_debouncing[this_encoder] & 0b111) == (direction?0b111:0)) {
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

static void select_next_key(uint8_t mode)
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
    _delay_us(6);
}

