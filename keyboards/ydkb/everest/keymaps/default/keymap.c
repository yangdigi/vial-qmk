#include QMK_KEYBOARD_H

// keymap sample for test. Some keys(PSCR, HOME, END) are for Everest.
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT( 
	KC_ESC,  KC_1,   KC_2,   KC_3,   KC_4,   KC_5,   KC_6,     KC_7,   KC_8,   KC_9,   KC_0,   KC_MINS,KC_EQL, KC_GRV, KC_BSPC, KC_PSCR,
	KC_TAB,  KC_Q,   KC_W,   KC_E,   KC_R,   KC_T,     KC_Y,   KC_U,   KC_I,   KC_O,   KC_P,   KC_LBRC,KC_RBRC,KC_BSLS,         KC_HOME,
	KC_CAPS, KC_A,   KC_S,   KC_D,   KC_F,   KC_G,     KC_H,   KC_J,   KC_K,   KC_L,   KC_SCLN,KC_QUOT,        KC_ENT,          KC_END,
	KC_LSFT,         KC_Z,   KC_X,   KC_C,   KC_V,   KC_B,     KC_N,   KC_M,   KC_COMM,KC_DOT, KC_SLSH,KC_RSFT,KC_UP,  
	KC_LCTL,         KC_LALT,        KC_SPC, KC_Del,           KC_B,   KC_SPC,         KC_RALT,KC_LEFT,KC_DOWN,KC_RGHT
	),
};
