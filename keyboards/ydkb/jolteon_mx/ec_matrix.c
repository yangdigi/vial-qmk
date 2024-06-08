/* Copyright 2023 YANG
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ch.h>
#include <hal.h>
#include "ec_matrix.h"

#include "quantum.h"
#include "print.h"

/* ADC */
#define C_CHARGE_WAIT() {if (col == 0 && row == 0) {int t=10;while (t-- > 0) asm("nop");}}
#define C_DISCHARGE_WAIT() { uint16_t t = 1; while (t-- > 0) asm("nop");}

static void ec_apc_init(void);

void adc_init(void)
{
    palSetLineMode(A2, PAL_MODE_INPUT_ANALOG);

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->CR1 = 0;
    ADC1->CR2 = ADC_CR2_ADON | ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0) ;

    ADC1->CR2 = ADC_CR2_ADON | ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0);

    ADC1->SMPR2 = (0b100 << ADC_SMPR2_SMP2_Pos); //

    ADC1->SQR3 = 2;
}

uint8_t adc_read8(void)
{
    uint8_t adc_value;
    ADC1->CR2 |= ADC_CR2_ADON;
    while ((ADC1->SR & ADC_SR_EOC) == 0);
    adc_value = ADC1->DR >> 4;

    return adc_value;
}

/* EC Matrix */
#define COL_PIN_MASK (1<<14 | 1<<13 | 1<<12 | 1<<11 | 1<<10)
static uint8_t ec_apc_row[MATRIX_ROWS] = {0};
uint8_t keys_down_row[MATRIX_ROWS] = {0};
uint8_t ec_actuation_point[MATRIX_ROWS][MATRIX_COLS] = {0};
uint8_t ec_key_value[MATRIX_ROWS][MATRIX_COLS];

static inline void C_CHARGE_READY(void) { palSetLine(A1); }
static inline void C_DISCHARGE(void)    { palClearLine(A1); }

static inline void ec_unselect_rows(void)
{
    palClearPort(GPIOB, 0b1111111);
    palSetGroupMode(GPIOB, 0b1111111, 0, PAL_MODE_OUTPUT_PUSHPULL);
    uint16_t t = 26;
    while (t-- > 0) asm("nop");
}

static inline void ec_select_row(uint8_t row)
{
    // Select row. Hi-Z
    palSetPadMode(GPIOB, row, PAL_MODE_INPUT_PULLUP);
    palSetPad(GPIOB, row);
}

void ec_matrix_init(void)
{
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

    //cols init. 4051 pin.
    palSetGroupMode(GPIOB, COL_PIN_MASK, 0, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPort(GPIOB, COL_PIN_MASK);
    palSetPort(GPIOB, 1<<11 | 1<<10);

    //discharge pin
    palSetLineMode(A1, PAL_MODE_OUTPUT_OPENDRAIN);
    palClearLine(A1);

    adc_init();

    ec_apc_init();
}

void ec_select_col(uint8_t col)
{
    //select col, B12(s0) B13(s1) B14(s2)
    // B11: 4051_EN0 B10: 4051_EN1
    palClearPort(GPIOB, COL_PIN_MASK);
    palSetPort(GPIOB, (col&0b111)<<12); //S2..0
    if (col < 8) palSetPad(GPIOB, 10); //EN0
    else palSetPad(GPIOB, 11); //EN1
}

#if 1
static void ec_apc_init(void)
{
    // default value
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        ec_apc_row[row] = VALID_EC_INIT_MIN;
    }
    uint8_t loops = 50;//EC_INIT_CHECK_TIMES;
    while(loops--) {
        uint8_t installed_keys = 0;
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            ec_select_col(col);
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                ec_get_key(row, col);

                uint8_t key_adc = ec_key_value[row][col] + 1;
                if ((key_adc >= VALID_EC_INIT_MIN) && (key_adc <= VALID_EC_INIT_MAX)) {
                    ec_apc_row[row] += key_adc;
                    ec_apc_row[row] >>= 1;
                }
            }
        }
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        ec_apc_row[row] += EC_ACTIVE_OFFSET;
    }
}
#endif
// Read adc raw
//static uint8_t ec_apc_value = 120;
uint8_t ec_get_key(uint8_t row, uint8_t col)
{
    chSysLock();
    C_CHARGE_READY();
    ec_select_row(row);
    C_CHARGE_WAIT();

    ec_key_value[row][col] = adc_read8();
    chSysUnlock();

    ec_unselect_rows();
    C_DISCHARGE();
    C_DISCHARGE_WAIT();

    uint8_t key_apc_calc = ec_apc_row[row] + keys_down_row[row];
    uint8_t key_adc = ec_key_value[row][col];
    if (key_adc <= (key_apc_calc - EC_RESET_OFFSET)) return 0;
    else if (key_adc >= key_apc_calc) return 0x80;
    else return 0b10;
}

#if 0
#define EC_APC_KEY_POS (VIA_EEPROM_CONFIG_END+1 + (APC_KEY_ROW * MATRIX_COLS + APC_KEY_COL) * 2)
void ec_apc_update(void) {
    static uint8_t ec_apc_level[10] = {88, 96, 104, 112, 120, 128, 136, 144, 152, 80};
    uint8_t apc_eeprom = eeprom_read_byte(EC_APC_KEY_POS);
    if (apc_eeprom >= 89) apc_eeprom -= 59; //P1 to P0
    apc_eeprom -= 30;
    //xprintf("apc_eeprom: %d\n",apc_eeprom);
    if (apc_eeprom < 10) ec_apc_value = ec_apc_level[apc_eeprom];
}
#endif

extern uint16_t scan_speed;
void ec_matrix_print(void)
{
    xprintf("\n%3d R\\C ",scan_speed);
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        xprintf("[%X],", col);
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        uint8_t key_apc_calc = ec_apc_row[row] + keys_down_row[row];
        xprintf("\n%3d [%d]:", key_apc_calc, row);
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            #if 0 //(EC_INIT_CHECK_TIMES)
            if (ec_actuation_point[row][col] == 0) xprintf("-%2d,", ec_key_value[row][col]);
            else
            #endif
            xprintf("%3d,", ec_key_value[row][col]);
            //xprintf("%3d,", ec_actuation_point[row][col]);
        }
    }
    print("\n");
}
