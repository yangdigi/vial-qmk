#include QMK_KEYBOARD_H

enum user_keycode {
    BT_USB = USER00, 
    KB_RESET, 
    BATT_LEVEL, 
    LOCK_MODE, 
    RGB_Toogle, 
    RGB_Mode_DN, 
    RGB_Mode_UP, 
    RGB_HUE_DN, 
    RGB_HUE_UP, 
    RGB_SAT_DN, 
    RGB_SAT_UP, 
    RGB_LUM_DN, 
    RGB_LUM_UP, 
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = {
    {KC_0, KC_EQUAL, KC_2, KC_BSPACE, KC_INSERT, KC_8, KC_9, KC_MINUS},
    {KC_P, KC_RBRACKET, KC_NO, KC_BSLASH, KC_DELETE, KC_I, KC_O, KC_LBRACKET},
    {KC_SCOLON, KC_QUOTE, KC_ENTER, KC_NO, KC_NO, KC_K, KC_L, KC_NO},
    {KC_DOT, KC_SLASH, KC_RSHIFT, KC_UP, KC_V, KC_M, KC_COMMA, KC_NO},
    {KC_NO, MO(1), KC_RCTRL, KC_LEFT, KC_DOWN, KC_RIGHT, KC_NO, KC_RALT},
    {KC_LCTRL, KC_LALT, KC_NO, KC_NO, KC_COMMA, KC_NO, KC_SPACE, KC_LGUI},
    {KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_NONUS_BSLASH},
    {KC_CAPSLOCK, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_A},
    {KC_TAB, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_Q},
    {KC_GESC, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_1}
  }, 
  [1] = {
    {KC_F10, KC_F12, KC_F2, KC_DELETE, KC_TRNS, KC_F8, KC_F9, KC_F11},
    {KC_PAUSE, KC_TRNS, KC_NO, KC_TRNS, KC_TRNS, KC_PSCREEN, KC_SCROLLLOCK, KC_TRNS},
    {KC_TRNS, KC_TRNS, KC_TRNS, KC_NO, KC_NO, KC_HOME, KC_PGUP, KC_NO},
    {KC_PGDOWN, KC_TRNS, KC_TRNS, KC_PGUP, KC_TRNS, KC_TRNS, KC_END, KC_NO},
    {KC_NO, KC_TRNS, KC_TRNS, KC_HOME, KC_PGDOWN, KC_END, KC_NO, KC_TRNS},
    {KC_TRNS, KC_TRNS, KC_NO, KC_NO, KC_END, KC_NO, KC_TRNS, KC_TRNS},
    {KC_TRNS, USER03, KC_TRNS, KC_TRNS, KC_TRNS, USER01, KC_TRNS, KC_NO},
    {KC_TRNS, KC_VOLU, KC_MUTE, USER08, USER10, USER12, KC_TRNS, KC_VOLD},
    {USER00, KC_TRNS, USER02, USER04, USER05, KC_TRNS, KC_TRNS, KC_TRNS},
    {KC_GRAVE, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F1}
  }, 

};
