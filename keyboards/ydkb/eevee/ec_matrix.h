#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint8_t ec_actuation_point[MATRIX_ROWS][MATRIX_COLS];
extern uint8_t ec_key_value[MATRIX_ROWS][MATRIX_COLS];

void ec_matrix_init(void);
void ec_matrix_print(void);
void ec_select_col(uint8_t col);
uint8_t ec_get_key(uint8_t row, uint8_t col);
