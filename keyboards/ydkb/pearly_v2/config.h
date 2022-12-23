#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER          QMK_DMCN
#define FW_VER_VIA      VIA_DMCN
#define FW_VER_VIAL     VIAL_DMCN
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x2242
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#define PRODUCT         Pearly v2 (FW_VER)


#define MATRIX_ROWS 4
#define MATRIX_COLS 12



#define TAPPING_TOGGLE  2


/* key combination for command */
#define IS_COMMAND() ( \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)

#define ws2812_PORTREG  PORTD
#define ws2812_DDRREG   DDRD
#define ws2812_pin PD0
#define RGBLED_NUM 12     // Number of LEDs

/* disable command for default layer */
#define MAGIC_KEY_SWITCH_LAYER_WITH_FKEYS  0
#define MAGIC_KEY_SWITCH_LAYER_WITH_NKEYS  0

/* fix space cadet rollover issue */
#define DISABLE_SPACE_CADET_ROLLOVER

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
#define bt_power_init()    do { DDRD |= (1<<5); PORTD &= ~(1<<5);} while(0)
#define turn_off_bt()    do { PORTD |= (1<<5); UCSR1B &= ~(1<<TXEN1); } while(0)
#define turn_on_bt()    do { PORTD &= ~(1<<5); UCSR1B |= (1<<TXEN1);} while(0)

#define BLE_NAME "Pearly v2 BLE"
#define BLE_BATTERY_SERVICE
#define BLE_LIGHT_ON (~PORTD & (1<<4))  //RGB Power IO
#define HARDWARE_BT_SWITCH

#define UPDATE_BATTERY_WHEN_CHARGING
#define BATTERY_CHARGING (~PINB & (1<<1))
#define CHARGING_FIX_VALUE 40
#define CHARGING_STATE_INIT() do { DDRB &= ~(1<<1); PORTB |= (1<<1);} while(0)

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
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
