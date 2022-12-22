#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define FW_VER          QMK_DMCM
#define FW_VER_VIA      VIA_DMCM
#define FW_VER_VIAL     VIAL_DMCM
#define VENDOR_ID       0x9D5B 
#define PRODUCT_ID      0x2110
#define DEVICE_VER      0x0001
#define MANUFACTURER    YDKB_Xikii
#define PRODUCT         Sasuke No.1 (FW_VER)

/* key matrix size */
#define MATRIX_ROWS 5  //595 num of each side.
#define MATRIX_COLS 8






/* key combination for command */
#define IS_COMMAND() ( \
    get_mods() == (MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT)) \
)

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
