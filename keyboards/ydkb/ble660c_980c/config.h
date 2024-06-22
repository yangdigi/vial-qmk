#pragma once

#include "config_common.h"
#include "config_ble51.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DO5U
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x19C1
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#define PRODUCT         BLE660C/BLE980C (FW_VER)


/* matrix size */
#define MATRIX_ROWS 7
#define MATRIX_COLS 16

/* BT Power Control */
#define BT_POWERED    (~PORTE & (1<<6))
#define bt_power_init()    do { DDRE |= (1<<6); PORTE &= ~(1<<6);} while(0)
#define bt_power_reset()    do {PORTE |= (1<<6); WAIT_MS(100); PORTE &= ~(1<<6);} while(0)
#define turn_off_bt()    do { PORTE |= (1<<6); UCSR1B = (1<<RXCIE1 | 1<<RXEN1); } while(0)
#define turn_on_bt()    do { PORTE &= ~(1<<6); if (UCSR1B == (1<<RXCIE1 | 1<<RXEN1)) WAIT_MS(200); UCSR1B = (1<<RXCIE1 | 1<<RXEN1 | 1<<TXEN1); } while(0)

#define BLE_NAME_VARIABLE
#define BT_POWER_SAVE_TIME 3000

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINE & (1<<2))
#define CHARGING_FIX_VALUE 50
#define CHARGING_STATE_INIT()    do { DDRE &= ~(1<<2); PORTE |= (1<<2);} while(0)

#define BLE51_CONSUMER_ON_DELAY 50  //scan slower

