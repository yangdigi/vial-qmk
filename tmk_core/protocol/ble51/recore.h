#ifndef RECORE_H
#define RECORE_H

#include <stdbool.h>
#include "host_driver.h"
#include "ble51.h"
#include "bootloader.h"
#include "quantum.h"
//#include "send_string.h"  //qmk
#include "keycode_config.h" //qmk

#ifdef NOT_BLE
#define BT_POWERED 0
#endif

// for lock mode, *debounce + x
#define KP(row,col) (row * MATRIX_COLS + col)

#define WAIT_MS(x) _delay_ms(x)
//#define type_num(x) send_nibble(x)
#define type_num(x) type_numbers(x) //save 40B

void type_numbers(uint16_t value);

void suspend_power_down_action(void);
void suspend_wakeup_init_action(void);
void watchdog_on(void);

void hook_early_init(void);
void hook_nkro_change(void) ;

#endif
