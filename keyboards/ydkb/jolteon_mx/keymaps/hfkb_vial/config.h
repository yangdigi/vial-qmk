#pragma once
#undef  PRODUCT
#if CONSOLE_ENABLE
#define PRODUCT    Jolteon #HFKB Debug (FW_VER)
#else
#define PRODUCT    Jolteon #HFKB (FW_VER)
#endif

#undef  MATRIX_ROWS
#define MATRIX_ROWS 5
#undef  MATRIX_COLS
#define MATRIX_COLS 14
#define MATRIX_KEYS 62

#define APC_KEY_ROW 4
#define APC_KEY_COL 3

#define DYNAMIC_KEYMAP_LAYER_COUNT 5
#define VIAL_KEYBOARD_UID {0x2E, 0xE6, 0x0E, 0x23, 0x34, 0xEF, 0x99, 0x37}
