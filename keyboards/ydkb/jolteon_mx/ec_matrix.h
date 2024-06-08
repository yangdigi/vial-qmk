#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint8_t ec_actuation_point[MATRIX_ROWS][MATRIX_COLS];
extern uint8_t ec_key_value[MATRIX_ROWS][MATRIX_COLS];
extern uint8_t keys_down_row[MATRIX_ROWS];

void ec_matrix_init(void);
void ec_matrix_print(void);
void ec_select_col(uint8_t col);
uint8_t ec_get_key(uint8_t row, uint8_t col);

#ifndef VALID_EC_CHECK_MIN
#define VALID_EC_CHECK_MIN 5
#endif
#ifndef VALID_EC_INIT_MIN
#define VALID_EC_INIT_MIN 16
#endif
#ifndef VALID_EC_INIT_MAX
#define VALID_EC_INIT_MAX 60
#endif

#ifndef EC_ACTIVE_OFFSET
#define EC_ACTIVE_OFFSET 25
#endif
#ifndef EC_RESET_OFFSET
#define EC_RESET_OFFSET 5
#endif