#include <stdbool.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "matrix.h"
#include "action.h"
#include "suspend.h"
#include "timer.h"
#include "send_string.h"
#include "recore.h"

void type_numbers(uint16_t value)
{
    for (uint16_t i=10000; i>=1; i=i/10) {
        uint8_t this_num = value/i % 10;
        if (value/i) {
            send_nibble(this_num);
        }
    }
}