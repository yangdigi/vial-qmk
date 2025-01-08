#pragma once
#undef  PRODUCT_ID
#define PRODUCT_ID    0x23CB

#undef  PRODUCT
#define PRODUCT    HFKB (FW_VER)

#undef  MATRIX_ROWS
#define MATRIX_ROWS 5
#undef  MATRIX_COLS
#define MATRIX_COLS 14
#define MATRIX_KEYS 69

#define APC_ENABLE

#undef  BLE_NAME
#define BLE_NAME "HFKB BLE"

#define DYNAMIC_KEYMAP_LAYER_COUNT 4
//#define FLASH_KEYMAP_COUNT 2
#define FLASH_KEYMAP8_COUNT 1
#define VIAL_KEYBOARD_UID {0x2E, 0xE6, 0x0E, 0x23, 0x34, 0xEF, 0x99, 0x37}
