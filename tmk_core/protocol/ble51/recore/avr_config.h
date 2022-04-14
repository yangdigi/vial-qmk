#ifndef AVR_CONFIG_H
#define AVR_CONFIG_H

#ifdef __AVR__
struct AVR_PINS{
    uint8_t  pin;
    uint8_t  mask;
};

#define PA(x) {.pin = 0x00, .mask = (1<<x)}
#define PB(x) {.pin = 0x03, .mask = (1<<x)}
#define PC(x) {.pin = 0x06, .mask = (1<<x)}
#define PD(x) {.pin = 0x09, .mask = (1<<x)}
#define PE(x) {.pin = 0x0C, .mask = (1<<x)}
#define PF(x) {.pin = 0x0F, .mask = (1<<x)}
#endif

#endif
