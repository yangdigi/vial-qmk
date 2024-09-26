#include QMK_KEYBOARD_H

#ifdef FLASH_KEYMAP_COUNT
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  {
    {KC_ESCAPE, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINUS, KC_EQUAL, KC_BSLASH},
    {KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRACKET, KC_RBRACKET, KC_BSPACE},
    {KC_LCTRL, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCOLON, KC_QUOTE, KC_ENTER, KC_GRAVE},
    {KC_LSHIFT, KC_NO, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH, KC_RSHIFT, MO(1)},
    {KC_NO, KC_LALT, KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_SPACE, KC_NO, KC_NO, KC_NO, KC_RGUI, KC_RALT, KC_NO}
  },
  {
    {KC_GRAVE, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_INSERT},
    {KC_CAPSLOCK, USER00, KC_TRNS, USER02, USER04, USER06, KC_TRNS, KC_TRNS, KC_PSCREEN, KC_SCROLLLOCK, KC_PAUSE, KC_UP, KC_TRNS, KC_TRNS},
    {KC_TRNS, KC_VOLD, KC_VOLU, KC_MUTE, USER08, USER10, USER12, KC_TRNS, KC_HOME, KC_PGUP, KC_LEFT, KC_RIGHT, KC_TRNS, KC_DELETE},
    {KC_TRNS, KC_NO, USER03, KC_TRNS, KC_CALC, KC_TRNS, USER01, KC_TRNS, KC_TRNS, KC_END, KC_PGDOWN, KC_DOWN, KC_VOLU, KC_TRNS},
    {KC_NO, KC_TRNS, KC_TRNS, KC_NO, KC_NO, KC_NO, KC_NO, KC_TRNS, KC_NO, KC_NO, KC_NO, KC_MUTE, KC_VOLD, KC_NO}
  }
};
#else
const uint8_t PROGMEM keymaps8[][MATRIX_ROWS][MATRIX_COLS] =
{
  {
    {KC_ESCAPE, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINUS, KC_EQUAL, KC_BSLASH},
    {KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRACKET, KC_RBRACKET, KC_BSPACE},
    {KC_LCTRL, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCOLON, KC_QUOTE, KC_ENTER, KC_GRAVE},
    {KC_LSHIFT, KC_NO, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH, KC_RSHIFT, KC_DEL},
    {KC_NO, KC_LALT, KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_SPACE, KC_NO, KC_NO, KC_NO, KC_RGUI, KC_RALT, KC_NO}
  }
};

#endif


