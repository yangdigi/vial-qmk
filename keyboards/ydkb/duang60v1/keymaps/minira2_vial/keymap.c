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
    { KC_ESC, KC_4,   KC_3,   KC_2,   KC_5,   KC_6,   KC_1,   KC_F13,      KC_MINS,KC_7,   KC_8,   KC_GRV, KC_9,   KC_0,   KC_EQL ,KC_F15, },
    { KC_TAB, KC_R,   KC_E,   KC_W,   KC_T,   KC_Y,   KC_Q,   KC_F14,      KC_P,   KC_BSPC,KC_U,   KC_RBRC,KC_I,   KC_O,   KC_LBRC,KC_F16, },
    { KC_CAPS,KC_F,   KC_D,   KC_S,   KC_G,   KC_H,   KC_A,   KC_NO,       KC_SCLN,KC_ENT, KC_J,   KC_BSLS,KC_K,   KC_L,   KC_QUOT,KC_NO,  },
    { KC_LSFT,KC_C,   KC_X,   KC_Z,   KC_V,   KC_B,   KC_NUBS,KC_NO,       KC_SLSH,KC_DEL, KC_M,   KC_UP,  KC_COMM,KC_DOT, KC_RSFT,KC_NO,  },
    { KC_LCTL,KC_NO,  KC_NO,  KC_LALT,KC_APP, KC_N,   KC_LGUI,KC_NO,       KC_RCTL,KC_RGHT,KC_SPC, KC_DOWN,KC_RGUI,KC_RALT,KC_LEFT,KC_NO,  }
	}, 
};
