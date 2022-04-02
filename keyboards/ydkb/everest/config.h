#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER          QMK_DM3U
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x2263  
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB
#define PRODUCT         Everest (FW_VER)

/* key matrix size */
#define MATRIX_ROWS 5
#define MATRIX_COLS 16

#define MOUSEKEY_INTERVAL       20
#define MOUSEKEY_DELAY          0
#define MOUSEKEY_TIME_TO_MAX    60
#define MOUSEKEY_MAX_SPEED      7
#define MOUSEKEY_WHEEL_DELAY 0

#define TAPPING_TOGGLE  1

#define TAPPING_TERM    200
#define IGNORE_MOD_TAP_INTERRUPT // this makes it possible to do rolling combos (zx) with keys that convert to other keys on hold (z becomes ctrl when you hold it, and when this option isn't enabled, z rapidly followed by x actually sends Ctrl-x. That's bad.)


/* key combination for command */
#define IS_COMMAND() ( \
    get_mods() == (MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT)) \
)

#define ws2812_PORTREG  PORTD
#define ws2812_DDRREG   DDRD
#define ws2812_pin PD5
#define RGBLED_NUM 3     // Number of LEDs

/* fix space cadet rollover issue */
#define DISABLE_SPACE_CADET_ROLLOVER

/* NKRO */
#ifndef FORCE_NKRO
    #define FORCE_NKRO  // Depends on NKRO_ENABLE.
#endif

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
// #define NO_DEBUG

/* disable print */
// #define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
//#define DEBUG_MATRIX_SCAN_RATE
