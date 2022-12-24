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
    { KC_I,   KC_LEFT,KC_N,   KC_X,   KC_LCTL,KC_ESC },
    { KC_U,   KC_SPC, KC_H,   KC_S,   KC_LSFT,KC_TAB },
    { KC_Y,   KC_NO,  KC_J,   KC_D,   KC_Z,   KC_A   },
    { KC_O,   KC_UP,  KC_M,   KC_C,   KC_LGUI,KC_Q   },
    { KC_SCLN,KC_SLSH,KC_K,   KC_F,   KC_NO,  KC_T   },
    { KC_P,   KC_DOWN,KC_COMM,KC_V,   KC_LALT,KC_W   },
    { KC_ENT, KC_RSFT,KC_L,   KC_G,   KC_RALT,KC_R   },
    { KC_BSPC,KC_RGHT,KC_DOT, KC_B,   KC_APP, KC_E   }
    },
  [1] = {
    { KC_I,   KC_LEFT,KC_N,   KC_X,   KC_LCTL,KC_ESC },
    { KC_U,   KC_SPC, KC_H,   KC_S,   KC_LSFT,KC_TAB },
    { KC_Y,   KC_NO,  KC_J,   KC_D,   KC_Z,   KC_A   },
    { KC_O,   KC_UP,  KC_M,   KC_C,   KC_LGUI,KC_Q   },
    { KC_SCLN,KC_SLSH,KC_K,   KC_F,   KC_NO,  KC_T   },
    { KC_P,   KC_DOWN,KC_COMM,KC_V,   KC_LALT,KC_W   },
    { KC_ENT, KC_RSFT,KC_L,   KC_G,   KC_RALT,KC_R   },
    { KC_BSPC,KC_RGHT,KC_DOT, KC_B,   KC_APP, KC_E   }
    },
};
