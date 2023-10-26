
#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER          QMK_DNAJ
#define FW_VER_VIA      VIA_DNAJ
#define FW_VER_VIAL     VIAL_DNAJ
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x2302
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB_KBDFans
#define PRODUCT         TacoPad Keyboard (FW_VER)

#define USB_MAX_POWER_CONSUMPTION 350

#define MATRIX_ROWS 3
#define MATRIX_COLS 8


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

//#define RGBLIGHT_LIMIT_VAL 192
#define RGBLIGHT_SLEEP
#define RGB_DI_PIN B15
#define RGBLED_NUM 7

//#define SUSPEND_ACTION

/* key combination for command */
#define IS_COMMAND() ( \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (get_mods() == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)

