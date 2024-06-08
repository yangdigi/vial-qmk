#pragma once
#undef  PRODUCT_ID
#define PRODUCT_ID    0x2306

#undef  PRODUCT
#if CONSOLE_ENABLE
#define PRODUCT    Jolteon #PAVLOV_MX Debug (FW_VER)
#else
#define PRODUCT    PAVLOV MX (FW_VER)
#endif

#undef  MATRIX_ROWS
#define MATRIX_ROWS 5
#undef  MATRIX_COLS
#define MATRIX_COLS 15
#define MATRIX_KEYS 67

#define APC_KEY_ROW 4
#define APC_KEY_COL 3
#undef  EPC_LV
#define EPC_LV KC_P0


#define DYNAMIC_KEYMAP_LAYER_COUNT 5
#define FLASH_KEYMAP_COUNT 2
#define VIAL_KEYBOARD_UID {0x2E, 0xE6, 0x0E, 0x23, 0x34, 0xEF, 0x99, 0x37}
