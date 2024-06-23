#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DO6N
#define CONTACT(x,y)    x##y
#define CONTACT2(x,y)   CONTACT(x,y)
#define FW_VER          CONTACT2(VIAL_, FW_VER_DATE)
#define VENDOR_ID       0x5944
#define PRODUCT_ID      0x23EF
#define DEVICE_VER      0x0001
#define MANUFACTURER    KBDFans_YDKB
#if CONSOLE_ENABLE
#define PRODUCT         Jolteon Keyboard Uni Debug (FW_VER)
#else
#define PRODUCT         Jolteon Keyboard Uni (FW_VER)
#endif

#define USB_MAX_POWER_CONSUMPTION 350
#define WAIT_FOR_USB

#define EC_INIT_CHECK_TIMES 0
#define EPC_LV KC_P5

/* key matrix size */
#define MATRIX_ROWS 7
#define MATRIX_COLS 16
//#define FORCE_NKRO //When FORCE_NKRO, Enable NKRO in QMK Settings will not be saved

#define DEBOUNCE_DN 3
#define DEBOUNCE_UP 3

#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_EFFECT_SNAKE
#define RGBLIGHT_EFFECT_KNIGHT
#define RGBLIGHT_EFFECT_CHRISTMAS
#define RGBLIGHT_EFFECT_STATIC_GRADIENT
#define RGBLIGHT_EFFECT_RGB_TEST
#define RGBLIGHT_EFFECT_ALTERNATING
#define RGBLIGHT_EFFECT_TWINKLE

#define RGBLIGHT_LIMIT_VAL 192
#define RGBLIGHT_SLEEP
#define RGB_DI_PIN B15
#define RGBLED_NUM 16
/* key combination for command */
#define IS_COMMAND() ( \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)

