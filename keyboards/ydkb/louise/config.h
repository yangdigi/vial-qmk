#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DO6A
#define CONTACT(x,y)    x##y
#define CONTACT2(x,y)   CONTACT(x,y)
#define FW_VER          CONTACT2(VIAL_, FW_VER_DATE)
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x2141
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#define PRODUCT         Louise (FW_VER)


#define MATRIX_ROWS 4  //595 num of each side.
#define MATRIX_COLS 16


#define BACKLIGHT_PIN C6
#define BACKLIGHT_LEVELS 6
#define BACKLIGHT_ON_STATE 0

/* key combination for command */
#define IS_COMMAND() ( \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)

#define ws2812_PORTREG  PORTD
#define ws2812_DDRREG   DDRD
#define ws2812_pin PD7
#define RGBLED_NUM 8     // Number of LEDs
#define RGBLIGHT_MODES 14 //less rgblight mode to save some space for vial 

/* disable command for default layer */
#define MAGIC_KEY_SWITCH_LAYER_WITH_FKEYS  0
#define MAGIC_KEY_SWITCH_LAYER_WITH_NKEYS  0


#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
    #define UCSR1D _SFR_MEM8(0xCB)
    #define RTSEN 0
    #define CTSEN 1

    #define SERIAL_UART_BAUD        76800
    #define SERIAL_UART_DATA        UDR1
    #define SERIAL_UART_UBRR        ((F_CPU/(8.0*SERIAL_UART_BAUD)-1+0.5))
    #define SERIAL_UART_RXD_VECT    USART1_RX_vect
    #define SERIAL_UART_TXD_READY   (UCSR1A&(1<<UDRE1))
    #define SERIAL_UART_INIT()      do { \
        cli(); \
        UBRR1L = (uint8_t) SERIAL_UART_UBRR;      \
        UBRR1H = ((uint16_t)SERIAL_UART_UBRR>>8); \
        UCSR1B |= (1<<RXCIE1) | (1<<RXEN1); \
        UCSR1B |= (0<<TXCIE1) | (1<<TXEN1); \
        UCSR1C |= (0<<UPM11) | (0<<UPM10); \
        UCSR1A |= (1<<U2X1); \
        sei(); \
    } while(0)
#else
    #error "USART configuration is needed."
#endif
/* BT Power Control */
#define BT_POWERED    (~PORTD & (1<<5))
#define bt_power_init()    do { DDRD |= (1<<5); PORTD &= ~(1<<5); } while(0)
#define bt_power_reset()    do {PORTD |= (1<<5); WAIT_MS(100); PORTD &= ~(1<<5);} while(0)
#define turn_off_bt()    do { PORTD |= (1<<5); UCSR1B = (1<<RXCIE1 | 1<<RXEN1); } while(0)
#define turn_on_bt()    do { PORTD &= ~(1<<5); if (UCSR1B == (1<<RXCIE1 | 1<<RXEN1)) WAIT_MS(200); UCSR1B = (1<<RXCIE1 | 1<<RXEN1 | 1<<TXEN1); } while(0)

#define BLE_NAME "Louise BLE"
#define BLE_BATTERY_SERVICE
#define BLE_LIGHT_ON (~PORTD & (1<<6)) //RGB Power IO

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINC & (1<<7))
#define CHARGING_FIX_VALUE 40
#define CHARGING_STATE_INIT()    do { DDRC &= ~(1<<7); PORTC |= (1<<7);} while(0)
#define HARDWARE_BT_SWITCH

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
//#define NO_ACTION_ONESHOT  //930B
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
#define NO_DEFAULT_COMMAND