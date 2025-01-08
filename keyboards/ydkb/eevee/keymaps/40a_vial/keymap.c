#include QMK_KEYBOARD_H

#if 0
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {LAYOUT_all(KC_NO)};
#else
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = 
{
  {
    {KC_GESC, KC_TAB, KC_LSHIFT, KC_Z, KC_P, KC_BSPACE, KC_ENTER, KC_RSHIFT},
    {KC_Q, KC_A, KC_Z, KC_LCTRL, KC_NO, KC_NO, KC_NO, KC_NO},
    {KC_W, KC_S, KC_X, KC_LALT, KC_O, KC_L, KC_DOT, KC_RCTRL},
    {KC_E, KC_D, KC_C, KC_X, KC_I, KC_K, KC_COMMA, KC_RALT},
    {KC_R, KC_F, KC_V, KC_NO, KC_U, KC_J, KC_M, KC_SPACE},
    {KC_T, KC_G, KC_B, KC_SPACE, KC_Y, KC_H, KC_N, KC_NO}
  }
};

#endif


