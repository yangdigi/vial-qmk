#pragma once

#include "config_common.h"
#include "config_ble51.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DP18
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x2242
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#if CONSOLE_ENABLE
#define PRODUCT         Pearly v2 Debug (FW_VER)
#else
#define PRODUCT         Pearly v2 (FW_VER)
#endif


#define MATRIX_ROWS 4
#define MATRIX_COLS 12



#define TAPPING_TOGGLE  2



#define DEFAULT_6KRO // macOS's Capslock switching between Chinese and English has compatibility issues with NKRO

#define ws2812_PORTREG  PORTD
#define ws2812_DDRREG   DDRD
#define ws2812_pin PD0
#define RGBLED_NUM 12     // Number of LEDs
#define RGBLIGHT_MODES 14 //less rgblight mode to save some space for vial 

/* BT Power Control */
#define BT_POWERED    (~PORTD & (1<<5))
#define bt_power_init()    do { DDRD |= (1<<5); PORTD &= ~(1<<5); } while(0)
#define bt_power_reset()    do {PORTD |= (1<<5); WAIT_MS(100); PORTD &= ~(1<<5);} while(0)
#define turn_off_bt()    do { PORTD |= (1<<5); UCSR1B = (1<<RXCIE1 | 1<<RXEN1); } while(0)
#define turn_on_bt()    do { PORTD &= ~(1<<5); if (UCSR1B == (1<<RXCIE1 | 1<<RXEN1)) WAIT_MS(200); UCSR1B = (1<<RXCIE1 | 1<<RXEN1 | 1<<TXEN1); } while(0)

#define BLE_NAME "Pearly v2 BLE"
#define BLE_BATTERY_SERVICE
#define BLE_LIGHT_ON (~PORTD & (1<<4))  //RGB Power IO
#define HARDWARE_BT_SWITCH

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINB & (1<<1))
#define CHARGING_FIX_VALUE 50
#define CHARGING_STATE_INIT() do { DDRB &= ~(1<<1); PORTB |= (1<<1);} while(0)


#define BLE51_NO_BATTERY_VOLTAGE
#define BLE51_NO_ULTRA_LOW_BATTERY
/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */
/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT  //930B
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
#define NO_DEFAULT_COMMAND
#define NO_RESET