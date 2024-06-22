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
    { KC_ESC, KC_Q,   KC_W,   KC_E,    KC_R,   KC_T,   KC_Y,   KC_U,   KC_I,   KC_O,   KC_BSPC },
    { KC_RCTL,KC_A,   KC_S,   KC_D,    KC_F,   KC_G,   KC_H,   KC_J,   KC_K,   KC_L,   KC_ENT  },
    { KC_LSFT,KC_Z,   KC_X,   KC_C,    KC_V,   KC_B,   KC_N,   KC_M,   KC_COMM,KC_DOT, KC_RSFT },
    { KC_LCTL,KC_LGUI,KC_LALT,KC_RALT, KC_APP, KC_NO,  KC_SPC, KC_P,   KC_LEFT,KC_DOWN,KC_RGHT }
	},
};
