#pragma once

#define CONTACT(x,y)    x##y //https://blog.csdn.net/aiynmimi/article/details/123486956
#define CONTACT2(x,y)   CONTACT(x,y)
#if CONSOLE_ENABLE
#define FW_VER          CONTACT2(DEBUG_VIAL_, FW_VER_DATE)
#else
#define FW_VER          CONTACT2(VIAL_, FW_VER_DATE)
#endif

/* command. LShift+RShift+X and LShift+RShift+LCtrl+X for YDKB*/
#define IS_COMMAND() ( \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)

/* disable command for default layer */
#define MAGIC_KEY_SWITCH_LAYER_WITH_FKEYS  0
#define MAGIC_KEY_SWITCH_LAYER_WITH_NKEYS  0

/* UART Config */
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

/* BLE51 DEF */
#define BLE_BATTERY_SERVICE
#define BLE51_NO_BATTERY_VOLTAGE
#define BLE51_NO_ULTRA_LOW_BATTERY

/* QMK Core DEF */
#define NO_DEFAULT_COMMAND
#define NO_RESET