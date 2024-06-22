
#pragma once

#include "config_common.h"
#include "config_ble51.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DO6M
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x21A0
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#define PRODUCT         Xikii i104 (FW_VER)


/* key matrix size */
#define MATRIX_ROWS 13 
#define MATRIX_COLS 8

#define DEFAULT_6KRO // macOS's Capslock switching between Chinese and English has compatibility issues with NKRO

#define SUSPEND_ACTION

#define BACKLIGHT_PIN C6
#define BACKLIGHT_LEVELS 6
#define BACKLIGHT_ON_STATE 0

#define ws2812_PORTREG  PORTD
#define ws2812_DDRREG   DDRD
#define ws2812_pin PD1
#define RGBLED_NUM 12     // Number of LEDs

/* BT Power Control */
#define BT_POWERED    (~PORTD & (1<<5))
#define bt_power_init()    do { DDRD |= (1<<5); PORTD &= ~(1<<5); } while(0)
#define bt_power_reset()    do {PORTD |= (1<<5); WAIT_MS(100); PORTD &= ~(1<<5);} while(0)
#define turn_off_bt()    do { PORTD |= (1<<5); UCSR1B = (1<<RXCIE1 | 1<<RXEN1); } while(0)
#define turn_on_bt()    do { PORTD &= ~(1<<5); if (UCSR1B == (1<<RXCIE1 | 1<<RXEN1)) WAIT_MS(200); UCSR1B = (1<<RXCIE1 | 1<<RXEN1 | 1<<TXEN1); } while(0)

#define BLE_NAME "Xikii i104 BLE"
#define BLE_LIGHT_ON (~PORTD & (1<<0)) //RGB Power IO
#define HARDWARE_BT_SWITCH

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINC & (1<<7))
#define CHARGING_FIX_VALUE 90
#define CHARGING_STATE_INIT()    do { DDRC &= ~(1<<7); PORTC |= (1<<7);} while(0)
