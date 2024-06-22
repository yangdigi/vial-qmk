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

const uint8_t PROGMEM keymaps8[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = {
    {KC_GESC, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_BSPACE, KC_MUTE}, 
    {KC_TAB, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCOLON, KC_ENTER}, 
    {KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH, KC_RSHIFT}, 
    {KC_MS_L, KC_LCTRL, KC_LALT, KC_LGUI, KC_MS_R, KC_DEL, KC_VOLD, KC_P, KC_RGUI, KC_RALT, KC_RCTRL, KC_VOLU}
  }, 
};
