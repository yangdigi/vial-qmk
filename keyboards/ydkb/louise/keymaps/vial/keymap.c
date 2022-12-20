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
    { KC_ESC, KC_E,   KC_W,   KC_Q,   KC_R,   KC_T,   KC_NO,   KC_NO,       KC_P,   KC_Y,   KC_U,   KC_BSPC,KC_I,   KC_O,   KC_BSLS,KC_NO,  },
    { KC_CAPS,KC_D,   KC_S,   KC_A,   KC_F,   KC_G,   KC_NO,   KC_NO,       KC_SCLN,KC_NO,  KC_H,   KC_ENT, KC_J,   KC_K,   KC_L,   KC_NO,  },
    { KC_LSFT,KC_C,   KC_X,   KC_Z,   KC_V,   KC_B,   KC_NO,   KC_NO,       KC_DOT, KC_SLSH,KC_N,   KC_RSFT,KC_M,   KC_COMM,KC_UP,  KC_NO,  },
    { KC_LCTL,KC_LALT,KC_NO,  KC_LGUI,KC_NO,  KC_APP, KC_NO,   KC_NO,       KC_DOWN,KC_NO,  KC_SPC, KC_RGHT,KC_NO,  KC_RALT,KC_LEFT,KC_NO,  }
	},
};
