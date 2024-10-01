#pragma once

#include "config_common.h"
#include "config_ble51.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DOA1
#define VENDOR_ID       0x5944
#define PRODUCT_ID      0x23EE
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#define PRODUCT         Eevee Keyboard Uni (FW_VER)

#define EC_INIT_CHECK_TIMES 0
#define EPC_LV KC_P5

/* key matrix size */
#define MATRIX_ROWS 7
#define MATRIX_COLS 16
#define DEBOUNCE_DN 3
#define DEBOUNCE_UP 3

#define DEFAULT_6KRO // macOS's Capslock switching between Chinese and English has compatibility issues with NKRO

#define ws2812_PORTREG  PORTE
#define ws2812_DDRREG   DDRE
#define ws2812_pin PE6
#define RGBLED_NUM 16     // Number of LEDs
#define RGBLIGHT_MODES 14 //less rgblight mode to save some space for vial 

/* BT Power Control */
#define BT_POWERED    (~PORTD & (1<<5))
#define bt_power_init()    do { DDRD |= (1<<5); PORTD &= ~(1<<5); } while(0)
#define bt_power_reset()    do {PORTD |= (1<<5); WAIT_MS(100); PORTD &= ~(1<<5);} while(0)
#define turn_off_bt()    do { PORTD |= (1<<5); UCSR1B = (1<<RXCIE1 | 1<<RXEN1); } while(0)
#define turn_on_bt()    do { PORTD &= ~(1<<5); if (UCSR1B == (1<<RXCIE1 | 1<<RXEN1)) WAIT_MS(200); UCSR1B = (1<<RXCIE1 | 1<<RXEN1 | 1<<TXEN1); } while(0)

#define BLE_NAME "Eevee!BLE"
#define BLE_LIGHT_ON (PORTD & (1<<7)) //RGB Power IO

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINE & (1<<2))
#define CHARGING_FIX_VALUE 50
#define CHARGING_STATE_INIT()    do { DDRE &= ~(1<<2); PORTE |= (1<<2);} while(0)

#define BLE51_CONSUMER_ON_DELAY 128

