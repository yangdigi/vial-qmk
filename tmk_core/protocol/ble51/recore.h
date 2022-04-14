#ifndef RECORE_H
#define RECORE_H

#include <stdbool.h>
#include "host_driver.h"
#include "ble51.h"
#include "bootloader.h"
#include "quantum.h"
#include "send_string.h"
#include "keycode_config.h"


// for lock mode, *debounce + x
#define KP(row,col) (row * MATRIX_COLS + col)

#define type_num(x) send_nibble(x)

void type_numbers(uint16_t value);


void suspend_power_down_action(void);
void suspend_wakeup_init_action(void);
void watchdog_on(void);

__attribute__ ((weak))
void hook_early_init(void);

#endif
