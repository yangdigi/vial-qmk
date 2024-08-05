#pragma once

#include "config_common.h"
#include "config_ble51.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DO6M
#define CONTACT(x,y)    x##y //https://blog.csdn.net/aiynmimi/article/details/123486956
#define CONTACT2(x,y)   CONTACT(x,y)
#define FW_VER          CONTACT2(VIAL_, FW_VER_DATE)
#define VENDOR_ID       0x9D5B
#define PRODUCT_ID      0x2167
#define DEVICE_VER      0x0011
#define MANUFACTURER    YDKB_KBDFans
#define PRODUCT         YD67BLE (FW_VER)


/* key matrix size */
#define MATRIX_ROWS 5
#define MATRIX_COLS 16

#define DEFAULT_6KRO // macOS's Capslock switching between Chinese and English has compatibility issues with NKRO

#define SUSPEND_ACTION
/* BT Power Control */
#define BT_POWERED    (~PORTF & (1<<7))
#define bt_power_init()    do { DDRF |= (1<<7); PORTF &= ~(1<<7);} while(0)
#define bt_power_reset()    do {PORTF |= (1<<7); WAIT_MS(100); PORTF &= ~(1<<7);} while(0)
#define turn_off_bt()    do { PORTF |= (1<<7); UCSR1B = (1<<RXCIE1 | 1<<RXEN1); } while(0)
#define turn_on_bt()    do { PORTF &= ~(1<<7); if (UCSR1B == (1<<RXCIE1 | 1<<RXEN1)) WAIT_MS(200); UCSR1B = (1<<RXCIE1 | 1<<RXEN1 | 1<<TXEN1); } while(0)

#define BLE_NAME "YD67BLE"
#define BT_POWER_SAVE_TIME 3000

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINC & (1<<7))
#define CHARGING_FIX_VALUE 70
#define CHARGING_STATE_INIT()    do { DDRC &= ~(1<<7); PORTC |= (1<<7);} while(0)
