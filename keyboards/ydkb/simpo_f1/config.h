
#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER_DATE     DO7S
#define CONTACT(x,y)    x##y
#define CONTACT2(x,y)   CONTACT(x,y)
#define FW_VER          CONTACT2(VIAL_, FW_VER_DATE)
#define VENDOR_ID       0x9D5B
#define PRODUCT_ID      0x2450
#define DEVICE_VER      0x0001
#define MANUFACTURER    KBDFans_YDKB
#if CONSOLE_ENABLE
#define PRODUCT         Simpo_F1 Uni Debug (FW_VER)
#else
#define PRODUCT         Simpo_F1 Uni(FW_VER)
#endif

#define USB_MAX_POWER_CONSUMPTION 350
#define WAIT_FOR_USB

/* key matrix size */
#define MATRIX_ROWS 5 
#define MATRIX_COLS 15
//#define FORCE_NKRO //When FORCE_NKRO, Enable NKRO in QMK Settings will not be saved

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
#define RGBLIGHT_DEFAULT_MODE 7 // defaut: RAINBOW_MOOD

#define RGBLIGHT_LIMIT_VAL must_redefine
#define RGBLIGHT_SLEEP
#define RGB_DI_PIN A7
#define RGBLED_NUM must_redefine

#define DRIVER_LED_TOTAL must_redefine

//#define SUSPEND_ACTION

/* key combination for command */
#define IS_COMMAND() ( \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)

