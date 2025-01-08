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

#include <avr/interrupt.h>
#include "ec_matrix.h"

#include "print.h"
#include "wait.h"
#include "ble51.h"

/* ADC */
#define ADC_MUX (_BV(MUX5) | _BV(MUX0)) //D6 ADC9 MUX5..0:100001
#define AREF _BV(REFS0) // AVCC with external capacitor on AREF pin

#define ADC_PRESCALER (_BV(ADPS1) | _BV(ADPS0))
#define C_CHARGE_WAIT() {if (col == 0 && row == 0) _delay_us(1);}
#define C_DISCHARGE_WAIT()
static inline void C_CHARGE_READY(void) { DDRD &= ~(1<<4); }
static inline void C_DISCHARGE(void)    { DDRD |=  (1<<4); }

static void ec_matrix_check(void);

void adc_init(void)
{
    // High speed mode and MUX5
    ADCSRB = _BV(ADHSM) | (ADC_MUX & _BV(MUX5));
    //ADLAR 1,   left adjusted,  and MUX4..0
    ADMUX = AREF | _BV(ADLAR) | (ADC_MUX & 0b11111);
}

uint8_t adc_read8(void)
{
    uint8_t adc_value;
    // Enable ADC and configure prescaler. Start ADC
    ADCSRA = _BV(ADEN) | _BV(ADSC) | ADC_PRESCALER;

    // Wait for result
    while (ADCSRA & _BV(ADSC));
    adc_value = ADCH;
    // turn off the ADC
    //ADCSRA &= ~(1 << ADEN);
    ADCSRA = 0;

    return adc_value;
}

/* EC Matrix */
#ifdef APC_ENABLE
#define EC_AP_VALUE ec_ap_value
static uint8_t ec_ap_value = 125;
#else
#define EC_AP_VALUE 128 //ec_ap_value // 120 for EC, 80 for MX
#endif
#define EC_RESET_OFFSET 10
uint8_t ec_actuation_point[MATRIX_ROWS][MATRIX_COLS] = {0};
uint8_t ec_key_value[MATRIX_ROWS][MATRIX_COLS];
//static bool ec_inited = 0;

static inline void ec_unselect_rows(void)
{
    // Clear row pin. Output low.
    PORTB = 0;
    DDRB = 0x7f;
    if (BLE51_PowerState < 2) _delay_us(6);
}

static inline void ec_select_row(uint8_t row)
{
    // Select row. Hi-Z
    DDRB  = ~row;
    PORTB =  row;
}

void ec_matrix_init(void)
{
    DDRF  |=  0b11110010;
    PORTF  =  0b10000010;

    //discharge pin
    DDRD |=  (1<<4);
    PORTD &= ~(1<<4);

    adc_init();

    //ec_matrix_check();
#if 0 //CONSOLE_ENABLE
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            ec_actuation_point[row][col] = EC_AP_VALUE;
        }
    }
#endif
}

void ec_select_col(uint8_t col)
{
    // select col, PF4(s0),PF5(s1),PF6(s2)
    // PF7: 4051_1, PF1:4051_2.  LOW EN.
    PORTF = (col<<4);
    // 如果使用 PF1 控制 4051_2
    if (col < 8) PORTF |= (1<<1);
    // 如果使用PF7加NMOS控制4051_2。则NMOS断开时，需要延迟。
    //if (col == 0) _delay_us(6);
}


// Read adc raw
uint8_t ec_get_key(uint8_t row, uint8_t col)
{
    cli();
    C_CHARGE_READY();
    ec_select_row(1<<row);
    C_CHARGE_WAIT();
    ec_key_value[row][col] = adc_read8();
    sei();

    ec_unselect_rows();
    C_DISCHARGE();
    C_DISCHARGE_WAIT();

    if (ec_key_value[row][col] < (EC_AP_VALUE - EC_RESET_OFFSET)) return 0;
    else if (ec_key_value[row][col] >= EC_AP_VALUE) return 0x80;
    else return 0b10;
}

#ifdef APC_ENABLE
#define EC_APC_KEY_POS (VIA_EEPROM_CONFIG_END+1 + (APC_KEY_ROW * MATRIX_COLS + APC_KEY_COL) * 2)
void ec_apc_update(void)
{
    static const uint8_t ec_ap_level[8] = {128, 90, 100, 110, 120, 128, 144, 152};
    static uint8_t last_level = 10;
    // 最后一个Layout，8个选项，占3bit。
    uint8_t new_level = (eeprom_read_byte(VIA_EEPROM_LAYOUT_OPTIONS_ADDR) & 0b111);
    if (new_level != last_level && new_level < 8) {
        ec_ap_value = ec_ap_level[new_level];
        last_level = new_level;
    }
}
#endif

extern uint16_t scan_speed;
void ec_matrix_print(void)
{
    xprintf("\n%3d ",scan_speed);
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        xprintf("[%X],", col);
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        xprintf("\n[%d]:",row);
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
